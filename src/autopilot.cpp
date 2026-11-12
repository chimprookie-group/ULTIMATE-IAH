#include "autopilot.h"

#include "memorystore.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

#include <algorithm>

namespace {
int autopilotCounter = 0;
QString nowIso() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); }
QString shellSafeProject(const QString &project) {
    return QFileInfo(project).isDir() ? QFileInfo(project).absoluteFilePath() : QDir::homePath();
}
} // namespace

Autopilot::Autopilot(const QString &path, QObject *parent)
    : QObject(parent),
      m_path(path.isEmpty() ? defaultPath() : path),
      m_connection(QStringLiteral("autopilot-%1").arg(++autopilotCounter)) {
    if (!open())
        return;
    loadEnabled();
    loadJobs();
    m_timer.setInterval(20000);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        if (!m_enabled || m_process)
            return;
        QSqlQuery query(m_db);
        query.prepare(QStringLiteral(
            "SELECT id FROM autopilot_jobs WHERE enabled=1 AND status IN ('pending','failed') "
            "AND next_run IS NOT NULL AND next_run <= ? ORDER BY next_run LIMIT 1"));
        query.addBindValue(nowIso());
        if (query.exec() && query.next())
            startJob(query.value(0).toInt());
    });
    m_timer.start();

    m_timeout.setSingleShot(true);
    m_timeout.setInterval(30 * 60 * 1000);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        if (!m_process)
            return;
        m_process->kill();
        m_lastOutput += QStringLiteral("\n[timeout après 30 minutes]\n");
        emit outputChanged();
    });
}

Autopilot::~Autopilot() {
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(1500);
    }
    if (m_db.isValid()) {
        const QString name = m_db.connectionName();
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
    }
}

QString Autopilot::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/autopilot.db";
}

bool Autopilot::open() {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
    m_db.setDatabaseName(m_path);
    if (!m_db.open()) {
        setError(tr("Impossible d'ouvrir l'autopilote : %1").arg(m_db.lastError().text()));
        return false;
    }
    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    pragma.exec(QStringLiteral("PRAGMA busy_timeout=5000"));
    m_ready = migrate();
    QFile::setPermissions(m_path, QFile::ReadOwner | QFile::WriteOwner);
    if (m_ready) {
        QSqlQuery reset(m_db);
        reset.exec(QStringLiteral("UPDATE autopilot_jobs SET status='pending' WHERE status='running'"));
    }
    return m_ready;
}

bool Autopilot::migrate() {
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS autopilot_jobs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "project TEXT NOT NULL,"
            "title TEXT NOT NULL,"
            "prompt TEXT NOT NULL,"
            "interval_minutes INTEGER NOT NULL DEFAULT 0,"
            "status TEXT NOT NULL DEFAULT 'pending',"
            "enabled INTEGER NOT NULL DEFAULT 1,"
            "attempts INTEGER NOT NULL DEFAULT 0,"
            "max_attempts INTEGER NOT NULL DEFAULT 3,"
            "next_run TEXT,"
            "last_run TEXT,"
            "last_output TEXT NOT NULL DEFAULT '',"
            "last_error TEXT NOT NULL DEFAULT '',"
            "created_at TEXT NOT NULL,"
            "updated_at TEXT NOT NULL)"))) {
        setError(query.lastError().text());
        return false;
    }
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS autopilot_runs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "job_id INTEGER NOT NULL,"
        "started_at TEXT NOT NULL,"
        "finished_at TEXT,"
        "exit_code INTEGER,"
        "output TEXT NOT NULL DEFAULT ''"
        ")"));
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS autopilot_meta (key TEXT PRIMARY KEY, value TEXT)"));
    return true;
}

void Autopilot::loadEnabled() {
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT value FROM autopilot_meta WHERE key='enabled'"));
    if (query.exec() && query.next())
        m_enabled = query.value(0).toString() == QLatin1String("1");
}

void Autopilot::saveEnabled() {
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT INTO autopilot_meta(key,value) VALUES('enabled',?) "
                                 "ON CONFLICT(key) DO UPDATE SET value=excluded.value"));
    query.addBindValue(m_enabled ? QStringLiteral("1") : QStringLiteral("0"));
    query.exec();
}

