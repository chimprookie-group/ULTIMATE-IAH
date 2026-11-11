#include "backend.h"

#include "acpclient.h"
#include "config.h"
#include "gitstatus.h"
#include "harnessserver.h"
#include "portalfolderpicker.h"
#include "theme.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QLocale>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QUuid>

namespace {

QString harnessStateName(HarnessServer::State state) {
    switch (state) {
    case HarnessServer::Starting: return "starting";
    case HarnessServer::Running: return "running";
    case HarnessServer::Failed: return "failed";
    case HarnessServer::Stopped: break;
    }
    return "stopped";
}

QString opencodeStateName(AcpClient::State state) {
    switch (state) {
    case AcpClient::Starting: return "starting";
    case AcpClient::Ready: return "ready";
    case AcpClient::Failed: return "failed";
    case AcpClient::Stopped: break;
    }
    return "stopped";
}

QString resolveBinary(const QString &binary) {
    if (binary.isEmpty())
        return {};
    const QFileInfo info(binary);
    if (info.isAbsolute())
        return info.isExecutable() ? binary : QString();
    return QStandardPaths::findExecutable(binary);
}

QString probeVersion(const QString &binary, const QStringList &arguments) {
    if (binary.isEmpty())
        return {};
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(binary, arguments);
    if (!process.waitForStarted(1500))
        return {};
    process.waitForFinished(2000);
    return QString::fromUtf8(process.readAll()).trimmed().split('\n').value(0).trimmed();
}

bool portalAvailable() {
    QDBusConnection bus = QDBusConnection::sessionBus();
    return bus.interface() && bus.interface()->isServiceRegistered(QStringLiteral("org.freedesktop.portal.Desktop"));
}

bool notificationsAvailable() {
    QDBusConnection bus = QDBusConnection::sessionBus();
    return bus.interface() && bus.interface()->isServiceRegistered(QStringLiteral("org.freedesktop.Notifications"));
}

bool executableExists(const QString &binary) {
    if (binary.isEmpty())
        return false;
    const QFileInfo info(binary);
    if (info.isAbsolute())
        return info.isExecutable();
    return !QStandardPaths::findExecutable(binary).isEmpty();
}

QVariantMap optionById(const QVariantList &options, const QString &id) {
    for (const auto &value : options) {
        const QVariantMap option = value.toMap();
        if (option.value("id").toString() == id)
            return option;
    }
    return {};
}

QVariantList choicesOf(const QVariantList &options, const QString &id) {
    return optionById(options, id).value("options").toList();
}

QString valueOf(const QVariantList &options, const QString &id) {
    return optionById(options, id).value("currentValue").toString();
}

QVariantMap message(const QString &role, const QString &text) {
    QVariantMap map;
    map["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    map["role"] = role;
    map["text"] = text;
    map["streaming"] = false;
    return map;
}

QVariantMap toolMessage(const QString &id, const QString &title, const QString &kind,
                        const QString &status, const QString &output, const QString &input = {}) {
    QVariantMap map;
    map["id"] = id;
    map["role"] = QStringLiteral("tool");
    map["toolId"] = id;
    map["title"] = title;
    map["kind"] = kind;
    map["status"] = status;
    map["output"] = output;
    if (!input.isEmpty())
        map["input"] = QVariantMap{{"command", input}};
    return map;
}

} // namespace

Backend::Backend(QObject *parent)
    : QObject(parent),
      m_config(new Config({}, this)),
      m_harness(new HarnessServer(this)),
      m_acp(new AcpClient(this)),
      m_portal(new PortalFolderPicker(this)),
      m_view(m_config->view()) {
    connect(m_harness, &HarnessServer::stateChanged, this, &Backend::harnessChanged);
    connect(m_harness, &HarnessServer::urlChanged, this, &Backend::harnessChanged);
    connect(m_harness, &HarnessServer::detailChanged, this, &Backend::harnessChanged);
    connect(m_harness, &HarnessServer::logChanged, this, &Backend::harnessChanged);
    connect(m_harness, &HarnessServer::profileChanged, this, &Backend::harnessProfileChanged);
    connect(m_harness, &HarnessServer::stateChanged, this, [this] {
        if (m_harness->state() != HarnessServer::Running && m_harnessHealth != "unknown") {
            m_harnessHealth = "unknown";
            emit harnessHealthChanged();
        }
    });
    connect(m_harness, &HarnessServer::urlChanged, this, [this] {
        m_harnessHealth = "unknown";
        emit harnessHealthChanged();
    });

    connect(m_acp, &AcpClient::stateChanged, this, &Backend::opencodeChanged);
    connect(m_acp, &AcpClient::detailChanged, this, &Backend::opencodeChanged);
    connect(m_acp, &AcpClient::messagesChanged, this, &Backend::messagesChanged);
    connect(m_acp, &AcpClient::configOptionsChanged, this, [this] {
        if (m_profileApplied)
            return;
        const QString id = m_config->activeProfileId();
        if (id.isEmpty())
            return;
        m_profileApplied = true;
        applyProfile(id);
    });
    connect(m_acp, &AcpClient::busyChanged, this, &Backend::busyChanged);
    connect(m_acp, &AcpClient::busyChanged, this, [this] {
        if (!m_acp->busy())
            refreshGit();
    });
    connect(m_acp, &AcpClient::sessionIdChanged, this, &Backend::sessionChanged);
    connect(m_acp, &AcpClient::sessionTitleChanged, this, &Backend::sessionChanged);
    connect(m_acp, &AcpClient::configOptionsChanged, this, &Backend::configOptionsChanged);
    connect(m_acp, &AcpClient::commandsChanged, this, &Backend::commandsChanged);
    connect(m_acp, &AcpClient::usageChanged, this, &Backend::usageChanged);
    connect(m_acp, &AcpClient::permissionChanged, this, &Backend::permissionChanged);
    connect(m_acp, &AcpClient::sessionsChanged, this, &Backend::sessionsChanged);
    connect(m_acp, &AcpClient::loadingSessionsChanged, this, &Backend::sessionsChanged);
    connect(m_acp, &AcpClient::ready, this, [this] { m_acp->newSession(workspace()); });

    connect(m_config, &Config::workspaceChanged, this, [this] {
        // One workspace drives both engines: the harness restarts so its
        // server works in the new directory, and OpenCode starts a session
        // rooted there.
        m_harness->setWorkingDirectory(workspace());
        if (m_harness->state() == HarnessServer::Running || m_harness->state() == HarnessServer::Starting)
            m_harness->restart();
        if (m_acp->state() == AcpClient::Ready)
            m_acp->newSession(workspace());
        refreshGit();
        emit workspaceChanged();
    });
    connect(m_config, &Config::harnessProfileChanged, this, [this] {
        m_harness->setProfile(m_config->harnessProfile());
        emit harnessProfileChanged();
    });
    connect(m_config, &Config::dshBinaryChanged, this, [this] {
        m_dshVersion.clear();
        m_harness->setBinary(m_config->dshBinary());
        restartHarness();
        emit dshBinaryChanged();
    });
    connect(m_config, &Config::opencodeBinaryChanged, this, [this] {
        m_opencodeVersion.clear();
        restartOpencode();
        emit opencodeBinaryChanged();
    });
    connect(m_config, &Config::optionsChanged, this, &Backend::optionsChanged);
    connect(m_config, &Config::profilesChanged, this, &Backend::profilesChanged);
    connect(m_config, &Config::viewChanged, this, [this] {
        if (m_config->view() == m_view)
            return;
        m_view = m_config->view();
        emit viewChanged();
    });

    connect(m_portal, &PortalFolderPicker::picked, this, &Backend::setWorkspace);
    connect(m_portal, &PortalFolderPicker::cancelled, this, [this](const QString &reason) {
        if (!reason.isEmpty())
            emit toast(reason);
    });
}

void Backend::attachTheme(Theme *theme) {
    m_theme = theme;
    if (!m_theme)
        return;
    connect(m_theme, &Theme::changed, this, &Backend::themeChanged);
    connect(m_theme, &Theme::themesChanged, this, &Backend::themeOptionsChanged);

    const QString saved = m_config->theme();
    if (!m_theme->setSelectedTheme(saved) && !saved.isEmpty())
        m_config->setTheme(QString());
}

QString Backend::themePreference() const {
    return m_config->theme();
}

QString Backend::languagePreference() const {
    return m_config->language();
}

void Backend::setLanguage(const QString &code) {
    if (code != "" && code != "fr" && code != "en")
        return;
    if (code == m_config->language())
        return;
    m_config->setLanguage(code);
    emit languageChanged();
}

QString Backend::activeTheme() const {
    return m_theme ? m_theme->label() : QStringLiteral("Omarchy");
}

QString Backend::systemTheme() const {
    return m_theme ? m_theme->systemTheme() : QStringLiteral("Omarchy");
}

QString Backend::systemThemeId() const {
    return m_theme ? m_theme->systemThemeId() : QStringLiteral("omarchy");
}

bool Backend::darkTheme() const {
    return m_theme ? m_theme->dark() : true;
}

QVariantList Backend::themeOptions() const {
    return m_theme ? m_theme->themes() : QVariantList{};
}

void Backend::setTheme(const QString &id) {
    if (!m_theme)
        return;
    if (!m_theme->setSelectedTheme(id))
        return;
    m_config->setTheme(m_theme->selectedTheme());
    emit themeChanged();
}

void Backend::refreshThemes() {
    if (m_theme)
        m_theme->refreshThemes();
}

void Backend::boot() {
    if (m_booted || m_demo)
        return;
    m_booted = true;
    if (m_config->autoStartHarness()) {
        m_harness->setBinary(dshBinary());
        m_harness->setProfile(harnessProfile());
        m_harness->setWorkingDirectory(workspace());
        m_harness->start();
    }
    if (m_config->autoStartOpencode())
        startOpencode();
    refreshGit();
}

void Backend::applyStartupView() {
    const QString start = m_config->startupView();
    if (start != "home" && start != "ecosystem" && start != "workspace" && start != "harness"
        && start != "opencode" && start != "split" && start != "terminal")
        return;
    if (start == m_view)
        return;
    m_view = start;
    emit viewChanged();
}

void Backend::startOpencode() {
    m_profileApplied = false;
    m_acp->start(opencodeBinary(), workspace());
}

QString Backend::workspace() const {
    return m_config->workspace();
}

QString Backend::workspaceName() const {
    const QFileInfo info(workspace());
    return info.fileName().isEmpty() ? workspace() : info.fileName();
}

QString Backend::appVersion() const {
    return QCoreApplication::applicationVersion();
}

QString Backend::monoFont() const {
    static const QString family = [] {
        const QStringList families = QFontDatabase::families();
        for (const auto &candidate : {"JetBrainsMono Nerd Font", "CaskaydiaMono Nerd Font",
                                      "JetBrains Mono", "Fira Code", "Source Code Pro",
                                      "DejaVu Sans Mono"}) {
            if (families.contains(QString::fromLatin1(candidate)))
                return QString::fromLatin1(candidate);
        }
        return QStringLiteral("monospace");
    }();
    return family;
}

QString Backend::uiFont() const {
    static const QString family = [] {
        const QStringList families = QFontDatabase::families();
        for (const auto &candidate : {"Inter", "Cantarell", "Noto Sans", "DejaVu Sans"}) {
            if (families.contains(QString::fromLatin1(candidate)))
                return QString::fromLatin1(candidate);
        }
        return QString();
    }();
    return family;
}

QString Backend::harnessState() const {
    return m_demo ? QStringLiteral("running") : harnessStateName(m_harness->state());
}

QString Backend::harnessUrl() const {
    return m_demo ? QString() : m_harness->url();
}

QString Backend::harnessDetail() const {
    return m_demo ? tr("Serving on 127.0.0.1:43110") : m_harness->detail();
}

QString Backend::harnessProfile() const {
    return m_demo ? QStringLiteral("web") : m_config->harnessProfile();
}

QString Backend::harnessLog() const {
    return m_harness->logTail();
}

QString Backend::dshBinary() const {
    return m_config->dshBinary();
}

bool Backend::dshAvailable() const {
    return executableExists(m_config->dshBinary());
}

QString Backend::opencodeState() const {
    return m_demo ? QStringLiteral("ready") : opencodeStateName(m_acp->state());
}

QString Backend::opencodeDetail() const {
    return m_demo ? tr("OpenCode 2.2.7") : m_acp->detail();
}

QString Backend::opencodeBinary() const {
    return m_config->opencodeBinary();
}

bool Backend::opencodeAvailable() const {
    return executableExists(m_config->opencodeBinary());
}

QVariantList Backend::messages() const {
    return m_demo ? m_demoMessages : m_acp->messages();
}

QAbstractListModel *Backend::messageModel() const {
    return m_acp->messageModel();
}

int Backend::messageCount() const {
    return m_demo ? m_demoMessages.size() : m_acp->messageCount();
}

bool Backend::busy() const {
    return m_demo ? false : m_acp->busy();
}

QString Backend::sessionId() const {
    return m_demo ? QStringLiteral("demo-session") : m_acp->sessionId();
}

QString Backend::sessionTitle() const {
    return m_demo ? m_demoSessionTitle : m_acp->sessionTitle();
}

QVariantList Backend::modelOptions() const {
    return m_demo ? m_demoModelOptions : choicesOf(m_acp->configOptions(), "model");
}

QVariantList Backend::modeOptions() const {
    return m_demo ? m_demoModeOptions : choicesOf(m_acp->configOptions(), "mode");
}

QString Backend::currentModel() const {
    return m_demo ? QStringLiteral("openrouter/deepseek-v4.1") : valueOf(m_acp->configOptions(), "model");
}

QString Backend::currentMode() const {
    return m_demo ? QStringLiteral("build") : valueOf(m_acp->configOptions(), "mode");
}

QVariantList Backend::commands() const {
    return m_acp->commands();
}

QVariantMap Backend::usage() const {
    return m_demo ? m_demoUsage : m_acp->usage();
}

QVariantMap Backend::permission() const {
    return m_acp->permission();
}

QVariantList Backend::sessions() const {
    return m_demo ? m_demoSessions : m_acp->sessions();
}

QString Backend::startupView() const {
    return m_config->startupView();
}

bool Backend::autoStartHarness() const {
    return m_config->autoStartHarness();
}

bool Backend::autoStartOpencode() const {
    return m_config->autoStartOpencode();
}

double Backend::splitRatio() const {
    return m_config->splitRatio();
}

bool Backend::notifyOnFinish() const {
    return m_config->notifyOnFinish();
}

QString Backend::harnessAddress() const {
    if (m_demo)
        return QStringLiteral("127.0.0.1:43110");
    const QUrl url(m_harness->url());
    return url.authority().isEmpty() ? tr("offline") : url.authority();
}

void Backend::setStartupView(const QString &view) {
    m_config->setStartupView(view);
}

QVariantList Backend::profiles() const {
    return m_config->profiles();
}

QString Backend::activeProfileId() const {
    return m_config->activeProfileId();
}

QString Backend::saveProfile(const QString &name) {
    if (name.trimmed().isEmpty()) {
        emit toast(tr("Give the profile a name first."));
        return {};
    }
    if (currentModel().isEmpty() || currentMode().isEmpty()) {
        emit toast(tr("OpenCode has not published a model or mode yet."));
        return {};
    }
    const QString id = m_config->saveProfile(name, currentModel(), currentMode());
    if (!id.isEmpty())
        emit toast(tr("Profile saved."));
    return id;
}

void Backend::applyProfile(const QString &id) {
    const QVariantMap profile = m_config->profileById(id);
    if (profile.isEmpty())
        return;
    m_config->setActiveProfile(id);
    if (!profile.value("model").toString().isEmpty())
        setConfigOption(QStringLiteral("model"), profile.value("model").toString());
    if (!profile.value("mode").toString().isEmpty())
        setConfigOption(QStringLiteral("mode"), profile.value("mode").toString());
    if (m_acp->state() != AcpClient::Ready)
        emit toast(tr("The profile will apply when OpenCode is ready."));
}

void Backend::deleteProfile(const QString &id) {
    m_config->removeProfile(id);
}

void Backend::setAutoStartHarness(bool enabled) {
    m_config->setAutoStartHarness(enabled);
}

void Backend::setAutoStartOpencode(bool enabled) {
    m_config->setAutoStartOpencode(enabled);
}

void Backend::setSplitRatio(double ratio) {
    m_config->setSplitRatio(ratio);
}

void Backend::setNotifyOnFinish(bool enabled) {
    m_config->setNotifyOnFinish(enabled);
}

void Backend::refreshGit() {
    if (m_gitProcess && m_gitProcess->state() != QProcess::NotRunning)
        return;

    const QString git = QStandardPaths::findExecutable(QStringLiteral("git"));
    if (git.isEmpty()) {
        m_gitAvailable = false;
        m_gitBranch.clear();
        m_gitDirty = 0;
        emit gitChanged();
        return;
    }

    if (!m_gitProcess) {
        m_gitProcess = new QProcess(this);
        connect(m_gitProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
            if (code != 0 || status != QProcess::NormalExit) {
                m_gitAvailable = false;
                m_gitBranch.clear();
                m_gitDirty = 0;
                emit gitChanged();
                return;
            }
            const QVariantMap result = GitStatus::parse(QString::fromUtf8(m_gitProcess->readAllStandardOutput()));
            m_gitAvailable = true;
            m_gitBranch = result.value("branch").toString();
            m_gitDirty = result.value("dirty").toInt();
            emit gitChanged();
        });
        connect(m_gitProcess, &QProcess::errorOccurred, this, [this] {
            m_gitAvailable = false;
            m_gitBranch.clear();
            m_gitDirty = 0;
            emit gitChanged();
        });
    }

