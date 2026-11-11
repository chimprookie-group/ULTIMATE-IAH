#include "mcpmanager.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

namespace {

constexpr int kStatusTimeoutMs = 20000;

QString strippedAnsi(const QString &text) {
    static const QRegularExpression ansi(QStringLiteral("\\x1b\\[[0-9;?]*[A-Za-z]"));
    QString clean = text;
    clean.remove(ansi);
    clean.remove('\r');
    return clean;
}

bool looksLikeAuthUrl(const QString &url) {
    const QString lower = url.toLower();
    return lower.contains("authorize") || lower.contains("oauth") || lower.contains("consent")
        || lower.contains("/login") || lower.contains("auth?");
}

QString lastMeaningfulLine(const QString &output) {
    const QStringList lines = strippedAnsi(output).split('\n', Qt::SkipEmptyParts);
    for (int i = lines.size() - 1; i >= 0; --i) {
        const QString line = lines.at(i).trimmed();
        if (!line.isEmpty())
            return line.left(220);
    }
    return {};
}

QString commandLine(const QJsonArray &command) {
    QStringList parts;
    for (const auto &arg : command) {
        const QString value = arg.toString();
        parts.append(value.contains(' ') ? "\"" + value + "\"" : value);
    }
    return parts.join(' ');
}

} // namespace

McpManager::McpManager(const QString &configPath, bool watchStatus, QObject *parent)
    : QObject(parent),
      m_configPath(configPath.isEmpty() ? defaultConfigPath() : configPath),
      m_watchStatus(watchStatus) {
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::finished, this, [this] { finishRefresh(); });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_timeout.stop();
            setLoading(false);
            setError(tr("OpenCode was not found."));
        }
    });

    m_timeout.setSingleShot(true);
    m_timeout.setInterval(kStatusTimeoutMs);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        m_timedOut = true;
        if (m_process->state() != QProcess::NotRunning)
            m_process->kill();
        setLoading(false);
        setError(tr("MCP status timed out."));
    });

    m_authProcess = new QProcess(this);
    m_authProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_authProcess, &QProcess::readyRead, this, &McpManager::readAuthOutput);
    connect(m_authProcess, &QProcess::finished, this,
            [this](int code, QProcess::ExitStatus status) { finishAuth(code, status); });
    connect(m_authProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && m_authRunning) {
            m_authTimeout.stop();
            m_authRunning = false;
            m_authOk = false;
            m_authMessage = tr("OpenCode was not found.");
            m_authDetail.clear();
            emit authChanged();
        }
    });

    m_authTimeout.setSingleShot(true);
    m_authTimeout.setInterval(180000);
    connect(&m_authTimeout, &QTimer::timeout, this, [this] {
        if (!m_authRunning)
            return;
        m_authTimedOut = true;
        if (m_authProcess->state() != QProcess::NotRunning)
            m_authProcess->kill();
        else
            finishAuth(-1, QProcess::CrashExit);
    });

    loadConfig();
}

QString McpManager::extractAuthUrl(const QString &output) {
    const QString clean = strippedAnsi(output);
    static const QRegularExpression pattern(QStringLiteral("https?://[^\\s<>\"']+"));
    QString firstExternal;
    auto matches = pattern.globalMatch(clean);
    while (matches.hasNext()) {
        QString url = matches.next().captured(0);
        while (!url.isEmpty() && QStringLiteral(").,;:!?]").contains(url.back()))
            url.chop(1);
        url.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
        const QUrl parsed(url);
        if (!parsed.isValid() || parsed.host().isEmpty())
            continue;
        const QString host = parsed.host().toLower();
        if (host == QStringLiteral("localhost") || host == QStringLiteral("127.0.0.1")
            || host == QStringLiteral("::1"))
            continue;
        if (looksLikeAuthUrl(url))
            return url;
        if (firstExternal.isEmpty())
            firstExternal = url;
    }
    return firstExternal;
}

