#include "crasyrouter.h"

#include <QDateTime>
#include <QGuiApplication>
#include <QClipboard>
#include <QSharedPointer>
#include <algorithm>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <QUuid>

namespace {
QByteArray jsonBytes(const QJsonObject &object) {
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

double priceValue(const QJsonValue &value) {
    if (value.isDouble())
        return value.toDouble();
    return value.toString().toDouble();
}
}

CrasyRouter::CrasyRouter(const QString &configDir, QObject *parent)
    : QObject(parent),
      m_network(new QNetworkAccessManager(this)),
      m_configDir(configDir.isEmpty()
                      ? QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                            + QStringLiteral("/4crasy")
                      : configDir) {
    connect(&m_server, &QTcpServer::newConnection, this, [this] {
        while (m_server.hasPendingConnections())
            handleConnection(m_server.nextPendingConnection());
    });
    loadConfig();
    QFile tokenFile(tokenPath());
    if (tokenFile.open(QIODevice::ReadOnly))
        m_token = QString::fromUtf8(tokenFile.readAll()).trimmed();
    if (m_token.size() < 24)
        m_token = QUuid::createUuid().toString(QUuid::WithoutBraces) + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QDir().mkpath(m_configDir);
    QSaveFile tokenSave(tokenPath());
    if (tokenSave.open(QIODevice::WriteOnly)) {
        tokenSave.write(m_token.toUtf8());
        tokenSave.commit();
    }
    QFile::setPermissions(tokenPath(), QFile::ReadOwner | QFile::WriteOwner);
    rebuildChannelView();
    if (m_autoStart)
        QTimer::singleShot(0, this, [this] { start(); });
}

CrasyRouter::~CrasyRouter() { stop(); }

QString CrasyRouter::configPath() const { return m_configDir + QStringLiteral("/router.json"); }
QString CrasyRouter::tokenPath() const { return m_configDir + QStringLiteral("/token"); }
QString CrasyRouter::endpoint() const {
    return running() ? QStringLiteral("http://127.0.0.1:%1/v1").arg(m_server.serverPort())
                     : QStringLiteral("http://127.0.0.1:%1/v1").arg(m_port);
}
int CrasyRouter::freeModelCount() const {
    int count = 0;
    for (const QVariant &value : m_models)
        if (value.toMap().value(QStringLiteral("free")).toBool()) ++count;
    return count;
}

QString CrasyRouter::resolveKey(const QString &envName) const {
    const QString env = qEnvironmentVariable(envName.toUtf8().constData());
    if (!env.isEmpty())
        return env;
    QStringList files = {
        QDir::homePath() + QStringLiteral("/.dsh/.credentials.yaml"),
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/agent.env"),
    };
    for (const QString &path : files) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) continue;
        const QString text = QString::fromUtf8(file.readAll());
        QRegularExpression expression(QStringLiteral("(?m)^\\s*%1\\s*[:=]\\s*(\\S+)").arg(QRegularExpression::escape(envName)));
        const auto match = expression.match(text);
        if (match.hasMatch()) return match.captured(1).trimmed().remove('"').remove('\'');
    }
    return {};
}

