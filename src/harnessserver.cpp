#include "harnessserver.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

namespace {
constexpr int kStartTimeoutMs = 60000;
constexpr int kLogLines = 120;
}

HarnessServer::HarnessServer(QObject *parent)
    : QObject(parent) {
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    m_process.setProgram(m_binary);

    m_timeout.setSingleShot(true);
    m_timeout.setInterval(kStartTimeoutMs);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        if (m_state == Starting)
            fail(tr("DeepSeek Harness did not report a web address within %1 seconds.").arg(kStartTimeoutMs / 1000));
    });

    m_killTimer.setSingleShot(true);
    m_killTimer.setInterval(3000);
    connect(&m_killTimer, &QTimer::timeout, this, [this] {
        if (m_process.state() != QProcess::NotRunning)
            m_process.kill();
    });

    connect(&m_process, &QProcess::started, this, [this] {
        setState(Starting, tr("Starting DeepSeek Harness…"));
        m_timeout.start();
    });
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        consume(m_process.readAllStandardOutput());
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && !m_stopping)
            fail(tr("Could not run %1. Install DeepSeek Harness or set its path in Settings.").arg(m_binary));
    });
    connect(&m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_timeout.stop();
        m_killTimer.stop();
        if (m_stopping || m_state == Stopped)
            return;
        const QString reason = status == QProcess::CrashExit
            ? tr("DeepSeek Harness crashed.")
            : tr("DeepSeek Harness exited with code %1.").arg(code);
        fail(reason);
    });
}

HarnessServer::~HarnessServer() {
    m_stopping = true;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(1500))
            m_process.kill();
    }
}

void HarnessServer::setBinary(const QString &binary) {
    if (binary.isEmpty() || binary == m_binary)
        return;
    m_binary = binary;
    m_process.setProgram(binary);
}

void HarnessServer::setProfile(const QString &profile) {
    const QString trimmed = profile.trimmed();
    if (trimmed.isEmpty() || trimmed == m_profile)
        return;
    m_profile = trimmed;
    emit profileChanged();
    if (m_state == Running || m_state == Starting)
        restart();
}

void HarnessServer::setWorkingDirectory(const QString &directory) {
    m_directory = directory;
}

QString HarnessServer::extractUrl(const QString &line) {
    static const QRegularExpression ansi(R"(\x1b\[[0-9;]*m)");
    static const QRegularExpression url(R"((https?://[^\s"'<>]+))");
    QString withoutAnsi = line;
    withoutAnsi.remove(ansi);
    const auto match = url.match(withoutAnsi);
    if (!match.hasMatch())
        return {};
    return match.captured(1);
}

void HarnessServer::consume(const QByteArray &chunk) {
    const QString text = QString::fromUtf8(chunk);
    for (const QString &line : text.split('\n', Qt::SkipEmptyParts)) {
        m_log.append(line.right(400));
        while (m_log.size() > kLogLines)
            m_log.removeFirst();
        if (m_state != Running) {
            const QString url = extractUrl(line);
            if (!url.isEmpty()) {
                m_timeout.stop();
                m_url = url;
                emit urlChanged();
                setState(Running, tr("Serving on %1").arg(QUrl(url).authority()));
            }
        }
    }
    emit logChanged();
}

void HarnessServer::setState(State state, const QString &detail) {
    const bool stateChangedNow = state != m_state;
    const bool detailChangedNow = detail != m_detail;
    m_state = state;
    m_detail = detail;
    if (stateChangedNow)
        emit stateChanged();
    if (detailChangedNow)
        emit detailChanged();
}

void HarnessServer::fail(const QString &detail) {
    m_timeout.stop();
    m_url.clear();
    emit urlChanged();
    setState(Failed, detail);
    if (m_process.state() != QProcess::NotRunning) {
        m_stopping = true;
        m_process.terminate();
        m_killTimer.start();
    }
}

void HarnessServer::start() {
    if (m_state == Starting || m_state == Running)
        return;
    m_url.clear();
    m_log.clear();
    m_stopping = false;
    emit urlChanged();
    emit logChanged();

    m_process.setProgram(m_binary);
    if (!m_directory.isEmpty())
        m_process.setWorkingDirectory(m_directory);

    QStringList arguments;
    // `--patch` is a launcher option. DSH only accepts it with an explicit
    // --profile, before the app arguments; `dsh web --patch p` rejects it.
    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QStringList patches;
    for (const QString &name : {QStringLiteral("nine-router.patch.yml"),
                                QStringLiteral("openmausbot.patch.yml")}) {
        const QString path = configDir + "/" + name;
        if (QFileInfo::exists(path))
            patches << path;
    }
    if (!patches.isEmpty()) {
        const QString profile = (m_profile.isEmpty() ? QStringLiteral("web") : m_profile);
        arguments << "--profile" << profile;
        for (const QString &patch : patches)
            arguments << "--patch" << patch;
    } else if (m_profile != "web" && !m_profile.isEmpty()) {
        arguments << "--profile" << m_profile;
    } else {
        arguments << "web";
    }
    arguments << "--no-open" << "--port" << "0";

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    // The desktop session exports the address of its own harness; a child
    // harness must not inherit it or it may advertise the wrong URL.
    environment.remove("DSH_WEB_URL");

    // 9Router integration: an app-owned overlay adds the nine-router provider,
    // and the key rides in the child environment instead of DSH settings.
    QFile envFile(configDir + "/nine-router.env");
    if (envFile.open(QIODevice::ReadOnly)) {
        const QString key = QString::fromUtf8(envFile.readAll()).trimmed();
        if (key.startsWith("NINE_ROUTER_API_KEY="))
            environment.insert("NINE_ROUTER_API_KEY", key.mid(QStringLiteral("NINE_ROUTER_API_KEY=").size()));
    }
    m_process.setProcessEnvironment(environment);

    setState(Starting, tr("Starting DeepSeek Harness…"));
    m_process.start(m_binary, arguments);
}

void HarnessServer::stop() {
    m_stopping = true;
    m_timeout.stop();
    m_killTimer.stop();
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(2000))
            m_process.kill();
    }
    m_url.clear();
    emit urlChanged();
    setState(Stopped, tr("Stopped"));
}

void HarnessServer::restart() {
    stop();
    m_stopping = false;
    start();
}
