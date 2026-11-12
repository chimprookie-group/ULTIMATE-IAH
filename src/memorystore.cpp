#include "memorystore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSaveFile>

namespace {
int connectionCounter = 0;
QString nowIso() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); }

QStringList queryTokens(const QString &query) {
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    QStringList tokens = query.split(whitespace, Qt::SkipEmptyParts);
    for (QString &token : tokens) {
        token.remove(QRegularExpression(QStringLiteral("[\\\"'()*:^]")));
        if (!token.isEmpty())
            token = QStringLiteral("\"") + token + QStringLiteral("\"*");
    }
    return tokens;
}
} // namespace

MemoryStore::MemoryStore(const QString &path, QObject *parent)
    : QObject(parent),
      m_path(path.isEmpty() ? defaultPath() : path),
      m_connection(QStringLiteral("memory-%1").arg(++connectionCounter)) {
    if (open()) {
        refresh();
        if (m_path == defaultPath())
            ensureMcpServer();
    }
}

MemoryStore::~MemoryStore() {
    if (m_db.isValid()) {
        const QString name = m_db.connectionName();
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
    }
}

QString MemoryStore::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/memory.db";
}

QString MemoryStore::mcpPatchPath() const {
    // HarnessServer explicitly adds patches from the app config directory.
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + "/mcp-memory.patch.yml";
}

QString MemoryStore::backupNow() {
    if (!m_ready)
        return {};
    QString backup = QFileInfo(m_path).absolutePath() + "/memory-backup.db";
    QFile::remove(backup);
    QSqlQuery checkpoint(m_db);
    checkpoint.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE)"));
    checkpoint.finish();
    QString escaped = backup;
    escaped.replace('\'', QStringLiteral("''"));
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("VACUUM INTO '%1'").arg(escaped))) {
        setError(query.lastError().text());
        return {};
    }
    QFile::setPermissions(backup, QFile::ReadOwner | QFile::WriteOwner);
    return backup;
}

bool MemoryStore::ensureMcpServer() {
    const QDir directory = QFileInfo(m_path).absoluteDir();
    const QString script = directory.filePath(QStringLiteral("memory_mcp.py"));
    QFile resource(QStringLiteral(":/mcp/memory_mcp.py"));
    if (!resource.open(QIODevice::ReadOnly)) {
        setError(tr("Le serveur MCP mémoire est absent des ressources."));
        return false;
    }
    const QByteArray content = resource.readAll();
    QSaveFile scriptFile(script);
    if (!scriptFile.open(QIODevice::WriteOnly)) {
        setError(tr("Impossible d'écrire le serveur MCP mémoire."));
        return false;
    }
    scriptFile.write(content);
    if (!scriptFile.commit()) {
        setError(tr("Impossible d'enregistrer le serveur MCP mémoire."));
        return false;
    }
    QFile::setPermissions(script, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    const auto quote = [](const QString &value) {
        QString escaped = value;
        escaped.replace('\\', QStringLiteral("\\\\"));
        escaped.replace('"', QStringLiteral("\\\""));
        return QStringLiteral("\"") + escaped + QStringLiteral("\"");
    };
    const QString patch =
        QStringLiteral("- insert:\n")
        + QStringLiteral("    - id: mcp-crasy-memory\n")
        + QStringLiteral("      name: \"@deepseek-ai/dsh-mcp-client\"\n")
        + QStringLiteral("      config:\n")
        + QStringLiteral("        serverName: crasy-memory\n")
        + QStringLiteral("        transport: stdio\n")
        + QStringLiteral("        command: \"python3\"\n")
        + QStringLiteral("        args: [") + quote(script) + QStringLiteral("]\n")
        + QStringLiteral("        env:\n")
        + QStringLiteral("          CRASY_MEMORY_DB: ") + quote(m_path) + QStringLiteral("\n");
    QDir().mkpath(QFileInfo(mcpPatchPath()).absolutePath());
    QSaveFile patchFile(mcpPatchPath());
    if (!patchFile.open(QIODevice::WriteOnly)) {
        setError(tr("Impossible d'écrire le patch MCP mémoire."));
        return false;
    }
    patchFile.write(patch.toUtf8());
    if (!patchFile.commit()) {
        setError(tr("Impossible d'enregistrer le patch MCP mémoire."));
        return false;
    }
    QFile::setPermissions(mcpPatchPath(), QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

bool MemoryStore::open() {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
    m_db.setDatabaseName(m_path);
    if (!m_db.open()) {
        setError(tr("Impossible d'ouvrir la mémoire : %1").arg(m_db.lastError().text()));
        return false;
    }
    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    pragma.exec(QStringLiteral("PRAGMA busy_timeout=5000"));
    m_ready = migrate();
    QFile::setPermissions(m_path, QFile::ReadOwner | QFile::WriteOwner);
    return m_ready;
}

bool MemoryStore::migrate() {
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS memories ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "project TEXT NOT NULL DEFAULT '',"
            "kind TEXT NOT NULL DEFAULT 'note',"
            "title TEXT NOT NULL,"
            "body TEXT NOT NULL DEFAULT '',"
            "tags TEXT NOT NULL DEFAULT '',"
            "pinned INTEGER NOT NULL DEFAULT 0,"
            "importance INTEGER NOT NULL DEFAULT 3,"
            "source TEXT NOT NULL DEFAULT 'user',"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL)"))) {
        setError(query.lastError().text());
        return false;
    }
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS memories_project ON memories(project)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS memories_updated ON memories(updated_at)"));
    query.exec(QStringLiteral(
        "CREATE VIRTUAL TABLE IF NOT EXISTS memories_fts USING fts5("
        "title, body, tags, content='memories', content_rowid='id',"
        "tokenize='unicode61 remove_diacritics 2')"));
    query.exec(QStringLiteral(
        "CREATE TRIGGER IF NOT EXISTS memories_ai AFTER INSERT ON memories BEGIN "
        "INSERT INTO memories_fts(rowid,title,body,tags) VALUES (new.id,new.title,new.body,new.tags); END"));
    query.exec(QStringLiteral(
        "CREATE TRIGGER IF NOT EXISTS memories_ad AFTER DELETE ON memories BEGIN "
        "INSERT INTO memories_fts(memories_fts,rowid,title,body,tags) VALUES('delete',old.id,old.title,old.body,old.tags); END"));
    query.exec(QStringLiteral(
        "CREATE TRIGGER IF NOT EXISTS memories_au AFTER UPDATE ON memories BEGIN "
        "INSERT INTO memories_fts(memories_fts,rowid,title,body,tags) VALUES('delete',old.id,old.title,old.body,old.tags);"
        "INSERT INTO memories_fts(rowid,title,body,tags) VALUES (new.id,new.title,new.body,new.tags); END"));
    query.exec(QStringLiteral("PRAGMA user_version=1"));
    return true;
}

bool MemoryStore::rebuildIndex() {
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("INSERT INTO memories_fts(memories_fts) VALUES('rebuild')"))) {
        setError(query.lastError().text());
        return false;
    }
    return true;
}

