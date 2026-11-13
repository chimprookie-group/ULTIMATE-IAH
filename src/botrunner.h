#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>

// Executes one bot task through the native agent job runner. One process at a time, no
// shell, private agent environment, explicit permission mode, bounded output
// and a hard timeout. BotManager owns the queue around it.
class BotRunner : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString output READ output NOTIFY outputChanged)

public:
    explicit BotRunner(QObject *parent = nullptr);

    bool running() const { return m_process != nullptr; }
    QString status() const { return m_status; }
    QString output() const { return m_output; }
    int currentRunId() const { return m_runId; }

    void start(int runId, const QString &botName, const QString &prompt,
               const QString &permission, const QString &workspace,
               const QString &dshBinary, const QString &provider = {},
               const QString &model = {});
    void cancel();

signals:
    void runningChanged();
    void statusChanged();
    void outputChanged();
    void finished(int runId, bool success, const QString &output, const QString &error);
    void cancelled(int runId);

private:
    QProcessEnvironment agentEnvironment() const;
    void setStatus(const QString &status);
    void cleanup();

    QProcess *m_process = nullptr;
    QTimer m_timeout;
    int m_runId = -1;
    QString m_status;
    QString m_output;
    QString m_diagnostics;
    bool m_cancelled = false;
};
