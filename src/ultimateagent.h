#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantList>
#include <QStringList>

class QNetworkReply;
class QTimer;

class AgentManager;
class Backend;
class ConnectionDoctor;
class HarnessConnections;
class InstallationDoctor;
class MausBotManager;
class McpManager;
class MemoryStore;
class MissionRunner;
class NineRouterManager;
class SkillManager;
class TerminalSession;

// The built-in Ultimate HAI operator. It is a small tool-calling agent loop over
// an OpenAI-compatible endpoint, with explicit tools for this application's
// real ecosystem. The model never gets direct access to secrets: tools return
// only masked values and errors.
class UltimateAgent : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString modelLabel READ modelLabel NOTIFY endpointChanged)
    Q_PROPERTY(QString endpointLabel READ endpointLabel NOTIFY endpointChanged)
    Q_PROPERTY(QString conversationTitle READ conversationTitle NOTIFY messagesChanged)

public:
    UltimateAgent(Backend *backend, HarnessConnections *connections,
                  NineRouterManager *nineRouter, MausBotManager *mausBot,
                  McpManager *mcp, MemoryStore *memory, SkillManager *skills,
                  AgentManager *agents, MissionRunner *missions,
                  TerminalSession *terminal, InstallationDoctor *installation,
                  ConnectionDoctor *connectionDoctor, QObject *parent = nullptr);

    QVariantList messages() const { return m_ui; }
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    QString status() const { return m_status; }
    QString modelLabel() const { return m_modelLabel; }
    QString endpointLabel() const { return m_endpointLabel; }
    QString conversationTitle() const { return m_title; }

    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void clearConversation();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void refreshEndpoint();
    Q_INVOKABLE void toggleToolDetails(int index);

    static QJsonArray tools();
    static QString staticKnowledge();
    static QStringList views();

signals:
    void messagesChanged();
    void busyChanged();
    void errorChanged();
    void statusChanged();
    void endpointChanged();

private:
    struct Endpoint {
        QString baseUrl;
        QString apiKey;
        QString model;
        QString label;
        QString source;
        bool valid() const { return !baseUrl.isEmpty() && !apiKey.isEmpty() && !model.isEmpty(); }
    };

    Endpoint resolveEndpoint() const;
    QString systemPrompt() const;
    QJsonObject snapshot() const;
    QJsonObject executeTool(const QString &name, const QJsonObject &arguments);
    QString toolSummary(const QString &name, const QJsonObject &arguments,
                        const QJsonObject &result) const;
    void runRequest();
    void handleReply();
    void finishWithError(const QString &message);
    void appendUser(const QString &text);
    void appendAssistant(const QString &text);
    void appendTool(const QString &name, const QString &detail, bool ok, const QString &payload);
    void setBusy(bool busy);
    void setStatus(const QString &status);
    void setError(const QString &error);
    void save();
    void load();
    void abortReply();
    QString credentialsPath() const;
    QString credential(const QString &name) const;
    bool destructiveConfirmed(const QJsonObject &arguments, const QString &action) const;

    Backend *m_backend = nullptr;
    HarnessConnections *m_connections = nullptr;
    NineRouterManager *m_nineRouter = nullptr;
    MausBotManager *m_mausBot = nullptr;
    McpManager *m_mcp = nullptr;
    MemoryStore *m_memory = nullptr;
    SkillManager *m_skills = nullptr;
    AgentManager *m_agents = nullptr;
    MissionRunner *m_missions = nullptr;
    TerminalSession *m_terminal = nullptr;
    InstallationDoctor *m_installation = nullptr;
    ConnectionDoctor *m_connectionDoctor = nullptr;

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
    QTimer *m_timeout = nullptr;

    QJsonArray m_api;
    QVariantList m_ui;
    Endpoint m_endpoint;
    QString m_modelLabel;
    QString m_endpointLabel;
    QString m_status;
    QString m_error;
    QString m_title;
    bool m_busy = false;
    int m_round = 0;
    int m_requestSerial = 0;
};
