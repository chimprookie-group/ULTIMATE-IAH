#include "notestore.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QHash>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QRegularExpression>
#include <algorithm>
#include <QSaveFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>

namespace {
QString nowIso() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

QString parseStatus(const QString &content) {
    const QRegularExpression expression(
        QStringLiteral("(?m)^\\s*status\\s*:\\s*(todo|doing|done)\\b"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match = expression.match(content);
    return match.hasMatch() ? match.captured(1).toLower() : QString();
}

QString withStatus(const QString &content, const QString &status) {
    if (status.isEmpty())
        return content;
    const QRegularExpression existing(QStringLiteral("(?m)^\\s*status\\s*:.*$"));
    if (existing.match(content).hasMatch())
        return content;
    if (content.startsWith(QStringLiteral("---\n"))) {
        const int close = content.indexOf(QStringLiteral("\n---"), 4);
        if (close > 0)
            return content.left(close) + QStringLiteral("\nstatus: ") + status
                + content.mid(close);
    }
    return QStringLiteral("---\nstatus: ") + status + QStringLiteral("\n---\n\n") + content;
}
QString slug(const QString &value) {
    QString slugValue = value.toLower().trimmed();
    slugValue.replace(QRegularExpression(QStringLiteral("[^a-z0-9à-ÿ_-]+")), QStringLiteral("-"));
    slugValue.remove(QRegularExpression(QStringLiteral("^-+|-+$")));
    return slugValue.isEmpty() ? QStringLiteral("note") : slugValue;
}
bool safeRelative(const QString &path) {
    return !path.isEmpty() && !path.startsWith('/') && !path.contains(QStringLiteral(".."));
}
} // namespace

NoteStore::NoteStore(const QString &vaultPath, QObject *parent)
    : QObject(parent),
      m_vaultPath(vaultPath.isEmpty() ? defaultVaultPath() : vaultPath),
      m_connectionName(QStringLiteral("notes-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces))) {
    QDir().mkpath(m_vaultPath);
    QDir().mkpath(m_vaultPath + QStringLiteral("/.ultimate-iai"));
    auto *database = new QSqlDatabase(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName));
    database->setDatabaseName(m_vaultPath + QStringLiteral("/.ultimate-iai/notes.db"));
    m_db = database;
    if (!database->open()) {
        setError(tr("Impossible d'ouvrir l'index des notes : %1").arg(database->lastError().text()));
        return;
    }
    QSqlQuery pragma(*database);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    pragma.exec(QStringLiteral("PRAGMA busy_timeout=5000"));
    ensureSchema();
    QFile::setPermissions(m_vaultPath + QStringLiteral("/.ultimate-iai/notes.db"),
                          QFile::ReadOwner | QFile::WriteOwner);
    refresh();
    ensureStarter();
    refresh();
}

NoteStore::~NoteStore() {
    if (m_db) {
        const QString connection = m_connectionName;
        delete m_db;
        m_db = nullptr;
        QSqlDatabase::removeDatabase(connection);
    }
}

QString NoteStore::defaultVaultPath() {
    const QString override = qEnvironmentVariable("ULTIMATE_IAH_VAULT");
    if (!override.trimmed().isEmpty())
        return override;
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/vault");
}

bool NoteStore::ensureSchema() {
    if (!m_db)
        return false;
    QSqlQuery query(*m_db);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS notes ("
            "path TEXT PRIMARY KEY,"
            "title TEXT NOT NULL,"
            "tags_json TEXT NOT NULL DEFAULT '[]',"
            "links_json TEXT NOT NULL DEFAULT '[]',"
            "excerpt TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL,"
            "word_count INTEGER NOT NULL DEFAULT 0,"
            "pinned INTEGER NOT NULL DEFAULT 0)"))) {
        setError(query.lastError().text());
        return false;
    }
    query.exec(QStringLiteral("ALTER TABLE notes ADD COLUMN status TEXT NOT NULL DEFAULT ''"));
    query.exec(QStringLiteral(
        "CREATE VIRTUAL TABLE IF NOT EXISTS notes_fts USING fts5(path UNINDEXED,title,content,tags)"));
    return true;
}

QVariantList NoteStore::notes() const {
    QVariantList out;
    if (!m_db)
        return out;
    QSqlQuery query(*m_db);
    if (!query.exec(QStringLiteral(
            "SELECT path,title,tags_json,links_json,excerpt,created_at,updated_at,word_count,pinned,status "
            "FROM notes ORDER BY pinned DESC, updated_at DESC"))) {
        return out;
    }
    while (query.next()) {
        out.append(QVariantMap{
            {QStringLiteral("path"), query.value(0).toString()},
            {QStringLiteral("title"), query.value(1).toString()},
            {QStringLiteral("tags"), QJsonDocument::fromJson(query.value(2).toString().toUtf8()).toVariant()},
            {QStringLiteral("links"), QJsonDocument::fromJson(query.value(3).toString().toUtf8()).toVariant()},
            {QStringLiteral("excerpt"), query.value(4).toString()},
            {QStringLiteral("createdAt"), query.value(5).toString()},
            {QStringLiteral("updatedAt"), query.value(6).toString()},
            {QStringLiteral("wordCount"), query.value(7).toInt()},
            {QStringLiteral("pinned"), query.value(8).toBool()},
            {QStringLiteral("status"), query.value(9).toString()},
        });
    }
    return out;
}

