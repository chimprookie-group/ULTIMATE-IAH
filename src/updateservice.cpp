#include "updateservice.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

UpdateService::UpdateService(QObject *parent, bool autoCheck)
    : QObject(parent), m_network(new QNetworkAccessManager(this)), m_autoCheck(autoCheck) {
    if (m_autoCheck) {
        m_startupTimer = new QTimer(this);
        m_startupTimer->setSingleShot(true);
        m_startupTimer->setInterval(1400);
        connect(m_startupTimer, &QTimer::timeout, this, &UpdateService::checkNow);
        m_startupTimer->start();
    }
}

UpdateService::~UpdateService() {
    if (m_installProcess && m_installProcess->state() != QProcess::NotRunning) {
        m_installProcess->kill();
        m_installProcess->waitForFinished(1000);
    }
}

QString UpdateService::currentVersion() const {
    return QCoreApplication::applicationVersion();
}

QString UpdateService::releaseApi() const {
    return QStringLiteral("https://api.github.com/repos/chimprookie-group/ULTIMATE-IAH/releases/latest");
}

QString UpdateService::statusText() const {
    if (m_installed)
        return tr("Installation terminée · redémarrez Ultimate IAH");
    if (m_installing)
        return tr("Installation en cours…");
    if (m_checking)
        return tr("Vérification des mises à jour…");
    if (m_updateAvailable)
        return tr("Mise à jour %1 disponible").arg(m_latestVersion);
    if (!m_latestVersion.isEmpty())
        return tr("À jour · %1").arg(currentVersion());
    return tr("Mises à jour non vérifiées");
}

bool UpdateService::isNewer(const QString &candidate, const QString &current) {
    auto parts = [](QString value) {
        value.remove(QRegularExpression(QStringLiteral("^[vV]")));
        value = value.section(QLatin1Char('-'), 0, 0).section(QLatin1Char('+'), 0, 0);
        QList<int> out;
        for (const QString &part : value.split(QLatin1Char('.')))
            out.append(part.toInt());
        while (out.size() < 3) out.append(0);
        return out;
    };
    const QList<int> a = parts(candidate);
    const QList<int> b = parts(current);
    for (int i = 0; i < 3; ++i) {
        if (a.at(i) != b.at(i))
            return a.at(i) > b.at(i);
    }
    return false;
}

void UpdateService::setChecking(bool value) {
    if (m_checking == value) return;
    m_checking = value;
    emit changed();
}

void UpdateService::setInstalling(bool value) {
    if (m_installing == value) return;
    m_installing = value;
    emit changed();
}

void UpdateService::setError(const QString &error) {
    if (m_error == error) return;
    m_error = error;
    emit errorChanged();
}

void UpdateService::setProgress(int value) {
    const int bounded = qBound(0, value, 100);
    if (m_progress == bounded) return;
    m_progress = bounded;
    emit progressChanged();
}

void UpdateService::clearError() { setError({}); }

void UpdateService::checkNow() {
    if (m_checking || m_installing) return;
    setError({});
    setChecking(true);
    QNetworkRequest request{QUrl(releaseApi())};
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("UltimateIAH/") + currentVersion());
    request.setRawHeader("Accept", "application/vnd.github+json");
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray data = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto error = reply->error();
        reply->deleteLater();
        if (error != QNetworkReply::NoError || status < 200 || status >= 300) {
            setChecking(false);
            setError(tr("Impossible de vérifier les mises à jour GitHub."));
            return;
        }
        parseRelease(data);
        setChecking(false);
    });
}

void UpdateService::parseRelease(const QByteArray &data) {
    const QJsonObject release = QJsonDocument::fromJson(data).object();
    QString tag = release.value(QStringLiteral("tag_name")).toString();
    tag.remove(QRegularExpression(QStringLiteral("^[vV]")));
    if (tag.isEmpty()) {
        setError(tr("Réponse de release invalide."));
        return;
    }
    m_latestVersion = tag;
    m_releaseUrl = release.value(QStringLiteral("html_url")).toString();
    m_packageUrl.clear();
    m_checksumUrl.clear();
    for (const QJsonValue &value : release.value(QStringLiteral("assets")).toArray()) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        const QString url = asset.value(QStringLiteral("browser_download_url")).toString();
        if (name.endsWith(QStringLiteral(".pkg.tar.zst")))
            m_packageUrl = url;
        else if (name.endsWith(QStringLiteral(".pkg.tar.zst.sha256")))
            m_checksumUrl = url;
    }
    m_updateAvailable = isNewer(tag, currentVersion());
    m_lastChecked = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    if (m_updateAvailable && (m_packageUrl.isEmpty() || m_checksumUrl.isEmpty())) {
        m_updateAvailable = false;
        setError(tr("Release sans paquet ou checksum vérifiable."));
    } else {
        setError({});
    }
    emit changed();
}