void MemoryStore::refresh() {
    if (!m_ready)
        return;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT id,project,kind,title,body,tags,pinned,importance,source,created_at,updated_at "
        "FROM memories ORDER BY pinned DESC, updated_at DESC LIMIT 300"));
    QVariantList rows;
    if (query.exec()) {
        while (query.next())
            rows.append(rowFromQuery(query));
    } else {
        setError(query.lastError().text());
    }
    m_entries = rows;

    QSqlQuery statsQuery(m_db);
    int total = 0, pinned = 0, projects = 0;
    if (statsQuery.exec(QStringLiteral("SELECT COUNT(*), SUM(pinned), COUNT(DISTINCT project) FROM memories"))
        && statsQuery.next()) {
        total = statsQuery.value(0).toInt();
        pinned = statsQuery.value(1).toInt();
        projects = statsQuery.value(2).toInt();
    }
    m_stats = QVariantMap{{"total", total}, {"pinned", pinned},
                          {"projects", projects},
                          {"bytes", QFileInfo(m_path).size()},
                          {"path", m_path}};
    setError({});
    emit changed();
}

void MemoryStore::search(const QString &queryText, const QString &project) {
    if (!m_ready)
        return;
    const QString clean = queryText.trimmed();
    m_query = clean;
    emit queryChanged();
    if (clean.isEmpty()) {
        clearSearch();
        return;
    }
    QVariantList rows;
    QSqlQuery query(m_db);
    const QStringList tokens = queryTokens(clean);
    const QString match = tokens.join(QStringLiteral(" AND "));
    if (!match.isEmpty()) {
        query.prepare(QStringLiteral(
            "SELECT m.id,m.project,m.kind,m.title,m.body,m.tags,m.pinned,m.importance,m.source,m.created_at,m.updated_at "
            "FROM memories_fts f JOIN memories m ON m.id=f.rowid "
            "WHERE memories_fts MATCH ? AND (? = '' OR m.project = ?) "
            "ORDER BY m.pinned DESC, bm25(memories_fts), m.updated_at DESC LIMIT 120"));
        query.addBindValue(match);
        query.addBindValue(project);
        query.addBindValue(project);
        if (!query.exec()) {
            query.finish();
            query.prepare(QStringLiteral(
                "SELECT id,project,kind,title,body,tags,pinned,importance,source,created_at,updated_at "
                "FROM memories WHERE (title LIKE ? OR body LIKE ? OR tags LIKE ?) "
                "AND (? = '' OR project = ?) ORDER BY pinned DESC, updated_at DESC LIMIT 120"));
            const QString like = QStringLiteral("%") + clean + QStringLiteral("%");
            query.addBindValue(like);
            query.addBindValue(like);
            query.addBindValue(like);
            query.addBindValue(project);
            query.addBindValue(project);
            query.exec();
        }
    }
    while (query.next())
        rows.append(rowFromQuery(query));
    m_results = rows;
    emit resultsChanged();
}