QVariantList NoteStore::filteredNotes() const {
    if (m_query.trimmed().isEmpty() && m_activeTag.isEmpty() && m_activeFolder.isEmpty())
        return notes();
    QVariantList out;
    if (!m_db)
        return out;
    QString text = m_query.trimmed();
    QString tag = m_activeTag;
    QString folder = m_activeFolder;
    QString titleFilter;
    QString pathFilter;
    QString statusFilter = m_activeStatus;
    QStringList words;
    const QStringList tokens = text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        if (token.startsWith(QStringLiteral("tag:"), Qt::CaseInsensitive))
            tag = token.mid(4);
        else if (token.startsWith(QStringLiteral("folder:"), Qt::CaseInsensitive))
            folder = token.mid(7);
        else if (token.startsWith(QStringLiteral("path:"), Qt::CaseInsensitive))
            pathFilter = token.mid(5);
        else if (token.startsWith(QStringLiteral("title:"), Qt::CaseInsensitive))
            titleFilter = token.mid(6);
        else if (token.startsWith(QStringLiteral("status:"), Qt::CaseInsensitive))
            statusFilter = token.mid(7);
        else if (token.startsWith(QLatin1Char('#')) && token.size() > 1)
            tag = token.mid(1);
        else
            words << token;
    }
    text = words.join(QLatin1Char(' '));

    QSqlQuery query(*m_db);
    QString sql = QStringLiteral(
        "SELECT path,title,tags_json,links_json,excerpt,created_at,updated_at,word_count,pinned,status "
        "FROM notes WHERE 1=1");
    if (!text.isEmpty())
        sql += QStringLiteral(" AND (title LIKE :text OR excerpt LIKE :text OR tags_json LIKE :text)");
    if (!titleFilter.isEmpty())
        sql += QStringLiteral(" AND title LIKE :title");
    if (!pathFilter.isEmpty())
        sql += QStringLiteral(" AND path LIKE :path");
    if (!tag.isEmpty())
        sql += QStringLiteral(" AND tags_json LIKE :tag");
    if (!folder.isEmpty())
        sql += QStringLiteral(" AND path LIKE :folder");
    if (!statusFilter.isEmpty())
        sql += QStringLiteral(" AND status = :status");
    sql += QStringLiteral(" ORDER BY pinned DESC, updated_at DESC");
    query.prepare(sql);
    if (!text.isEmpty())
        query.bindValue(QStringLiteral(":text"), QStringLiteral("%") + text + QStringLiteral("%"));
    if (!titleFilter.isEmpty())
        query.bindValue(QStringLiteral(":title"), QStringLiteral("%") + titleFilter + QStringLiteral("%"));
    if (!pathFilter.isEmpty())
        query.bindValue(QStringLiteral(":path"), QStringLiteral("%") + pathFilter + QStringLiteral("%"));
    if (!tag.isEmpty())
        query.bindValue(QStringLiteral(":tag"), QStringLiteral("%\"") + tag + QStringLiteral("\"%"));
    if (!folder.isEmpty())
        query.bindValue(QStringLiteral(":folder"),
                        (folder == QLatin1String("/") ? QString() : folder) + QStringLiteral("/%"));
    if (!statusFilter.isEmpty())
        query.bindValue(QStringLiteral(":status"), statusFilter);
    if (!query.exec())
        return out;
    while (query.next()) {
        out.append(QVariantMap{
            {QStringLiteral("path"), query.value(0).toString()},
            {QStringLiteral("title"), query.value(1).toString()},
            {QStringLiteral("tags"), QJsonDocument::fromJson(query.value(2).toString().toUtf8()).toVariant()},
            {QStringLiteral("links"), QJsonDocument::fromJson(query.value(3).toString().toUtf8()).toVariant()},
            {QStringLiteral("excerpt"), query.value(4).toString()},
            {QStringLiteral("createdAt"), query.value(5).toString()},
            {QStringLiteral("updatedAt"), query.value(6).toString()},
            {QStringLiteral("wordCount"), query.value(7).toInt()},
            {QStringLiteral("pinned"), query.value(8).toBool()},
            {QStringLiteral("status"), query.value(9).toString()},
        });
    }
    return out;
}

QVariantList NoteStore::tags() const {
    QHash<QString, int> counts;
    for (const QVariant &value : notes()) {
        const QVariantList list = value.toMap().value(QStringLiteral("tags")).toList();
        for (const QVariant &tag : list)
            counts[tag.toString()]++;
    }
    QList<QPair<QString, int>> sorted;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) {
        return a.second == b.second ? a.first < b.first : a.second > b.second;
    });
    QVariantList out;
    for (const auto &entry : sorted)
        out.append(QVariantMap{{QStringLiteral("name"), entry.first}, {QStringLiteral("count"), entry.second}});
    return out;
}

