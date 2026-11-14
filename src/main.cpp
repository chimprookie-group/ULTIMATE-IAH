// Ultimate IAH, the DeepSeek Harness cockpit for Omarchy.

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QStandardPaths>
#include <QtQuick/private/qquickitem_p.h>
#include <QTextStream>
#include <QTemporaryDir>
#include <QTimer>
#include <QtWebEngineQuick/QtWebEngineQuick>

#include <unistd.h>

#include "backend.h"
#include "designtokens.h"
#include "memorystore.h"
#include "mcpmanager.h"
#include "connectiondoctor.h"
#include "installationdoctor.h"
#include "ultimateagent.h"
#include "agentmanager.h"
#include "skillmanager.h"
#include "missionrunner.h"
#include "notestore.h"
#include "pluginmanager.h"
#include "webservice.h"
#include "githubservice.h"
#include "crasyrouter.h"
#include "updateservice.h"
#include "learningengine.h"
#include "knowledgebase.h"
#include "autopilot.h"
#include "botmanager.h"
#include "ecosystem.h"
#include "iconprovider.h"
#include "harnessconnections.h"
#include "hoverservice.h"
#include "terminalsession.h"
#include "theme.h"
#include "workspacemodel.h"
#include "translator.h"

namespace {

// Qt 6.12 can hand a glyph node with a null matrix to the batch renderer;
// Element::computeBounds() then dereferences node->matrix() and segfaults.
// A null matrix means identity, so make that explicit before each render.
void setMissingMatrices(QSGNode *node) {
    if (!node)
        return;
    if (node->type() == QSGNode::GeometryNodeType) {
        auto *geometry = static_cast<QSGGeometryNode *>(node);
        if (!geometry->matrix()) {
            static const QMatrix4x4 identity;
            geometry->setRendererMatrix(&identity);
        }
    }
    for (QSGNode *child = node->firstChild(); child; child = child->nextSibling())
        setMissingMatrices(child);
}

QString probe(const QString &binary, const QStringList &arguments) {
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(binary, arguments);
    if (!process.waitForStarted(4000))
        return {};
    process.waitForFinished(12000);
    const QString output = QString::fromUtf8(process.readAll()).trimmed();
    return output.split('\n').value(0).trimmed();
}

int runDoctor(bool json) {
    QJsonArray checks;
    auto add = [&checks](const QString &name, bool ok, const QString &detail) {
        checks.append(QJsonObject{{"name", name}, {"ok", ok}, {"detail", detail}});
    };


    const bool portal = QDBusConnection::sessionBus().interface()
        && QDBusConnection::sessionBus().interface()->isServiceRegistered("org.freedesktop.portal.Desktop");
    add("portal", portal, portal ? QStringLiteral("org.freedesktop.portal.Desktop is available")
                                 : QStringLiteral("xdg-desktop-portal is not running"));

    const QString home = QDir::homePath();
    const bool writable = QFileInfo(home).isWritable();
    add("home", writable, writable ? home + QStringLiteral(" is writable")
                                   : home + QStringLiteral(" is not writable"));

    const bool webengine = QFileInfo::exists(QStringLiteral("/usr/lib/qt6/qml/QtWebEngine"));
    add("webengine", webengine, webengine ? QStringLiteral("QtWebEngine QML module is installed")
                                          : QStringLiteral("qt6-webengine is missing"));

    const QString config = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    const QString data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QDir skillsDir(home + QStringLiteral("/.agents/skills"));
    const int skillsCount = skillsDir.exists()
        ? skillsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size() : 0;
    add("skills", skillsCount >= 12,
        QStringLiteral("%1 skills installed in ~/.agents/skills").arg(skillsCount));
    const QFileInfo knowledge(data + QStringLiteral("/knowledge.db"));
    add("knowledge", knowledge.exists() && knowledge.size() > 0,
        knowledge.exists() ? QStringLiteral("knowledge.db is ready")
                           : QStringLiteral("knowledge.db is missing"));
    const QFileInfo routerToken(config + QStringLiteral("/4crasy/token"));
    add("4crasy", routerToken.exists(), routerToken.exists()
        ? QStringLiteral("4Crasy token is present")
        : QStringLiteral("4Crasy token is missing"));
    QFile overlay(config + QStringLiteral("/connections.patch.yml"));
    bool hasProvider = false;
    if (overlay.open(QIODevice::ReadOnly))
        hasProvider = QString::fromUtf8(overlay.readAll()).contains(QStringLiteral("agent-default-model"));
    add("provider", hasProvider, hasProvider
        ? QStringLiteral("a default agent provider is configured")
        : QStringLiteral("no default agent provider; run bootstrap"));

    bool ok = true;
    for (const auto &value : checks)
        ok = ok && value.toObject().value("ok").toBool();

    if (json) {
        const QJsonObject object{{"ok", ok}, {"checks", checks}};
        QTextStream(stdout) << QJsonDocument(object).toJson(QJsonDocument::Compact) << '\n';
    } else {
        for (const auto &value : checks) {
            const QJsonObject check = value.toObject();
            QTextStream(stdout) << (check.value("ok").toBool() ? "ok   " : "fail ")
                                << check.value("name").toString().leftJustified(10)
                                << check.value("detail").toString() << '\n';
        }
    }
    return ok ? 0 : 1;
}

QString skillText() {
    return QStringLiteral(R"(# Ultimate IAH

Ultimate IAH is a native Omarchy cockpit for the agentic harness, with a real
terminal, notes, GitHub, web research and project-aware tools.

## Commands

- `ultimate-iah open [folder]` opens the GUI, optionally rooted at a folder.
- `ultimate-iah doctor --json` checks QtWebEngine, the desktop portal and the
  local runtime. Exit code 0 means everything is ready.
- `ultimate-iah skill install` writes this file.

## What it does

- Ultimate Agent: native tool-calling loop, sessions, plans, jobs, subagents,
  memory, skills, web search/fetch and MCP.
- Notes: Markdown vault, wikilinks, backlinks and a force-directed graph.
- GitHub: gh-backed repositories, projects, issues and pull requests.
- Plugins: ArtCraft suite and external command manifests.
- Terminal agent: a real PTY in the active workspace.

Use it for agentic project work without an embedded web harness.
)");
}

int installSkill() {
    const QString directory = QDir::homePath() + "/.agents/skills/ultimate-iah";
    QDir().mkpath(directory);
    QFile file(directory + "/SKILL.md");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream(stderr) << "Could not write " << file.fileName() << '\n';
        return 1;
    }
    file.write(skillText().toUtf8());
    file.close();
    QTextStream(stdout) << "Installed " << file.fileName() << '\n';
    return 0;
}

} // namespace

