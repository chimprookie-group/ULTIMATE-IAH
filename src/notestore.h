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
    Q_PROPERTY(QVariantList outgoingLinks READ outgoingLinks NOTIFY selectedChanged)
    Q_PROPERTY(QVariantList headings READ headings NOTIFY selectedChanged)
    Q_PROPERTY(QVariantMap selectedNote READ selectedNote NOTIFY selectedChanged)
    Q_PROPERTY(QString selectedPath READ selectedPath NOTIFY selectedChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY changed)
    Q_PROPERTY(QString activeTag READ activeTag WRITE setActiveTag NOTIFY changed)
    Q_PROPERTY(QString activeFolder READ activeFolder WRITE setActiveFolder NOTIFY changed)
    Q_PROPERTY(QString activeStatus READ activeStatus WRITE setActiveStatus NOTIFY changed)
    Q_PROPERTY(QVariantMap graph READ graph NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(bool graphOpen READ graphOpen WRITE setGraphOpen NOTIFY graphOpenChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool gitAvailable READ gitAvailable NOTIFY gitChanged)
    Q_PROPERTY(QString gitBranch READ gitBranch NOTIFY gitChanged)
    Q_PROPERTY(bool gitDirty READ gitDirty NOTIFY gitChanged)
    Q_PROPERTY(QString gitRemote READ gitRemote NOTIFY gitChanged)
    Q_PROPERTY(QString gitLastCommit READ gitLastCommit NOTIFY gitChanged)
    Q_PROPERTY(bool gitTokenConfigured READ gitTokenConfigured NOTIFY gitChanged)
    Q_PROPERTY(QVariantList gitConflicts READ gitConflicts NOTIFY gitChanged)

public:
    explicit NoteStore(const QString &vaultPath = {}, QObject *parent = nullptr);
    ~NoteStore() override;

    QString vaultPath() const { return m_vaultPath; }
    QVariantList notes() const;
    QVariantList filteredNotes() const;
    QVariantList tags() const;
    QVariantList folders() const;
    QVariantList backlinks() const;
    QVariantList outgoingLinks() const;
    QVariantList headings() const;
    QVariantMap selectedNote() const;
    QString selectedPath() const { return m_selectedPath; }
    QString query() const { return m_query; }
    QString activeTag() const { return m_activeTag; }
    QString activeFolder() const { return m_activeFolder; }
    QString activeStatus() const { return m_activeStatus; }
    QVariantMap graph() const;
    int count() const;
    QString error() const { return m_error; }
    bool graphOpen() const { return m_graphOpen; }
    bool gitAvailable() const;
    QString gitBranch() const { return m_gitBranch; }
    bool gitDirty() const { return m_gitDirty; }
    QString gitRemote() const { return m_gitRemote; }
    QString gitLastCommit() const { return m_gitLastCommit; }
    bool gitTokenConfigured() const;
    QVariantList gitConflicts() const { return m_gitConflicts; }
    void setGraphOpen(bool open) {
        if (m_graphOpen == open) return;
        m_graphOpen = open;
        emit graphOpenChanged();
        if (open) refresh();
    }

    void setQuery(const QString &query);
    void setActiveTag(const QString &tag);
    void setActiveFolder(const QString &folder);
    void setActiveStatus(const QString &status);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void selectNote(const QString &path);
    Q_INVOKABLE QString createNote(const QString &title, const QString &folder = {});
    Q_INVOKABLE bool saveNote(const QString &path, const QString &content);
    Q_INVOKABLE bool deleteNote(const QString &path);
    Q_INVOKABLE bool renameNote(const QString &path, const QString &title);
    Q_INVOKABLE QString duplicateNote(const QString &path);
    Q_INVOKABLE QVariantList templates() const;
    Q_INVOKABLE QString createFromTemplate(const QString &templatePath, const QString &title);
    Q_INVOKABLE QString createTaskNote(const QString &title, const QString &prompt,
                                       const QString &workspace = {});
    Q_INVOKABLE bool setNoteStatus(const QString &path, const QString &status);
    Q_INVOKABLE bool moveNote(const QString &path, const QString &folder);
    Q_INVOKABLE bool togglePin(const QString &path);
    Q_INVOKABLE QString dailyNote();
    Q_INVOKABLE QString importAttachment(const QUrl &url);
    Q_INVOKABLE bool openWikiLink(const QString &target);
    Q_INVOKABLE QString contentOf(const QString &path) const;
    Q_INVOKABLE QVariantList searchNotes(const QString &query) const;
    Q_INVOKABLE void clearError();
    Q_INVOKABLE QString backupVault();
    Q_INVOKABLE QString exportVault(const QString &destination, bool withMetadata = true);
    Q_INVOKABLE void gitRefresh();
    Q_INVOKABLE bool gitInit();
    Q_INVOKABLE bool gitSetRemote(const QString &url);
    Q_INVOKABLE bool gitSync(const QString &message = {});
    Q_INVOKABLE bool gitPull();
    Q_INVOKABLE bool gitPush();
    Q_INVOKABLE bool gitSetToken(const QString &token);
    Q_INVOKABLE void gitClearToken();
    Q_INVOKABLE bool gitResolve(const QString &path, const QString &side);
    Q_INVOKABLE bool gitAbort();
    Q_INVOKABLE int pruneBackups(int keep = 10);

    static QString defaultVaultPath();

signals:
    void changed();
    void selectedChanged();
    void errorChanged();
    void graphOpenChanged();
    void gitChanged();

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
    QString m_activeFolder;
    QString m_activeStatus;
    QString m_error;
    bool m_graphOpen = false;
    QString m_gitBranch;
    bool m_gitDirty = false;
    QString m_gitRemote;
    QString m_gitLastCommit;
    QVariantList m_gitConflicts;
};
