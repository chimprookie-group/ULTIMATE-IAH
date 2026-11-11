#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "acpclient.h"
#include "config.h"
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
        }
        Config reloaded(path);
        QCOMPARE(reloaded.workspace(), QFileInfo(dir.path()).absoluteFilePath());
        QCOMPARE(reloaded.harnessProfile(), QString("custom"));
        QCOMPARE(reloaded.view(), QString("split"));
        QCOMPARE(reloaded.theme(), QString("tokyo-night"));
        QCOMPARE(reloaded.language(), QString("fr"));
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
