#include "backend.h"

#include "config.h"
#include "gitstatus.h"
#include "harnessserver.h"
#include "portalfolderpicker.h"
#include "theme.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

namespace {
QString resolveBinary(const QString &binary) {
    if (binary.isEmpty())
        return {};
    const QFileInfo info(binary);
    if (info.isAbsolute())
        return info.isExecutable() ? binary : QString();
    return QStandardPaths::findExecutable(binary);
}

QString probeVersion(const QString &binary) {
    if (binary.isEmpty())
        return {};
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(binary, {QStringLiteral("--version")});
    if (!process.waitForStarted(1500))
        return {};
    process.waitForFinished(2500);
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
} // namespace

Backend::Backend(QObject *parent)
    : QObject(parent),
      m_config(new Config({}, this)),
      m_harness(new HarnessServer(this)),
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

    connect(m_config, &Config::workspaceChanged, this, [this] {
        m_harness->setWorkingDirectory(workspace());
        if (m_harness->state() == HarnessServer::Running || m_harness->state() == HarnessServer::Starting)
            m_harness->restart();
        refreshGit();
        emit workspaceChanged();
    });
    connect(m_config, &Config::harnessProfileChanged, this, [this] {
        m_harness->setProfile(m_config->harnessProfile());
        emit harnessProfileChanged();
    });
    connect(m_config, &Config::dshBinaryChanged, this, [this] {
        m_harness->setBinary(m_config->dshBinary());
        restartHarness();
        emit dshBinaryChanged();
    });
    connect(m_config, &Config::optionsChanged, this, &Backend::optionsChanged);
    connect(m_config, &Config::viewChanged, this, [this] {
        if (m_config->view() == m_view)
            return;
        m_view = m_config->view();
        emit viewChanged();
    });
    connect(m_config, &Config::themeChanged, this, &Backend::themeChanged);
    connect(m_config, &Config::languageChanged, this, &Backend::languageChanged);

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
    refreshGit();
}

void Backend::loadDemo() {
    m_demo = true;
    m_view = "home";
}

void Backend::applyStartupView() {
    const QString start = m_config->startupView();
    if (start != "home" && start != "ecosystem" && start != "workspace" && start != "harness"
        && start != "terminal"
        && start != "connections" && start != "memory" && start != "autopilot"
        && start != "mcp" && start != "agents" && start != "installation"
        && start != "agent" && start != "settings")
        return;
    if (start == m_view)
        return;
    m_view = start;
    emit viewChanged();
}

QString Backend::themePreference() const { return m_config->theme(); }
QString Backend::languagePreference() const { return m_config->language(); }

void Backend::setLanguage(const QString &code) {
    if (code != "" && code != "fr" && code != "en")
        return;
    if (code == m_config->language())
        return;
    m_config->setLanguage(code);
    emit languageChanged();
}

QString Backend::activeTheme() const { return m_theme ? m_theme->label() : QStringLiteral("Omarchy"); }
QString Backend::systemTheme() const { return m_theme ? m_theme->systemTheme() : QStringLiteral("Omarchy"); }
QString Backend::systemThemeId() const { return m_theme ? m_theme->systemThemeId() : QStringLiteral("omarchy"); }
bool Backend::darkTheme() const { return m_theme ? m_theme->dark() : true; }
QVariantList Backend::themeOptions() const { return m_theme ? m_theme->themes() : QVariantList{}; }

void Backend::setTheme(const QString &id) {
    if (!m_theme || !m_theme->setSelectedTheme(id))
        return;
    m_config->setTheme(m_theme->selectedTheme());
    emit themeChanged();
}

void Backend::refreshThemes() {
    if (m_theme)
        m_theme->refreshThemes();
}

QString Backend::startupView() const { return m_config->startupView(); }
bool Backend::autoStartHarness() const { return m_config->autoStartHarness(); }

void Backend::setStartupView(const QString &view) {
    if (view != "home" && view != "harness" && view != "terminal" && view != "ecosystem"
        && view != "workspace"
        && view != "connections" && view != "memory" && view != "autopilot"
        && view != "mcp" && view != "agents" && view != "installation"
        && view != "agent" && view != "settings")
        return;
    m_config->setStartupView(view);
    emit optionsChanged();
}

