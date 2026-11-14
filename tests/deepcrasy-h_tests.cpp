#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

#include "config.h"
#include "ecosystem.h"
#include "workspacemodel.h"
#include "gitstatus.h"
#include "terminalsession.h"
#include "harnessserver.h"
#include "harnessconnections.h"
#include "memorystore.h"
#include "mcpmanager.h"
#include "agentmanager.h"
#include "skillmanager.h"
#include "missionrunner.h"
#include "autopilot.h"
#include "botstore.h"
#include "agentconversationstore.h"
#include "notestore.h"
#include "pluginmanager.h"
#include "webservice.h"
#include "crasyrouter.h"
#include "decisionengine.h"
#include "learningengine.h"
#include "knowledgebase.h"
#include "updateservice.h"
#include "githubservice.h"
#include "theme.h"
#include "translator.h"

class TestWebService : public WebService {
public:
    using WebService::httpGet;
};

class UltimateIAHTests : public QObject {
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

    void configFindsSystemDshRuntime()
    {
        const QString dsh = Config::detectDsh();
        QVERIFY(!dsh.isEmpty());
        QVERIFY(dsh == QString("dsh") || QFileInfo(dsh).isExecutable());
    }

    void configDefaultsAreUsable()
    {
        QTemporaryDir dir;
        Config config(dir.path() + "/config.json");
        QCOMPARE(config.harnessProfile(), QString("web"));
        QVERIFY(QFileInfo(config.workspace()).isDir());
        QVERIFY(!config.dshBinary().isEmpty());
    }

