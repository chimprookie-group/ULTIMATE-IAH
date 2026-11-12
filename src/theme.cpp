// Palette handling follows the shape of Omarchy's colors.toml; the watcher
// pattern comes from Omacut (MIT, David Heinemeier Hansson).
#include "theme.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <algorithm>
#include <cmath>

namespace {

QVariantMap fallbackPalette()
{
    return {
        {"mode", "dark"},
        {"accent", "#FFD60A"},
        {"background", "#0e0e10"},
        {"dark_background", "#0a0a0c"},
        {"foreground", "#e6e6e6"},
        {"bright_foreground", "#f5f5f5"},
        {"red", "#f06a6a"},
        {"green", "#5bd6a0"},
        {"yellow", "#f2c94c"},
        {"blue", "#7aa2f7"},
        {"cyan", "#3fc9d6"},
        {"magenta", "#c792ea"},
        {"orange", "#f2914e"},
        {"selection", "#2a2a32"},
    };
}

QColor colorOr(const QVariantMap &palette, const QString &key, const QColor &fallback)
{
    const QColor color(palette.value(key).toString());
    return color.isValid() ? color : fallback;
}

QString hex(const QColor &color)
{
    return color.name(QColor::HexRgb);
}

QString alphaHex(const QColor &color, int alpha)
{
    QColor copy = color;
    copy.setAlpha(alpha);
    return copy.name(QColor::HexArgb);
}

QColor mix(const QColor &a, const QColor &b, qreal t)
{
    return QColor::fromRgbF(a.redF() * (1.0 - t) + b.redF() * t,
                            a.greenF() * (1.0 - t) + b.greenF() * t,
                            a.blueF() * (1.0 - t) + b.blueF() * t);
}

double luminance(const QColor &color)
{
    auto linear = [](double v) { return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4); };
    return .2126 * linear(color.redF()) + .7152 * linear(color.greenF()) + .0722 * linear(color.blueF());
}

} // namespace

Theme::Theme(const QString &directory, QObject *parent, const QString &hyprctl)
    : QObject(parent),
      m_directory(directory.isEmpty() ? QDir::homePath() + "/.local/state/omarchy/current" : directory) {
    m_roots = {
        QDir::homePath() + "/.config/omarchy/themes",
        qEnvironmentVariable("OMARCHY_PATH", "/usr/share/omarchy") + "/themes",
        QStringLiteral("/usr/share/omarchy/themes"),
    };
    m_roots.removeDuplicates();

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);

    // Corners follow Hyprland's effective rounding, which themes and personal
    // overrides can both set, so ask hyprctl rather than reading its config.
    m_hyprctl = !hyprctl.isEmpty() ? hyprctl
              : directory.isEmpty() && !qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")
                  ? QStandardPaths::findExecutable("hyprctl") : QString();
    m_roundingTimeout.setSingleShot(true);
    m_roundingTimeout.setInterval(500);
    connect(&m_roundingTimeout, &QTimer::timeout, &m_roundingProcess, &QProcess::kill);
    connect(&m_roundingProcess, &QProcess::errorOccurred, this, [this] { m_roundingTimeout.stop(); });
    connect(&m_roundingProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_roundingTimeout.stop();
        const auto value = QJsonDocument::fromJson(m_roundingProcess.readAllStandardOutput()).object().value("int");
        if (code != 0 || status != QProcess::NormalExit || !value.isDouble() || value.toInt(-1) < 0)
            return;
        if (value.toInt() != m_radius) {
            m_radius = value.toInt();
            emit changed();
        }
    });
    m_roundingPoll.setInterval(1000);
    connect(&m_roundingPoll, &QTimer::timeout, this, &Theme::refreshRounding);
    if (!m_hyprctl.isEmpty())
        m_roundingPoll.start();

    m_themes = scanThemes(m_roots);
    reload();
}

Theme::~Theme() {
    if (m_roundingProcess.state() != QProcess::NotRunning) {
        m_roundingProcess.kill();
        m_roundingProcess.waitForFinished(1000);
    }
}

void Theme::refreshRounding() {
    if (m_hyprctl.isEmpty() || m_roundingProcess.state() != QProcess::NotRunning)
        return;
    m_roundingProcess.start(m_hyprctl, {"-j", "getoption", "decoration:rounding"});
    m_roundingTimeout.start();
}

