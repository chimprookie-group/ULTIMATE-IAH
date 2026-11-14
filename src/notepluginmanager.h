#pragma once

#include <QHash>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class NoteStore;
class QProcess;

// Note plugins: builtin harness actions and user command manifests.
// External plugins run without a shell, receive the note path as argv[1],
// the title as argv[2] and the Markdown on stdin. Their stdout is applied
// according to the declared mode (replace, append or create).
class NotePluginManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList plugins READ plugins NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString lastOutput READ lastOutput NOTIFY changed)

public:
    struct Plugin {
        QString id;
        QString name;
        QString description;
        QString command;
        QString mode = QStringLiteral("append");
        bool readOnly = false;
        bool builtin = false;
        bool enabled = true;
    };

    explicit NotePluginManager(NoteStore *notes, const QString &workspace = {},
                               const QString &configDir = {}, QObject *parent = nullptr);
    void setWorkspace(const QString &workspace) { m_workspace = workspace; }

    QVariantList plugins() const;
    QString error() const { return m_error; }
    QString lastOutput() const { return m_lastOutput; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool setEnabled(const QString &id, bool enabled);
    Q_INVOKABLE QVariantMap run(const QString &id, const QString &path,
                                const QString &mode = {});

    static QString defaultConfigDir();

signals:
    void changed();
    void errorChanged();
    void lastOutputChanged();

private:
    void loadManifests();
    void loadSettings();
    bool saveSettings() const;
    QString settingsPath() const;
    QVariantMap runBuiltin(const Plugin &plugin, const QString &path);
    QVariantMap runCommand(const Plugin &plugin, const QString &path);
    void setError(const QString &error);
    void setLastOutput(const QString &output);

    NoteStore *m_notes = nullptr;
    QString m_workspace;
    QString m_configDir;
    QVariantList m_plugins;
    QHash<QString, bool> m_enabled;
    QString m_error;
    QString m_lastOutput;
};
