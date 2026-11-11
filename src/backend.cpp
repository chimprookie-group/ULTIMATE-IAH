#include "backend.h"

#include "acpclient.h"
#include "config.h"
#include "harnessserver.h"
#include "portalfolderpicker.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QStandardPaths>
#include <QTimer>
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

    connect(m_acp, &AcpClient::stateChanged, this, &Backend::opencodeChanged);
    connect(m_acp, &AcpClient::detailChanged, this, &Backend::opencodeChanged);
    connect(m_acp, &AcpClient::messagesChanged, this, &Backend::messagesChanged);
    connect(m_acp, &AcpClient::busyChanged, this, &Backend::busyChanged);
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
    connect(m_config, &Config::opencodeBinaryChanged, this, [this] {
        restartOpencode();
        emit opencodeBinaryChanged();
    });
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

void Backend::boot() {
    if (m_booted || m_demo)
        return;
    m_booted = true;
    m_harness->setBinary(dshBinary());
    m_harness->setProfile(harnessProfile());
    m_harness->setWorkingDirectory(workspace());
    m_harness->start();
    startOpencode();
}

void Backend::startOpencode() {
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
    return m_acp->sessions();
}

bool Backend::loadingSessions() const {
    return m_acp->loadingSessions();
}

void Backend::setView(const QString &view) {
    if (view != "harness" && view != "opencode" && view != "split" && view != "settings")
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
    if (m_acp->state() != AcpClient::Ready) {
        emit toast(tr("OpenCode is not ready yet."));
        return;
    }
    m_acp->newSession(workspace());
}

void Backend::refreshSessions() {
    if (m_acp->state() != AcpClient::Ready) {
        emit toast(tr("OpenCode is not ready yet."));
        return;
    }
    m_acp->refreshSessions(workspace());
}

void Backend::loadSession(const QString &id) {
    m_acp->loadSession(id, workspace());
}

void Backend::setConfigOption(const QString &id, const QString &value) {
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
    m_demoSessionTitle = QStringLiteral("Retry the fetch helper");

    m_demoMessages = {
        message("user", QStringLiteral("Add a retry with backoff to fetchJson() in src/net/fetch.ts, then run the test suite.")),
        message("assistant", QStringLiteral(
            "I'll read the helper and its tests first.\n\n"
            "`fetchJson()` wraps `fetch` but only retries on HTTP 429. Network errors are thrown "
            "straight through, which is why a flaky Wi-Fi connection fails the upload path.")),
        toolMessage(QStringLiteral("tool-1"), QStringLiteral("src/net/fetch.ts"), QStringLiteral("read"),
                    QStringLiteral("completed"),
                    QStringLiteral(" 1  export async function fetchJson(url: string, init?: RequestInit) {\n"
                                   " 2    const response = await fetch(url, init)\n"
                                   " 3    if (response.status === 429) return retry(url, init)\n"
                                   " 4    if (!response.ok) throw new HttpError(response)\n"
                                   " 5    return response.json()\n"
                                   " 6  }")),
        message("assistant", QStringLiteral(
            "I'll extract a small `withRetry` helper around the fetch call, retrying twice with "
            "exponential backoff on network errors and 5xx responses, then rethrowing the last error.")),
        toolMessage(QStringLiteral("tool-2"), QStringLiteral("src/net/fetch.ts"), QStringLiteral("edit"),
                    QStringLiteral("completed"),
                    QStringLiteral("Updated src/net/fetch.ts (+18 −3)\n\n"
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
                    QStringLiteral("12 pass, 0 fail\nRan 12 tests across 3 files. [42ms]"),
                    QStringLiteral("bun test src/net")),
        message("assistant", QStringLiteral(
            "Done. `fetchJson()` now retries twice on network errors and 5xx responses, with 250 ms "
            "and 750 ms backoff, and the suite still passes.\n\n"
            "- added `withRetry()` in `src/net/fetch.ts`\n"
            "- covered 500 → success and exhausted retries\n"
            "- 12 tests pass, nothing else touched")),
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
    m_demoUsage = QVariantMap{
        {"used", 18432},
        {"size", 200000},
        {"cost", QVariantMap{{"amount", 0.0}, {"currency", "USD"}}},
    };
}
