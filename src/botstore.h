#pragma once

#include <QSqlDatabase>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Persistent registry for Ultimate IAH bots, their conversations, runs and
// schedules. Every object is scoped to a bot; messages and runs are pruned on
// demand, never silently. The store is safe to open while the app is running.
class BotStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList bots READ bots NOTIFY changed)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QVariantList runs READ runs NOTIFY runsChanged)
    Q_PROPERTY(QVariantList schedules READ schedules NOTIFY schedulesChanged)
    Q_PROPERTY(int selectedBotId READ selectedBotId NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString databasePath READ databasePath CONSTANT)

public:
    explicit BotStore(const QString &path = {}, QObject *parent = nullptr);
    ~BotStore() override;

    QVariantList bots() const { return m_bots; }
    QVariantList messages() const { return m_messages; }
    QVariantList runs() const { return m_runs; }
    QVariantList schedules() const { return m_schedules; }
    int selectedBotId() const { return m_selectedBotId; }
    QVariantMap stats() const { return m_stats; }
    QString error() const { return m_error; }
    QString databasePath() const { return m_path; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void selectBot(int id);
    Q_INVOKABLE int createBot(const QString &name, const QString &description,
                              const QString &instructions = {},
                              const QString &permission = QStringLiteral("read-only"),
                              const QString &provider = {}, const QString &model = {});
    Q_INVOKABLE bool updateBot(int id, const QString &name, const QString &description,
                               const QString &instructions, const QString &permission,
                               bool enabled, const QString &provider = {}, const QString &model = {});
    Q_INVOKABLE bool removeBot(int id);
    Q_INVOKABLE int appendMessage(int botId, const QString &role, const QString &content,
                                  const QString &kind = QStringLiteral("chat"), int runId = -1);
    Q_INVOKABLE void clearMessages(int botId);
    Q_INVOKABLE QVariantList messagesFor(int botId) const;
    Q_INVOKABLE int createRun(int botId, const QString &task,
                              const QString &status = QStringLiteral("queued"));
    Q_INVOKABLE bool updateRun(int id, const QString &status, const QString &output,
                               const QString &error = {}, int exitCode = -1);
    Q_INVOKABLE QVariantMap runById(int id) const;
    Q_INVOKABLE QVariantList recentRuns(int botId = 0, int limit = 20) const;
    Q_INVOKABLE int createSchedule(int botId, const QString &title, const QString &prompt,
                                   int intervalMinutes);
    Q_INVOKABLE bool toggleSchedule(int id, bool enabled);
    Q_INVOKABLE bool removeSchedule(int id);
    Q_INVOKABLE QVariantList dueSchedules() const;
    Q_INVOKABLE void markScheduleRun(int id, bool success, const QString &nextRun,
                                     const QString &output);
    Q_INVOKABLE void clearError();

    static QString defaultPath();

signals:
    void changed();
    void messagesChanged();
    void runsChanged();
    void schedulesChanged();
    void selectionChanged();
    void errorChanged();

private:
    bool open();
    bool migrate();
    bool ensureColumn(const QString &table, const QString &column, const QString &definition);
    void seed();
    void loadBots();
    void loadMessages();
    void loadRuns();
    void loadSchedules();
    void loadStats();
    void setError(const QString &error);

    QString m_path;
    QString m_connection;
    QSqlDatabase m_db;
    QVariantList m_bots;
    QVariantList m_messages;
    QVariantList m_runs;
    QVariantList m_schedules;
    QVariantMap m_stats;
    int m_selectedBotId = -1;
    QString m_error;
    bool m_ready = false;
};