QVariantList NoteStore::folders() const {
    QHash<QString, int> counts;
    for (const QVariant &value : notes()) {
        const QString path = value.toMap().value(QStringLiteral("path")).toString();
        const QString folder = QFileInfo(path).path();
        counts[folder == QLatin1String(".") ? QStringLiteral("/") : folder]++;
    }
    QVariantList out;
    QList<QPair<QString, int>> sorted;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end());
    for (const auto &entry : sorted)
        out.append(QVariantMap{{QStringLiteral("name"), entry.first}, {QStringLiteral("path"), entry.first == "/" ? QString() : entry.first}, {QStringLiteral("count"), entry.second}});
    return out;
}

QVariantList NoteStore::backlinks() const {
    QVariantList out;
    if (m_selectedPath.isEmpty())
        return out;
    const QVariantMap active = selectedNote();
    const QString title = active.value(QStringLiteral("title")).toString();
    const QString base = QFileInfo(m_selectedPath).completeBaseName();
    for (const QVariant &value : notes()) {
        const QVariantMap note = value.toMap();
        if (note.value(QStringLiteral("path")).toString() == m_selectedPath)
            continue;
        for (const QVariant &link : note.value(QStringLiteral("links")).toList()) {
            const QString target = link.toString().trimmed();
            if (target.compare(title, Qt::CaseInsensitive) == 0
                || target.compare(base, Qt::CaseInsensitive) == 0
                || target.compare(m_selectedPath, Qt::CaseInsensitive) == 0
                || target.compare(QFileInfo(m_selectedPath).completeBaseName(), Qt::CaseInsensitive) == 0) {
                out.append(QVariantMap{{QStringLiteral("path"), note.value(QStringLiteral("path"))},
                                       {QStringLiteral("title"), note.value(QStringLiteral("title"))},
                                       {QStringLiteral("excerpt"), note.value(QStringLiteral("excerpt"))}});
                break;
            }
        }
    }
    return out;
}

QVariantList NoteStore::outgoingLinks() const {
    QVariantList out;
    if (m_selectedPath.isEmpty())
        return out;
    const QVariantMap active = selectedNote();
    QSet<QString> seen;
    for (const QVariant &value : active.value(QStringLiteral("links")).toList()) {
        const QString target = value.toString().trimmed();
        if (target.isEmpty() || seen.contains(target.toLower()))
            continue;
        seen.insert(target.toLower());
        const QString path = resolveLink(target);
        QString title = target;
        if (!path.isEmpty()) {
            for (const QVariant &noteValue : notes()) {
                const QVariantMap note = noteValue.toMap();
                if (note.value(QStringLiteral("path")).toString() == path) {
                    title = note.value(QStringLiteral("title")).toString();
                    break;
                }
            }
        }
        out.append(QVariantMap{{QStringLiteral("target"), target},
                               {QStringLiteral("title"), title},
                               {QStringLiteral("path"), path},
                               {QStringLiteral("exists"), !path.isEmpty()}});
    }
    return out;
}

QVariantList NoteStore::headings() const {
    QVariantList out;
    if (m_selectedPath.isEmpty())
        return out;
    const QStringList lines = contentOf(m_selectedPath).split(QLatin1Char('\n'));
    bool fenced = false;
    const QRegularExpression heading(QStringLiteral("^(#{1,6})\\s+(.+?)\\s*$"));
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i);
        if (line.trimmed().startsWith(QStringLiteral("```"))) {
            fenced = !fenced;
            continue;
        }
        if (fenced)
            continue;
        const auto match = heading.match(line);
        if (!match.hasMatch())
            continue;
        out.append(QVariantMap{{QStringLiteral("level"), match.captured(1).size()},
                               {QStringLiteral("text"), match.captured(2)},
                               {QStringLiteral("line"), i + 1}});
    }
    return out;
}

QVariantMap NoteStore::selectedNote() const {
    if (!m_db || m_selectedPath.isEmpty())
        return {};
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("SELECT path,title,tags_json,links_json,excerpt,created_at,updated_at,word_count,pinned,status FROM notes WHERE path=:path"));
    query.bindValue(QStringLiteral(":path"), m_selectedPath);
    if (!query.exec() || !query.next())
        return {};
    QVariantMap note{
        {QStringLiteral("path"), query.value(0).toString()},
        {QStringLiteral("title"), query.value(1).toString()},
        {QStringLiteral("tags"), QJsonDocument::fromJson(query.value(2).toString().toUtf8()).toVariant()},
        {QStringLiteral("links"), QJsonDocument::fromJson(query.value(3).toString().toUtf8()).toVariant()},
        {QStringLiteral("excerpt"), query.value(4).toString()},
        {QStringLiteral("createdAt"), query.value(5).toString()},
        {QStringLiteral("updatedAt"), query.value(6).toString()},
        {QStringLiteral("wordCount"), query.value(7).toInt()},
        {QStringLiteral("pinned"), query.value(8).toBool()},
        {QStringLiteral("status"), query.value(9).toString()},
        {QStringLiteral("content"), contentOf(m_selectedPath)},
    };
    return note;
}