void Backend::setAutoStartHarness(bool enabled) {
    m_config->setAutoStartHarness(enabled);
    emit optionsChanged();
}

void Backend::setView(const QString &view) {
    if (view != "home" && view != "ecosystem" && view != "workspace"
        && view != "harness" && view != "terminal" && view != "connections"
        && view != "memory" && view != "autopilot" && view != "mcp"
        && view != "agents" && view != "installation" && view != "agent"
        && view != "settings")
        return;
    if (view == m_view)
        return;
    m_view = view;
    m_config->setView(view);
    emit viewChanged();
}

QString Backend::workspace() const { return m_config->workspace(); }

QString Backend::workspaceName() const {
    const QFileInfo info(workspace());
    return info.fileName().isEmpty() ? workspace() : info.fileName();
}

QString Backend::appVersion() const { return QCoreApplication::applicationVersion(); }

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
    if (m_demo)
        return QStringLiteral("running");
    return harnessStateName();
}

QString Backend::harnessStateName() const {
    switch (m_harness->state()) {
    case HarnessServer::Starting: return QStringLiteral("starting");
    case HarnessServer::Running: return QStringLiteral("running");
    case HarnessServer::Failed: return QStringLiteral("failed");
    case HarnessServer::Stopped: break;
    }
    return QStringLiteral("stopped");
}

QString Backend::harnessUrl() const { return m_demo ? QString() : m_harness->url(); }
QString Backend::harnessDetail() const { return m_demo ? tr("Serving on 127.0.0.1:43110") : m_harness->detail(); }
QString Backend::harnessProfile() const { return m_demo ? QStringLiteral("web") : m_config->harnessProfile(); }
QString Backend::harnessLog() const { return m_harness->logTail(); }
QString Backend::dshBinary() const {
    const QString configured = m_config->dshBinary();
    if (configured.isEmpty() || configured == QLatin1String("dsh"))
        return Config::detectDsh();
    const QString resolved = resolveBinary(configured);
    return resolved.isEmpty() ? Config::detectDsh() : resolved;
}

bool Backend::dshAvailable() const { return !resolveBinary(dshBinary()).isEmpty(); }

QString Backend::harnessAddress() const {
    const QString url = harnessUrl();
    return url.isEmpty() ? tr("not running") : QUrl(url).authority();
}

void Backend::setDshBinary(const QString &binary) {
    if (binary.trimmed().isEmpty())
        return;
    m_config->setDshBinary(binary.trimmed());
    emit dshBinaryChanged();
}

void Backend::setHarnessProfile(const QString &profile) {
    if (profile.trimmed().isEmpty())
        return;
    m_config->setHarnessProfile(profile.trimmed());
}

void Backend::restartHarness() {
    m_harness->setBinary(dshBinary());
    m_harness->setProfile(harnessProfile());
    m_harness->setWorkingDirectory(workspace());
    m_harness->restart();
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

void Backend::refreshGit() {
    refreshGitNow();
}

void Backend::refreshGitNow() {
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
    const QString version = probeVersion(dsh);
    add(tr("DeepSeek Harness"), !dsh.isEmpty(),
        dsh.isEmpty() ? tr("not found in PATH") : (version.isEmpty() ? dsh : version + " · " + dsh));
    add(tr("Harness server"), m_demo || m_harness->state() == HarnessServer::Running,
        m_demo || m_harness->state() == HarnessServer::Running ? harnessAddress() : m_harness->detail());
    add(tr("Desktop portal"), portalAvailable(),
        portalAvailable() ? tr("File chooser available") : tr("xdg-desktop-portal is not running"));
    const bool webengine = QFileInfo::exists(QStringLiteral("/usr/lib/qt6/qml/QtWebEngine"));
    add(tr("QtWebEngine"), webengine, webengine ? tr("QML module installed") : tr("qt6-webengine is missing"));
    add(tr("Workspace"), QFileInfo(workspace()).isWritable(),
        QFileInfo(workspace()).isWritable() ? workspace() : tr("%1 is not writable").arg(workspace()));
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
