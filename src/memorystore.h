#pragma once

#include <QSqlDatabase>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Persistent project memory backed by SQLite + FTS5. One database holds facts,
// decisions, preferences, task results and notes for every workspace. Writes
// are transactional, searches use FTS5 with a LIKE fallback, and integrity is
// checked explicitly from the UI.
class MemoryStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList entries READ entries NOTIFY changed)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(QString query READ query NOTIFY queryChanged)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString databasePath READ databasePath CONSTANT)
    Q_PROPERTY(QString mcpPatchPath READ mcpPatchPath CONSTANT)

public:
    explicit MemoryStore(const QString &path = {}, QObject *parent = nullptr);
    ~MemoryStore() override;

    QVariantList entries() const { return m_entries; }
    QVariantList results() const { return m_results; }
    QString query() const { return m_query; }
    QVariantMap stats() const { return m_stats; }
    QString error() const { return m_error; }
    QString databasePath() const { return m_path; }
    QString mcpPatchPath() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void search(const QString &query, const QString &project = {});
    Q_INVOKABLE void clearSearch();
    Q_INVOKABLE int remember(const QString &project, const QString &kind,
                             const QString &title, const QString &body,
                             const QString &tags = {}, bool pinned = false,
                             int importance = 3, const QString &source = QStringLiteral("user"));
    Q_INVOKABLE bool updateEntry(int id, const QString &kind, const QString &title,
                                 const QString &body, const QString &tags,
                                 bool pinned, int importance);
    Q_INVOKABLE bool removeEntry(int id);
    Q_INVOKABLE bool togglePin(int id);
    Q_INVOKABLE QString recall(const QString &project, const QString &query = {}, int limit = 8);
    Q_INVOKABLE QVariantMap integrityCheck();
    Q_INVOKABLE bool exportTo(const QString &path);
    Q_INVOKABLE int importFrom(const QString &path);
    Q_INVOKABLE void clearError();
    Q_INVOKABLE bool ensureMcpServer();
    Q_INVOKABLE QString backupNow();

    static QString defaultPath();

signals:
    void changed();
    void resultsChanged();
    void queryChanged();
    void errorChanged();

private:
    bool open();
    bool migrate();
    bool rebuildIndex();
    void setError(const QString &error);
    QVariantMap rowFromQuery(const class QSqlQuery &query) const;
    QString formatRecall(const QVariantList &rows) const;

    QString m_path;
    QString m_connection;
    QSqlDatabase m_db;
    QVariantList m_entries;
    QVariantList m_results;
    QVariantMap m_stats;
    QString m_query;
    QString m_error;
    bool m_ready = false;
};
