#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class AcpClient;
class Config;
class HarnessServer;
class PortalFolderPicker;

// The single object QML talks to. It owns the configuration, the two engines
// (DeepSeek Harness over HTTP, OpenCode over ACP), and the one workspace they
// share, and flattens their state into properties a view can bind to.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString view READ view NOTIFY viewChanged)
    Q_PROPERTY(QString workspace READ workspace NOTIFY workspaceChanged)
    Q_PROPERTY(QString workspaceName READ workspaceName NOTIFY workspaceChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString monoFont READ monoFont CONSTANT)
    Q_PROPERTY(QString uiFont READ uiFont CONSTANT)
    Q_PROPERTY(bool demo READ demo CONSTANT)

    Q_PROPERTY(QString harnessState READ harnessState NOTIFY harnessChanged)
    Q_PROPERTY(QString harnessUrl READ harnessUrl NOTIFY harnessChanged)
    Q_PROPERTY(QString harnessDetail READ harnessDetail NOTIFY harnessChanged)
    Q_PROPERTY(QString harnessProfile READ harnessProfile NOTIFY harnessProfileChanged)
    Q_PROPERTY(QString harnessLog READ harnessLog NOTIFY harnessChanged)
    Q_PROPERTY(QString dshBinary READ dshBinary NOTIFY dshBinaryChanged)
    Q_PROPERTY(bool dshAvailable READ dshAvailable NOTIFY dshBinaryChanged)

    Q_PROPERTY(QString opencodeState READ opencodeState NOTIFY opencodeChanged)
    Q_PROPERTY(QString opencodeDetail READ opencodeDetail NOTIFY opencodeChanged)
    Q_PROPERTY(QString opencodeBinary READ opencodeBinary NOTIFY opencodeBinaryChanged)
    Q_PROPERTY(bool opencodeAvailable READ opencodeAvailable NOTIFY opencodeBinaryChanged)

    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY sessionChanged)
    Q_PROPERTY(QString sessionTitle READ sessionTitle NOTIFY sessionChanged)
    Q_PROPERTY(QVariantList modelOptions READ modelOptions NOTIFY configOptionsChanged)
    Q_PROPERTY(QVariantList modeOptions READ modeOptions NOTIFY configOptionsChanged)
    Q_PROPERTY(QString currentModel READ currentModel NOTIFY configOptionsChanged)
    Q_PROPERTY(QString currentMode READ currentMode NOTIFY configOptionsChanged)
    Q_PROPERTY(QVariantList commands READ commands NOTIFY commandsChanged)
    Q_PROPERTY(QVariantMap usage READ usage NOTIFY usageChanged)
    Q_PROPERTY(QVariantMap permission READ permission NOTIFY permissionChanged)
    Q_PROPERTY(QVariantList sessions READ sessions NOTIFY sessionsChanged)
    Q_PROPERTY(bool loadingSessions READ loadingSessions NOTIFY sessionsChanged)

public:
    explicit Backend(QObject *parent = nullptr);

    void boot();
    void loadDemo();

    QString view() const { return m_view; }
    QString workspace() const;
    QString workspaceName() const;
    QString appVersion() const;
    QString monoFont() const;
    QString uiFont() const;
    bool demo() const { return m_demo; }

    QString harnessState() const;
    QString harnessUrl() const;
    QString harnessDetail() const;
    QString harnessProfile() const;
    QString harnessLog() const;
    QString dshBinary() const;
    bool dshAvailable() const;

    QString opencodeState() const;
    QString opencodeDetail() const;
    QString opencodeBinary() const;
    bool opencodeAvailable() const;

    QVariantList messages() const;
    bool busy() const;
    QString sessionId() const;
    QString sessionTitle() const;
    QVariantList modelOptions() const;
    QVariantList modeOptions() const;
    QString currentModel() const;
    QString currentMode() const;
    QVariantList commands() const;
    QVariantMap usage() const;
    QVariantMap permission() const;
    QVariantList sessions() const;
    bool loadingSessions() const;

    Q_INVOKABLE void setView(const QString &view);
    Q_INVOKABLE void setWorkspace(const QString &workspace);
    Q_INVOKABLE void pickWorkspace();
    Q_INVOKABLE void restartHarness();
    Q_INVOKABLE void restartOpencode();
    Q_INVOKABLE void setHarnessProfile(const QString &profile);
    Q_INVOKABLE void setOpencodeBinary(const QString &binary);
    Q_INVOKABLE void setDshBinary(const QString &binary);

    Q_INVOKABLE void sendPrompt(const QString &text);
    Q_INVOKABLE void cancelPrompt();
    Q_INVOKABLE void newSession();
    Q_INVOKABLE void refreshSessions();
    Q_INVOKABLE void loadSession(const QString &id);
    Q_INVOKABLE void setConfigOption(const QString &id, const QString &value);
    Q_INVOKABLE void respondPermission(const QString &optionId);

signals:
    void viewChanged();
    void workspaceChanged();
    void harnessChanged();
    void harnessProfileChanged();
    void dshBinaryChanged();
    void opencodeChanged();
    void opencodeBinaryChanged();
    void messagesChanged();
    void busyChanged();
    void sessionChanged();
    void configOptionsChanged();
    void commandsChanged();
    void usageChanged();
    void permissionChanged();
    void sessionsChanged();
    void toast(const QString &message);

private:
    void startOpencode();
    void buildDemo();

    Config *m_config = nullptr;
    HarnessServer *m_harness = nullptr;
    AcpClient *m_acp = nullptr;
    PortalFolderPicker *m_portal = nullptr;

    QString m_view;
    bool m_booted = false;
    bool m_demo = false;

    QVariantList m_demoMessages;
    QVariantList m_demoModelOptions;
    QVariantList m_demoModeOptions;
    QVariantMap m_demoUsage;
    QString m_demoSessionTitle;
};