void CrasyRouter::loadConfig() {
    m_channelData.clear();
    auto defaults = [this] {
        m_channelData = {
            {QStringLiteral("openrouter"), QStringLiteral("OpenRouter Free"), QStringLiteral("https://openrouter.ai/api/v1"),
             QStringLiteral("OPENROUTER_API_KEY"), 1, true, true},
            {QStringLiteral("groq"), QStringLiteral("Groq"), QStringLiteral("https://api.groq.com/openai/v1"),
             QStringLiteral("GROQ_API_KEY"), 2, true, true},
            {QStringLiteral("gemini"), QStringLiteral("Gemini"), QStringLiteral("https://generativelanguage.googleapis.com/v1beta/openai"),
             QStringLiteral("GEMINI_API_KEY"), 3, true, true},
            {QStringLiteral("cerebras"), QStringLiteral("Cerebras"), QStringLiteral("https://api.cerebras.ai/v1"),
             QStringLiteral("CEREBRAS_API_KEY"), 4, true, true},
            {QStringLiteral("mistral"), QStringLiteral("Mistral"), QStringLiteral("https://api.mistral.ai/v1"),
             QStringLiteral("MISTRAL_API_KEY"), 5, true, true},
            {QStringLiteral("huggingface"), QStringLiteral("Hugging Face"), QStringLiteral("https://router.huggingface.co/v1"),
             QStringLiteral("HF_TOKEN"), 6, true, true},
            {QStringLiteral("deepseek"), QStringLiteral("DeepSeek"), QStringLiteral("https://api.deepseek.com/v1"),
             QStringLiteral("DEEPSEEK_API_KEY"), 7, true, false},
            {QStringLiteral("ollama"), QStringLiteral("Ollama local"), QStringLiteral("http://127.0.0.1:11434/v1"),
             QString(), 0, true, true},
        };
        for (Channel &channel : m_channelData)
            if (channel.apiKey.isEmpty())
            channel.apiKey = resolveKey(channel.apiKeyEnv);
    };
    defaults();
    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly)) {
        rebuildChannelView();
        return;
    }
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    m_port = qBound(1024, root.value(QStringLiteral("port")).toInt(m_port), 65535);
    m_freeOnly = root.value(QStringLiteral("freeOnly")).toBool(m_freeOnly);
    m_maxFallbacks = qBound(0, root.value(QStringLiteral("maxFallbacks")).toInt(m_maxFallbacks), 5);
    m_autoStart = root.value(QStringLiteral("autoStart")).toBool(m_autoStart);
    m_strategy = root.value(QStringLiteral("strategy")).toString(m_strategy);
    m_preferFree = root.value(QStringLiteral("preferFree")).toBool(m_preferFree);
    const QJsonArray channelsArray = root.value(QStringLiteral("channels")).toArray();
    if (channelsArray.isEmpty()) {
        rebuildChannelView();
        return;
    }
    QList<Channel> configured;
    for (const QJsonValue &value : channelsArray) {
        const QJsonObject object = value.toObject();
        const QString id = object.value(QStringLiteral("id")).toString().trimmed();
        if (id.isEmpty()) continue;
        Channel channel;
        channel.id = id;
        channel.name = object.value(QStringLiteral("name")).toString(id);
        channel.baseUrl = object.value(QStringLiteral("baseUrl")).toString();
        channel.apiKeyEnv = object.value(QStringLiteral("apiKeyEnv")).toString();
        channel.apiKey = object.value(QStringLiteral("apiKey")).toString();
        channel.priority = qBound(0, object.value(QStringLiteral("priority")).toInt(5), 100);
        channel.enabled = object.value(QStringLiteral("enabled")).toBool(true);
        channel.free = object.value(QStringLiteral("free")).toBool(true);
        if (channel.apiKey.isEmpty())
            if (channel.apiKey.isEmpty())
            channel.apiKey = resolveKey(channel.apiKeyEnv);
        if (!channel.baseUrl.isEmpty())
            configured.append(channel);
    }
    if (!configured.isEmpty())
        m_channelData = configured;
    rebuildChannelView();
}

