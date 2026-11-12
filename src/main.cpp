// Ultimate HAI, the DeepSeek Harness cockpit for Omarchy.

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDir>
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
#include "autopilot.h"
#include "ecosystem.h"
#include "iconprovider.h"
#include "nineroutermanager.h"
#include "harnessconnections.h"
#include "mausbotmanager.h"
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

    const QString dsh = QStandardPaths::findExecutable("dsh");
    add("dsh", !dsh.isEmpty(), dsh.isEmpty() ? QStringLiteral("not found in PATH")
                                             : dsh + " · " + probe(dsh, {"--version"}));

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
    return QStringLiteral(R"(# Ultimate HAI

Ultimate HAI is an Omarchy desktop cockpit for the DeepSeek Harness, with a
real terminal agent and project-aware tools.

## Commands

- `ultimate-hai open [folder]` opens the GUI, optionally rooted at a folder.
- `ultimate-hai doctor --json` checks that `dsh`, QtWebEngine, and the desktop
  portal are available. Exit code 0 means everything is ready.
- `ultimate-hai skill install` writes this file.

## What it does

- Harness pane: `dsh web` is started on a free port and its tokenized URL is
  embedded in the window.
- Terminal agent: a real PTY and a `dsh headless` task bar in the same workspace.
- Project and files: Git, diffs, tools and skills stay one gesture away.

Use it when the user wants the DeepSeek Harness plus a terminal agent.
)");
}

int installSkill() {
    const QString directory = QDir::homePath() + "/.agents/skills/ultimate-hai";
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
// They shadow the new Ultimate HAI launcher and keep showing the retired logo.
// Migrate only when the packaged launcher is present, and only touch launcher
// files, never user configuration or workspace data.
void migrateLegacyLauncher() {
    if (!QFileInfo::exists(QStringLiteral("/usr/share/applications/ultimate-hai.desktop")))
        return;

    const QString applications = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(applications);
    QFile::remove(applications + QStringLiteral("/deepcrasy-h.desktop"));

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
        const QString base = iconRoot + QLatin1Char('/') + size + QStringLiteral("/apps/deepcrasy-h");
        QFile::remove(base + QStringLiteral(".svg"));
        QFile::remove(base + QStringLiteral(".png"));
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
    // integrations. The visible product is Ultimate HAI.
    QCoreApplication::setApplicationName(QStringLiteral("deepcrasy-h"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.4.4"));
    QCoreApplication::setOrganizationName(QStringLiteral("Crasy"));

    QStringList arguments;
    for (int i = 1; i < argc; ++i)
        arguments.append(QString::fromLocal8Bit(argv[i]));

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
    bool agentDock = false;
    QString screenshot;
    QString view;
    QString workspace;
    QString themeId;
    QString agentPrompt;
    int windowHeight = 0;
    int windowWidth = 0;
    int delay = 1800;
    for (int i = 0; i < arguments.size(); ++i) {
        const QString &argument = arguments.at(i);
        if (argument == "--demo") {
            demo = true;
        } else if (argument == "--agent-dock") {
            agentDock = true;
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
    QtWebEngineQuick::initialize();

    QGuiApplication app(argc, argv);
    migrateLegacyLauncher();

    // Matches the window to ultimate-hai.desktop, so the compositor (Wayland
    // app_id) and the launcher pick up the installed icon.
    app.setDesktopFileName(QStringLiteral("ultimate-hai"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("ultimate-hai")));

    // One window per user: a second launch must raise the existing one, not
    // spawn another QtWebEngine profile and fight over the same portal id.
    QQuickWindow *mainWindow = nullptr;
    const QString instanceName = QStringLiteral("ultimate-hai-%1").arg(QString::number(getuid()));
    QLocalServer instanceServer;
    if (screenshot.isEmpty() && !demo) {
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

    Theme theme;
    Backend backend;
    backend.attachTheme(&theme);
    DesignTokens design;

    TerminalSession terminal;
    NineRouterManager nineRouter;
    MausBotManager mausBot;
    HarnessConnections connections;
    MemoryStore memory;
    McpManager mcpManager;
    AgentManager agents;
    SkillManager skills;
    skills.setWorkspace(backend.workspace());
    QObject::connect(&backend, &Backend::workspaceChanged, &skills, [&backend, &skills] {
        skills.setWorkspace(backend.workspace());
    });
    MissionRunner missions(MissionRunner::defaultPath(), &agents, &memory);
    missions.setDshBinary(backend.dshBinary());
    Autopilot autopilot;
    autopilot.setMemory(&memory);
    ConnectionDoctor connectionDoctor(&backend, &connections, &nineRouter,
                                      &mausBot, &mcpManager, &memory);
    InstallationDoctor installation(&backend, &nineRouter, &mausBot, &memory);
    UltimateAgent ultimateAgent(&backend, &connections, &nineRouter, &mausBot,
                                &mcpManager, &memory, &skills, &agents, &missions,
                                &terminal, &installation, &connectionDoctor);
    autopilot.setDshBinary(backend.dshBinary());
    WorkspaceModel workspaceModel;
    Ecosystem ecosystem;
    QObject::connect(&backend, &Backend::workspaceChanged, &ecosystem,
                     [&backend, &ecosystem] { ecosystem.refresh(backend.workspace()); });

    // The translator auto-detects (Omarchy, then the browser language), and an
    // explicit app preference wins over both.
    translator.setOverride(backend.languagePreference());
    QObject::connect(&backend, &Backend::dshBinaryChanged, &autopilot, [&backend, &autopilot] {
        autopilot.setDshBinary(backend.dshBinary());
    });
    QObject::connect(&connections, &HarnessConnections::deepSeekChanged, &nineRouter,
                     [&nineRouter] { nineRouter.syncDeepSeekFromDsh(); });
    QObject::connect(&backend, &Backend::dshBinaryChanged, &missions, [&backend, &missions] {
        missions.setDshBinary(backend.dshBinary());
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
    engine.rootContext()->setContextProperty("nineRouter", &nineRouter);
    engine.rootContext()->setContextProperty("mausBot", &mausBot);
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