    void configPersistsEverySetting()
    {
        QTemporaryDir dir;
        const QString path = dir.path() + "/nested/config.json";
        {
            Config config(path);
            config.setWorkspace(dir.path());
            config.setHarnessProfile("custom");
            config.setView("terminal");
            config.setTheme("tokyo-night");
            config.setLanguage("fr");
            config.setStartupView("home");
            config.setAutoStartHarness(false);
        }
        Config reloaded(path);
        QCOMPARE(reloaded.workspace(), QFileInfo(dir.path()).absoluteFilePath());
        QCOMPARE(reloaded.harnessProfile(), QString("custom"));
        QCOMPARE(reloaded.view(), QString("terminal"));
        QCOMPARE(reloaded.theme(), QString("tokyo-night"));
        QCOMPARE(reloaded.language(), QString("fr"));
        QCOMPARE(reloaded.startupView(), QString("home"));
        QCOMPARE(reloaded.autoStartHarness(), false);
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
        config.setView("notes");
        config.setView("bogus");
        QCOMPARE(config.view(), QString("notes"));
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





    void memoryStoresSearchesAndChecksIntegrity()
    {
        QTemporaryDir dir;
        MemoryStore memory(dir.path() + "/memory.db");
        QCOMPARE(memory.error(), QString());
        const int id = memory.remember(dir.path(), "decision", "Choisir SQLite",
                                       "Mémoire locale avec FTS5", "db,architecture",
                                       true, 5, "test");
        QVERIFY(id > 0);
        QCOMPARE(memory.entries().size(), 1);
        QCOMPARE(memory.stats().value("total").toInt(), 1);

        memory.search("SQLite", dir.path());
        QCOMPARE(memory.results().size(), 1);
        const QVariantMap found = memory.results().at(0).toMap();
        QCOMPARE(found.value("title").toString(), QString("Choisir SQLite"));
        QVERIFY(memory.recall(dir.path(), "SQLite", 5).contains("SQLite"));

        QVERIFY(memory.togglePin(id));
        QVERIFY(memory.removeEntry(id));
        QCOMPARE(memory.entries().size(), 0);
        const QVariantMap report = memory.integrityCheck();
        QCOMPARE(report.value("ok").toBool(), true);
        QVERIFY2(memory.ensureMcpServer(), qPrintable(memory.error()));
        QVERIFY(QFileInfo(memory.mcpPatchPath()).isFile());
        const QString backup = memory.backupNow();
        QVERIFY2(!backup.isEmpty(), qPrintable(memory.error()));
        QVERIFY(QFileInfo(backup).isFile());
    }

    void mcpConfigurationsCompileToAPatch()
    {
        QTemporaryDir dir;
        McpManager mcp(dir.path() + "/mcp-servers.json");
        QVERIFY(mcp.addServer("demo", "stdio", "echo", "hello\nworld", "TOKEN=secret", "", ""));
        QCOMPARE(mcp.servers().size(), 1);
        QVERIFY(QFileInfo(mcp.patchPath()).isFile());
        QFile patch(mcp.patchPath());
        QVERIFY(patch.open(QIODevice::ReadOnly));
        const QByteArray content = patch.readAll();
        QVERIFY(content.contains("serverName: demo"));
        QVERIFY(content.contains("mcp-demo"));
        patch.close();
        QVERIFY(mcp.setEnabled("demo", false));
        QVERIFY(!QFileInfo::exists(mcp.patchPath()));
        QVERIFY(mcp.removeServer("demo"));
        QCOMPARE(mcp.servers().size(), 0);
    }

    void skillsAreCreatedAndIndexed()
    {
        QTemporaryDir dir;
        SkillManager skills(dir.path() + "/skills");
        skills.setWorkspace(dir.path());
        QVERIFY(skills.createSkill("Release Notes", "Rédige des notes claires",
                                   "Lis le diff et rédige des notes de version."));
        QCOMPARE(skills.skills().size(), 1);
        const QVariantMap skill = skills.skills().at(0).toMap();
        QCOMPARE(skill.value("name").toString(), QString("Release Notes"));
        QVERIFY(QFileInfo(skill.value("path").toString()).isFile());
        QVERIFY(skills.removeSkill(skill.value("id").toString()));
        QCOMPARE(skills.skills().size(), 0);
    }

    void missionRunsPlannerBuilderReviewer()
    {
        QTemporaryDir dir;
        const QString fake = dir.path() + "/fake-dsh";
        QFile script(fake);
        QVERIFY(script.open(QIODevice::WriteOnly));
        script.write("#!/bin/sh\ncase \"$*\" in *planificateur*) echo PLAN;; *exécutant*) echo BUILD;; *réviseur*) echo REVIEW;; *) echo $*;; esac\n");
        script.close();
        QFile::setPermissions(fake, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

        AgentManager agents(dir.path() + "/agents.json");
        MissionRunner missions(dir.path() + "/missions.db", &agents, nullptr);
        missions.setDshBinary(fake);
        const int id = missions.startMission("Test mission", "Vérifie le flux.", dir.path());
        QVERIFY(id > 0);
        QTRY_VERIFY_WITH_TIMEOUT(!missions.running(), 6000);
        const QVariantMap mission = missions.missions().at(0).toMap();
        QCOMPARE(mission.value("status").toString(), QString("done"));
        QCOMPARE(mission.value("steps").toList().size(), 3);
        for (const QVariant &step : mission.value("steps").toList())
            QCOMPARE(step.toMap().value("status").toString(), QString("done"));
        QVERIFY(mission.value("steps").toList().at(2).toMap().value("output").toString().contains("REVIEW"));
    }

    void autopilotPersistsBoundedJobs()
    {
        QTemporaryDir dir;
        Autopilot autopilot(dir.path() + "/autopilot.db");
        const int id = autopilot.addJob("Vérifier", "Lance les tests puis résume.", 0,
                                        dir.path(), 3);
        QVERIFY(id > 0);
        QCOMPARE(autopilot.jobs().size(), 1);
        QCOMPARE(autopilot.jobs().at(0).toMap().value("status").toString(), QString("pending"));
        autopilot.setEnabled(true);
        QCOMPARE(autopilot.enabled(), true);
        QVERIFY(autopilot.removeJob(id));
        QCOMPARE(autopilot.jobs().size(), 0);
    }

    void autopilotRunsAJobThroughDsh()
    {
        QTemporaryDir dir;
        const QString fake = dir.path() + "/fake-dsh";
        QFile script(fake);
        QVERIFY(script.open(QIODevice::WriteOnly));
        script.write("#!/bin/sh\necho \"AUTOPILOT_OK $2\"\n");
        script.close();
        QFile::setPermissions(fake, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

        Autopilot autopilot(dir.path() + "/autopilot.db");
        autopilot.setDshBinary(fake);
        QSignalSpy completed(&autopilot, &Autopilot::completed);
        const int id = autopilot.addJob("Test", "Vérifie le runner.", 0, dir.path(), 1);
        QVERIFY(id > 0);
        autopilot.runNow(id);
        QTRY_VERIFY_WITH_TIMEOUT(!autopilot.running(), 5000);
        QCOMPARE(completed.count(), 1);
        const QVariantMap job = autopilot.jobs().at(0).toMap();
        QCOMPARE(job.value("status").toString(), QString("done"));
        QVERIFY(job.value("lastOutput").toString().contains("AUTOPILOT_OK"));
    }

    void connectionsPersistHarnessProviders()
    {
        QTemporaryDir dir;
        qputenv("DSH_HOME", dir.path().toUtf8());
        QFile creds(dir.path() + "/.credentials.yaml");
        QVERIFY(creds.open(QIODevice::WriteOnly));
        creds.write("version: 1\nrefs: {}\nrecords: {}\n");
        creds.close();
        const QString overlay = dir.path() + "/connections.patch.yml";

        HarnessConnections connections(overlay);
        connections.saveProvider("mock", "Mock", "http://127.0.0.1:18080/v1",
                                 "secret-key", "mock-model=Mock Model", true);
        QCOMPARE(connections.error(), QString());
        QCOMPARE(connections.providers().size(), 1);
        const QVariantMap provider = connections.providers().at(0).toMap();
        QCOMPARE(provider.value("id").toString(), QString("mock"));
        QCOMPARE(provider.value("hasKey").toBool(), true);
        QCOMPARE(connections.defaultModel(), QString("mock/mock-model"));

        QFile agentEnv(dir.path() + "/agent.env");
        QVERIFY(agentEnv.open(QIODevice::ReadOnly));
        QVERIFY(agentEnv.readAll().contains("HARNESS_MOCK_API_KEY=secret-key"));
        agentEnv.close();

        QFile written(dir.path() + "/.credentials.yaml");
        QVERIFY(written.open(QIODevice::ReadOnly));
        QVERIFY(written.readAll().contains("HARNESS_MOCK_API_KEY"));
        written.close();

        connections.removeProvider("mock");
        QCOMPARE(connections.providers().size(), 0);
        qunsetenv("DSH_HOME");
    }

    void providerDetectionReadsModelsFromTheApi()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QByteArray previousHome = qgetenv("DSH_HOME");
        qputenv("DSH_HOME", dir.path().toUtf8());
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        auto replied = std::make_shared<bool>(false);
        connect(&server, &QTcpServer::newConnection, &server, [&server, replied] {
            QTcpSocket *socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [socket, replied] {
                if (*replied) return;
                *replied = true;
                socket->readAll();
                const QByteArray body = "{\"data\":[{\"id\":\"alpha\"},{\"id\":\"beta\",\"name\":\"Beta\"}]}";
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
                              + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->flush();
            });
        });
        HarnessConnections connections(dir.path() + "/connections.patch.yml");
        QSignalSpy spy(&connections, &HarnessConnections::detectedModelsChanged);
        connections.detectModelsFor(QString("http://127.0.0.1:%1/v1").arg(server.serverPort()),
                                    "test-key", "openai-completions");
        QVERIFY(spy.wait(8000));
        QCOMPARE(connections.detectedModels().size(), 2);
        QCOMPARE(connections.detectedModels().at(0).toMap().value("id").toString(), QString("alpha"));
        QCOMPARE(connections.detectedModels().at(1).toMap().value("name").toString(), QString("Beta"));
        if (previousHome.isEmpty())
            qunsetenv("DSH_HOME");
        else
            qputenv("DSH_HOME", previousHome);
    }

    void providerModelDetectionParsesCommonShapes()
    {
        const QJsonObject openai{{"data", QJsonArray{
            QJsonObject{{"id", "alpha"}},
            QJsonObject{{"id", "alpha"}},
            QJsonObject{{"id", "beta"}, {"name", "Beta"}}}}};
        const QJsonArray models = HarnessConnections::parseModels(openai);
        QCOMPARE(models.size(), 2);
        QCOMPARE(models.at(0).toObject().value("id").toString(), QString("alpha"));
        QCOMPARE(models.at(1).toObject().value("name").toString(), QString("Beta"));
        const QJsonObject gemini{{"models", QJsonArray{
            QJsonObject{{"name", "models/gemini-2.5-pro"}, {"display_name", "Gemini 2.5 Pro"}}}}};
        const QJsonArray geminiModels = HarnessConnections::parseModels(gemini);
        QCOMPARE(geminiModels.size(), 1);
        QCOMPARE(geminiModels.at(0).toObject().value("id").toString(), QString("gemini-2.5-pro"));
        QCOMPARE(geminiModels.at(0).toObject().value("name").toString(), QString("Gemini 2.5 Pro"));
    }

    void deepSeekCreatesAUsableDefaultProvider()
    {
        QTemporaryDir dir;
        qputenv("DSH_HOME", dir.path().toUtf8());
        QFile creds(dir.path() + "/.credentials.yaml");
        QVERIFY(creds.open(QIODevice::WriteOnly));
        creds.write("version: 1\nrefs: {}\nrecords: {}\n");
        creds.close();
        const QString overlay = dir.path() + "/connections.patch.yml";
        HarnessConnections connections(overlay);
        connections.saveDeepSeek("sk-test-deepseek");
        QCOMPARE(connections.error(), QString());
        QCOMPARE(connections.defaultProvider(), QString("deepseek-official"));
        QCOMPARE(connections.defaultModelId(), QString("deepseek-chat"));
        bool found = false;
        for (const QVariant &value : connections.providers()) {
            const QVariantMap provider = value.toMap();
            if (provider.value("id").toString() == "deepseek-official") {
                found = true;
                QVERIFY(provider.value("builtin").toBool());
                QVERIFY(provider.value("models").toStringList().contains("deepseek-chat"));
            }
        }
        QVERIFY(found);
        QFile written(overlay);
        QVERIFY(written.open(QIODevice::ReadOnly));
        const QByteArray yaml = written.readAll();
        QVERIFY(yaml.contains("deepseek-official"));
        QVERIFY(yaml.contains("DEEPSEEK_API_KEY"));
        qunsetenv("DSH_HOME");
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

    void retiredServicesAreAbsentFromBundledUi()
    {
        QDirIterator it(QStringLiteral(":/"), QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            if (!(path.endsWith(QStringLiteral(".qml")) || path.endsWith(QStringLiteral(".json"))
                  || path.endsWith(QStringLiteral(".md")))) {
                continue;
            }
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                continue;
            const QString text = QString::fromUtf8(file.readAll());
            const QString retiredRouter = QStringLiteral("9") + QStringLiteral("Router");
            const QString retiredAgent = QStringLiteral("Maus") + QStringLiteral("Bot");
            QVERIFY2(!text.contains(retiredRouter, Qt::CaseInsensitive), qPrintable(path));
            QVERIFY2(!text.contains(retiredAgent, Qt::CaseInsensitive), qPrintable(path));
        }
    }

    void botStorePersistsRegistryRunsAndSchedules()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        BotStore store(dir.path() + "/bots.db");
        QVERIFY2(store.error().isEmpty(), qPrintable(store.error()));
        QCOMPARE(store.bots().size(), 3);

        const int id = store.createBot("Scout", "Veille et synthèse", "Tu explores et tu résumes.",
                                       "read-only", "deepseek-official", "deepseek-chat");
        QVERIFY(id > 0);
        QCOMPARE(store.selectedBotId(), id);
        QVERIFY(store.messages().isEmpty());
        QVariantMap created;
        for (const QVariant &value : store.bots()) {
            if (value.toMap().value("id").toInt() == id)
                created = value.toMap();
        }
        QCOMPARE(created.value("provider").toString(), QString("deepseek-official"));
        QCOMPARE(created.value("model").toString(), QString("deepseek-chat"));

        const int message = store.appendMessage(id, "user", "Analyse le workspace", "chat");
        QVERIFY(message > 0);
        QCOMPARE(store.messages().size(), 1);
        const int run = store.createRun(id, "Analyse le workspace");
        QVERIFY(run > 0);
        QVERIFY(store.updateRun(run, "done", "Résultat vérifié", "", 0));
        const QVariantMap finished = store.runById(run);
        QCOMPARE(finished.value("status").toString(), QString("done"));
        QCOMPARE(finished.value("output").toString(), QString("Résultat vérifié"));

        QVERIFY(store.updateBot(id, "Scout", "Veille et synthèse", "Tu explores et tu résumes.",
                                "read-only", true, "openrouter", "deepseek/deepseek-chat"));
        QVariantMap updated;
        for (const QVariant &value : store.bots()) {
            if (value.toMap().value("id").toInt() == id)
                updated = value.toMap();
        }
        QCOMPARE(updated.value("provider").toString(), QString("openrouter"));
        QCOMPARE(updated.value("model").toString(), QString("deepseek/deepseek-chat"));

        const int schedule = store.createSchedule(id, "Veille", "Vérifie le workspace", 30);
        QVERIFY(schedule > 0);
        QCOMPARE(store.schedules().size(), 1);
        QVERIFY(store.toggleSchedule(schedule, false));
        QVERIFY(store.removeSchedule(schedule));
        QVERIFY(store.removeBot(id));
        QCOMPARE(store.bots().size(), 3);
    }

    void agentConversationStorePersistsSearchesAndDeletes()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        AgentConversationStore store(dir.path() + "/conversations.db");
        QVERIFY2(store.error().isEmpty(), qPrintable(store.error()));
        const QString first = store.create("Première discussion", "deepseek-chat", "DeepSeek");
        QVERIFY(!first.isEmpty());
        QCOMPARE(store.conversations().size(), 1);
        const QJsonArray ui{{QJsonObject{{"role", "user"}, {"text", "Bonjour"}, {"time", "10:00"}}},
                            {QJsonObject{{"role", "assistant"}, {"text", "Salut"}, {"time", "10:01"}}}};
        const QJsonArray api{{QJsonObject{{"role", "user"}, {"content", "Bonjour"}}},
                             {QJsonObject{{"role", "assistant"}, {"content", "Salut"}}}};
        QVERIFY(store.save(first, "Première discussion", api, ui, "deepseek-chat", "DeepSeek"));
        QCOMPARE(store.conversations().size(), 1);
        const QVariantMap summary = store.conversations().first().toMap();
        QCOMPARE(summary.value("messageCount").toInt(), 2);
        QVERIFY(summary.value("preview").toString().contains("Salut"));
        QVERIFY(store.rename(first, "Discussion renommée"));
        QVERIFY(store.setPinned(first, true));
        QCOMPARE(store.conversations().first().toMap().value("title").toString(), QString("Discussion renommée"));
        QVERIFY(store.conversations("renommée").size() == 1);
        QVERIFY(store.conversations("absente").isEmpty());
        QCOMPARE(store.lastConversationId(), first);
        const QJsonObject loaded = store.conversation(first);
        QCOMPARE(loaded.value("ui").toArray().size(), 2);
        QVERIFY(store.remove(first));
        QVERIFY(store.conversations().isEmpty());
        QVERIFY(store.lastConversationId().isEmpty());
    }

