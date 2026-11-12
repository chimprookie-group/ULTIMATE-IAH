#include "providermanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

namespace {

constexpr auto kPackage = "@opencode/ai/providers/openai-compatible";

QString masked(const QString &key) {
    if (key.isEmpty())
        return {};
    if (key.size() <= 4)
        return QStringLiteral("••••");
    return QStringLiteral("••••") + key.right(4);
}

QString modelUrl(QString baseUrl) {
    while (baseUrl.endsWith('/'))
        baseUrl.chop(1);
    if (baseUrl.endsWith("/models"))
        return baseUrl;
    return baseUrl + "/models";
}

} // namespace

ProviderManager::ProviderManager(const QString &configPath, QObject *parent)
    : QObject(parent),
      m_configPath(configPath.isEmpty() ? defaultConfigPath() : configPath) {
    m_network = new QNetworkAccessManager(this);
    m_timeout = new QTimer(this);
    m_timeout->setSingleShot(true);
    m_timeout->setInterval(10000);
    connect(m_timeout, &QTimer::timeout, this, [this] {
        if (m_reply) {
            m_reply->abort();
        }
    });
}

QString ProviderManager::defaultConfigPath() {
    const QString explicitPath = qEnvironmentVariable("OPENCODE_CONFIG");
    if (!explicitPath.isEmpty())
        return explicitPath;
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + "/opencode/opencode.json";
}

bool ProviderManager::readConfig(QJsonObject &root) const {
    QFile file(m_configPath);
    if (!file.exists()) {
        root = QJsonObject{};
        return true;
    }
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return false;
    root = document.object();
    return true;
}

bool ProviderManager::writeConfig(const QJsonObject &root) {
    if (!QDir().mkpath(QFileInfo(m_configPath).absolutePath()))
        return false;
    QSaveFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit())
        return false;
    QFile::setPermissions(m_configPath, QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

void ProviderManager::refresh() {
    QJsonObject root;
    if (!readConfig(root)) {
        setError(tr("The OpenCode configuration could not be read."));
        m_providers.clear();
        emit changed();
        return;
    }

    QVariantList providers;
    const QJsonObject entries = root.value("providers").toObject();
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        const QJsonObject settings = entry.value("settings").toObject();
        const QString baseUrl = settings.value("baseURL").toString();
        const QString key = settings.value("apiKey").toString();
        const QJsonObject models = entry.value("models").toObject();
        QVariantList modelList;
        for (auto model = models.begin(); model != models.end(); ++model) {
            const QString name = model.value().toObject().value("name").toString();
            modelList.append(QVariantMap{{"id", model.key()},
                                         {"name", name.isEmpty() ? model.key() : name}});
        }
        providers.append(QVariantMap{
            {"id", it.key()},
            {"name", entry.value("name").toString(it.key())},
            {"baseURL", baseUrl},
            {"keyMasked", masked(key)},
            {"hasKey", !key.isEmpty()},
            {"package", entry.value("package").toString()},
            {"models", modelList},
            {"modelCount", modelList.size()},
            {"custom", entry.value("package").toString().contains("openai-compatible")
                           || entry.value("package").toString().isEmpty()},
        });
    }
    m_providers = providers;
    setError({});
    emit changed();
}

void ProviderManager::refreshAccounts() {
    QVariantList accounts;
    const QString opencode = QStandardPaths::findExecutable(QStringLiteral("opencode"));
    if (!opencode.isEmpty()) {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(opencode, {QStringLiteral("auth"), QStringLiteral("list"),
                                 QStringLiteral("--standalone")});
        if (process.waitForStarted(1200) && process.waitForFinished(4000)) {
            const QString output = QString::fromUtf8(process.readAll()).trimmed();
            if (!output.contains("No authenticated integrations")) {
                for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
                    const QStringList columns = line.trimmed().split(
                        QRegularExpression(QStringLiteral("\\s{2,}")), Qt::SkipEmptyParts);
                    if (columns.size() < 3)
                        continue;
                    accounts.append(QVariantMap{{"id", columns.at(0).trimmed().toLower().replace(' ', '-')},
                                                {"name", columns.at(1).trimmed()},
                                                {"status", columns.at(2).trimmed()}});
                }
            }
        } else if (process.state() != QProcess::NotRunning) {
            process.kill();
            process.waitForFinished(50);
        }
    }
    m_accounts = accounts;
    emit changed();
}


