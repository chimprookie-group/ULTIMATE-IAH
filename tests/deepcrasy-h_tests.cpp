#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

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
#include "theme.h"
#include "translator.h"

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
        config.setView("harness");
        config.setView("bogus");
        QCOMPARE(config.view(), QString("harness"));
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

        const int id = store.createBot("Scout", "Veille et synthèse", "Tu explores et tu résumes.", "read-only");
        QVERIFY(id > 0);
        QCOMPARE(store.selectedBotId(), id);
        QVERIFY(store.messages().isEmpty());

        const int message = store.appendMessage(id, "user", "Analyse le workspace", "chat");
        QVERIFY(message > 0);
        QCOMPARE(store.messages().size(), 1);
        const int run = store.createRun(id, "Analyse le workspace");
        QVERIFY(run > 0);
        QVERIFY(store.updateRun(run, "done", "Résultat vérifié", "", 0));
        const QVariantMap finished = store.runById(run);
        QCOMPARE(finished.value("status").toString(), QString("done"));
        QCOMPARE(finished.value("output").toString(), QString("Résultat vérifié"));

        const int schedule = store.createSchedule(id, "Veille", "Vérifie le workspace", 30);
        QVERIFY(schedule > 0);
        QCOMPARE(store.schedules().size(), 1);
        QVERIFY(store.toggleSchedule(schedule, false));
        QVERIFY(store.removeSchedule(schedule));
        QVERIFY(store.removeBot(id));
        QCOMPARE(store.bots().size(), 3);
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