void Autopilot::loadJobs() {
    QSqlQuery query(m_db);
    QVariantList jobs;
    if (query.exec(QStringLiteral(
            "SELECT id,project,title,prompt,interval_minutes,status,enabled,attempts,max_attempts,"
            "next_run,last_run,last_output,last_error,created_at,updated_at "
            "FROM autopilot_jobs ORDER BY enabled DESC, updated_at DESC LIMIT 200"))) {
        while (query.next()) {
            jobs.append(QVariantMap{
                {"id", query.value(0).toInt()},
                {"project", query.value(1).toString()},
                {"title", query.value(2).toString()},
                {"prompt", query.value(3).toString()},
                {"intervalMinutes", query.value(4).toInt()},
                {"status", query.value(5).toString()},
                {"enabled", query.value(6).toBool()},
                {"attempts", query.value(7).toInt()},
                {"maxAttempts", query.value(8).toInt()},
                {"nextRun", query.value(9).toString()},
                {"lastRun", query.value(10).toString()},
                {"lastOutput", query.value(11).toString()},
                {"lastError", query.value(12).toString()},
                {"createdAt", query.value(13).toString()},
                {"updatedAt", query.value(14).toString()},
            });
        }
    }
    m_jobs = jobs;
    emit changed();
}

void Autopilot::refresh() {
    if (m_ready)
        loadJobs();
}

int Autopilot::addJob(const QString &title, const QString &prompt, int intervalMinutes,
                      const QString &project, int maxAttempts) {
    if (!m_ready || title.trimmed().isEmpty() || prompt.trimmed().isEmpty()) {
        setError(tr("Le titre et la tâche sont obligatoires."));
        return -1;
    }
    if (project.isEmpty() || !QFileInfo(project).isDir()) {
        setError(tr("Le projet de la tâche n'existe pas."));
        return -1;
    }
    const QString timestamp = nowIso();
    const QString next = intervalMinutes > 0
        ? QDateTime::currentDateTimeUtc().addSecs(intervalMinutes * 60).toString(Qt::ISODateWithMs)
        : QString();
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO autopilot_jobs(project,title,prompt,interval_minutes,status,enabled,attempts,"
        "max_attempts,next_run,created_at,updated_at) VALUES(?,?,?,?, 'pending',1,0,?,?,?,?)"));
    query.addBindValue(QFileInfo(project).absoluteFilePath());
    query.addBindValue(title.trimmed());
    query.addBindValue(prompt.trimmed());
    query.addBindValue(intervalMinutes);
    query.addBindValue(qBound(1, maxAttempts, 10));
    query.addBindValue(next);
    query.addBindValue(timestamp);
    query.addBindValue(timestamp);
    if (!query.exec()) {
        setError(query.lastError().text());
        return -1;
    }
    const int id = query.lastInsertId().toInt();
    setError({});
    loadJobs();
    emit toast(tr("Tâche autonome ajoutée."));
    return id;
}

bool Autopilot::removeJob(int id) {
    if (!m_ready || id == m_currentJobId)
        return false;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM autopilot_jobs WHERE id=?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setError(query.lastError().text());
        return false;
    }
    loadJobs();
    return true;
}

void Autopilot::runNow(int id) {
    if (!m_ready || m_process)
        return;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE autopilot_jobs SET status='pending', next_run=?, attempts=0, updated_at=? WHERE id=?"));
    query.addBindValue(nowIso());
    query.addBindValue(nowIso());
    query.addBindValue(id);
    query.exec();
    loadJobs();
    startJob(id);
}

void Autopilot::toggleJob(int id) {
    if (!m_ready)
        return;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE autopilot_jobs SET enabled=1-enabled, status=CASE WHEN enabled=1 THEN 'paused' ELSE 'pending' END, updated_at=? WHERE id=?"));
    query.addBindValue(nowIso());
    query.addBindValue(id);
    query.exec();
    loadJobs();
}

void Autopilot::setEnabled(bool enabled) {
    if (enabled == m_enabled)
        return;
    m_enabled = enabled;
    saveEnabled();
    emit enabledChanged();
    emit toast(enabled ? tr("Autopilote activé.") : tr("Autopilote en pause."));
}

QVariantMap Autopilot::jobById(int id) const {
    for (const QVariant &value : m_jobs)
        if (value.toMap().value("id").toInt() == id)
            return value.toMap();
    return {};
}