    m_gitProcess->setProgram(git);
    m_gitProcess->setArguments({"-C", workspace(), "status", "--porcelain=v1", "-b"});
    m_gitProcess->start();
}

void Backend::runDiagnostics() {
    if (m_diagnosticsRunning)
        return;
    m_diagnosticsRunning = true;
    emit diagnosticsChanged();

    QVariantList checks;
    const auto add = [&checks](const QString &label, bool ok, const QString &detail) {
        checks.append(QVariantMap{{"label", label}, {"ok", ok}, {"detail", detail}});
    };

    const QString dsh = resolveBinary(dshBinary());
    if (m_dshVersion.isEmpty() && !dsh.isEmpty())
        m_dshVersion = probeVersion(dsh, {"--version"});
    add(tr("DeepSeek Harness"), !dsh.isEmpty(),
        dsh.isEmpty() ? tr("not found in PATH")
                      : (m_dshVersion.isEmpty() ? dsh : m_dshVersion + " · " + dsh));

    const QString opencode = resolveBinary(opencodeBinary());
    if (m_opencodeVersion.isEmpty() && !opencode.isEmpty())
        m_opencodeVersion = probeVersion(opencode, {"--version"});
    add(tr("OpenCode"), !opencode.isEmpty(),
        opencode.isEmpty() ? tr("not found in PATH")
                           : (m_opencodeVersion.isEmpty() ? opencode : m_opencodeVersion + " · " + opencode));

    add(tr("Harness server"), m_demo || m_harness->state() == HarnessServer::Running,
        m_demo || m_harness->state() == HarnessServer::Running ? harnessAddress() : m_harness->detail());

    add(tr("OpenCode session"), m_demo || m_acp->state() == AcpClient::Ready,
        m_demo || m_acp->state() == AcpClient::Ready
            ? tr("%n model(s) available", "", modelOptions().size())
            : m_acp->detail());

    add(tr("Desktop portal"), portalAvailable(),
        portalAvailable() ? tr("File chooser available") : tr("xdg-desktop-portal is not running"));

    const bool webengine = QFileInfo::exists(QStringLiteral("/usr/lib/qt6/qml/QtWebEngine"));
    add(tr("QtWebEngine"), webengine,
        webengine ? tr("QML module installed") : tr("qt6-webengine is missing"));

    const bool writable = QFileInfo(workspace()).isWritable();
    add(tr("Workspace"), writable,
        writable ? workspace() : tr("%1 is not writable").arg(workspace()));

    add(tr("Git"), m_demo || m_gitAvailable,
        m_demo ? tr("main · 3 changes")
               : (m_gitAvailable ? (m_gitBranch.isEmpty() ? tr("repository") : m_gitBranch)
                                 : tr("not a git repository")));

    add(tr("Notifications"), notificationsAvailable(),
        notificationsAvailable() ? tr("Desktop notifications available")
                                 : tr("notification service is not running"));

    m_diagnostics = checks;
    m_diagnosticsRunning = false;
    emit diagnosticsChanged();
}