QVariantMap Theme::readPalette(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    static const QRegularExpression colorLine(R"re(^\s*([a-z_]+)\s*=\s*"(#[0-9a-fA-F]{3,8})"\s*$)re");
    static const QRegularExpression modeLine(R"re(^\s*mode\s*=\s*"([^"]+)"\s*$)re");

    QVariantMap palette;
    const QString text = QString::fromUtf8(file.readAll());
    for (const QString &line : text.split('\n')) {
        const auto colorMatch = colorLine.match(line);
        if (colorMatch.hasMatch()) {
            const QColor color(colorMatch.captured(2));
            if (color.isValid())
                palette.insert(colorMatch.captured(1), color.name(QColor::HexRgb));
            continue;
        }
        const auto modeMatch = modeLine.match(line);
        if (modeMatch.hasMatch())
            palette.insert("mode", modeMatch.captured(1).toLower());
    }
    if (!palette.contains("mode"))
        palette.insert("mode", "dark");
    return palette;
}

QString Theme::readAccent(const QString &path) {
    const QVariantMap palette = readPalette(path);
    return palette.value("accent", "#FFD60A").toString();
}

QString Theme::prettyName(const QString &id) {
    if (id.isEmpty())
        return QStringLiteral("Omarchy");
    QStringList words = id.split(QRegularExpression("[_-]"), Qt::SkipEmptyParts);
    for (QString &word : words) {
        if (!word.isEmpty())
            word[0] = word[0].toUpper();
    }
    return words.join(' ');
}

QVariantList Theme::scanThemes(const QStringList &roots) {
    QVariantList themes;
    QSet<QString> seen;

    for (const QString &root : roots) {
        QDir directory(root);
        const QStringList entries = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString &id : entries) {
            if (seen.contains(id))
                continue;
            const QString path = root + "/" + id + "/colors.toml";
            const QVariantMap palette = readPalette(path);
            if (palette.isEmpty())
                continue;
            seen.insert(id);

            const QColor accent = colorOr(palette, "accent", QColor("#FFD60A"));
            const QVariantMap fallbacks = fallbackPalette();
            QVariantMap item;
            item.insert("id", id);
            item.insert("label", prettyName(id));
            item.insert("mode", palette.value("mode", "dark"));
            item.insert("background", palette.value("background", fallbacks.value("background")));
            item.insert("foreground", palette.value("foreground", fallbacks.value("foreground")));
            item.insert("accent", hex(accent));
            item.insert("cyan", palette.value("cyan", fallbacks.value("cyan")));
            item.insert("red", palette.value("red", fallbacks.value("red")));
            item.insert("green", palette.value("green", fallbacks.value("green")));
            item.insert("yellow", palette.value("yellow", fallbacks.value("yellow")));
            item.insert("blue", palette.value("blue", fallbacks.value("blue")));
            item.insert("orange", palette.value("orange",
                        palette.value("bright_yellow", fallbacks.value("orange"))));
            themes.append(item);
        }
    }

    std::sort(themes.begin(), themes.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value("label").toString().localeAwareCompare(b.toMap().value("label").toString()) < 0;
    });
    return themes;
}

QString Theme::themePath(const QString &id) const {
    if (id.isEmpty())
        return {};
    for (const QString &root : m_roots) {
        const QString path = root + "/" + id + "/colors.toml";
        if (QFileInfo::exists(path))
            return path;
    }
    return {};
}

bool Theme::setSelectedTheme(const QString &id) {
    if (id == m_selected)
        return true;
    if (!id.isEmpty() && themePath(id).isEmpty())
        return false;
    m_selected = id;
    m_signature.clear();
    reload();
    emit selectionChanged();
    return true;
}

void Theme::refreshThemes() {
    const QVariantList next = scanThemes(m_roots);
    if (next == m_themes)
        return;
    m_themes = next;
    emit themesChanged();
}

void Theme::armWatcher(const QString &palettePath) {
    const QStringList existing = m_watcher.files() + m_watcher.directories();
    if (!existing.isEmpty())
        m_watcher.removePaths(existing);

    QStringList candidates{palettePath, QFileInfo(palettePath).absolutePath()};
    if (m_selected.isEmpty()) {
        // A theme switch replaces the state directory and its files, so watch
        // the parents too and re-arm on every reload.
        candidates << m_directory + "/theme"
                   << m_directory + "/theme.name"
                   << m_directory
                   << QFileInfo(m_directory).absolutePath();
        QString ancestor = m_directory;
        while (!QFileInfo::exists(ancestor) && ancestor != "/")
            ancestor = QFileInfo(ancestor).absolutePath();
        candidates << ancestor;
    }
    candidates.removeDuplicates();
    for (const QString &path : candidates)
        if (QFileInfo::exists(path))
            m_watcher.addPath(path);
}

void Theme::reload() {
    refreshRounding();

    QString palettePath;
    QString activeId;
    if (m_selected.isEmpty()) {
        palettePath = m_directory + "/theme/colors.toml";
        QFile nameFile(m_directory + "/theme.name");
        if (nameFile.open(QIODevice::ReadOnly))
            activeId = QString::fromUtf8(nameFile.readLine()).trimmed();
        if (activeId.isEmpty())
            activeId = QStringLiteral("omarchy");
    } else {
        palettePath = themePath(m_selected);
        activeId = m_selected;
        if (palettePath.isEmpty())
            palettePath = m_directory + "/theme/colors.toml";
    }

    QVariantMap palette = readPalette(palettePath);
    if (palette.isEmpty())
        palette = fallbackPalette();

    QString systemId;
    QFile systemName(m_directory + "/theme.name");
    if (systemName.open(QIODevice::ReadOnly))
        systemId = QString::fromUtf8(systemName.readLine()).trimmed();
    if (systemId.isEmpty())
        systemId = QStringLiteral("omarchy");
    m_systemId = systemId;
    m_systemLabel = prettyName(systemId);

    const QString signature = palettePath + "|" + systemId + "|" + QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(palette)).toJson(QJsonDocument::Compact));
    armWatcher(palettePath);
    if (signature == m_signature)
        return;

    m_signature = signature;
    m_name = activeId;
    m_label = prettyName(activeId);
    applyPalette(palette);
    emit changed();
}

