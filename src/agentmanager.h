#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Named agent roles used by missions and manual launches. Built-in roles are
// stable; custom roles live in a private JSON file.
class AgentManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit AgentManager(const QString &path = {}, QObject *parent = nullptr);

    QVariantList profiles() const { return m_profiles; }
    QString error() const { return m_error; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool addProfile(const QString &name, const QString &role, const QString &prompt);
    Q_INVOKABLE bool removeProfile(const QString &id);
    Q_INVOKABLE QVariantMap profile(const QString &id) const;
    Q_INVOKABLE QString promptFor(const QString &id) const;

    static QString defaultPath();

signals:
    void changed();
    void errorChanged();

private:
    bool load();
    bool save();
    void rebuild();
    void setError(const QString &error);
    QString uniqueId(const QString &name) const;
    QVariantMap builtin(const QString &id) const;

    QString m_path;
    QVariantList m_profiles;
    QVariantList m_custom;
    QString m_error;
};