bool CrasyRouter::saveConfig() const {
    QDir().mkpath(m_configDir);
    QJsonArray channelsArray;
    for (const Channel &channel : m_channelData) {
        QJsonObject object{{QStringLiteral("id"), channel.id},
                           {QStringLiteral("name"), channel.name},
                           {QStringLiteral("baseUrl"), channel.baseUrl},
                           {QStringLiteral("apiKeyEnv"), channel.apiKeyEnv},
                           {QStringLiteral("priority"), channel.priority},
                           {QStringLiteral("enabled"), channel.enabled},
                           {QStringLiteral("free"), channel.free}};
        if (!channel.apiKey.isEmpty())
            object.insert(QStringLiteral("apiKey"), channel.apiKey);
        channelsArray.append(object);
    }
    QSaveFile file(configPath());
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(jsonBytes(QJsonObject{
        {QStringLiteral("channels"), channelsArray},
        {QStringLiteral("port"), m_port},
        {QStringLiteral("freeOnly"), m_freeOnly},
        {QStringLiteral("maxFallbacks"), m_maxFallbacks},
        {QStringLiteral("autoStart"), m_autoStart},
        {QStringLiteral("strategy"), m_strategy},
        {QStringLiteral("preferFree"), m_preferFree},
    }));
    if (!file.commit()) return false;
    QFile::setPermissions(configPath(), QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

void CrasyRouter::rebuildChannelView() {
    QVariantList view;
    for (const Channel &channel : m_channelData) {
        view.append(QVariantMap{
            {QStringLiteral("id"), channel.id},
            {QStringLiteral("name"), channel.name},
            {QStringLiteral("baseUrl"), channel.baseUrl},
            {QStringLiteral("apiKeyEnv"), channel.apiKeyEnv},
            {QStringLiteral("enabled"), channel.enabled},
            {QStringLiteral("free"), channel.free},
            {QStringLiteral("priority"), channel.priority},
            {QStringLiteral("hasKey"), !channel.apiKey.isEmpty() || channel.apiKeyEnv.isEmpty()},
            {QStringLiteral("modelCount"), channel.modelIds.size()},
            {QStringLiteral("freeModelCount"), channel.freeModelIds.size()},
            {QStringLiteral("status"), !channel.enabled ? QStringLiteral("disabled")
                 : (channel.modelIds.isEmpty() ? QStringLiteral("idle") : QStringLiteral("ready"))},
        });
    }
    m_channels = view;
    m_configState = QStringLiteral("saved");
    emit changed();
}

void CrasyRouter::saveSettings(int port, bool freeOnly, int maxFallbacks,
                              bool autoStart, const QString &strategy) {
    m_port = qBound(1024, port, 65535);
    m_freeOnly = freeOnly;
    m_maxFallbacks = qBound(0, maxFallbacks, 5);
    m_autoStart = autoStart;
    m_strategy = (strategy == QLatin1String("free") || strategy == QLatin1String("quality"))
        ? strategy : QStringLiteral("balanced");
    saveConfig();
    emit changed();
}

bool CrasyRouter::saveChannel(const QString &idValue, const QString &nameValue,
                             const QString &baseUrlValue, const QString &apiKeyEnv,
                             const QString &apiKey, int priority, bool free, bool enabled) {
    QString id = idValue.trimmed().toLower();
    id.replace(QRegularExpression(QStringLiteral("[^a-z0-9_-]+")), QStringLiteral("-"));
    id.remove(QRegularExpression(QStringLiteral("^-+|-+$")));
    if (id.isEmpty())
        id = QStringLiteral("channel");
    const QUrl url(baseUrlValue.trimmed());
    if (!url.isValid() || (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https"))) {
        setError(tr("URL de base HTTP(S) invalide."));
        return false;
    }
    Channel *target = nullptr;
    for (Channel &channel : m_channelData) {
        if (channel.id == id) { target = &channel; break; }
    }
    if (!target) {
        m_channelData.append(Channel{});
        target = &m_channelData.last();
        target->id = id;
    }
    target->name = nameValue.trimmed().isEmpty() ? id : nameValue.trimmed();
    target->baseUrl = url.toString().endsWith(QLatin1Char('/'))
        ? url.toString().chopped(1) : url.toString();
    target->apiKeyEnv = apiKeyEnv.trimmed();
    if (!apiKey.trimmed().isEmpty())
        target->apiKey = apiKey.trimmed();
    else if (target->apiKey.isEmpty())
        target->apiKey = resolveKey(target->apiKeyEnv);
    target->priority = qBound(0, priority, 100);
    target->free = free;
    target->enabled = enabled;
    if (!saveConfig()) {
        setError(tr("Impossible d'enregistrer la configuration 4Crasy."));
        return false;
    }
    setError({});
    rebuildChannelView();
    return true;
}

bool CrasyRouter::removeChannel(const QString &id) {
    for (int i = 0; i < m_channelData.size(); ++i) {
        if (m_channelData.at(i).id != id) continue;
        m_channelData.removeAt(i);
        for (int m = m_models.size() - 1; m >= 0; --m) {
            if (m_models.at(m).toMap().value(QStringLiteral("channel")).toString() == id)
                m_models.removeAt(m);
        }
        saveConfig();
        rebuildChannelView();
        return true;
    }
    return false;
}

void CrasyRouter::resetChannels() {
    QFile::remove(configPath());
    loadConfig();
    saveConfig();
}

QString CrasyRouter::testChannel(const QString &id) {
    for (Channel &channel : m_channelData) {
        if (channel.id != id) continue;
        if (channel.baseUrl.isEmpty()) return tr("URL de base absente.");
        if (!channel.apiKeyEnv.isEmpty() && channel.apiKey.isEmpty())
            return tr("Clé API absente.");
        QNetworkRequest request{QUrl(channel.baseUrl + QStringLiteral("/models"))};
        request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("4Crasy/1.0"));
        if (!channel.apiKey.isEmpty())
            request.setRawHeader("Authorization", "Bearer " + channel.apiKey.toUtf8());
        QNetworkReply *reply = m_network->get(request);
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(8000);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start();
        loop.exec();
        const bool finished = reply->isFinished();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray data = finished ? reply->readAll() : QByteArray();
        reply->abort();
        reply->deleteLater();
        if (!finished || status < 200 || status >= 300)
            return tr("Échec HTTP %1.").arg(status > 0 ? QString::number(status) : tr("timeout"));
        const QJsonObject root = QJsonDocument::fromJson(data).object();
        QJsonArray models = root.value(QStringLiteral("data")).toArray();
        if (models.isEmpty()) models = root.value(QStringLiteral("models")).toArray();
        channel.modelIds.clear();
        channel.freeModelIds.clear();
        for (const QJsonValue &value : models) {
            const QJsonObject object = value.toObject();
            const QString modelId = object.value(QStringLiteral("id")).toString(object.value(QStringLiteral("name")).toString());
            if (!modelId.isEmpty()) channel.modelIds.append(modelId);
        }
        rebuildChannelView();
        return tr("Connexion OK · %1 modèle(s).").arg(channel.modelIds.size());
    }
    return tr("Canal introuvable.");
}

void CrasyRouter::addModel(const QString &channel, const QJsonObject &model) {
    const QString channelId = channel;
    const QString id = model.value(QStringLiteral("id")).toString();
    if (id.isEmpty()) return;
    const QJsonObject pricing = model.value(QStringLiteral("pricing")).toObject();
    const bool free = !pricing.isEmpty()
        && priceValue(pricing.value(QStringLiteral("prompt"))) == 0
        && priceValue(pricing.value(QStringLiteral("completion"))) == 0;
    const QJsonObject architecture = model.value(QStringLiteral("architecture")).toObject();
    for (Channel &channel : m_channelData) {
        if (channel.id == channelId) {
            if (!channel.modelIds.contains(id)) channel.modelIds.append(id);
            if (free && !channel.freeModelIds.contains(id)) channel.freeModelIds.append(id);
            break;
        }
    }
    m_models.append(QVariantMap{
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), model.value(QStringLiteral("name")).toString(id)},
        {QStringLiteral("channel"), channel},
        {QStringLiteral("free"), free},
        {QStringLiteral("context"), model.value(QStringLiteral("context_length")).toInt()},
        {QStringLiteral("modalities"), architecture.value(QStringLiteral("input_modalities")).toArray().toVariantList()},
    });
}

