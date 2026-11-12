#include "mausbotmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

namespace {
constexpr int kPort = 8799;
constexpr int kStartTimeoutMs = 60000;

QString yamlString(QString value) {
    value.replace('\\', "\\\\");
    value.replace('"', "\\\"");
    return '"' + value + '"';
}
} // namespace

MausBotManager::MausBotManager(QObject *parent)
    : QObject(parent) {
    m_network = new QNetworkAccessManager(this);
    m_configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(m_configDir);

    m_healthTimer = new QTimer(this);
    m_healthTimer->setInterval(1000);
    connect(m_healthTimer, &QTimer::timeout, this, &MausBotManager::probeHealth);

    m_startTimer = new QTimer(this);
    m_startTimer->setSingleShot(true);
    m_startTimer->setInterval(kStartTimeoutMs);
    connect(m_startTimer, &QTimer::timeout, this, [this] {
        if (!m_starting)
            return;
        setStarting(false);
        setRunning(false);
        setError(tr("OpenMausBot did not become ready within %1 seconds.").arg(kStartTimeoutMs / 1000));
        setDetail(tr("Start timed out"));
    });

    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        const QString text = QString::fromUtf8(m_process.readAllStandardOutput());
        for (const QString &line : text.split('\n', Qt::SkipEmptyParts))
            appendLog(line);
    });
    connect(&m_process, &QProcess::started, this, [this] {
        setStarting(true);
        setDetail(tr("Starting OpenMausBot…"));
        m_healthTimer->start();
        m_startTimer->start();
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && !m_stopping) {
            setStarting(false);
            setRunning(false);
            setError(tr("Could not start OpenMausBot."));
        }
    });
    connect(&m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_healthTimer->stop();
        m_startTimer->stop();
        if (m_stopping)
            return;
        setRunning(false);
        setStarting(false);
        setDetail(status == QProcess::CrashExit
                      ? tr("OpenMausBot crashed")
                      : tr("OpenMausBot exited with code %1").arg(code));
        if (status == QProcess::CrashExit)
            setError(tr("OpenMausBot crashed. Check the log."));
    });

    m_installProcess.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_installProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_installLog += QString::fromUtf8(m_installProcess.readAllStandardOutput());
        if (m_installLog.size() > 14000)
            m_installLog = m_installLog.right(14000);
        emit installLogChanged();
    });
    connect(&m_installProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        m_installing = false;
        emit installingChanged();
        detect();
        if (code == 0 && installed()) {
            verifyInstalled();
        } else {
            setError(tr("The OpenMausBot installation failed."));
            setDetail(tr("Not installed"));
        }
    });

    m_privilegedProcess.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_privilegedProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_privilegedLog += QString::fromUtf8(m_privilegedProcess.readAllStandardOutput());
        if (m_privilegedLog.size() > 12000)
            m_privilegedLog = m_privilegedLog.right(12000);
        emit privilegedLogChanged();
    });
    connect(&m_privilegedProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_privilegedBusy = false;
            emit privilegedBusyChanged();
            setError(tr("pkexec could not be started. Is a polkit authentication agent running?"));
        }
    });
    connect(&m_privilegedProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        m_privilegedBusy = false;
        emit privilegedBusyChanged();
        if (code != 0) {
            setError(tr("Dependency installation failed or was cancelled."));
            return;
        }
        QString dependencyError;
        if (!verifyDependencies(&dependencyError)) {
            setError(dependencyError);
            return;
        }
        m_installReport = tr("System dependencies verified: node, npm");
        emit installReportChanged();
        detect();
        install();
    });

    m_pairProcess.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_pairProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        m_pairing = false;
        emit pairingChanged();
        if (code != 0) {
            setError(tr("OpenMausBot pairing failed. Check the log."));
            return;
        }
        const QString output = QString::fromUtf8(m_pairProcess.readAll());
        static const QRegularExpression pattern(QStringLiteral("(https?://[^\\s]*/pair[^\\s]*)"));
        const auto match = pattern.match(output);
        if (match.hasMatch()) {
            m_pairUrl = match.captured(1);
            emit pairUrlChanged();
        } else {
            setError(tr("OpenMausBot did not return a pairing link."));
        }
    });

    detect();
    refresh();
    if (installed() && QFileInfo::exists(m_configDir + "/openmausbot.enabled"))
        QTimer::singleShot(900, this, &MausBotManager::start);
}

MausBotManager::~MausBotManager() {
    m_stopping = true;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(4000))
            m_process.kill();
    }
}

void MausBotManager::detect() {
    QString binary = QStandardPaths::findExecutable(QStringLiteral("openmausbot"));
    if (binary.isEmpty()) {
        const QString local = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + "/openmausbot-app/node_modules/.bin/openmausbot";
        if (QFileInfo(local).isExecutable())
            binary = local;
    }
    if (binary == m_binary)
        return;
    m_binary = binary;
    emit installedChanged();
    setInstalledState();
}

