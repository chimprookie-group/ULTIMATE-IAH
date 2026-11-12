#include "mcpmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
QString yamlQuote(const QString &value) {
    QString escaped = value;
    escaped.replace('\\', QStringLiteral("\\\\"));
    escaped.replace('"', QStringLiteral("\\\""));
    escaped.replace('\n', QStringLiteral("\\n"));
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}

QStringList splitLines(const QString &text) {
    QStringList values;
    for (const QString &line : text.split('\n', Qt::SkipEmptyParts)) {
        const QString clean = line.trimmed();
        if (!clean.isEmpty())
            values << clean;
    }
    return values;
}

QVariantMap parseKeyValues(const QString &text) {
    QVariantMap values;
    for (const QString &line : splitLines(text)) {
        const int equals = line.indexOf('=');
        if (equals <= 0)
            continue;
        values.insert(line.left(equals).trimmed(), line.mid(equals + 1).trimmed());
    }
    return values;
}
} // namespace

McpManager::McpManager(const QString &configPath, QObject *parent)
    : QObject(parent),
      m_configPath(configPath.isEmpty() ? defaultConfigPath() : configPath),
      m_patchPath(QFileInfo(m_configPath).absolutePath() + "/mcp-servers.patch.yml") {
    load();
}

QString McpManager::defaultConfigPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/mcp-servers.json";
}

QString McpManager::defaultPatchPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/mcp-servers.patch.yml";
}

bool McpManager::load() {
    m_servers.clear();
    QFile file(m_configPath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        const QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
        for (const QJsonValue &value : array) {
            const QJsonObject object = value.toObject();
            const QString name = object.value("name").toString();
            if (name.isEmpty())
                continue;
            m_servers.append(QVariantMap{
                {"id", object.value("id").toString(name)},
                {"name", name},
                {"transport", object.value("transport").toString("stdio")},
                {"command", object.value("command").toString()},
                {"args", object.value("args").toArray().toVariantList()},
                {"env", object.value("env").toObject().toVariantMap()},
                {"url", object.value("url").toString()},
                {"headers", object.value("headers").toObject().toVariantMap()},
                {"enabled", object.value("enabled").toBool(true)},
            });
        }
    }
    writePatch();
    rebuildBuiltins();
    emit changed();
    return true;
}

bool McpManager::save() {
    QJsonArray array;
    for (const QVariant &value : m_servers) {
        const QVariantMap server = value.toMap();
        QJsonObject object;
        object["id"] = server.value("id").toString();
        object["name"] = server.value("name").toString();
        object["transport"] = server.value("transport").toString();
        object["command"] = server.value("command").toString();
        object["args"] = QJsonArray::fromVariantList(server.value("args").toList());
        object["env"] = QJsonObject::fromVariantMap(server.value("env").toMap());
        object["url"] = server.value("url").toString();
        object["headers"] = QJsonObject::fromVariantMap(server.value("headers").toMap());
        object["enabled"] = server.value("enabled").toBool();
        array.append(object);
    }
    QDir().mkpath(QFileInfo(m_configPath).absolutePath());
    QSaveFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(tr("Impossible d'écrire la configuration MCP."));
        return false;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        setError(tr("Impossible d'enregistrer la configuration MCP."));
        return false;
    }
    QFile::setPermissions(m_configPath, QFile::ReadOwner | QFile::WriteOwner);
    return writePatch();
}

