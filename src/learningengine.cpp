#include "learningengine.h"

#include "skillmanager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace {
QString now() { return QDateTime::currentDateTime().toString(Qt::ISODate); }
QString compact(const QString &text, int limit) {
    return text.simplified().left(limit);
}
}

LearningEngine::LearningEngine(const QString &path, SkillManager *skills, QObject *parent)
    : QObject(parent),
      m_path(path.isEmpty() ? defaultPath() : path),
      m_connection(QStringLiteral("learning-%1").arg(quintptr(this))),
      m_skills(skills) {
    if (!open() || !migrate()) {
        setError(tr("Impossible d'initialiser la base d'apprentissage."));
        return;
    }
    m_ready = true;
    refresh();
}

LearningEngine::~LearningEngine() {
    if (QSqlDatabase::contains(m_connection)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connection, false);
            if (db.isOpen()) db.close();
        }
        QSqlDatabase::removeDatabase(m_connection);
    }
}

QString LearningEngine::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/learning.db");
}

bool LearningEngine::open() {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
    db.setDatabaseName(m_path);
    if (!db.open()) {
        setError(db.lastError().text());
        return false;
    }
    QSqlQuery pragma(db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    QFile::setPermissions(m_path, QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

bool LearningEngine::migrate() {
    QSqlDatabase db = QSqlDatabase::database(m_connection, false);
    QSqlQuery query(db);
    const QStringList statements = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS events ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, created_at TEXT NOT NULL,"
                       "project TEXT NOT NULL, prompt TEXT NOT NULL, outcome TEXT NOT NULL,"
                       "model TEXT, tool_count INTEGER NOT NULL DEFAULT 0,"
                       "failed_tools TEXT, error TEXT, lesson_id INTEGER)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS lessons ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, created_at TEXT NOT NULL,"
                       "updated_at TEXT NOT NULL, project TEXT NOT NULL, kind TEXT NOT NULL,"
                       "title TEXT NOT NULL, body TEXT NOT NULL, evidence TEXT,"
                       "confidence REAL NOT NULL DEFAULT 0.4, uses INTEGER NOT NULL DEFAULT 1,"
                       "enabled INTEGER NOT NULL DEFAULT 1, auto_generated INTEGER NOT NULL DEFAULT 1,"
                       "UNIQUE(project, kind, title))"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS proposals ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, created_at TEXT NOT NULL,"
                       "project TEXT NOT NULL, title TEXT NOT NULL, body TEXT NOT NULL,"
                       "status TEXT NOT NULL DEFAULT 'pending', lesson_id INTEGER,"
                       "UNIQUE(project, title))"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS settings (key TEXT PRIMARY KEY, value TEXT NOT NULL)"),
    };
    for (const QString &statement : statements) {
        if (!query.exec(statement)) {
            setError(query.lastError().text());
            return false;
        }
    }
    query.prepare(QStringLiteral("INSERT OR IGNORE INTO settings(key, value) VALUES('enabled','1')"));
    query.exec();
    query.exec(QStringLiteral("SELECT value FROM settings WHERE key='enabled'"));
    if (query.next()) m_enabled = query.value(0).toString() != QLatin1String("0");
    return true;
}

void LearningEngine::setError(const QString &error) {
    if (m_error == error) return;
    m_error = error;
    emit errorChanged();
}

void LearningEngine::clearError() { setError({}); }

QString LearningEngine::redact(QString text) {
    static const QList<QRegularExpression> patterns = {
        QRegularExpression(QStringLiteral("\\bsk-[A-Za-z0-9_-]{12,}\\b")),
        QRegularExpression(QStringLiteral("\\bgh[pousr]_[A-Za-z0-9]{12,}\\b")),
        QRegularExpression(QStringLiteral("(?i)\\bBearer\\s+[A-Za-z0-9._~+/-]{12,}")),
        QRegularExpression(QStringLiteral("(?i)(api[_-]?key|token|secret)\\s*[:=]\\s*[^\\s,;]+")),
    };
    for (const QRegularExpression &pattern : patterns)
        text.replace(pattern, QStringLiteral("[redacted]"));
    return text;
}

void LearningEngine::setEnabled(bool value) {
    if (m_enabled == value) return;
    m_enabled = value;
    if (m_ready) {
        QSqlQuery query(QSqlDatabase::database(m_connection, false));
        query.prepare(QStringLiteral("INSERT OR REPLACE INTO settings(key,value) VALUES('enabled',?)"));
        query.addBindValue(value ? QStringLiteral("1") : QStringLiteral("0"));
        query.exec();
    }
    emit changed();
}

QString LearningEngine::categoryOf(const QString &prompt) {
    const QString text = prompt.toLower();
    if (text.contains(QStringLiteral("bug")) || text.contains(QStringLiteral("erreur"))
        || text.contains(QStringLiteral("fix"))) return QStringLiteral("debug");
    if (text.contains(QStringLiteral("test"))) return QStringLiteral("tests");
    if (text.contains(QStringLiteral("github")) || text.contains(QStringLiteral("pull request"))
        || text.contains(QStringLiteral("issue"))) return QStringLiteral("git");
    if (text.contains(QStringLiteral("note")) || text.contains(QStringLiteral("vault"))) return QStringLiteral("notes");
    if (text.contains(QStringLiteral("web")) || text.contains(QStringLiteral("recherche"))) return QStringLiteral("research");
    if (text.contains(QStringLiteral("design")) || text.contains(QStringLiteral("qml"))
        || text.contains(QStringLiteral("ui"))) return QStringLiteral("design");
    return QStringLiteral("general");
}

int LearningEngine::upsertLesson(const QString &project, const QString &kind,
                                 const QString &title, const QString &body,
                                 const QString &evidence, double confidence) {
    QSqlDatabase db = QSqlDatabase::database(m_connection, false);
    QSqlQuery existing(db);
    existing.prepare(QStringLiteral("SELECT id, confidence, uses FROM lessons WHERE project=? AND kind=? AND title=?"));
    existing.addBindValue(project);
    existing.addBindValue(kind);
    existing.addBindValue(title);
    existing.exec();
    const QString timestamp = now();
    if (existing.next()) {
        const int id = existing.value(0).toInt();
        const double nextConfidence = qMin(0.98, qMax(existing.value(1).toDouble(), confidence) + 0.06);
        QSqlQuery update(db);
        update.prepare(QStringLiteral("UPDATE lessons SET updated_at=?, confidence=?, uses=uses+1, "
                                      "body=?, evidence=?, enabled=1 WHERE id=?"));
        update.addBindValue(timestamp);
        update.addBindValue(nextConfidence);
        update.addBindValue(body);
        update.addBindValue(evidence);
        update.addBindValue(id);
        update.exec();
        return id;
    }
    QSqlQuery insert(db);
    insert.prepare(QStringLiteral("INSERT INTO lessons(created_at,updated_at,project,kind,title,body,evidence,"
                                  "confidence,uses,enabled,auto_generated) VALUES(?,?,?,?,?,?,?,?,1,1,1)"));
    insert.addBindValue(timestamp);
    insert.addBindValue(timestamp);
    insert.addBindValue(project);
    insert.addBindValue(kind);
    insert.addBindValue(title);
    insert.addBindValue(body);
    insert.addBindValue(evidence);
    insert.addBindValue(confidence);
    insert.exec();
    return insert.lastInsertId().toInt();
}

void LearningEngine::maybePropose(int lessonId, const QString &project,
                                 const QString &title, const QString &body) {
    QSqlQuery find(QSqlDatabase::database(m_connection, false));
    find.prepare(QStringLiteral("SELECT uses, confidence FROM lessons WHERE id=?"));
    find.addBindValue(lessonId);
    find.exec();
    if (!find.next()) return;
    if (find.value(0).toInt() < 2 && find.value(1).toDouble() < 0.75) return;
    const QString proposalTitle = tr("Skill proposé : %1").arg(title);
    QSqlQuery insert(QSqlDatabase::database(m_connection, false));
    insert.prepare(QStringLiteral("INSERT OR IGNORE INTO proposals(created_at,project,title,body,status,lesson_id) "
                                  "VALUES(?,?,?,?,'pending',?)"));
    insert.addBindValue(now());
    insert.addBindValue(project);
    insert.addBindValue(proposalTitle);
    insert.addBindValue(body);
    insert.addBindValue(lessonId);
    insert.exec();
}

QVariantMap LearningEngine::observe(const QJsonObject &trajectory) {
    if (!m_ready) return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), m_error}};
    if (!m_enabled) return {{QStringLiteral("ok"), true}, {QStringLiteral("learned"), false}};
    const QString prompt = redact(trajectory.value(QStringLiteral("prompt")).toString()).left(4000);
    if (prompt.trimmed().isEmpty())
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Prompt vide.")}};
    const QString project = trajectory.value(QStringLiteral("project")).toString();
    const bool success = trajectory.value(QStringLiteral("ok")).toBool(false);
    const QString answer = redact(trajectory.value(QStringLiteral("answer")).toString()).left(4000);
    const QString error = redact(trajectory.value(QStringLiteral("error")).toString()).left(1200);
    const QString model = trajectory.value(QStringLiteral("model")).toString().left(120);
    const QJsonArray tools = trajectory.value(QStringLiteral("tools")).toArray();
    QStringList failed;
    QStringList sequence;
    for (const QJsonValue &value : tools) {
        const QJsonObject tool = value.toObject();
        const QString name = tool.value(QStringLiteral("name")).toString();
        if (name.isEmpty()) continue;
        sequence.append(name);
        if (!tool.value(QStringLiteral("ok")).toBool(true))
            failed.append(name + QStringLiteral(": ") + tool.value(QStringLiteral("detail")).toString().left(160));
    }
    const QString timestamp = now();
    QSqlQuery event(QSqlDatabase::database(m_connection, false));
    event.prepare(QStringLiteral("INSERT INTO events(created_at,project,prompt,outcome,model,tool_count,"
                                 "failed_tools,error,lesson_id) VALUES(?,?,?,?,?,?,?,?,NULL)"));
    event.addBindValue(timestamp);
    event.addBindValue(project);
    event.addBindValue(prompt);
    event.addBindValue(success ? QStringLiteral("success") : QStringLiteral("failure"));
    event.addBindValue(model);
    event.addBindValue(tools.size());
    event.addBindValue(failed.join(QStringLiteral("; ")).left(1000));
    event.addBindValue(error);
    event.exec();
    const int eventId = event.lastInsertId().toInt();

    int lessonId = 0;
    const QString evidenceText = error.isEmpty() ? failed.join(QStringLiteral("; ")) : error;
    const QString evidenceLower = evidenceText.toLower();
    const bool hallucinatedTool = evidenceLower.contains(QStringLiteral("outil inconnu"))
        || evidenceLower.contains(QStringLiteral("unknown tool"));
    const bool policyRefusal = evidenceLower.contains(QStringLiteral("redirection"))
        || evidenceLower.contains(QStringLiteral("web_redirect_blocked"));
    if ((!success || !failed.isEmpty()) && !hallucinatedTool && !policyRefusal) {
        const QString evidence = evidenceText;
        QString title;
        if (!failed.isEmpty()) {
            const QString first = failed.first();
            const QString tool = first.section(QLatin1Char(':'), 0, 0).trimmed();
            const QString cause = first.section(QLatin1Char(':'), 1).trimmed();
            const QRegularExpression http(QStringLiteral("HTTP (\\d{3})"));
            const auto match = http.match(cause);
            if (match.hasMatch())
                title = tr("Échec %1 : HTTP %2").arg(tool, match.captured(1));
            else if (cause.contains(QStringLiteral("redirection"), Qt::CaseInsensitive))
                title = tr("Échec %1 : redirection refusée").arg(tool);
            else
                title = tr("Échec %1 : %2").arg(tool, cause.left(80));
        } else {
            title = tr("Échec : %1").arg(error.left(80));
        }
        const QString body = tr("Contexte : %1\nÉchec observé : %2\nRègle apprise : reproduire d'abord, "
                                "lire l'erreur réelle, vérifier la configuration et ne pas répéter la même "
                                "tentative sans nouvelle preuve.")
                                 .arg(prompt.left(500), evidence.left(700));
        lessonId = upsertLesson(project, QStringLiteral("failure"), title, body, evidence, 0.55);
    } else if (success && sequence.size() >= 3
               && QSet<QString>(sequence.begin(), sequence.end()).size() >= 2) {
        const QString title = tr("Procédure : %1").arg(sequence.join(QStringLiteral(" → ")));
        const QString evidence = sequence.join(QStringLiteral(" → "));
        const QString body = tr("Pour une tâche « %1 », la séquence d'outils qui a fonctionné est : %2.\n"
                                "Vérifier le résultat réel avant de conclure.")
                                 .arg(prompt.left(400), evidence);
        lessonId = upsertLesson(project, QStringLiteral("workflow"), title, body, evidence, 0.42);
    }
    if (lessonId > 0) {
        QSqlQuery link(QSqlDatabase::database(m_connection, false));
        link.prepare(QStringLiteral("UPDATE events SET lesson_id=? WHERE id=?"));
        link.addBindValue(lessonId);
        link.addBindValue(eventId);
        link.exec();
        QSqlQuery lesson(QSqlDatabase::database(m_connection, false));
        lesson.prepare(QStringLiteral("SELECT title, body FROM lessons WHERE id=?"));
        lesson.addBindValue(lessonId);
        if (lesson.exec() && lesson.next())
            maybePropose(lessonId, project, lesson.value(0).toString(), lesson.value(1).toString());
    }
    QSqlQuery trim(QSqlDatabase::database(m_connection, false));
    trim.exec(QStringLiteral("DELETE FROM events WHERE id NOT IN (SELECT id FROM events ORDER BY id DESC LIMIT 1000)"));
    trim.exec(QStringLiteral("DELETE FROM lessons WHERE id NOT IN (SELECT id FROM lessons ORDER BY confidence DESC, uses DESC LIMIT 200)"));
    refresh();
    return {{QStringLiteral("ok"), true},
            {QStringLiteral("learned"), lessonId > 0},
            {QStringLiteral("lessonId"), lessonId},
            {QStringLiteral("eventId"), eventId},
            {QStringLiteral("lessonCount"), m_stats.value(QStringLiteral("lessons")).toInt()}};
}

