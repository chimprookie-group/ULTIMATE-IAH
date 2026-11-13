#include "botstore.h"

#include <QDateTime>

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSaveFile>

namespace {
QString nowIso() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

QString addMinutes(int minutes) {
    return QDateTime::currentDateTimeUtc().addSecs(qMax(1, minutes) * 60)
        .toString(Qt::ISODateWithMs);
}
} // namespace

BotStore::BotStore(const QString &path, QObject *parent)
    : QObject(parent),
      m_path(path.isEmpty() ? defaultPath() : path),
      m_connection(QStringLiteral("bots-%1").arg(reinterpret_cast<quintptr>(this))) {
    if (!open())
        return;
    m_ready = true;
    seed();
    refresh();
}

BotStore::~BotStore() {
    if (m_db.isOpen())
        m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connection);
}

QString BotStore::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/bots.db");
}

bool BotStore::open() {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
    m_db.setDatabaseName(m_path);
    if (!m_db.open()) {
        setError(tr("Impossible d'ouvrir le registre des bots : %1").arg(m_db.lastError().text()));
        return false;
    }
    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    if (!migrate()) {
        setError(tr("Impossible de préparer le registre des bots : %1").arg(m_db.lastError().text()));
        return false;
    }
    QFile::setPermissions(m_path, QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

bool BotStore::ensureColumn(const QString &table, const QString &column, const QString &definition) {
    QSqlQuery info(m_db);
    if (!info.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table)))
        return false;
    while (info.next()) {
        if (info.value(1).toString() == column)
            return true;
    }
    QSqlQuery alter(m_db);
    return alter.exec(QStringLiteral("ALTER TABLE %1 ADD COLUMN %2 %3").arg(table, column, definition));
}

bool BotStore::migrate() {
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS bots("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL UNIQUE,"
            "description TEXT NOT NULL DEFAULT '',"
            "instructions TEXT NOT NULL DEFAULT '',"
            "permission TEXT NOT NULL DEFAULT 'read-only',"
            "provider TEXT NOT NULL DEFAULT '',"
            "model TEXT NOT NULL DEFAULT '',"
            "enabled INTEGER NOT NULL DEFAULT 1,"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL)")))
        return false;
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS bot_messages("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "bot_id INTEGER NOT NULL,"
            "role TEXT NOT NULL,"
            "content TEXT NOT NULL,"
            "kind TEXT NOT NULL DEFAULT 'chat',"
            "run_id INTEGER NOT NULL DEFAULT -1,"
            "created_at TEXT NOT NULL,"
            "FOREIGN KEY(bot_id) REFERENCES bots(id) ON DELETE CASCADE)")))
        return false;
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS bot_runs("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "bot_id INTEGER NOT NULL,"
            "task TEXT NOT NULL,"
            "status TEXT NOT NULL DEFAULT 'queued',"
            "output TEXT NOT NULL DEFAULT '',"
            "error TEXT NOT NULL DEFAULT '',"
            "exit_code INTEGER NOT NULL DEFAULT -1,"
            "created_at TEXT NOT NULL,"
            "finished_at TEXT NOT NULL DEFAULT '',"
            "FOREIGN KEY(bot_id) REFERENCES bots(id) ON DELETE CASCADE)")))
        return false;
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS bot_schedules("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "bot_id INTEGER NOT NULL,"
            "title TEXT NOT NULL,"
            "prompt TEXT NOT NULL,"
            "interval_minutes INTEGER NOT NULL DEFAULT 60,"
            "enabled INTEGER NOT NULL DEFAULT 1,"
            "next_run TEXT NOT NULL,"
            "last_run TEXT NOT NULL DEFAULT '',"
            "last_status TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL,"
            "FOREIGN KEY(bot_id) REFERENCES bots(id) ON DELETE CASCADE)")))
        return false;
    if (!ensureColumn(QStringLiteral("bots"), QStringLiteral("provider"),
                      QStringLiteral("TEXT NOT NULL DEFAULT ''"))
        || !ensureColumn(QStringLiteral("bots"), QStringLiteral("model"),
                         QStringLiteral("TEXT NOT NULL DEFAULT ''")))
        return false;
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_bot_messages_bot ON bot_messages(bot_id, id)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_bot_runs_bot ON bot_runs(bot_id, id)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_bot_schedules_due ON bot_schedules(enabled, next_run)"));
    return true;
}

