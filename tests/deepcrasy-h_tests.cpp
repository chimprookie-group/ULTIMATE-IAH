#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "acpclient.h"
#include "config.h"
#include "ecosystem.h"
#include "workspacemodel.h"
#include "gitstatus.h"
#include "mcpmanager.h"
#include "messagemodel.h"
#include "terminalsession.h"
#include "harnessserver.h"
#include "theme.h"
#include "translator.h"

class DeepCrasyTests : public QObject {
    Q_OBJECT

private slots:
    void harnessExtractsTokenizedUrl()
    {
        const QString line = "dsh web: http://127.0.0.1:35043/?token=abc123";
        QCOMPARE(HarnessServer::extractUrl(line), QString("http://127.0.0.1:35043/?token=abc123"));
    }

    void harnessExtractsUrlFromAnsiOutput()
    {
        const QString line = "\x1b[32mdsh web:\x1b[0m http://127.0.0.1:8080/?token=xyz";
        QCOMPARE(HarnessServer::extractUrl(line), QString("http://127.0.0.1:8080/?token=xyz"));
    }

    void harnessIgnoresPlainLogLines()
    {
        QVERIFY(HarnessServer::extractUrl("starting plugins…").isEmpty());
        QVERIFY(HarnessServer::extractUrl("error: --profile <name> is required").isEmpty());
    }

    void configDefaultsAreUsable()
    {
        QTemporaryDir dir;
        Config config(dir.path() + "/config.json");
        QCOMPARE(config.harnessProfile(), QString("web"));
        QVERIFY(QFileInfo(config.workspace()).isDir());
        QVERIFY(!config.dshBinary().isEmpty());
        QVERIFY(!config.opencodeBinary().isEmpty());
    }

    void configPersistsEverySetting()
    {
        QTemporaryDir dir;
        const QString path = dir.path() + "/nested/config.json";
        {
            Config config(path);
            config.setWorkspace(dir.path());
            config.setHarnessProfile("custom");
            config.setView("split");
            config.setTheme("tokyo-night");
            config.setLanguage("fr");
            config.setStartupView("home");
            config.setAutoStartHarness(false);
            config.setAutoStartOpencode(false);
            config.setSplitRatio(0.62);
            config.setNotifyOnFinish(false);
        }
        Config reloaded(path);
        QCOMPARE(reloaded.workspace(), QFileInfo(dir.path()).absoluteFilePath());
        QCOMPARE(reloaded.harnessProfile(), QString("custom"));
        QCOMPARE(reloaded.view(), QString("split"));
        QCOMPARE(reloaded.theme(), QString("tokyo-night"));
        QCOMPARE(reloaded.language(), QString("fr"));
        QCOMPARE(reloaded.startupView(), QString("home"));
        QCOMPARE(reloaded.autoStartHarness(), false);
        QCOMPARE(reloaded.autoStartOpencode(), false);
        QCOMPARE(reloaded.splitRatio(), 0.62);
        QCOMPARE(reloaded.notifyOnFinish(), false);
    }

    void configRejectsMissingWorkspace()
    {
        QTemporaryDir dir;
        Config config(dir.path() + "/config.json");
        const QString before = config.workspace();
        config.setWorkspace(dir.path() + "/does-not-exist");
        QCOMPARE(config.workspace(), before);
    }

    void configIgnoresUnknownView()
    {
        QTemporaryDir dir;
        Config config(dir.path() + "/config.json");
        config.setView("opencode");
        config.setView("bogus");
        QCOMPARE(config.view(), QString("opencode"));
    }

    void gitStatusParsesBranchAndChanges()
    {
        const QVariantMap clean = GitStatus::parse("## main...origin/main\n");
        QCOMPARE(clean.value("branch").toString(), QString("main"));
        QCOMPARE(clean.value("dirty").toInt(), 0);

        const QVariantMap dirty = GitStatus::parse("## feature/login...origin/feature/login\n M src/a.cpp\n?? notes.md\n");
        QCOMPARE(dirty.value("branch").toString(), QString("feature/login"));
        QCOMPARE(dirty.value("dirty").toInt(), 2);

        const QVariantMap init = GitStatus::parse("## No commits yet on main\n");
        QCOMPARE(init.value("branch").toString(), QString("main"));
        QCOMPARE(init.value("dirty").toInt(), 0);
    }

