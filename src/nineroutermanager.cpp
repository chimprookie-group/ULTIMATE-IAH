#include "nineroutermanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

#include <functional>

namespace {
constexpr int kPort = 20128;
constexpr int kHealthIntervalMs = 900;
constexpr int kStartTimeoutMs = 60000;

QString mask(const QString &key) {
    if (key.isEmpty())
        return {};
    if (key.size() <= 4)
        return QStringLiteral("••••");
    return QStringLiteral("••••") + key.right(4);
}

QString tail(const QList<QString> &lines, int count) {
    return lines.mid(qMax(0, lines.size() - count)).join('\n');
}
} // namespace

NineRouterManager::NineRouterManager(QObject *parent)
    : QObject(parent) {
    m_network = new QNetworkAccessManager(this);
    m_configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(m_configDir);

    m_healthTimer = new QTimer(this);
    m_healthTimer->setInterval(kHealthIntervalMs);
    connect(m_healthTimer, &QTimer::timeout, this, &NineRouterManager::probeHealth);

    m_startTimer = new QTimer(this);
    m_startTimer->setSingleShot(true);
    m_startTimer->setInterval(kStartTimeoutMs);
    connect(m_startTimer, &QTimer::timeout, this, [this] {
        if (m_starting) {
            setStarting(false);
            setRunning(false);
            setError(tr("9Router did not become ready within %1 seconds.").arg(kStartTimeoutMs / 1000));
            setDetail(tr("Start timed out"));
        }
    });

    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        const QString text = QString::fromUtf8(m_process.readAllStandardOutput());
        for (const QString &line : text.split('\n', Qt::SkipEmptyParts))
            appendLog(line);
    });
    connect(&m_process, &QProcess::started, this, [this] {
        setStarting(true);
        setDetail(tr("Starting 9Router…"));
        m_healthTimer->start();
        m_startTimer->start();
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && !m_stopping) {
            setStarting(false);
            setRunning(false);
            setError(tr("Could not start 9Router."));
            setDetail(tr("Not running"));
        }
    });
    connect(&m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_healthTimer->stop();
        m_startTimer->stop();
        if (m_stopping)
            return;
        setRunning(false);
        setStarting(false);
        setDetail(status == QProcess::CrashExit
                      ? tr("9Router crashed")
                      : tr("9Router exited with code %1").arg(code));
        if (status == QProcess::CrashExit)
            setError(tr("9Router crashed. Check the log."));
    });

    m_installProcess.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_installProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_installLog += QString::fromUtf8(m_installProcess.readAllStandardOutput());
        if (m_installLog.size() > 12000)
            m_installLog = m_installLog.right(12000);
        emit installLogChanged();
    });
    connect(&m_installProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        m_installing = false;
        emit installingChanged();
        detect();
        if (code == 0 && installed()) {
            verifyInstalled();
        } else {
            setError(tr("The 9Router installation failed."));
            setDetail(tr("Not installed"));
        }
    });

    m_privilegedProcess.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_privilegedProcess, &QProcess::readyReadStandardOutput, this, [this] {
        m_privilegedLog += QString::fromUtf8(m_privilegedProcess.readAllStandardOutput());
        if (m_privilegedLog.size() > 12000)
            m_privilegedLog = m_privilegedLog.right(12000);
        emit privilegedLogChanged();
    });
    connect(&m_privilegedProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            m_privilegedBusy = false;
            emit privilegedBusyChanged();
            setError(tr("pkexec could not be started. Is a polkit authentication agent running?"));
        }
    });
    connect(&m_privilegedProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        m_privilegedBusy = false;
        emit privilegedBusyChanged();
        if (code != 0) {
            setError(tr("Dependency installation failed or was cancelled."));
            return;
        }
        QString verificationError;
        if (!verifyDependencies(&verificationError)) {
            setError(verificationError);
            return;
        }
        m_installReport = tr("System dependencies verified: node, npm, PyYAML");
        emit installReportChanged();
        detect();
        emit dependenciesInstalled();
        install();
    });

    detect();
    if (installed() && (QFileInfo::exists(m_configDir + "/nine-router.patch.yml")
                        || QFileInfo::exists(m_configDir + "/nine-router.enabled")))
        QTimer::singleShot(900, this, &NineRouterManager::start);
}

NineRouterManager::~NineRouterManager() {
    m_stopping = true;
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(3000))
            m_process.kill();
    }
}