void Backend::testHarness() {
    if (m_harness->url().isEmpty()) {
        m_harnessHealth = "offline";
        emit harnessHealthChanged();
        return;
    }
    if (m_healthReply) {
        m_healthReply->abort();
        m_healthReply->deleteLater();
        m_healthReply = nullptr;
    }

    m_harnessHealth = "checking";
    emit harnessHealthChanged();

    QNetworkRequest request{QUrl(m_harness->url())};
    request.setTransferTimeout(5000);
    m_healthReply = m_network.get(request);
    connect(m_healthReply, &QNetworkReply::finished, this, [this] {
        const int code = m_healthReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_harnessHealth = (code >= 200 && code < 400) ? "ok" : "failed";
        m_healthReply->deleteLater();
        m_healthReply = nullptr;
        emit harnessHealthChanged();
    });
}

QString Backend::sessionLabel(const QVariantMap &session) const {
    QString title = session.value("title").toString().trimmed();
    if (title.startsWith("New session - ") || title.startsWith("Nouvelle session - ")
        || title == "New session" || title == "Nouvelle session") {
        title.clear();
    }
    if (!title.isEmpty())
        return title;
    const QDateTime updated = QDateTime::fromString(session.value("updatedAt").toString(), Qt::ISODate);
    if (updated.isValid())
        return tr("Session of %1").arg(QLocale().toString(updated, "d MMM · HH:mm"));
    return tr("Untitled session");
}

