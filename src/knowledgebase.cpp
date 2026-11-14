#include "knowledgebase.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

KnowledgeBase::KnowledgeBase(const QString &path, QObject *parent)
    : QObject(parent),
      m_path(path.isEmpty() ? defaultPath() : path),
      m_connection(QStringLiteral("knowledge-%1").arg(quintptr(this))) {
    if (!open() || !migrate()) {
        setError(tr("Impossible d'initialiser la base de connaissances."));
        return;
    }
    m_ready = true;
    refresh();
}

KnowledgeBase::~KnowledgeBase() {
    if (QSqlDatabase::contains(m_connection)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connection, false);
            if (db.isOpen()) {
                QSqlQuery checkpoint(db);
                checkpoint.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE)"));
                checkpoint.finish();
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connection);
    }
}

QString KnowledgeBase::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/knowledge.db");
}

bool KnowledgeBase::open() {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
    db.setDatabaseName(m_path);
    if (!db.open()) {
        setError(db.lastError().text());
        return false;
    }
    QSqlQuery pragma(db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA wal_autocheckpoint=256"));
    pragma.exec(QStringLiteral("PRAGMA journal_size_limit=1048576"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    QFile::setPermissions(m_path, QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

bool KnowledgeBase::migrate() {
    QSqlDatabase db = QSqlDatabase::database(m_connection, false);
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS entries ("
                                   "key TEXT PRIMARY KEY, category TEXT NOT NULL, name TEXT NOT NULL,"
                                   "summary TEXT NOT NULL, details TEXT NOT NULL, source TEXT NOT NULL,"
                                   "updated_at TEXT NOT NULL)"))) {
        setError(query.lastError().text());
        return false;
    }
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS entries_category ON entries(category)"));
    return true;
}

void KnowledgeBase::setError(const QString &error) {
    if (m_error == error) return;
    m_error = error;
    emit errorChanged();
}

void KnowledgeBase::clearError() { setError({}); }

void KnowledgeBase::upsert(const QString &key, const QString &category, const QString &name,
                           const QString &summary, const QString &details, const QString &source) {
    QSqlQuery query(QSqlDatabase::database(m_connection, false));
    query.prepare(QStringLiteral("INSERT INTO entries(key,category,name,summary,details,source,updated_at) "
                                 "VALUES(?,?,?,?,?,?,?) ON CONFLICT(key) DO UPDATE SET "
                                 "category=excluded.category,name=excluded.name,summary=excluded.summary,"
                                 "details=excluded.details,source=excluded.source,updated_at=excluded.updated_at"));
    query.addBindValue(key);
    query.addBindValue(category);
    query.addBindValue(name.left(300));
    query.addBindValue(summary.left(2000));
    query.addBindValue(details.left(20000));
    query.addBindValue(source);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.exec();
}

void KnowledgeBase::seed(const QJsonArray &tools, const QVariantList &plugins,
                         const QVariantList &skills, const QVariantList &capabilities) {
    if (!m_ready) return;
    QSqlQuery clear(QSqlDatabase::database(m_connection, false));
    clear.exec(QStringLiteral("DELETE FROM entries WHERE source='system'"));
    for (const QJsonValue &value : tools) {
        const QJsonObject tool = value.toObject();
        const QJsonObject function = tool.value(QStringLiteral("function")).toObject();
        const QString name = function.value(QStringLiteral("name")).toString();
        if (name.isEmpty()) continue;
        const QString description = function.value(QStringLiteral("description")).toString();
        upsert(QStringLiteral("tool:") + name, QStringLiteral("tool"), name,
               description, QString::fromUtf8(QJsonDocument(function.value(QStringLiteral("parameters")).toObject())
                                                   .toJson(QJsonDocument::Compact)),
               QStringLiteral("system"));
    }
    for (const QVariant &value : plugins) {
        const QVariantMap plugin = value.toMap();
        const QString name = plugin.value(QStringLiteral("toolName")).toString();
        if (name.isEmpty()) continue;
        const QString pluginName = plugin.value(QStringLiteral("name")).toString();
        upsert(QStringLiteral("plugin:") + name, QStringLiteral("plugin"),
               pluginName.isEmpty() ? name : pluginName,
               plugin.value(QStringLiteral("description")).toString(),
               QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(plugin)).toJson(QJsonDocument::Compact)),
               QStringLiteral("system"));
    }
    for (const QVariant &value : skills) {
        const QVariantMap skill = value.toMap();
        const QString id = skill.value(QStringLiteral("id")).toString();
        if (id.isEmpty()) continue;
        const QString skillName = skill.value(QStringLiteral("name")).toString();
        upsert(QStringLiteral("skill:") + id, QStringLiteral("skill"),
               skillName.isEmpty() ? id : skillName,
               skill.value(QStringLiteral("description")).toString(),
               QStringLiteral("pack=%1 path=%2").arg(skill.value(QStringLiteral("pack")).toString(),
                                                      skill.value(QStringLiteral("path")).toString()),
               QStringLiteral("system"));
    }
    QVariantList capabilityEntries = capabilities;
    if (capabilityEntries.isEmpty()) capabilityEntries = {
        QVariantMap{{QStringLiteral("id"), QStringLiteral("ultimate_agent")}, {QStringLiteral("name"), QStringLiteral("Ultimate Agent")}, {QStringLiteral("summary"), QStringLiteral("Agent natif principal : raisonnement, outils, mémoire, skills, apprentissage et permissions.")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("4crasy")}, {QStringLiteral("name"), QStringLiteral("4Crasy")}, {QStringLiteral("summary"), QStringLiteral("Routeur local de modèles gratuits avec fallback et API OpenAI-compatible.")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("learning")}, {QStringLiteral("name"), QStringLiteral("Apprentissage")}, {QStringLiteral("summary"), QStringLiteral("Boucle locale : trajectoires, leçons, propositions de skills et réinjection de contexte.")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("skills")}, {QStringLiteral("name"), QStringLiteral("Skills")}, {QStringLiteral("summary"), QStringLiteral("Bibliothèque de procédures découvertes dans ~/.agents/skills, dont Leanrea et DeepSeek Stack.")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("memory")}, {QStringLiteral("name"), QStringLiteral("Mémoire")}, {QStringLiteral("summary"), QStringLiteral("Mémoire projet SQLite FTS5, recherche, épinglage et serveur MCP.")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("missions")}, {QStringLiteral("name"), QStringLiteral("Missions")}, {QStringLiteral("summary"), QStringLiteral("Orchestration planificateur → exécutant → réviseur avec profils.")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("bots")}, {QStringLiteral("name"), QStringLiteral("Bots")}, {QStringLiteral("summary"), QStringLiteral("Bots persistants, file d’attente et serveur HTTP local.")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("autopilot")}, {QStringLiteral("name"), QStringLiteral("Autopilote")}, {QStringLiteral("summary"), QStringLiteral("Surveillance et exécution autonome de tâches planifiées.")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("github")}, {QStringLiteral("name"), QStringLiteral("GitHub")}, {QStringLiteral("summary"), QStringLiteral("Dépôts, issues, pulls, clones et authentification gh.")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("notes")}, {QStringLiteral("name"), QStringLiteral("Notes")}, {QStringLiteral("summary"), QStringLiteral("Vault Markdown, recherche, liens et graphe.")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("mcp")}, {QStringLiteral("name"), QStringLiteral("MCP")}, {QStringLiteral("summary"), QStringLiteral("Serveurs MCP intégrés mémoire/bots et serveurs personnalisés.")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("terminal")}, {QStringLiteral("name"), QStringLiteral("Terminal")}, {QStringLiteral("summary"), QStringLiteral("Terminal intégré et exécution de commandes avec permissions.")}}
    };
    for (const QVariant &value : capabilityEntries) {
        const QVariantMap capability = value.toMap();
        const QString id = capability.value(QStringLiteral("id")).toString();
        if (id.isEmpty())
            continue;
        QString name = capability.value(QStringLiteral("name")).toString();
        if (name.isEmpty())
            name = id;
        QString summary = capability.value(QStringLiteral("summary")).toString();
        if (summary.isEmpty())
            summary = capability.value(QStringLiteral("description")).toString();
        upsert(QStringLiteral("capability:") + id, QStringLiteral("capability"),
               name, summary,
               QStringLiteral("Composant natif du harness Ultimate IAH."), QStringLiteral("system"));
    }
    refresh();
}

void KnowledgeBase::refresh() {
    if (!m_ready) return;
    QSqlDatabase db = QSqlDatabase::database(m_connection, false);
    m_entries.clear();
    QSqlQuery query(db);
    query.exec(QStringLiteral("SELECT key,category,name,summary,source,updated_at FROM entries "
                              "ORDER BY CASE category WHEN 'tool' THEN 0 WHEN 'capability' THEN 1 "
                              "WHEN 'plugin' THEN 2 WHEN 'skill' THEN 3 ELSE 4 END, name LIMIT 250"));
    QVariantMap counts;
    while (query.next()) {
        const QString category = query.value(1).toString();
        counts[category] = counts.value(category).toInt() + 1;
        m_entries.append(QVariantMap{{QStringLiteral("key"), query.value(0).toString()},
                                     {QStringLiteral("category"), category},
                                     {QStringLiteral("name"), query.value(2).toString()},
                                     {QStringLiteral("summary"), query.value(3).toString()},
                                     {QStringLiteral("source"), query.value(4).toString()},
                                     {QStringLiteral("updatedAt"), query.value(5).toString()}});
    }
    QSqlQuery total(db);
    total.exec(QStringLiteral("SELECT COUNT(*) FROM entries"));
    const int count = total.next() ? total.value(0).toInt() : 0;
    m_stats = QVariantMap{{QStringLiteral("entries"), count},
                          {QStringLiteral("tools"), counts.value(QStringLiteral("tool")).toInt()},
                          {QStringLiteral("capabilities"), counts.value(QStringLiteral("capability")).toInt()},
                          {QStringLiteral("plugins"), counts.value(QStringLiteral("plugin")).toInt()},
                          {QStringLiteral("skills"), counts.value(QStringLiteral("skill")).toInt()}};
    QFile::setPermissions(m_path, QFile::ReadOwner | QFile::WriteOwner);
    emit changed();
}

QVariantList KnowledgeBase::search(const QString &query, int limit) const {
    if (!m_ready || query.trimmed().isEmpty()) return m_entries.mid(0, limit);
    QSqlQuery search(QSqlDatabase::database(m_connection, false));
    search.prepare(QStringLiteral("SELECT key,category,name,summary,source,updated_at FROM entries "
                                  "WHERE name LIKE ? OR summary LIKE ? OR details LIKE ? "
                                  "ORDER BY category, name LIMIT ?"));
    const QString pattern = QStringLiteral("%") + query.trimmed() + QStringLiteral("%");
    search.addBindValue(pattern);
    search.addBindValue(pattern);
    search.addBindValue(pattern);
    search.addBindValue(qBound(1, limit, 100));
    QVariantList results;
    if (search.exec()) {
        while (search.next()) {
            results.append(QVariantMap{{QStringLiteral("key"), search.value(0).toString()},
                                       {QStringLiteral("category"), search.value(1).toString()},
                                       {QStringLiteral("name"), search.value(2).toString()},
                                       {QStringLiteral("summary"), search.value(3).toString()},
                                       {QStringLiteral("source"), search.value(4).toString()},
                                       {QStringLiteral("updatedAt"), search.value(5).toString()}});
        }
    }
    return results;
}

QString KnowledgeBase::promptContext(const QString &query, int maxChars) const {
    if (!m_ready) return {};
    QString text = QStringLiteral("BASE NEURONALE NATIVE — outils et composants enregistrés\n"
                                  "Utilise ultimate_knowledge action=search pour le détail d’un outil. "
                                  "Tous ces outils existent réellement et sont exécutables.\n");
    const QVariantList matches = query.trimmed().isEmpty() ? QVariantList() : search(query, 8);
    if (!matches.isEmpty()) {
        text += QStringLiteral("Résultats pertinents :\n");
        for (const QVariant &value : matches) {
            const QVariantMap entry = value.toMap();
            text += QStringLiteral("- %1 [%2] %3: %4\n")
                        .arg(entry.value(QStringLiteral("name")).toString(),
                             entry.value(QStringLiteral("category")).toString(),
                             entry.value(QStringLiteral("key")).toString(),
                             entry.value(QStringLiteral("summary")).toString().left(260));
        }
    }
    text += QStringLiteral("Catalogue :\n");
    for (const QVariant &value : m_entries) {
        const QVariantMap entry = value.toMap();
        const QString category = entry.value(QStringLiteral("category")).toString();
        if (category != QLatin1String("tool") && category != QLatin1String("capability")
            && category != QLatin1String("plugin"))
            continue;
        text += QStringLiteral("- %1 [%2] %3\n")
                    .arg(entry.value(QStringLiteral("name")).toString(),
                         category, entry.value(QStringLiteral("summary")).toString().left(220));
        if (text.size() >= maxChars) break;
    }
    return text.left(maxChars);
}