namespace {

// Older builds shipped a user-level Crasy desktop entry and palette icons.
// They shadow the new Ultimate IAH launcher and keep showing the retired logo.
// Migrate only when the packaged launcher is present, and only touch launcher
// files, never user configuration or workspace data.
void migrateLegacyLauncher() {
    if (!QFileInfo::exists(QStringLiteral("/usr/share/applications/ultimate-iah.desktop")))
        return;

    const QString applications = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(applications);
    QFile::remove(applications + QStringLiteral("/deepcrasy-h.desktop"));
    QFile::remove(applications + QStringLiteral("/ultimate-hai.desktop"));

    // The old crasy package keeps a system entry with the old whale icon. Hide
    // it through a user override so the package stays installed (it provides
    // the system DSH runtime) without showing a second retired launcher.
    const QString oldEntry = applications + QStringLiteral("/crasy.desktop");
    if (QFileInfo::exists(QStringLiteral("/usr/share/applications/crasy.desktop"))
        && !QFileInfo::exists(oldEntry)) {
        QSaveFile file(oldEntry);
        if (file.open(QIODevice::WriteOnly)) {
            file.write("[Desktop Entry]\n"
                       "Type=Application\n"
                       "Name=Crasy\n"
                       "Hidden=true\n"
                       "NoDisplay=true\n");
            file.commit();
        }
    }

    const QString iconRoot = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/icons/hicolor");
    for (const QString &size : {QStringLiteral("scalable"), QStringLiteral("256x256"),
                                QStringLiteral("512x512")}) {
        for (const QString &legacy : {QStringLiteral("deepcrasy-h"), QStringLiteral("ultimate-hai")}) {
            const QString base = iconRoot + QLatin1Char('/') + size + QStringLiteral("/apps/") + legacy;
            QFile::remove(base + QStringLiteral(".svg"));
            QFile::remove(base + QStringLiteral(".png"));
        }
    }

    // Invalidate user-level caches. Failure is harmless: the files are gone and
    // the packaged desktop entry is authoritative.
    QProcess::startDetached(QStringLiteral("update-desktop-database"), {applications});
    const QString iconCache = QStandardPaths::findExecutable(QStringLiteral("gtk-update-icon-cache"));
    if (!iconCache.isEmpty() && QFileInfo::exists(iconRoot))
        QProcess::startDetached(iconCache, {QStringLiteral("-f"), QStringLiteral("-t"), iconRoot});
}

} // namespace

