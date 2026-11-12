#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QVariantList>
#include <QSet>

// Native plugin registry and ArtCraft bridge. Plugins are either the built-in
// ArtCraft CLI suite or user manifests in the app data plugins/ directory.
// Every installed and enabled plugin contributes a real tool to Ultimate Agent.
class PluginManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList plugins READ plugins NOTIFY changed)
    Q_PROPERTY(int installedCount READ installedCount NOTIFY changed)
    Q_PROPERTY(int enabledCount READ enabledCount NOTIFY changed)
    Q_PROPERTY(int toolCount READ toolCount NOTIFY changed)
    Q_PROPERTY(QString artcraftPath READ artcraftPath CONSTANT)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    struct ArtCraft {
        QString id;
        QString product;
        QString binary;
        QString description;
        QStringList extensions;
    };

    explicit PluginManager(const QString &configDir = {}, QObject *parent = nullptr);

    QVariantList plugins() const { return m_plugins; }
    int installedCount() const;
    int enabledCount() const;
    int toolCount() const { return toolDefinitions().size(); }
    QString artcraftPath() const;
    QString error() const { return m_error; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool setEnabled(const QString &id, bool enabled);
    Q_INVOKABLE void openPluginFolder();
    Q_INVOKABLE void openArtcraftInstallGuide();

    QJsonArray toolDefinitions() const;
    bool isPluginTool(const QString &name) const;
    QJsonObject executeTool(const QString &name, const QJsonObject &arguments,
                            const QString &workspace, const QString &permission);
    QJsonObject runArtcraft(const QString &craft, const QString &arguments,
                            const QString &workspace, const QString &permission);

    static QString defaultConfigDir();

signals:
    void changed();
    void errorChanged();

private:
    void loadSettings();
    bool saveSettings() const;
    void setError(const QString &error);
    QString settingsPath() const;
    QString externalDir() const;
    QString findBinary(const QString &name) const;
    QVariantMap buildArtcraft(const ArtCraft &craft) const;
    QVariantMap buildExternal(const QString &path) const;
    QVariantMap pluginById(const QString &id) const;
    QJsonObject processResult(const QString &program, const QStringList &arguments,
                              const QString &workspace, int timeoutMs) const;

    QString m_configDir;
    QVariantList m_plugins;
    QSet<QString> m_disabled;
    QString m_error;
};
