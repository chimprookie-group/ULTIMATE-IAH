#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QVariantList>

class QNetworkReply;
class QTimer;

// Runs 9Router as a local service and connects it to the two model surfaces of
// DeepCrasy H: OpenCode (native 9Router integration endpoint) and the
// DeepSeek Harness (llm-pi-ai provider patch + NINE_ROUTER_API_KEY).
class NineRouterManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool installed READ installed NOTIFY installedChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool starting READ starting NOTIFY startingChanged)
    Q_PROPERTY(bool installing READ installing NOTIFY installingChanged)
    Q_PROPERTY(bool authRequired READ authRequired NOTIFY authRequiredChanged)
    Q_PROPERTY(QString detail READ detail NOTIFY detailChanged)
    Q_PROPERTY(QString version READ version NOTIFY versionChanged)
    Q_PROPERTY(QString dashboardUrl READ dashboardUrl NOTIFY runningChanged)
    Q_PROPERTY(QString keyMasked READ keyMasked NOTIFY keyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString logTail READ logTail NOTIFY logChanged)
    Q_PROPERTY(QString installLog READ installLog NOTIFY installLogChanged)
    Q_PROPERTY(bool privilegedBusy READ privilegedBusy NOTIFY privilegedBusyChanged)
    Q_PROPERTY(QString privilegedLog READ privilegedLog NOTIFY privilegedLogChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)
    Q_PROPERTY(QVariantList models READ models NOTIFY modelsChanged)

public:
    explicit NineRouterManager(QObject *parent = nullptr);
    ~NineRouterManager() override;

    bool installed() const { return !m_binary.isEmpty(); }
    bool running() const { return m_running; }
    bool starting() const { return m_starting; }
    bool installing() const { return m_installing; }
    bool authRequired() const { return m_authRequired; }
    QString detail() const { return m_detail; }
    QString version() const { return m_version; }
    QString dashboardUrl() const { return QStringLiteral("http://127.0.0.1:%1/dashboard").arg(m_port); }
    QString keyMasked() const { return m_keyMasked; }
    QString error() const { return m_error; }
    QString logTail() const { return m_log.join('\n'); }
    QString installLog() const { return m_installLog; }
    bool privilegedBusy() const { return m_privilegedBusy; }
    QString privilegedLog() const { return m_privilegedLog; }
    QString configPath() const { return m_configDir; }
    QVariantList models() const { return m_models; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void install();
    Q_INVOKABLE void installDependencies();
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void login(const QString &password);
    Q_INVOKABLE void applyToOpenCode(const QStringList &models);
    Q_INVOKABLE void applyToHarness(const QStringList &models, bool makeDefault);
    Q_INVOKABLE void disconnectHarness();
    Q_INVOKABLE void clearError();

signals:
    void installedChanged();
    void runningChanged();
    void startingChanged();
    void installingChanged();
    void authRequiredChanged();
    void detailChanged();
    void versionChanged();
    void keyChanged();
    void errorChanged();
    void logChanged();
    void installLogChanged();
    void privilegedBusyChanged();
    void privilegedLogChanged();
    void dependenciesInstalled();
    void modelsChanged();
    void openCodeApplied();
    void harnessApplied();
    void harnessDisconnected();

private:
    void detect();
    void setError(const QString &error);
    void setDetail(const QString &detail);
    void setRunning(bool running);
    void setStarting(bool starting);
    void setAuthRequired(bool required);
    void setKey(const QString &key);
    void setModels(const QVariantList &models);
    void appendLog(const QString &line);
    void launchProcess();
    void probeHealth();
    void authenticate();
    void authenticateWith(const QString &password);
    void loadKeyAndModels();
    void fetchModels();
    void send(const QByteArray &method, const QString &path, const QJsonObject *body,
              bool cookie, bool bearer, const std::function<void(QNetworkReply *)> &callback);
    QJsonObject readJson(QNetworkReply *reply, int *status = nullptr) const;
    QString harnessProfile() const;
    QString profilePatchPath() const;
    bool writeHarnessFiles(const QStringList &models, bool makeDefault, QString *error);
    QString pythonHelper() const;

    QNetworkAccessManager *m_network = nullptr;
    QProcess m_process;
    QProcess m_installProcess;
    QProcess m_privilegedProcess;
    QTimer *m_healthTimer = nullptr;
    QTimer *m_startTimer = nullptr;
    QString m_binary;
    QString m_configDir;
    QString m_detail;
    QString m_version;
    QString m_cookie;
    QString m_apiKey;
    QString m_keyMasked;
    QString m_error;
    QString m_installLog;
    QString m_privilegedLog;
    QStringList m_log;
    QVariantList m_models;
    int m_port = 20128;
    bool m_running = false;
    bool m_starting = false;
    bool m_installing = false;
    bool m_privilegedBusy = false;
    bool m_authRequired = false;
    bool m_stopping = false;
    bool m_probeBeforeStart = false;
};
