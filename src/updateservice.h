#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QProcess;
class QTimer;

// Native application updater. Checks the public GitHub release endpoint on
// startup, then offers a checksum-verified package installation through pkexec.
// It never installs silently: the package manager prompt is the final consent.
class UpdateService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool checking READ checking NOTIFY changed)
    Q_PROPERTY(bool installing READ installing NOTIFY changed)
    Q_PROPERTY(bool installed READ installed NOTIFY changed)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY changed)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY changed)
    Q_PROPERTY(QString statusText READ statusText NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString lastChecked READ lastChecked NOTIFY changed)

public:
    explicit UpdateService(QObject *parent = nullptr, bool autoCheck = true);
    ~UpdateService() override;

    bool checking() const { return m_checking; }
    bool installing() const { return m_installing; }
    bool installed() const { return m_installed; }
    bool updateAvailable() const { return m_updateAvailable; }
    QString currentVersion() const;
    QString latestVersion() const { return m_latestVersion; }
    QString releaseUrl() const { return m_releaseUrl; }
    QString statusText() const;
    QString error() const { return m_error; }
    int progress() const { return m_progress; }
    QString lastChecked() const { return m_lastChecked; }

    Q_INVOKABLE void checkNow();
    Q_INVOKABLE void install();
    Q_INVOKABLE void clearError();

    static bool isNewer(const QString &candidate, const QString &current);
    QJsonObject status() const;

signals:
    void changed();
    void progressChanged();
    void errorChanged();

private:
    void setChecking(bool value);
    void setInstalling(bool value);
    void setError(const QString &error);
    void setProgress(int value);
    void parseRelease(const QByteArray &data);
    void downloadChecksum();
    void downloadPackage();
    void verifyAndInstall();
    QString cacheDirectory() const;
    QString releaseApi() const;

    QNetworkAccessManager *m_network = nullptr;
    QTimer *m_startupTimer = nullptr;
    QProcess *m_installProcess = nullptr;
    bool m_checking = false;
    bool m_installing = false;
    bool m_installed = false;
    bool m_autoCheck = true;
    bool m_updateAvailable = false;
    QString m_latestVersion;
    QString m_releaseUrl;
    QString m_packageUrl;
    QString m_checksumUrl;
    QString m_expectedChecksum;
    QString m_packagePath;
    QString m_error;
    QString m_lastChecked;
    int m_progress = 0;
};
