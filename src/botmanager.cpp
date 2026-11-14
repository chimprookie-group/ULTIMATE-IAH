#include "botmanager.h"

#include "backend.h"
#include "botrunner.h"
#include "botserver.h"
#include "botstore.h"
#include "memorystore.h"
#include "decisionengine.h"

#include <QClipboard>
#include <QCoreApplication>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

BotManager::BotManager(Backend *backend, MemoryStore *memory, QObject *parent)
    : QObject(parent),
      m_backend(backend),
      m_memory(memory),
      m_store(new BotStore({}, this)),
      m_runner(new BotRunner(this)),
      m_server(new BotServer(this, this)) {
    m_dshBinary = QCoreApplication::applicationFilePath();
    ensureToken();

    connectStore();

    connect(m_runner, &BotRunner::statusChanged, this, &BotManager::statusChanged);
    connect(m_runner, &BotRunner::outputChanged, this, &BotManager::outputChanged);
    connect(m_runner, &BotRunner::runningChanged, this, &BotManager::runningChanged);
    connect(m_runner, &BotRunner::finished, this,
            [this](int runId, bool success, const QString &output, const QString &error) {
                finishRun(runId, success, output, error);
                processQueue();
            });
    connect(m_runner, &BotRunner::cancelled, this, [this](int runId) {
        finishRun(runId, false, {}, tr("Tâche annulée."));
        processQueue();
    });
    connect(m_server, &BotServer::stateChanged, this, [this] {
        emit serverChanged();
        ensureMcpBridge();
    });
    connect(m_server, &BotServer::errorChanged, this, [this] {
        setError(m_server->error());
        emit serverChanged();
    });

    m_scheduleTimer.setInterval(30 * 1000);
    connect(&m_scheduleTimer, &QTimer::timeout, this, &BotManager::checkSchedules);
    m_scheduleTimer.start();

    if (qEnvironmentVariableIsEmpty("ULTIMATE_IAH_AGENT_JOB")) {
        m_server->setToken(m_token);
        m_server->start(8790);
        ensureMcpBridge();
    }
}

BotManager::~BotManager() = default;

void BotManager::connectStore() {
    connect(m_store, &BotStore::changed, this, &BotManager::botsChanged);
    connect(m_store, &BotStore::changed, this, &BotManager::statsChanged);
    connect(m_store, &BotStore::messagesChanged, this, &BotManager::messagesChanged);
    connect(m_store, &BotStore::runsChanged, this, &BotManager::runsChanged);
    connect(m_store, &BotStore::schedulesChanged, this, &BotManager::schedulesChanged);
    connect(m_store, &BotStore::selectionChanged, this, &BotManager::selectionChanged);
    connect(m_store, &BotStore::errorChanged, this, [this] {
        setError(m_store->error());
    });
}

void BotManager::ensureToken() {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/bot.token");
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        m_token = QString::fromUtf8(file.readAll()).trimmed();
    }
    if (m_token.size() < 32) {
        m_token = QUuid::createUuid().toString(QUuid::WithoutBraces)
            + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QDir().mkpath(QFileInfo(path).absolutePath());
        QSaveFile out(path);
        if (out.open(QIODevice::WriteOnly)) {
            out.write(m_token.toUtf8() + '\n');
            out.commit();
            QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
        }
    }
}

QVariantList BotManager::bots() const { return m_store->bots(); }
QVariantList BotManager::messages() const { return m_store->messages(); }
QVariantList BotManager::runs() const { return m_store->runs(); }
QVariantList BotManager::schedules() const { return m_store->schedules(); }
int BotManager::selectedBotId() const { return m_store->selectedBotId(); }
QVariantMap BotManager::selectedBot() const {
    const int id = selectedBotId();
    for (const QVariant &value : m_store->bots()) {
        const QVariantMap bot = value.toMap();
        if (bot.value(QStringLiteral("id")).toInt() == id)
            return bot;
    }
    return {};
}
QVariantMap BotManager::stats() const { return m_store->stats(); }
bool BotManager::running() const { return m_runner->running(); }
QString BotManager::status() const { return m_runner->status(); }
QString BotManager::output() const { return m_runner->output(); }
int BotManager::queueCount() const { return m_queue.size(); }
bool BotManager::serverRunning() const { return m_server->running(); }
bool BotManager::serverHealthy() const { return m_server->healthy(); }
QString BotManager::serverUrl() const { return m_server->url(); }
int BotManager::serverPort() const { return m_server->port(); }
QString BotManager::token() const { return m_token; }
QString BotManager::tokenMasked() const {
    return m_token.size() > 4 ? QStringLiteral("••••") + m_token.right(4) : QStringLiteral("••••");
}
QString BotManager::error() const { return m_error.isEmpty() ? m_store->error() : m_error; }