QVariantList CrasyRouter::parseModels(const QJsonArray &array, const QString &channel) {
    QVariantList out;
    for (const QJsonValue &value : array) {
        QJsonObject object = value.toObject();
        const QString id = object.value(QStringLiteral("id")).toString();
        if (id.isEmpty()) continue;
        if (!object.contains(QStringLiteral("pricing")))
            object.insert(QStringLiteral("pricing"), QJsonObject{{QStringLiteral("prompt"), QStringLiteral("0")},
                                                                 {QStringLiteral("completion"), QStringLiteral("0")}});
        if (channel == QLatin1String("ollama"))
            object.insert(QStringLiteral("name"), id);
        const QJsonObject pricing = object.value(QStringLiteral("pricing")).toObject();
        const bool free = priceValue(pricing.value(QStringLiteral("prompt"))) == 0
            && priceValue(pricing.value(QStringLiteral("completion"))) == 0;
        out.append(QVariantMap{{QStringLiteral("id"), id},
                               {QStringLiteral("name"), object.value(QStringLiteral("name")).toString(id)},
                               {QStringLiteral("channel"), channel},
                               {QStringLiteral("free"), free},
                               {QStringLiteral("context"), object.value(QStringLiteral("context_length")).toInt()}});
    }
    return out;
}

void CrasyRouter::refreshModels() {
    m_models.clear();
    for (Channel &channel : m_channelData) {
        channel.modelIds.clear();
        channel.freeModelIds.clear();
        if (channel.apiKey.isEmpty())
            channel.apiKey = resolveKey(channel.apiKeyEnv);
        if (!channel.enabled) continue;
        if (!channel.apiKeyEnv.isEmpty() && channel.apiKey.isEmpty()) continue;
        QNetworkRequest request{QUrl(channel.baseUrl + QStringLiteral("/models"))};
        request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("4Crasy/1.0"));
        if (!channel.apiKey.isEmpty())
            request.setRawHeader("Authorization", "Bearer " + channel.apiKey.toUtf8());
        QNetworkReply *reply = m_network->get(request);
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(8000);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start();
        loop.exec();
        if (!reply->isFinished()) {
            reply->abort();
            reply->deleteLater();
            continue;
        }
        const QByteArray data = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        if (status < 200 || status >= 300) continue;
        const QJsonObject root = QJsonDocument::fromJson(data).object();
        QJsonArray models = root.value(QStringLiteral("data")).toArray();
        if (models.isEmpty()) models = root.value(QStringLiteral("models")).toArray();
        for (const QJsonValue &value : models)
            addModel(channel.id, value.toObject());
    }
    rebuildChannelView();
    emit changed();
}