void MausBotManager::setInstalledState() {
    if (m_binary.isEmpty())
        setDetail(tr("Not installed"));
    else if (!m_running && !m_starting)
        setDetail(tr("Installed"));
}

void MausBotManager::refresh() {
    detect();
    const bool harness = QFileInfo::exists(harnessPatchPath());
    if (harness != m_mcpHarness) {
        m_mcpHarness = harness;
        emit mcpChanged();
    }
}

void MausBotManager::install() {
    if (m_installing)
        return;
    const QString npm = QStandardPaths::findExecutable(QStringLiteral("npm"));
    if (npm.isEmpty()) {
        setError(tr("npm was not found. Use System dependencies (sudo), or install Node.js first."));
        return;
    }
    if (!m_binary.isEmpty()) {
        setError({});
        setDetail(tr("Already installed"));
        return;
    }
    m_installing = true;
    m_installLog.clear();
    emit installingChanged();
    emit installLogChanged();
    setError({});
    setDetail(tr("Installing OpenMausBot…"));

    const QString prefix = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/openmausbot-app";
    QDir().mkpath(prefix);
    m_installProcess.setWorkingDirectory(prefix);
    m_installProcess.start(npm, {QStringLiteral("install"), QStringLiteral("--prefix"), prefix,
                                 QStringLiteral("openmausbot"), QStringLiteral("--no-audit"),
                                 QStringLiteral("--no-fund")});
}

void MausBotManager::installDependencies() {
    if (m_privilegedBusy)
        return;
    const QString pkexec = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
    const QString pacman = QStandardPaths::findExecutable(QStringLiteral("pacman"));
    if (pkexec.isEmpty() || pacman.isEmpty()) {
        setError(tr("pkexec and pacman are required for the system dependency helper."));
        return;
    }
    m_privilegedBusy = true;
    m_privilegedLog.clear();
    emit privilegedBusyChanged();
    emit privilegedLogChanged();
    setError({});
    setDetail(tr("Waiting for polkit authorization…"));
    m_privilegedProcess.start(pkexec, {pacman, QStringLiteral("-S"), QStringLiteral("--needed"),
                                       QStringLiteral("--noconfirm"),
                                       QStringLiteral("nodejs"), QStringLiteral("npm")});
}

void MausBotManager::verifyInstalled() {
    cleanupInstallArtifacts();
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(m_binary, {QStringLiteral("--version")});
    if (!process.waitForStarted(2500) || !process.waitForFinished(8000)
        || process.exitCode() != 0) {
        process.kill();
        process.waitForFinished(100);
        setError(tr("OpenMausBot was installed but its version check failed."));
        setDetail(tr("Installation could not be verified"));
        return;
    }
    const QString version = QString::fromUtf8(process.readAll()).trimmed();
    if (version.isEmpty()) {
        setError(tr("OpenMausBot was installed but its version check failed."));
        return;
    }
    m_installReport = tr("Verified: OpenMausBot %1 · temporary files cleaned").arg(version);
    emit installReportChanged();
    setDetail(tr("Installed and verified"));
    start();
}

void MausBotManager::cleanupInstallArtifacts() {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/openmausbot-app";
    QDir(root + "/node_modules/.cache").removeRecursively();
    QDir(root + "/.cache").removeRecursively();
    QDir(root + "/.npm").removeRecursively();
    QFile::remove(root + "/npm-debug.log");
}

bool MausBotManager::verifyDependencies(QString *error) {
    const QString node = QStandardPaths::findExecutable(QStringLiteral("node"));
    const QString npm = QStandardPaths::findExecutable(QStringLiteral("npm"));
    if (node.isEmpty() || npm.isEmpty()) {
        if (error)
            *error = tr("node or npm is still missing after installation.");
        return false;
    }
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(node, {QStringLiteral("--version")});
    if (!process.waitForStarted(2000) || !process.waitForFinished(4000)
        || process.exitCode() != 0) {
        process.kill();
        if (error)
            *error = tr("node does not run after installation.");
        return false;
    }
    return true;
}

void MausBotManager::start() {
    detect();
    if (!installed()) {
        setError(tr("OpenMausBot is not installed."));
        return;
    }
    if (m_running || m_starting)
        return;
    setError({});
    setStarting(true);
    setDetail(tr("Looking for OpenMausBot…"));
    m_probeBeforeStart = true;
    probeHealth();
}

void MausBotManager::launchProcess() {
    m_stopping = false;
    m_log.clear();
    emit logChanged();
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("OMB_PORT", QString::number(m_port));
    m_process.setProcessEnvironment(environment);
    m_process.setProgram(m_binary);
    m_process.setWorkingDirectory(QFileInfo(m_binary).absolutePath());
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/openmausbot-data";
    QDir().mkpath(dataDir);
    m_process.start(m_binary, {QStringLiteral("serve"), QStringLiteral("--port"), QString::number(m_port),
                               QStringLiteral("--data-dir"), dataDir,
                               QStringLiteral("--label"), QStringLiteral("Ultimate HAI")});
}