void NineRouterManager::detect() {
    QString binary = QStandardPaths::findExecutable(QStringLiteral("9router"));
    if (binary.isEmpty()) {
        const QString local = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + "/9router/node_modules/.bin/9router";
        if (QFileInfo(local).isExecutable())
            binary = local;
    }
    if (binary == m_binary)
        return;
    m_binary = binary;
    emit installedChanged();
    if (m_binary.isEmpty())
        setDetail(tr("Not installed"));
}

void NineRouterManager::cleanupInstallArtifacts() {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/9router";
    QDir(root + "/node_modules/.cache").removeRecursively();
    QDir(root + "/.cache").removeRecursively();
    QDir(root + "/.npm").removeRecursively();
    QFile::remove(root + "/npm-debug.log");
}

void NineRouterManager::verifyInstalled() {
    cleanupInstallArtifacts();
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(m_binary, {QStringLiteral("--version")});
    if (!process.waitForStarted(2500) || !process.waitForFinished(8000)) {
        process.kill();
        process.waitForFinished(100);
        setError(tr("9Router was installed but its version check timed out."));
        setDetail(tr("Installation could not be verified"));
        return;
    }
    const QString version = QString::fromUtf8(process.readAll()).trimmed();
    if (process.exitCode() != 0 || version.isEmpty()) {
        setError(tr("9Router was installed but the version check failed."));
        setDetail(tr("Installation could not be verified"));
        return;
    }
    m_installReport = tr("Verified: 9Router %1 · temporary files cleaned").arg(version);
    emit installReportChanged();
    setDetail(tr("Installed and verified"));
    start();
}

bool NineRouterManager::verifyDependencies(QString *error) {
    const QString node = QStandardPaths::findExecutable(QStringLiteral("node"));
    const QString npm = QStandardPaths::findExecutable(QStringLiteral("npm"));
    const QString python = QStandardPaths::findExecutable(QStringLiteral("python3"));
    if (node.isEmpty() || npm.isEmpty() || python.isEmpty()) {
        if (error)
            *error = tr("node, npm or python3 is still missing after installation.");
        return false;
    }
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(python, {QStringLiteral("-c"), QStringLiteral("import yaml; print(yaml.__version__)")});
    if (!process.waitForStarted(2000) || !process.waitForFinished(5000) || process.exitCode() != 0) {
        process.kill();
        if (error)
            *error = tr("PyYAML is still missing after installation.");
        return false;
    }
    return true;
}

void NineRouterManager::installDependencies() {
    if (m_privilegedBusy)
        return;
    const QString pkexec = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
    if (pkexec.isEmpty()) {
        setError(tr("pkexec was not found. Install polkit first."));
        return;
    }
    const QString pacman = QStandardPaths::findExecutable(QStringLiteral("pacman"));
    if (pacman.isEmpty()) {
        setError(tr("This dependency helper expects the pacman package manager."));
        return;
    }
    m_privilegedBusy = true;
    m_privilegedLog.clear();
    emit privilegedBusyChanged();
    emit privilegedLogChanged();
    setError({});
    setDetail(tr("Waiting for polkit authorization…"));
    m_privilegedProcess.start(pkexec, {pacman, QStringLiteral("-S"), QStringLiteral("--needed"),
                                       QStringLiteral("--noconfirm"),
                                       QStringLiteral("nodejs"), QStringLiteral("npm"),
                                       QStringLiteral("python-yaml")});
}

void NineRouterManager::install() {
    if (m_installing)
        return;
    const QString npm = QStandardPaths::findExecutable(QStringLiteral("npm"));
    if (npm.isEmpty()) {
        setError(tr("npm was not found. Install Node.js first."));
        return;
    }
    if (!m_binary.isEmpty()) {
        setError({});
        setDetail(tr("Already installed"));
        return;
    }
    m_installing = true;
    m_installLog.clear();
    emit installingChanged();
    emit installLogChanged();
    setError({});
    setDetail(tr("Installing 9Router…"));

    const QString prefix = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/9router";
    QDir().mkpath(prefix);
    m_installProcess.setWorkingDirectory(prefix);
    m_installProcess.start(npm, {QStringLiteral("install"), QStringLiteral("--prefix"), prefix,
                                 QStringLiteral("9router"), QStringLiteral("--no-audit"),
                                 QStringLiteral("--no-fund")});
}

void NineRouterManager::start() {
    detect();
    if (!installed()) {
        setError(tr("9Router is not installed."));
        return;
    }
    if (m_running || m_starting)
        return;

    // If another instance is already serving the port, attach to it instead of
    // starting a second process.
    setError({});
    setStarting(true);
    setDetail(tr("Looking for 9Router…"));
    m_probeBeforeStart = true;
    probeHealth();
}

