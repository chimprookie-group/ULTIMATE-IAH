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
        }
        Config reloaded(path);
        QCOMPARE(reloaded.workspace(), QFileInfo(dir.path()).absoluteFilePath());
        QCOMPARE(reloaded.harnessProfile(), QString("custom"));
        QCOMPARE(reloaded.view(), QString("split"));
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
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
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

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
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