QString McpManager::defaultConfigPath() {
    const QString explicitPath = qEnvironmentVariable("OPENCODE_CONFIG");
    if (!explicitPath.isEmpty())
        return explicitPath;
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + "/opencode/opencode.json";
}

bool McpManager::readConfig(const QString &path, QJsonObject &root) const {
    QFile file(path);
    if (!file.exists()) {
        root = QJsonObject{};
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        root = {};
        return false;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        root = {};
        return false;
    }
    root = document.object();
    return true;
}

bool McpManager::writeConfig(const QString &path, const QJsonObject &root) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        return false;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return file.commit();
}

void McpManager::loadConfig() {
    QJsonObject global;
    m_configOk = readConfig(m_configPath, global);
    if (!m_configOk) {
        m_servers.clear();
        emit serversChanged();
        setError(tr("The OpenCode configuration could not be read."));
        return;
    }

    QVariantList list;
    const auto append = [&list](const QJsonObject &root, const QString &scope) {
        const QJsonObject servers = root.value("mcp").toObject().value("servers").toObject();
        for (auto it = servers.begin(); it != servers.end(); ++it) {
            const QJsonObject server = it.value().toObject();
            const QString type = server.value("type").toString(
                server.contains("url") ? QStringLiteral("remote") : QStringLiteral("local"));
            QVariantMap entry;
            entry["name"] = it.key();
            entry["scope"] = scope;
            entry["type"] = type;
            entry["target"] = type == "remote"
                ? server.value("url").toString()
                : commandLine(server.value("command").toArray());
            entry["status"] = "unknown";
            entry["detail"] = "";
            list.append(entry);
        }
    };

    append(global, QStringLiteral("global"));
    if (!m_projectConfigPath.isEmpty() && QFileInfo::exists(m_projectConfigPath)) {
        QJsonObject project;
        if (readConfig(m_projectConfigPath, project)) {
            append(project, QStringLiteral("project"));
        } else {
            setError(tr("The project OpenCode configuration could not be read."));
        }
    }

    m_servers = list;
    emit serversChanged();
}

void McpManager::setWorkspace(const QString &workspace) {
    const QString clean = workspace.isEmpty() ? QString() : QDir::cleanPath(workspace);
    if (clean == m_workspace)
        return;
    m_workspace = clean;
    m_projectConfigPath = clean.isEmpty() ? QString() : QDir(clean).filePath(QStringLiteral("opencode.json"));
    emit workspaceChanged();
    emit projectConfigPathChanged();
    refresh();
}

void McpManager::refresh() {
    setError({});
    loadConfig();
    if (!m_configOk)
        return;
    if (!m_watchStatus) {
        setLoading(false);
        return;
    }
    if (m_servers.isEmpty()) {
        setLoading(false);
        return;
    }

    const QString opencode = QStandardPaths::findExecutable(QStringLiteral("opencode"));
    if (opencode.isEmpty()) {
        setError(tr("OpenCode was not found."));
        return;
    }

    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(500);
    }

    if (!m_workspace.isEmpty() && QFileInfo(m_workspace).isDir())
        m_process->setWorkingDirectory(m_workspace);
    else
        m_process->setWorkingDirectory(QDir::currentPath());

    m_timedOut = false;
    setLoading(true);

    m_process->start(opencode, {QStringLiteral("mcp"), QStringLiteral("list")});
    m_timeout.start();
}

void McpManager::finishRefresh() {
    m_timeout.stop();
    const QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    setLoading(false);

    if (m_timedOut) {
        m_timedOut = false;
        return;
    }
    if (m_process->exitStatus() != QProcess::NormalExit || m_process->exitCode() != 0) {
        if (!output.trimmed().isEmpty())
            setError(output.trimmed().split('\n').value(0));
        return;
    }
    parseStatuses(output);
}