void MemoryStore::clearSearch() {
    m_query.clear();
    m_results.clear();
    emit queryChanged();
    emit resultsChanged();
}

int MemoryStore::remember(const QString &project, const QString &kind, const QString &title,
                          const QString &body, const QString &tags, bool pinned,
                          int importance, const QString &source) {
    if (!m_ready || title.trimmed().isEmpty()) {
        setError(tr("Le titre de la mémoire est vide."));
        return -1;
    }
    const QString timestamp = nowIso();
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO memories(project,kind,title,body,tags,pinned,importance,source,created_at,updated_at) "
        "VALUES(?,?,?,?,?,?,?,?,?,?)"));
    query.addBindValue(project);
    query.addBindValue(kind);
    query.addBindValue(title.trimmed());
    query.addBindValue(body.trimmed());
    query.addBindValue(tags.trimmed());
    query.addBindValue(pinned ? 1 : 0);
    query.addBindValue(qBound(0, importance, 5));
    query.addBindValue(source);
    query.addBindValue(timestamp);
    query.addBindValue(timestamp);
    if (!query.exec()) {
        setError(query.lastError().text());
        return -1;
    }
    const int id = query.lastInsertId().toInt();
    setError({});
    refresh();
    return id;
}

bool MemoryStore::updateEntry(int id, const QString &kind, const QString &title,
                              const QString &body, const QString &tags,
                              bool pinned, int importance) {
    if (!m_ready || title.trimmed().isEmpty())
        return false;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE memories SET kind=?,title=?,body=?,tags=?,pinned=?,importance=?,updated_at=? WHERE id=?"));
    query.addBindValue(kind);
    query.addBindValue(title.trimmed());
    query.addBindValue(body.trimmed());
    query.addBindValue(tags.trimmed());
    query.addBindValue(pinned ? 1 : 0);
    query.addBindValue(qBound(0, importance, 5));
    query.addBindValue(nowIso());
    query.addBindValue(id);
    if (!query.exec()) {
        setError(query.lastError().text());
        return false;
    }
    refresh();
    return true;
}

bool MemoryStore::removeEntry(int id) {
    if (!m_ready)
        return false;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM memories WHERE id=?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(query.lastError().text());
        return false;
    }
    refresh();
    return true;
}

bool MemoryStore::togglePin(int id) {
    if (!m_ready)
        return false;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE memories SET pinned=1-pinned, updated_at=? WHERE id=?"));
    query.addBindValue(nowIso());
    query.addBindValue(id);
    if (!query.exec()) {
        setError(query.lastError().text());
        return false;
    }
    refresh();
    return true;
}

QString MemoryStore::recall(const QString &project, const QString &queryText, int limit) {
    if (!m_ready)
        return {};
    QVariantList rows;
    QSqlQuery query(m_db);
    const QStringList tokens = queryTokens(queryText);
    if (!tokens.isEmpty()) {
        query.prepare(QStringLiteral(
            "SELECT m.id,m.project,m.kind,m.title,m.body,m.tags,m.pinned,m.importance,m.source,m.created_at,m.updated_at "
            "FROM memories_fts f JOIN memories m ON m.id=f.rowid "
            "WHERE memories_fts MATCH ? AND (? = '' OR m.project = ?) "
            "ORDER BY m.pinned DESC, bm25(memories_fts), m.updated_at DESC LIMIT ?"));
        query.addBindValue(tokens.join(QStringLiteral(" AND ")));
        query.addBindValue(project);
        query.addBindValue(project);
        query.addBindValue(qBound(1, limit, 20));
        query.exec();
        while (query.next())
            rows.append(rowFromQuery(query));
    }
    if (rows.size() < limit) {
        QSqlQuery pinned(m_db);
        pinned.prepare(QStringLiteral(
            "SELECT id,project,kind,title,body,tags,pinned,importance,source,created_at,updated_at "
            "FROM memories WHERE (? = '' OR project = ?) AND pinned=1 ORDER BY updated_at DESC LIMIT ?"));
        pinned.addBindValue(project);
        pinned.addBindValue(project);
        pinned.addBindValue(limit - rows.size());
        pinned.exec();
        while (pinned.next()) {
            const QVariantMap row = rowFromQuery(pinned);
            bool duplicate = false;
            for (const QVariant &value : rows)
                if (value.toMap().value("id").toInt() == row.value("id").toInt())
                    duplicate = true;
            if (!duplicate)
                rows.append(row);
        }
    }
    return formatRecall(rows);
}

