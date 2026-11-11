#include "config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

Config::Config(const QString &path, QObject *parent)
    : QObject(parent),
      m_path(path.isEmpty() ? defaultPath() : path) {
    load();
}

QString Config::defaultPath() {
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return directory + "/config.json";
}

QString Config::defaultWorkspace() {
    const QString home = QDir::homePath();
    const QString work = home + "/Work";
    return QFileInfo(work).isDir() ? work : home;
}

QString Config::detectBinary(const QString &name) {
    const QString found = QStandardPaths::findExecutable(name);
    return found.isEmpty() ? name : found;
}

void Config::load() {
    // Defaults are what a first launch gets. The stock web profile is the safe
    // choice: a personal profile can hold singleton plugins (the desktop
    // session's own agent, for instance) that refuse a second instance. The
    // profile stays editable in Settings for anyone who wants it.
    m_workspace = defaultWorkspace();
    m_harnessProfile = QStringLiteral("web");
    // Keep the command name, not its resolved path: mise and npm upgrades move
    // the real binary, and QProcess already searches PATH at launch. An
    // absolute path set in Settings is honored as-is.
    m_dshBinary = QStringLiteral("dsh");
    m_opencodeBinary = QStringLiteral("opencode");
    m_view = (!QStandardPaths::findExecutable("dsh").isEmpty()
              && !QStandardPaths::findExecutable("opencode").isEmpty()) ? "split" : "opencode";

    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject object = QJsonDocument::fromJson(file.readAll()).object();

    const QString workspace = object.value("workspace").toString();
    const QFileInfo info(workspace);
    if (!workspace.isEmpty() && info.isDir())
        m_workspace = info.absoluteFilePath();

    const QString profile = object.value("harnessProfile").toString();
    if (!profile.isEmpty())
        m_harnessProfile = profile;

    const QString dsh = object.value("dshBinary").toString();
    if (!dsh.isEmpty())
        m_dshBinary = dsh;

    const QString opencode = object.value("opencodeBinary").toString();
    if (!opencode.isEmpty())
        m_opencodeBinary = opencode;

    // Older builds stored versioned absolute paths; if that version is gone,
    // fall back to the name so an upgrade does not leave the app broken.
    if (QFileInfo(m_dshBinary).isAbsolute() && !QFileInfo::exists(m_dshBinary))
        m_dshBinary = QStringLiteral("dsh");
    if (QFileInfo(m_opencodeBinary).isAbsolute() && !QFileInfo::exists(m_opencodeBinary))
        m_opencodeBinary = QStringLiteral("opencode");

    const QString view = object.value("view").toString();
    if (view == "harness" || view == "opencode" || view == "split" || view == "settings")
        m_view = view;
}

void Config::save() {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QJsonObject object;
    object.insert("workspace", m_workspace);
    object.insert("harnessProfile", m_harnessProfile);
    object.insert("dshBinary", m_dshBinary);
    object.insert("opencodeBinary", m_opencodeBinary);
    object.insert("view", m_view);

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
    const QString trimmed = profile.trimmed();
    if (trimmed.isEmpty() || trimmed == m_harnessProfile)
        return;
    m_harnessProfile = trimmed;
    save();
    emit harnessProfileChanged();
}

void Config::setDshBinary(const QString &binary) {
    if (binary == m_dshBinary)
        return;
    m_dshBinary = binary;
    save();
    emit dshBinaryChanged();
}

void Config::setOpencodeBinary(const QString &binary) {
    if (binary == m_opencodeBinary)
        return;
    m_opencodeBinary = binary;
    save();
    emit opencodeBinaryChanged();
}

void Config::setView(const QString &view) {
    if (view != "harness" && view != "opencode" && view != "split" && view != "settings")
        return;
    if (view == m_view)
        return;
    m_view = view;
    save();
    emit viewChanged();
}
