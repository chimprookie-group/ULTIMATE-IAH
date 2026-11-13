#pragma once

#include <QJsonArray>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Persistent native capability map. It registers every built-in tool, plugin
// tool, skill and harness component in a local SQLite knowledge base. The agent
// receives a compact map in its system prompt and can search full records on
// demand through ultimate_knowledge.
class KnowledgeBase : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList entries READ entries NOTIFY changed)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString databasePath READ databasePath CONSTANT)

public:
    explicit KnowledgeBase(const QString &path = {}, QObject *parent = nullptr);
    ~KnowledgeBase() override;

    QVariantList entries() const { return m_entries; }
    QVariantMap stats() const { return m_stats; }
    QString error() const { return m_error; }
    QString databasePath() const { return m_path; }

    void seed(const QJsonArray &tools, const QVariantList &plugins, const QVariantList &skills);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QVariantList search(const QString &query, int limit = 20) const;
    Q_INVOKABLE void clearError();
    QString promptContext(const QString &query, int maxChars = 6500) const;

    static QString defaultPath();

signals:
    void changed();
    void errorChanged();

private:
    bool open();
    bool migrate();
    void upsert(const QString &key, const QString &category, const QString &name,
                const QString &summary, const QString &details, const QString &source);
    void setError(const QString &error);

    QString m_path;
    QString m_connection;
    QVariantList m_entries;
    QVariantMap m_stats;
    QString m_error;
    bool m_ready = false;
};
