#pragma once

#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QVariantList>
#include <QTimer>

// Owns the MCP servers section of the OpenCode configuration. The config file
// is edited directly (atomic write, all other keys preserved); live statuses
// come from `opencode mcp list`, which understands OPENCODE_CONFIG.
class McpManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList servers READ servers NOTIFY serversChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)
    Q_PROPERTY(QString projectConfigPath READ projectConfigPath NOTIFY projectConfigPathChanged)
    Q_PROPERTY(QString workspace READ workspace NOTIFY workspaceChanged)
    Q_PROPERTY(bool authRunning READ authRunning NOTIFY authChanged)
    Q_PROPERTY(QString authName READ authName NOTIFY authChanged)
    Q_PROPERTY(QString authUrl READ authUrl NOTIFY authChanged)
    Q_PROPERTY(QString authOutput READ authOutput NOTIFY authChanged)
    Q_PROPERTY(QString authDetail READ authDetail NOTIFY authChanged)
    Q_PROPERTY(QString authMessage READ authMessage NOTIFY authChanged)
    Q_PROPERTY(bool authOk READ authOk NOTIFY authChanged)
    Q_PROPERTY(QString authAction READ authAction NOTIFY authChanged)

public:
    explicit McpManager(const QString &configPath = {}, bool watchStatus = true, QObject *parent = nullptr);

    QVariantList servers() const { return m_servers; }
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    QString configPath() const { return m_configPath; }
    QString projectConfigPath() const { return m_projectConfigPath; }
    QString workspace() const { return m_workspace; }
    bool authRunning() const { return m_authRunning; }
    QString authName() const { return m_authName; }
    QString authUrl() const { return m_authUrl; }
    QString authOutput() const { return m_authOutput; }
    QString authDetail() const { return m_authDetail; }
    QString authMessage() const { return m_authMessage; }
    bool authOk() const { return m_authOk; }
    QString authAction() const { return m_authAction; }

    Q_INVOKABLE void setWorkspace(const QString &workspace);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void addServer(const QString &name, const QString &type, const QString &target,
                               const QString &scope);
    Q_INVOKABLE void removeServer(const QString &name, const QString &scope);
    Q_INVOKABLE void authenticate(const QString &name, const QString &scope);
    Q_INVOKABLE void logoutServer(const QString &name, const QString &scope);
    Q_INVOKABLE void cancelAuth();
    Q_INVOKABLE void clearAuth();
    Q_INVOKABLE void clearError();

    static QString defaultConfigPath();
    static QString extractAuthUrl(const QString &output);

signals:
    void serversChanged();
    void loadingChanged();
    void errorChanged();
    void projectConfigPathChanged();
    void workspaceChanged();
    void authChanged();

private:
    void loadConfig();
    bool readConfig(const QString &path, QJsonObject &root) const;
    bool writeConfig(const QString &path, const QJsonObject &root);
    bool findServer(const QString &name, const QString &scope, QJsonObject &server,
                    QString &path) const;
    void readAuthOutput();
    void finishAuth(int exitCode, QProcess::ExitStatus status);
    void resetAuth();
    void parseStatuses(const QString &output);
    void finishRefresh();
    void setLoading(bool loading);
    void setError(const QString &error);

    QString m_configPath;
    QString m_projectConfigPath;
    QString m_workspace;
    QVariantList m_servers;
    QProcess *m_process = nullptr;
    QProcess *m_authProcess = nullptr;
    QTimer m_timeout;
    QTimer m_authTimeout;
    bool m_watchStatus = true;
    bool m_loading = false;
    bool m_timedOut = false;
    bool m_configOk = true;
    QString m_error;
    bool m_authRunning = false;
    bool m_authOk = false;
    bool m_authUrlOpened = false;
    bool m_authCancelled = false;
    bool m_authTimedOut = false;
    QString m_authAction = QStringLiteral("auth");
    QString m_authName;
    QString m_authUrl;
    QString m_authOutput;
    QString m_authDetail;
    QString m_authMessage;
};