void BotManager::setDshBinary(const QString &binary) {
    if (!binary.trimmed().isEmpty())
        m_dshBinary = binary;
}

void BotManager::refresh() { m_store->refresh(); }
void BotManager::selectBot(int id) { m_store->selectBot(id); }

int BotManager::createBot(const QString &name, const QString &description,
                          const QString &instructions, const QString &permission,
                          const QString &provider, const QString &model) {
    return m_store->createBot(name, description, instructions, permission, provider, model);
}

bool BotManager::updateBot(int id, const QString &name, const QString &description,
                           const QString &instructions, const QString &permission,
                           bool enabled, const QString &provider, const QString &model) {
    return m_store->updateBot(id, name, description, instructions, permission, enabled,
                              provider, model);
}

bool BotManager::removeBot(int id) {
    if (id == m_activeBotId && m_runner->running()) {
        setError(tr("Impossible de supprimer le bot pendant une exécution."));
        return false;
    }
    return m_store->removeBot(id);
}

int BotManager::sendMessage(int botId, const QString &text) {
    return enqueue(botId, text, QStringLiteral("ui"));
}

void BotManager::clearConversation(int botId) {
    m_store->clearMessages(botId);
}

void BotManager::cancel() {
    m_queue.clear();
    emit queueChanged();
    if (m_runner->running())
        m_runner->cancel();
}

void BotManager::startServer() {
    if (!m_server->running())
        m_server->setToken(m_token);
    m_server->start(8790);
    emit serverChanged();
}

void BotManager::stopServer() {
    m_server->stop();
    emit serverChanged();
}

void BotManager::restartServer() {
    m_server->stop();
    m_server->start(8790);
    emit serverChanged();
}

void BotManager::copyToken() {
    if (QGuiApplication::clipboard())
        QGuiApplication::clipboard()->setText(m_token);
}

int BotManager::createSchedule(int botId, const QString &title, const QString &prompt,
                               int intervalMinutes) {
    return m_store->createSchedule(botId, title, prompt, intervalMinutes);
}

bool BotManager::toggleSchedule(int id, bool enabled) {
    return m_store->toggleSchedule(id, enabled);
}

bool BotManager::removeSchedule(int id) {
    return m_store->removeSchedule(id);
}

void BotManager::clearError() { setError({}); }

int BotManager::enqueue(int botId, const QString &text, const QString &source,
                        int scheduleId) {
    if (text.trimmed().isEmpty())
        return -1;
    QVariantMap bot;
    for (const QVariant &value : m_store->bots()) {
        if (value.toMap().value(QStringLiteral("id")).toInt() == botId) {
            bot = value.toMap();
            break;
        }
    }
    if (bot.isEmpty()) {
        setError(tr("Bot introuvable."));
        return -1;
    }
    if (!bot.value(QStringLiteral("enabled")).toBool()) {
        setError(tr("Ce bot est désactivé."));
        return -1;
    }
    const int runId = m_store->createRun(botId, text, QStringLiteral("queued"));
    if (runId < 0)
        return -1;
    m_store->appendMessage(botId, QStringLiteral("user"), text,
                           source == QLatin1String("schedule")
                               ? QStringLiteral("schedule") : QStringLiteral("chat"), runId);
    m_queue.enqueue(Job{runId, botId, scheduleId, text, source});
    emit queueChanged();
    setError({});
    processQueue();
    return runId;
}

void BotManager::processQueue() {
    if (m_runner->running() || m_queue.isEmpty())
        return;
    const Job job = m_queue.dequeue();
    emit queueChanged();
    startJob(job);
}

void BotManager::startJob(const Job &job) {
    QVariantMap bot;
    for (const QVariant &value : m_store->bots()) {
        if (value.toMap().value(QStringLiteral("id")).toInt() == job.botId) {
            bot = value.toMap();
            break;
        }
    }
    if (bot.isEmpty()) {
        finishRun(job.runId, false, {}, tr("Bot introuvable."));
        return;
    }
    m_activeRunId = job.runId;
    m_activeBotId = job.botId;
    m_activeScheduleId = job.scheduleId;
    m_store->updateRun(job.runId, QStringLiteral("running"), {}, {});
    QString provider = bot.value(QStringLiteral("provider")).toString();
    QString model = bot.value(QStringLiteral("model")).toString();
    if (m_decisions && (provider.isEmpty() || model.isEmpty())) {
        const DecisionEngine::Endpoint endpoint = DecisionEngine::resolve(m_decisions->context());
        if (endpoint.valid()) {
            if (provider.isEmpty())
                provider = endpoint.providerId;
            if (model.isEmpty())
                model = endpoint.model;
        }
    }
    m_runner->start(job.runId, bot.value(QStringLiteral("name")).toString(),
                    buildPrompt(bot, job.task),
                    bot.value(QStringLiteral("permission")).toString(),
                    m_backend ? m_backend->workspace() : QDir::homePath(),
                    m_dshBinary, provider, model);
    emit runningChanged();
    emit statusChanged();
}