void McpManager::parseStatuses(const QString &output) {
    static const QRegularExpression linePattern(
        QStringLiteral("^([\\u2713\\u2714\\u2717\\u2718\\u25CB\\u25CC-])\\s+(\\S+)\\s*(.*)$"));

    QHash<QString, QPair<QString, QString>> statuses;
    const QStringList lines = strippedAnsi(output).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const auto match = linePattern.match(line.trimmed());
        if (!match.hasMatch())
            continue;
        const QString glyph = match.captured(1);
        const QString name = match.captured(2);
        QString detail = match.captured(3).trimmed();
        if (detail.startsWith(QStringLiteral("failed:")))
            detail = detail.mid(7).trimmed();
        const QString status = glyph == QString::fromUtf8("✓") || glyph == QString::fromUtf8("✔")
            ? QStringLiteral("connected")
            : (glyph == QString::fromUtf8("✗") || glyph == QString::fromUtf8("✘")
                   ? QStringLiteral("failed")
                   : (glyph == QString::fromUtf8("○") || glyph == QString::fromUtf8("◌")
                          ? QStringLiteral("pending")
                          : QStringLiteral("disabled")));
        if (status != "failed")
            detail.clear();
        statuses.insert(name, {status, detail});
    }

    if (statuses.isEmpty()) {
        emit serversChanged();
        return;
    }

    for (int i = 0; i < m_servers.size(); ++i) {
        QVariantMap server = m_servers.at(i).toMap();
        const auto found = statuses.constFind(server.value("name").toString());
        if (found != statuses.constEnd()) {
            server["status"] = found->first;
            server["detail"] = found->second;
            m_servers[i] = server;
        }
    }
    emit serversChanged();
}

void McpManager::addServer(const QString &name, const QString &type, const QString &target,
                           const QString &scope) {
    const QString trimmedName = name.trimmed();
    const QString trimmedTarget = target.trimmed();
    static const QRegularExpression validName(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9_-]*$"));
    if (!validName.match(trimmedName).hasMatch()) {
        setError(tr("Invalid MCP server name."));
        return;
    }
    if (type != "local" && type != "remote") {
        setError(tr("Choose a local or remote MCP server."));
        return;
    }
    if (scope != "global" && scope != "project") {
        setError(tr("Choose a global or project MCP server."));
        return;
    }
    if (trimmedTarget.isEmpty()) {
        setError(type == "local" ? tr("A local MCP server needs a command.")
                                 : tr("A remote MCP server needs an http(s) URL."));
        return;
    }

    QString path = m_configPath;
    if (scope == "project") {
        if (m_projectConfigPath.isEmpty()) {
            setError(tr("A project MCP server needs a workspace."));
            return;
        }
        path = m_projectConfigPath;
    }

    QJsonObject root;
    if (!readConfig(path, root)) {
        setError(tr("The OpenCode configuration could not be read."));
        return;
    }

    QJsonObject mcp = root.value("mcp").toObject();
    QJsonObject servers = mcp.value("servers").toObject();
    if (servers.contains(trimmedName)) {
        setError(tr("This MCP server already exists."));
        return;
    }

    QJsonObject server;
    server["type"] = type;
    if (type == "remote") {
        const QUrl url(trimmedTarget);
        if (!url.isValid() || (url.scheme() != "http" && url.scheme() != "https")) {
            setError(tr("A remote MCP server needs an http(s) URL."));
            return;
        }
        server["url"] = trimmedTarget;
    } else {
        const QStringList command = QProcess::splitCommand(trimmedTarget);
        if (command.isEmpty()) {
            setError(tr("A local MCP server needs a command."));
            return;
        }
        QJsonArray array;
        for (const QString &part : command)
            array.append(part);
        server["command"] = array;
    }

    servers.insert(trimmedName, server);
    mcp["servers"] = servers;
    root["mcp"] = mcp;
    if (!writeConfig(path, root)) {
        setError(tr("The OpenCode configuration could not be written."));
        return;
    }
    setError({});
    refresh();
}