QVariantMap NoteStore::graph() const {
    QVariantList nodes;
    QVariantList edges;
    QHash<QString, QString> byName;
    for (const QVariant &value : notes()) {
        const QVariantMap note = value.toMap();
        const QString path = note.value(QStringLiteral("path")).toString();
        byName.insert(note.value(QStringLiteral("title")).toString().toLower(), path);
        byName.insert(QFileInfo(path).completeBaseName().toLower(), path);
        nodes.append(QVariantMap{{QStringLiteral("id"), path},
                                 {QStringLiteral("title"), note.value(QStringLiteral("title"))},
                                 {QStringLiteral("tags"), note.value(QStringLiteral("tags"))},
                                 {QStringLiteral("wordCount"), note.value(QStringLiteral("wordCount"))}});
    }
    for (const QVariant &value : notes()) {
        const QVariantMap note = value.toMap();
        const QString source = note.value(QStringLiteral("path")).toString();
        for (const QVariant &link : note.value(QStringLiteral("links")).toList()) {
            QString target = link.toString().trimmed().toLower();
            target = target.section('#', 0, 0).section('|', 0, 0).trimmed();
            const QString resolved = byName.value(target);
            if (!resolved.isEmpty() && resolved != source)
                edges.append(QVariantMap{{QStringLiteral("source"), source}, {QStringLiteral("target"), resolved}});
        }
    }
    return QVariantMap{{QStringLiteral("nodes"), nodes}, {QStringLiteral("edges"), edges}};
}

int NoteStore::count() const { return notes().size(); }

void NoteStore::setQuery(const QString &query) {
    if (m_query == query)
        return;
    m_query = query;
    emit changed();
}

void NoteStore::setActiveTag(const QString &tag) {
    if (m_activeTag == tag)
        return;
    m_activeTag = tag;
    emit changed();
}

void NoteStore::setActiveStatus(const QString &status) {
    if (m_activeStatus == status)
        return;
    m_activeStatus = status;
    emit changed();
}

void NoteStore::setActiveFolder(const QString &folder) {
    if (m_activeFolder == folder)
        return;
    m_activeFolder = folder;
    emit changed();
}

void NoteStore::refresh() {
    if (!m_db)
        return;
    scanVault();
    if (m_selectedPath.isEmpty() || !QFileInfo::exists(notePath(m_selectedPath))) {
        const QVariantList list = notes();
        m_selectedPath = list.isEmpty() ? QString() : list.first().toMap().value(QStringLiteral("path")).toString();
        emit selectedChanged();
    }
    emit changed();
}

void NoteStore::scanVault() {
    QHash<QString, qint64> seen;
    QDirIterator iterator(m_vaultPath, {QStringLiteral("*.md")}, QDir::Files, QDirIterator::Subdirectories);
    QStringList dbPaths;
    while (iterator.hasNext()) {
        const QString absolute = iterator.next();
        const QString relative = QDir(m_vaultPath).relativeFilePath(absolute);
        if (relative.startsWith(QStringLiteral(".ultimate-iai/")) || relative.startsWith(QStringLiteral(".obsidian/")))
            continue;
        seen.insert(relative, QFileInfo(absolute).lastModified().toSecsSinceEpoch());
        upsertFile(relative);
    }
    QSqlQuery query(*m_db);
    query.exec(QStringLiteral("SELECT path FROM notes"));
    while (query.next())
        dbPaths.append(query.value(0).toString());
    for (const QString &path : dbPaths) {
        if (!seen.contains(path)) {
            removeIndex(path);
            if (m_selectedPath == path)
                m_selectedPath.clear();
        }
    }
}

void NoteStore::ensureStarter() {
    if (count() > 0)
        return;
    const QString content = QStringLiteral(
        "# Bienvenue dans Notes\n\n"
        "Votre vault Markdown personnel, natif dans Ultimate IAH.\n\n"
        "## Commencer\n\n"
        "- Créez une note avec **Nouvelle note**.\n"
        "- Reliez les idées avec des wikilinks : [[Guide]] ou [[Projets/Ultimate IAH]].\n"
        "- Ajoutez des tags : #bienvenue #ultimate-iah.\n"
        "- Utilisez la note quotidienne pour votre journal.\n\n"
        "> Tout est stocké localement en Markdown. Le graphe et les backlinks sont calculés par l'application.\n");
    const QString path = uniquePath({}, QStringLiteral("Bienvenue"));
    QDir().mkpath(QFileInfo(notePath(path)).absolutePath());
    QSaveFile file(notePath(path));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content.toUtf8());
        if (file.commit()) {
            indexNote(path, content, nowIso());
            m_selectedPath = path;
        }
    }
}