void LearningEngine::refresh() {
    if (!m_ready) return;
    QSqlDatabase db = QSqlDatabase::database(m_connection, false);
    m_lessons.clear();
    QSqlQuery lessons(db);
    lessons.exec(QStringLiteral("SELECT id,created_at,updated_at,project,kind,title,body,evidence,confidence,uses "
                                "FROM lessons WHERE enabled=1 ORDER BY confidence DESC, uses DESC, updated_at DESC LIMIT 100"));
    while (lessons.next()) {
        m_lessons.append(QVariantMap{{QStringLiteral("id"), lessons.value(0).toInt()},
                                     {QStringLiteral("createdAt"), lessons.value(1).toString()},
                                     {QStringLiteral("updatedAt"), lessons.value(2).toString()},
                                     {QStringLiteral("project"), lessons.value(3).toString()},
                                     {QStringLiteral("kind"), lessons.value(4).toString()},
                                     {QStringLiteral("title"), lessons.value(5).toString()},
                                     {QStringLiteral("body"), lessons.value(6).toString()},
                                     {QStringLiteral("evidence"), lessons.value(7).toString()},
                                     {QStringLiteral("confidence"), lessons.value(8).toDouble()},
                                     {QStringLiteral("uses"), lessons.value(9).toInt()}});
    }
    m_proposals.clear();
    QSqlQuery proposals(db);
    proposals.exec(QStringLiteral("SELECT id,created_at,project,title,body,status,lesson_id FROM proposals "
                                  "WHERE status='pending' ORDER BY id DESC LIMIT 50"));
    while (proposals.next()) {
        m_proposals.append(QVariantMap{{QStringLiteral("id"), proposals.value(0).toInt()},
                                       {QStringLiteral("createdAt"), proposals.value(1).toString()},
                                       {QStringLiteral("project"), proposals.value(2).toString()},
                                       {QStringLiteral("title"), proposals.value(3).toString()},
                                       {QStringLiteral("body"), proposals.value(4).toString()},
                                       {QStringLiteral("status"), proposals.value(5).toString()},
                                       {QStringLiteral("lessonId"), proposals.value(6).toInt()}});
    }
    m_events.clear();
    QSqlQuery events(db);
    events.exec(QStringLiteral("SELECT id,created_at,prompt,outcome,model,tool_count,error FROM events ORDER BY id DESC LIMIT 30"));
    while (events.next()) {
        m_events.append(QVariantMap{{QStringLiteral("id"), events.value(0).toInt()},
                                    {QStringLiteral("createdAt"), events.value(1).toString()},
                                    {QStringLiteral("prompt"), events.value(2).toString()},
                                    {QStringLiteral("outcome"), events.value(3).toString()},
                                    {QStringLiteral("model"), events.value(4).toString()},
                                    {QStringLiteral("toolCount"), events.value(5).toInt()},
                                    {QStringLiteral("error"), events.value(6).toString()}});
    }
    QSqlQuery metric(db);
    metric.exec(QStringLiteral("SELECT COUNT(*), SUM(CASE WHEN outcome='success' THEN 1 ELSE 0 END) FROM events"));
    QFile::setPermissions(m_path, QFile::ReadOwner | QFile::WriteOwner);
    const bool hasMetric = metric.next();
    const int totalEvents = hasMetric ? metric.value(0).toInt() : 0;
    const int successes = hasMetric ? metric.value(1).toInt() : 0;
    metric.exec(QStringLiteral("SELECT COUNT(*) FROM lessons WHERE enabled=1"));
    const int lessonsCount = metric.next() ? metric.value(0).toInt() : 0;
    metric.exec(QStringLiteral("SELECT COUNT(*) FROM proposals WHERE status='pending'"));
    const int proposalsCount = metric.next() ? metric.value(0).toInt() : 0;
    m_stats = QVariantMap{{QStringLiteral("events"), totalEvents},
                          {QStringLiteral("successes"), successes},
                          {QStringLiteral("lessons"), lessonsCount},
                          {QStringLiteral("proposals"), proposalsCount},
                          {QStringLiteral("enabled"), m_enabled}};
    emit changed();
}