void CrasyRouter::refresh() {
    loadConfig();
    rebuildChannelView();
}

void CrasyRouter::setAutoRefresh(bool enabled) {
    if (m_autoRefresh == enabled) return;
    m_autoRefresh = enabled;
    emit changed();
}

void CrasyRouter::setPreferFree(bool enabled) {
    if (m_preferFree == enabled) return;
    m_preferFree = enabled;
    saveConfig();
    emit changed();
}

void CrasyRouter::setFreeOnly(bool enabled) {
    if (m_freeOnly == enabled) return;
    m_freeOnly = enabled;
    saveConfig();
    emit changed();
}

void CrasyRouter::setPort(int value) {
    const int port = qBound(1024, value, 65535);
    if (m_port == port) return;
    m_port = port;
    saveConfig();
    emit changed();
}

void CrasyRouter::setMaxFallbacks(int value) {
    const int count = qBound(0, value, 5);
    if (m_maxFallbacks == count) return;
    m_maxFallbacks = count;
    saveConfig();
    emit changed();
}

void CrasyRouter::setStrategy(const QString &value) {
    const QString strategy = (value == QLatin1String("free") || value == QLatin1String("quality"))
        ? value : QStringLiteral("balanced");
    if (m_strategy == strategy) return;
    m_strategy = strategy;
    saveConfig();
    emit changed();
}

void CrasyRouter::setAutoStart(bool enabled) {
    if (m_autoStart == enabled) return;
    m_autoStart = enabled;
    saveConfig();
    emit changed();
}

bool CrasyRouter::setChannelEnabled(const QString &id, bool enabled) {
    bool found = false;
    for (Channel &channel : m_channelData) {
        if (channel.id != id) continue;
        channel.enabled = enabled;
        found = true;
        break;
    }
    if (!found) return false;
    saveConfig();
    rebuildChannelView();
    return true;
}

QString CrasyRouter::chooseChannel(const QJsonObject &request, const QString &requestedModel,
                                   QJsonObject *selection) const {
    QString text = requestedModel;
    for (const QJsonValue &value : request.value(QStringLiteral("messages")).toArray())
        text += QLatin1Char(' ') + value.toObject().value(QStringLiteral("content")).toString();
    text = text.toLower();
    const bool vision = request.value(QStringLiteral("messages")).toArray().toVariantList().size() > 0
        && QJsonDocument(request).toJson().contains("\"image_url\"");
    const bool code = text.contains(QStringLiteral("code")) || text.contains(QStringLiteral("bug"))
        || text.contains(QStringLiteral("compile")) || text.contains(QStringLiteral("test"))
        || text.contains(QStringLiteral("c++")) || text.contains(QStringLiteral("qml"));
    const bool reasoning = text.contains(QStringLiteral("plan")) || text.contains(QStringLiteral("architecture"))
        || text.contains(QStringLiteral("analyse")) || text.contains(QStringLiteral("reason"));
    auto score = [&](const QVariantMap &model) {
        int value = 1000 - model.value(QStringLiteral("channelPriority")).toInt() * 20;
        if (m_preferFree && model.value(QStringLiteral("free")).toBool()) value += 300;
        if (m_strategy == QLatin1String("quality") && !model.value(QStringLiteral("free")).toBool()) value += 80;
        if (m_strategy == QLatin1String("free") && model.value(QStringLiteral("free")).toBool()) value += 160;
        if (requestedModel != QLatin1String("auto") && model.value(QStringLiteral("id")).toString() == requestedModel) value += 10000;
        const QString id = model.value(QStringLiteral("id")).toString().toLower();
        if (code && (id.contains("code") || id.contains("coder") || id.contains("deepseek"))) value += 120;
        if (reasoning && (id.contains("reason") || id.contains("r1") || id.contains("think"))) value += 100;
        if (vision && model.value(QStringLiteral("modalities")).toStringList().contains(QStringLiteral("image"))) value += 150;
        const int context = model.value(QStringLiteral("context")).toInt();
        if (context > 32000) value += 20;
        return value;
    };
    QString bestChannel;
    QVariantMap bestModel;
    for (const QVariant &value : m_models) {
        QVariantMap model = value.toMap();
        const QString channelId = model.value(QStringLiteral("channel")).toString();
        const auto iterator = std::find_if(m_channelData.cbegin(), m_channelData.cend(),
                                           [&](const Channel &c) { return c.id == channelId; });
        if (iterator == m_channelData.cend() || !iterator->enabled) continue;
        if (m_freeOnly && !model.value(QStringLiteral("free")).toBool()) continue;
        model.insert(QStringLiteral("channelPriority"), iterator->priority);
        if (bestModel.isEmpty() || score(model) > score(bestModel)) {
            bestModel = model;
            bestChannel = channelId;
        }
    }
    if (bestModel.isEmpty()) return {};
    if (selection) {
        selection->insert(QStringLiteral("channel"), bestChannel);
        selection->insert(QStringLiteral("model"), QJsonObject::fromVariantMap(bestModel));
    }
    return bestChannel;
}