void Backend::notify(const QString &title, const QString &body) {
    if (!notificationsAvailable())
        return;
    QDBusMessage message = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("Notify"));
    message << QStringLiteral("DeepCrasy H")
            << 0u
            << QStringLiteral("deepcrasy-h")
            << title
            << body
            << QStringList()
            << QVariantMap{{"desktop-entry", "deepcrasy-h"}}
            << 5000;
    QDBusConnection::sessionBus().send(message);
}

bool Backend::loadingSessions() const {
    return m_acp->loadingSessions();
}

void Backend::setView(const QString &view) {
    if (view != "home" && view != "ecosystem" && view != "workspace" && view != "harness"
        && view != "opencode" && view != "split" && view != "terminal" && view != "settings")
        return;
    if (view == m_view)
        return;
    m_view = view;
    m_config->setView(view);
    emit viewChanged();
}

void Backend::setWorkspace(const QString &workspace) {
    if (!QFileInfo(workspace).isDir()) {
        emit toast(tr("That folder does not exist."));
        return;
    }
    m_config->setWorkspace(workspace);
}

void Backend::pickWorkspace() {
    m_portal->open(tr("Choose workspace"));
}

void Backend::restartHarness() {
    m_harness->setBinary(dshBinary());
    m_harness->restart();
}