QString LearningEngine::contextFor(const QString &prompt, const QString &project, int limit) const {
    if (!m_enabled || !m_ready) return {};
    QStringList terms;
    for (const QString &term : prompt.toLower().split(QRegularExpression(QStringLiteral("[^a-z0-9àâçéèêëîïôûùüÿñæœ]+")),
                                                      Qt::SkipEmptyParts)) {
        if (term.size() > 3) terms.append(term);
    }
    QStringList lines;
    for (const QVariant &value : m_lessons) {
        const QVariantMap lesson = value.toMap();
        if (!project.isEmpty() && !lesson.value(QStringLiteral("project")).toString().isEmpty()
            && lesson.value(QStringLiteral("project")).toString() != project)
            continue;
        const QString haystack = (lesson.value(QStringLiteral("title")).toString() + " "
                                  + lesson.value(QStringLiteral("body")).toString()).toLower();
        int score = 0;
        for (const QString &term : terms)
            if (haystack.contains(term)) ++score;
        if (score == 0 && lesson.value(QStringLiteral("kind")).toString() != QLatin1String("failure"))
            continue;
        lines.append(QStringLiteral("- [%1 · %2] %3")
                         .arg(lesson.value(QStringLiteral("kind")).toString(),
                              QString::number(lesson.value(QStringLiteral("confidence")).toDouble(), 'f', 2),
                              lesson.value(QStringLiteral("body")).toString().left(700)));
        if (lines.size() >= limit) break;
    }
    if (lines.isEmpty()) return {};
    return QStringLiteral("LEÇONS APPRISES (auto-apprentissage local)\n") + lines.join(QLatin1Char('\n'));
}

