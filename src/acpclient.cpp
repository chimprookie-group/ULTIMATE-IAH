#include "acpclient.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QUuid>

namespace {

QString contentText(const QJsonObject &content) {
    return content.value("type").toString() == "text" ? content.value("text").toString() : QString();
}

// A tool update carries content blocks; the text of those blocks is what the
// user wants to read under the call. Updates replace rather than append, so a
// repeated cumulative update never duplicates output.
QString toolOutput(const QJsonArray &blocks) {
    QStringList parts;
    for (const auto &value : blocks) {
        const QJsonObject block = value.toObject();
        if (block.value("type").toString() != "content")
            continue;
        const QString text = contentText(block.value("content").toObject());
        if (!text.isEmpty())
            parts.append(text);
    }
    return parts.join('\n');
}

QString shortText(const QString &text, int limit = 240) {
    const QString flat = text.simplified();
    return flat.size() > limit ? flat.left(limit) + "…" : flat;
}

} // namespace

AcpClient::AcpClient(QObject *parent)
    : QObject(parent) {
    m_process.setProcessChannelMode(QProcess::SeparateChannels);

    connect(&m_process, &QProcess::started, this, [this] {
        setState(Starting, tr("Handshaking with OpenCode…"));
        request("initialize", QJsonObject{
            {"protocolVersion", 1},
            {"clientCapabilities", QJsonObject{
                {"fs", QJsonObject{{"readTextFile", false}, {"writeTextFile", false}}},
                {"terminal", false},
            }},
        });
    });
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &AcpClient::consumeStdout);
    connect(&m_process, &QProcess::readyReadStandardError, this, &AcpClient::consumeStderr);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && !m_stopping)
            fail(tr("Could not run %1. Install OpenCode or set its path in Settings.").arg(m_binary));
    });
    connect(&m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        if (m_stopping)
            return;
        Q_UNUSED(status)
        const QString tail = shortText(m_stderr.trimmed(), 300);
        if (m_busy) {
            setBusy(false);
            addNotice(tr("OpenCode stopped before finishing this turn."));
        }
        fail(tr("OpenCode exited with code %1.").arg(code));
        if (!tail.isEmpty())
            addNotice(tail);
    });
}

AcpClient::~AcpClient() {
    m_stopping = true;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(1200))
            m_process.kill();
    }
}

void AcpClient::start(const QString &binary, const QString &workspace) {
    stop();
    m_stopping = false;
    m_binary = binary.trimmed().isEmpty() ? QStringLiteral("opencode") : binary.trimmed();
    m_workspace = workspace;
    m_stderr.clear();
    clearTranscript();
    m_sessions.clear();
    emit sessionsChanged();
    m_usage.clear();
    emit usageChanged();

    setState(Starting, tr("Starting OpenCode…"));
    m_process.setProgram(m_binary);
    m_process.setWorkingDirectory(workspace);
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("NO_COLOR", "1");
    m_process.setProcessEnvironment(environment);
    m_process.start(m_binary, {QStringLiteral("acp")});
}

void AcpClient::stop() {
    m_stopping = true;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(1500))
            m_process.kill();
    }
    m_buffer.clear();
    m_pending.clear();
    setBusy(false);
    m_permission.clear();
    emit permissionChanged();
    setState(Stopped, tr("Stopped"));
}

void AcpClient::setState(State state, const QString &detail) {
    const bool stateChangedNow = state != m_state;
    const bool detailChangedNow = detail != m_detail;
    m_state = state;
    m_detail = detail;
    if (stateChangedNow)
        emit stateChanged();
    if (detailChangedNow)
        emit detailChanged();
}

void AcpClient::fail(const QString &detail) {
    setBusy(false);
    m_permission.clear();
    emit permissionChanged();
    setState(Failed, detail);
}

void AcpClient::consumeStdout() {
    m_buffer.append(m_process.readAllStandardOutput());
    int newline = m_buffer.indexOf('\n');
    while (newline >= 0) {
        const QByteArray line = m_buffer.left(newline).trimmed();
        m_buffer.remove(0, newline + 1);
        if (!line.isEmpty())
            handleLine(line);
        newline = m_buffer.indexOf('\n');
    }
}