void BotStore::seed() {
    QSqlQuery count(m_db);
    count.exec(QStringLiteral("SELECT COUNT(*) FROM bots"));
    if (count.next() && count.value(0).toInt() > 0)
        return;
    struct Seed { const char *name; const char *description; const char *instructions; const char *permission; };
    static const Seed seeds[] = {
        {"Atlas", "Chef d'orchestre de l'écosystème Ultimate IAH.",
         "Tu es Atlas, le bot de coordination d'Ultimate IAH. Analyse la demande, inspecte l'état réel du système et propose ou exécute la meilleure suite d'actions. Tu coordonnes les autres bots sans dupliquer leur travail. Réponds de façon concise et concrète.", "read-only"},
        {"Forge", "Constructeur et exécutant du workspace.",
         "Tu es Forge, le bot de construction d'Ultimate IAH. Tu travailles dans le workspace actif, tu modifies les fichiers nécessaires, tu lances les vérifications et tu corriges les erreurs. Tu ne touches jamais aux secrets et tu confirmes les actions destructrices.", "workspace-write"},
        {"Sentry", "Auditeur et garde-fou.",
         "Tu es Sentry, le bot d'audit d'Ultimate IAH. Tu inspectes le code, les configurations, les permissions, les secrets exposés et les dépendances. Tu classes les constats par sévérité et tu proposes des correctifs vérifiables. Tu ne modifies rien.", "read-only"},
    };
    for (const Seed &seed : seeds) {
        QSqlQuery insert(m_db);
        insert.prepare(QStringLiteral(
            "INSERT INTO bots(name,description,instructions,permission,enabled,created_at,updated_at) "
            "VALUES(?,?,?,?,1,?,?)"));
        insert.addBindValue(QString::fromUtf8(seed.name));
        insert.addBindValue(QString::fromUtf8(seed.description));
        insert.addBindValue(QString::fromUtf8(seed.instructions));
        insert.addBindValue(QString::fromUtf8(seed.permission));
        insert.addBindValue(nowIso());
        insert.addBindValue(nowIso());
        insert.exec();
    }
}

void BotStore::refresh() {
    if (!m_ready)
        return;
    loadBots();
    if (m_selectedBotId < 0 || !m_bots.isEmpty()) {
        bool found = false;
        for (const QVariant &value : m_bots) {
            if (value.toMap().value(QStringLiteral("id")).toInt() == m_selectedBotId) {
                found = true;
                break;
            }
        }
        if (!found && !m_bots.isEmpty()) {
            m_selectedBotId = m_bots.first().toMap().value(QStringLiteral("id")).toInt();
            emit selectionChanged();
        }
    }
    loadMessages();
    loadRuns();
    loadSchedules();
    loadStats();
    emit changed();
}

void BotStore::loadBots() {
    QVariantList bots;
    QSqlQuery query(m_db);
    if (query.exec(QStringLiteral(
            "SELECT id,name,description,instructions,permission,enabled,created_at,updated_at,provider,model "
            "FROM bots ORDER BY enabled DESC, id ASC"))) {
        while (query.next()) {
            bots.append(QVariantMap{
                {QStringLiteral("id"), query.value(0).toInt()},
                {QStringLiteral("name"), query.value(1).toString()},
                {QStringLiteral("description"), query.value(2).toString()},
                {QStringLiteral("instructions"), query.value(3).toString()},
                {QStringLiteral("permission"), query.value(4).toString()},
                {QStringLiteral("enabled"), query.value(5).toBool()},
                {QStringLiteral("createdAt"), query.value(6).toString()},
                {QStringLiteral("updatedAt"), query.value(7).toString()},
                {QStringLiteral("provider"), query.value(8).toString()},
                {QStringLiteral("model"), query.value(9).toString()},
            });
        }
    }
    m_bots = bots;
}