QString CrasyRouter::routePreview(const QString &prompt, const QString &requestedModel) {
    QJsonObject request{{QStringLiteral("messages"), QJsonArray{QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                                                          {QStringLiteral("content"), prompt}}}}};
    QJsonObject selection;
    const QString channel = chooseChannel(request, requestedModel, &selection);
    if (channel.isEmpty()) return tr("Aucun modèle disponible.");
    const QVariantMap model = selection.value(QStringLiteral("model")).toObject().toVariantMap();
    return QStringLiteral("%1 → %2").arg(channel, model.value(QStringLiteral("id")).toString());
}

bool CrasyRouter::start(quint16 preferredPort) {
    if (m_server.isListening()) return true;
    const quint16 requestedPort = preferredPort == 8788 ? quint16(m_port) : preferredPort;
    if (!m_server.listen(QHostAddress::LocalHost, requestedPort)
        && !m_server.listen(QHostAddress::LocalHost, 0)) {
        setError(tr("Impossible de démarrer 4Crasy : %1").arg(m_server.errorString()));
        return false;
    }
    setError({});
    if (m_autoRefresh && m_models.isEmpty())
        refreshModels();
    emit stateChanged();
    return true;
}

void CrasyRouter::stop() {
    if (!m_server.isListening()) return;
    m_server.close();
    emit stateChanged();
}

void CrasyRouter::setError(const QString &error) {
    if (m_error == error) return;
    m_error = error;
    emit errorChanged();
}

void CrasyRouter::clearError() { setError({}); }

void CrasyRouter::copyToken() const {
    QGuiApplication::clipboard()->setText(m_token);
}

bool CrasyRouter::healthy() const {
    // Listening is the health fact. Idle time is reported separately through
    // lastActivityAt; a quiet but listening router is healthy.
    return m_server.isListening();
}

QJsonObject CrasyRouter::status() const {
    QJsonArray channelArray;
    for (const QVariant &value : m_channels)
        channelArray.append(QJsonObject::fromVariantMap(value.toMap()));
    return QJsonObject{
        {QStringLiteral("ok"), running()},
        {QStringLiteral("running"), running()},
        {QStringLiteral("listening"), listening()},
        {QStringLiteral("healthy"), healthy()},
        {QStringLiteral("lastActivityAt"), m_lastServedAt > 0
             ? QDateTime::fromMSecsSinceEpoch(m_lastServedAt).toString(Qt::ISODate)
             : QString()},
        {QStringLiteral("endpoint"), endpoint()},
        {QStringLiteral("channels"), channelArray},
        {QStringLiteral("models"), m_models.size()},
        {QStringLiteral("freeModels"), freeModelCount()},
        {QStringLiteral("requests"), m_requestCount},
        {QStringLiteral("preferFree"), m_preferFree},
        {QStringLiteral("tokenConfigured"), !m_token.isEmpty()},
        {QStringLiteral("port"), m_port},
        {QStringLiteral("freeOnly"), m_freeOnly},
        {QStringLiteral("maxFallbacks"), m_maxFallbacks},
        {QStringLiteral("strategy"), m_strategy},
        {QStringLiteral("autoStart"), m_autoStart},
        {QStringLiteral("configPath"), configPath()},
    };
}