int main(int argc, char *argv[]) {
    // Keep the legacy application name and organization: they define the
    // stable config/data paths that hold the user's memory, providers and
    // integrations. The visible product is Ultimate IAH.
    QCoreApplication::setApplicationName(QStringLiteral("deepcrasy-h"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.25.19"));
    QCoreApplication::setOrganizationName(QStringLiteral("Crasy"));

    QStringList arguments;
    for (int i = 1; i < argc; ++i)
        arguments.append(QString::fromLocal8Bit(argv[i]));

    if (arguments.value(0) == "--version" || arguments.value(0) == "-v") {
        QTextStream(stdout) << "Ultimate IAH " << QCoreApplication::applicationVersion() << '\n';
        return 0;
    }
    if (arguments.value(0) == "bootstrap") {
        QCoreApplication app(argc, argv);
        const QString config = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        const QString data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(config);
        QDir().mkpath(data);
        QFile::setPermissions(config, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        QFile::setPermissions(data, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        SkillManager skills;
        const bool seeded = skills.seedDefaults();
        LearningEngine learning(QString(), &skills);
        KnowledgeBase knowledge;
        const QJsonArray tools = UltimateAgent::tools();
        knowledge.seed(tools, {}, skills.skills());
        CrasyRouter router;
        HarnessConnections connections;
        if (connections.defaultProvider().isEmpty()) {
            QVariantMap chosen;
            for (const QVariant &value : connections.providers()) {
                const QVariantMap provider = value.toMap();
                if (provider.value(QStringLiteral("hasKey")).toBool()
                    && !provider.value(QStringLiteral("models")).toStringList().isEmpty()) {
                    chosen = provider;
                    break;
                }
            }
            if (chosen.isEmpty()) {
                connections.saveProvider(QStringLiteral("4crasy"), QStringLiteral("4Crasy"),
                                         router.endpoint(), router.token(),
                                         QStringLiteral("auto=Auto"), true,
                                         QStringLiteral("openai-completions"));
            } else {
                connections.setDefault(chosen.value(QStringLiteral("id")).toString(),
                                       chosen.value(QStringLiteral("models")).toStringList().first());
            }
        }
        // Keep the built-in router available for the default local provider.
        router.saveSettings(router.port(), router.freeOnly(), router.maxFallbacks(), true,
                            router.strategy());
        const int entries = knowledge.stats().value(QStringLiteral("entries")).toInt();
        const QString provider = connections.defaultProvider();
        QTextStream(stdout) << "Ultimate IAH bootstrap\n"
                            << "config=" << config << "\n"
                            << "data=" << data << "\n"
                            << "skills=" << skills.defaultCount() << "\n"
                            << "tools=" << tools.size() << "\n"
                            << "knowledge=" << entries << "\n"
                            << "provider=" << (provider.isEmpty() ? QStringLiteral("none") : provider) << "\n"
                            << "4crasy-token=" << QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                   + QStringLiteral("/4crasy/token") << "\n";
        if (!seeded || skills.defaultCount() < 42 || tools.isEmpty() || entries < tools.size()) {
            QTextStream(stderr) << "ultimate-iah bootstrap: default wiring incomplete\n";
            return 1;
        }
        return 0;
    }
    if (arguments.value(0) == "doctor") {
        QCoreApplication app(argc, argv);
        return runDoctor(arguments.contains("--json"));
    }
    if (arguments.value(0) == "skill" && arguments.value(1) == "install") {
        QCoreApplication app(argc, argv);
        return installSkill();
    }
    if (arguments.value(0) == "open")
        arguments.removeFirst();

    bool demo = false;
    bool agentJob = false;
    bool agentDock = false;
    bool routerService = false;
    bool routerConfigure = false;
    QString screenshot;
    QString view;
    QString workspace;
    QString themeId;
    QString agentPrompt;
    QString agentPermission = QStringLiteral("workspace-write");
    int windowHeight = 0;
    QString agentProvider;
    QString agentModel;
    int windowWidth = 0;
    int delay = 1800;
    for (int i = 0; i < arguments.size(); ++i) {
        const QString &argument = arguments.at(i);
        if (argument == "--demo") {
            demo = true;
        } else if (argument == "--agent-job") {
            agentJob = true;
        } else if (argument == "--agent-dock") {
            agentDock = true;
        } else if (argument == "--router-service") {
            routerService = true;
        } else if (argument == "--router-configure") {
            routerConfigure = true;
        } else if (argument == "--screenshot" && i + 1 < arguments.size()) {
            screenshot = arguments.at(++i);
        } else if (argument == "--view" && i + 1 < arguments.size()) {
            view = arguments.at(++i);
        } else if (argument == "--workspace" && i + 1 < arguments.size()) {
            workspace = arguments.at(++i);
        } else if (argument == "--theme" && i + 1 < arguments.size()) {
            themeId = arguments.at(++i);
        } else if (argument == "--agent-prompt" && i + 1 < arguments.size()) {
            agentPrompt = arguments.at(++i);
        } else if (argument == "--permission" && i + 1 < arguments.size()) {
            agentPermission = arguments.at(++i);
        } else if (argument == "--agent-provider" && i + 1 < arguments.size()) {
            agentProvider = arguments.at(++i);
        } else if (argument == "--agent-model" && i + 1 < arguments.size()) {
            agentModel = arguments.at(++i);
        } else if (argument == "--height" && i + 1 < arguments.size()) {
            windowHeight = arguments.at(++i).toInt();
        } else if (argument == "--width" && i + 1 < arguments.size()) {
            windowWidth = arguments.at(++i).toInt();
        } else if (argument == "--screenshot-delay" && i + 1 < arguments.size()) {
            delay = arguments.at(++i).toInt();
        } else if (!argument.startsWith("--") && QFileInfo(argument).isDir()) {
            workspace = argument;
        }
    }

    if (agentJob) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        qputenv("ULTIMATE_IAH_AGENT_JOB", "1");
    }
    if (routerService)
        qputenv("QT_QPA_PLATFORM", "offscreen");

    // The QML disk cache keys qrc URLs by path, so a rebuilt binary can keep
    // serving an old QML resource from cache. Start fresh every time; the
    // app is small enough that recompiling costs nothing noticeable.
    qputenv("QML_DISABLE_DISK_CACHE", "1");

    // Qt 6.12's threaded Quick render loop can segfault inside
    // QSGBatchRenderer::prepareAlphaBatches() while the transcript is
    // streaming. Render on the GUI thread so the scene-graph guard below
    // runs in lockstep with the UI.
    qputenv("QSG_RENDER_LOOP", "basic");

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    if (!agentJob && !routerService)
        QtWebEngineQuick::initialize();

    QGuiApplication app(argc, argv);
    migrateLegacyLauncher();

    // Matches the window to ultimate-iah.desktop, so the compositor (Wayland
    // app_id) and the launcher pick up the installed icon.
    app.setDesktopFileName(QStringLiteral("ultimate-iah"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("ultimate-iah")));

    // One window per user: a second launch must raise the existing one, not
    // spawn another QtWebEngine profile and fight over the same portal id.
    QQuickWindow *mainWindow = nullptr;
    const QString instanceName = QStringLiteral("ultimate-iah-%1").arg(QString::number(getuid()));
    QLocalServer instanceServer;
    if (screenshot.isEmpty() && !demo && !agentJob && !routerService && !routerConfigure) {
        QLocalSocket probe;
        probe.connectToServer(instanceName);
        if (probe.waitForConnected(250)) {
            probe.write("raise\n");
            probe.flush();
            probe.waitForBytesWritten(250);
            if (probe.waitForReadyRead(500)) {
                probe.readAll();
                return 0;
            }
        }
        QLocalServer::removeServer(instanceName);
        instanceServer.listen(instanceName);
        QObject::connect(&instanceServer, &QLocalServer::newConnection, &app, [&] {
            while (QLocalSocket *client = instanceServer.nextPendingConnection()) {
                client->readAll();
                client->write("ok\n");
                client->flush();
                client->disconnectFromServer();
                client->deleteLater();
            }
            if (mainWindow) {
                mainWindow->show();
                mainWindow->raise();
                mainWindow->requestActivate();
            }
        });
    }

    QQuickStyle::setStyle(QStringLiteral("Material"));

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    QTemporaryDir agentJobDir;
    if (agentJob && agentJobDir.isValid())
        qputenv("ULTIMATE_IAH_AGENT_DB", (agentJobDir.path() + QStringLiteral("/agent.db")).toUtf8());

    Theme theme;
    Backend backend;
    backend.attachTheme(&theme);
    DesignTokens design;

    TerminalSession terminal;
    HoverService hoverService;
    HarnessConnections connections;
    MemoryStore memory;
    McpManager mcpManager;
    AgentManager agents;
    SkillManager skills;
    skills.setWorkspace(backend.workspace());
    NoteStore noteStore;
    PluginManager plugins;
    LearningEngine learning(QString(), &skills);
    KnowledgeBase knowledge;
    UpdateService updates;
    WebService web;
    GitHubService gitHub(&backend);
    CrasyRouter crasy;
    if (routerConfigure || routerService) {
        if (!crasy.start()) {
            QTextStream(stderr) << "4Crasy: " << crasy.error() << Qt::endl;
            return 1;
        }
        if (routerConfigure) {
            connections.saveProvider(QStringLiteral("4crasy"), QStringLiteral("4Crasy"),
                                     crasy.endpoint(), crasy.token(), QStringLiteral("auto"), true);
            if (!connections.error().isEmpty()) {
                QTextStream(stderr) << "4Crasy provider: " << connections.error() << Qt::endl;
                return 1;
            }
            QTextStream(stdout) << "4Crasy configured as default provider: " << crasy.endpoint() << Qt::endl;
        } else {
            QTextStream(stdout) << "4Crasy API " << crasy.endpoint()
                                << " token-file " << QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                       + QStringLiteral("/4crasy/token") << Qt::endl;
        }
        QTextStream(stdout).flush();
        return app.exec();
    }
    QObject::connect(&backend, &Backend::workspaceChanged, &skills, [&backend, &skills] {
        skills.setWorkspace(backend.workspace());
    });
    MissionRunner missions(MissionRunner::defaultPath(), &agents, &memory);
    missions.setDshBinary(QCoreApplication::applicationFilePath());
    translator.setOverride(backend.languagePreference());
    Autopilot autopilot;
    autopilot.setMemory(&memory);
    BotManager bots(&backend, &memory);
    ConnectionDoctor connectionDoctor(&backend, &connections, &mcpManager, &memory, &bots);
    InstallationDoctor installation(&backend, &memory);
    UltimateAgent ultimateAgent(&backend, &connections, &mcpManager, &memory,
                                &skills, &agents, &missions, &terminal,
                                &installation, &connectionDoctor, &bots, &noteStore, &plugins, &web, &gitHub, &crasy, &learning, &knowledge);
    knowledge.seed(UltimateAgent::tools(), plugins.plugins(), skills.skills());
    if (agentJob) {
        if (!workspace.isEmpty())
            backend.setWorkspace(workspace);
        ultimateAgent.setPermissionMode(agentPermission);
        if (!agentProvider.isEmpty() || !agentModel.isEmpty())
            ultimateAgent.setRuntimeModel(agentProvider, agentModel);
        if (agentPrompt.trimmed().isEmpty()) {
            QTextStream(stderr) << "ultimate-iah: --agent-job requires --agent-prompt" << Qt::endl;
            return 2;
        }
        ultimateAgent.sendMessage(agentPrompt);
        if (!ultimateAgent.busy()) {
            QTextStream(stderr) << (ultimateAgent.error().isEmpty()
                                    ? QStringLiteral("ultimate-iah: the agent did not start")
                                    : ultimateAgent.error()) << Qt::endl;
            return 1;
        }
        QEventLoop loop;
        QTimer watchdog;
        watchdog.setSingleShot(true);
        watchdog.setInterval(8 * 60 * 1000);
        QObject::connect(&watchdog, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&ultimateAgent, &UltimateAgent::busyChanged, &loop, [&] {
            if (!ultimateAgent.busy())
                loop.quit();
        });
        watchdog.start();
        loop.exec();
        if (ultimateAgent.busy()) {
            ultimateAgent.cancel();
            QTextStream(stderr) << "ultimate-iah: agent job timed out" << Qt::endl;
            return 1;
        }
        QString answer;
        const QVariantList transcript = ultimateAgent.messages();
        for (int i = transcript.size() - 1; i >= 0; --i) {
            const QVariantMap message = transcript.at(i).toMap();
            if (message.value(QStringLiteral("role")).toString() == QLatin1String("assistant")) {
                answer = message.value(QStringLiteral("text")).toString();
                break;
            }
        }
        if (!ultimateAgent.error().isEmpty() && answer.isEmpty()) {
            QTextStream(stderr) << ultimateAgent.error() << Qt::endl;
            return 1;
        }
        QTextStream(stdout) << answer << Qt::endl;
        return 0;
    }

    autopilot.setDshBinary(QCoreApplication::applicationFilePath());
    WorkspaceModel workspaceModel;
    Ecosystem ecosystem;
    QObject::connect(&backend, &Backend::workspaceChanged, &ecosystem,
                     [&backend, &ecosystem] { ecosystem.refresh(backend.workspace()); });

    // The translator auto-detects (Omarchy, then the browser language), and an
    // explicit app preference wins over both.
    QObject::connect(&backend, &Backend::dshBinaryChanged, &autopilot, [&backend, &autopilot] {
        autopilot.setDshBinary(QCoreApplication::applicationFilePath());
    });
    QObject::connect(&backend, &Backend::dshBinaryChanged, &missions, [&backend, &missions] {
        missions.setDshBinary(QCoreApplication::applicationFilePath());
    });
    QObject::connect(&backend, &Backend::dshBinaryChanged, &bots, [&backend, &bots] {
        bots.setDshBinary(backend.dshBinary());
    });
    QObject::connect(&backend, &Backend::languageChanged, &translator, [&backend, &translator] {
        translator.setOverride(backend.languagePreference());
    });

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("design", &design);
    engine.rootContext()->setContextProperty("terminal", &terminal);
    engine.rootContext()->setContextProperty("hoverService", &hoverService);
    engine.rootContext()->setContextProperty("connections", &connections);
    engine.rootContext()->setContextProperty("memory", &memory);
    engine.rootContext()->setContextProperty("mcpManager", &mcpManager);
    engine.rootContext()->setContextProperty("connectionDoctor", &connectionDoctor);
    engine.rootContext()->setContextProperty("installation", &installation);
    engine.rootContext()->setContextProperty("ultimateAgent", &ultimateAgent);
    engine.rootContext()->setContextProperty("agents", &agents);
    engine.rootContext()->setContextProperty("skills", &skills);
    engine.rootContext()->setContextProperty("missions", &missions);
    engine.rootContext()->setContextProperty("autopilot", &autopilot);
    engine.rootContext()->setContextProperty("bots", &bots);
    engine.rootContext()->setContextProperty("noteStore", &noteStore);
    engine.rootContext()->setContextProperty("plugins", &plugins);
    engine.rootContext()->setContextProperty("updates", &updates);
    engine.rootContext()->setContextProperty("learning", &learning);
    engine.rootContext()->setContextProperty("knowledge", &knowledge);
    engine.rootContext()->setContextProperty("web", &web);
    engine.rootContext()->setContextProperty("gitHub", &gitHub);
    engine.rootContext()->setContextProperty("crasy", &crasy);
    engine.rootContext()->setContextProperty("ecosystem", &ecosystem);
    engine.rootContext()->setContextProperty("workspaceModel", &workspaceModel);
    engine.addImageProvider(QStringLiteral("themeicon"), new IconProvider);

    if (!themeId.isEmpty())
        theme.setSelectedTheme(themeId);

    backend.applyStartupView();

    if (demo)
        backend.loadDemo();
    if (!workspace.isEmpty())
        backend.setWorkspace(workspace);
    if (!view.isEmpty())
        backend.setView(view);

    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app,
                     [](const QList<QQmlError> &warnings) {
                         for (const QQmlError &warning : warnings)
                             qWarning().noquote() << "QML:" << warning.toString();
                     });
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0))) {
        mainWindow = window;
        QObject::connect(window, &QQuickWindow::beforeRendering, window, [window] {
            setMissingMatrices(QQuickItemPrivate::get(window->contentItem())->itemNode());
        }, Qt::DirectConnection);
    }

    if (windowHeight > 0 || windowWidth > 0) {
        if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0)))
            window->resize(windowWidth > 0 ? windowWidth : window->width(),
                           windowHeight > 0 ? windowHeight : window->height());
    }

    if (agentDock) {
        if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0)))
            window->setProperty("agentDockOpen", true);
    }

    if (!demo)
        backend.boot();

    if (!agentPrompt.isEmpty()) {
        QTimer::singleShot(700, &app, [&backend, &ultimateAgent, agentPrompt, agentDock] {
            backend.setView(agentDock ? "home" : "agent");
            ultimateAgent.sendMessage(agentPrompt);
        });
    }

    if (!screenshot.isEmpty()) {
        QTimer::singleShot(delay, &app, [&engine, screenshot] {
            if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0))) {
                const QImage image = window->grabWindow();
                if (!image.isNull())
                    image.save(screenshot);
            }
            QCoreApplication::exit(0);
        });
    }

    return app.exec();
}
