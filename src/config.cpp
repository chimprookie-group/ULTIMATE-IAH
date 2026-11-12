#include "config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

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
    m_view = QStringLiteral("home");
    m_profiles.clear();
    m_activeProfileId.clear();

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

    const QJsonArray profiles = object.value("profiles").toArray();
    for (const QJsonValue &value : profiles) {
        const QJsonObject profile = value.toObject();
        const QString id = profile.value("id").toString();
        const QString name = profile.value("name").toString();
        const QString model = profile.value("model").toString();
        const QString mode = profile.value("mode").toString();
        if (id.isEmpty() || name.isEmpty() || model.isEmpty() || mode.isEmpty())
            continue;
        m_profiles.append(QVariantMap{{"id", id}, {"value", id}, {"name", name},
                                      {"model", model}, {"mode", mode},
                                      {"description", model + " · " + mode}});
    }
    m_activeProfileId = object.value("activeProfile").toString();
    if (!profileById(m_activeProfileId).isEmpty())
        m_activeProfileId = m_activeProfileId;
    else
        m_activeProfileId.clear();

    // Older builds stored versioned absolute paths; if that version is gone,
    // fall back to the name so an upgrade does not leave the app broken.
    if (QFileInfo(m_dshBinary).isAbsolute() && !QFileInfo::exists(m_dshBinary))
        m_dshBinary = QStringLiteral("dsh");
    if (QFileInfo(m_opencodeBinary).isAbsolute() && !QFileInfo::exists(m_opencodeBinary))
        m_opencodeBinary = QStringLiteral("opencode");

    const QString view = object.value("view").toString();
    if (view == "home" || view == "ecosystem" || view == "workspace" || view == "providers" || view == "ninerouter" || view == "mausbot"
        || view == "harness" || view == "opencode" || view == "split" || view == "terminal"
        || view == "settings")
        m_view = view;

    m_theme = object.value("theme").toString();
    m_language = object.value("language").toString();

    const QString startup = object.value("startupView").toString();
    if (startup == "last" || startup == "home" || startup == "ecosystem" || startup == "workspace"
        || startup == "harness" || startup == "opencode" || startup == "split" || startup == "terminal")
        m_startupView = startup;
    m_autoStartHarness = object.value("autoStartHarness").toBool(true);
    m_autoStartOpencode = object.value("autoStartOpencode").toBool(true);
    const double ratio = object.value("splitRatio").toDouble(0.57);
    if (ratio >= 0.45 && ratio <= 0.68)
        m_splitRatio = ratio;
    m_notifyOnFinish = object.value("notifyOnFinish").toBool(true);
}

void Config::save() {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QJsonObject object;
    object.insert("workspace", m_workspace);
    object.insert("harnessProfile", m_harnessProfile);
    object.insert("dshBinary", m_dshBinary);
    object.insert("opencodeBinary", m_opencodeBinary);
    object.insert("view", m_view);
    object.insert("theme", m_theme);
    object.insert("language", m_language);
    object.insert("startupView", m_startupView);
    object.insert("autoStartHarness", m_autoStartHarness);
    object.insert("autoStartOpencode", m_autoStartOpencode);
    object.insert("splitRatio", m_splitRatio);
    object.insert("notifyOnFinish", m_notifyOnFinish);
    QJsonArray profiles;
    for (const QVariant &value : m_profiles) {
        const QVariantMap profile = value.toMap();
        profiles.append(QJsonObject{{"id", profile.value("id").toString()},
                                    {"name", profile.value("name").toString()},
                                    {"model", profile.value("model").toString()},
                                    {"mode", profile.value("mode").toString()}});
    }
    object.insert("profiles", profiles);
    object.insert("activeProfile", m_activeProfileId);

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

QVariantMap Config::profileById(const QString &id) const {
    for (const QVariant &value : m_profiles) {
        const QVariantMap profile = value.toMap();
        if (profile.value("id").toString() == id)
            return profile;
    }
    return {};
}

QString Config::saveProfile(const QString &name, const QString &model, const QString &mode) {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || model.isEmpty() || mode.isEmpty())
        return {};
    QString id;
    for (int i = 0; i < m_profiles.size(); ++i) {
        QVariantMap profile = m_profiles.at(i).toMap();
        if (profile.value("name").toString().compare(trimmed, Qt::CaseInsensitive) == 0) {
            id = profile.value("id").toString();
            profile["model"] = model;
            profile["mode"] = mode;
            profile["description"] = model + " · " + mode;
            m_profiles[i] = profile;
            break;
        }
    }
    if (id.isEmpty()) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_profiles.append(QVariantMap{{"id", id}, {"value", id}, {"name", trimmed},
                                      {"model", model}, {"mode", mode},
                                      {"description", model + " · " + mode}});
    }
    m_activeProfileId = id;
    save();
    emit profilesChanged();
    return id;
}

void Config::removeProfile(const QString &id) {
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles.at(i).toMap().value("id").toString() == id) {
            m_profiles.removeAt(i);
            if (m_activeProfileId == id)
                m_activeProfileId.clear();
            save();
            emit profilesChanged();
            return;
        }
    }
}

void Config::setActiveProfile(const QString &id) {
    if (!id.isEmpty() && profileById(id).isEmpty())
        return;
    if (m_activeProfileId == id)
        return;
    m_activeProfileId = id;
    save();
    emit profilesChanged();
}

void Config::setStartupView(const QString &view) {
    if (view != "last" && view != "home" && view != "ecosystem" && view != "workspace"
        && view != "harness" && view != "opencode" && view != "split" && view != "terminal")
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

void Config::setAutoStartOpencode(bool enabled) {
    if (enabled == m_autoStartOpencode)
        return;
    m_autoStartOpencode = enabled;
    save();
    emit optionsChanged();
}

void Config::setSplitRatio(double ratio) {
    const double clamped = qBound(0.45, ratio, 0.68);
    if (qFuzzyCompare(clamped, m_splitRatio))
        return;
    m_splitRatio = clamped;
    save();
    emit optionsChanged();
}

void Config::setNotifyOnFinish(bool enabled) {
    if (enabled == m_notifyOnFinish)
        return;
    m_notifyOnFinish = enabled;
    save();
    emit optionsChanged();
}

void Config::setLanguage(const QString &language) {
    if (language == m_language)
        return;
    m_language = language;
    save();
    emit languageChanged();
}

void Config::setTheme(const QString &theme) {
    if (theme == m_theme)
        return;
    m_theme = theme;
    save();
    emit themeChanged();
}

void Config::setView(const QString &view) {
    if (view != "home" && view != "ecosystem" && view != "workspace" && view != "providers" && view != "ninerouter" && view != "mausbot"
        && view != "harness" && view != "opencode" && view != "split" && view != "terminal"
        && view != "settings")
        return;
    if (view == m_view)
        return;
    m_view = view;
    save();
    emit viewChanged();
}