bool CrasyRouter::authorized(const QHash<QByteArray, QByteArray> &headers) const {
    QByteArray provided = headers.value(QByteArrayLiteral("authorization"));
    if (provided.startsWith("Bearer ")) provided = provided.mid(7).trimmed();
    else provided = headers.value(QByteArrayLiteral("x-api-key"));
    return !m_token.isEmpty() && provided == m_token.toUtf8();
}

QByteArray CrasyRouter::httpStatus(int code) {
    switch (code) {
    case 200: return "OK";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Payload Too Large";
    case 429: return "Too Many Requests";
    case 502: return "Bad Gateway";
    default: return "Error";
    }
}

void CrasyRouter::respond(QTcpSocket *socket, int status, const QByteArray &type,
                          const QByteArray &body) {
    if (!socket) return;
    m_lastServedAt = QDateTime::currentMSecsSinceEpoch();
    QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + ' ' + httpStatus(status) + "\r\n";
    response += "Content-Type: " + type + "\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    socket->write(response);
    socket->disconnectFromHost();
}

void CrasyRouter::handleConnection(QTcpSocket *socket) {
    if (!socket) return;
    const QPointer<QTcpSocket> guard(socket);
    auto buffer = QSharedPointer<QByteArray>::create();
    auto handled = QSharedPointer<bool>::create(false);
    connect(socket, &QTcpSocket::readyRead, this, [this, guard, buffer, handled] {
        if (!guard || *handled) return;
        buffer->append(guard->readAll());
        if (buffer->size() > 4 * 1024 * 1024) {
            *handled = true;
            respond(guard, 413, "application/json", R"({"error":{"message":"payload too large","type":"invalid_request_error"}})");
            return;
        }
        const int headerEnd = buffer->indexOf("\r\n\r\n");
        if (headerEnd < 0) return;
        const QList<QByteArray> lines = buffer->left(headerEnd).split('\n');
        if (lines.isEmpty()) return;
        const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
        if (requestLine.size() < 2) return;
        const QByteArray method = requestLine.at(0).toUpper();
        const QByteArray path = requestLine.at(1).split('?').first();
        QHash<QByteArray, QByteArray> headers;
        for (int i = 1; i < lines.size(); ++i) {
            const QByteArray line = lines.at(i).trimmed();
            const int colon = line.indexOf(':');
            if (colon > 0) headers.insert(line.left(colon).trimmed().toLower(), line.mid(colon + 1).trimmed());
        }
        const int length = qBound(0, headers.value("content-length").toInt(), 4 * 1024 * 1024);
        if (buffer->size() < headerEnd + 4 + length) return;
        const QByteArray body = buffer->mid(headerEnd + 4, length);
        *handled = true;
        if (method == "GET" && path == "/health") {
            respond(guard, 200, "application/json", jsonBytes(status()));
            return;
        }
        if (!authorized(headers)) {
            respond(guard, 401, "application/json", R"({"error":{"message":"invalid 4Crasy token","type":"authentication_error"}})");
            return;
        }
        if (method == "GET" && (path == "/v1/models" || path == "/models")) {
            QJsonArray data;
            for (const QVariant &value : m_models) {
                const QVariantMap model = value.toMap();
                if (m_freeOnly && !model.value(QStringLiteral("free")).toBool()) continue;
                data.append(QJsonObject{{QStringLiteral("id"), model.value(QStringLiteral("id")).toString()},
                                        {QStringLiteral("object"), QStringLiteral("model")},
                                        {QStringLiteral("owned_by"), model.value(QStringLiteral("channel")).toString()},
                                        {QStringLiteral("free"), model.value(QStringLiteral("free")).toBool()}});
            }
            respond(guard, 200, "application/json", jsonBytes(QJsonObject{{QStringLiteral("object"), QStringLiteral("list")},
                                                                          {QStringLiteral("data"), data}}));
            return;
        }
        if (method == "POST" && (path == "/v1/chat/completions" || path == "/chat/completions")) {
            QJsonParseError error;
            const QJsonDocument document = QJsonDocument::fromJson(body, &error);
            if (error.error != QJsonParseError::NoError || !document.isObject()) {
                respond(guard, 400, "application/json", R"({"error":{"message":"invalid JSON","type":"invalid_request_error"}})");
                return;
            }
            QJsonObject request = document.object();
            if (request.value(QStringLiteral("stream")).toBool()) {
                respond(guard, 400, "application/json", R"({"error":{"message":"streaming is not supported by 4Crasy yet","type":"invalid_request_error"}})");
                return;
            }
            request.insert(QStringLiteral("stream"), false);
            QJsonObject selection;
            const QString requested = request.value(QStringLiteral("model")).toString(QStringLiteral("auto"));
            const QString channel = chooseChannel(request, requested, &selection);
            if (channel.isEmpty()) {
                respond(guard, 503, "application/json", R"({"error":{"message":"no free model available","type":"server_error"}})");
                return;
            }
            const QVariantMap model = selection.value(QStringLiteral("model")).toObject().toVariantMap();
            const auto channelIterator = std::find_if(m_channelData.begin(), m_channelData.end(),
                                                      [&](const Channel &c) { return c.id == channel; });
            if (channelIterator == m_channelData.end()) {
                respond(guard, 503, "application/json", R"({"error":{"message":"channel unavailable","type":"server_error"}})");
                return;
            }
            QJsonArray fallbacks;
            const QString chosenModel = model.value(QStringLiteral("id")).toString();
            for (const QVariant &candidate : m_models) {
                const QVariantMap candidateMap = candidate.toMap();
                if (candidateMap.value(QStringLiteral("channel")).toString() == channel)
                    continue;
                fallbacks.append(QJsonObject{{QStringLiteral("channel"), candidateMap.value(QStringLiteral("channel")).toString()},
                                             {QStringLiteral("model"), candidateMap.value(QStringLiteral("id")).toString()}});
                if (fallbacks.size() >= m_maxFallbacks) break;
            }
            sendProxy(guard, *channelIterator, request, chosenModel, fallbacks, 0);
            return;
        }
        respond(guard, 404, "application/json", R"({"error":{"message":"not found","type":"invalid_request_error"}})");
    });
}