QProcessEnvironment Autopilot::agentEnvironment() const {
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    QFile file(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/agent.env");
    if (file.open(QIODevice::ReadOnly)) {
        const QStringList lines = QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const int equals = line.indexOf('=');
            if (equals <= 0)
                continue;
            const QString key = line.left(equals).trimmed();
            const QString value = line.mid(equals + 1).trimmed();
            if (!key.isEmpty() && !value.isEmpty() && key.at(0).isLetter())
                environment.insert(key, value);
        }
    }
    return environment;
}

QString Autopilot::enrichedPrompt(const QVariantMap &job) const {
    QString prompt = job.value("prompt").toString();
    if (m_memory) {
        const QString context = m_memory->recall(job.value("project").toString(), prompt, 8);
        if (!context.isEmpty())
            prompt = tr("Mémoire persistante du projet (contexte fiable, ne la répète pas) :\n%1\n\nTâche autonome :\n%2")
                         .arg(context, prompt);
    }
    return prompt;
}

void Autopilot::startJob(int id) {
    if (!m_ready || m_process)
        return;
    const QVariantMap job = jobById(id);
    if (job.isEmpty()) {
        setError(tr("Tâche introuvable."));
        return;
    }
    const QString project = job.value("project").toString();
    if (!QFileInfo(project).isDir()) {
        setError(tr("Le dossier du projet est absent : %1").arg(project));
        updateJobState(id, QStringLiteral("failed"), m_error);
        return;
    }
    m_currentJobId = id;
    m_currentTitle = job.value("title").toString();
    m_currentAttempts = job.value("attempts").toInt();
    m_currentMaxAttempts = qMax(1, job.value("maxAttempts").toInt());
    m_lastOutput.clear();
    emit outputChanged();
    updateJobState(id, QStringLiteral("running"));
    setRunning(true, id, m_currentTitle);

    QSqlQuery run(m_db);
    run.prepare(QStringLiteral(
        "INSERT INTO autopilot_runs(job_id,started_at) VALUES(?,?)"));
    run.addBindValue(id);
    run.addBindValue(nowIso());
    run.exec();

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->setWorkingDirectory(project);
    m_process->setProcessEnvironment(agentEnvironment());
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
        if (!m_process)
            return;
        const QString chunk = QString::fromUtf8(m_process->readAllStandardOutput());
        if (m_lastOutput.size() < 300000) {
            m_lastOutput += chunk;
            if (m_lastOutput.size() > 300000)
                m_lastOutput = m_lastOutput.left(300000) + QStringLiteral("\n[tronqué]\n");
            emit outputChanged();
        }
    });
    connect(m_process, &QProcess::finished, this, [this, job](int code, QProcess::ExitStatus status) {
        const bool success = status == QProcess::NormalExit && code == 0;
        QString output = m_lastOutput + QString::fromUtf8(m_process->readAllStandardOutput());
        if (success) {
            m_currentAttempts = 0;
        } else {
            ++m_currentAttempts;
        }
        const int id = m_currentJobId;
        const int attempts = m_currentAttempts;
        const int maxAttempts = m_currentMaxAttempts;
        const QString error = success ? QString() : tr("dsh headless a échoué (code %1).").arg(code);
        if (success && m_memory && !output.trimmed().isEmpty()) {
            m_memory->remember(
                projectOfJob(id), QStringLiteral("result"),
                tr("Autopilote : %1").arg(m_currentTitle),
                output.left(6000), QStringLiteral("autopilot,result"), false, 3,
                QStringLiteral("autopilot"));
        }
        QSqlQuery run(m_db);
        run.prepare(QStringLiteral(
            "UPDATE autopilot_runs SET finished_at=?, exit_code=?, output=? "
            "WHERE id=(SELECT id FROM autopilot_runs WHERE job_id=? ORDER BY id DESC LIMIT 1)"));
        run.addBindValue(nowIso());
        run.addBindValue(code);
        run.addBindValue(output.left(200000));
        run.addBindValue(id);
        run.exec();

        if (success) {
            QSqlQuery update(m_db);
            update.prepare(QStringLiteral(
                "UPDATE autopilot_jobs SET status='done', attempts=0, last_run=?, last_output=?, last_error='', updated_at=? WHERE id=?"));
            update.addBindValue(nowIso());
            update.addBindValue(output.left(20000));
            update.addBindValue(nowIso());
            update.addBindValue(id);
            update.exec();
            scheduleNext(id, job.value("intervalMinutes").toInt(), true);
        } else {
            QSqlQuery update(m_db);
            update.prepare(QStringLiteral(
                "UPDATE autopilot_jobs SET status=?, attempts=?, last_run=?, last_output=?, last_error=?, updated_at=? WHERE id=?"));
            const bool retry = attempts < maxAttempts;
            update.addBindValue(retry ? QStringLiteral("pending") : QStringLiteral("failed"));
            update.addBindValue(attempts);
            update.addBindValue(nowIso());
            update.addBindValue(output.left(20000));
            update.addBindValue(error);
            update.addBindValue(nowIso());
            update.addBindValue(id);
            update.exec();
            if (retry) {
                QSqlQuery next(m_db);
                next.prepare(QStringLiteral("UPDATE autopilot_jobs SET next_run=? WHERE id=?"));
                next.addBindValue(QDateTime::currentDateTimeUtc().addSecs(5 * 60).toString(Qt::ISODateWithMs));
                next.addBindValue(id);
                next.exec();
            }
        }
        m_process->deleteLater();
        m_process = nullptr;
        m_timeout.stop();
        setRunning(false);
        m_currentJobId = -1;
        loadJobs();
        emit completed(id, success, output.left(700));
        emit toast(success ? tr("Tâche autonome terminée.") : tr("Tâche autonome en échec."));
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !m_process)
            return;
        const int id = m_currentJobId;
        QSqlQuery update(m_db);
        update.prepare(QStringLiteral(
            "UPDATE autopilot_jobs SET status='failed', last_error=?, updated_at=? WHERE id=?"));
        update.addBindValue(tr("Le moteur agent est introuvable ou ne peut pas démarrer."));
        update.addBindValue(nowIso());
        update.addBindValue(id);
        update.exec();
        m_process->deleteLater();
        m_process = nullptr;
        m_timeout.stop();
        setRunning(false);
        m_currentJobId = -1;
        loadJobs();
        emit toast(tr("Tâche autonome impossible à démarrer."));
    });
    m_timeout.start();
    m_process->start(m_dshBinary, {QStringLiteral("--agent-job"),
                                   QStringLiteral("--agent-prompt"), enrichedPrompt(job),
                                   QStringLiteral("--workspace"), project,
                                   QStringLiteral("--permission"), QStringLiteral("workspace-write")});
}