void MausBotManager::probeHealth() {
    QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:%1/api/health").arg(m_port)));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("UltimateHAI/1.0"));
    QNetworkReply *reply = m_network->get(request);
    QTimer::singleShot(4000, reply, [reply] { reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QJsonObject health = QJsonDocument::fromJson(reply->readAll()).object();
        reply->deleteLater();
        const bool ok = status >= 200 && status < 300
            && (health.value("app").toString() == QStringLiteral("openmausbot")
                || health.value("static").toBool());
        if (!ok) {
            if (m_probeBeforeStart) {
                m_probeBeforeStart = false;
                launchProcess();
            }
            return;
        }
        m_probeBeforeStart = false;
        if (m_running)
            return;
        m_healthTimer->stop();
        m_startTimer->stop();
        setRunning(true);
        setStarting(false);
        setDetail(tr("Running on 127.0.0.1:%1").arg(m_port));
        refresh();
        setDetail(tr("Running on 127.0.0.1:%1").arg(m_port));
    });
}

void MausBotManager::stop() {
    m_stopping = true;
    m_healthTimer->stop();
    m_startTimer->stop();
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(4000))
            m_process.kill();
    }
    setRunning(false);
    setStarting(false);
    m_pairUrl.clear();
    emit pairUrlChanged();
    setDetail(tr("Stopped"));
}

QString MausBotManager::harnessPatchPath() const {
    return m_configDir + "/openmausbot.patch.yml";
}



bool MausBotManager::writeHarnessPatch(QString *error) {
    const QString content =
        QStringLiteral("- insert:\n")
        + QStringLiteral("    - id: mcp-openmausbot\n")
        + QStringLiteral("      name: \"@deepseek-ai/dsh-mcp-client\"\n")
        + QStringLiteral("      config:\n")
        + QStringLiteral("        serverName: openmausbot\n")
        + QStringLiteral("        transport: stdio\n")
        + QStringLiteral("        command: ") + yamlString(m_binary) + QStringLiteral("\n")
        + QStringLiteral("        args: [\"mcp\"]\n")
        + QStringLiteral("        env:\n")
        + QStringLiteral("          OMB_PORT: \"") + QString::number(m_port) + QStringLiteral("\"\n");

    QSaveFile file(harnessPatchPath());
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = tr("The harness MCP patch could not be written.");
        return false;
    }
    file.write(content.toUtf8());
    if (!file.commit()) {
        if (error) *error = tr("The harness MCP patch could not be saved.");
        return false;
    }
    QFile::setPermissions(harnessPatchPath(), QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

void MausBotManager::pair() {
    if (m_pairing || !m_running)
        return;
    m_pairing = true;
    emit pairingChanged();
    setError({});
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("OMB_PORT", QString::number(m_port));
    m_pairProcess.setProcessEnvironment(environment);
    m_pairProcess.start(m_binary, {QStringLiteral("pair"), QStringLiteral("--label"),
                                   QStringLiteral("Ultimate HAI"), QStringLiteral("--ttl"),
                                   QStringLiteral("24h")});
}

void MausBotManager::connectMcp() {
    if (m_binary.isEmpty()) {
        setError(tr("Install OpenMausBot before connecting its MCP server."));
        return;
    }
    QString error;
    if (!writeHarnessPatch(&error)) {
        setError(error);
        return;
    }
    QFile marker(m_configDir + "/openmausbot.enabled");
    if (marker.open(QIODevice::WriteOnly))
        marker.write("enabled\n");
    refresh();
    setError({});
    emit mcpConnected();
}

void MausBotManager::disconnectMcp() {
    QString error;
    QFile::remove(harnessPatchPath());
    QFile::remove(m_configDir + "/openmausbot.enabled");
    refresh();
    setError({});
    emit mcpDisconnected();
}

void MausBotManager::appendLog(const QString &line) {
    m_log.append(line.right(400));
    while (m_log.size() > 200)
        m_log.removeFirst();
    emit logChanged();
}

void MausBotManager::setError(const QString &error) {
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

void MausBotManager::setDetail(const QString &detail) {
    if (m_detail == detail)
        return;
    m_detail = detail;
    emit detailChanged();
}

void MausBotManager::setRunning(bool running) {
    if (m_running == running)
        return;
    m_running = running;
    emit runningChanged();
}

void MausBotManager::setStarting(bool starting) {
    if (m_starting == starting)
        return;
    m_starting = starting;
    emit startingChanged();
}

void MausBotManager::clearError() { setError({}); }
