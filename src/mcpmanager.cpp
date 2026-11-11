#include "mcpmanager.h"

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

namespace {

constexpr int kStatusTimeoutMs = 20000;

QString strippedAnsi(const QString &text) {
    static const QRegularExpression ansi(QStringLiteral("\\x1b\\[[0-9;]*m"));
    QString clean = text;
    clean.remove(ansi);
    return clean;
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

    loadConfig();
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