bool LearningEngine::approveProposal(int id) {
    QSqlDatabase db = QSqlDatabase::database(m_connection, false);
    QSqlQuery find(db);
    find.prepare(QStringLiteral("SELECT project,title,body,lesson_id FROM proposals WHERE id=? AND status='pending'"));
    find.addBindValue(id);
    if (!find.exec() || !find.next()) return false;
    const QString title = find.value(1).toString();
    const QString body = find.value(2).toString();
    if (m_skills) {
        // The proposal title carries a display prefix; the skill itself must use
        // the lesson title so its id stays readable.
        const QString prefix = tr("Skill proposé : ");
        QString name = title;
        if (name.startsWith(prefix))
            name = name.mid(prefix.size()).trimmed();
        if (name.isEmpty())
            name = title;
        if (!m_skills->createSkill(name, tr("Skill appris automatiquement par Ultimate IAH."), body))
            return false;
    }
    QSqlQuery update(db);
    update.prepare(QStringLiteral("UPDATE proposals SET status='approved' WHERE id=?"));
    update.addBindValue(id);
    if (!update.exec()) return false;
    if (m_skills) m_skills->refresh();
    refresh();
    return true;
}

bool LearningEngine::rejectProposal(int id) {
    QSqlQuery query(QSqlDatabase::database(m_connection, false));
    query.prepare(QStringLiteral("UPDATE proposals SET status='rejected' WHERE id=?"));
    query.addBindValue(id);
    if (!query.exec()) return false;
    refresh();
    return true;
}

bool LearningEngine::clear() {
    QSqlQuery query(QSqlDatabase::database(m_connection, false));
    query.exec(QStringLiteral("DELETE FROM proposals"));
    query.exec(QStringLiteral("DELETE FROM lessons"));
    query.exec(QStringLiteral("DELETE FROM events"));
    refresh();
    return true;
}