void UpdateService::install() {
    if (!m_updateAvailable || m_installing) return;
    if (m_packageUrl.isEmpty() || m_checksumUrl.isEmpty()) {
        setError(tr("Aucun paquet vérifiable à installer."));
        return;
    }
    setError({});
    setInstalling(true);
    setProgress(1);
    downloadChecksum();
}

void UpdateService::downloadChecksum() {
    QNetworkRequest request{QUrl(m_checksumUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("UltimateIAH/") + currentVersion());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray data = reply->readAll();
        const auto error = reply->error();
        reply->deleteLater();
        if (error != QNetworkReply::NoError) {
            setInstalling(false);
            setError(tr("Téléchargement du checksum impossible."));
            return;
        }
        const QString expected = QString::fromUtf8(data).trimmed().section(QLatin1Char(' '), 0, 0).toLower();
        if (expected.size() != 64) {
            setInstalling(false);
            setError(tr("Checksum SHA256 invalide."));
            return;
        }
        m_expectedChecksum = expected;
        setProgress(5);
        downloadPackage();
    });
}

void UpdateService::downloadPackage() {
    QNetworkRequest request{QUrl(m_packageUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("UltimateIAH/") + currentVersion());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0)
            setProgress(5 + int((received * 85) / total));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray data = reply->readAll();
        const auto error = reply->error();
        reply->deleteLater();
        if (error != QNetworkReply::NoError) {
            setInstalling(false);
            setError(tr("Téléchargement du paquet impossible."));
            return;
        }
        QDir().mkpath(cacheDirectory());
        m_packagePath = cacheDirectory() + QStringLiteral("/ultimate-iah-") + m_latestVersion
            + QStringLiteral(".pkg.tar.zst");
        QFile file(m_packagePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setInstalling(false);
            setError(tr("Écriture du paquet impossible."));
            return;
        }
        file.write(data);
        file.close();
        const QByteArray actual = QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
        if (QString::fromLatin1(actual) != m_expectedChecksum) {
            QFile::remove(m_packagePath);
            setInstalling(false);
            setError(tr("Le checksum du paquet ne correspond pas."));
            return;
        }
        setProgress(92);
        verifyAndInstall();
    });
}

void UpdateService::verifyAndInstall() {
    m_installProcess = new QProcess(this);
    m_installProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_installProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        const QString output = QString::fromUtf8(m_installProcess->readAll()).trimmed();
        m_installProcess->deleteLater();
        m_installProcess = nullptr;
        setInstalling(false);
        if (status == QProcess::NormalExit && code == 0) {
            setProgress(100);
            setError({});
            m_installed = true;
            m_updateAvailable = false;
            QFile::remove(m_packagePath);
            emit changed();
            return;
        }
        setError(output.isEmpty() ? tr("L'installation du paquet a été annulée ou a échoué.") : output.left(400));
    });
    emit changed();
    m_installProcess->start(QStringLiteral("pkexec"),
                            {QStringLiteral("pacman"), QStringLiteral("-U"), QStringLiteral("--noconfirm"),
                             m_packagePath});
    if (!m_installProcess->waitForStarted(4000)) {
        m_installProcess->deleteLater();
        m_installProcess = nullptr;
        setInstalling(false);
        setError(tr("Impossible de lancer pkexec."));
    }
}

QString UpdateService::cacheDirectory() const {
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/updates");
}

QJsonObject UpdateService::status() const {
    return QJsonObject{
        {QStringLiteral("ok"), true},
        {QStringLiteral("current"), currentVersion()},
        {QStringLiteral("latest"), m_latestVersion},
        {QStringLiteral("available"), m_updateAvailable},
        {QStringLiteral("releaseUrl"), m_releaseUrl},
        {QStringLiteral("lastChecked"), m_lastChecked},
    };
}