bool NoteStore::upsertFile(const QString &path) {
    const QFileInfo info(notePath(path));
    if (!info.isFile())
        return false;
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QString content = QString::fromUtf8(file.readAll());
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("SELECT updated_at FROM notes WHERE path=:path"));
    query.bindValue(QStringLiteral(":path"), path);
    if (query.exec() && query.next()) {
        const QDateTime stored = QDateTime::fromString(query.value(0).toString(), Qt::ISODateWithMs);
        if (stored.isValid() && stored.toSecsSinceEpoch() == info.lastModified().toSecsSinceEpoch()
            && !content.isEmpty())
            return true;
    }
    return indexNote(path, content, info.birthTime().isValid() ? info.birthTime().toString(Qt::ISODateWithMs) : nowIso());
}

bool NoteStore::indexNote(const QString &path, const QString &content, const QString &created) {
    if (!m_db)
        return false;
    const QString title = parseTitle(path, content);
    const QStringList tags = parseTags(content);
    const QStringList links = parseLinks(content);
    const QString updated = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    const QString createdValue = created.isEmpty() ? updated : created;
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO notes(path,title,tags_json,links_json,excerpt,created_at,updated_at,word_count,pinned,status) "
        "VALUES(:path,:title,:tags,:links,:excerpt,:created,:updated,:words,"
        "COALESCE((SELECT pinned FROM notes WHERE path=:path2),0),:status) "
        "ON CONFLICT(path) DO UPDATE SET title=excluded.title,tags_json=excluded.tags_json,"
        "links_json=excluded.links_json,excerpt=excluded.excerpt,updated_at=excluded.updated_at,"
        "word_count=excluded.word_count,status=excluded.status"));
    query.bindValue(QStringLiteral(":path"), path);
    query.bindValue(QStringLiteral(":path2"), path);
    query.bindValue(QStringLiteral(":title"), title);
    query.bindValue(QStringLiteral(":tags"), QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(tags)).toJson(QJsonDocument::Compact)));
    query.bindValue(QStringLiteral(":links"), QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(links)).toJson(QJsonDocument::Compact)));
    query.bindValue(QStringLiteral(":excerpt"), excerpt(content));
    query.bindValue(QStringLiteral(":created"), createdValue);
    query.bindValue(QStringLiteral(":updated"), updated);
    query.bindValue(QStringLiteral(":words"), content.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts).size());
    const QString status = parseStatus(content);
    query.bindValue(QStringLiteral(":status"), status.isNull() ? QStringLiteral("") : status);
    if (!query.exec()) {
        setError(query.lastError().text());
        return false;
    }
    QSqlQuery fts(*m_db);
    fts.prepare(QStringLiteral("DELETE FROM notes_fts WHERE path=:path"));
    fts.bindValue(QStringLiteral(":path"), path);
    fts.exec();
    fts.prepare(QStringLiteral("INSERT INTO notes_fts(path,title,content,tags) VALUES(:path,:title,:content,:tags)"));
    fts.bindValue(QStringLiteral(":path"), path);
    fts.bindValue(QStringLiteral(":title"), title);
    fts.bindValue(QStringLiteral(":content"), content);
    fts.bindValue(QStringLiteral(":tags"), tags.join(' '));
    fts.exec();
    return true;
}

void NoteStore::removeIndex(const QString &path) {
    if (!m_db)
        return;
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("DELETE FROM notes WHERE path=:path"));
    query.bindValue(QStringLiteral(":path"), path);
    query.exec();
    QSqlQuery fts(*m_db);
    fts.prepare(QStringLiteral("DELETE FROM notes_fts WHERE path=:path"));
    fts.bindValue(QStringLiteral(":path"), path);
    fts.exec();
}

void NoteStore::selectNote(const QString &path) {
    if (path == m_selectedPath || !QFileInfo::exists(notePath(path)))
        return;
    m_selectedPath = path;
    emit selectedChanged();
}

QString NoteStore::createNote(const QString &titleValue, const QString &folder) {
    const QString title = titleValue.trimmed().isEmpty() ? tr("Note sans titre") : titleValue.trimmed();
    const QString path = uniquePath(folder, title);
    if (!safeRelative(path)) {
        setError(tr("Dossier de note invalide."));
        return {};
    }
    QDir().mkpath(QFileInfo(notePath(path)).absolutePath());
    QString content = QStringLiteral("# %1\n\n").arg(title);
    QSaveFile file(notePath(path));
    if (!file.open(QIODevice::WriteOnly)) {
        setError(tr("Impossible de créer la note."));
        return {};
    }
    file.write(content.toUtf8());
    if (!file.commit()) {
        setError(tr("Impossible d'enregistrer la note."));
        return {};
    }
    indexNote(path, content, nowIso());
    m_selectedPath = path;
    emit selectedChanged();
    emit changed();
    return path;
}

