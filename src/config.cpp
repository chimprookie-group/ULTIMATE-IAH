#include "config.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
const QStringList kViews = {
    QStringLiteral("home"), QStringLiteral("ecosystem"), QStringLiteral("workspace"),
    QStringLiteral("harness"), QStringLiteral("terminal"),
    QStringLiteral("connections"),
    QStringLiteral("memory"), QStringLiteral("autopilot"), QStringLiteral("mcp"),
    QStringLiteral("agents"), QStringLiteral("installation"),
    QStringLiteral("agent"), QStringLiteral("bots"), QStringLiteral("settings")
};
}

Config::Config(const QString &path, QObject *parent)
    : QObject(parent), m_path(path.isEmpty() ? defaultPath() : path) {
    load();
}

QString Config::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/config.json";
}

QString Config::defaultWorkspace() {
    const QString work = QDir::homePath() + "/Work";
    return QFileInfo(work).isDir() ? work : QDir::homePath();
}

QString Config::detectBinary(const QString &name) {
    const QString found = QStandardPaths::findExecutable(name);
    return found.isEmpty() ? name : found;
}

QString Config::detectDsh() {
    // Prefer the system runtime shipped by the harness package so a fresh
    // install does not depend on the user's mise/npm state.
    const QString system = QStringLiteral("/usr/lib/crasy/dsh/node_modules/.bin/dsh");
    if (QFileInfo(system).isExecutable())
        return system;
    const QString path = QStandardPaths::findExecutable(QStringLiteral("dsh"));
    if (!path.isEmpty())
        return path;
    QDir mise(QDir::homePath() + "/.local/share/mise/installs");
    const QFileInfoList candidates = mise.entryInfoList(
        {QStringLiteral("npm-deepseek-ai-dsh*")}, QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &candidate : candidates) {
        const QDir versions(candidate.absoluteFilePath());
        for (const QFileInfo &version : versions.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            const QString binary = version.absoluteFilePath()
                + "/node_modules/.bin/dsh";
            if (QFileInfo(binary).isExecutable())
                return binary;
        }
    }
    const QString local = QDir::homePath() + "/.local/bin/dsh";
    if (QFileInfo(local).isExecutable())
        return local;
    return QStringLiteral("dsh");
}

bool Config::validView(const QString &view) {
    return kViews.contains(view);
}

void Config::load() {
    m_workspace = defaultWorkspace();
    m_harnessProfile = QStringLiteral("web");
    m_dshBinary = QStringLiteral("dsh");
    m_view = QStringLiteral("home");
    m_theme.clear();
    m_language.clear();
    m_startupView = QStringLiteral("home");
    m_autoStartHarness = true;

    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();

    const QString workspace = object.value("workspace").toString();
    if (!workspace.isEmpty() && QFileInfo(workspace).isDir())
        m_workspace = QFileInfo(workspace).absoluteFilePath();
    const QString profile = object.value("harnessProfile").toString();
    if (!profile.isEmpty())
        m_harnessProfile = profile;
    const QString dsh = object.value("dshBinary").toString();
    if (!dsh.isEmpty())
        m_dshBinary = dsh;
    const QString theme = object.value("theme").toString();
    if (!theme.isEmpty())
        m_theme = theme;
    const QString language = object.value("language").toString();
    if (language == "fr" || language == "en")
        m_language = language;
    bool needsMigration = false;
    const QString view = object.value("view").toString();
    if (validView(view))
        m_view = view;
    else if (!view.isEmpty())
        needsMigration = true;
    const QString start = object.value("startupView").toString();
    if (start == "last" || validView(start))
        m_startupView = start;
    else if (!start.isEmpty())
        needsMigration = true;
    if (object.contains("autoStartHarness"))
        m_autoStartHarness = object.value("autoStartHarness").toBool(true);

    if (QFileInfo(m_dshBinary).isAbsolute() && !QFileInfo::exists(m_dshBinary))
        m_dshBinary = QStringLiteral("dsh");
    if (needsMigration)
        save();
}

void Config::save() {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QJsonObject object;
    object["workspace"] = m_workspace;
    object["harnessProfile"] = m_harnessProfile;
    object["dshBinary"] = m_dshBinary;
    object["view"] = m_view;
    object["theme"] = m_theme;
    object["language"] = m_language;
    object["startupView"] = m_startupView;
    object["autoStartHarness"] = m_autoStartHarness;

    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    file.commit();
}

void Config::setWorkspace(const QString &workspace) {
    const QFileInfo info(workspace);
    if (!info.isDir())
        return;
    const QString absolute = info.absoluteFilePath();
    if (absolute == m_workspace)
        return;
    m_workspace = absolute;
    save();
    emit workspaceChanged();
}

void Config::setHarnessProfile(const QString &profile) {
    if (profile.trimmed().isEmpty() || profile == m_harnessProfile)
        return;
    m_harnessProfile = profile.trimmed();
    save();
    emit harnessProfileChanged();
}

void Config::setDshBinary(const QString &binary) {
    if (binary.trimmed().isEmpty() || binary == m_dshBinary)
        return;
    m_dshBinary = binary.trimmed();
    save();
    emit dshBinaryChanged();
}

void Config::setView(const QString &view) {
    if (!validView(view) || view == m_view)
        return;
    m_view = view;
    save();
    emit viewChanged();
}

void Config::setTheme(const QString &theme) {
    if (theme == m_theme)
        return;
    m_theme = theme;
    save();
    emit themeChanged();
}

void Config::setLanguage(const QString &language) {
    if (language != "" && language != "fr" && language != "en")
        return;
    if (language == m_language)
        return;
    m_language = language;
    save();
    emit languageChanged();
}

void Config::setStartupView(const QString &view) {
    if (view != "last" && !validView(view))
        return;
    if (view == m_startupView)
        return;
    m_startupView = view;
    save();
    emit optionsChanged();
}

void Config::setAutoStartHarness(bool enabled) {
    if (enabled == m_autoStartHarness)
        return;
    m_autoStartHarness = enabled;
    save();
    emit optionsChanged();
}