void BotStore::loadMessages() {
    QVariantList messages;
    if (m_selectedBotId > 0) {
        QSqlQuery query(m_db);
        query.prepare(QStringLiteral(
            "SELECT id,role,content,kind,run_id,created_at FROM bot_messages "
            "WHERE bot_id=? ORDER BY id DESC LIMIT 200"));
        query.addBindValue(m_selectedBotId);
        if (query.exec()) {
            QVariantList reverse;
            while (query.next()) {
                reverse.append(QVariantMap{
                    {QStringLiteral("id"), query.value(0).toInt()},
                    {QStringLiteral("role"), query.value(1).toString()},
                    {QStringLiteral("content"), query.value(2).toString()},
                    {QStringLiteral("kind"), query.value(3).toString()},
                    {QStringLiteral("runId"), query.value(4).toInt()},
                    {QStringLiteral("time"), query.value(5).toString()},
                });
            }
            for (int i = reverse.size() - 1; i >= 0; --i)
                messages.append(reverse.at(i));
        }
    }
    m_messages = messages;
    emit messagesChanged();
}

void BotStore::loadRuns() {
    m_runs = recentRuns(0, 40);
    emit runsChanged();
}

void BotStore::loadSchedules() {
    QVariantList schedules;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT s.id,s.bot_id,b.name,s.title,s.prompt,s.interval_minutes,s.enabled,"
        "s.next_run,s.last_run,s.last_status FROM bot_schedules s "
        "JOIN bots b ON b.id=s.bot_id ORDER BY s.id DESC"));
    if (query.exec()) {
        while (query.next()) {
            schedules.append(QVariantMap{
                {QStringLiteral("id"), query.value(0).toInt()},
                {QStringLiteral("botId"), query.value(1).toInt()},
                {QStringLiteral("botName"), query.value(2).toString()},
                {QStringLiteral("title"), query.value(3).toString()},
                {QStringLiteral("prompt"), query.value(4).toString()},
                {QStringLiteral("intervalMinutes"), query.value(5).toInt()},
                {QStringLiteral("enabled"), query.value(6).toBool()},
                {QStringLiteral("nextRun"), query.value(7).toString()},
                {QStringLiteral("lastRun"), query.value(8).toString()},
                {QStringLiteral("lastStatus"), query.value(9).toString()},
            });
        }
    }
    m_schedules = schedules;
    emit schedulesChanged();
}

void BotStore::loadStats() {
    QSqlQuery query(m_db);
    int bots = 0, runs = 0, messages = 0, schedules = 0;
    if (query.exec(QStringLiteral("SELECT COUNT(*) FROM bots")) && query.next()) bots = query.value(0).toInt();
    if (query.exec(QStringLiteral("SELECT COUNT(*) FROM bot_runs")) && query.next()) runs = query.value(0).toInt();
    if (query.exec(QStringLiteral("SELECT COUNT(*) FROM bot_messages")) && query.next()) messages = query.value(0).toInt();
    if (query.exec(QStringLiteral("SELECT COUNT(*) FROM bot_schedules WHERE enabled=1")) && query.next()) schedules = query.value(0).toInt();
    m_stats = QVariantMap{{QStringLiteral("bots"), bots}, {QStringLiteral("runs"), runs},
                          {QStringLiteral("messages"), messages}, {QStringLiteral("schedules"), schedules}};
}

void BotStore::selectBot(int id) {
    if (id == m_selectedBotId)
        return;
    if (id > 0) {
        bool found = false;
        for (const QVariant &value : m_bots) {
            if (value.toMap().value(QStringLiteral("id")).toInt() == id) {
                found = true;
                break;
            }
        }
        if (!found)
            return;
    }
    m_selectedBotId = id;
    loadMessages();
    loadRuns();
    emit selectionChanged();
    emit changed();
}

