#include "botrunner.h"

#include <QCoreApplication>
#include <QFile>

#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

BotRunner::BotRunner(QObject *parent)
    : QObject(parent) {
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(5 * 60 * 1000);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        if (!m_process)
            return;
        setStatus(tr("Délai dépassé"));
        m_process->kill();
    });
}

QProcessEnvironment BotRunner::agentEnvironment() const {
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    QFile file(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
               + QStringLiteral("/agent.env"));
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

void BotRunner::start(int runId, const QString &botName, const QString &prompt,
                      const QString &permission, const QString &workspace,
                      const QString &dshBinary) {
    if (m_process)
        return;
    if (prompt.trimmed().isEmpty()) {
        emit finished(runId, false, {}, tr("La tâche est vide."));
        return;
    }
    if (!QFileInfo(workspace).isDir()) {
        emit finished(runId, false, {}, tr("Le workspace est introuvable : %1").arg(workspace));
        return;
    }
    const QString binary = dshBinary.trimmed().isEmpty()
        ? QCoreApplication::applicationFilePath() : dshBinary;
    m_runId = runId;
    m_output.clear();
    m_cancelled = false;
    emit outputChanged();
    setStatus(tr("%1 travaille…").arg(botName));

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    m_process->setWorkingDirectory(workspace);
    m_process->setProcessEnvironment(agentEnvironment());
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
        if (!m_process)
            return;
        const QString chunk = QString::fromUtf8(m_process->readAllStandardOutput());
        if (m_output.size() < 300000) {
            m_output += chunk;
            if (m_output.size() > 300000)
                m_output = m_output.left(300000) + QStringLiteral("\n[tronqué]\n");
            emit outputChanged();
        }
    });
    connect(m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        if (!m_process)
            return;
        m_output += QString::fromUtf8(m_process->readAllStandardOutput());
        m_diagnostics += QString::fromUtf8(m_process->readAllStandardError());
        const bool success = !m_cancelled && status == QProcess::NormalExit && code == 0;
        if (m_cancelled) {
            const int run = m_runId;
            cleanup();
            emit cancelled(run);
            return;
        }
        const QString error = success ? QString()
            : (m_diagnostics.trimmed().isEmpty()
               ? tr("Le moteur agent a échoué (code %1).").arg(code)
               : m_diagnostics.trimmed().right(4000));
        const int run = m_runId;
        const QString output = m_output;
        cleanup();
        emit finished(run, success, output, error);
    });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !m_process)
            return;
        const int run = m_runId;
        cleanup();
        emit finished(run, false, {}, tr("Le moteur agent est introuvable ou ne peut pas démarrer."));
    });
    m_timeout.start();
    const QString mode = permission == QLatin1String("workspace-write")
        ? QStringLiteral("workspace-write") : QStringLiteral("read-only");
    m_process->start(binary, {QStringLiteral("--agent-job"),
                              QStringLiteral("--agent-prompt"), prompt,
                              QStringLiteral("--workspace"), workspace,
                              QStringLiteral("--permission"), mode});
}

void BotRunner::cancel() {
    if (!m_process)
        return;
    m_cancelled = true;
    setStatus(tr("Annulation…"));
    m_process->terminate();
    QTimer::singleShot(1500, this, [this] {
        if (m_process)
            m_process->kill();
    });
}

void BotRunner::cleanup() {
    m_timeout.stop();
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_runId = -1;
    m_diagnostics.clear();
    m_status.clear();
    emit runningChanged();
    emit statusChanged();
    emit outputChanged();
}

void BotRunner::setStatus(const QString &status) {
    if (status == m_status)
        return;
    m_status = status;
    emit statusChanged();
    emit runningChanged();
}
