#pragma once

#include <QObject>

// Persistent settings for Ultimate IAH, stored as one JSON object under the
// user's config directory. Every setter writes atomically.
class Config : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString workspace READ workspace WRITE setWorkspace NOTIFY workspaceChanged)
    Q_PROPERTY(QString harnessProfile READ harnessProfile WRITE setHarnessProfile NOTIFY harnessProfileChanged)
    Q_PROPERTY(QString dshBinary READ dshBinary WRITE setDshBinary NOTIFY dshBinaryChanged)
    Q_PROPERTY(QString view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString startupView READ startupView WRITE setStartupView NOTIFY optionsChanged)
    Q_PROPERTY(bool autoStartHarness READ autoStartHarness WRITE setAutoStartHarness NOTIFY optionsChanged)

public:
    explicit Config(const QString &path = {}, QObject *parent = nullptr);

    QString workspace() const { return m_workspace; }
    void setWorkspace(const QString &workspace);
    QString harnessProfile() const { return m_harnessProfile; }
    void setHarnessProfile(const QString &profile);
    QString dshBinary() const { return m_dshBinary; }
    void setDshBinary(const QString &binary);
    QString view() const { return m_view; }
    void setView(const QString &view);
    QString theme() const { return m_theme; }
    void setTheme(const QString &theme);
    QString language() const { return m_language; }
    void setLanguage(const QString &language);
    QString startupView() const { return m_startupView; }
    void setStartupView(const QString &view);
    bool autoStartHarness() const { return m_autoStartHarness; }
    void setAutoStartHarness(bool enabled);
    QString path() const { return m_path; }

    static QString defaultPath();
    static QString defaultWorkspace();
    static QString detectBinary(const QString &name);
    static QString detectDsh();

signals:
    void workspaceChanged();
    void harnessProfileChanged();
    void dshBinaryChanged();
    void viewChanged();
    void themeChanged();
    void languageChanged();
    void optionsChanged();

private:
    void load();
    void save();
    static bool validView(const QString &view);

    QString m_path;
    QString m_workspace;
    QString m_harnessProfile;
    QString m_dshBinary;
    QString m_view;
    QString m_theme;
    QString m_language;
    QString m_startupView = QStringLiteral("home");
    bool m_autoStartHarness = true;
};