void Backend::restartOpencode() {
    startOpencode();
}

void Backend::setHarnessProfile(const QString &profile) {
    m_config->setHarnessProfile(profile);
}

void Backend::setOpencodeBinary(const QString &binary) {
    if (binary.trimmed().isEmpty())
        return;
    m_config->setOpencodeBinary(binary.trimmed());
}

void Backend::setDshBinary(const QString &binary) {
    if (binary.trimmed().isEmpty())
        return;
    m_config->setDshBinary(binary.trimmed());
}

void Backend::sendPrompt(const QString &text) {
    if (m_demo)
        return;
    m_acp->prompt(text);
}

void Backend::cancelPrompt() {
    m_acp->cancel();
}

void Backend::newSession() {
    if (m_demo)
        return;
    if (m_acp->state() != AcpClient::Ready) {
        emit toast(tr("OpenCode is not ready yet."));
        return;
    }
    m_profileApplied = false;
    m_acp->newSession(workspace());
}

void Backend::refreshSessions() {
    if (m_demo)
        return;
    if (m_acp->state() != AcpClient::Ready) {
        emit toast(tr("OpenCode is not ready yet."));
        return;
    }
    m_acp->refreshSessions(workspace());
}

void Backend::loadSession(const QString &id) {
    if (m_demo)
        return;
    m_profileApplied = false;
    m_acp->loadSession(id, workspace());
}

