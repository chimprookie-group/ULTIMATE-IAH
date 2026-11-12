#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QVariantList>

class QNetworkReply;
class QTimer;

// Runs the OpenMausBot web harness locally and bridges its stdio MCP server
// into the DeepSeek Harness.
class MausBotManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool installed READ installed NOTIFY installedChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool starting READ starting NOTIFY startingChanged)
    Q_PROPERTY(bool installing READ installing NOTIFY installingChanged)
    Q_PROPERTY(bool privilegedBusy READ privilegedBusy NOTIFY privilegedBusyChanged)
    Q_PROPERTY(QString detail READ detail NOTIFY detailChanged)
    Q_PROPERTY(QString version READ version NOTIFY versionChanged)
    Q_PROPERTY(QString url READ url NOTIFY runningChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString logTail READ logTail NOTIFY logChanged)
    Q_PROPERTY(QString installLog READ installLog NOTIFY installLogChanged)
    Q_PROPERTY(QString privilegedLog READ privilegedLog NOTIFY privilegedLogChanged)
    Q_PROPERTY(QString installReport READ installReport NOTIFY installReportChanged)
    Q_PROPERTY(bool mcpHarness READ mcpHarness NOTIFY mcpChanged)
    Q_PROPERTY(QString pairUrl READ pairUrl NOTIFY pairUrlChanged)
    Q_PROPERTY(bool pairing READ pairing NOTIFY pairingChanged)

public:
    explicit MausBotManager(QObject *parent = nullptr);
    ~MausBotManager() override;

    bool installed() const { return !m_binary.isEmpty(); }
    bool running() const { return m_running; }
    bool starting() const { return m_starting; }
    bool installing() const { return m_installing; }
    bool privilegedBusy() const { return m_privilegedBusy; }
    QString detail() const { return m_detail; }
    QString version() const { return m_version; }
    QString url() const { return QStringLiteral("http://127.0.0.1:%1/").arg(m_port); }
    QString error() const { return m_error; }
    QString logTail() const { return m_log.join('\n'); }
    QString installLog() const { return m_installLog; }
    QString privilegedLog() const { return m_privilegedLog; }
    QString installReport() const { return m_installReport; }
    bool mcpHarness() const { return m_mcpHarness; }
    QString pairUrl() const { return m_pairUrl; }
    bool pairing() const { return m_pairing; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void install();
    Q_INVOKABLE void installDependencies();
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void pair();
    Q_INVOKABLE void connectMcp();
    Q_INVOKABLE void disconnectMcp();
    Q_INVOKABLE void clearError();

signals:
    void installedChanged();
    void runningChanged();
    void startingChanged();
    void installingChanged();
    void privilegedBusyChanged();
    void detailChanged();
    void versionChanged();
    void errorChanged();
    void logChanged();
    void installLogChanged();
    void privilegedLogChanged();
    void installReportChanged();
    void mcpChanged();
    void mcpConnected();
    void mcpDisconnected();
    void pairUrlChanged();
    void pairingChanged();

private:
    void detect();
    void launchProcess();
    void probeHealth();
    void verifyInstalled();
    void cleanupInstallArtifacts();
    bool verifyDependencies(QString *error);
    void setError(const QString &error);
    void setDetail(const QString &detail);
    void setRunning(bool running);
    void setStarting(bool starting);
    void setInstalledState();
    QString harnessPatchPath() const;
    bool writeHarnessPatch(QString *error);
    void appendLog(const QString &line);

    QNetworkAccessManager *m_network = nullptr;
    QProcess m_process;
    QProcess m_installProcess;
    QProcess m_privilegedProcess;
    QProcess m_pairProcess;
    QTimer *m_healthTimer = nullptr;
    QTimer *m_startTimer = nullptr;
    QString m_binary;
    QString m_configDir;
    QString m_detail;
    QString m_version;
    QString m_error;
    QString m_installLog;
    QString m_privilegedLog;
    QString m_installReport;
    QString m_pairUrl;
    QStringList m_log;
    int m_port = 8799;
    bool m_running = false;
    bool m_starting = false;
    bool m_installing = false;
    bool m_privilegedBusy = false;
    bool m_mcpHarness = false;
    bool m_stopping = false;
    bool m_probeBeforeStart = false;
    bool m_pairing = false;
};
