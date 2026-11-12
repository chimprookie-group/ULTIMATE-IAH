#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Reads the local development ecosystem around the workspace: installed
// tools, project type, runnable commands, installed skills and a compact
// context brief that can be sent to an agent.
class Ecosystem : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList tools READ tools NOTIFY changed)
    Q_PROPERTY(QVariantMap project READ project NOTIFY changed)
    Q_PROPERTY(QVariantList skills READ skills NOTIFY changed)
    Q_PROPERTY(QString workspace READ workspace NOTIFY changed)

public:
    explicit Ecosystem(QObject *parent = nullptr);

    QVariantList tools() const { return m_tools; }
    QVariantMap project() const { return m_project; }
    QVariantList skills() const { return m_skills; }
    QString workspace() const { return m_workspace; }

    Q_INVOKABLE void refresh(const QString &workspace);
    Q_INVOKABLE void rescan();
    Q_INVOKABLE QString brief() const;

    static QVariantMap inspectProject(const QString &workspace);
    static QVariantList inspectSkills(const QString &root);

signals:
    void changed();

private:
    QVariantList scanTools();
    QVariantList scanSkills() const;

    QString m_workspace;
    QVariantList m_tools;
    QVariantMap m_project;
    QVariantList m_skills;
    bool m_toolsLoaded = false;
};