bool NoteStore::saveNote(const QString &path, const QString &content) {
    if (!safeRelative(path) || !path.endsWith(QStringLiteral(".md")))
        return false;
    const QString absolute = notePath(path);
    QDir().mkpath(QFileInfo(absolute).absolutePath());
    QSaveFile file(absolute);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(tr("Impossible d'écrire la note."));
        return false;
    }
    file.write(content.toUtf8());
    if (!file.commit()) {
        setError(tr("Impossible d'enregistrer la note."));
        return false;
    }
    if (!indexNote(path, content, nowIso()))
        return false;
    m_selectedPath = path;
    emit selectedChanged();
    emit changed();
    return true;
}

QString NoteStore::duplicateNote(const QString &path) {
    if (!safeRelative(path))
        return {};
    const QString content = contentOf(path);
    if (content.isEmpty())
        return {};
    const QVariantMap current = selectedNote();
    QString title = current.value(QStringLiteral("title")).toString();
    if (title.isEmpty())
        title = QFileInfo(path).completeBaseName();
    const QString folder = QFileInfo(path).path() == QLatin1String(".") ? QString()
                                                                        : QFileInfo(path).path();
    const QString newPath = uniquePath(folder, title + tr(" copie"));
    QDir().mkpath(QFileInfo(notePath(newPath)).absolutePath());
    QSaveFile file(notePath(newPath));
    if (!file.open(QIODevice::WriteOnly))
        return {};
    file.write(content.toUtf8());
    if (!file.commit())
        return {};
    indexNote(newPath, content, nowIso());
    m_selectedPath = newPath;
    emit selectedChanged();
    emit changed();
    return newPath;
}

bool NoteStore::moveNote(const QString &path, const QString &folderValue) {
    if (!safeRelative(path))
        return false;
    const QString folder = folderValue.trimmed();
    if (!folder.isEmpty() && !safeRelative(folder + QStringLiteral("/placeholder.md"))) {
        setError(tr("Dossier de destination invalide."));
        return false;
    }
    const QString content = contentOf(path);
    if (content.isEmpty())
        return false;
    const QString fileName = QFileInfo(path).fileName();
    const QString newPath = (folder.isEmpty() ? QString() : folder + QLatin1Char('/')) + fileName;
    if (newPath == path)
        return true;
    if (QFileInfo::exists(notePath(newPath))) {
        setError(tr("Une note existe déjà à cet emplacement."));
        return false;
    }
    QDir().mkpath(QFileInfo(notePath(newPath)).absolutePath());
    if (!QFile::rename(notePath(path), notePath(newPath)))
        return false;
    removeIndex(path);
    indexNote(newPath, content, nowIso());
    m_selectedPath = newPath;
    emit selectedChanged();
    emit changed();
    return true;
}

QVariantList NoteStore::templates() const {
    QVariantList out;
    const QDir directory(m_vaultPath + QStringLiteral("/Templates"));
    if (!directory.exists())
        return out;
    const QFileInfoList files = directory.entryInfoList({QStringLiteral("*.md")}, QDir::Files, QDir::Name);
    for (const QFileInfo &file : files) {
        out.append(QVariantMap{{QStringLiteral("name"), file.completeBaseName()},
                               {QStringLiteral("path"), QStringLiteral("Templates/") + file.fileName()}});
    }
    return out;
}

QString NoteStore::createFromTemplate(const QString &templatePath, const QString &titleValue) {
    if (!safeRelative(templatePath))
        return {};
    QString content = contentOf(templatePath);
    if (content.isEmpty())
        return {};
    const QString title = titleValue.trimmed().isEmpty() ? tr("Nouvelle note") : titleValue.trimmed();
    content.replace(QStringLiteral("{{title}}"), title);
    content.replace(QStringLiteral("{{date}}"), QDate::currentDate().toString(Qt::ISODate));
    const QString path = uniquePath({}, title);
    QDir().mkpath(QFileInfo(notePath(path)).absolutePath());
    QSaveFile file(notePath(path));
    if (!file.open(QIODevice::WriteOnly))
        return {};
    file.write(content.toUtf8());
    if (!file.commit())
        return {};
    indexNote(path, content, nowIso());
    m_selectedPath = path;
    emit selectedChanged();
    emit changed();
    return path;
}

QString NoteStore::createTaskNote(const QString &titleValue, const QString &prompt,
                                  const QString &workspace) {
    const QString title = titleValue.trimmed().isEmpty() ? tr("Nouvelle tâche") : titleValue.trimmed();
    const QString path = uniquePath({}, title);
    QString content = QStringLiteral("---\nstatus: todo\n");
    if (!workspace.trimmed().isEmpty())
        content += QStringLiteral("workspace: %1\n").arg(workspace.trimmed());
    content += QStringLiteral("created: %1\n---\n\n").arg(QDate::currentDate().toString(Qt::ISODate));
    content += QStringLiteral("# %1\n\n## Demande\n\n%2\n\n## Journal\n\n")
                   .arg(title, prompt.trimmed());
    QDir().mkpath(QFileInfo(notePath(path)).absolutePath());
    QSaveFile file(notePath(path));
    if (!file.open(QIODevice::WriteOnly)) {
        setError(tr("Impossible de créer la tâche."));
        return {};
    }
    file.write(content.toUtf8());
    if (!file.commit()) {
        setError(tr("Impossible d'enregistrer la tâche."));
        return {};
    }
    indexNote(path, content, nowIso());
    m_selectedPath = path;
    emit selectedChanged();
    emit changed();
    return path;
}