void NineRouterManager::launchProcess() {
    m_stopping = false;
    m_log.clear();
    emit logChanged();
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert("PORT", QString::number(m_port));
    environment.insert("HOSTNAME", QStringLiteral("127.0.0.1"));
    environment.insert("NEXT_PUBLIC_BASE_URL", QStringLiteral("http://127.0.0.1:%1").arg(m_port));
    m_process.setProcessEnvironment(environment);
    m_process.setProgram(m_binary);
    m_process.setWorkingDirectory(QFileInfo(m_binary).absolutePath());
    m_process.start(m_binary, {QStringLiteral("--host"), QStringLiteral("127.0.0.1"),
                               QStringLiteral("--port"), QString::number(m_port),
                               QStringLiteral("--no-browser"), QStringLiteral("--skip-update"),
                               QStringLiteral("--log")});
}

void NineRouterManager::stop() {
    m_stopping = true;
    m_healthTimer->stop();
    m_startTimer->stop();
    if (m_process.state() != QProcess::NotRunning) {
        m_process.terminate();
        if (!m_process.waitForFinished(4000))
            m_process.kill();
    }
    setRunning(false);
    setStarting(false);
    setDetail(tr("Stopped"));
    setAuthRequired(false);
    m_cookie.clear();
}

void NineRouterManager::refresh() {
    detect();
    if (m_running) {
        authenticate();
        return;
    }
    if (installed() && !m_starting)
        start();
}

void NineRouterManager::probeHealth() {
    send("GET", QStringLiteral("/api/health"), nullptr, false, false, [this](QNetworkReply *reply) {
        int status = 0;
        readJson(reply, &status);
        const bool ok = reply->error() == QNetworkReply::NoError && status >= 200 && status < 300;
        if (!ok) {
            if (m_probeBeforeStart) {
                m_probeBeforeStart = false;
                launchProcess();
            }
            return;
        }
        m_probeBeforeStart = false;
        if (!m_running) {
            m_healthTimer->stop();
            m_startTimer->stop();
            setRunning(true);
            setStarting(false);
            setDetail(tr("Running on 127.0.0.1:%1").arg(m_port));
            send("GET", QStringLiteral("/api/version"), nullptr, false, false, [this](QNetworkReply *reply) {
                const QJsonObject root = readJson(reply);
                const QString version = root.value("currentVersion").toString();
                if (!version.isEmpty() && version != m_version) {
                    m_version = version;
                    emit versionChanged();
                }
            });
            authenticate();
        }
    });
}

void NineRouterManager::authenticate() {
    send("GET", QStringLiteral("/api/auth/status"), nullptr, false, false, [this](QNetworkReply *reply) {
        int status = 0;
        const QJsonObject root = readJson(reply, &status);
        if (status < 200 || status >= 300)
            return;
        const bool hasPassword = root.value("hasPassword").toBool();
        if (!hasPassword || root.value("authenticated").toBool()) {
            if (!hasPassword)
                authenticateWith(QStringLiteral("123456"));
            else
                loadKeyAndModels();
        } else {
            authenticateWith(QStringLiteral("123456"));
        }
    });
}

void NineRouterManager::authenticateWith(const QString &password) {
    if (m_cookie.isEmpty() && password.isEmpty()) {
        setAuthRequired(true);
        return;
    }
    QJsonObject body{{"password", password}};
    send("POST", QStringLiteral("/api/auth/login"), &body, false, false, [this](QNetworkReply *reply) {
        int status = 0;
        readJson(reply, &status);
        if (status < 200 || status >= 300) {
            setAuthRequired(true);
            setError(tr("9Router asks for a dashboard password."));
            return;
        }
        const QByteArray cookie = reply->rawHeader("Set-Cookie");
        static const QRegularExpression pattern(QStringLiteral("auth_token=([^;]+)"));
        const auto match = pattern.match(QString::fromUtf8(cookie));
        if (match.hasMatch()) {
            m_cookie = QStringLiteral("auth_token=") + match.captured(1);
            setAuthRequired(false);
            setError({});
            loadKeyAndModels();
        } else {
            setAuthRequired(true);
            setError(tr("9Router login succeeded without a session cookie."));
        }
    });
}

void NineRouterManager::login(const QString &password) {
    m_cookie.clear();
    setError({});
    authenticateWith(password);
}

