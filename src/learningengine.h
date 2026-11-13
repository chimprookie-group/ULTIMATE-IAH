#pragma once

#include <QJsonObject>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class SkillManager;

// Local self-learning loop for Ultimate Agent. It records trajectories, turns
// repeated failures and successful tool sequences into durable lessons, injects
// the relevant lessons into future prompts, and proposes skills for review.
// Learning data is local SQLite; secrets are redacted before storage.
class LearningEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(QVariantList lessons READ lessons NOTIFY changed)
    Q_PROPERTY(QVariantList proposals READ proposals NOTIFY changed)
    Q_PROPERTY(QVariantList events READ events NOTIFY changed)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString databasePath READ databasePath CONSTANT)

public:
    explicit LearningEngine(const QString &path = {}, SkillManager *skills = nullptr,
                            QObject *parent = nullptr);
    ~LearningEngine() override;

    bool enabled() const { return m_enabled; }
    QVariantList lessons() const { return m_lessons; }
    QVariantList proposals() const { return m_proposals; }
    QVariantList events() const { return m_events; }
    QVariantMap stats() const { return m_stats; }
    QString error() const { return m_error; }
    QString databasePath() const { return m_path; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantMap observe(const QJsonObject &trajectory);
    Q_INVOKABLE QString contextFor(const QString &prompt, const QString &project, int limit = 4) const;
    Q_INVOKABLE bool approveProposal(int id);
    Q_INVOKABLE bool rejectProposal(int id);
    Q_INVOKABLE bool clear();
    Q_INVOKABLE void clearError();
    void setEnabled(bool value);

    static QString defaultPath();

signals:
    void changed();
    void errorChanged();

private:
    bool open();
    bool migrate();
    void setError(const QString &error);
    int upsertLesson(const QString &project, const QString &kind, const QString &title,
                     const QString &body, const QString &evidence, double confidence);
    void maybePropose(int lessonId, const QString &project, const QString &title,
                      const QString &body);
    static QString redact(QString text);
    static QString categoryOf(const QString &prompt);

    QString m_path;
    QString m_connection;
    SkillManager *m_skills = nullptr;
    QVariantList m_lessons;
    QVariantList m_proposals;
    QVariantList m_events;
    QVariantMap m_stats;
    QString m_error;
    bool m_enabled = true;
    bool m_ready = false;
};
