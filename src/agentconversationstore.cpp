#include "agentconversationstore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>

namespace {
QString nowIso() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}
} // namespace

AgentConversationStore::AgentConversationStore(const QString &path, QObject *parent)
    : QObject(parent),
      m_path(path.isEmpty() ? defaultPath() : path),
      m_connectionName(QStringLiteral("agent-conversations-%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))) {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    auto *database = new QSqlDatabase(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName));
    database->setDatabaseName(m_path);
    m_db = database;
    if (!database->open()) {
        setError(tr("Impossible d'ouvrir l'historique des discussions : %1").arg(database->lastError().text()));
        return;
    }
    QSqlQuery pragma(*database);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA wal_autocheckpoint=1000"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    pragma.exec(QStringLiteral("PRAGMA busy_timeout=5000"));
    ensureSchema();
    QFile::setPermissions(m_path, QFile::ReadOwner | QFile::WriteOwner);
}

AgentConversationStore::~AgentConversationStore() {
    if (m_db) {
        QSqlQuery checkpoint(*m_db);
        checkpoint.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE)"));
        checkpoint.finish();
        const QString connection = m_connectionName;
        delete m_db;
        m_db = nullptr;
        QSqlDatabase::removeDatabase(connection);
    }
}

QString AgentConversationStore::defaultPath() {
    const QString override = qEnvironmentVariable("ULTIMATE_IAH_AGENT_DB");
    if (!override.trimmed().isEmpty())
        return override;
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/agent-conversations.db");
}

bool AgentConversationStore::ensureSchema() {
    if (!m_db)
        return false;
    QSqlQuery query(*m_db);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS conversations ("
            "id TEXT PRIMARY KEY,"
            "title TEXT NOT NULL,"
            "preview TEXT NOT NULL DEFAULT '',"
            "api_json TEXT NOT NULL DEFAULT '[]',"
            "ui_json TEXT NOT NULL DEFAULT '[]',"
            "model TEXT NOT NULL DEFAULT '',"
            "endpoint TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL,"
            "pinned INTEGER NOT NULL DEFAULT 0,"
            "archived INTEGER NOT NULL DEFAULT 0,"
            "message_count INTEGER NOT NULL DEFAULT 0)"))) {
        setError(query.lastError().text());
        return false;
    }
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS meta ("
            "key TEXT PRIMARY KEY,"
            "value TEXT NOT NULL)"))) {
        setError(query.lastError().text());
        return false;
    }
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS conversations_updated ON conversations(updated_at DESC)"));
    return true;
}

QVariantList AgentConversationStore::conversations(const QString &queryText, bool includeArchived) const {
    QVariantList out;
    if (!m_db)
        return out;
    QSqlQuery query(*m_db);
    QString sql = QStringLiteral(
        "SELECT id,title,preview,model,endpoint,created_at,updated_at,pinned,archived,message_count "
        "FROM conversations WHERE archived=0");
    if (includeArchived)
        sql = QStringLiteral(
            "SELECT id,title,preview,model,endpoint,created_at,updated_at,pinned,archived,message_count "
            "FROM conversations WHERE 1=1");
    if (!queryText.trimmed().isEmpty())
        sql += QStringLiteral(" AND (title LIKE :q OR preview LIKE :q)");
    sql += QStringLiteral(" ORDER BY pinned DESC, updated_at DESC");
    query.prepare(sql);
    if (!queryText.trimmed().isEmpty())
        query.bindValue(QStringLiteral(":q"), QStringLiteral("%") + queryText.trimmed() + QStringLiteral("%"));
    if (!query.exec()) {
        return out;
    }
    while (query.next()) {
        out.append(QVariantMap{
            {QStringLiteral("id"), query.value(0).toString()},
            {QStringLiteral("title"), query.value(1).toString()},
            {QStringLiteral("preview"), query.value(2).toString()},
            {QStringLiteral("model"), query.value(3).toString()},
            {QStringLiteral("endpoint"), query.value(4).toString()},
            {QStringLiteral("createdAt"), query.value(5).toString()},
            {QStringLiteral("updatedAt"), query.value(6).toString()},
            {QStringLiteral("pinned"), query.value(7).toBool()},
            {QStringLiteral("archived"), query.value(8).toBool()},
            {QStringLiteral("messageCount"), query.value(9).toInt()},
        });
    }
    return out;
}

