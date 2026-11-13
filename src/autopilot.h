#pragma once

#include <QObject>
#include <QProcess>
#include <QSqlDatabase>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class MemoryStore;

// Bounded autonomous executor. Jobs are stored in SQLite, run one at a time
// through the native agent job runner (never a shell), inherit the private harness
// environment, receive relevant memory as context, and record their results
// back into memory. Global autonomy is opt-in.
class Autopilot : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList jobs READ jobs NOTIFY changed)
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int currentJobId READ currentJobId NOTIFY runningChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY runningChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString lastOutput READ lastOutput NOTIFY outputChanged)
    Q_PROPERTY(QString databasePath READ databasePath CONSTANT)

public:
    explicit Autopilot(const QString &path = {}, QObject *parent = nullptr);
    ~Autopilot() override;

    QVariantList jobs() const { return m_jobs; }
    bool enabled() const { return m_enabled; }
    bool running() const { return m_process != nullptr; }
    int currentJobId() const { return m_currentJobId; }
    QString currentTitle() const { return m_currentTitle; }
    QString error() const { return m_error; }
    QString lastOutput() const { return m_lastOutput; }
    QString databasePath() const { return m_path; }

    void setMemory(MemoryStore *memory) { m_memory = memory; }
    Q_INVOKABLE void setDshBinary(const QString &binary);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE int addJob(const QString &title, const QString &prompt,
                           int intervalMinutes, const QString &project,
                           int maxAttempts = 3);
    Q_INVOKABLE bool removeJob(int id);
    Q_INVOKABLE void runNow(int id);
    Q_INVOKABLE void toggleJob(int id);
    Q_INVOKABLE void setEnabled(bool enabled);
    Q_INVOKABLE void clearError();

    static QString defaultPath();

signals:
    void changed();
    void enabledChanged();
    void runningChanged();
    void errorChanged();
    void outputChanged();
    void completed(int id, bool success, const QString &summary);
    void toast(const QString &message);

private:
    bool open();
    bool migrate();
    void loadJobs();
    void loadEnabled();
    void saveEnabled();
    void startJob(int id);
    void scheduleNext(int id, int intervalMinutes, bool success);
    void updateJobState(int id, const QString &status, const QString &error = {});
    QVariantMap jobById(int id) const;
    QProcessEnvironment agentEnvironment() const;
    QString enrichedPrompt(const QVariantMap &job) const;
    QString projectOfJob(int id) const;
    void setError(const QString &error);
    void setRunning(bool running, int id = -1, const QString &title = {});

    QString m_path;
    QString m_connection;
    QSqlDatabase m_db;
    QVariantList m_jobs;
    QTimer m_timer;
    QProcess *m_process = nullptr;
    QTimer m_timeout;
    MemoryStore *m_memory = nullptr;
    QString m_dshBinary = QStringLiteral("dsh");
    QString m_error;
    QString m_lastOutput;
    QString m_currentTitle;
    int m_currentJobId = -1;
    int m_currentAttempts = 0;
    int m_currentMaxAttempts = 3;
    bool m_enabled = false;
    bool m_ready = false;
};