bool NoteStore::setNoteStatus(const QString &path, const QString &status) {
    if (!safeRelative(path) || status.isEmpty())
        return false;
    const QString content = contentOf(path);
    if (content.isEmpty())
        return false;
    QString updated = content;
    const QRegularExpression expression(QStringLiteral("(?m)^\\s*status\\s*:.*$"));
    if (expression.match(updated).hasMatch())
        updated.replace(expression, QStringLiteral("status: ") + status);
    else
        updated = withStatus(updated, status);
    return saveNote(path, updated);
}

bool NoteStore::deleteNote(const QString &path) {
    if (!safeRelative(path))
        return false;
    const QString absolute = notePath(path);
    if (!QFileInfo::exists(absolute))
        return false;
    const QString trash = m_vaultPath + QStringLiteral("/.ultimate-iai/trash/")
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-")) + QFileInfo(path).fileName();
    QDir().mkpath(QFileInfo(trash).absolutePath());
    if (!QFile::rename(absolute, trash)) {
        setError(tr("Impossible de déplacer la note vers la corbeille."));
        return false;
    }
    removeIndex(path);
    if (m_selectedPath == path)
        m_selectedPath.clear();
    refresh();
    return true;
}

bool NoteStore::renameNote(const QString &path, const QString &titleValue) {
    if (!safeRelative(path) || titleValue.trimmed().isEmpty())
        return false;
    QString content = contentOf(path);
    if (content.isEmpty())
        return false;
    const QString title = titleValue.trimmed();
    const QRegularExpression heading(QStringLiteral("^#\\s+.*$"), QRegularExpression::MultilineOption);
    if (heading.match(content).hasMatch())
        content.replace(heading, QStringLiteral("# ") + title);
    else
        content.prepend(QStringLiteral("# %1\n\n").arg(title));
    const QString folder = QFileInfo(path).path() == QLatin1String(".") ? QString() : QFileInfo(path).path();
    const QString newPath = uniquePath(folder, title);
    QDir().mkpath(QFileInfo(notePath(newPath)).absolutePath());
    QSaveFile file(notePath(newPath));
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(content.toUtf8());
    if (!file.commit())
        return false;
    if (newPath != path) {
        QFile::remove(notePath(path));
        removeIndex(path);
    }
    indexNote(newPath, content, nowIso());
    m_selectedPath = newPath;
    emit selectedChanged();
    emit changed();
    return true;
}

bool NoteStore::togglePin(const QString &path) {
    if (!m_db)
        return false;
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("UPDATE notes SET pinned = CASE pinned WHEN 1 THEN 0 ELSE 1 END WHERE path=:path"));
    query.bindValue(QStringLiteral(":path"), path);
    if (!query.exec())
        return false;
    emit changed();
    return true;
}

QString NoteStore::dailyNote() {
    const QString folder = QStringLiteral("Daily");
    const QString title = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    const QString path = folder + QStringLiteral("/") + title + QStringLiteral(".md");
    if (QFileInfo::exists(notePath(path))) {
        selectNote(path);
        return path;
    }
    QDir().mkpath(QFileInfo(notePath(path)).absolutePath());
    const QString content = QStringLiteral("# %1\n\n## Notes\n\n\n## Tâches\n\n- [ ] \n\n## Journal\n\n").arg(title);
    QSaveFile file(notePath(path));
    if (!file.open(QIODevice::WriteOnly))
        return {};
    file.write(content.toUtf8());
    if (!file.commit())
        return {};
    indexNote(path, content, nowIso());
    m_selectedPath = path;
    emit selectedChanged();
    emit changed();
    return path;
}

QString NoteStore::importAttachment(const QUrl &url) {
    const QString source = url.isLocalFile() ? url.toLocalFile() : url.toString();
    QFileInfo info(source);
    if (!info.isFile())
        return {};
    QImageReader reader(source);
    if (!reader.canRead()) {
        setError(tr("Ce fichier n'est pas une image lisible."));
        return {};
    }
    const QString folder = m_vaultPath + QStringLiteral("/assets");
    QDir().mkpath(folder);
    const QString target = folder + QStringLiteral("/")
        + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral("-") + info.fileName();
    if (!QFile::copy(source, target))
        return {};
    return QStringLiteral("![](assets/%1)").arg(QFileInfo(target).fileName());
}

bool NoteStore::openWikiLink(const QString &target) {
    const QString resolved = resolveLink(target);
    if (resolved.isEmpty())
        return false;
    selectNote(resolved);
    return true;
}

