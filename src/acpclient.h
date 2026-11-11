#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QVariantList>
#include <QVariantMap>

#include "messagemodel.h"

// Speaks the Agent Client Protocol to `opencode acp` over stdio, so OpenCode
// runs as a native chat pane in DeepCrasy H instead of a terminal. The client
// owns the process, the session, and the transcript; QML only renders it.
class AcpClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString detail READ detail NOTIFY detailChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QAbstractListModel *messageModel READ messageModel CONSTANT)
    Q_PROPERTY(int messageCount READ messageCount NOTIFY messagesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(QString sessionTitle READ sessionTitle NOTIFY sessionTitleChanged)
    Q_PROPERTY(QVariantList configOptions READ configOptions NOTIFY configOptionsChanged)
    Q_PROPERTY(QVariantList commands READ commands NOTIFY commandsChanged)
    Q_PROPERTY(QVariantMap usage READ usage NOTIFY usageChanged)
    Q_PROPERTY(QVariantMap permission READ permission NOTIFY permissionChanged)
    Q_PROPERTY(QVariantList sessions READ sessions NOTIFY sessionsChanged)
    Q_PROPERTY(bool loadingSessions READ loadingSessions NOTIFY loadingSessionsChanged)

public:
    enum State { Stopped, Starting, Ready, Failed };
    Q_ENUM(State)

    explicit AcpClient(QObject *parent = nullptr);
    ~AcpClient() override;

    State state() const { return m_state; }
    QString detail() const { return m_detail; }
    QVariantList messages() const { return m_messages.items(); }
    QAbstractListModel *messageModel() { return &m_messages; }
    int messageCount() const { return m_messages.count(); }
    bool busy() const { return m_busy; }
    QString sessionId() const { return m_sessionId; }
    QString sessionTitle() const { return m_sessionTitle; }
    QVariantList configOptions() const { return m_configOptions; }
    QVariantList commands() const { return m_commands; }
    QVariantMap usage() const { return m_usage; }
    QVariantMap permission() const { return m_permission; }
    QVariantList sessions() const { return m_sessions; }
    bool loadingSessions() const { return m_loadingSessions; }

    Q_INVOKABLE void start(const QString &binary, const QString &workspace);
    Q_INVOKABLE void stop();

    Q_INVOKABLE void newSession(const QString &cwd);
    Q_INVOKABLE void loadSession(const QString &id, const QString &cwd);
    Q_INVOKABLE void refreshSessions(const QString &cwd);
    Q_INVOKABLE void prompt(const QString &text);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void setConfigOption(const QString &id, const QString &value);
    Q_INVOKABLE void respondPermission(const QString &optionId);
    void setConversation(const QVariantList &messages);

signals:
    void stateChanged();
    void detailChanged();
    void messagesChanged();
    void busyChanged();
    void sessionIdChanged();
    void sessionTitleChanged();
    void configOptionsChanged();
    void commandsChanged();
    void usageChanged();
    void permissionChanged();
    void sessionsChanged();
    void loadingSessionsChanged();
    void ready();

private:
    struct PendingCall {
        QString method;
    };

    void setState(State state, const QString &detail = {});
    void consumeStdout();
    void consumeStderr();
    void handleLine(const QByteArray &line);
    void handleResponse(const QJsonObject &object);
    void handleUpdate(const QJsonObject &params);
    void handlePermission(const QJsonObject &object);
    void handleAgentRequest(const QJsonObject &object);
    void send(const QJsonObject &object);
    int request(const QString &method, const QJsonObject &params = {});
    void respond(int id, const QJsonObject &result);
    void fail(const QString &detail);

    int findMessage(const QString &role, const QString &messageId) const;
    void appendChunk(const QString &role, const QString &messageId, const QString &text);
    void upsertTool(const QJsonObject &update);
    void clearTranscript();
    void setBusy(bool busy);
    void addNotice(const QString &text);
    void rebuildConfigOptions(const QJsonArray &options);
    void updateMode(const QString &modeId);

    QProcess m_process;
    QByteArray m_buffer;
    QHash<int, PendingCall> m_pending;
    int m_nextId = 1;

    State m_state = Stopped;
    QString m_detail;
    QString m_binary;
    QString m_workspace;
    QString m_sessionId;
    QString m_sessionTitle;
    QString m_stderr;
    MessageListModel m_messages;
    QVariantList m_configOptions;
    QVariantList m_commands;
    QVariantMap m_usage;
    QVariantMap m_permission;
    QVariantList m_sessions;
    bool m_busy = false;
    bool m_loadingSessions = false;
    bool m_stopping = false;
    int m_streamingIndex = -1;
};
