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

public:
    explicit McpManager(const QString &configPath = {}, bool watchStatus = true, QObject *parent = nullptr);

    QVariantList servers() const { return m_servers; }
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    QString configPath() const { return m_configPath; }
    QString projectConfigPath() const { return m_projectConfigPath; }
    QString workspace() const { return m_workspace; }

    Q_INVOKABLE void setWorkspace(const QString &workspace);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void addServer(const QString &name, const QString &type, const QString &target,
                               const QString &scope);
    Q_INVOKABLE void removeServer(const QString &name, const QString &scope);
    Q_INVOKABLE void clearError();

    static QString defaultConfigPath();

signals:
    void serversChanged();
    void loadingChanged();
    void errorChanged();
    void projectConfigPathChanged();
    void workspaceChanged();

private:
    void loadConfig();
    bool readConfig(const QString &path, QJsonObject &root) const;
    bool writeConfig(const QString &path, const QJsonObject &root);
    void parseStatuses(const QString &output);
    void finishRefresh();
    void setLoading(bool loading);
    void setError(const QString &error);

    QString m_configPath;
    QString m_projectConfigPath;
    QString m_workspace;
    QVariantList m_servers;
    QProcess *m_process = nullptr;
    QTimer m_timeout;
    bool m_watchStatus = true;
    bool m_loading = false;
    bool m_timedOut = false;
    bool m_configOk = true;
    QString m_error;
};