QString BotManager::buildPrompt(const QVariantMap &bot, const QString &task) const {
    QString prompt;
    prompt += tr("Tu es « %1 », un bot natif d'Ultimate IAH.\n").arg(bot.value("name").toString());
    prompt += tr("Rôle : %1\n").arg(bot.value("description").toString());
    prompt += tr("Instructions : %1\n").arg(bot.value("instructions").toString());
    prompt += tr("Permission : %1\n").arg(bot.value("permission").toString());
    if (m_backend) {
        prompt += tr("Workspace : %1\n").arg(m_backend->workspace());
        prompt += tr("État du harness : %1 — %2\n")
            .arg(m_backend->harnessState(), m_backend->harnessDetail());
    }
    if (m_memory && m_backend) {
        const QString recall = m_memory->recall(m_backend->workspace(), task, 6);
        if (!recall.isEmpty())
            prompt += tr("\nMémoire pertinente :\n%1\n").arg(recall);
    }
    const QVariantList history = m_store->messagesFor(bot.value("id").toInt());
    if (history.size() > 1) {
        QStringList recent;
        const int from = qMax(0, history.size() - 12);
        for (int i = from; i < history.size(); ++i) {
            const QVariantMap message = history.at(i).toMap();
            if (message.value("role").toString() == "assistant")
                recent << tr("Bot : %1").arg(message.value("content").toString().left(1800));
        }
        if (!recent.isEmpty())
            prompt += tr("\nHistorique récent :\n%1\n").arg(recent.join("\n"));
    }
    prompt += tr("\nTâche :\n%1").arg(task);
    return prompt;
}

void BotManager::finishRun(int runId, bool success, const QString &output,
                           const QString &error) {
    const int botId = m_activeBotId;
    const int scheduleId = m_activeScheduleId;
    if (botId > 0) {
        if (success) {
            m_store->appendMessage(botId, QStringLiteral("assistant"),
                                   output.trimmed().isEmpty() ? tr("Tâche terminée.") : output,
                                   QStringLiteral("chat"), runId);
        } else {
            m_store->appendMessage(botId, QStringLiteral("assistant"),
                                   error.isEmpty() ? tr("La tâche a échoué.") : error,
                                   QStringLiteral("error"), runId);
        }
    }
    if (!m_store->updateRun(runId, success ? QStringLiteral("done") : QStringLiteral("failed"),
                            output, error, success ? 0 : 1))
        setError(m_store->error());
    if (success && m_memory && m_backend && !output.trimmed().isEmpty()) {
        QString botName;
        for (const QVariant &value : m_store->bots()) {
            if (value.toMap().value(QStringLiteral("id")).toInt() == botId) {
                botName = value.toMap().value(QStringLiteral("name")).toString();
                break;
            }
        }
        m_memory->remember(m_backend->workspace(), QStringLiteral("result"),
                           tr("Bot : %1").arg(botName), output.left(6000),
                           QStringLiteral("bot,result"), false, 2, QStringLiteral("bot"));
    }
    if (scheduleId > 0) {
        int interval = 60;
        for (const QVariant &value : m_store->schedules()) {
            const QVariantMap schedule = value.toMap();
            if (schedule.value("id").toInt() == scheduleId) {
                interval = schedule.value("intervalMinutes").toInt();
                break;
            }
        }
        m_store->markScheduleRun(scheduleId, success,
                                 QDateTime::currentDateTimeUtc().addSecs(qMax(1, interval) * 60)
                                     .toString(Qt::ISODateWithMs),
                                 output.left(500));
        m_pendingSchedules.remove(scheduleId);
    }
    m_activeRunId = -1;
    m_activeBotId = -1;
    m_activeScheduleId = -1;
    emit runningChanged();
    emit statusChanged();
    emit outputChanged();
    emit botsChanged();
}