void NineRouterManager::loadKeyAndModels() {
    send("GET", QStringLiteral("/api/keys"), nullptr, true, false, [this](QNetworkReply *reply) {
        int status = 0;
        const QJsonObject root = readJson(reply, &status);
        if (status == 401 || status == 403) {
            setAuthRequired(true);
            setError(tr("9Router dashboard login required."));
            fetchModels();
            return;
        }
        QJsonArray keys = root.value("keys").toArray();
        QString chosen;
        for (const QJsonValue &value : keys) {
            const QJsonObject key = value.toObject();
            if (key.value("isActive").toBool(false) || !key.contains("isActive")) {
                if (key.value("name").toString() == QStringLiteral("deepcrasy-h")) {
                    chosen = key.value("key").toString();
                    break;
                }
                if (chosen.isEmpty())
                    chosen = key.value("key").toString();
            }
        }
        if (!chosen.isEmpty()) {
            setKey(chosen);
            fetchModels();
            return;
        }
        QJsonObject body{{"name", QStringLiteral("deepcrasy-h")}};
        send("POST", QStringLiteral("/api/keys"), &body, true, false, [this](QNetworkReply *reply) {
            int status = 0;
            const QJsonObject created = readJson(reply, &status);
            if (status >= 200 && status < 300) {
                setKey(created.value("key").toString());
            } else {
                setError(tr("Could not create a local 9Router API key."));
            }
            fetchModels();
        });
    });
}

void NineRouterManager::fetchModels() {
    send("GET", QStringLiteral("/v1/models"), nullptr, false, !m_apiKey.isEmpty(),
         [this](QNetworkReply *reply) {
        int status = 0;
        const QJsonObject root = readJson(reply, &status);
        if (status < 200 || status >= 300)
            return;
        QVariantList models;
        for (const QJsonValue &value : root.value("data").toArray()) {
            const QJsonObject model = value.toObject();
            const QString id = model.value("id").toString();
            if (id.isEmpty())
                continue;
            models.append(QVariantMap{{"id", id},
                                      {"name", model.value("name").toString(id)},
                                      {"ownedBy", model.value("owned_by").toString()}});
        }
        setModels(models);
    });
}


void NineRouterManager::applyToHarness(const QStringList &models, bool makeDefault) {
    if (models.isEmpty()) {
        setError(tr("Select at least one 9Router model."));
        return;
    }
    if (m_apiKey.isEmpty()) {
        setError(tr("9Router is not authenticated yet."));
        return;
    }
    QString error;
    if (!writeHarnessFiles(models, makeDefault, &error)) {
        setError(error);
        return;
    }
    QFile marker(m_configDir + "/nine-router.enabled");
    if (marker.open(QIODevice::WriteOnly))
        marker.write("enabled\n");
    setError({});
    emit harnessApplied();
}

void NineRouterManager::disconnectHarness() {
    QFile::remove(m_configDir + "/nine-router.patch.yml");
    QFile::remove(m_configDir + "/nine-router.env");
    emit harnessDisconnected();
}

QString NineRouterManager::harnessProfile() const {
    QFile file(m_configDir + "/config.json");
    if (!file.open(QIODevice::ReadOnly))
        return QStringLiteral("web");
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QString profile = root.value("harnessProfile").toString().trimmed();
    return profile.isEmpty() ? QStringLiteral("web") : profile;
}

QString NineRouterManager::profilePatchPath() const {
    const QString home = qEnvironmentVariable("DSH_HOME", QDir::homePath() + "/.dsh");
    return home + "/profiles/" + harnessProfile() + "/cordis.patch.yml";
}

QString NineRouterManager::pythonHelper() const {
    return QStringLiteral(R"PY(
import sys, json, os, yaml
class Loader(yaml.SafeLoader):
    pass
Loader.add_constructor("!js", lambda loader, node: loader.construct_scalar(node))
Loader.add_constructor("tag:yaml.org,2002:js", lambda loader, node: loader.construct_scalar(node))
data = json.load(sys.stdin)
providers = {}
profile_patch = data.get("profilePatch")
if profile_patch and os.path.exists(profile_patch):
    with open(profile_patch, encoding="utf-8") as handle:
        entries = yaml.load(handle, Loader=Loader) or []
    for entry in entries:
        if isinstance(entry, dict) and entry.get("id") == "llm-pi-ai":
            providers.update(((entry.get("config") or {}).get("providers") or {}))
models = [{"id": item["id"], "name": item.get("name") or item["id"]} for item in data.get("models", [])]
providers["nine-router"] = {
    "displayName": "9Router",
    "apiKeyEnv": "NINE_ROUTER_API_KEY",
    "api": "openai-completions",
    "baseURL": data["baseURL"],
    "models": models,
}
overlay = [{"id": "llm-pi-ai", "name": "@deepseek-ai/dsh-llm-pi-ai",
            "config": {"providers": providers}}]
if data.get("defaultModel"):
    overlay.append({"id": "agent-default-model",
                    "config": {"provider": "nine-router", "model": data["defaultModel"]}})
os.makedirs(os.path.dirname(data["out"]), exist_ok=True)
with open(data["out"], "w", encoding="utf-8") as handle:
    yaml.safe_dump(overlay, handle, sort_keys=False, allow_unicode=True)
print("ok")
)PY");
}

