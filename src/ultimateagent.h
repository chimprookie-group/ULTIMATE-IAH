#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QStringList>

class QNetworkReply;
class QTimer;
class AgentConversationStore;

class AgentManager;
class BotManager;
class Backend;
class ConnectionDoctor;
class HarnessConnections;
class InstallationDoctor;
class McpManager;
class MemoryStore;
class MissionRunner;
class NoteStore;
class PluginManager;
class WebService;
class GitHubService;
class CrasyRouter;
class SkillManager;
class LearningEngine;
class TerminalSession;

// The built-in Ultimate IAH operator. It is a small tool-calling agent loop over
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
    Q_PROPERTY(QString conversationId READ conversationId NOTIFY conversationsChanged)
    Q_PROPERTY(QVariantList conversations READ conversations NOTIFY conversationsChanged)
    Q_PROPERTY(QString conversationQuery READ conversationQuery WRITE setConversationQuery NOTIFY conversationQueryChanged)
    Q_PROPERTY(QVariantList pendingAttachments READ pendingAttachments NOTIFY attachmentsChanged)
    Q_PROPERTY(QVariantList availableSkills READ availableSkills NOTIFY skillsChanged)
    Q_PROPERTY(QStringList activeSkills READ activeSkills NOTIFY skillsChanged)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(int contextPercent READ contextPercent NOTIFY messagesChanged)

public:
    UltimateAgent(Backend *backend, HarnessConnections *connections,
                  McpManager *mcp, MemoryStore *memory, SkillManager *skills,
                  AgentManager *agents, MissionRunner *missions,
                  TerminalSession *terminal, InstallationDoctor *installation,
                  ConnectionDoctor *connectionDoctor, BotManager *bots, NoteStore *notes,
                  PluginManager *plugins, WebService *web, GitHubService *gitHub,
                  CrasyRouter *crasy, LearningEngine *learning, QObject *parent = nullptr);

    QVariantList messages() const { return m_ui; }
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    QString status() const { return m_status; }
    QString modelLabel() const { return m_modelLabel; }
    QString endpointLabel() const { return m_endpointLabel; }
    QString conversationTitle() const { return m_title; }
    QString conversationId() const { return m_conversationId; }
    QVariantList conversations() const;
    QString conversationQuery() const { return m_conversationQuery; }
    void setConversationQuery(const QString &query);
    QVariantList pendingAttachments() const { return m_pendingAttachments; }
    QVariantList availableSkills() const;
    QStringList activeSkills() const { return m_activeSkills; }
    QString mode() const { return m_mode; }
    void setMode(const QString &mode);
    void setPermissionMode(const QString &mode) { m_permissionMode = mode == "read-only" ? "read-only" : "workspace-write"; }
    int contextPercent() const;

    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void clearConversation();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void refreshEndpoint();
    Q_INVOKABLE void toggleToolDetails(int index);
    Q_INVOKABLE void newConversation();
    Q_INVOKABLE void selectConversation(const QString &id);
    Q_INVOKABLE void deleteConversation(const QString &id);
    Q_INVOKABLE void renameConversation(const QString &id, const QString &title);
    Q_INVOKABLE void pinConversation(const QString &id);
    Q_INVOKABLE void archiveConversation(const QString &id);
    Q_INVOKABLE QString conversationMarkdown(const QString &id) const;
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE bool addAttachment(const QUrl &url);
    Q_INVOKABLE void removeAttachment(int index);
    Q_INVOKABLE void clearAttachments();
    Q_INVOKABLE void toggleSkill(const QString &id);
    Q_INVOKABLE void compactConversation();

    static QJsonArray tools();
    static QString staticKnowledge();
    static QStringList views();

signals:
    void messagesChanged();
    void busyChanged();
    void errorChanged();
    void statusChanged();
    void endpointChanged();
    void conversationsChanged();
    void conversationQueryChanged();
    void attachmentsChanged();
    void skillsChanged();
    void modeChanged();

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
    QJsonArray allTools() const;
    QJsonObject executeTool(const QString &name, const QJsonObject &arguments);
    QString toolSummary(const QString &name, const QJsonObject &arguments,
                        const QJsonObject &result) const;
    void runRequest();
    void handleReply();
    void finishWithError(const QString &message);
    void appendUser(const QString &text, const QVariantList &images);
    void appendAssistant(const QString &text);
    void appendTool(const QString &name, const QString &detail, bool ok, const QString &payload);
    void saveConversation();
    bool loadConversation(const QString &id);
    void ensureConversation();
    void refreshConversationList();
    QJsonValue userContent(const QString &text, const QVariantList &images) const;
    QString attachmentDataUrl(const QString &path) const;
    QString attachmentDirectory() const;
    void clearPendingAttachments(bool deleteFiles);
    void removeAttachmentsIn(const QJsonArray &ui);
    void setBusy(bool busy);
    void setStatus(const QString &status);
    void setError(const QString &error);
    void save();
    void recordLearning(bool success, const QString &answer, const QString &error = {});
    void load();
    void abortReply();
    QString credentialsPath() const;
    QString credential(const QString &name) const;
    bool destructiveConfirmed(const QJsonObject &arguments, const QString &action) const;

    Backend *m_backend = nullptr;
    HarnessConnections *m_connections = nullptr;
    McpManager *m_mcp = nullptr;
    MemoryStore *m_memory = nullptr;
    SkillManager *m_skills = nullptr;
    AgentManager *m_agents = nullptr;
    MissionRunner *m_missions = nullptr;
    TerminalSession *m_terminal = nullptr;
    InstallationDoctor *m_installation = nullptr;
    ConnectionDoctor *m_connectionDoctor = nullptr;
    BotManager *m_bots = nullptr;
    NoteStore *m_notes = nullptr;
    PluginManager *m_plugins = nullptr;
    WebService *m_web = nullptr;
    GitHubService *m_gitHub = nullptr;
    CrasyRouter *m_crasy = nullptr;
    LearningEngine *m_learning = nullptr;
    AgentConversationStore *m_conversations = nullptr;

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
    QString m_conversationId;
    QString m_conversationQuery;
    QVariantList m_pendingAttachments;
    QStringList m_activeSkills;
    QString m_userPrompt;
    QString m_mode = QStringLiteral("assistant");
    QString m_permissionMode = QStringLiteral("workspace-write");
    bool m_busy = false;
    int m_round = 0;
    int m_requestSerial = 0;
    bool m_learningRecorded = false;
};