    void ecosystemInspectsProjectAndSkills()
    {
        QTemporaryDir dir;
        QFile package(dir.path() + "/package.json");
        QVERIFY(package.open(QIODevice::WriteOnly));
        package.write("{\"scripts\":{\"test\":\"vitest\",\"build\":\"vite build\"}}");
        package.close();
        QFile lock(dir.path() + "/package-lock.json");
        QVERIFY(lock.open(QIODevice::WriteOnly));
        lock.close();

        const QVariantMap project = Ecosystem::inspectProject(dir.path());
        QCOMPARE(project.value("type").toString(), QString("node"));
        QCOMPARE(project.value("manager").toString(), QString("npm"));
        bool hasTest = false;
        for (const QVariant &value : project.value("commands").toList()) {
            if (value.toMap().value("id").toString() == "script:test")
                hasTest = true;
        }
        QVERIFY(hasTest);

        QVERIFY(QDir().mkpath(dir.path() + "/skills/demo"));
        QFile skill(dir.path() + "/skills/demo/SKILL.md");
        QVERIFY(skill.open(QIODevice::WriteOnly));
        skill.write("---\nname: Demo Skill\ndescription: A demo skill\n---\n");
        skill.close();
        const QVariantList skills = Ecosystem::inspectSkills(dir.path() + "/skills");
        QCOMPARE(skills.size(), 1);
        QCOMPARE(skills.at(0).toMap().value("name").toString(), QString("Demo Skill"));
    }

    void profilesPersistAcrossRestarts()
    {
        QTemporaryDir dir;
        const QString path = dir.path() + "/config.json";
        Config config(path);
        const QString id = config.saveProfile("Review", "openrouter/deepseek-v4.1", "plan");
        QVERIFY(!id.isEmpty());
        QCOMPARE(config.profiles().size(), 1);
        QCOMPARE(config.activeProfileId(), id);

        Config reloaded(path);
        QCOMPARE(reloaded.profiles().size(), 1);
        QCOMPARE(reloaded.activeProfileId(), id);
        const QVariantMap profile = reloaded.profileById(id);
        QCOMPARE(profile.value("name").toString(), QString("Review"));
        QCOMPARE(profile.value("mode").toString(), QString("plan"));

        reloaded.removeProfile(id);
        QCOMPARE(reloaded.profiles().size(), 0);
        QVERIFY(reloaded.activeProfileId().isEmpty());
    }