void McpManager::removeServer(const QString &name, const QString &scope) {
    QString path = m_configPath;
    if (scope == "project")
        path = m_projectConfigPath;
    if (path.isEmpty()) {
        setError(tr("A project MCP server needs a workspace."));
        return;
    }

    QJsonObject root;
    if (!readConfig(path, root)) {
        setError(tr("The OpenCode configuration could not be read."));
        return;
    }

    QJsonObject mcp = root.value("mcp").toObject();
    QJsonObject servers = mcp.value("servers").toObject();
    if (!servers.contains(name))
        return;
    servers.remove(name);
    mcp["servers"] = servers;
    root["mcp"] = mcp;
    if (!writeConfig(path, root)) {
        setError(tr("The OpenCode configuration could not be written."));
        return;
    }
    setError({});
    refresh();
}

bool McpManager::findServer(const QString &name, const QString &scope, QJsonObject &server,
                            QString &path) const {
    if (scope != QStringLiteral("global") && scope != QStringLiteral("project"))
        return false;
    path = scope == QStringLiteral("project") ? m_projectConfigPath : m_configPath;
    if (path.isEmpty())
        return false;
    QJsonObject root;
    if (!readConfig(path, root))
        return false;
    const QJsonObject servers = root.value("mcp").toObject().value("servers").toObject();
    if (!servers.contains(name))
        return false;
    server = servers.value(name).toObject();
    return true;
}

void McpManager::authenticate(const QString &name, const QString &scope) {
    if (m_authRunning)
        return;

    m_authAction = QStringLiteral("auth");
    m_authName = name;
    m_authUrl.clear();
    m_authOutput.clear();
    m_authDetail.clear();
    m_authMessage.clear();
    m_authOk = false;
    m_authUrlOpened = false;
    m_authCancelled = false;
    m_authTimedOut = false;

    const auto fail = [this](const QString &message) {
        m_authRunning = false;
        m_authOk = false;
        m_authMessage = message;
        m_authDetail.clear();
        emit authChanged();
    };

    QJsonObject server;
    QString path;
    if (!findServer(name, scope, server, path)) {
        fail(tr("MCP server not found."));
        return;
    }
    const QString type = server.value("type").toString(
        server.contains("url") ? QStringLiteral("remote") : QStringLiteral("local"));
    if (type != QStringLiteral("remote")) {
        fail(tr("This MCP server is not remote."));
        return;
    }

    const QString opencode = QStandardPaths::findExecutable(QStringLiteral("opencode"));
    if (opencode.isEmpty()) {
        fail(tr("OpenCode was not found."));
        return;
    }

    m_authName = name;
    m_authUrl.clear();
    m_authOutput.clear();
    m_authDetail.clear();
    m_authMessage.clear();
    m_authOk = false;
    m_authRunning = true;
    m_authUrlOpened = false;
    m_authCancelled = false;
    m_authTimedOut = false;
    emit authChanged();

    const QString directory = (scope == QStringLiteral("project") && QFileInfo(m_workspace).isDir())
        ? m_workspace
        : QDir::tempPath();
    m_authProcess->setWorkingDirectory(directory);
    m_authProcess->start(opencode, {QStringLiteral("mcp"), QStringLiteral("auth"), name,
                                    QStringLiteral("--standalone")});
    m_authTimeout.start();
}

void McpManager::logoutServer(const QString &name, const QString &scope) {
    if (m_authRunning)
        return;

    m_authAction = QStringLiteral("logout");
    m_authName = name;
    m_authUrl.clear();
    m_authOutput.clear();
    m_authDetail.clear();
    m_authMessage.clear();
    m_authOk = false;
    m_authUrlOpened = false;
    m_authCancelled = false;
    m_authTimedOut = false;

    const auto fail = [this](const QString &message) {
        m_authRunning = false;
        m_authOk = false;
        m_authMessage = message;
        m_authDetail.clear();
        emit authChanged();
    };

    QJsonObject server;
    QString path;
    if (!findServer(name, scope, server, path)) {
        fail(tr("MCP server not found."));
        return;
    }
    const QString type = server.value("type").toString(
        server.contains("url") ? QStringLiteral("remote") : QStringLiteral("local"));
    if (type != QStringLiteral("remote")) {
        fail(tr("This MCP server is not remote."));
        return;
    }

    const QString opencode = QStandardPaths::findExecutable(QStringLiteral("opencode"));
    if (opencode.isEmpty()) {
        fail(tr("OpenCode was not found."));
        return;
    }

    m_authRunning = true;
    emit authChanged();

    const QString directory = (scope == QStringLiteral("project") && QFileInfo(m_workspace).isDir())
        ? m_workspace
        : QDir::tempPath();
    m_authProcess->setWorkingDirectory(directory);
    m_authProcess->start(opencode, {QStringLiteral("mcp"), QStringLiteral("logout"), name,
                                    QStringLiteral("--standalone")});
    m_authTimeout.start();
}