bool McpManager::writePatch() {
    QString yaml;
    int count = 0;
    for (const QVariant &value : m_servers) {
        const QVariantMap server = value.toMap();
        if (!server.value("enabled").toBool())
            continue;
        const QString id = server.value("id").toString();
        const QString name = server.value("name").toString();
        const QString transport = server.value("transport").toString();
        if (id.isEmpty() || name.isEmpty())
            continue;
        yaml += QStringLiteral("    - id: mcp-%1\n").arg(id);
        yaml += QStringLiteral("      name: \"@deepseek-ai/dsh-mcp-client\"\n");
        yaml += QStringLiteral("      config:\n");
        yaml += QStringLiteral("        serverName: %1\n").arg(id);
        yaml += QStringLiteral("        transport: %1\n").arg(transport);
        if (transport == "streamable-http") {
            yaml += QStringLiteral("        url: %1\n").arg(yamlQuote(server.value("url").toString()));
            const QVariantMap headers = server.value("headers").toMap();
            if (!headers.isEmpty()) {
                yaml += QStringLiteral("        headers:\n");
                for (auto it = headers.cbegin(); it != headers.cend(); ++it)
                    yaml += QStringLiteral("          %1: %2\n").arg(it.key(), yamlQuote(it.value().toString()));
            }
        } else {
            yaml += QStringLiteral("        command: %1\n").arg(yamlQuote(server.value("command").toString()));
            const QVariantList args = server.value("args").toList();
            if (!args.isEmpty()) {
                QStringList quoted;
                for (const QVariant &arg : args)
                    quoted << yamlQuote(arg.toString());
                yaml += QStringLiteral("        args: [%1]\n").arg(quoted.join(QStringLiteral(", ")));
            }
            const QVariantMap env = server.value("env").toMap();
            if (!env.isEmpty()) {
                yaml += QStringLiteral("        env:\n");
                for (auto it = env.cbegin(); it != env.cend(); ++it)
                    yaml += QStringLiteral("          %1: %2\n").arg(it.key(), yamlQuote(it.value().toString()));
            }
        }
        ++count;
    }

    if (count == 0) {
        QFile::remove(m_patchPath);
        return true;
    }
    QDir().mkpath(QFileInfo(m_patchPath).absolutePath());
    QSaveFile file(m_patchPath);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(tr("Impossible d'écrire le patch MCP."));
        return false;
    }
    file.write((QStringLiteral("- insert:\n") + yaml).toUtf8());
    if (!file.commit()) {
        setError(tr("Impossible d'enregistrer le patch MCP."));
        return false;
    }
    QFile::setPermissions(m_patchPath, QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

void McpManager::rebuildBuiltins() {
    QVariantList builtins;
    builtins.append(QVariantMap{
        {"name", "ultimate-hai-memory"},
        {"label", tr("Mémoire Ultimate HAI")},
        {"detail", tr("Outils memory_search, memory_remember, memory_recent")},
        {"transport", "stdio"},
        {"active", QFileInfo::exists(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/mcp-memory.patch.yml")},
    });
    builtins.append(QVariantMap{
        {"name", "openmausbot"},
        {"label", tr("MausBot")},
        {"detail", tr("Équipe OpenMausBot via MCP stdio")},
        {"transport", "stdio"},
        {"active", QFileInfo::exists(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/openmausbot.patch.yml")},
    });
    m_builtins = builtins;
}

QString McpManager::uniqueId(const QString &name) const {
    QString base = name.toLower().trimmed();
    base.replace(QRegularExpression(QStringLiteral("[^a-z0-9_-]+")), QStringLiteral("-"));
    base.remove(QRegularExpression(QStringLiteral("^-+|-+$")));
    if (base.isEmpty())
        base = QStringLiteral("server");
    QString candidate = base;
    int suffix = 2;
    while (true) {
        bool used = false;
        for (const QVariant &value : m_servers)
            if (value.toMap().value("id").toString() == candidate)
                used = true;
        if (!used)
            return candidate;
        candidate = base + "-" + QString::number(suffix++);
    }
}

void McpManager::refresh() {
    load();
}

bool McpManager::addServer(const QString &nameValue, const QString &transportValue,
                           const QString &commandValue, const QString &argsText,
                           const QString &envText, const QString &urlValue,
                           const QString &headersText) {
    const QString name = nameValue.trimmed();
    if (name.isEmpty()) {
        setError(tr("Le nom du serveur MCP est obligatoire."));
        return false;
    }
    const QString transport = transportValue == "streamable-http"
        ? QStringLiteral("streamable-http") : QStringLiteral("stdio");
    const QString id = uniqueId(name);
    if (transport == "stdio" && commandValue.trimmed().isEmpty()) {
        setError(tr("La commande stdio est obligatoire."));
        return false;
    }
    if (transport == "streamable-http") {
        const QUrl url(urlValue.trimmed());
        if (!url.isValid() || (url.scheme() != "http" && url.scheme() != "https")) {
            setError(tr("L'URL MCP doit commencer par http:// ou https://."));
            return false;
        }
    }
    QVariantList args;
    for (const QString &arg : splitLines(argsText))
        args.append(arg);
    QVariantMap server{
        {"id", id},
        {"name", name},
        {"transport", transport},
        {"command", commandValue.trimmed()},
        {"args", args},
        {"env", parseKeyValues(envText)},
        {"url", urlValue.trimmed()},
        {"headers", parseKeyValues(headersText)},
        {"enabled", true},
    };
    m_servers.append(server);
    if (!save())
        return false;
    setError({});
    rebuildBuiltins();
    emit changed();
    emit applied();
    return true;
}

bool McpManager::removeServer(const QString &id) {
    for (int i = 0; i < m_servers.size(); ++i) {
        if (m_servers.at(i).toMap().value("id").toString() == id) {
            m_servers.removeAt(i);
            if (!save())
                return false;
            rebuildBuiltins();
            emit changed();
            emit applied();
            return true;
        }
    }
    return false;
}

bool McpManager::setEnabled(const QString &id, bool enabled) {
    for (int i = 0; i < m_servers.size(); ++i) {
        QVariantMap server = m_servers.at(i).toMap();
        if (server.value("id").toString() != id)
            continue;
        server["enabled"] = enabled;
        m_servers[i] = server;
        if (!save())
            return false;
        emit changed();
        emit applied();
        return true;
    }
    return false;
}

void McpManager::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}

void McpManager::clearError() { setError({}); }