void BotManager::checkSchedules() {
    if (m_runner->running() && m_queue.size() > 2)
        return;
    const QVariantList due = m_store->dueSchedules();
    for (const QVariant &value : due) {
        const QVariantMap schedule = value.toMap();
        const int id = schedule.value("id").toInt();
        if (m_pendingSchedules.contains(id))
            continue;
        const int runId = enqueue(schedule.value("botId").toInt(),
                                  schedule.value("prompt").toString(),
                                  QStringLiteral("schedule"), id);
        if (runId > 0)
            m_pendingSchedules.insert(id);
    }
}

QJsonArray BotManager::botsJson() const {
    return QJsonArray::fromVariantList(m_store->bots());
}

QJsonObject BotManager::botJson(int id) const {
    for (const QVariant &value : m_store->bots()) {
        if (value.toMap().value(QStringLiteral("id")).toInt() == id)
            return QJsonObject::fromVariantMap(value.toMap());
    }
    return {};
}

QJsonArray BotManager::messagesJson(int botId) const {
    return QJsonArray::fromVariantList(m_store->messagesFor(botId));
}

QJsonObject BotManager::sendBotMessage(int botId, const QString &text, const QString &source) {
    const int runId = enqueue(botId, text, source);
    if (runId < 0)
        return QJsonObject{{QStringLiteral("error"), error()}};
    return QJsonObject{{QStringLiteral("ok"), true},
                       {QStringLiteral("runId"), runId},
                       {QStringLiteral("status"), QStringLiteral("queued")}};
}

QJsonObject BotManager::runJson(int id) const {
    return QJsonObject::fromVariantMap(m_store->runById(id));
}

QJsonObject BotManager::statusJson() const {
    return QJsonObject{
        {QStringLiteral("ok"), true},
        {QStringLiteral("service"), QStringLiteral("ultimate-iah-bots")},
        {QStringLiteral("version"), 1},
        {QStringLiteral("running"), m_runner->running()},
        {QStringLiteral("queue"), m_queue.size()},
        {QStringLiteral("bots"), m_store->bots().size()},
        {QStringLiteral("runs"), m_store->stats().value(QStringLiteral("runs")).toInt()},
    };
}

QJsonObject BotManager::createBotFromApi(const QString &name, const QString &description,
                                         const QString &instructions,
                                         const QString &permission,
                                         const QString &provider,
                                         const QString &model) {
    const int id = createBot(name, description, instructions, permission, provider, model);
    if (id < 0)
        return QJsonObject{{QStringLiteral("error"), error()}};
    return botJson(id);
}

void BotManager::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}

void BotManager::ensureMcpBridge() {
    if (!m_server->running())
        return;
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    const QString script = dataDir + QStringLiteral("/bots_mcp.py");
    QFile resource(QStringLiteral(":/mcp/bots_mcp.py"));
    if (resource.open(QIODevice::ReadOnly)) {
        QSaveFile out(script);
        if (out.open(QIODevice::WriteOnly)) {
            out.write(resource.readAll());
            out.commit();
            QFile::setPermissions(script, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        }
    }
    if (!QFileInfo::exists(script))
        return;
    const auto quote = [](const QString &value) {
        QString escaped = value;
        escaped.replace('\\', QStringLiteral("\\\\"));
        escaped.replace('"', QStringLiteral("\\\""));
        return QStringLiteral("\"") + escaped + QStringLiteral("\"");
    };
    const QString patchPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/mcp-bots.patch.yml");
    const QString patch =
        QStringLiteral("- insert:\n")
        + QStringLiteral("    - id: mcp-ultimate-iah-bots\n")
        + QStringLiteral("      name: \"@deepseek-ai/dsh-mcp-client\"\n")
        + QStringLiteral("      config:\n")
        + QStringLiteral("        serverName: ultimate-iah-bots\n")
        + QStringLiteral("        transport: stdio\n")
        + QStringLiteral("        command: \"python3\"\n")
        + QStringLiteral("        args: [") + quote(script) + QStringLiteral("]\n")
        + QStringLiteral("        env:\n")
        + QStringLiteral("          ULTIMATE_IAH_BOTS_URL: ") + quote(m_server->url()) + QStringLiteral("\n")
        + QStringLiteral("          ULTIMATE_IAH_BOTS_TOKEN: ") + quote(m_token) + QStringLiteral("\n");
    QSaveFile out(patchPath);
    if (out.open(QIODevice::WriteOnly)) {
        out.write(patch.toUtf8());
        out.commit();
        QFile::setPermissions(patchPath, QFile::ReadOwner | QFile::WriteOwner);
    }
}