int BotStore::createBot(const QString &name, const QString &description,
                        const QString &instructions, const QString &permission,
                        const QString &provider, const QString &model) {
    if (!m_ready)
        return -1;
    const QString cleanName = name.trimmed();
    if (cleanName.isEmpty()) {
        setError(tr("Le nom du bot est obligatoire."));
        return -1;
    }
    const QString perm = permission == QLatin1String("workspace-write")
        ? QStringLiteral("workspace-write") : QStringLiteral("read-only");
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO bots(name,description,instructions,permission,provider,model,enabled,created_at,updated_at) "
        "VALUES(?,?,?,?,?,?,1,?,?)"));
    query.addBindValue(cleanName);
    query.addBindValue(description.trimmed());
    query.addBindValue(instructions.trimmed());
    query.addBindValue(perm);
    query.addBindValue(provider.trimmed());
    query.addBindValue(model.trimmed());
    query.addBindValue(nowIso());
    query.addBindValue(nowIso());
    if (!query.exec()) {
        setError(tr("Impossible de créer le bot : %1").arg(query.lastError().text()));
        return -1;
    }
    const int id = query.lastInsertId().toInt();
    setError({});
    m_selectedBotId = id;
    refresh();
    emit selectionChanged();
    return id;
}

bool BotStore::updateBot(int id, const QString &name, const QString &description,
                         const QString &instructions, const QString &permission,
                         bool enabled, const QString &provider, const QString &model) {
    if (!m_ready || id <= 0)
        return false;
    if (name.trimmed().isEmpty()) {
        setError(tr("Le nom du bot est obligatoire."));
        return false;
    }
    const QString perm = permission == QLatin1String("workspace-write")
        ? QStringLiteral("workspace-write") : QStringLiteral("read-only");
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE bots SET name=?,description=?,instructions=?,permission=?,provider=?,model=?,enabled=?,updated_at=? WHERE id=?"));
    query.addBindValue(name.trimmed());
    query.addBindValue(description.trimmed());
    query.addBindValue(instructions.trimmed());
    query.addBindValue(perm);
    query.addBindValue(provider.trimmed());
    query.addBindValue(model.trimmed());
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(nowIso());
    query.addBindValue(id);
    if (!query.exec()) {
        setError(tr("Impossible de mettre à jour le bot : %1").arg(query.lastError().text()));
        return false;
    }
    setError({});
    refresh();
    return true;
}

bool BotStore::removeBot(int id) {
    if (!m_ready || id <= 0)
        return false;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM bots WHERE id=?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(tr("Impossible de supprimer le bot : %1").arg(query.lastError().text()));
        return false;
    }
    if (m_selectedBotId == id)
        m_selectedBotId = -1;
    setError({});
    refresh();
    emit selectionChanged();
    return true;
}

int BotStore::appendMessage(int botId, const QString &role, const QString &content,
                            const QString &kind, int runId) {
    if (!m_ready || botId <= 0)
        return -1;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO bot_messages(bot_id,role,content,kind,run_id,created_at) VALUES(?,?,?,?,?,?)"));
    query.addBindValue(botId);
    query.addBindValue(role);
    query.addBindValue(content);
    query.addBindValue(kind);
    query.addBindValue(runId);
    query.addBindValue(nowIso());
    if (!query.exec()) {
        setError(query.lastError().text());
        return -1;
    }
    if (botId == m_selectedBotId)
        loadMessages();
    loadStats();
    emit changed();
    return query.lastInsertId().toInt();
}

void BotStore::clearMessages(int botId) {
    if (!m_ready || botId <= 0)
        return;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM bot_messages WHERE bot_id=?"));
    query.addBindValue(botId);
    query.exec();
    loadMessages();
    loadStats();
    emit changed();
}