void McpManager::readAuthOutput() {
    if (!m_authProcess)
        return;
    m_authOutput += QString::fromUtf8(m_authProcess->readAll());
    if (m_authOutput.size() > 8000)
        m_authOutput = m_authOutput.right(8000);

    if (m_authAction != QStringLiteral("auth")) {
        emit authChanged();
        return;
    }

    const QString url = extractAuthUrl(m_authOutput);
    if (!url.isEmpty() && url != m_authUrl) {
        m_authUrl = url;
        if (!m_authUrlOpened && looksLikeAuthUrl(url)) {
            m_authUrlOpened = true;
            QDesktopServices::openUrl(QUrl(url));
        }
    }
    emit authChanged();
}

void McpManager::finishAuth(int exitCode, QProcess::ExitStatus status) {
    if (!m_authRunning && m_authMessage.isEmpty())
        return;
    m_authTimeout.stop();
    if (m_authProcess && m_authProcess->bytesAvailable() > 0) {
        m_authOutput += QString::fromUtf8(m_authProcess->readAll());
    }
    if (m_authOutput.size() > 8000)
        m_authOutput = m_authOutput.right(8000);

    m_authRunning = false;
    const QString clean = strippedAnsi(m_authOutput);
    const QString lower = clean.toLower();
    const bool failed = status != QProcess::NormalExit || exitCode != 0
        || lower.contains(QStringLiteral("failed"))
        || lower.contains(QStringLiteral("not found"))
        || lower.contains(QStringLiteral("error"));

    const bool logout = m_authAction == QStringLiteral("logout");
    if (m_authCancelled) {
        m_authOk = false;
        m_authMessage = logout ? tr("Logout cancelled.") : tr("Authentication cancelled.");
        m_authDetail.clear();
    } else if (m_authTimedOut) {
        m_authOk = false;
        m_authMessage = logout ? tr("Logout timed out.") : tr("Authentication timed out.");
        m_authDetail.clear();
    } else if (failed) {
        m_authOk = false;
        m_authMessage = logout ? tr("Logout failed.") : tr("Authentication failed.");
        m_authDetail = lastMeaningfulLine(clean);
    } else {
        m_authOk = true;
        m_authMessage = logout ? tr("Logout completed.") : tr("Authentication completed.");
        m_authDetail.clear();
    }

    m_authCancelled = false;
    m_authTimedOut = false;
    refresh();
    emit authChanged();
}

void McpManager::cancelAuth() {
    if (!m_authRunning)
        return;
    m_authCancelled = true;
    m_authTimeout.stop();
    if (m_authProcess && m_authProcess->state() != QProcess::NotRunning)
        m_authProcess->kill();
    else
        finishAuth(-1, QProcess::CrashExit);
}

void McpManager::clearAuth() {
    if (m_authRunning)
        return;
    resetAuth();
    emit authChanged();
}

void McpManager::resetAuth() {
    m_authAction = QStringLiteral("auth");
    m_authName.clear();
    m_authUrl.clear();
    m_authOutput.clear();
    m_authDetail.clear();
    m_authMessage.clear();
    m_authOk = false;
    m_authUrlOpened = false;
}

void McpManager::clearError() {
    setError({});
}

void McpManager::setLoading(bool loading) {
    if (loading == m_loading)
        return;
    m_loading = loading;
    emit loadingChanged();
}

void McpManager::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}