    void workspaceListsFiltersAndPreviewsFiles()
    {
        QTemporaryDir dir;
        QFile first(dir.path() + "/alpha.txt");
        QVERIFY(first.open(QIODevice::WriteOnly));
        first.write("alpha contents\n");
        first.close();
        QVERIFY(QDir().mkpath(dir.path() + "/src"));
        QFile second(dir.path() + "/src/beta.cpp");
        QVERIFY(second.open(QIODevice::WriteOnly));
        second.write("int main() {}\n");
        second.close();

        WorkspaceModel model;
        model.refresh(dir.path());
        QCOMPARE(model.count(), 2);
        QVERIFY(model.preview("alpha.txt").contains("alpha contents"));
        QVERIFY(model.preview("src/beta.cpp").contains("int main"));
        QVERIFY(model.preview("../outside.txt").isEmpty()
                || model.preview("../outside.txt").contains("outside"));

        model.setFilter("beta");
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.files().at(0).toMap().value("path").toString(), QString("src/beta.cpp"));
        QVERIFY(model.absolutePath("src/beta.cpp").endsWith("/src/beta.cpp"));
    }

    void terminalRunQueuesBeforeStart()
    {
        TerminalSession session;
        QSignalSpy output(&session, &TerminalSession::output);
        session.run("echo deepcrasy-queued");
        session.start(QDir::tempPath());
        QTRY_VERIFY_WITH_TIMEOUT(session.running(), 5000);
        QString transcript;
        const auto hasOutput = [&] {
            while (output.size() > 0)
                transcript += output.takeFirst().value(0).toString();
            return transcript.contains("deepcrasy-queued");
        };
        QTRY_VERIFY_WITH_TIMEOUT(hasOutput(), 10000);
        session.stop();
    }

    void mcpConfigurationIsEditedSafely()
    {
        QTemporaryDir dir;
        const QString path = dir.path() + "/opencode.json";
        QFile seed(path);
        QVERIFY(seed.open(QIODevice::WriteOnly));
        seed.write("{\"$schema\": \"https://opencode.ai/config.json\", \"theme\": \"system\"}");
        seed.close();

        McpManager mcp(path, false);
        QCOMPARE(mcp.servers().size(), 0);

        mcp.addServer("filesystem", "local", "npx -y @modelcontextprotocol/server-filesystem /tmp", "global");
        QCOMPARE(mcp.servers().size(), 1);
        QCOMPARE(mcp.servers().at(0).toMap().value("name").toString(), QString("filesystem"));
        QCOMPARE(mcp.error(), QString());

        QFile written(path);
        QVERIFY(written.open(QIODevice::ReadOnly));
        const QJsonObject root = QJsonDocument::fromJson(written.readAll()).object();
        written.close();
        QCOMPARE(root.value("theme").toString(), QString("system"));
        const QJsonObject server = root.value("mcp").toObject().value("servers").toObject()
                                       .value("filesystem").toObject();
        QCOMPARE(server.value("type").toString(), QString("local"));
        QCOMPARE(server.value("command").toArray().size(), 4);

        mcp.addServer("remote", "remote", "https://example.com/mcp", "global");
        QCOMPARE(mcp.servers().size(), 2);

        QTemporaryDir project;
        mcp.setWorkspace(project.path());
        QCOMPARE(mcp.projectConfigPath(), project.path() + "/opencode.json");
        mcp.addServer("project-tools", "remote", "https://example.com/project", "project");
        QCOMPARE(mcp.servers().size(), 3);
        QFile projectConfig(project.path() + "/opencode.json");
        QVERIFY(projectConfig.open(QIODevice::ReadOnly));
        const QJsonObject projectRoot = QJsonDocument::fromJson(projectConfig.readAll()).object();
        projectConfig.close();
        QVERIFY(projectRoot.value("mcp").toObject().value("servers").toObject()
                    .contains("project-tools"));

        mcp.removeServer("filesystem", "global");
        QCOMPARE(mcp.servers().size(), 2);
        QCOMPARE(mcp.servers().at(0).toMap().value("name").toString(), QString("remote"));
        mcp.removeServer("project-tools", "project");
        QCOMPARE(mcp.servers().size(), 1);

        mcp.addServer("local-only", "local", "echo", "global");
        mcp.authenticate("local-only", "global");
        QVERIFY(!mcp.authRunning());
        QVERIFY(!mcp.authMessage().isEmpty());
        mcp.logoutServer("local-only", "global");
        QVERIFY(!mcp.authRunning());
        QVERIFY(!mcp.authMessage().isEmpty());

        mcp.addServer("bad name", "local", "echo", "global");
        QVERIFY(!mcp.error().isEmpty());
    }

    void mcpExtractsTheAuthorizationUrl()
    {
        const QString output = QStringLiteral(
            "\x1b[32mStarting\x1b[0m http://127.0.0.1:43123/callback "
            "then https://auth.example.com/oauth/authorize?client_id=crasy.");
        QCOMPARE(McpManager::extractAuthUrl(output),
                 QStringLiteral("https://auth.example.com/oauth/authorize?client_id=crasy"));
        QCOMPARE(McpManager::extractAuthUrl(QStringLiteral("http://127.0.0.1:9000/callback")),
                 QString());
        QCOMPARE(McpManager::extractAuthUrl(QStringLiteral("no url here")), QString());
    }

    void messageModelUpdatesOneRowAtATime()
    {
        MessageListModel model;
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);

        model.append(QVariantMap{{"id", "1"}, {"role", "assistant"}, {"text", "a"}});
        model.append(QVariantMap{{"id", "2"}, {"role", "tool"}, {"text", ""}});
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(inserted.count(), 2);
        QCOMPARE(changed.count(), 0);

        model.setAt(0, QVariantMap{{"id", "1"}, {"role", "assistant"}, {"text", "ab"}});
        QCOMPARE(changed.count(), 1);
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0), MessageListModel::TextRole).toString(), QString("ab"));
        QCOMPARE(model.data(model.index(0), MessageListModel::RoleRole).toString(), QString("assistant"));
    }

    void terminalRunsACommand()
    {
        TerminalSession session;
        QSignalSpy output(&session, &TerminalSession::output);
        session.start(QDir::tempPath());
        QTRY_VERIFY_WITH_TIMEOUT(session.running(), 5000);

        session.input(QStringLiteral("echo deepcrasy-terminal\n"));
        QString transcript;
        const auto hasOutput = [&] {
            while (output.size() > 0) {
                const QList<QVariant> args = output.takeFirst();
                transcript += args.value(0).toString();
            }
            return transcript.contains("deepcrasy-terminal");
        };
        QTRY_VERIFY_WITH_TIMEOUT(hasOutput(), 10000);

        session.stop();
        QVERIFY(!session.running());
    }

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#ffd60a"));
        QCOMPARE(theme.accentForeground(), QString("black"));
        QVERIFY(theme.dark());
    }

    void themeReadsAccent()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        QFile colors(dir.path() + "/theme/colors.toml");
        QVERIFY(colors.open(QIODevice::WriteOnly));
        colors.write("accent = \"#112233\"\n");
        colors.close();

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
    }

    void themeParsesALightPalette()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        QFile colors(dir.path() + "/theme/colors.toml");
        QVERIFY(colors.open(QIODevice::WriteOnly));
        colors.write("mode = \"light\"\n"
                     "accent = \"#1e66f5\"\n"
                     "background = \"#eff1f5\"\n"
                     "foreground = \"#4c4f69\"\n"
                     "cyan = \"#179299\"\n"
                     "green = \"#40a02b\"\n");
        colors.close();

        Theme theme(dir.path());
        QVERIFY(!theme.dark());
        QCOMPARE(theme.background(), QString("#eff1f5"));
        QCOMPARE(theme.text(), QString("#4c4f69"));
        QCOMPARE(theme.accent(), QString("#1e66f5"));
        QCOMPARE(theme.accentForeground(), QString("white"));
        QCOMPARE(theme.cyan(), QString("#179299"));
        QCOMPARE(theme.green(), QString("#40a02b"));
        // Surfaces have to separate themselves from the background.
        QVERIFY(theme.raised() != theme.background());
        QVERIFY(theme.border() != theme.background());
    }

    void themeScansInstalledPalettes()
    {
        QTemporaryDir dir;
        const auto writeTheme = [&dir](const QString &id, const QByteArray &palette) {
            QVERIFY(QDir(dir.path()).mkpath(id));
            QFile file(dir.path() + "/" + id + "/colors.toml");
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(palette);
        };
        writeTheme("acme-dark", "mode = \"dark\"\naccent = \"#112233\"\nbackground = \"#101010\"\nforeground = \"#eeeeee\"\n");
        writeTheme("acme-light", "mode = \"light\"\naccent = \"#334455\"\nbackground = \"#fafafa\"\nforeground = \"#111111\"\n");

        const QVariantList themes = Theme::scanThemes({dir.path()});
        QCOMPARE(themes.size(), 2);
        QCOMPARE(themes.at(0).toMap().value("label").toString(), QString("Acme Dark"));
        QCOMPARE(themes.at(1).toMap().value("label").toString(), QString("Acme Light"));
        QCOMPARE(themes.at(0).toMap().value("accent").toString(), QString("#112233"));
        QCOMPARE(themes.at(1).toMap().value("mode").toString(), QString("light"));
    }

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorAutoDetectsBrowserLanguage()
    {
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--lang=fr-FR");
        QTemporaryDir dir;
        write(dir.path() + "/fr.json", R"({"Settings": "Réglages"})");

        Translator translator(dir.path() + "/missing-language", dir.path());
        QCOMPARE(translator.language(), QString("fr"));
        QCOMPARE(translator.translate("Main", "Settings"), QString("Réglages"));
        qunsetenv("QTWEBENGINE_CHROMIUM_FLAGS");
    }

    void translatorOverrideWins()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "en\n");
        write(dir.path() + "/fr.json", R"({"Settings": "Réglages"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Settings").isNull());

        translator.setOverride("fr");
        QCOMPARE(translator.language(), QString("fr"));
        QCOMPARE(translator.translate("Main", "Settings"), QString("Réglages"));

        translator.setOverride("en");
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Settings").isNull());
    }

    void translatorFollowsFrenchCatalog()
    {
        QTemporaryDir dir;
        write(dir.path() + "/language", "fr\n");
        write(dir.path() + "/fr.json", R"({"Settings": "Réglages", "Harness": "Harness"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("fr"));
        QCOMPARE(translator.translate("Main", "Settings"), QString("Réglages"));
        QCOMPARE(translator.translate("Main", "Harness"), QString("Harness"));
        QVERIFY(translator.translate("Main", "Untranslated").isNull());
    }

    // The real contract test: spawn `opencode acp`, create a session, send a
    // prompt, and read the streamed answer back. It needs the network and a
    // free model, so it can be skipped with DEEPCRASY_SKIP_INTEGRATION=1.
    void opencodeAnswersOverAcp()
    {
        if (!qEnvironmentVariableIsEmpty("DEEPCRASY_SKIP_INTEGRATION"))
            QSKIP("DEEPCRASY_SKIP_INTEGRATION is set");
        const QString opencode = QStandardPaths::findExecutable("opencode");
        if (opencode.isEmpty())
            QSKIP("opencode is not installed");

        QTemporaryDir dir;
        AcpClient client;
        client.start(opencode, dir.path());
        QTRY_VERIFY_WITH_TIMEOUT(client.state() == AcpClient::Ready, 30000);

        client.newSession(dir.path());
        QTRY_VERIFY_WITH_TIMEOUT(!client.sessionId().isEmpty(), 20000);
        QTRY_VERIFY_WITH_TIMEOUT(!client.configOptions().isEmpty(), 10000);

        client.prompt("Reply with the single word pong and nothing else. Do not use any tools.");
        QTRY_VERIFY(client.busy());
        QTRY_VERIFY_WITH_TIMEOUT(!client.busy(), 120000);

        QString transcript;
        for (const auto &value : client.messages())
            transcript += value.toMap().value("text").toString() + "\n";
        QVERIFY2(transcript.contains("pong", Qt::CaseInsensitive), qPrintable(transcript));
        QVERIFY(transcript.contains("pong", Qt::CaseInsensitive));
    }

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(DeepCrasyTests)
#include "deepcrasy-h_tests.moc"