void Backend::setConfigOption(const QString &id, const QString &value) {
    if (m_demo)
        return;
    m_acp->setConfigOption(id, value);
}

void Backend::respondPermission(const QString &optionId) {
    m_acp->respondPermission(optionId);
}

void Backend::loadDemo() {
    m_demo = true;
    m_view = "opencode";
    buildDemo();
}

void Backend::buildDemo() {
    m_demoSessionTitle = QStringLiteral("Réessayer le helper fetch");

    m_demoMessages = {
        message("user", QStringLiteral("Ajoute un retry avec backoff à fetchJson() dans src/net/fetch.ts, puis lance la suite de tests.")),
        message("assistant", QStringLiteral(
            "Je lis d'abord le helper et ses tests.\n\n"
            "`fetchJson()` entoure `fetch` mais ne réessaie que sur un HTTP 429. Les erreurs "
            "réseau remontent directement, d'où l'échec du téléversement en Wi-Fi instable.")),
        toolMessage(QStringLiteral("tool-1"), QStringLiteral("src/net/fetch.ts"), QStringLiteral("read"),
                    QStringLiteral("completed"),
                    QStringLiteral(" 1  export async function fetchJson(url: string, init?: RequestInit) {\n"
                                   " 2    const response = await fetch(url, init)\n"
                                   " 3    if (response.status === 429) return retry(url, init)\n"
                                   " 4    if (!response.ok) throw new HttpError(response)\n"
                                   " 5    return response.json()\n"
                                   " 6  }")),
        message("assistant", QStringLiteral(
            "J'extrais un petit helper `withRetry` autour de l'appel fetch, avec deux tentatives "
            "et un backoff exponentiel sur les erreurs réseau et les 5xx, puis je relance la "
            "dernière erreur.")),
        toolMessage(QStringLiteral("tool-2"), QStringLiteral("src/net/fetch.ts"), QStringLiteral("edit"),
                    QStringLiteral("completed"),
                    QStringLiteral("Mis à jour src/net/fetch.ts (+18 −3)\n\n"
                                   "+ async function withRetry<T>(run: () => Promise<T>, attempts = 2) {\n"
                                   "+   for (let i = 0; ; i++) {\n"
                                   "+     try { return await run() }\n"
                                   "+     catch (error) {\n"
                                   "+       if (i >= attempts || !isRetryable(error)) throw error\n"
                                   "+       await sleep(250 * 2 ** i)\n"
                                   "+     }\n"
                                   "+   }\n"
                                   "+ }")),
        toolMessage(QStringLiteral("tool-3"), QStringLiteral("bun test src/net"), QStringLiteral("execute"),
                    QStringLiteral("completed"),
                    QStringLiteral("12 réussis, 0 échec\n12 tests exécutés dans 3 fichiers. [42ms]"),
                    QStringLiteral("bun test src/net")),
        message("assistant", QStringLiteral(
            "C'est fait. `fetchJson()` réessaie maintenant deux fois sur les erreurs réseau et les "
            "5xx, avec 250 ms puis 750 ms de backoff, et la suite passe toujours.\n\n"
            "- ajout de `withRetry()` dans `src/net/fetch.ts`\n"
            "- couverture du cas 500 → succès et des tentatives épuisées\n"
            "- 12 tests passent, rien d'autre touché")),
    };

    m_demoModelOptions = {
        QVariantMap{{"value", "openrouter/deepseek-v4.1"}, {"name", "DeepSeek V4.1"}},
        QVariantMap{{"value", "anthropic/claude-sonnet-4.6"}, {"name", "Claude Sonnet 4.6"}},
        QVariantMap{{"value", "openai/gpt-5.3-codex"}, {"name", "GPT-5.3 Codex"}},
    };
    m_demoModeOptions = {
        QVariantMap{{"value", "build"}, {"name", "Build"}},
        QVariantMap{{"value", "plan"}, {"name", "Plan"}},
    };
    const QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_demoSessions = {
        QVariantMap{{"sessionId", "demo-session-1"}, {"title", m_demoSessionTitle}, {"updatedAt", now}},
        QVariantMap{{"sessionId", "demo-session-2"}, {"title", QStringLiteral("Refonte du thème et des couleurs")},
                    {"updatedAt", QDateTime::currentDateTime().addDays(-1).toString(Qt::ISODate)}},
        QVariantMap{{"sessionId", "demo-session-3"}, {"title", QStringLiteral("Tests du connecteur Git")},
                    {"updatedAt", QDateTime::currentDateTime().addDays(-3).toString(Qt::ISODate)}},
    };

    m_demoUsage = QVariantMap{
        {"used", 18432},
        {"size", 200000},
        {"cost", QVariantMap{{"amount", 0.0}, {"currency", "USD"}}},
    };

    m_acp->setConversation(m_demoMessages);
}
