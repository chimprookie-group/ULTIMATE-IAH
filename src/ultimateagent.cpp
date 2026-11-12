#include "ultimateagent.h"

#include "agentmanager.h"
#include "backend.h"
#include "connectiondoctor.h"
#include "harnessconnections.h"
#include "installationdoctor.h"
#include "mausbotmanager.h"
#include "mcpmanager.h"
#include "memorystore.h"
#include "missionrunner.h"
#include "nineroutermanager.h"
#include "skillmanager.h"
#include "terminalsession.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

namespace {

QString compactJson(const QJsonObject &value) {
    return QString::fromUtf8(QJsonDocument(value).toJson(QJsonDocument::Compact));
}

QJsonObject okResult(QJsonObject data = {}) {
    data.insert(QStringLiteral("ok"), true);
    return data;
}

QJsonObject errorResult(const QString &error) {
    return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}};
}

QString truncateText(const QString &text, int limit) {
    return text.size() <= limit ? text : text.left(limit) + QStringLiteral("\n…");
}

QJsonObject properties(std::initializer_list<QPair<QString, QJsonObject>> values) {
    QJsonObject out;
    for (const auto &value : values)
        out.insert(value.first, value.second);
    return out;
}

QJsonObject agentProperty(const QString &type, const QString &description) {
    return QJsonObject{{QStringLiteral("type"), type},
                       {QStringLiteral("description"), description}};
}

QJsonObject enumProperty(const QString &description, const QStringList &values) {
    QJsonObject prop = agentProperty(QStringLiteral("string"), description);
    prop.insert(QStringLiteral("enum"), QJsonArray::fromStringList(values));
    return prop;
}

QJsonObject tool(const QString &name, const QString &description,
                 const QJsonObject &props, const QStringList &required = {}) {
    return QJsonObject{
        {QStringLiteral("type"), QStringLiteral("function")},
        {QStringLiteral("function"),
         QJsonObject{{QStringLiteral("name"), name},
                     {QStringLiteral("description"), description},
                     {QStringLiteral("parameters"),
                      QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
                                  {QStringLiteral("properties"), props},
                                  {QStringLiteral("required"), QJsonArray::fromStringList(required)}}}}}};
}

} // namespace

UltimateAgent::UltimateAgent(Backend *backend, HarnessConnections *connections,
                             NineRouterManager *nineRouter, MausBotManager *mausBot,
                             McpManager *mcp, MemoryStore *memory, SkillManager *skills,
                             AgentManager *agents, MissionRunner *missions,
                             TerminalSession *terminal, InstallationDoctor *installation,
                             ConnectionDoctor *connectionDoctor, QObject *parent)
    : QObject(parent), m_backend(backend), m_connections(connections),
      m_nineRouter(nineRouter), m_mausBot(mausBot), m_mcp(mcp), m_memory(memory),
      m_skills(skills), m_agents(agents), m_missions(missions), m_terminal(terminal),
      m_installation(installation), m_connectionDoctor(connectionDoctor),
      m_network(new QNetworkAccessManager(this)), m_timeout(new QTimer(this)) {
    m_timeout->setSingleShot(true);
    m_timeout->setInterval(120000);
    QObject::connect(m_timeout, &QTimer::timeout, this, [this] {
        if (!m_reply)
            return;
        m_reply->abort();
        finishWithError(tr("Le modèle n'a pas répondu dans les 2 minutes."));
    });
    m_status = tr("Prêt");
    load();
    refreshEndpoint();
}

void UltimateAgent::refreshEndpoint() {
    m_endpoint = resolveEndpoint();
    const QString model = m_endpoint.valid() ? m_endpoint.model : tr("Non configuré");
    const QString label = m_endpoint.valid() ? m_endpoint.label : tr("Aucun provider");
    if (model == m_modelLabel && label == m_endpointLabel)
        return;
    m_modelLabel = model;
    m_endpointLabel = label;
    emit endpointChanged();
}

QString UltimateAgent::credentialsPath() const {
    return qEnvironmentVariable("DSH_HOME", QDir::homePath() + QStringLiteral("/.dsh"))
        + QStringLiteral("/.credentials.yaml");
}

QString UltimateAgent::credential(const QString &name) const {
    QFile file(credentialsPath());
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QString text = QString::fromUtf8(file.readAll());
    const QRegularExpression expression(
        QStringLiteral("(?m)^\\s*%1:\\s*(\\S+)").arg(QRegularExpression::escape(name)));
    const auto match = expression.match(text);
    if (!match.hasMatch())
        return {};
    QString value = match.captured(1).trimmed();
    value.remove(QRegularExpression(QStringLiteral("^[\\\"']|[\\\"']$")));
    return value;
}

UltimateAgent::Endpoint UltimateAgent::resolveEndpoint() const {
    const QString deepSeek = credential(QStringLiteral("DEEPSEEK_API_KEY"));
    if (!deepSeek.isEmpty()) {
        return {QStringLiteral("https://api.deepseek.com/v1"), deepSeek,
                QStringLiteral("deepseek-chat"), QStringLiteral("DeepSeek Chat"),
                QStringLiteral("DeepSeek API")};
    }
    if (m_nineRouter && m_nineRouter->running()) {
        const QString routerKey = credential(QStringLiteral("NINE_ROUTER_API_KEY"));
        if (!routerKey.isEmpty()) {
            return {QStringLiteral("http://127.0.0.1:20128/v1"), routerKey,
                    QStringLiteral("ds/deepseek-chat"), QStringLiteral("9Router · DeepSeek Chat"),
                    QStringLiteral("9Router local")};
        }
    }
    return {};
}

QString UltimateAgent::systemPrompt() const {
    return staticKnowledge() + QStringLiteral("\n\nLIVE SNAPSHOT\n")
        + QString::fromUtf8(QJsonDocument(snapshot()).toJson(QJsonDocument::Compact))
        + QStringLiteral("\n\nTOOLS\n")
        + QString::fromUtf8(QJsonDocument(tools()).toJson(QJsonDocument::Compact));
}