bool NineRouterManager::writeHarnessFiles(const QStringList &models, bool makeDefault, QString *error) {
    QJsonArray modelArray;
    for (const QString &id : models)
        modelArray.append(QJsonObject{{"id", id}, {"name", id}});
    const QString patch = m_configDir + "/nine-router.patch.yml";
    QJsonObject payload{{"profilePatch", profilePatchPath()},
                        {"out", patch},
                        {"baseURL", QStringLiteral("http://127.0.0.1:%1/v1").arg(m_port)},
                        {"models", modelArray},
                        {"defaultModel", makeDefault ? models.first() : QString()}};

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(QStandardPaths::findExecutable(QStringLiteral("python3")),
                  {QStringLiteral("-c"), pythonHelper()});
    if (!process.waitForStarted(2000)) {
        if (error)
            *error = tr("Python 3 is required to write the harness patch.");
        return false;
    }
    process.write(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    process.closeWriteChannel();
    if (!process.waitForFinished(12000)) {
        process.kill();
        if (error)
            *error = tr("Writing the harness provider timed out.");
        return false;
    }
    if (process.exitCode() != 0) {
        if (error)
            *error = tr("The harness patch could not be generated: %1")
                         .arg(QString::fromUtf8(process.readAll()).trimmed().left(300));
        return false;
    }

    QSaveFile envFile(m_configDir + "/nine-router.env");
    if (!envFile.open(QIODevice::WriteOnly)) {
        if (error)
            *error = tr("The 9Router key file could not be written.");
        return false;
    }
    envFile.write("NINE_ROUTER_API_KEY=" + m_apiKey.toUtf8() + "\n");
    if (!envFile.commit()) {
        if (error)
            *error = tr("The 9Router key file could not be saved.");
        return false;
    }
    QFile::setPermissions(m_configDir + "/nine-router.env", QFile::ReadOwner | QFile::WriteOwner);
    return true;
}

void NineRouterManager::send(const QByteArray &method, const QString &path,
                             const QJsonObject *body, bool cookie, bool bearer,
                             const std::function<void(QNetworkReply *)> &callback) {
    QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(m_port).arg(path)));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("DeepCrasyH/1.0"));
    request.setRawHeader("Accept", "application/json");
    if (cookie && !m_cookie.isEmpty())
        request.setRawHeader("Cookie", m_cookie.toUtf8());
    if (bearer && !m_apiKey.isEmpty())
        request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
    QByteArray payload;
    if (body) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        payload = QJsonDocument(*body).toJson(QJsonDocument::Compact);
    }

    QNetworkReply *reply = m_network->sendCustomRequest(request, method, payload);
    QTimer::singleShot(12000, reply, [reply] { reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [callback, reply] {
        callback(reply);
        reply->deleteLater();
    });
}

QJsonObject NineRouterManager::readJson(QNetworkReply *reply, int *status) const {
    if (status)
        *status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    return document.object();
}

void NineRouterManager::setError(const QString &error) {
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

void NineRouterManager::setDetail(const QString &detail) {
    if (m_detail == detail)
        return;
    m_detail = detail;
    emit detailChanged();
}

void NineRouterManager::setRunning(bool running) {
    if (m_running == running)
        return;
    m_running = running;
    emit runningChanged();
}

void NineRouterManager::setStarting(bool starting) {
    if (m_starting == starting)
        return;
    m_starting = starting;
    emit startingChanged();
}

void NineRouterManager::setAuthRequired(bool required) {
    if (m_authRequired == required)
        return;
    m_authRequired = required;
    emit authRequiredChanged();
}

void NineRouterManager::setKey(const QString &key) {
    m_apiKey = key;
    const QString maskedKey = mask(key);
    if (m_keyMasked != maskedKey) {
        m_keyMasked = maskedKey;
        emit keyChanged();
    }
}

void NineRouterManager::setModels(const QVariantList &models) {
    m_models = models;
    emit modelsChanged();
}

void NineRouterManager::appendLog(const QString &line) {
    m_log.append(line.right(400));
    while (m_log.size() > 200)
        m_log.removeFirst();
    emit logChanged();
}

void NineRouterManager::clearError() { setError({}); }