void Theme::applyPalette(const QVariantMap &palette) {
    m_dark = palette.value("mode", "dark").toString() != "light";

    const QColor bg = colorOr(palette, "background", QColor("#0e0e10"));
    const QColor fg = colorOr(palette, "foreground", QColor("#e6e6e6"));
    const QColor accent = colorOr(palette, "accent", QColor("#FFD60A"));
    const QColor cyan = colorOr(palette, "cyan", mix(accent, bg, 0.35));
    const QColor green = colorOr(palette, "green", QColor("#5bd6a0"));
    const QColor yellow = colorOr(palette, "yellow", QColor("#f2c94c"));

    const qreal surfaceStep = m_dark ? 0.035 : 0.025;
    m_background = hex(bg);
    m_surface = hex(colorOr(palette, "dark_background", mix(bg, fg, surfaceStep)));
    m_raised = hex(mix(bg, fg, m_dark ? 0.06 : 0.045));
    m_raisedHover = hex(mix(bg, fg, m_dark ? 0.10 : 0.075));
    m_chip = hex(mix(bg, fg, m_dark ? 0.08 : 0.06));
    m_chipHover = hex(mix(bg, fg, m_dark ? 0.13 : 0.10));
    m_menuHover = hex(mix(bg, fg, 0.08));
    m_menuSelected = hex(mix(mix(bg, fg, 0.12), accent, m_dark ? 0.22 : 0.16));
    m_border = hex(mix(bg, fg, 0.15));
    m_borderStrong = hex(mix(bg, fg, 0.28));

    m_text = hex(fg);
    m_brightText = hex(colorOr(palette, "bright_foreground",
                               colorOr(palette, "light_foreground", fg)));
    // Light surfaces need more foreground weight to keep 9–10 px metadata
    // readable; dark surfaces can stay softer without losing contrast.
    m_mutedText = hex(mix(fg, bg, m_dark ? 0.38 : 0.12));
    m_dimText = hex(mix(fg, bg, m_dark ? 0.55 : 0.25));

    m_accent = hex(accent);
    m_accentForeground = luminance(accent) > .179 ? "black" : "white";
    m_accentSoft = alphaHex(accent, 30);
    m_selection = hex(colorOr(palette, "selection", mix(bg, fg, 0.15)));

    m_red = hex(colorOr(palette, "red", QColor("#f06a6a")));
    m_green = hex(green);
    m_yellow = hex(yellow);
    m_blue = hex(colorOr(palette, "blue", accent));
    m_cyan = hex(cyan);
    m_magenta = hex(colorOr(palette, "magenta", QColor("#c792ea")));
    m_orange = hex(colorOr(palette, "orange",
                           colorOr(palette, "bright_yellow", yellow)));

    m_ambientTop = hex(mix(bg, accent, m_dark ? 0.10 : 0.06));
    m_ambientBottom = hex(mix(bg, cyan, m_dark ? 0.05 : 0.035));
    m_shadow = alphaHex(QColor(0, 0, 0), m_dark ? 110 : 40);
}
