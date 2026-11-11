#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

// Follows Omarchy's theme: the active palette by default, or any installed
// theme picked in the app. Every surface color is derived from the theme's
// background, foreground, accent, and terminal colors, so light and dark
// themes both stay readable instead of flipping a hardcoded dark palette.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(QString label READ label NOTIFY changed)
    Q_PROPERTY(QString systemTheme READ systemTheme NOTIFY changed)
    Q_PROPERTY(QString systemThemeId READ systemThemeId NOTIFY changed)
    Q_PROPERTY(bool dark READ dark NOTIFY changed)
    Q_PROPERTY(QString selectedTheme READ selectedTheme NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList themes READ themes NOTIFY themesChanged)
    Q_PROPERTY(int radius READ radius NOTIFY changed)

    Q_PROPERTY(QString background READ background NOTIFY changed)
    Q_PROPERTY(QString surface READ surface NOTIFY changed)
    Q_PROPERTY(QString raised READ raised NOTIFY changed)
    Q_PROPERTY(QString raisedHover READ raisedHover NOTIFY changed)
    Q_PROPERTY(QString chip READ chip NOTIFY changed)
    Q_PROPERTY(QString chipHover READ chipHover NOTIFY changed)
    Q_PROPERTY(QString menuHover READ menuHover NOTIFY changed)
    Q_PROPERTY(QString menuSelected READ menuSelected NOTIFY changed)
    Q_PROPERTY(QString border READ border NOTIFY changed)
    Q_PROPERTY(QString borderStrong READ borderStrong NOTIFY changed)
    Q_PROPERTY(QString text READ text NOTIFY changed)
    Q_PROPERTY(QString brightText READ brightText NOTIFY changed)
    Q_PROPERTY(QString mutedText READ mutedText NOTIFY changed)
    Q_PROPERTY(QString dimText READ dimText NOTIFY changed)
    Q_PROPERTY(QString accent READ accent NOTIFY changed)
    Q_PROPERTY(QString accentForeground READ accentForeground NOTIFY changed)
    Q_PROPERTY(QString accentSoft READ accentSoft NOTIFY changed)
    Q_PROPERTY(QString selection READ selection NOTIFY changed)
    Q_PROPERTY(QString red READ red NOTIFY changed)
    Q_PROPERTY(QString green READ green NOTIFY changed)
    Q_PROPERTY(QString yellow READ yellow NOTIFY changed)
    Q_PROPERTY(QString blue READ blue NOTIFY changed)
    Q_PROPERTY(QString cyan READ cyan NOTIFY changed)
    Q_PROPERTY(QString magenta READ magenta NOTIFY changed)
    Q_PROPERTY(QString orange READ orange NOTIFY changed)
    Q_PROPERTY(QString ambientTop READ ambientTop NOTIFY changed)
    Q_PROPERTY(QString ambientBottom READ ambientBottom NOTIFY changed)
    Q_PROPERTY(QString shadow READ shadow NOTIFY changed)

public:
    explicit Theme(const QString &currentDirectory = {}, QObject *parent = nullptr, const QString &hyprctl = {});
    ~Theme() override;

    QString name() const { return m_name; }
    QString label() const { return m_label; }
    QString systemTheme() const { return m_systemLabel; }
    QString systemThemeId() const { return m_systemId; }
    bool dark() const { return m_dark; }
    QString selectedTheme() const { return m_selected; }
    QVariantList themes() const { return m_themes; }
    int radius() const { return m_radius; }

    QString background() const { return m_background; }
    QString surface() const { return m_surface; }
    QString raised() const { return m_raised; }
    QString raisedHover() const { return m_raisedHover; }
    QString chip() const { return m_chip; }
    QString chipHover() const { return m_chipHover; }
    QString menuHover() const { return m_menuHover; }
    QString menuSelected() const { return m_menuSelected; }
    QString border() const { return m_border; }
    QString borderStrong() const { return m_borderStrong; }
    QString text() const { return m_text; }
    QString brightText() const { return m_brightText; }
    QString mutedText() const { return m_mutedText; }
    QString dimText() const { return m_dimText; }
    QString accent() const { return m_accent; }
    QString accentForeground() const { return m_accentForeground; }
    QString accentSoft() const { return m_accentSoft; }
    QString selection() const { return m_selection; }
    QString red() const { return m_red; }
    QString green() const { return m_green; }
    QString yellow() const { return m_yellow; }
    QString blue() const { return m_blue; }
    QString cyan() const { return m_cyan; }
    QString magenta() const { return m_magenta; }
    QString orange() const { return m_orange; }
    QString ambientTop() const { return m_ambientTop; }
    QString ambientBottom() const { return m_ambientBottom; }
    QString shadow() const { return m_shadow; }

    bool setSelectedTheme(const QString &id);
    Q_INVOKABLE void refreshThemes();

    static QVariantMap readPalette(const QString &path);
    static QString readAccent(const QString &path);
    static QVariantList scanThemes(const QStringList &roots);
    static QString prettyName(const QString &id);

signals:
    void changed();
    void selectionChanged();
    void themesChanged();

private:
    void reload();
    void refreshRounding();
    void applyPalette(const QVariantMap &palette);
    void armWatcher(const QString &palettePath);
    QString themePath(const QString &id) const;

    QString m_directory;
    QString m_selected;
    QString m_name;
    QString m_label;
    QString m_systemId;
    QString m_systemLabel = QStringLiteral("Omarchy");
    QString m_signature;
    QVariantList m_themes;
    QStringList m_roots;

    bool m_dark = true;
    QString m_background = "#0e0e10";
    QString m_surface = "#0a0a0c";
    QString m_raised = "#151519";
    QString m_raisedHover = "#1b1b21";
    QString m_chip = "#17171c";
    QString m_chipHover = "#202028";
    QString m_menuHover = "#1a1a20";
    QString m_menuSelected = "#242433";
    QString m_border = "#26262c";
    QString m_borderStrong = "#3a3a44";
    QString m_text = "#e6e6e6";
    QString m_brightText = "#f5f5f5";
    QString m_mutedText = "#a0a0aa";
    QString m_dimText = "#6f6f7a";
    QString m_accent = "#FFD60A";
    QString m_accentForeground = "black";
    QString m_accentSoft = "#1FFFD60A";
    QString m_selection = "#2a2a32";
    QString m_red = "#f06a6a";
    QString m_green = "#5bd6a0";
    QString m_yellow = "#f2c94c";
    QString m_blue = "#7aa2f7";
    QString m_cyan = "#3fc9d6";
    QString m_magenta = "#c792ea";
    QString m_orange = "#f2914e";
    QString m_ambientTop = "#12121a";
    QString m_ambientBottom = "#0e0e10";
    QString m_shadow = "#6E000000";

    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QString m_hyprctl;
    int m_radius = 0;
    QProcess m_roundingProcess;
    QTimer m_roundingPoll, m_roundingTimeout;
};