QString MemoryStore::formatRecall(const QVariantList &rows) const {
    if (rows.isEmpty())
        return {};
    QStringList lines;
    int size = 0;
    for (const QVariant &value : rows) {
        const QVariantMap row = value.toMap();
        QString body = row.value("body").toString().simplified();
        if (body.size() > 420)
            body = body.left(420) + QStringLiteral("…");
        const QString line = QStringLiteral("- [%1] %2 : %3")
                                 .arg(row.value("kind").toString(),
                                      row.value("title").toString(), body);
        size += line.size();
        if (size > 6000)
            break;
        lines << line;
    }
    return lines.join('\n');
}

QVariantMap MemoryStore::rowFromQuery(const QSqlQuery &query) const {
    QVariantMap row;
    row["id"] = query.value(0).toInt();
    row["project"] = query.value(1).toString();
    row["kind"] = query.value(2).toString();
    row["title"] = query.value(3).toString();
    row["body"] = query.value(4).toString();
    row["tags"] = query.value(5).toString();
    row["pinned"] = query.value(6).toBool();
    row["importance"] = query.value(7).toInt();
    row["source"] = query.value(8).toString();
    row["createdAt"] = query.value(9).toString();
    row["updatedAt"] = query.value(10).toString();
    return row;
}

QVariantMap MemoryStore::integrityCheck() {
    QVariantMap result{{"ok", false}, {"database", QString()}, {"fts", QString()},
                       {"total", 0}, {"bytes", QFileInfo(m_path).size()}};
    if (!m_ready) {
        result["database"] = tr("base non ouverte");
        return result;
    }
    QSqlQuery query(m_db);
    if (query.exec(QStringLiteral("PRAGMA integrity_check")) && query.next())
        result["database"] = query.value(0).toString();
    QSqlQuery fts(m_db);
    if (fts.exec(QStringLiteral("INSERT INTO memories_fts(memories_fts) VALUES('integrity-check')")))
        result["fts"] = QStringLiteral("ok");
    else
        result["fts"] = fts.lastError().text();
    QSqlQuery count(m_db);
    if (count.exec(QStringLiteral("SELECT COUNT(*) FROM memories")) && count.next())
        result["total"] = count.value(0).toInt();
    result["ok"] = result["database"].toString() == QLatin1String("ok")
        && result["fts"].toString() == QLatin1String("ok");
    return result;
}

bool MemoryStore::exportTo(const QString &path) {
    if (!m_ready || path.isEmpty())
        return false;
    QJsonArray array;
    QSqlQuery query(m_db);
    query.exec(QStringLiteral(
        "SELECT id,project,kind,title,body,tags,pinned,importance,source,created_at,updated_at "
        "FROM memories ORDER BY id"));
    while (query.next()) {
        const QVariantMap row = rowFromQuery(query);
        array.append(QJsonObject::fromVariantMap(row));
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return file.commit();
}

int MemoryStore::importFrom(const QString &path) {
    if (!m_ready)
        return 0;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return 0;
    const QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    int imported = 0;
    m_db.transaction();
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        const QString title = object.value("title").toString().trimmed();
        if (title.isEmpty())
            continue;
        QSqlQuery query(m_db);
        query.prepare(QStringLiteral(
            "INSERT INTO memories(project,kind,title,body,tags,pinned,importance,source,created_at,updated_at) "
            "VALUES(?,?,?,?,?,?,?,?,?,?)"));
        query.addBindValue(object.value("project").toString());
        query.addBindValue(object.value("kind").toString(QStringLiteral("note")));
        query.addBindValue(title);
        query.addBindValue(object.value("body").toString());
        query.addBindValue(object.value("tags").toString());
        query.addBindValue(object.value("pinned").toBool() ? 1 : 0);
        query.addBindValue(qBound(0, object.value("importance").toInt(3), 5));
        query.addBindValue(QStringLiteral("import"));
        query.addBindValue(nowIso());
        query.addBindValue(nowIso());
        if (query.exec())
            ++imported;
    }
    m_db.commit();
    refresh();
    return imported;
}

void MemoryStore::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}

void MemoryStore::clearError() { setError({}); }