void AcpClient::consumeStderr() {
    const QString text = QString::fromUtf8(m_process.readAllStandardError());
    m_stderr.append(text);
    if (m_stderr.size() > 8000)
        m_stderr = m_stderr.right(4000);
}

void AcpClient::handleLine(const QByteArray &line) {
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(line, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return;

    const QJsonObject object = document.object();
    const bool isResponse = object.contains("id")
        && (object.contains("result") || object.contains("error"))
        && !object.contains("method");

    if (isResponse) {
        handleResponse(object);
        return;
    }

    const QString method = object.value("method").toString();
    if (method.isEmpty())
        return;

    if (object.contains("id"))
        handleAgentRequest(object);
    else if (method == "session/update")
        handleUpdate(object.value("params").toObject());
}

void AcpClient::handleResponse(const QJsonObject &object) {
    const int id = object.value("id").toInt(-1);
    if (!m_pending.contains(id))
        return;
    const QString method = m_pending.take(id).method;

    if (object.contains("error")) {
        const QJsonObject error = object.value("error").toObject();
        const QString message = error.value("message").toString().trimmed();
        if (method == "initialize") {
            fail(tr("OpenCode refused the handshake: %1").arg(message.isEmpty() ? tr("unknown error") : message));
        } else if (method == "session/list") {
            m_loadingSessions = false;
            emit loadingSessionsChanged();
        } else if (method == "session/prompt") {
            setBusy(false);
            addNotice(message.isEmpty() ? tr("OpenCode reported an error.") : message);
        } else {
            addNotice(message.isEmpty() ? tr("OpenCode reported an error.") : message);
        }
        return;
    }

    const QJsonObject result = object.value("result").toObject();

    if (method == "initialize") {
        setState(Ready, tr("Ready"));
        emit ready();
    } else if (method == "session/new" || method == "session/load") {
        const QString returned = result.value("sessionId").toString();
        if (!returned.isEmpty()) {
            m_sessionId = returned;
            emit sessionIdChanged();
        }
        rebuildConfigOptions(result.value("configOptions").toArray());
        setBusy(false);
        if (method == "session/new") {
            m_sessionTitle = tr("New session");
            emit sessionTitleChanged();
        }
    } else if (method == "session/list") {
        m_sessions.clear();
        for (const auto &value : result.value("sessions").toArray())
            m_sessions.append(value.toObject().toVariantMap());
        m_loadingSessions = false;
        emit loadingSessionsChanged();
        emit sessionsChanged();
    } else if (method == "session/prompt") {
        setBusy(false);
        for (int i = 0; i < m_messages.size(); ++i) {
            QVariantMap message = m_messages.at(i).toMap();
            if (message.value("streaming").toBool()) {
                message["streaming"] = false;
                m_messages[i] = message;
            }
        }
        emit messagesChanged();
        const QJsonObject promptUsage = result.value("usage").toObject();
        if (!promptUsage.isEmpty()) {
            m_usage["inputTokens"] = promptUsage.value("inputTokens").toInt();
            m_usage["outputTokens"] = promptUsage.value("outputTokens").toInt();
            m_usage["totalTokens"] = promptUsage.value("totalTokens").toInt();
            emit usageChanged();
        }
    } else if (method == "session/set_config_option") {
        rebuildConfigOptions(result.value("configOptions").toArray());
    }
}

void AcpClient::handleUpdate(const QJsonObject &params) {
    const QString session = params.value("sessionId").toString();
    // Child sessions (subagents) have their own id; mixing their transcript
    // into the parent would be confusing, so only the active session counts.
    if (!session.isEmpty() && !m_sessionId.isEmpty() && session != m_sessionId)
        return;

    const QJsonObject update = params.value("update").toObject();
    const QString kind = update.value("sessionUpdate").toString();
    const QString messageId = update.value("messageId").toString();

    if (kind == "agent_message_chunk") {
        appendChunk("assistant", messageId, contentText(update.value("content").toObject()));
    } else if (kind == "agent_thought_chunk") {
        appendChunk("thought", messageId, contentText(update.value("content").toObject()));
    } else if (kind == "user_message_chunk") {
        appendChunk("user", messageId, contentText(update.value("content").toObject()));
    } else if (kind == "tool_call" || kind == "tool_call_update") {
        upsertTool(update);
    } else if (kind == "plan") {
        QVariantMap plan;
        plan["id"] = QStringLiteral("plan");
        plan["role"] = QStringLiteral("plan");
        plan["messageId"] = QStringLiteral("plan");
        QVariantList entries;
        for (const auto &value : update.value("entries").toArray()) {
            const QJsonObject entry = value.toObject();
            QVariantMap mapped;
            mapped["content"] = entry.value("content").toString();
            mapped["status"] = entry.value("status").toString();
            mapped["priority"] = entry.value("priority").toString();
            entries.append(mapped);
        }
        plan["entries"] = entries;
        bool replaced = false;
        for (int i = 0; i < m_messages.size(); ++i) {
            if (m_messages.at(i).toMap().value("role").toString() == "plan") {
                m_messages[i] = plan;
                replaced = true;
                break;
            }
        }
        if (!replaced)
            m_messages.append(plan);
        emit messagesChanged();
    } else if (kind == "available_commands_update") {
        m_commands.clear();
        for (const auto &value : update.value("availableCommands").toArray()) {
            const QJsonObject command = value.toObject();
            QVariantMap mapped;
            mapped["name"] = command.value("name").toString();
            mapped["description"] = command.value("description").toString();
            m_commands.append(mapped);
        }
        emit commandsChanged();
    } else if (kind == "current_mode_update") {
        updateMode(update.value("currentModeId").toString());
    } else if (kind == "config_option_update") {
        rebuildConfigOptions(update.value("configOptions").toArray());
    } else if (kind == "usage_update") {
        m_usage["used"] = update.value("used").toInt();
        m_usage["size"] = update.value("size").toInt();
        if (update.value("cost").isObject())
            m_usage["cost"] = update.value("cost").toObject().toVariantMap();
        emit usageChanged();
    } else if (kind == "session_info_update") {
        const QString title = update.value("title").toString();
        if (!title.isEmpty() && title != m_sessionTitle) {
            m_sessionTitle = title;
            emit sessionTitleChanged();
        }
    }
}

void AcpClient::handleAgentRequest(const QJsonObject &object) {
    const QString method = object.value("method").toString();
    const int id = object.value("id").toInt(-1);

    if (method == "session/request_permission") {
        const QJsonObject params = object.value("params").toObject();
        const QJsonObject tool = params.value("toolCall").toObject();
        QVariantMap permission;
        permission["requestId"] = id;
        permission["title"] = tool.value("title").toString();
        permission["kind"] = tool.value("kind").toString();
        permission["input"] = tool.value("rawInput").toObject().toVariantMap();
        QVariantList options;
        for (const auto &value : params.value("options").toArray()) {
            const QJsonObject option = value.toObject();
            QVariantMap mapped;
            mapped["optionId"] = option.value("optionId").toString();
            mapped["name"] = option.value("name").toString();
            mapped["kind"] = option.value("kind").toString();
            options.append(mapped);
        }
        permission["options"] = options;
        m_permission = permission;
        emit permissionChanged();
        return;
    }

    // The client advertises no filesystem or terminal capabilities, so any
    // other request is outside the contract. Refusing is safer than guessing.
    QJsonObject error;
    error["code"] = -32601;
    error["message"] = QString("Method not supported by DeepCrasy H: %1").arg(method);
    respond(id, QJsonObject{{"error", error}});
}

void AcpClient::send(const QJsonObject &object) {
    if (m_process.state() != QProcess::Running)
        return;
    m_process.write(QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n');
}

int AcpClient::request(const QString &method, const QJsonObject &params) {
    const int id = m_nextId++;
    QJsonObject object;
    object["jsonrpc"] = "2.0";
    object["id"] = id;
    object["method"] = method;
    if (!params.isEmpty())
        object["params"] = params;
    m_pending.insert(id, PendingCall{method});
    send(object);
    return id;
}

void AcpClient::respond(int id, const QJsonObject &result) {
    QJsonObject object;
    object["jsonrpc"] = "2.0";
    object["id"] = id;
    for (auto it = result.begin(); it != result.end(); ++it)
        object.insert(it.key(), it.value());
    send(object);
}

void AcpClient::clearTranscript() {
    m_messages.clear();
    m_streamingIndex = -1;
    emit messagesChanged();
}

void AcpClient::setBusy(bool busy) {
    if (busy == m_busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void AcpClient::addNotice(const QString &text) {
    QVariantMap notice;
    notice["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    notice["role"] = QStringLiteral("notice");
    notice["text"] = text;
    m_messages.append(notice);
    emit messagesChanged();
}

int AcpClient::findMessage(const QString &role, const QString &messageId) const {
    for (int i = m_messages.size() - 1; i >= 0; --i) {
        const QVariantMap message = m_messages.at(i).toMap();
        if (message.value("role").toString() == role && message.value("messageId").toString() == messageId)
            return i;
    }
    return -1;
}

void AcpClient::appendChunk(const QString &role, const QString &messageId, const QString &text) {
    if (text.isEmpty())
        return;
    const QString id = messageId.isEmpty() ? QStringLiteral("anonymous") : messageId;
    const int index = findMessage(role, id);
    if (index >= 0) {
        QVariantMap message = m_messages.at(index).toMap();
        message["text"] = message.value("text").toString() + text;
        message["streaming"] = true;
        m_messages[index] = message;
    } else {
        QVariantMap message;
        message["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
        message["role"] = role;
        message["messageId"] = id;
        message["text"] = text;
        message["streaming"] = true;
        m_messages.append(message);
    }
    emit messagesChanged();
}

void AcpClient::upsertTool(const QJsonObject &update) {
    const QString toolId = update.value("toolCallId").toString();
    if (toolId.isEmpty())
        return;

    int index = -1;
    for (int i = 0; i < m_messages.size(); ++i) {
        const QVariantMap message = m_messages.at(i).toMap();
        if (message.value("role").toString() == "tool" && message.value("toolId").toString() == toolId) {
            index = i;
            break;
        }
    }

    QVariantMap message = index >= 0 ? m_messages.at(index).toMap() : QVariantMap{};
    if (index < 0) {
        message["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
        message["role"] = QStringLiteral("tool");
        message["toolId"] = toolId;
        message["status"] = QStringLiteral("pending");
    }

    const QString title = update.value("title").toString();
    if (!title.isEmpty())
        message["title"] = title;
    else if (!message.contains("title"))
        message["title"] = update.value("name").toString();

    if (!update.value("kind").toString().isEmpty())
        message["kind"] = update.value("kind").toString();
    if (!update.value("status").toString().isEmpty())
        message["status"] = update.value("status").toString();
    if (update.value("rawInput").isObject())
        message["input"] = update.value("rawInput").toObject().toVariantMap();
    if (update.value("locations").isArray())
        message["locations"] = update.value("locations").toArray().toVariantList();

    const QJsonArray content = update.value("content").toArray();
    if (!content.isEmpty()) {
        const QString output = toolOutput(content);
        if (!output.isEmpty())
            message["output"] = output;
    }

    const QJsonObject rawOutput = update.value("rawOutput").toObject();
    const QJsonObject metadata = rawOutput.value("metadata").toObject();
    if (metadata.contains("exit"))
        message["exitCode"] = metadata.value("exit").toInt();

    if (index >= 0)
        m_messages[index] = message;
    else
        m_messages.append(message);
    emit messagesChanged();
}

void AcpClient::rebuildConfigOptions(const QJsonArray &options) {
    if (options.isEmpty())
        return;
    m_configOptions = options.toVariantList();
    emit configOptionsChanged();
}

void AcpClient::updateMode(const QString &modeId) {
    if (modeId.isEmpty())
        return;
    for (int i = 0; i < m_configOptions.size(); ++i) {
        QVariantMap option = m_configOptions.at(i).toMap();
        if (option.value("id").toString() != "mode")
            continue;
        option["currentValue"] = modeId;
        m_configOptions[i] = option;
        emit configOptionsChanged();
        return;
    }
}

void AcpClient::newSession(const QString &cwd) {
    if (m_state != Ready)
        return;
    clearTranscript();
    m_sessionId.clear();
    m_sessionTitle = tr("New session");
    emit sessionIdChanged();
    emit sessionTitleChanged();
    m_permission.clear();
    emit permissionChanged();
    m_commands.clear();
    emit commandsChanged();
    m_usage.clear();
    emit usageChanged();
    request("session/new", QJsonObject{{"cwd", cwd}, {"mcpServers", QJsonArray{}}});
}

void AcpClient::loadSession(const QString &id, const QString &cwd) {
    if (m_state != Ready || id.isEmpty())
        return;
    clearTranscript();
    m_sessionId = id;
    emit sessionIdChanged();
    for (const auto &value : m_sessions) {
        const QVariantMap session = value.toMap();
        if (session.value("sessionId").toString() == id) {
            m_sessionTitle = session.value("title").toString();
            emit sessionTitleChanged();
            break;
        }
    }
    m_permission.clear();
    emit permissionChanged();
    request("session/load", QJsonObject{
        {"sessionId", id},
        {"cwd", cwd},
        {"mcpServers", QJsonArray{}},
    });
}

void AcpClient::refreshSessions(const QString &cwd) {
    if (m_state != Ready)
        return;
    m_loadingSessions = true;
    emit loadingSessionsChanged();
    request("session/list", QJsonObject{{"cwd", cwd}});
}

void AcpClient::prompt(const QString &text) {
    if (m_state != Ready || m_sessionId.isEmpty() || m_busy)
        return;
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return;

    QVariantMap user;
    user["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    user["role"] = QStringLiteral("user");
    user["text"] = trimmed;
    user["streaming"] = false;
    m_messages.append(user);
    m_streamingIndex = -1;
    emit messagesChanged();

    setBusy(true);
    QJsonArray blocks;
    blocks.append(QJsonObject{{"type", "text"}, {"text", trimmed}});
    request("session/prompt", QJsonObject{{"sessionId", m_sessionId}, {"prompt", blocks}});
}

void AcpClient::cancel() {
    if (!m_busy || m_sessionId.isEmpty())
        return;
    QJsonObject object;
    object["jsonrpc"] = "2.0";
    object["method"] = "session/cancel";
    object["params"] = QJsonObject{{"sessionId", m_sessionId}};
    send(object);
}

void AcpClient::setConfigOption(const QString &id, const QString &value) {
    if (m_state != Ready || m_sessionId.isEmpty() || id.isEmpty())
        return;
    for (int i = 0; i < m_configOptions.size(); ++i) {
        QVariantMap option = m_configOptions.at(i).toMap();
        if (option.value("id").toString() != id)
            continue;
        option["currentValue"] = value;
        m_configOptions[i] = option;
        emit configOptionsChanged();
        break;
    }
    request("session/set_config_option", QJsonObject{
        {"sessionId", m_sessionId},
        {"configId", id},
        {"value", value},
    });
}

void AcpClient::respondPermission(const QString &optionId) {
    if (m_permission.isEmpty())
        return;
    const int requestId = m_permission.value("requestId").toInt();
    QJsonObject outcome;
    if (optionId.isEmpty()) {
        outcome["outcome"] = "cancelled";
    } else {
        outcome["outcome"] = "selected";
        outcome["optionId"] = optionId;
    }
    respond(requestId, QJsonObject{{"outcome", outcome}});
    m_permission.clear();
    emit permissionChanged();
}
