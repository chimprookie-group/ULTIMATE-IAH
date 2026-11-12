#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class QSqlDatabase;

// Markdown vault for Ultimate IAH. Files remain plain Markdown on disk; SQLite
// is only a fast index for titles, tags, wiki-links, search and the graph.
class NoteStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString vaultPath READ vaultPath CONSTANT)
    Q_PROPERTY(QVariantList notes READ notes NOTIFY changed)
    Q_PROPERTY(QVariantList filteredNotes READ filteredNotes NOTIFY changed)
    Q_PROPERTY(QVariantList tags READ tags NOTIFY changed)
    Q_PROPERTY(QVariantList folders READ folders NOTIFY changed)
    Q_PROPERTY(QVariantList backlinks READ backlinks NOTIFY selectedChanged)
    Q_PROPERTY(QVariantMap selectedNote READ selectedNote NOTIFY selectedChanged)
    Q_PROPERTY(QString selectedPath READ selectedPath NOTIFY selectedChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY changed)
    Q_PROPERTY(QString activeTag READ activeTag WRITE setActiveTag NOTIFY changed)
    Q_PROPERTY(QVariantMap graph READ graph NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit NoteStore(const QString &vaultPath = {}, QObject *parent = nullptr);
    ~NoteStore() override;

    QString vaultPath() const { return m_vaultPath; }
    QVariantList notes() const;
    QVariantList filteredNotes() const;
    QVariantList tags() const;
    QVariantList folders() const;
    QVariantList backlinks() const;
    QVariantMap selectedNote() const;
    QString selectedPath() const { return m_selectedPath; }
    QString query() const { return m_query; }
    QString activeTag() const { return m_activeTag; }
    QVariantMap graph() const;
    int count() const;
    QString error() const { return m_error; }

    void setQuery(const QString &query);
    void setActiveTag(const QString &tag);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void selectNote(const QString &path);
    Q_INVOKABLE QString createNote(const QString &title, const QString &folder = {});
    Q_INVOKABLE bool saveNote(const QString &path, const QString &content);
    Q_INVOKABLE bool deleteNote(const QString &path);
    Q_INVOKABLE bool renameNote(const QString &path, const QString &title);
    Q_INVOKABLE bool togglePin(const QString &path);
    Q_INVOKABLE QString dailyNote();
    Q_INVOKABLE QString importAttachment(const QUrl &url);
    Q_INVOKABLE bool openWikiLink(const QString &target);
    Q_INVOKABLE QString contentOf(const QString &path) const;
    Q_INVOKABLE QVariantList searchNotes(const QString &query) const;
    Q_INVOKABLE void clearError();

    static QString defaultVaultPath();

signals:
    void changed();
    void selectedChanged();
    void errorChanged();

private:
    bool ensureSchema();
    void scanVault();
    void ensureStarter();
    bool upsertFile(const QString &path);
    bool indexNote(const QString &path, const QString &content, const QString &created);
    void removeIndex(const QString &path);
    QString parseTitle(const QString &path, const QString &content) const;
    QStringList parseTags(const QString &content) const;
    QStringList parseLinks(const QString &content) const;
    QString excerpt(const QString &content) const;
    QString notePath(const QString &path) const;
    QString uniquePath(const QString &folder, const QString &title) const;
    QString resolveLink(const QString &target) const;
    void setError(const QString &error);
    QString m_vaultPath;
    QString m_connectionName;
    QSqlDatabase *m_db = nullptr;
    QString m_selectedPath;
    QString m_query;
    QString m_activeTag;
    QString m_error;
};
