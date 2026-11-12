#pragma once

#include <QObject>
#include <QVariantList>

// Agent skills are Markdown SKILL.md files discovered by the harness. This
// manager indexes the user and project libraries and can create a new skill
// without touching existing ones.
class SkillManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList skills READ skills NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit SkillManager(const QString &rootPath = {}, QObject *parent = nullptr);

    QVariantList skills() const { return m_skills; }
    QString error() const { return m_error; }

    void setWorkspace(const QString &workspace);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool createSkill(const QString &name, const QString &description,
                                 const QString &instructions);
    Q_INVOKABLE bool removeSkill(const QString &id);
    Q_INVOKABLE void clearError();

    static QString defaultRoot();

signals:
    void changed();
    void errorChanged();

private:
    void setError(const QString &error);
    QString slug(const QString &name) const;

    QString m_root;
    QString m_workspace;
    QVariantList m_skills;
    QString m_error;
};