    void noteStoreBuildsLinksTagsAndGraph()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        NoteStore store(dir.path() + "/vault");
        QVERIFY2(store.error().isEmpty(), qPrintable(store.error()));
        QVERIFY(store.count() >= 1);
        const QString alpha = store.createNote("Alpha");
        QVERIFY(!alpha.isEmpty());
        QVERIFY(store.saveNote(alpha, "# Alpha\n\nVoir [[Beta]] #projet #test"));
        const QString beta = store.createNote("Beta");
        QVERIFY(!beta.isEmpty());
        QVERIFY(store.saveNote(beta, "# Beta\n\nRetour vers [[Alpha]] #projet"));
        QVERIFY(store.renameNote(beta, "Beta renommée"));
        QVERIFY(store.searchNotes("Alpha").size() >= 1);
        QVERIFY(store.tags().size() >= 2);
        store.selectNote(alpha);
        const QVariantList backlinks = store.backlinks();
        QVERIFY(backlinks.size() >= 1);
        QVERIFY(store.graph().value("nodes").toList().size() >= 3);
        QVERIFY(store.graph().value("edges").toList().size() >= 1);
        QVERIFY(store.deleteNote(alpha));
        QVERIFY(store.count() >= 2);
        QVERIFY(QFileInfo::exists(dir.path() + "/vault/.ultimate-iai/trash"));
    }

    void noteStoreSupportsFoldersOutlineAndSearchFilters()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        NoteStore store(dir.path() + "/vault");
        const QString project = store.createNote("Projet", "docs");
        QVERIFY(project.startsWith("docs/"));
        QVERIFY(store.saveNote(project, "# Projet\n\n## Section\n\nVoir [[Autre]] #ia"));
        const QString other = store.createNote("Autre");
        QVERIFY(store.saveNote(other, "# Autre\n\nRetour [[Projet]] #ia"));
        store.selectNote(project);
        QVERIFY(store.headings().size() >= 2);
        QCOMPARE(store.headings().first().toMap().value("level").toInt(), 1);
        QVERIFY(store.outgoingLinks().size() >= 1);
        QCOMPARE(store.outgoingLinks().first().toMap().value("exists").toBool(), true);
        bool foundFolder = false;
        for (const QVariant &value : store.folders())
            if (value.toMap().value("path").toString() == "docs")
                foundFolder = true;
        QVERIFY(foundFolder);

        const QString copy = store.duplicateNote(project);
        QVERIFY(!copy.isEmpty());
        QVERIFY(store.moveNote(copy, "archive"));
        QVERIFY(QFileInfo::exists(dir.path() + "/vault/archive/" + QFileInfo(copy).fileName()));

        store.setActiveFolder("docs");
        QCOMPARE(store.filteredNotes().size(), 1);
        store.setActiveFolder({});
        store.setQuery("tag:ia");
        QVERIFY(store.filteredNotes().size() >= 2);
        store.setQuery("title:Autre");
        QCOMPARE(store.filteredNotes().size(), 1);
        store.setQuery({});
    }

    void defaultSkillLibraryIsSeededAndSearchable()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        SkillManager skills(dir.path() + "/skills");
        skills.setWorkspace(dir.path());
        QVERIFY(skills.seedDefaults());
        QCOMPARE(skills.defaultCount(), 42);
        QCOMPARE(skills.packs().size(), 3);
        const QVariantMap leanrea = skills.readSkill("leanrea");
        QCOMPARE(leanrea.value("pack").toString(), QString("leanrea"));
        QVERIFY(leanrea.value("content").toString().contains("Reverse engineer"));
        const QVariantMap research = skills.readSkill("research");
        QCOMPARE(research.value("id").toString(), QString("research"));
        QVERIFY(research.value("content").toString().contains("web_search"));
        const QVariantList suggestions = skills.suggest("recherche web sources", 2);
        QVERIFY(!suggestions.isEmpty());
        QCOMPARE(suggestions.first().toMap().value("id").toString(), QString("research"));
        QCOMPARE(skills.skills().size(), 42);
        const QVariantMap dsWatch = skills.readSkill("ds-watch-pr");
        QCOMPARE(dsWatch.value("pack").toString(), QString("deepseek-stack"));
        const QString script = QFileInfo(dsWatch.value("path").toString()).absolutePath()
            + "/scripts/watch_pr.py";
        QVERIFY2(QFileInfo::exists(script), qPrintable(script));
    }

    void webFetchBlocksPrivateHosts()
    {
        WebService web;
        const QJsonObject local = web.fetch("http://127.0.0.1:8080/private");
        QVERIFY(!local.value("ok").toBool());
        QCOMPARE(local.value("code").toString(), QString("WEB_PRIVATE_HOST"));
        const QJsonObject loopback = web.fetch("http://localhost/private");
        QVERIFY(!loopback.value("ok").toBool());
        QCOMPARE(loopback.value("code").toString(), QString("WEB_PRIVATE_HOST"));
        const QJsonObject dotted = web.fetch("http://localhost.:9/private");
        QVERIFY(!dotted.value("ok").toBool());
        QCOMPARE(dotted.value("code").toString(), QString("WEB_PRIVATE_HOST"));
        const QJsonObject mapped = web.fetch("http://[::ffff:127.0.0.1]:9/private");
        QVERIFY(!mapped.value("ok").toBool());
        QCOMPARE(mapped.value("code").toString(), QString("WEB_PRIVATE_HOST"));
    }

    void webFetchStopsAtTheByteCap()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        auto replied = std::make_shared<bool>(false);
        connect(&server, &QTcpServer::newConnection, &server, [&server, replied] {
            QTcpSocket *socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [socket, replied] {
                if (*replied) return;
                *replied = true;
                socket->readAll();
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 1000000\r\n\r\n");
                socket->write(QByteArray(5000, 'a'));
                socket->flush();
            });
        });
        TestWebService web;
        const qint64 started = QDateTime::currentMSecsSinceEpoch();
        const QJsonObject reply = web.httpGet(
            QStringLiteral("http://127.0.0.1:%1/").arg(server.serverPort()), 1000, 3000);
        const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - started;
        QVERIFY2(elapsed < 2500, qPrintable(QStringLiteral("waited %1 ms").arg(elapsed)));
        QCOMPARE(reply.value("status").toInt(), 200);
        QVERIFY(reply.value("truncated").toBool());
        QCOMPARE(reply.value("body").toString().toUtf8().size(), 1000);
    }

    void updateVersionComparisonRejectsDowngrades()
    {
        QVERIFY(UpdateService::isNewer("1.17.0", "1.16.3"));
        QVERIFY(UpdateService::isNewer("v1.2", "1.1.9"));
        QVERIFY(!UpdateService::isNewer("1.16.3", "1.17.0"));
        QVERIFY(!UpdateService::isNewer("1.17.0", "1.17.0"));
        QVERIFY(!UpdateService::isNewer("1.17.0-rc1", "1.17.0"));
        QVERIFY(UpdateService::isNewer("1.17.0", "1.17.0-rc1"));
        QVERIFY(UpdateService::isNewer("1.17.0-rc2", "1.17.0-rc1"));
        QVERIFY(!UpdateService::isNewer("1.17.0-rc1", "1.17.0-rc2"));
        QVERIFY(!UpdateService::isNewer("1.17.0-beta", "1.17.0-rc1"));
        QVERIFY(UpdateService::isNewer("1.17.0-rc1", "1.17.0-beta"));
        QVERIFY(UpdateService::isNewer("1.17.0-rc.10", "1.17.0-rc.2"));
        QVERIFY(!UpdateService::isNewer("1.17.0+build1", "1.17.0"));
        QVERIFY(UpdateService::isNewer("1.22.0.1", "1.22.0"));
        QVERIFY(!UpdateService::isNewer("1.22.0", "1.22.0.1"));
    }

    void gitHubPullRequestArgumentsAreExplicit()
    {
        const QStringList full = GitHubCli::pullRequestArguments(
            "owner/repo", "  Fix the bug  ", "Body with 'quotes' and $shell", "", "feat/branch");
        QCOMPARE(full, QStringList({"pr", "create", "--repo", "owner/repo", "--title", "Fix the bug",
                                    "--body", "Body with 'quotes' and $shell", "--base", "main",
                                    "--head", "feat/branch"}));
        const QStringList local = GitHubCli::pullRequestArguments(
            "owner/repo", "Title", "", "develop", "  ");
        QCOMPARE(local, QStringList({"pr", "create", "--repo", "owner/repo", "--title", "Title",
                                     "--body", "", "--base", "develop"}));
        QVERIFY(!local.contains("--head"));
    }

    void decisionEngineResolvesProvidersAndModels()
    {
        DecisionEngine::ProviderInfo provider{
            QStringLiteral("harness"), QStringLiteral("Harness"),
            QStringLiteral("https://example.test/v1"),
            {QStringLiteral("model-chat"), QStringLiteral("model-pro")},
            QStringLiteral("key"), false};
        DecisionEngine::Context context;
        context.providers = {provider};
        context.defaultProvider = QStringLiteral("harness");
        context.defaultModel = QStringLiteral("model-chat");
        context.strategy = QStringLiteral("balanced");
        DecisionEngine::Endpoint endpoint = DecisionEngine::resolve(context);
        QVERIFY(endpoint.valid());
        QCOMPARE(endpoint.providerId, QString("harness"));
        QCOMPARE(endpoint.source, QString("harness"));
        QCOMPARE(endpoint.model, QString("model-chat"));
        context.defaultModel.clear();
        context.strategy = QStringLiteral("quality");
        endpoint = DecisionEngine::resolve(context);
        QCOMPARE(endpoint.model, QString("model-pro"));

        DecisionEngine::Context deepSeek;
        deepSeek.deepSeekKey = QStringLiteral("sk-test");
        deepSeek.strategy = QStringLiteral("quality");
        endpoint = DecisionEngine::resolve(deepSeek);
        QCOMPARE(endpoint.source, QString("deepseek"));
        QCOMPARE(endpoint.model, QString("deepseek-reasoner"));
        QCOMPARE(endpoint.baseUrl, QString("https://api.deepseek.com/v1"));

        DecisionEngine::Context router;
        router.routerEndpoint = QStringLiteral("http://127.0.0.1:8788/v1");
        router.routerToken = QStringLiteral("token");
        router.source = QStringLiteral("4crasy");
        endpoint = DecisionEngine::resolve(router);
        QCOMPARE(endpoint.source, QString("4crasy"));
        QCOMPARE(endpoint.model, QString("auto"));

        DecisionEngine::Context empty;
        QVERIFY(!DecisionEngine::resolve(empty).valid());
    }

    void decisionEngineBuildsFallbackChain()
    {
        DecisionEngine::Context context;
        context.deepSeekKey = QStringLiteral("sk-test");
        context.routerEndpoint = QStringLiteral("http://127.0.0.1:8788/v1");
        context.routerToken = QStringLiteral("token");
        DecisionEngine::Endpoint endpoint =
            DecisionEngine::fallbackFrom(context, {QStringLiteral("harness")});
        QCOMPARE(endpoint.source, QString("deepseek"));
        endpoint = DecisionEngine::fallbackFrom(
            context, {QStringLiteral("harness"), QStringLiteral("deepseek")});
        QCOMPARE(endpoint.source, QString("4crasy"));
        QCOMPARE(endpoint.model, QString("auto"));
        QVERIFY(!DecisionEngine::fallbackFrom(
            context, {QStringLiteral("harness"), QStringLiteral("deepseek"),
                      QStringLiteral("4crasy")}).valid());
    }

    void decisionEngineAppliesDecisionRules()
    {
        DecisionEngine engine;
        engine.setStrategy("balanced");
        engine.setAutonomy("assisted");
        engine.setPermissionCeiling("workspace-write");
        engine.setMaxRounds(5);
        engine.setMaxRetries(2);
        engine.setAllowFallback(true);
        engine.setReviewChanges(true);
        DecisionEngine::Decision decision = engine.decide("fix", "high", "build", 1);
        QCOMPARE(decision.permission, QString("workspace-write"));
        QVERIFY(decision.review);
        QVERIFY(decision.fallback);
        QCOMPARE(decision.retries, 2);
        QVERIFY(!decision.stop);

        engine.setPermissionCeiling("read-only");
        decision = engine.decide("inspect", "low", "diagnostic", 1);
        QCOMPARE(decision.permission, QString("read-only"));

        engine.setPermissionCeiling("full");
        engine.setAutonomy("auto");
        decision = engine.decide("ship", "low", "build", 1);
        QCOMPARE(decision.permission, QString("full"));

        engine.setMaxRounds(2);
        decision = engine.decide("again", "low", "build", 2);
        QVERIFY(decision.stop);
    }

    void crasyRouterPersistsChannelAndRoutingConfig()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        CrasyRouter router(dir.path());
        router.setAutoRefresh(false);
        QVERIFY(router.saveChannel("custom", "Custom", "https://example.com/v1",
                                   "CUSTOM_KEY", "secret", 7, false, true));
        QCOMPARE(router.channels().size(), 9);
        bool found = false;
        for (const QVariant &value : router.channels()) {
            const QVariantMap channel = value.toMap();
            if (channel.value("id").toString() == "custom") {
                found = true;
                QCOMPARE(channel.value("priority").toInt(), 7);
                QVERIFY(!channel.value("free").toBool());
                QVERIFY(channel.value("hasKey").toBool());
            }
        }
        QVERIFY(found);
        router.setPort(8899);
        router.setFreeOnly(true);
        router.setMaxFallbacks(2);
        router.setStrategy("quality");
        QCOMPARE(router.port(), 8899);
        QVERIFY(router.freeOnly());
        QCOMPARE(router.maxFallbacks(), 2);
        QCOMPARE(router.strategy(), QString("quality"));
        router.saveSettings(8899, true, 2, true, "quality");
        QVERIFY(router.autoStart());
        CrasyRouter reloaded(dir.path());
        QVERIFY(reloaded.autoStart());
        QCOMPARE(reloaded.port(), 8899);
        QVERIFY(router.removeChannel("custom"));
        QCOMPARE(router.channels().size(), 8);
    }

    void knowledgeBaseRegistersToolsAndSearches()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        KnowledgeBase knowledge(dir.path() + "/knowledge.db");
        const QJsonArray tools = {QJsonObject{{"type", "function"},
            {"function", QJsonObject{{"name", "utility_probe"},
                                     {"description", "Probe an installed capability."},
                                     {"parameters", QJsonObject{{"type", "object"}}}}}}};
        const QVariantList skills = {QVariantMap{{"id", "probe-skill"}, {"name", "Probe Skill"},
                                                  {"description", "A test skill."}, {"pack", "core"}}};
        knowledge.seed(tools, {}, skills);
        QVERIFY(knowledge.stats().value("entries").toInt() >= 14);
        QCOMPARE(knowledge.stats().value("tools").toInt(), 1);
        const QVariantList found = knowledge.search("utility_probe", 5);
        QVERIFY(!found.isEmpty());
        QCOMPARE(found.first().toMap().value("category").toString(), QString("tool"));
        QVERIFY(knowledge.promptContext("utility_probe", 6500).contains("utility_probe"));
    }

    void learningEngineExtractsLessonsAndProposesSkills()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        SkillManager skills(dir.path() + "/skills");
        skills.setWorkspace(dir.path());
        LearningEngine learning(dir.path() + "/learning.db", &skills);
        QCOMPARE(learning.stats().value("events").toInt(), 0);
        QJsonObject trajectory{
            {"prompt", "Corrige le bug web et vérifie le résultat"},
            {"project", dir.path()},
            {"ok", false},
            {"error", "Redirection refusée"},
            {"model", "test-model"},
            {"tools", QJsonArray{QJsonObject{{"name", "web_fetch"}, {"ok", false},
                                            {"detail", "Redirection refusée"}}}},
        };
        QVERIFY(learning.observe(trajectory).value("learned").toBool());
        QCOMPARE(learning.stats().value("events").toInt(), 1);
        QCOMPARE(learning.stats().value("lessons").toInt(), 1);
        QVERIFY(learning.observe(trajectory).value("learned").toBool());
        QCOMPARE(learning.stats().value("proposals").toInt(), 1);
        const QString context = learning.contextFor("corrige le bug web", dir.path(), 4);
        QVERIFY(context.contains("LEÇONS APPRISES"));
        const QVariantList proposals = learning.proposals();
        QVERIFY(!proposals.isEmpty());
        const int proposalId = proposals.first().toMap().value("id").toInt();
        QVERIFY(learning.approveProposal(proposalId));
        QVERIFY(skills.skills().size() >= 1);
        const QVariantMap learnedSkill = skills.skills().first().toMap();
        QVERIFY(!learnedSkill.value("name").toString().contains(QStringLiteral("proposé")));
        QVERIFY(!learnedSkill.value("id").toString().startsWith(QStringLiteral("skill-propose")));
        QVERIFY(learning.clear());
        QCOMPARE(learning.stats().value("events").toInt(), 0);
    }

    void crasyRouterStartsLoopbackApiAndParsesModels()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        CrasyRouter router(dir.path());
        router.setAutoRefresh(false);
        QVERIFY(router.start(0));
        QVERIFY(router.running());
        QVERIFY(router.endpoint().startsWith("http://127.0.0.1:"));
        QVERIFY(router.token().size() >= 24);
        router.stop();
        QVERIFY(!router.running());
        const QJsonArray models{
            QJsonObject{{"id", "free/model"}, {"pricing", QJsonObject{{"prompt", "0"}, {"completion", "0"}}}},
            QJsonObject{{"id", "paid/model"}, {"pricing", QJsonObject{{"prompt", "0.1"}, {"completion", "0.2"}}}},
        };
        const QVariantList parsed = CrasyRouter::parseModels(models, "test");
        QCOMPARE(parsed.size(), 2);
        QVERIFY(parsed.at(0).toMap().value("free").toBool());
        QVERIFY(!parsed.at(1).toMap().value("free").toBool());
    }

    void pluginManagerRejectsUnknownArtcraftCraft()
    {
        QTemporaryDir dir;
        PluginManager plugins(dir.path());
        plugins.installArtcraftCraft("does-not-exist");
        QVERIFY(!plugins.error().isEmpty());
        QVERIFY(!plugins.artcraftBusy());
    }

    void pluginManagerDiscoversNewArtcraftCrafts()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QByteArray previous = qgetenv("ULTIMATE_IAH_ARTCRAFT_DIR");
        const QString bin = dir.path() + "/artcraft";
        QVERIFY(QDir().mkpath(bin));
        QFile fake(bin + "/newcraft-cli");
        QVERIFY(fake.open(QIODevice::WriteOnly));
        fake.write("#!/bin/sh\nexit 0\n");
        fake.write(QByteArray(1100 * 1024, '\0'));
        fake.close();
        QFile::setPermissions(fake.fileName(),
                              QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        qputenv("ULTIMATE_IAH_ARTCRAFT_DIR", bin.toUtf8());
        PluginManager plugins(dir.path() + "/config");
        bool found = false;
        for (const QVariant &value : plugins.plugins()) {
            const QVariantMap plugin = value.toMap();
            if (plugin.value("id").toString() == "newcraft") {
                found = true;
                QCOMPARE(plugin.value("toolName").toString(), QString("artcraft_newcraft"));
                QVERIFY(plugin.value("installed").toBool());
            }
        }
        QVERIFY(found);
        if (previous.isEmpty())
            qunsetenv("ULTIMATE_IAH_ARTCRAFT_DIR");
        else
            qputenv("ULTIMATE_IAH_ARTCRAFT_DIR", previous);
    }

    void pluginManagerDiscoversAndRunsExternalCommands()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir().mkpath(dir.path() + "/manifests"));
        QFile manifest(dir.path() + "/manifests/echo.json");
        QVERIFY(manifest.open(QIODevice::WriteOnly));
        manifest.write(R"({"id":"echo","name":"Echo","description":"Echo tool","command":"/bin/echo","args":["hello"]})");
        manifest.close();
        PluginManager plugins(dir.path());
        QVERIFY2(plugins.error().isEmpty(), qPrintable(plugins.error()));
        QCOMPARE(plugins.installedCount(), 1);
        QCOMPARE(plugins.enabledCount(), 1);
        QCOMPARE(plugins.toolCount(), 1);
        const QJsonArray tools = plugins.toolDefinitions();
        QCOMPARE(tools.size(), 1);
        QVERIFY(plugins.isPluginTool("plugin_echo"));
        const QJsonObject result = plugins.executeTool("plugin_echo",
            QJsonObject{{"action", "run"}, {"arguments", "plugin-ok"}}, dir.path(), "workspace-write");
        QVERIFY2(result.value("ok").toBool(), qPrintable(result.value("error").toString()));
        QVERIFY(result.value("output").toString().contains("plugin-ok"));
        QVERIFY(plugins.setEnabled("echo", false));
        QCOMPARE(plugins.toolCount(), 0);
    }

    void originalOneShotPolicyIsEmbedded()
    {
        QFile behavior(QStringLiteral(":/agent/BEHAVIOR.md"));
        QVERIFY(behavior.open(QIODevice::ReadOnly));
        const QString text = QString::fromUtf8(behavior.readAll());
        QVERIFY(text.contains(QStringLiteral("One-shot operating policy")));
        QVERIFY(text.contains(QStringLiteral("Read the actual output")));
        QVERIFY(text.contains(QStringLiteral("Hover is a colour/opacity transition only")));
    }

    void englishCatalogCoversFrenchCatalog()
    {
        QFile french(QStringLiteral(":/i18n/fr.json"));
        QFile english(QStringLiteral(":/i18n/en.json"));
        QVERIFY(french.open(QIODevice::ReadOnly));
        QVERIFY(english.open(QIODevice::ReadOnly));
        const QJsonObject fr = QJsonDocument::fromJson(french.readAll()).object();
        const QJsonObject en = QJsonDocument::fromJson(english.readAll()).object();
        QVERIFY(fr.size() > 600);
        for (auto it = fr.begin(); it != fr.end(); ++it)
            QVERIFY2(en.contains(it.key()), qPrintable(it.key()));
        QVERIFY(en.value(QStringLiteral("%n file(s)")).isObject());
        QCOMPARE(en.value(QStringLiteral("%n file(s)")).toObject().value("one").toString(),
                 QStringLiteral("%n file"));
        QCOMPARE(en.value(QStringLiteral("%n file(s)")).toObject().value("other").toString(),
                 QStringLiteral("%n files"));
    }

    void translationSwitchesBetweenEnglishAndFrench()
    {
        Translator translator(QString(), QStringLiteral(":/i18n"));
        translator.setOverride("fr");
        QCOMPARE(translator.translate("Main", "Réglages"), QString("Réglages"));
        QCOMPARE(translator.translate("Main", "Aucune tâche autonome"), QString("Aucune tâche autonome"));
        translator.setOverride("en");
        QCOMPARE(translator.translate("Main", "Réglages"), QString("Settings"));
        QCOMPARE(translator.translate("Main", "Aucune tâche autonome"), QString("No autonomous task"));
        QCOMPARE(translator.translate("Main", "%n file(s)", nullptr, 2), QString("%n files"));
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

private:
    static void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(contents);
    }
};

QTEST_MAIN(UltimateIAHTests)
#include "deepcrasy-h_tests.moc"
