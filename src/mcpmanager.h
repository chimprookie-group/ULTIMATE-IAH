#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Native management of DeepSeek Harness MCP servers. Configurations are kept
// in a private JSON file and compiled into an mcp-servers.patch.yml overlay
// consumed by HarnessServer. The built-in memory bridge stays managed by its
// owner and is shown read-only.
class McpManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList servers READ servers NOTIFY changed)
    Q_PROPERTY(QVariantList builtins READ builtins NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString patchPath READ patchPath CONSTANT)

public:
    explicit McpManager(const QString &configPath = {}, QObject *parent = nullptr);

    QVariantList servers() const { return m_servers; }
    QVariantList builtins() const { return m_builtins; }
    QString error() const { return m_error; }
    QString patchPath() const { return m_patchPath; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool addServer(const QString &name, const QString &transport,
                               const QString &command, const QString &argsText,
                               const QString &envText, const QString &url,
                               const QString &headersText);
    Q_INVOKABLE bool removeServer(const QString &id);
    Q_INVOKABLE bool setEnabled(const QString &id, bool enabled);
    Q_INVOKABLE void clearError();

    static QString defaultConfigPath();
    static QString defaultPatchPath();

signals:
    void changed();
    void errorChanged();
    void applied();

private:
    bool load();
    bool save();
    bool writePatch();
    void rebuildBuiltins();
    void setError(const QString &error);
    QString uniqueId(const QString &name) const;

    QString m_configPath;
    QString m_patchPath;
    QVariantList m_servers;
    QVariantList m_builtins;
    QString m_error;
};