void ProviderManager::saveProvider(const QString &idValue, const QString &nameValue,
                                   const QString &baseUrlValue, const QString &apiKey,
                                   const QString &modelsText) {
    const QString id = idValue.trimmed().toLower();
    const QString name = nameValue.trimmed().isEmpty() ? id : nameValue.trimmed();
    const QString baseUrl = baseUrlValue.trimmed();

    static const QRegularExpression idPattern(QStringLiteral("^[a-z0-9][a-z0-9_-]{0,40}$"));
    if (!idPattern.match(id).hasMatch()) {
        setError(tr("Provider ID must use lowercase letters, digits, - or _."));
        return;
    }
    const QUrl url(baseUrl);
    if (!url.isValid() || (url.scheme() != "http" && url.scheme() != "https")) {
        setError(tr("The base URL must start with http:// or https://."));
        return;
    }

    QJsonObject models;
    for (const QString &line : modelsText.split('\n', Qt::SkipEmptyParts)) {
        QString modelId = line.trimmed();
        if (modelId.isEmpty())
            continue;
        QString modelName = modelId;
        const int equals = modelId.indexOf('=');
        if (equals > 0) {
            modelName = modelId.mid(equals + 1).trimmed();
            modelId = modelId.left(equals).trimmed();
        }
        if (modelId.isEmpty())
            continue;
        models.insert(modelId, QJsonObject{{"name", modelName.isEmpty() ? modelId : modelName}});
    }
    if (models.isEmpty()) {
        setError(tr("Add at least one model ID."));
        return;
    }

    QJsonObject root;
    if (!readConfig(root)) {
        setError(tr("The OpenCode configuration could not be read."));
        return;
    }
    QJsonObject providers = root.value("providers").toObject();
    QJsonObject entry = providers.value(id).toObject();
    const QString package = entry.value("package").toString();
    if (!package.isEmpty() && !package.contains("openai-compatible")) {
        setError(tr("This built-in provider must be connected with OpenCode auth."));
        return;
    }

    entry["name"] = name;
    entry["package"] = QString::fromLatin1(kPackage);
    QJsonObject settings = entry.value("settings").toObject();
    settings["baseURL"] = baseUrl;
    if (!apiKey.trimmed().isEmpty())
        settings["apiKey"] = apiKey.trimmed();
    entry["settings"] = settings;
    entry["models"] = models;
    providers[id] = entry;
    root["providers"] = providers;

    if (!writeConfig(root)) {
        setError(tr("The OpenCode configuration could not be written."));
        return;
    }
    setError({});
    setTestMessage({});
    refresh();
}

void ProviderManager::removeProvider(const QString &id) {
    QJsonObject root;
    if (!readConfig(root)) {
        setError(tr("The OpenCode configuration could not be read."));
        return;
    }
    QJsonObject providers = root.value("providers").toObject();
    if (!providers.contains(id))
        return;
    providers.remove(id);
    root["providers"] = providers;
    if (!writeConfig(root)) {
        setError(tr("The OpenCode configuration could not be written."));
        return;
    }
    setError({});
    refresh();
}

QVariantMap ProviderManager::findProvider(const QString &id) const {
    for (const QVariant &value : m_providers) {
        const QVariantMap provider = value.toMap();
        if (provider.value("id").toString() == id)
            return provider;
    }
    return {};
}

void ProviderManager::testProvider(const QString &id) {
    QJsonObject root;
    if (!readConfig(root)) {
        setError(tr("The OpenCode configuration could not be read."));
        return;
    }
    const QJsonObject entry = root.value("providers").toObject().value(id).toObject();
    if (entry.isEmpty()) {
        setError(tr("Provider not found."));
        return;
    }
    const QJsonObject settings = entry.value("settings").toObject();
    const QString baseUrl = settings.value("baseURL").toString();
    const QString key = settings.value("apiKey").toString();
    const QUrl url(modelUrl(baseUrl));
    if (!url.isValid()) {
        setError(tr("The provider URL is invalid."));
        return;
    }

    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    setError({});
    setTestMessage(tr("Testing %1…").arg(id));
    setTesting(true);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("DeepCrasyH/1.0"));
    request.setRawHeader("Accept", "application/json");
    if (!key.isEmpty())
        request.setRawHeader("Authorization", "Bearer " + key.toUtf8());

    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this] { finishTest(m_reply); });
    m_timeout->start();
}

void ProviderManager::finishTest(QNetworkReply *reply) {
    m_timeout->stop();
    if (!reply)
        return;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray data = reply->readAll();
    if (reply == m_reply)
        m_reply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError && status == 0) {
        setTestMessage(tr("Connection failed: %1").arg(reply->errorString()));
        setTesting(false);
        return;
    }

    int modelCount = -1;
    const QJsonDocument document = QJsonDocument::fromJson(data);
    if (document.isObject()) {
        const QJsonArray list = document.object().value("data").toArray();
        if (document.object().contains("data"))
            modelCount = list.size();
    }
    if (status >= 200 && status < 300) {
        setTestMessage(modelCount >= 0
                           ? tr("Connected · %n model(s) visible", "", modelCount)
                           : tr("Connected (HTTP %1)").arg(status));
    } else {
        setTestMessage(tr("HTTP %1: the provider rejected the request.").arg(status));
    }
    setTesting(false);
}

void ProviderManager::restartService() {
    const QString opencode = QStandardPaths::findExecutable(QStringLiteral("opencode"));
    if (opencode.isEmpty()) {
        setError(tr("OpenCode was not found."));
        return;
    }
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(opencode, {QStringLiteral("service"), QStringLiteral("restart")});
    if (!process.waitForStarted(1200) || !process.waitForFinished(5000)) {
        process.kill();
        process.waitForFinished(100);
        setError(tr("The OpenCode service could not be restarted."));
        return;
    }
    emit applied();
}

void ProviderManager::clearError() { setError({}); }
void ProviderManager::clearTest() { setTestMessage({}); }

void ProviderManager::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}

void ProviderManager::setTestMessage(const QString &message) {
    if (message == m_testMessage)
        return;
    m_testMessage = message;
    emit testMessageChanged();
}

void ProviderManager::setTesting(bool testing) {
    if (testing == m_testing)
        return;
    m_testing = testing;
    emit testingChanged();
}