QJsonObject UltimateAgent::snapshot() const {
    QJsonObject root;
    root.insert(QStringLiteral("app"), QStringLiteral("Ultimate HAI"));
    root.insert(QStringLiteral("version"), m_backend ? m_backend->appVersion() : QString());
    root.insert(QStringLiteral("workspace"), m_backend ? m_backend->workspace() : QString());
    root.insert(QStringLiteral("view"), m_backend ? m_backend->view() : QString());
    root.insert(QStringLiteral("theme"), m_backend ? m_backend->activeTheme() : QString());
    root.insert(QStringLiteral("language"), m_backend ? m_backend->languagePreference() : QString());
    if (m_backend) {
        root.insert(QStringLiteral("git"),
                    QJsonObject{{QStringLiteral("available"), m_backend->gitAvailable()},
                                {QStringLiteral("branch"), m_backend->gitBranch()},
                                {QStringLiteral("dirty"), m_backend->gitDirtyCount()}});
        root.insert(QStringLiteral("harness"),
                    QJsonObject{{QStringLiteral("state"), m_backend->harnessState()},
                                {QStringLiteral("url"), m_backend->harnessUrl()},
                                {QStringLiteral("detail"), m_backend->harnessDetail()},
                                {QStringLiteral("dsh"), m_backend->dshBinary()}});
    }
    if (m_connections) {
        QJsonArray providers;
        const QVariantList list = m_connections->providers();
        for (const QVariant &value : list) {
            const QVariantMap map = value.toMap();
            providers.append(QJsonObject{{QStringLiteral("id"), map.value(QStringLiteral("id")).toString()},
                                         {QStringLiteral("name"), map.value(QStringLiteral("name")).toString()},
                                         {QStringLiteral("models"), QJsonValue::fromVariant(map.value(QStringLiteral("models")))}});
        }
        root.insert(QStringLiteral("connections"),
                    QJsonObject{{QStringLiteral("providers"), providers},
                                {QStringLiteral("defaultModel"), m_connections->defaultModel()},
                                {QStringLiteral("deepseekConfigured"), m_connections->deepseekConfigured()},
                                {QStringLiteral("envPath"), m_connections->envPath()}});
    }
    if (m_nineRouter) {
        root.insert(QStringLiteral("nineRouter"),
                    QJsonObject{{QStringLiteral("installed"), m_nineRouter->installed()},
                                {QStringLiteral("running"), m_nineRouter->running()},
                                {QStringLiteral("authRequired"), m_nineRouter->authRequired()},
                                {QStringLiteral("keyMasked"), m_nineRouter->keyMasked()},
                                {QStringLiteral("models"), m_nineRouter->models().size()},
                                {QStringLiteral("detail"), m_nineRouter->detail()},
                                {QStringLiteral("version"), m_nineRouter->version()}});
    }
    if (m_mausBot) {
        root.insert(QStringLiteral("mausBot"),
                    QJsonObject{{QStringLiteral("installed"), m_mausBot->installed()},
                                {QStringLiteral("running"), m_mausBot->running()},
                                {QStringLiteral("mcpHarness"), m_mausBot->mcpHarness()},
                                {QStringLiteral("pairing"), m_mausBot->pairing()},
                                {QStringLiteral("detail"), m_mausBot->detail()}});
    }
    if (m_mcp) {
        QJsonArray servers;
        for (const QVariant &value : m_mcp->servers()) {
            const QVariantMap map = value.toMap();
            servers.append(QJsonObject{{QStringLiteral("id"), map.value(QStringLiteral("id")).toString()},
                                       {QStringLiteral("name"), map.value(QStringLiteral("name")).toString()},
                                       {QStringLiteral("transport"), map.value(QStringLiteral("transport")).toString()},
                                       {QStringLiteral("enabled"), map.value(QStringLiteral("enabled")).toBool()}});
        }
        root.insert(QStringLiteral("mcp"),
                    QJsonObject{{QStringLiteral("customServers"), servers},
                                {QStringLiteral("builtins"), m_mcp->builtins().size()}});
    }
    if (m_memory) {
        root.insert(QStringLiteral("memory"), QJsonObject::fromVariantMap(m_memory->stats()));
        root.insert(QStringLiteral("memoryPath"), m_memory->databasePath());
    }
    if (m_skills) {
        QJsonArray skills;
        for (const QVariant &value : m_skills->skills()) {
            const QVariantMap map = value.toMap();
            skills.append(QJsonObject{{QStringLiteral("name"), map.value(QStringLiteral("name")).toString()},
                                      {QStringLiteral("description"), map.value(QStringLiteral("description")).toString()},
                                      {QStringLiteral("path"), map.value(QStringLiteral("path")).toString()}});
        }
        root.insert(QStringLiteral("skills"), skills);
    }
    if (m_agents) {
        QJsonArray profiles;
        for (const QVariant &value : m_agents->profiles()) {
            const QVariantMap map = value.toMap();
            profiles.append(QJsonObject{{QStringLiteral("id"), map.value(QStringLiteral("id")).toString()},
                                        {QStringLiteral("name"), map.value(QStringLiteral("name")).toString()},
                                        {QStringLiteral("role"), map.value(QStringLiteral("role")).toString()}});
        }
        root.insert(QStringLiteral("agentProfiles"), profiles);
    }
    root.insert(QStringLiteral("views"), QJsonArray::fromStringList(views()));
    return root;
}

void UltimateAgent::sendMessage(const QString &text) {
    if (m_busy)
        return;
    const QString prompt = text.trimmed();
    if (prompt.isEmpty())
        return;
    refreshEndpoint();
    if (!m_endpoint.valid()) {
        setError(tr("Aucun provider utilisable. Ajoutez une clé DeepSeek dans Connexions."));
        return;
    }
    m_error.clear();
    emit errorChanged();
    m_round = 0;
    appendUser(prompt);
    m_api.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                             {QStringLiteral("content"), prompt}});
    setBusy(true);
    setStatus(tr("Réflexion…"));
    save();
    runRequest();
}

void UltimateAgent::runRequest() {
    if (!m_endpoint.valid()) {
        finishWithError(tr("Aucun provider utilisable."));
        return;
    }
    if (m_round >= 8) {
        finishWithError(tr("L'agent a dépassé la limite de 8 tours d'outils."));
        return;
    }
    ++m_round;

    QJsonArray messages;
    messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("system")},
                                {QStringLiteral("content"), systemPrompt()}});
    for (const QJsonValue &message : m_api)
        messages.append(message);

    QJsonObject body{
        {QStringLiteral("model"), m_endpoint.model},
        {QStringLiteral("messages"), messages},
        {QStringLiteral("tools"), tools()},
        {QStringLiteral("tool_choice"), QStringLiteral("auto")},
        {QStringLiteral("temperature"), 0.2},
        {QStringLiteral("max_tokens"), 4096},
        {QStringLiteral("stream"), false}};

    QString base = m_endpoint.baseUrl;
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    QNetworkRequest request(QUrl(base + QStringLiteral("/chat/completions")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", "Bearer " + m_endpoint.apiKey.toUtf8());
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(120000);

    m_reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    QObject::connect(m_reply, &QNetworkReply::finished, this, &UltimateAgent::handleReply);
    m_timeout->start();
    setStatus(m_round == 1 ? tr("Réflexion…") : tr("Tour %1…").arg(m_round));
}

void UltimateAgent::handleReply() {
    QNetworkReply *reply = m_reply;
    if (!reply)
        return;
    m_reply = nullptr;
    m_timeout->stop();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray payload = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError && httpStatus == 0) {
        finishWithError(tr("Connexion au modèle impossible : %1").arg(reply->errorString()));
        return;
    }
    const QJsonObject root = QJsonDocument::fromJson(payload).object();
    if (httpStatus < 200 || httpStatus >= 300) {
        const QString detail = root.value(QStringLiteral("error")).toObject()
                                   .value(QStringLiteral("message")).toString();
        finishWithError(tr("Le modèle a renvoyé HTTP %1 : %2")
                            .arg(httpStatus)
                            .arg(detail.isEmpty() ? QString::fromUtf8(payload).left(400) : detail));
        return;
    }
    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        finishWithError(tr("Réponse du modèle vide."));
        return;
    }
    const QJsonObject message = choices.first().toObject().value(QStringLiteral("message")).toObject();
    const QJsonArray calls = message.value(QStringLiteral("tool_calls")).toArray();
    if (!calls.isEmpty()) {
        QJsonObject assistant{
            {QStringLiteral("role"), QStringLiteral("assistant")},
            {QStringLiteral("content"), message.value(QStringLiteral("content")).toString()},
            {QStringLiteral("tool_calls"), calls}};
        m_api.append(assistant);
        for (const QJsonValue &value : calls) {
            const QJsonObject call = value.toObject();
            const QString id = call.value(QStringLiteral("id")).toString();
            const QJsonObject function = call.value(QStringLiteral("function")).toObject();
            const QString name = function.value(QStringLiteral("name")).toString();
            const QString argumentsText = function.value(QStringLiteral("arguments")).toString();
            QJsonObject arguments;
            if (!argumentsText.trimmed().isEmpty())
                arguments = QJsonDocument::fromJson(argumentsText.toUtf8()).object();
            setStatus(tr("Outil : %1").arg(name));
            const QJsonObject result = executeTool(name, arguments);
            const QString resultText = truncateText(compactJson(result), 12000);
            m_api.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("tool")},
                                     {QStringLiteral("tool_call_id"), id},
                                     {QStringLiteral("name"), name},
                                     {QStringLiteral("content"), resultText}});
            appendTool(name, toolSummary(name, arguments, result),
                       result.value(QStringLiteral("ok")).toBool(),
                       resultText);
        }
        save();
        QTimer::singleShot(0, this, [this] { runRequest(); });
        return;
    }

    const QString content = message.value(QStringLiteral("content")).toString().trimmed();
    if (content.isEmpty()) {
        finishWithError(tr("Le modèle n'a renvoyé aucun texte."));
        return;
    }
    m_api.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("assistant")},
                             {QStringLiteral("content"), content}});
    appendAssistant(content);
    setBusy(false);
    setStatus(tr("Prêt"));
    save();
}

void UltimateAgent::finishWithError(const QString &message) {
    abortReply();
    setError(message);
    setBusy(false);
    setStatus(tr("Erreur"));
    save();
}