void CrasyRouter::sendProxy(QTcpSocket *socket, const Channel &channel, const QJsonObject &request,
                            const QString &model, const QJsonArray &fallbacks, int attempt) {
    Q_UNUSED(attempt);
    QNetworkRequest networkRequest{QUrl(channel.baseUrl + QStringLiteral("/chat/completions"))};
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("4Crasy/1.0"));
    if (!channel.apiKey.isEmpty())
        networkRequest.setRawHeader("Authorization", "Bearer " + channel.apiKey.toUtf8());
    QJsonObject body = request;
    body.insert(QStringLiteral("model"), model);
    QNetworkReply *reply = m_network->post(networkRequest, jsonBytes(body));
    const QPointer<QTcpSocket> guard(socket);
    connect(reply, &QNetworkReply::finished, this, [this, reply, guard, channel, request, model, fallbacks, attempt] {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray data = reply->readAll();
        const QByteArray type = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
        const auto networkError = reply->error();
        reply->deleteLater();
        ++m_requestCount;
        m_recentRoutes.prepend(QVariantMap{{QStringLiteral("time"), QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))},
                                           {QStringLiteral("channel"), channel.id},
                                           {QStringLiteral("model"), model},
                                           {QStringLiteral("status"), status},
                                           {QStringLiteral("ok"), status >= 200 && status < 300 && networkError == QNetworkReply::NoError}});
        while (m_recentRoutes.size() > 20) m_recentRoutes.removeLast();
        emit changed();
        if (!guard) return;
        if (status >= 200 && status < 300 && networkError == QNetworkReply::NoError) {
            respond(guard, status, type.isEmpty() ? "application/json" : type, data);
            return;
        }
        if (!fallbacks.isEmpty() && attempt < 2) {
            QJsonArray remaining = fallbacks;
            const QJsonObject next = remaining.first().toObject();
            remaining.removeAt(0);
            const QString nextChannel = next.value(QStringLiteral("channel")).toString();
            const QString nextModel = next.value(QStringLiteral("model")).toString();
            for (const Channel &candidate : m_channelData) {
                if (candidate.id == nextChannel && candidate.enabled) {
                    sendProxy(guard, candidate, request, nextModel, remaining, attempt + 1);
                    return;
                }
            }
        }
        QJsonObject error{{QStringLiteral("message"), tr("Le provider 4Crasy a échoué.")},
                          {QStringLiteral("type"), QStringLiteral("provider_error")},
                          {QStringLiteral("provider_status"), status},
                          {QStringLiteral("provider"), channel.id}};
        respond(guard, 502, "application/json", jsonBytes(QJsonObject{{QStringLiteral("error"), error}}));
    });
}