QVariantList BotStore::messagesFor(int botId) const {
    QVariantList messages;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT id,role,content,kind,run_id,created_at FROM bot_messages WHERE bot_id=? ORDER BY id ASC LIMIT 500"));
    query.addBindValue(botId);
    if (query.exec()) {
        while (query.next()) {
            messages.append(QVariantMap{
                {QStringLiteral("id"), query.value(0).toInt()},
                {QStringLiteral("role"), query.value(1).toString()},
                {QStringLiteral("content"), query.value(2).toString()},
                {QStringLiteral("kind"), query.value(3).toString()},
                {QStringLiteral("runId"), query.value(4).toInt()},
                {QStringLiteral("time"), query.value(5).toString()},
            });
        }
    }
    return messages;
}

int BotStore::createRun(int botId, const QString &task, const QString &status) {
    if (!m_ready || botId <= 0)
        return -1;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO bot_runs(bot_id,task,status,created_at) VALUES(?,?,?,?)"));
    query.addBindValue(botId);
    query.addBindValue(task);
    query.addBindValue(status);
    query.addBindValue(nowIso());
    if (!query.exec()) {
        setError(query.lastError().text());
        return -1;
    }
    loadRuns();
    loadStats();
    emit changed();
    return query.lastInsertId().toInt();
}

bool BotStore::updateRun(int id, const QString &status, const QString &output,
                         const QString &error, int exitCode) {
    if (!m_ready || id <= 0)
        return false;
    QSqlQuery query(m_db);
    const bool done = status == QLatin1String("done") || status == QLatin1String("failed")
        || status == QLatin1String("cancelled");
    query.prepare(QStringLiteral(
        "UPDATE bot_runs SET status=?,output=?,error=?,exit_code=?,finished_at=? WHERE id=?"));
    query.addBindValue(status.isNull() ? QStringLiteral("") : status);
    query.addBindValue(output.isNull() ? QStringLiteral("") : output.left(200000));
    query.addBindValue(error.isNull() ? QStringLiteral("") : error.left(8000));
    query.addBindValue(exitCode);
    query.addBindValue(done ? nowIso() : QStringLiteral(""));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(query.lastError().text());
        return false;
    }
    loadRuns();
    loadStats();
    emit changed();
    return true;
}

QVariantMap BotStore::runById(int id) const {
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT r.id,r.bot_id,b.name,r.task,r.status,r.output,r.error,r.exit_code,r.created_at,r.finished_at "
        "FROM bot_runs r JOIN bots b ON b.id=r.bot_id WHERE r.id=?"));
    query.addBindValue(id);
    if (!query.exec() || !query.next())
        return {};
    return {
        {QStringLiteral("id"), query.value(0).toInt()},
        {QStringLiteral("botId"), query.value(1).toInt()},
        {QStringLiteral("botName"), query.value(2).toString()},
        {QStringLiteral("task"), query.value(3).toString()},
        {QStringLiteral("status"), query.value(4).toString()},
        {QStringLiteral("output"), query.value(5).toString()},
        {QStringLiteral("error"), query.value(6).toString()},
        {QStringLiteral("exitCode"), query.value(7).toInt()},
        {QStringLiteral("createdAt"), query.value(8).toString()},
        {QStringLiteral("finishedAt"), query.value(9).toString()},
    };
}

