#pragma once

#include <QObject>
#include <QProcess>
#include <QSqlDatabase>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class AgentManager;
class MemoryStore;

// Sequential multi-agent mission: Planner -> Builder -> Reviewer. Every step
// runs through the native agent job runner in the mission workspace, gets the previous
// results and project memory, and is persisted in SQLite. One mission at a time.
class MissionRunner : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList missions READ missions NOTIFY changed)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY runningChanged)
    Q_PROPERTY(QString currentStep READ currentStep NOTIFY runningChanged)
    Q_PROPERTY(QString lastOutput READ lastOutput NOTIFY outputChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit MissionRunner(const QString &path, AgentManager *agents,
                           MemoryStore *memory, QObject *parent = nullptr);
    ~MissionRunner() override;

    QVariantList missions() const { return m_missions; }
    bool running() const { return m_process != nullptr; }
    QString currentTitle() const { return m_currentMissionTitle; }
    QString currentStep() const { return m_currentStepTitle; }
    QString lastOutput() const { return m_lastOutput; }
    QString error() const { return m_error; }

    void setDshBinary(const QString &binary) { if (!binary.trimmed().isEmpty()) m_dshBinary = binary.trimmed(); }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE int startMission(const QString &title, const QString &task,
                                 const QString &project);
    Q_INVOKABLE int startMissionWithAgents(const QString &title, const QString &task,
                                           const QString &project, const QString &plannerId,
                                           const QString &builderId, const QString &reviewerId);
    Q_INVOKABLE bool removeMission(int id);
    Q_INVOKABLE void cancel();

    static QString defaultPath();

signals:
    void changed();
    void runningChanged();
    void outputChanged();
    void errorChanged();
    void completed(int id, bool success);
    void toast(const QString &message);

private:
    bool open();
    bool migrate();
    void loadMissions();
    void startNextStep();
    void runStep(int missionId, int stepId);
    void finishStep(bool success, int exitCode);
    QString stepPrompt(const QVariantMap &mission, const QVariantMap &step,
                       const QVariantList &previous);
    QVariantList previousSteps(int missionId, int position);
    QVariantMap missionById(int id) const;
    QVariantMap stepById(int id) const;
    QProcessEnvironment agentEnvironment() const;
    void updateMission(int id, const QString &status, const QString &error = {});
    void updateStep(int id, const QString &status, const QString &output = {}, const QString &error = {});
    void setError(const QString &error);
    void stopProcess();

    QString m_path;
    QString m_connection;
    QSqlDatabase m_db;
    QVariantList m_missions;
    QTimer m_timeout;
    QProcess *m_process = nullptr;
    AgentManager *m_agents = nullptr;
    MemoryStore *m_memory = nullptr;
    int m_currentMissionId = -1;
    int m_currentStepId = -1;
    QString m_currentMissionTitle;
    QString m_currentStepTitle;
    QString m_lastOutput;
    QString m_dshBinary = QStringLiteral("dsh");
    QString m_error;
    bool m_ready = false;
    bool m_cancelled = false;
};
