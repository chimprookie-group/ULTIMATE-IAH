// deepcrasy-h — DeepSeek Harness and OpenCode in one Omarchy window.

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
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QtWebEngineQuick/QtWebEngineQuick>

#include "backend.h"
#include "iconprovider.h"
#include "mcpmanager.h"
#include "terminalsession.h"
#include "theme.h"
#include "translator.h"

namespace {

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

    const QString opencode = QStandardPaths::findExecutable("opencode");
    QString opencodeVersion;
    if (!opencode.isEmpty())
        opencodeVersion = probe(opencode, {"--version"});
    add("opencode", !opencode.isEmpty(),
        opencode.isEmpty() ? QStringLiteral("not found in PATH")
                           : opencode + (opencodeVersion.isEmpty() ? QString() : " · " + opencodeVersion));

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
    return QStringLiteral(R"(# DeepCrasy H

DeepCrasy H is an Omarchy desktop application that puts the DeepSeek Harness
web UI and an OpenCode chat in one window, sharing one workspace.

## Commands

- `deepcrasy-h open [folder]` opens the GUI, optionally rooted at a folder.
- `deepcrasy-h doctor --json` checks that `dsh`, `opencode`, QtWebEngine, and
  the desktop portal are available. Exit code 0 means everything is ready.
- `deepcrasy-h skill install` writes this file.

## What it does

- Harness pane: `dsh web` is started on a free port and its tokenized URL is
  embedded in the window.
- OpenCode pane: `opencode acp` is driven over the Agent Client Protocol, so
  sessions, models, modes, tool calls, and permissions are native UI.
- Split pane: both at once, one workspace.

Use it when the user asks to run both agents against the same project.
)");
}

int installSkill() {
    const QString directory = QDir::homePath() + "/.agents/skills/deepcrasy-h";
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

int main(int argc, char *argv[]) {
    QCoreApplication::setApplicationName(QStringLiteral("deepcrasy-h"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.10.0"));
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
    QString screenshot;
    QString view;
    QString workspace;
    QString themeId;
    int windowHeight = 0;
    int delay = 1800;
    for (int i = 0; i < arguments.size(); ++i) {
        const QString &argument = arguments.at(i);
        if (argument == "--demo") {
            demo = true;
        } else if (argument == "--screenshot" && i + 1 < arguments.size()) {
            screenshot = arguments.at(++i);
        } else if (argument == "--view" && i + 1 < arguments.size()) {
            view = arguments.at(++i);
        } else if (argument == "--workspace" && i + 1 < arguments.size()) {
            workspace = arguments.at(++i);
        } else if (argument == "--theme" && i + 1 < arguments.size()) {
            themeId = arguments.at(++i);
        } else if (argument == "--height" && i + 1 < arguments.size()) {
            windowHeight = arguments.at(++i).toInt();
        } else if (argument == "--screenshot-delay" && i + 1 < arguments.size()) {
            delay = arguments.at(++i).toInt();
        } else if (!argument.startsWith("--") && QFileInfo(argument).isDir()) {
            workspace = argument;
        }
    }

    // The QML disk cache keys qrc URLs by path, so a rebuilt binary can keep
    // serving an old Composer.qml from ~/.cache. Start fresh every time; the
    // app is small enough that recompiling costs nothing noticeable.
    qputenv("QML_DISABLE_DISK_CACHE", "1");

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QtWebEngineQuick::initialize();

    QGuiApplication app(argc, argv);

    // Matches the window to deepcrasy-h.desktop, so the compositor (Wayland
    // app_id) and the launcher pick up the installed icon.
    app.setDesktopFileName(QStringLiteral("deepcrasy-h"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("deepcrasy-h")));

    QQuickStyle::setStyle(QStringLiteral("Material"));

    // Installed before anything is translated, and every qsTr() binding is
    // re-evaluated when the Omarchy language changes.
    Translator translator;
    app.installTranslator(&translator);

    Theme theme;
    Backend backend;
    backend.attachTheme(&theme);

    TerminalSession terminal;
    McpManager mcp;
    mcp.setWorkspace(backend.workspace());
    QObject::connect(&backend, &Backend::workspaceChanged, &mcp,
                     [&backend, &mcp] { mcp.setWorkspace(backend.workspace()); });

    // The translator auto-detects (Omarchy, then the browser language), and an
    // explicit app preference wins over both.
    translator.setOverride(backend.languagePreference());
    QObject::connect(&backend, &Backend::languageChanged, &translator, [&backend, &translator] {
        translator.setOverride(backend.languagePreference());
    });

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("terminal", &terminal);
    engine.rootContext()->setContextProperty("mcp", &mcp);
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

    if (windowHeight > 0) {
        if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0)))
            window->resize(window->width(), windowHeight);
    }

    if (!demo)
        backend.boot();

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
