#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QVariantList>
#include <QVariantMap>

class QNetworkReply;
class Config;
class HarnessServer;
class PortalFolderPicker;
class Theme;

// Harness-only application backend: owns the DeepSeek Harness process, the
// workspace, Git state, diagnostics, themes and the app settings. The terminal
// agent is a separate PTY object owned by QML.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString view READ view NOTIFY viewChanged)
    Q_PROPERTY(QString themePreference READ themePreference NOTIFY themeChanged)
    Q_PROPERTY(QString languagePreference READ languagePreference NOTIFY languageChanged)
    Q_PROPERTY(QString activeTheme READ activeTheme NOTIFY themeChanged)
    Q_PROPERTY(QString systemTheme READ systemTheme NOTIFY themeChanged)
    Q_PROPERTY(QString systemThemeId READ systemThemeId NOTIFY themeChanged)
    Q_PROPERTY(bool darkTheme READ darkTheme NOTIFY themeChanged)
    Q_PROPERTY(QVariantList themeOptions READ themeOptions NOTIFY themeOptionsChanged)
    Q_PROPERTY(QString startupView READ startupView NOTIFY optionsChanged)
    Q_PROPERTY(bool autoStartHarness READ autoStartHarness NOTIFY optionsChanged)
    Q_PROPERTY(QString gitBranch READ gitBranch NOTIFY gitChanged)
    Q_PROPERTY(int gitDirtyCount READ gitDirtyCount NOTIFY gitChanged)
    Q_PROPERTY(bool gitAvailable READ gitAvailable NOTIFY gitChanged)
    Q_PROPERTY(QVariantList diagnostics READ diagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(bool diagnosticsRunning READ diagnosticsRunning NOTIFY diagnosticsChanged)
    Q_PROPERTY(QString harnessHealth READ harnessHealth NOTIFY harnessHealthChanged)
    Q_PROPERTY(QString harnessAddress READ harnessAddress NOTIFY harnessChanged)
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

public:
    explicit Backend(QObject *parent = nullptr);

    void boot();
    void loadDemo();
    void attachTheme(Theme *theme);
    void applyStartupView();

    QString view() const { return m_view; }
    QString themePreference() const;
    QString languagePreference() const;
    QString activeTheme() const;
    QString systemTheme() const;
    QString systemThemeId() const;
    bool darkTheme() const;
    QVariantList themeOptions() const;
    QString startupView() const;
    bool autoStartHarness() const;
    QString gitBranch() const { return m_demo ? QStringLiteral("main") : m_gitBranch; }
    int gitDirtyCount() const { return m_demo ? 3 : m_gitDirty; }
    bool gitAvailable() const { return m_demo ? true : m_gitAvailable; }
    QVariantList diagnostics() const { return m_diagnostics; }
    bool diagnosticsRunning() const { return m_diagnosticsRunning; }
    QString harnessHealth() const { return m_demo ? QStringLiteral("ok") : m_harnessHealth; }
    QString harnessAddress() const;
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

    Q_INVOKABLE void setView(const QString &view);
    Q_INVOKABLE void setTheme(const QString &id);
    Q_INVOKABLE void setLanguage(const QString &code);
    Q_INVOKABLE void refreshThemes();
    Q_INVOKABLE void setStartupView(const QString &view);
    Q_INVOKABLE void setAutoStartHarness(bool enabled);
    Q_INVOKABLE void setDshBinary(const QString &binary);
    Q_INVOKABLE void setHarnessProfile(const QString &profile);
    Q_INVOKABLE void restartHarness();
    Q_INVOKABLE void testHarness();
    Q_INVOKABLE void runDiagnostics();
    Q_INVOKABLE void refreshGit();
    Q_INVOKABLE void pickWorkspace();

    void setWorkspace(const QString &workspace);

signals:
    void viewChanged();
    void themeChanged();
    void languageChanged();
    void themeOptionsChanged();
    void optionsChanged();
    void workspaceChanged();
    void harnessChanged();
    void harnessHealthChanged();
    void harnessProfileChanged();
    void dshBinaryChanged();
    void gitChanged();
    void diagnosticsChanged();
    void toast(const QString &message);

private:
    QString harnessStateName() const;
    void refreshGitNow();

    Config *m_config = nullptr;
    HarnessServer *m_harness = nullptr;
    Theme *m_theme = nullptr;
    PortalFolderPicker *m_portal = nullptr;
    QNetworkAccessManager m_network;
    QNetworkReply *m_healthReply = nullptr;
    QProcess *m_gitProcess = nullptr;
    QString m_view;
    QString m_harnessHealth = QStringLiteral("unknown");
    QString m_gitBranch;
    int m_gitDirty = 0;
    bool m_gitAvailable = false;
    QVariantList m_diagnostics;
    bool m_diagnosticsRunning = false;
    bool m_booted = false;
    bool m_demo = false;
};