QString Autopilot::projectOfJob(int id) const {
    return jobById(id).value("project").toString();
}

void Autopilot::scheduleNext(int id, int intervalMinutes, bool success)
{
    if (!success)
        return;
    QSqlQuery query(m_db);
    if (intervalMinutes > 0) {
        query.prepare(QStringLiteral(
            "UPDATE autopilot_jobs SET status='pending', next_run=?, updated_at=? WHERE id=?"));
        query.addBindValue(QDateTime::currentDateTimeUtc().addSecs(intervalMinutes * 60).toString(Qt::ISODateWithMs));
        query.addBindValue(nowIso());
        query.addBindValue(id);
    } else {
        query.prepare(QStringLiteral(
            "UPDATE autopilot_jobs SET enabled=0, status='done', next_run=NULL, updated_at=? WHERE id=?"));
        query.addBindValue(nowIso());
        query.addBindValue(id);
    }
    query.exec();
}

void Autopilot::updateJobState(int id, const QString &status, const QString &error) {
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE autopilot_jobs SET status=?, last_error=?, updated_at=? WHERE id=?"));
    query.addBindValue(status);
    query.addBindValue(error);
    query.addBindValue(nowIso());
    query.addBindValue(id);
    query.exec();
}

void Autopilot::setDshBinary(const QString &binary) {
    if (!binary.trimmed().isEmpty())
        m_dshBinary = binary.trimmed();
}

void Autopilot::setRunning(bool running, int id, const QString &title) {
    m_currentJobId = id;
    m_currentTitle = title;
    if (!running) {
        m_currentTitle.clear();
    }
    emit runningChanged();
}

void Autopilot::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}

void Autopilot::clearError() { setError({}); }