QString NoteStore::contentOf(const QString &path) const {
    QFile file(notePath(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
}

QVariantList NoteStore::searchNotes(const QString &query) const {
    QVariantList out;
    if (!m_db)
        return out;
    const QString value = query.trimmed();
    QSqlQuery sql(*m_db);
    if (value.isEmpty()) {
        sql.exec(QStringLiteral("SELECT path,title,tags_json,links_json,excerpt,created_at,updated_at,word_count,pinned FROM notes ORDER BY pinned DESC, updated_at DESC"));
    } else {
        sql.prepare(QStringLiteral("SELECT path,title,tags_json,links_json,excerpt,created_at,updated_at,word_count,pinned FROM notes WHERE title LIKE :q OR excerpt LIKE :q OR tags_json LIKE :q ORDER BY pinned DESC, updated_at DESC"));
        sql.bindValue(QStringLiteral(":q"), QStringLiteral("%") + value + QStringLiteral("%"));
        sql.exec();
    }
    while (sql.next()) {
        out.append(QVariantMap{
            {QStringLiteral("path"), sql.value(0).toString()},
            {QStringLiteral("title"), sql.value(1).toString()},
            {QStringLiteral("tags"), QJsonDocument::fromJson(sql.value(2).toString().toUtf8()).toVariant()},
            {QStringLiteral("links"), QJsonDocument::fromJson(sql.value(3).toString().toUtf8()).toVariant()},
            {QStringLiteral("excerpt"), sql.value(4).toString()},
            {QStringLiteral("updatedAt"), sql.value(6).toString()},
            {QStringLiteral("wordCount"), sql.value(7).toInt()},
            {QStringLiteral("pinned"), sql.value(8).toBool()},
        });
    }
    return out;
}

void NoteStore::clearError() { setError({}); }

QString NoteStore::parseTitle(const QString &path, const QString &content) const {
    const QRegularExpression heading(QStringLiteral("^#\\s+(.+)$"), QRegularExpression::MultilineOption);
    const auto match = heading.match(content);
    if (match.hasMatch())
        return match.captured(1).trimmed();
    return QFileInfo(path).completeBaseName();
}

QStringList NoteStore::parseTags(const QString &content) const {
    QStringList tags;
    const QRegularExpression expression(QStringLiteral("(?:^|[\\s(])#([A-Za-z0-9_./-]+)"));
    auto iterator = expression.globalMatch(content);
    while (iterator.hasNext()) {
        const QString tag = iterator.next().captured(1);
        if (!tag.isEmpty() && !tags.contains(tag))
            tags.append(tag);
    }
    tags.sort(Qt::CaseInsensitive);
    return tags;
}

QStringList NoteStore::parseLinks(const QString &content) const {
    QStringList links;
    const QRegularExpression expression(QStringLiteral("\\[\\[([^\\]|#]+)(?:#[^\\]|]+)?(?:\\|[^\\]]+)?\\]\\]"));
    auto iterator = expression.globalMatch(content);
    while (iterator.hasNext()) {
        const QString link = iterator.next().captured(1).trimmed();
        if (!link.isEmpty() && !links.contains(link))
            links.append(link);
    }
    return links;
}

QString NoteStore::excerpt(const QString &content) const {
    QString text = content;
    text.remove(QRegularExpression(QStringLiteral("```[\\s\\S]*?```")));
    text.remove(QRegularExpression(QStringLiteral("!\\[[^\\]]*\\]\\([^)]*\\)")));
    text.remove(QRegularExpression(QStringLiteral("\\[\\[([^\\]|]+)(?:\\|[^\\]]+)?\\]\\]")));
    text.replace(QRegularExpression(QStringLiteral("[#>*_`~-]+")), QStringLiteral(" "));
    return text.simplified().left(220);
}

QString NoteStore::notePath(const QString &path) const {
    return m_vaultPath + QStringLiteral("/") + path;
}

QString NoteStore::uniquePath(const QString &folder, const QString &title) const {
    const QString directory = folder.trimmed().isEmpty() ? QString() : folder.trimmed() + QStringLiteral("/");
    const QString base = slug(title);
    QString candidate = directory + base + QStringLiteral(".md");
    int suffix = 2;
    while (QFileInfo::exists(notePath(candidate)))
        candidate = directory + base + QStringLiteral("-") + QString::number(suffix++) + QStringLiteral(".md");
    return candidate;
}

QString NoteStore::resolveLink(const QString &target) const {
    const QString normal = target.trimmed().section('#', 0, 0).section('|', 0, 0).trimmed().toLower();
    if (normal.isEmpty())
        return {};
    for (const QVariant &value : notes()) {
        const QVariantMap note = value.toMap();
        const QString path = note.value(QStringLiteral("path")).toString();
        if (note.value(QStringLiteral("title")).toString().toLower() == normal
            || QFileInfo(path).completeBaseName().toLower() == normal
            || path.toLower() == normal + QStringLiteral(".md")
            || path.toLower() == normal)
            return path;
    }
    return {};
}

void NoteStore::setError(const QString &error) {
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}
