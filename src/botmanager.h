#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QQueue>
#include <QSet>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class Backend;
class DecisionEngine;
class BotRunner;
class BotServer;
class BotStore;
class MemoryStore;

// Native bot service for Ultimate IAH. Bots are persisted in SQLite, executed
// one at a time through the native agent job runner, scheduled locally, exposed over a
// token-protected loopback HTTP API and bridged into DSH through MCP.
class BotManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList bots READ bots NOTIFY botsChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QVariantList runs READ runs NOTIFY runsChanged)
    Q_PROPERTY(QVariantList schedules READ schedules NOTIFY schedulesChanged)
    Q_PROPERTY(int selectedBotId READ selectedBotId NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap selectedBot READ selectedBot NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY statsChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString output READ output NOTIFY outputChanged)
    Q_PROPERTY(int queueCount READ queueCount NOTIFY queueChanged)
    Q_PROPERTY(bool serverRunning READ serverRunning NOTIFY serverChanged)
    Q_PROPERTY(QString serverUrl READ serverUrl NOTIFY serverChanged)
    Q_PROPERTY(int serverPort READ serverPort NOTIFY serverChanged)
    Q_PROPERTY(QString tokenMasked READ tokenMasked NOTIFY serverChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    BotManager(Backend *backend, MemoryStore *memory, QObject *parent = nullptr);
    void setDecisionEngine(DecisionEngine *engine) { m_decisions = engine; }
    ~BotManager() override;

    QVariantList bots() const;
    QVariantList messages() const;
    QVariantList runs() const;
    QVariantList schedules() const;
    int selectedBotId() const;
    QVariantMap selectedBot() const;
    QVariantMap stats() const;
    bool running() const;
    QString status() const;
    QString output() const;
    int queueCount() const;
    bool serverRunning() const;
    QString serverUrl() const;
    int serverPort() const;
    QString tokenMasked() const;
    QString error() const;

    void setDshBinary(const QString &binary);
    QString token() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void selectBot(int id);
    Q_INVOKABLE int createBot(const QString &name, const QString &description,
                              const QString &instructions, const QString &permission,
                              const QString &provider = {}, const QString &model = {});
    Q_INVOKABLE bool updateBot(int id, const QString &name, const QString &description,
                               const QString &instructions, const QString &permission,
                               bool enabled, const QString &provider = {}, const QString &model = {});
    Q_INVOKABLE bool removeBot(int id);
    Q_INVOKABLE int sendMessage(int botId, const QString &text);
    Q_INVOKABLE void clearConversation(int botId);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void startServer();
    Q_INVOKABLE void stopServer();
    Q_INVOKABLE void restartServer();
    Q_INVOKABLE void copyToken();
    Q_INVOKABLE int createSchedule(int botId, const QString &title, const QString &prompt,
                                   int intervalMinutes);
    Q_INVOKABLE bool toggleSchedule(int id, bool enabled);
    Q_INVOKABLE bool removeSchedule(int id);
    Q_INVOKABLE void clearError();

    // HTTP/MCP-facing contract.
    QJsonArray botsJson() const;
    QJsonObject botJson(int id) const;
    QJsonArray messagesJson(int botId) const;
    QJsonObject sendBotMessage(int botId, const QString &text, const QString &source);
    QJsonObject runJson(int id) const;
    QJsonObject statusJson() const;
    QJsonObject createBotFromApi(const QString &name, const QString &description,
                                 const QString &instructions, const QString &permission,
                                 const QString &provider = {}, const QString &model = {});

signals:
    void botsChanged();
    void messagesChanged();
    void runsChanged();
    void schedulesChanged();
    void selectionChanged();
    void statsChanged();
    void runningChanged();
    void statusChanged();
    void outputChanged();
    void queueChanged();
    void serverChanged();
    void errorChanged();

private:
    struct Job {
        int runId = -1;
        int botId = -1;
        int scheduleId = -1;
        QString task;
        QString source;
    };

    void ensureToken();
    void ensureMcpBridge();
    void connectStore();
    int enqueue(int botId, const QString &text, const QString &source, int scheduleId = -1);
    void processQueue();
    void startJob(const Job &job);
    QString buildPrompt(const QVariantMap &bot, const QString &task) const;
    void finishRun(int runId, bool success, const QString &output, const QString &error);
    void checkSchedules();
    void setError(const QString &error);

    Backend *m_backend = nullptr;
    MemoryStore *m_memory = nullptr;
    DecisionEngine *m_decisions = nullptr;
    BotStore *m_store = nullptr;
    BotRunner *m_runner = nullptr;
    BotServer *m_server = nullptr;
    QQueue<Job> m_queue;
    QSet<int> m_pendingSchedules;
    QTimer m_scheduleTimer;
    QString m_dshBinary;
    QString m_token;
    QString m_error;
    int m_activeRunId = -1;
    int m_activeBotId = -1;
    int m_activeScheduleId = -1;
};