void UltimateAgent::abortReply() {
    m_timeout->stop();
    if (m_reply) {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
}

void UltimateAgent::cancel() {
    if (!m_busy)
        return;
    abortReply();
    setBusy(false);
    setStatus(tr("Annulé"));
    setError(tr("Réponse annulée."));
    save();
}

void UltimateAgent::toggleToolDetails(int index) {
    if (index < 0 || index >= m_ui.size())
        return;
    QVariantMap message = m_ui.at(index).toMap();
    if (message.value(QStringLiteral("role")).toString() != QLatin1String("tool"))
        return;
    message.insert(QStringLiteral("expanded"), !message.value(QStringLiteral("expanded")).toBool());
    m_ui[index] = message;
    emit messagesChanged();
}

void UltimateAgent::clearConversation() {
    if (m_busy)
        cancel();
    m_api = QJsonArray();
    m_ui.clear();
    m_title.clear();
    m_error.clear();
    setStatus(tr("Prêt"));
    emit messagesChanged();
    emit errorChanged();
    save();
}

void UltimateAgent::appendUser(const QString &text) {
    if (m_title.isEmpty())
        m_title = text.simplified().left(56);
    m_ui.append(QVariantMap{{QStringLiteral("role"), QStringLiteral("user")},
                            {QStringLiteral("text"), text},
                            {QStringLiteral("time"), QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"))}});
    emit messagesChanged();
}

void UltimateAgent::appendAssistant(const QString &text) {
    m_ui.append(QVariantMap{{QStringLiteral("role"), QStringLiteral("assistant")},
                            {QStringLiteral("text"), text},
                            {QStringLiteral("time"), QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"))}});
    emit messagesChanged();
}

void UltimateAgent::appendTool(const QString &name, const QString &detail, bool ok,
                               const QString &payload) {
    m_ui.append(QVariantMap{{QStringLiteral("role"), QStringLiteral("tool")},
                            {QStringLiteral("tool"), name},
                            {QStringLiteral("detail"), detail},
                            {QStringLiteral("ok"), ok},
                            {QStringLiteral("payload"), payload},
                            {QStringLiteral("expanded"), false},
                            {QStringLiteral("time"), QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"))}});
    emit messagesChanged();
}

void UltimateAgent::setBusy(bool busy) {
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void UltimateAgent::setStatus(const QString &status) {
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void UltimateAgent::setError(const QString &error) {
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

void UltimateAgent::save() {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/ultimate-agent.json");
    QDir().mkpath(QFileInfo(path).absolutePath());
    QJsonObject root{{QStringLiteral("title"), m_title},
                     {QStringLiteral("api"), m_api},
                     {QStringLiteral("ui"), QJsonArray::fromVariantList(m_ui)}};
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.commit();
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
}

void UltimateAgent::load() {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/ultimate-agent.json");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    m_title = root.value(QStringLiteral("title")).toString();
    m_api = root.value(QStringLiteral("api")).toArray();
    m_ui = root.value(QStringLiteral("ui")).toArray().toVariantList();
}

QStringList UltimateAgent::views() {
    return {QStringLiteral("agent"), QStringLiteral("home"), QStringLiteral("ecosystem"),
            QStringLiteral("workspace"), QStringLiteral("harness"), QStringLiteral("terminal"),
            QStringLiteral("connects"), QStringLiteral("connections"), QStringLiteral("memory"),
            QStringLiteral("autopilot"), QStringLiteral("mcp"), QStringLiteral("agents"),
            QStringLiteral("installation"), QStringLiteral("ninerouter"),
            QStringLiteral("mausbot"), QStringLiteral("settings")};
}

QString UltimateAgent::staticKnowledge() {
    QString behavior;
    QFile behaviorFile(QStringLiteral(":/agent/BEHAVIOR.md"));
    if (behaviorFile.open(QIODevice::ReadOnly))
        behavior = QString::fromUtf8(behaviorFile.readAll());
    return QStringLiteral(R"PROMPT(You are Ultimate Agent, the built-in operator and expert of Ultimate HAI, an Omarchy desktop cockpit for the DeepSeek Harness (DSH).

IDENTITY AND MISSION
- You are not a generic chatbot. You know this exact application, its local ecosystem, its files and its live state.
- You can operate the application through tools: navigation, providers/connections, DSH harness, 9Router, MausBot, MCP, persistent memory, skills, missions, terminal, workspace files, Git, installation and settings.
- Answer in the user's language (French by default) and keep the tone direct, technical and useful.
- Be concise by default, but be complete when diagnosing or changing configuration. Use Markdown and short code blocks.

SYSTEM MAP
- App: Qt 6/QML, C++ backend. Data: ~/.config/Crasy/deepcrasy-h and ~/.local/share/Crasy/deepcrasy-h.
- DSH runtime: /usr/lib/crasy/dsh/node_modules/.bin/dsh (fallback to dsh in PATH or a local install).
- Harness: dsh web, patched with openmausbot.patch.yml, connections.patch.yml, mcp-memory.patch.yml and mcp-servers.patch.yml.
- Providers: connections.patch.yml + ~/.dsh/.credentials.yaml. DeepSeek key is the single source, and it is propagated to agent.env and 9Router.
- 9Router: local service on 127.0.0.1:20128. MausBot: local service on 127.0.0.1:8799.
- Memory: SQLite/FTS5 via the memory MCP server. Skills: ~/.agents/skills and project skills.
- Views: agent, home, ecosystem, workspace, harness, terminal, connections, memory, autopilot, mcp, agents, installation, ninerouter, mausbot, settings.
- Shortcuts: Ctrl+0 Cockpit, Ctrl+1 Harness, Ctrl+2 Terminal, Ctrl+4 Settings, Ctrl+6 Project, Ctrl+7 Files, Ctrl+8 Connections, Ctrl+9 9Router, Ctrl+Alt+C MCP, Ctrl+Alt+G Agents, Ctrl+Alt+I Installation, Ctrl+Alt+M Memory, Ctrl+Alt+A Autopilot, Ctrl+Alt+U Ultimate Agent, Ctrl+Shift+M MausBot, Ctrl+K palette.

ACTION RULES
- Use tools for actions and facts. Never claim a command, configuration change or service start succeeded without a successful tool result.
- Before acting on project work, search persistent memory with the memory tool. After a durable decision, remember it if the user asked or it is clearly useful.
- Never print, repeat or persist API keys, tokens or passwords. When the user supplies a key, pass it directly to the appropriate tool and report only a masked confirmation.
- Destructive tools require {"confirm": true}. Set confirm only when the user explicitly asked for that exact destructive action. Otherwise explain what you will do and ask first.
- For file writes and terminal commands, stay in the active workspace, explain destructive intent, and verify the result.
- For provider/service changes, inspect the status first, make the smallest change, then verify it.
- If a tool fails, state the real error and propose one concrete next step. Do not loop blindly.
- For app help, use the live snapshot and the actual tool result; do not invent menu paths.

TOOL STRATEGY
- Start with ultimate_status for any state question.
- Use ultimate_diagnose for connection/installation problems.
- Use ultimate_connections for DeepSeek/provider/default/restart.
- Use ultimate_services for 9Router, MausBot and harness lifecycle.
- Use ultimate_install for setup and dependency gaps.
- Use ultimate_memory to search and remember. Use ultimate_skills to inspect or create skills.
- Use ultimate_terminal/git/files for real project work. Prefer read-only inspection before writing.
- Use ultimate_mission for the multi-agent Planner -> Executor -> Reviewer path.
)PROMPT") + QStringLiteral("\n\nORIGINAL ONE-SHOT POLICY\n") + behavior;
}

QJsonArray UltimateAgent::tools() {
    QJsonArray out;
    out.append(tool(QStringLiteral("ultimate_status"),
                    QStringLiteral("Return live Ultimate HAI state: app, workspace, Git, harness, providers, 9Router, MausBot, MCP, memory, skills, agents."),
                    {}));
    out.append(tool(QStringLiteral("ultimate_diagnose"),
                    QStringLiteral("Run the real connection diagnostics and the full installation audit, and return every check with its result."),
                    {}));
    out.append(tool(QStringLiteral("ultimate_navigate"),
                    QStringLiteral("Open an Ultimate HAI view for the user."),
                    properties({{QStringLiteral("view"),
                                 enumProperty(QStringLiteral("Target view."), views())}}),
                    {QStringLiteral("view")}));
    out.append(tool(QStringLiteral("ultimate_connections"),
                    QStringLiteral("Inspect or change harness providers, the DeepSeek key, the default model, or restart the harness."),
                    properties({{QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."),
                                              {QStringLiteral("list"), QStringLiteral("set_deepseek"),
                                               QStringLiteral("set_provider"), QStringLiteral("remove_provider"),
                                               QStringLiteral("set_default"), QStringLiteral("refresh"),
                                               QStringLiteral("sync"), QStringLiteral("restart_harness")})},
                                {QStringLiteral("apiKey"), agentProperty(QStringLiteral("string"), QStringLiteral("API key. Never echoed back."))},
                                {QStringLiteral("providerId"), agentProperty(QStringLiteral("string"), QStringLiteral("Provider id."))},
                                {QStringLiteral("name"), agentProperty(QStringLiteral("string"), QStringLiteral("Display name."))},
                                {QStringLiteral("baseUrl"), agentProperty(QStringLiteral("string"), QStringLiteral("OpenAI-compatible base URL."))},
                                {QStringLiteral("models"), agentProperty(QStringLiteral("string"), QStringLiteral("One model id per line."))},
                                {QStringLiteral("makeDefault"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Make this provider/model the default."))},
                                {QStringLiteral("provider"), agentProperty(QStringLiteral("string"), QStringLiteral("Default provider id."))},
                                {QStringLiteral("model"), agentProperty(QStringLiteral("string"), QStringLiteral("Default model id."))},
                                {QStringLiteral("confirm"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Required true for destructive changes."))}}),
                    {QStringLiteral("action")}));
    out.append(tool(QStringLiteral("ultimate_services"),
                    QStringLiteral("Inspect or control the harness, 9Router and MausBot services."),
                    properties({{QStringLiteral("target"),
                                 enumProperty(QStringLiteral("Service target."),
                                              {QStringLiteral("harness"), QStringLiteral("9router"),
                                               QStringLiteral("mausbot"), QStringLiteral("all")})},
                                {QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."),
                                              {QStringLiteral("status"), QStringLiteral("start"),
                                               QStringLiteral("stop"), QStringLiteral("restart"),
                                               QStringLiteral("install")})},
                                {QStringLiteral("confirm"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Required true to stop a service."))}}),
                    {QStringLiteral("target"), QStringLiteral("action")}));
    out.append(tool(QStringLiteral("ultimate_router"),
                    QStringLiteral("Operate 9Router: status, models, apply models to the harness, login, start/stop, install, sync, disconnect."),
                    properties({{QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."),
                                              {QStringLiteral("status"), QStringLiteral("models"),
                                               QStringLiteral("apply"), QStringLiteral("start"),
                                               QStringLiteral("stop"), QStringLiteral("install"),
                                               QStringLiteral("login"), QStringLiteral("disconnect"),
                                               QStringLiteral("sync")})},
                                {QStringLiteral("models"),
                                 QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
                                             {QStringLiteral("description"), QStringLiteral("Model ids to apply. Empty applies all available models.")},
                                             {QStringLiteral("items"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}}},
                                {QStringLiteral("makeDefault"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Set the first model as harness default."))},
                                {QStringLiteral("password"), agentProperty(QStringLiteral("string"), QStringLiteral("Dashboard password. Never echoed back."))},
                                {QStringLiteral("confirm"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Required true for disconnect/stop."))}}),
                    {QStringLiteral("action")}));
    out.append(tool(QStringLiteral("ultimate_mcp"),
                    QStringLiteral("Inspect, add, remove or enable/disable custom MCP servers. Built-in servers are read-only."),
                    properties({{QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."),
                                              {QStringLiteral("list"), QStringLiteral("add"),
                                               QStringLiteral("remove"), QStringLiteral("toggle")})},
                                {QStringLiteral("name"), agentProperty(QStringLiteral("string"), QStringLiteral("Server name."))},
                                {QStringLiteral("transport"), enumProperty(QStringLiteral("Transport."), {QStringLiteral("stdio"), QStringLiteral("HTTP")})},
                                {QStringLiteral("command"), agentProperty(QStringLiteral("string"), QStringLiteral("stdio command."))},
                                {QStringLiteral("args"), agentProperty(QStringLiteral("string"), QStringLiteral("Arguments, one per line."))},
                                {QStringLiteral("env"), agentProperty(QStringLiteral("string"), QStringLiteral("KEY=VALUE, one per line."))},
                                {QStringLiteral("url"), agentProperty(QStringLiteral("string"), QStringLiteral("HTTP MCP URL."))},
                                {QStringLiteral("headers"), agentProperty(QStringLiteral("string"), QStringLiteral("HTTP headers, one per line."))},
                                {QStringLiteral("id"), agentProperty(QStringLiteral("string"), QStringLiteral("Custom server id."))},
                                {QStringLiteral("enabled"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Enable or disable."))},
                                {QStringLiteral("confirm"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Required true to remove."))}}),
                    {QStringLiteral("action")}));
    out.append(tool(QStringLiteral("ultimate_memory"),
                    QStringLiteral("Search, store or list persistent project memories."),
                    properties({{QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."),
                                              {QStringLiteral("search"), QStringLiteral("remember"),
                                               QStringLiteral("recent")})},
                                {QStringLiteral("query"), agentProperty(QStringLiteral("string"), QStringLiteral("Search query."))},
                                {QStringLiteral("title"), agentProperty(QStringLiteral("string"), QStringLiteral("Memory title."))},
                                {QStringLiteral("body"), agentProperty(QStringLiteral("string"), QStringLiteral("Memory body."))},
                                {QStringLiteral("kind"), enumProperty(QStringLiteral("Memory kind."),
                                                                      {QStringLiteral("note"), QStringLiteral("fact"),
                                                                       QStringLiteral("decision"), QStringLiteral("preference"),
                                                                       QStringLiteral("task"), QStringLiteral("result")})},
                                {QStringLiteral("tags"), agentProperty(QStringLiteral("string"), QStringLiteral("Comma-separated tags."))},
                                {QStringLiteral("pinned"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Pin the memory."))},
                                {QStringLiteral("project"), agentProperty(QStringLiteral("string"), QStringLiteral("Project path; defaults to the active workspace."))},
                                {QStringLiteral("limit"), agentProperty(QStringLiteral("integer"), QStringLiteral("Maximum results (1-50)."))}}),
                    {QStringLiteral("action")}));
    out.append(tool(QStringLiteral("ultimate_skills"),
                    QStringLiteral("List, read, create or remove harness skills."),
                    properties({{QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."),
                                              {QStringLiteral("list"), QStringLiteral("read"),
                                               QStringLiteral("create"), QStringLiteral("remove")})},
                                {QStringLiteral("id"), agentProperty(QStringLiteral("string"), QStringLiteral("Skill id or name."))},
                                {QStringLiteral("name"), agentProperty(QStringLiteral("string"), QStringLiteral("New skill name."))},
                                {QStringLiteral("description"), agentProperty(QStringLiteral("string"), QStringLiteral("Short description."))},
                                {QStringLiteral("instructions"), agentProperty(QStringLiteral("string"), QStringLiteral("Full SKILL.md instructions."))},
                                {QStringLiteral("confirm"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Required true to remove."))}}),
                    {QStringLiteral("action")}));
    out.append(tool(QStringLiteral("ultimate_mission"),
                    QStringLiteral("List, start or cancel the Planner -> Executor -> Reviewer mission runner."),
                    properties({{QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."),
                                              {QStringLiteral("list"), QStringLiteral("start"), QStringLiteral("cancel")})},
                                {QStringLiteral("title"), agentProperty(QStringLiteral("string"), QStringLiteral("Mission title."))},
                                {QStringLiteral("task"), agentProperty(QStringLiteral("string"), QStringLiteral("Full mission task."))},
                                {QStringLiteral("plannerId"), agentProperty(QStringLiteral("string"), QStringLiteral("Planner profile id."))},
                                {QStringLiteral("builderId"), agentProperty(QStringLiteral("string"), QStringLiteral("Executor profile id."))},
                                {QStringLiteral("reviewerId"), agentProperty(QStringLiteral("string"), QStringLiteral("Reviewer profile id."))}}),
                    {QStringLiteral("action")}));
    out.append(tool(QStringLiteral("ultimate_terminal"),
                    QStringLiteral("Run a shell command in the active workspace. Requires explicit confirmation from the user."),
                    properties({{QStringLiteral("command"), agentProperty(QStringLiteral("string"), QStringLiteral("Command to run."))},
                                {QStringLiteral("timeoutSeconds"), agentProperty(QStringLiteral("integer"), QStringLiteral("Timeout, max 60 seconds."))},
                                {QStringLiteral("confirm"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Required true; only after explicit user request."))}}),
                    {QStringLiteral("command"), QStringLiteral("confirm")}));
    out.append(tool(QStringLiteral("ultimate_files"),
                    QStringLiteral("List, read or write files in the active workspace."),
                    properties({{QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."),
                                              {QStringLiteral("list"), QStringLiteral("read"), QStringLiteral("write")})},
                                {QStringLiteral("path"), agentProperty(QStringLiteral("string"), QStringLiteral("Path relative to the workspace."))},
                                {QStringLiteral("content"), agentProperty(QStringLiteral("string"), QStringLiteral("Content for write."))},
                                {QStringLiteral("confirm"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Required true to write."))}}),
                    {QStringLiteral("action")}));
    out.append(tool(QStringLiteral("ultimate_git"),
                    QStringLiteral("Read Git status, a diff or recent log in the active workspace."),
                    properties({{QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."),
                                              {QStringLiteral("status"), QStringLiteral("diff"), QStringLiteral("log")})},
                                {QStringLiteral("path"), agentProperty(QStringLiteral("string"), QStringLiteral("Optional path for diff."))}}),
                    {QStringLiteral("action")}));
    out.append(tool(QStringLiteral("ultimate_install"),
                    QStringLiteral("Audit or install missing ecosystem components: system dependencies, DSH, 9Router, MausBot and services."),
                    properties({{QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."),
                                              {QStringLiteral("audit"), QStringLiteral("system"),
                                               QStringLiteral("install_dsh"), QStringLiteral("install_9router"),
                                               QStringLiteral("install_mausbot"), QStringLiteral("start_services")})}}),
                    {QStringLiteral("action")}));
    out.append(tool(QStringLiteral("ultimate_settings"),
                    QStringLiteral("Read or change app theme, language, startup view and harness autostart."),
                    properties({{QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."), {QStringLiteral("get"), QStringLiteral("set")})},
                                {QStringLiteral("theme"), agentProperty(QStringLiteral("string"), QStringLiteral("Theme id."))},
                                {QStringLiteral("language"), enumProperty(QStringLiteral("Language."), {QStringLiteral("fr"), QStringLiteral("en")})},
                                {QStringLiteral("startupView"), agentProperty(QStringLiteral("string"), QStringLiteral("Startup view id."))},
                                {QStringLiteral("autoStartHarness"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Start harness on launch."))}}),
                    {QStringLiteral("action")}));
    out.append(tool(QStringLiteral("ultimate_agents"),
                    QStringLiteral("List, add or remove agent role profiles used by missions."),
                    properties({{QStringLiteral("action"),
                                 enumProperty(QStringLiteral("Operation."),
                                              {QStringLiteral("list"), QStringLiteral("add_profile"),
                                               QStringLiteral("remove_profile")})},
                                {QStringLiteral("name"), agentProperty(QStringLiteral("string"), QStringLiteral("Profile name."))},
                                {QStringLiteral("role"), agentProperty(QStringLiteral("string"), QStringLiteral("Role id."))},
                                {QStringLiteral("prompt"), agentProperty(QStringLiteral("string"), QStringLiteral("Custom role prompt."))},
                                {QStringLiteral("id"), agentProperty(QStringLiteral("string"), QStringLiteral("Profile id."))},
                                {QStringLiteral("confirm"), agentProperty(QStringLiteral("boolean"), QStringLiteral("Required true to remove."))}}),
                    {QStringLiteral("action")}));
    return out;
}

QJsonObject UltimateAgent::executeTool(const QString &name, const QJsonObject &arguments) {
    const QString action = arguments.value(QStringLiteral("action")).toString();
    const QString workspace = m_backend ? m_backend->workspace() : QDir::homePath();

    auto confirmOrError = [&](const QString &what) -> QJsonObject {
        if (destructiveConfirmed(arguments, what))
            return {};
        return QJsonObject{{QStringLiteral("ok"), false},
                           {QStringLiteral("confirmation_required"), true},
                           {QStringLiteral("error"), tr("Confirmation explicite requise avant : %1.").arg(what)}};
    };
    auto listJson = [](const QVariantList &list) {
        QJsonArray array;
        for (const QVariant &value : list)
            array.append(QJsonValue::fromVariant(value));
        return array;
    };
    auto safePath = [&](const QString &relative) -> QString {
        if (relative.trimmed().isEmpty() || QDir::isAbsolutePath(relative))
            return {};
        const QString root = QFileInfo(workspace).absoluteFilePath();
        const QString target = QFileInfo(root + QLatin1Char('/') + relative).absoluteFilePath();
        return target == root || target.startsWith(root + QLatin1Char('/')) ? target : QString();
    };

    if (name == QLatin1String("ultimate_status"))
        return okResult({{QStringLiteral("snapshot"), snapshot()}});

    if (name == QLatin1String("ultimate_diagnose")) {
        if (m_connectionDoctor) m_connectionDoctor->run();
        if (m_installation) m_installation->check();
        QJsonObject result;
        if (m_connectionDoctor)
            result.insert(QStringLiteral("connections"),
                          QJsonObject{{QStringLiteral("summary"), m_connectionDoctor->summary()},
                                      {QStringLiteral("checks"), listJson(m_connectionDoctor->checks())}});
        if (m_installation)
            result.insert(QStringLiteral("installation"),
                          QJsonObject{{QStringLiteral("summary"), m_installation->summary()},
                                      {QStringLiteral("checks"), listJson(m_installation->checks())}});
        return okResult(result);
    }

    if (name == QLatin1String("ultimate_navigate")) {
        const QString view = arguments.value(QStringLiteral("view")).toString();
        if (!views().contains(view))
            return errorResult(tr("Vue inconnue : %1. Vues : %2").arg(view, views().join(QStringLiteral(", "))));
        if (m_backend) m_backend->setView(view);
        return okResult({{QStringLiteral("view"), view}});
    }

    if (name == QLatin1String("ultimate_connections")) {
        if (!m_connections)
            return errorResult(tr("Connexions indisponibles."));
        if (action == QLatin1String("list") || action.isEmpty()) {
            return okResult({{QStringLiteral("providers"), listJson(m_connections->providers())},
                             {QStringLiteral("defaultModel"), m_connections->defaultModel()},
                             {QStringLiteral("deepseekConfigured"), m_connections->deepseekConfigured()},
                             {QStringLiteral("envPath"), m_connections->envPath()}});
        }
        if (action == QLatin1String("set_deepseek")) {
            const QString key = arguments.value(QStringLiteral("apiKey")).toString();
            if (key.isEmpty()) return errorResult(tr("Clé DeepSeek manquante."));
            m_connections->saveDeepSeek(key);
            return m_connections->error().isEmpty()
                ? okResult({{QStringLiteral("deepseekConfigured"), m_connections->deepseekConfigured()}})
                : errorResult(m_connections->error());
        }
        if (action == QLatin1String("set_provider")) {
            const QString id = arguments.value(QStringLiteral("providerId")).toString();
            const QString displayName = arguments.value(QStringLiteral("name")).toString();
            const QString baseUrl = arguments.value(QStringLiteral("baseUrl")).toString();
            const QString key = arguments.value(QStringLiteral("apiKey")).toString();
            const QString models = arguments.value(QStringLiteral("models")).toString();
            if (id.isEmpty() || baseUrl.isEmpty())
                return errorResult(tr("providerId et baseUrl sont obligatoires."));
            m_connections->saveProvider(id, displayName.isEmpty() ? id : displayName, baseUrl,
                                        key, models, arguments.value(QStringLiteral("makeDefault")).toBool());
            return m_connections->error().isEmpty()
                ? okResult({{QStringLiteral("providers"), listJson(m_connections->providers())},
                            {QStringLiteral("defaultModel"), m_connections->defaultModel()}})
                : errorResult(m_connections->error());
        }
        if (action == QLatin1String("remove_provider")) {
            if (auto error = confirmOrError(tr("supprimer un provider")); !error.isEmpty()) return error;
            m_connections->removeProvider(arguments.value(QStringLiteral("providerId")).toString());
            return m_connections->error().isEmpty()
                ? okResult({{QStringLiteral("providers"), listJson(m_connections->providers())}})
                : errorResult(m_connections->error());
        }
        if (action == QLatin1String("set_default")) {
            m_connections->setDefault(arguments.value(QStringLiteral("provider")).toString(),
                                      arguments.value(QStringLiteral("model")).toString());
            return m_connections->error().isEmpty()
                ? okResult({{QStringLiteral("defaultModel"), m_connections->defaultModel()}})
                : errorResult(m_connections->error());
        }
        if (action == QLatin1String("refresh")) {
            m_connections->refresh();
            return okResult({{QStringLiteral("providers"), listJson(m_connections->providers())},
                             {QStringLiteral("defaultModel"), m_connections->defaultModel()}});
        }
        if (action == QLatin1String("sync")) {
            m_connections->refresh();
            if (m_nineRouter) {
                m_nineRouter->probe();
                m_nineRouter->syncDeepSeekFromDsh();
            }
            if (m_backend) m_backend->restartHarness();
            return okResult({{QStringLiteral("providers"), listJson(m_connections->providers())},
                             {QStringLiteral("defaultModel"), m_connections->defaultModel()},
                             {QStringLiteral("restarted"), true}});
        }
        if (action == QLatin1String("restart_harness")) {
            if (m_backend) m_backend->restartHarness();
            return okResult({{QStringLiteral("restarted"), true}});
        }
        return errorResult(tr("Action de connexion inconnue : %1").arg(action));
    }

    if (name == QLatin1String("ultimate_services")) {
        const QString target = arguments.value(QStringLiteral("target")).toString();
        if (action == QLatin1String("status") || action.isEmpty()) {
            return okResult({{QStringLiteral("harness"), m_backend ? m_backend->harnessState() : QString()},
                             {QStringLiteral("nineRouter"), m_nineRouter ? m_nineRouter->running() : false},
                             {QStringLiteral("nineRouterDetail"), m_nineRouter ? m_nineRouter->detail() : QString()},
                             {QStringLiteral("mausBot"), m_mausBot ? m_mausBot->running() : false},
                             {QStringLiteral("mausBotDetail"), m_mausBot ? m_mausBot->detail() : QString()}});
        }
        if (action == QLatin1String("stop") || action == QLatin1String("restart")
            || action == QLatin1String("install")) {
            if (auto error = confirmOrError(tr("%1 le service %2").arg(action, target)); !error.isEmpty()) return error;
        }
        if (target == QLatin1String("harness")) {
            if (action == QLatin1String("restart") || action == QLatin1String("start")) {
                if (m_backend) m_backend->restartHarness();
                return okResult({{QStringLiteral("started"), true}});
            }
            return errorResult(tr("Le harness ne peut être que relancé par cet outil."));
        }
        if (target == QLatin1String("9router")) {
            if (action == QLatin1String("start")) { if (m_nineRouter) m_nineRouter->start(); }
            else if (action == QLatin1String("stop")) { if (m_nineRouter) m_nineRouter->stop(); }
            else if (action == QLatin1String("install")) { if (m_nineRouter) m_nineRouter->install(); }
            else return errorResult(tr("Action 9Router inconnue : %1").arg(action));
            return okResult({{QStringLiteral("started"), action == QLatin1String("start") || action == QLatin1String("install")}});
        }
        if (target == QLatin1String("mausbot")) {
            if (action == QLatin1String("start")) { if (m_mausBot) m_mausBot->start(); }
            else if (action == QLatin1String("stop")) { if (m_mausBot) m_mausBot->stop(); }
            else if (action == QLatin1String("install")) { if (m_mausBot) m_mausBot->install(); }
            else return errorResult(tr("Action MausBot inconnue : %1").arg(action));
            return okResult({{QStringLiteral("started"), action == QLatin1String("start") || action == QLatin1String("install")}});
        }
        if (target == QLatin1String("all")) {
            if (action == QLatin1String("start")) { if (m_installation) m_installation->startServices(); }
            else if (action == QLatin1String("install")) {
                if (m_installation) { m_installation->installNineRouter(); m_installation->installMausBot(); }
            } else if (action == QLatin1String("stop")) {
                if (m_nineRouter) m_nineRouter->stop();
                if (m_mausBot) m_mausBot->stop();
            } else return errorResult(tr("Action globale inconnue : %1").arg(action));
            return okResult({{QStringLiteral("started"), true}});
        }
        return errorResult(tr("Cible de service inconnue : %1").arg(target));
    }

    if (name == QLatin1String("ultimate_router")) {
        if (!m_nineRouter)
            return errorResult(tr("9Router indisponible."));
        if (action == QLatin1String("start")) { m_nineRouter->start(); return okResult({{QStringLiteral("starting"), true}}); }
        if (action == QLatin1String("stop")) {
            if (auto error = confirmOrError(tr("arrêter 9Router")); !error.isEmpty()) return error;
            m_nineRouter->stop(); return okResult({{QStringLiteral("stopped"), true}});
        }
        if (action == QLatin1String("install")) { m_nineRouter->install(); return okResult({{QStringLiteral("installing"), true}}); }
        if (action == QLatin1String("disconnect")) {
            if (auto error = confirmOrError(tr("retirer 9Router du harness")); !error.isEmpty()) return error;
            m_nineRouter->disconnectHarness(); return okResult({{QStringLiteral("disconnected"), true}});
        }
        if (action == QLatin1String("login")) {
            m_nineRouter->login(arguments.value(QStringLiteral("password")).toString());
            return okResult({{QStringLiteral("authRequired"), m_nineRouter->authRequired()}});
        }
        if (action == QLatin1String("sync")) {
            m_nineRouter->probe();
            m_nineRouter->syncDeepSeekFromDsh();
            return okResult({{QStringLiteral("running"), m_nineRouter->running()},
                             {QStringLiteral("authRequired"), m_nineRouter->authRequired()}});
        }
        if (action == QLatin1String("models")) {
            return okResult({{QStringLiteral("models"), listJson(m_nineRouter->models())}});
        }
        if (action == QLatin1String("apply")) {
            QStringList models;
            for (const QJsonValue &value : arguments.value(QStringLiteral("models")).toArray())
                models.append(value.toString());
            if (models.isEmpty()) {
                for (const QVariant &value : m_nineRouter->models())
                    models.append(value.toMap().value(QStringLiteral("id")).toString());
            }
            if (models.isEmpty()) return errorResult(tr("Aucun modèle 9Router disponible."));
            m_nineRouter->applyToHarness(models, arguments.value(QStringLiteral("makeDefault")).toBool());
            return okResult({{QStringLiteral("applied"), models.size()},
                             {QStringLiteral("makeDefault"), arguments.value(QStringLiteral("makeDefault")).toBool()}});
        }
        return okResult({{QStringLiteral("installed"), m_nineRouter->installed()},
                         {QStringLiteral("running"), m_nineRouter->running()},
                         {QStringLiteral("authRequired"), m_nineRouter->authRequired()},
                         {QStringLiteral("keyMasked"), m_nineRouter->keyMasked()},
                         {QStringLiteral("models"), m_nineRouter->models().size()},
                         {QStringLiteral("detail"), m_nineRouter->detail()}});
    }

    if (name == QLatin1String("ultimate_mcp")) {
        if (!m_mcp) return errorResult(tr("MCP indisponible."));
        if (action == QLatin1String("list") || action.isEmpty()) {
            QJsonArray servers;
            for (const QVariant &value : m_mcp->servers()) {
                const QVariantMap map = value.toMap();
                servers.append(QJsonObject{{QStringLiteral("id"), map.value(QStringLiteral("id")).toString()},
                                           {QStringLiteral("name"), map.value(QStringLiteral("name")).toString()},
                                           {QStringLiteral("transport"), map.value(QStringLiteral("transport")).toString()},
                                           {QStringLiteral("enabled"), map.value(QStringLiteral("enabled")).toBool()}});
            }
            return okResult({{QStringLiteral("servers"), servers},
                             {QStringLiteral("builtins"), listJson(m_mcp->builtins())}});
        }
        if (action == QLatin1String("add")) {
            const bool ok = m_mcp->addServer(arguments.value(QStringLiteral("name")).toString(),
                                             arguments.value(QStringLiteral("transport")).toString(),
                                             arguments.value(QStringLiteral("command")).toString(),
                                             arguments.value(QStringLiteral("args")).toString(),
                                             arguments.value(QStringLiteral("env")).toString(),
                                             arguments.value(QStringLiteral("url")).toString(),
                                             arguments.value(QStringLiteral("headers")).toString());
            return ok ? okResult({{QStringLiteral("restartRequired"), true}}) : errorResult(m_mcp->error());
        }
        if (action == QLatin1String("remove")) {
            if (auto error = confirmOrError(tr("supprimer un serveur MCP")); !error.isEmpty()) return error;
            return m_mcp->removeServer(arguments.value(QStringLiteral("id")).toString())
                ? okResult({{QStringLiteral("removed"), true}}) : errorResult(m_mcp->error());
        }
        if (action == QLatin1String("toggle")) {
            return m_mcp->setEnabled(arguments.value(QStringLiteral("id")).toString(),
                                     arguments.value(QStringLiteral("enabled")).toBool())
                ? okResult({{QStringLiteral("toggled"), true}}) : errorResult(m_mcp->error());
        }
        return errorResult(tr("Action MCP inconnue : %1").arg(action));
    }

    if (name == QLatin1String("ultimate_memory")) {
        if (!m_memory) return errorResult(tr("Mémoire indisponible."));
        const QString project = arguments.value(QStringLiteral("project")).toString().isEmpty()
            ? workspace : arguments.value(QStringLiteral("project")).toString();
        if (action == QLatin1String("search")) {
            m_memory->search(arguments.value(QStringLiteral("query")).toString(), project);
            return okResult({{QStringLiteral("results"), listJson(m_memory->results())}});
        }
        if (action == QLatin1String("remember")) {
            const QString title = arguments.value(QStringLiteral("title")).toString();
            const QString body = arguments.value(QStringLiteral("body")).toString();
            if (title.isEmpty()) return errorResult(tr("Le titre de la mémoire est obligatoire."));
            const int id = m_memory->remember(project,
                                              arguments.value(QStringLiteral("kind")).toString(QStringLiteral("note")),
                                              title, body,
                                              arguments.value(QStringLiteral("tags")).toString(),
                                              arguments.value(QStringLiteral("pinned")).toBool(),
                                              3, QStringLiteral("ultimate-agent"));
            return id > 0 ? okResult({{QStringLiteral("id"), id}}) : errorResult(m_memory->error());
        }
        m_memory->refresh();
        return okResult({{QStringLiteral("entries"), listJson(m_memory->entries())},
                         {QStringLiteral("stats"), QJsonObject::fromVariantMap(m_memory->stats())}});
    }

    if (name == QLatin1String("ultimate_skills")) {
        if (!m_skills) return errorResult(tr("Skills indisponibles."));
        m_skills->refresh();
        if (action == QLatin1String("list") || action.isEmpty())
            return okResult({{QStringLiteral("skills"), listJson(m_skills->skills())}});
        const QString id = arguments.value(QStringLiteral("id")).toString();
        if (action == QLatin1String("read")) {
            for (const QVariant &value : m_skills->skills()) {
                const QVariantMap map = value.toMap();
                if (map.value(QStringLiteral("id")).toString() == id
                    || map.value(QStringLiteral("name")).toString() == id) {
                    QFile file(map.value(QStringLiteral("path")).toString());
                    if (!file.open(QIODevice::ReadOnly)) return errorResult(tr("Skill introuvable."));
                    return okResult({{QStringLiteral("id"), id},
                                     {QStringLiteral("path"), file.fileName()},
                                     {QStringLiteral("content"), truncateText(QString::fromUtf8(file.readAll()), 30000)}});
                }
            }
            return errorResult(tr("Skill introuvable : %1").arg(id));
        }
        if (action == QLatin1String("create")) {
            const bool ok = m_skills->createSkill(arguments.value(QStringLiteral("name")).toString(),
                                                  arguments.value(QStringLiteral("description")).toString(),
                                                  arguments.value(QStringLiteral("instructions")).toString());
            return ok ? okResult({{QStringLiteral("created"), true}}) : errorResult(m_skills->error());
        }
        if (action == QLatin1String("remove")) {
            if (auto error = confirmOrError(tr("supprimer un skill")); !error.isEmpty()) return error;
            return m_skills->removeSkill(id) ? okResult({{QStringLiteral("removed"), true}})
                                             : errorResult(m_skills->error());
        }
        return errorResult(tr("Action skills inconnue : %1").arg(action));
    }

    if (name == QLatin1String("ultimate_mission")) {
        if (!m_missions) return errorResult(tr("Missions indisponibles."));
        if (action == QLatin1String("cancel")) { m_missions->cancel(); return okResult({{QStringLiteral("cancelled"), true}}); }
        if (action == QLatin1String("start")) {
            const QString title = arguments.value(QStringLiteral("title")).toString();
            const QString task = arguments.value(QStringLiteral("task")).toString();
            if (task.isEmpty()) return errorResult(tr("La tâche de mission est obligatoire."));
            QString planner = arguments.value(QStringLiteral("plannerId")).toString();
            QString builder = arguments.value(QStringLiteral("builderId")).toString();
            QString reviewer = arguments.value(QStringLiteral("reviewerId")).toString();
            QStringList ids;
            if (m_agents) for (const QVariant &value : m_agents->profiles())
                ids.append(value.toMap().value(QStringLiteral("id")).toString());
            if (planner.isEmpty()) planner = ids.value(0);
            if (builder.isEmpty()) builder = ids.value(1, ids.value(0));
            if (reviewer.isEmpty()) reviewer = ids.value(2, ids.value(0));
            const int id = m_missions->startMissionWithAgents(
                title.isEmpty() ? tr("Mission Ultimate Agent") : title, task, workspace,
                planner, builder, reviewer);
            return id > 0 ? okResult({{QStringLiteral("missionId"), id}, {QStringLiteral("started"), true}})
                          : errorResult(m_missions->error());
        }
        return okResult({{QStringLiteral("missions"), listJson(m_missions->missions())},
                         {QStringLiteral("running"), m_missions->running()}});
    }

    if (name == QLatin1String("ultimate_terminal")) {
        if (auto error = confirmOrError(tr("exécuter une commande terminal")); !error.isEmpty()) return error;
        const QString command = arguments.value(QStringLiteral("command")).toString();
        if (command.trimmed().isEmpty()) return errorResult(tr("Commande vide."));
        QProcess process;
        process.setWorkingDirectory(workspace);
        process.setProcessChannelMode(QProcess::SeparateChannels);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        if (m_connections) {
            QFile envFile(m_connections->envPath());
            if (envFile.open(QIODevice::ReadOnly)) {
                for (const QString &line : QString::fromUtf8(envFile.readAll()).split(QLatin1Char('\n'))) {
                    const int equals = line.indexOf(QLatin1Char('='));
                    if (equals > 0) env.insert(line.left(equals), line.mid(equals + 1));
                }
            }
        }
        process.setProcessEnvironment(env);
        process.start(QStringLiteral("/bin/bash"), {QStringLiteral("-lc"), command});
        const int timeout = qBound(1, arguments.value(QStringLiteral("timeoutSeconds")).toInt(30), 60);
        if (!process.waitForStarted(3000)) return errorResult(tr("Impossible de lancer le shell."));
        if (!process.waitForFinished(timeout * 1000)) {
            process.kill();
            process.waitForFinished(1000);
            return errorResult(tr("Commande interrompue après %1 secondes.").arg(timeout));
        }
        return okResult({{QStringLiteral("exitCode"), process.exitCode()},
                         {QStringLiteral("stdout"), truncateText(QString::fromUtf8(process.readAllStandardOutput()), 16000)},
                         {QStringLiteral("stderr"), truncateText(QString::fromUtf8(process.readAllStandardError()), 8000)}});
    }

    if (name == QLatin1String("ultimate_files")) {
        if (action == QLatin1String("list") || action.isEmpty()) {
            QJsonArray files;
            QDirIterator it(workspace, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
            int count = 0;
            while (it.hasNext() && count < 250) {
                const QFileInfo info(it.next());
                if (info.absoluteFilePath().contains(QStringLiteral("/.git/"))
                    || info.absoluteFilePath().contains(QStringLiteral("/node_modules/")))
                    continue;
                files.append(QJsonObject{{QStringLiteral("path"), QDir(workspace).relativeFilePath(info.absoluteFilePath())},
                                         {QStringLiteral("size"), info.size()}});
                ++count;
            }
            return okResult({{QStringLiteral("files"), files}, {QStringLiteral("truncated"), count >= 250}});
        }
        const QString path = safePath(arguments.value(QStringLiteral("path")).toString());
        if (path.isEmpty()) return errorResult(tr("Chemin hors du workspace."));
        if (action == QLatin1String("read")) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) return errorResult(tr("Fichier illisible."));
            return okResult({{QStringLiteral("path"), arguments.value(QStringLiteral("path")).toString()},
                             {QStringLiteral("content"), truncateText(QString::fromUtf8(file.read(64000)), 64000)}});
        }
        if (action == QLatin1String("write")) {
            if (auto error = confirmOrError(tr("écrire un fichier")); !error.isEmpty()) return error;
            QDir().mkpath(QFileInfo(path).absolutePath());
            QSaveFile file(path);
            if (!file.open(QIODevice::WriteOnly)) return errorResult(tr("Fichier non inscriptible."));
            file.write(arguments.value(QStringLiteral("content")).toString().toUtf8());
            if (!file.commit()) return errorResult(tr("Écriture du fichier impossible."));
            return okResult({{QStringLiteral("path"), arguments.value(QStringLiteral("path")).toString()}});
        }
        return errorResult(tr("Action fichiers inconnue : %1").arg(action));
    }

    if (name == QLatin1String("ultimate_git")) {
        QStringList args;
        if (action == QLatin1String("diff")) {
            args << QStringLiteral("diff") << QStringLiteral("--stat");
            const QString path = arguments.value(QStringLiteral("path")).toString();
            if (!path.isEmpty()) args << path;
        } else if (action == QLatin1String("log")) {
            args << QStringLiteral("log") << QStringLiteral("--oneline") << QStringLiteral("-15");
        } else {
            args << QStringLiteral("status") << QStringLiteral("--short") << QStringLiteral("--branch");
        }
        QProcess process;
        process.setWorkingDirectory(workspace);
        process.start(QStringLiteral("git"), args);
        if (!process.waitForStarted(3000) || !process.waitForFinished(10000)) {
            process.kill(); process.waitForFinished(200);
            return errorResult(tr("Git ne répond pas."));
        }
        return okResult({{QStringLiteral("command"), QStringLiteral("git ") + args.join(QLatin1Char(' '))},
                         {QStringLiteral("exitCode"), process.exitCode()},
                         {QStringLiteral("output"), truncateText(QString::fromUtf8(process.readAllStandardOutput()), 20000)},
                         {QStringLiteral("error"), truncateText(QString::fromUtf8(process.readAllStandardError()), 4000)}});
    }

    if (name == QLatin1String("ultimate_install")) {
        if (action == QLatin1String("audit") || action.isEmpty()) {
            if (m_installation) m_installation->check();
            return okResult({{QStringLiteral("summary"), m_installation ? m_installation->summary() : QString()},
                             {QStringLiteral("checks"), m_installation ? listJson(m_installation->checks()) : QJsonArray()}});
        }
        if (!m_installation) return errorResult(tr("Installation indisponible."));
        if (action == QLatin1String("system")) m_installation->installSystemDependencies();
        else if (action == QLatin1String("install_dsh")) m_installation->installDsh();
        else if (action == QLatin1String("install_9router")) m_installation->installNineRouter();
        else if (action == QLatin1String("install_mausbot")) m_installation->installMausBot();
        else if (action == QLatin1String("start_services")) m_installation->startServices();
        else return errorResult(tr("Action installation inconnue : %1").arg(action));
        return okResult({{QStringLiteral("started"), true}, {QStringLiteral("action"), action}});
    }

    if (name == QLatin1String("ultimate_settings")) {
        if (action == QLatin1String("get") || action.isEmpty()) {
            return okResult({{QStringLiteral("theme"), m_backend ? m_backend->activeTheme() : QString()},
                             {QStringLiteral("language"), m_backend ? m_backend->languagePreference() : QString()},
                             {QStringLiteral("startupView"), m_backend ? m_backend->startupView() : QString()},
                             {QStringLiteral("autoStartHarness"), m_backend ? m_backend->autoStartHarness() : false}});
        }
        if (m_backend) {
            const QString theme = arguments.value(QStringLiteral("theme")).toString();
            const QString language = arguments.value(QStringLiteral("language")).toString();
            const QString startup = arguments.value(QStringLiteral("startupView")).toString();
            if (!theme.isEmpty()) m_backend->setTheme(theme);
            if (!language.isEmpty()) m_backend->setLanguage(language);
            if (!startup.isEmpty()) m_backend->setStartupView(startup);
            if (arguments.contains(QStringLiteral("autoStartHarness")))
                m_backend->setAutoStartHarness(arguments.value(QStringLiteral("autoStartHarness")).toBool());
        }
        return okResult({{QStringLiteral("theme"), m_backend ? m_backend->activeTheme() : QString()},
                         {QStringLiteral("language"), m_backend ? m_backend->languagePreference() : QString()},
                         {QStringLiteral("startupView"), m_backend ? m_backend->startupView() : QString()},
                         {QStringLiteral("autoStartHarness"), m_backend ? m_backend->autoStartHarness() : false}});
    }

    if (name == QLatin1String("ultimate_agents")) {
        if (!m_agents) return errorResult(tr("Agents indisponibles."));
        m_agents->refresh();
        if (action == QLatin1String("add_profile")) {
            const bool ok = m_agents->addProfile(arguments.value(QStringLiteral("name")).toString(),
                                                 arguments.value(QStringLiteral("role")).toString(QStringLiteral("custom")),
                                                 arguments.value(QStringLiteral("prompt")).toString());
            return ok ? okResult({{QStringLiteral("profiles"), listJson(m_agents->profiles())}})
                      : errorResult(m_agents->error());
        }
        if (action == QLatin1String("remove_profile")) {
            if (auto error = confirmOrError(tr("supprimer un profil d'agent")); !error.isEmpty()) return error;
            return m_agents->removeProfile(arguments.value(QStringLiteral("id")).toString())
                ? okResult({{QStringLiteral("profiles"), listJson(m_agents->profiles())}})
                : errorResult(m_agents->error());
        }
        return okResult({{QStringLiteral("profiles"), listJson(m_agents->profiles())}});
    }

    return errorResult(tr("Outil inconnu : %1").arg(name));
}

QString UltimateAgent::toolSummary(const QString &name, const QJsonObject &arguments,
                                   const QJsonObject &result) const {
    if (!result.value(QStringLiteral("ok")).toBool())
        return result.value(QStringLiteral("error")).toString(tr("Échec de l'outil."));
    if (name == QLatin1String("ultimate_status")) return tr("État du système");
    if (name == QLatin1String("ultimate_diagnose")) {
        const QString c = result.value(QStringLiteral("connections")).toObject()
                              .value(QStringLiteral("summary")).toString();
        const QString i = result.value(QStringLiteral("installation")).toObject()
                              .value(QStringLiteral("summary")).toString();
        return QStringList{c, i}.filter(QRegularExpression(QStringLiteral("."))).join(QStringLiteral(" · "));
    }
    if (name == QLatin1String("ultimate_navigate"))
        return tr("Vue : %1").arg(arguments.value(QStringLiteral("view")).toString());
    if (name == QLatin1String("ultimate_connections"))
        return tr("Connexions · %1").arg(arguments.value(QStringLiteral("action")).toString());
    if (name == QLatin1String("ultimate_services"))
        return tr("%1 · %2").arg(arguments.value(QStringLiteral("target")).toString(),
                                   arguments.value(QStringLiteral("action")).toString());
    if (name == QLatin1String("ultimate_router"))
        return tr("9Router · %1").arg(arguments.value(QStringLiteral("action")).toString());
    if (name == QLatin1String("ultimate_mcp"))
        return tr("MCP · %1").arg(arguments.value(QStringLiteral("action")).toString());
    if (name == QLatin1String("ultimate_memory"))
        return tr("Mémoire · %1").arg(arguments.value(QStringLiteral("action")).toString());
    if (name == QLatin1String("ultimate_skills"))
        return tr("Skills · %1").arg(arguments.value(QStringLiteral("action")).toString());
    if (name == QLatin1String("ultimate_mission"))
        return tr("Mission · %1").arg(arguments.value(QStringLiteral("action")).toString());
    if (name == QLatin1String("ultimate_terminal"))
        return tr("Terminal · code %1").arg(result.value(QStringLiteral("exitCode")).toInt());
    if (name == QLatin1String("ultimate_files"))
        return tr("Fichiers · %1").arg(arguments.value(QStringLiteral("path")).toString());
    if (name == QLatin1String("ultimate_git"))
        return tr("Git · %1").arg(arguments.value(QStringLiteral("action")).toString());
    if (name == QLatin1String("ultimate_install"))
        return tr("Installation · %1").arg(arguments.value(QStringLiteral("action")).toString());
    if (name == QLatin1String("ultimate_settings"))
        return tr("Réglages · %1").arg(arguments.value(QStringLiteral("action")).toString());
    if (name == QLatin1String("ultimate_agents"))
        return tr("Agents · %1").arg(arguments.value(QStringLiteral("action")).toString());
    return name;
}

bool UltimateAgent::destructiveConfirmed(const QJsonObject &arguments, const QString &) const {
    return arguments.value(QStringLiteral("confirm")).toBool();
}