QVariantList BotStore::recentRuns(int botId, int limit) const {
    QVariantList runs;
    QSqlQuery query(m_db);
    if (botId > 0) {
        query.prepare(QStringLiteral(
            "SELECT r.id,r.bot_id,b.name,r.task,r.status,r.output,r.error,r.exit_code,r.created_at,r.finished_at "
            "FROM bot_runs r JOIN bots b ON b.id=r.bot_id WHERE r.bot_id=? ORDER BY r.id DESC LIMIT ?"));
        query.addBindValue(botId);
        query.addBindValue(qBound(1, limit, 100));
    } else {
        query.prepare(QStringLiteral(
            "SELECT r.id,r.bot_id,b.name,r.task,r.status,r.output,r.error,r.exit_code,r.created_at,r.finished_at "
            "FROM bot_runs r JOIN bots b ON b.id=r.bot_id ORDER BY r.id DESC LIMIT ?"));
        query.addBindValue(qBound(1, limit, 100));
    }
    if (query.exec()) {
        while (query.next()) {
            runs.append(QVariantMap{
                {QStringLiteral("id"), query.value(0).toInt()},
                {QStringLiteral("botId"), query.value(1).toInt()},
                {QStringLiteral("botName"), query.value(2).toString()},
                {QStringLiteral("task"), query.value(3).toString()},
                {QStringLiteral("status"), query.value(4).toString()},
                {QStringLiteral("output"), query.value(5).toString()},
                {QStringLiteral("error"), query.value(6).toString()},
                {QStringLiteral("exitCode"), query.value(7).toInt()},
                {QStringLiteral("createdAt"), query.value(8).toString()},
                {QStringLiteral("finishedAt"), query.value(9).toString()},
            });
        }
    }
    return runs;
}

int BotStore::createSchedule(int botId, const QString &title, const QString &prompt,
                             int intervalMinutes) {
    if (!m_ready || botId <= 0 || title.trimmed().isEmpty() || prompt.trimmed().isEmpty())
        return -1;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO bot_schedules(bot_id,title,prompt,interval_minutes,enabled,next_run,created_at) "
        "VALUES(?,?,?,?,1,?,?)"));
    query.addBindValue(botId);
    query.addBindValue(title.trimmed());
    query.addBindValue(prompt.trimmed());
    query.addBindValue(qMax(1, intervalMinutes));
    query.addBindValue(addMinutes(intervalMinutes));
    query.addBindValue(nowIso());
    if (!query.exec()) {
        setError(query.lastError().text());
        return -1;
    }
    loadSchedules();
    loadStats();
    emit changed();
    return query.lastInsertId().toInt();
}

bool BotStore::toggleSchedule(int id, bool enabled) {
    if (!m_ready || id <= 0)
        return false;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE bot_schedules SET enabled=?, next_run=CASE WHEN ?=1 THEN ? ELSE next_run END WHERE id=?"));
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(enabled ? addMinutes(60) : QString());
    query.addBindValue(id);
    if (!query.exec())
        return false;
    loadSchedules();
    emit changed();
    return true;
}

bool BotStore::removeSchedule(int id) {
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM bot_schedules WHERE id=?"));
    query.addBindValue(id);
    if (!query.exec())
        return false;
    loadSchedules();
    loadStats();
    emit changed();
    return true;
}

QVariantList BotStore::dueSchedules() const {
    QVariantList due;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT s.id,s.bot_id,s.title,s.prompt,s.interval_minutes FROM bot_schedules s "
        "JOIN bots b ON b.id=s.bot_id WHERE s.enabled=1 AND b.enabled=1 AND s.next_run<=? "
        "ORDER BY s.next_run ASC LIMIT 10"));
    query.addBindValue(nowIso());
    if (query.exec()) {
        while (query.next()) {
            due.append(QVariantMap{
                {QStringLiteral("id"), query.value(0).toInt()},
                {QStringLiteral("botId"), query.value(1).toInt()},
                {QStringLiteral("title"), query.value(2).toString()},
                {QStringLiteral("prompt"), query.value(3).toString()},
                {QStringLiteral("intervalMinutes"), query.value(4).toInt()},
            });
        }
    }
    return due;
}

void BotStore::markScheduleRun(int id, bool success, const QString &nextRun,
                               const QString &output) {
    Q_UNUSED(output)
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE bot_schedules SET last_run=?,last_status=?,next_run=? WHERE id=?"));
    query.addBindValue(nowIso());
    query.addBindValue(success ? QStringLiteral("done") : QStringLiteral("failed"));
    query.addBindValue(nextRun);
    query.addBindValue(id);
    query.exec();
    loadSchedules();
    emit changed();
}

void BotStore::clearError() {
    setError({});
}

void BotStore::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}