QJsonObject AgentConversationStore::conversation(const QString &id) const {
    QJsonObject out;
    if (!m_db || id.isEmpty())
        return out;
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral(
        "SELECT id,title,preview,api_json,ui_json,model,endpoint,created_at,updated_at,pinned,archived "
        "FROM conversations WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next())
        return out;
    out.insert(QStringLiteral("id"), query.value(0).toString());
    out.insert(QStringLiteral("title"), query.value(1).toString());
    out.insert(QStringLiteral("preview"), query.value(2).toString());
    out.insert(QStringLiteral("api"), QJsonDocument::fromJson(query.value(3).toString().toUtf8()).array());
    out.insert(QStringLiteral("ui"), QJsonDocument::fromJson(query.value(4).toString().toUtf8()).array());
    out.insert(QStringLiteral("model"), query.value(5).toString());
    out.insert(QStringLiteral("endpoint"), query.value(6).toString());
    out.insert(QStringLiteral("createdAt"), query.value(7).toString());
    out.insert(QStringLiteral("updatedAt"), query.value(8).toString());
    out.insert(QStringLiteral("pinned"), query.value(9).toBool());
    out.insert(QStringLiteral("archived"), query.value(10).toBool());
    return out;
}

QString AgentConversationStore::lastConversationId() const {
    return meta(QStringLiteral("lastConversation"));
}

QString AgentConversationStore::create(const QString &title, const QString &model,
                                       const QString &endpoint) {
    if (!m_db)
        return {};
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString now = nowIso();
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO conversations(id,title,preview,api_json,ui_json,model,endpoint,created_at,updated_at,pinned,archived,message_count) "
        "VALUES(:id,:title,'','[]','[]',:model,:endpoint,:created,:updated,0,0,0)"));
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":title"), titleFor(title, {}));
    query.bindValue(QStringLiteral(":model"), model.isNull() ? QStringLiteral("") : model);
    query.bindValue(QStringLiteral(":endpoint"), endpoint.isNull() ? QStringLiteral("") : endpoint);
    query.bindValue(QStringLiteral(":created"), now);
    query.bindValue(QStringLiteral(":updated"), now);
    if (!query.exec()) {
        setError(query.lastError().text());
        return {};
    }
    setMeta(QStringLiteral("lastConversation"), id);
    emit changed();
    return id;
}

bool AgentConversationStore::save(const QString &id, const QString &title,
                                  const QJsonArray &api, const QJsonArray &ui,
                                  const QString &model, const QString &endpoint) {
    if (!m_db || id.isEmpty())
        return false;
    const QString now = nowIso();
    const QString apiJson = QString::fromUtf8(QJsonDocument(api).toJson(QJsonDocument::Compact));
    const QString uiJson = QString::fromUtf8(QJsonDocument(ui).toJson(QJsonDocument::Compact));
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral(
        "UPDATE conversations SET title=:title,preview=:preview,api_json=:api,ui_json=:ui,"
        "model=:model,endpoint=:endpoint,updated_at=:updated,message_count=:count WHERE id=:id"));
    query.bindValue(QStringLiteral(":title"), titleFor(title, ui));
    query.bindValue(QStringLiteral(":preview"), previewFor(ui));
    query.bindValue(QStringLiteral(":api"), apiJson);
    query.bindValue(QStringLiteral(":ui"), uiJson);
    query.bindValue(QStringLiteral(":model"), model.isNull() ? QStringLiteral("") : model);
    query.bindValue(QStringLiteral(":endpoint"), endpoint.isNull() ? QStringLiteral("") : endpoint);
    query.bindValue(QStringLiteral(":updated"), now);
    query.bindValue(QStringLiteral(":count"), ui.size());
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        setError(query.lastError().text());
        return false;
    }
    setMeta(QStringLiteral("lastConversation"), id);
    emit changed();
    return true;
}

bool AgentConversationStore::remove(const QString &id) {
    if (!m_db || id.isEmpty())
        return false;
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("DELETE FROM conversations WHERE id=:id"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        setError(query.lastError().text());
        return false;
    }
    if (lastConversationId() == id)
        setMeta(QStringLiteral("lastConversation"), QString());
    emit changed();
    return true;
}

bool AgentConversationStore::rename(const QString &id, const QString &title) {
    if (!m_db || id.isEmpty() || title.trimmed().isEmpty())
        return false;
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("UPDATE conversations SET title=:title,updated_at=:updated WHERE id=:id"));
    query.bindValue(QStringLiteral(":title"), title.trimmed().left(90));
    query.bindValue(QStringLiteral(":updated"), nowIso());
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        setError(query.lastError().text());
        return false;
    }
    emit changed();
    return true;
}

bool AgentConversationStore::setPinned(const QString &id, bool pinned) {
    if (!m_db || id.isEmpty())
        return false;
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("UPDATE conversations SET pinned=:pinned WHERE id=:id"));
    query.bindValue(QStringLiteral(":pinned"), pinned ? 1 : 0);
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec())
        return false;
    emit changed();
    return true;
}

bool AgentConversationStore::setArchived(const QString &id, bool archived) {
    if (!m_db || id.isEmpty())
        return false;
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("UPDATE conversations SET archived=:archived WHERE id=:id"));
    query.bindValue(QStringLiteral(":archived"), archived ? 1 : 0);
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec())
        return false;
    emit changed();
    return true;
}

bool AgentConversationStore::importLegacy(const QString &path) {
    if (!m_db || !conversations().isEmpty())
        return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonArray api = root.value(QStringLiteral("api")).toArray();
    const QJsonArray ui = root.value(QStringLiteral("ui")).toArray();
    if (api.isEmpty() && ui.isEmpty())
        return false;
    const QString id = create(root.value(QStringLiteral("title")).toString(), {}, {});
    if (id.isEmpty())
        return false;
    if (!save(id, root.value(QStringLiteral("title")).toString(), api, ui, {}, {}))
        return false;
    return true;
}

QString AgentConversationStore::meta(const QString &key) const {
    if (!m_db)
        return {};
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("SELECT value FROM meta WHERE key=:key"));
    query.bindValue(QStringLiteral(":key"), key);
    if (!query.exec() || !query.next())
        return {};
    return query.value(0).toString();
}

bool AgentConversationStore::setMeta(const QString &key, const QString &value) {
    if (!m_db)
        return false;
    QSqlQuery query(*m_db);
    query.prepare(QStringLiteral("INSERT INTO meta(key,value) VALUES(:key,:value) "
                                 "ON CONFLICT(key) DO UPDATE SET value=excluded.value"));
    query.bindValue(QStringLiteral(":key"), key);
    query.bindValue(QStringLiteral(":value"), value.isNull() ? QStringLiteral("") : value);
    return query.exec();
}

QString AgentConversationStore::previewFor(const QJsonArray &ui) const {
    for (int i = ui.size() - 1; i >= 0; --i) {
        const QJsonObject message = ui.at(i).toObject();
        const QString role = message.value(QStringLiteral("role")).toString();
        if (role != QLatin1String("user") && role != QLatin1String("assistant"))
            continue;
        QString text = message.value(QStringLiteral("text")).toString().simplified();
        if (text.isEmpty() && !message.value(QStringLiteral("images")).toArray().isEmpty())
            text = tr("[image]");
        if (!text.isEmpty())
            return text.left(180);
    }
    return {};
}

QString AgentConversationStore::titleFor(const QString &title, const QJsonArray &ui) const {
    if (!title.trimmed().isEmpty())
        return title.trimmed().left(90);
    for (const QJsonValue &value : ui) {
        const QJsonObject message = value.toObject();
        if (message.value(QStringLiteral("role")).toString() == QLatin1String("user")) {
            const QString text = message.value(QStringLiteral("text")).toString().simplified();
            if (!text.isEmpty())
                return text.left(56);
        }
    }
    return tr("Nouvelle discussion");
}

void AgentConversationStore::setError(const QString &error) {
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

void AgentConversationStore::clearError() { setError({}); }
