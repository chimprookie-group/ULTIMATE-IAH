#include "harnessconnections.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>
#include <QStringList>
#include <QRegularExpression>
#include <QUrl>

namespace {
QString credentialsPath() {
    const QString home = qEnvironmentVariable("DSH_HOME", QDir::homePath() + "/.dsh");
    return home + "/.credentials.yaml";
}

QString profilePatchPath(const QString &profile) {
    const QString home = qEnvironmentVariable("DSH_HOME", QDir::homePath() + "/.dsh");
    return home + "/profiles/" + profile + "/cordis.patch.yml";
}
} // namespace

HarnessConnections::HarnessConnections(const QString &overlayPath, QObject *parent)
    : QObject(parent),
      m_overlayPath(overlayPath.isEmpty() ? defaultOverlayPath() : overlayPath),
      m_envPath(QFileInfo(m_overlayPath).absolutePath() + "/agent.env") {
    refresh();
}

QString HarnessConnections::defaultOverlayPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + "/connections.patch.yml";
}

QString HarnessConnections::harnessProfile() const {
    QFile file(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/config.json");
    if (!file.open(QIODevice::ReadOnly))
        return QStringLiteral("web");
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QString profile = root.value("harnessProfile").toString().trimmed();
    return profile.isEmpty() ? QStringLiteral("web") : profile;
}

QString HarnessConnections::pythonHelper() const {
    return QStringLiteral(R"PY(
import sys, json, os, yaml

class Loader(yaml.SafeLoader):
    pass
Loader.add_constructor("!js", lambda loader, node: loader.construct_scalar(node))
Loader.add_constructor("tag:yaml.org,2002:js", lambda loader, node: loader.construct_scalar(node))

data = json.load(sys.stdin)
profile_patch = data["profilePatch"]
overlay_path = data["overlay"]
creds_path = data["credentials"]

providers = {}
for path in (profile_patch, overlay_path):
    if path and os.path.exists(path):
        with open(path, encoding="utf-8") as handle:
            try:
                entries = yaml.load(handle, Loader=Loader) or []
            except Exception:
                entries = []
        for entry in entries:
            if isinstance(entry, dict) and entry.get("id") == "llm-pi-ai":
                providers.update(((entry.get("config") or {}).get("providers") or {}))

creds = {}
if os.path.exists(creds_path):
    with open(creds_path, encoding="utf-8") as handle:
        try:
            creds = yaml.safe_load(handle) or {}
        except Exception:
            creds = {}
if not isinstance(creds, dict):
    creds = {}
creds.setdefault("version", 1)
refs = creds.get("refs") if isinstance(creds.get("refs"), dict) else {}
creds["refs"] = refs
refs = creds.get("records") if isinstance(creds.get("records"), dict) else {}
creds["records"] = refs
refs = creds["refs"]

default_provider = ""
default_model = ""
if os.path.exists(overlay_path):
    try:
        with open(overlay_path, encoding="utf-8") as handle:
            existing_overlay = yaml.load(handle, Loader=Loader) or []
        for entry in existing_overlay:
            if isinstance(entry, dict) and entry.get("id") == "agent-default-model":
                cfg = entry.get("config") or {}
                default_provider = cfg.get("provider", "")
                default_model = cfg.get("model", "")
    except Exception:
        pass

def provider_summary(provider_id, entry):
    models = entry.get("models") or []
    ids = []
    for model in models:
        if isinstance(model, dict):
            ids.append(model.get("id", ""))
        elif isinstance(model, str):
            ids.append(model)
    env = entry.get("apiKeyEnv", "")
    key = refs.get(env, "") if env else ""
    return {
        "id": provider_id,
        "name": entry.get("displayName") or provider_id,
        "baseURL": entry.get("baseURL", ""),
        "api": entry.get("api", "openai-completions"),
        "models": ids,
        "hasKey": bool(key),
        "apiKeyEnv": env,
        "keyMasked": ("••••" + key[-4:]) if key else "",
        "builtin": provider_id == "deepseek-official",
    }

op = data.get("op", "list")
if op == "saveDeepSeek":
    key = data.get("apiKey", "").strip()
    if key:
        refs["DEEPSEEK_API_KEY"] = key
    providers["deepseek-official"] = {
        "displayName": "DeepSeek",
        "api": "openai-completions",
        "baseURL": "https://api.deepseek.com",
        "apiKeyEnv": "DEEPSEEK_API_KEY",
        "models": [
            {"id": "deepseek-chat", "name": "DeepSeek Chat"},
            {"id": "deepseek-reasoner", "name": "DeepSeek Reasoner"},
        ],
    }
    if data.get("makeDefault") and not default_model:
        default_provider, default_model = "deepseek-official", "deepseek-chat"
elif op == "saveProvider":
    provider_id = data["id"]
    ref = data["apiKeyEnv"]
    key = data.get("apiKey", "").strip()
    if key:
        refs[ref] = key
    models = []
    for item in data.get("models", []):
        models.append({"id": item["id"], "name": item.get("name") or item["id"]})
    providers[provider_id] = {
        "displayName": data["name"],
        "api": data.get("api") or "openai-completions",
        "baseURL": data["baseURL"],
        "models": models,
    }
    if key or ref in refs:
        providers[provider_id]["apiKeyEnv"] = ref
    if data.get("makeDefault") and models:
        default_provider, default_model = provider_id, models[0]["id"]
elif op == "removeProvider":
    providers.pop(data["id"], None)
elif op == "setDefault":
    default_provider, default_model = data["provider"], data["model"]

deepseek_key = refs.get("DEEPSEEK_API_KEY", "")
if deepseek_key and "deepseek-official" not in providers:
    providers["deepseek-official"] = {
        "displayName": "DeepSeek",
        "api": "openai-completions",
        "baseURL": "https://api.deepseek.com",
        "apiKeyEnv": "DEEPSEEK_API_KEY",
        "models": [
            {"id": "deepseek-chat", "name": "DeepSeek Chat"},
            {"id": "deepseek-reasoner", "name": "DeepSeek Reasoner"},
        ],
    }
if deepseek_key and not default_provider:
    default_provider, default_model = "deepseek-official", "deepseek-chat"
if default_provider == "deepseek-official" and default_model in ("deepseek-flash", "deepseek-v4-pro"):
    default_model = "deepseek-chat"

overlay = [{
    "id": "llm-pi-ai",
    "name": "@deepseek-ai/dsh-llm-pi-ai",
    "config": {"providers": providers},
}]
if default_provider and default_model:
    overlay.append({
        "id": "agent-default-model",
        "config": {"provider": default_provider, "model": default_model},
    })

os.makedirs(os.path.dirname(overlay_path), exist_ok=True)
with open(overlay_path, "w", encoding="utf-8") as handle:
    yaml.safe_dump(overlay, handle, sort_keys=False, allow_unicode=True)
os.makedirs(os.path.dirname(creds_path), exist_ok=True)
with open(creds_path, "w", encoding="utf-8") as handle:
    yaml.safe_dump(creds, handle, sort_keys=False, allow_unicode=True)
os.chmod(creds_path, 0o600)
env_path = data.get("env", "")
if env_path:
    os.makedirs(os.path.dirname(env_path), exist_ok=True)
    with open(env_path, "w", encoding="utf-8") as handle:
        for key, value in refs.items():
            if isinstance(key, str) and isinstance(value, str) and "\\n" not in value:
                handle.write(key + "=" + value + "\n")
    os.chmod(env_path, 0o600)

summary = [provider_summary(pid, entry) for pid, entry in sorted(providers.items())]
deepseek = refs.get("DEEPSEEK_API_KEY", "")
if deepseek and not any(item.get("id") == "deepseek-official" for item in summary):
    summary.insert(0, {
        "id": "deepseek-official",
        "name": "DeepSeek",
        "baseURL": "https://api.deepseek.com",
        "models": ["deepseek-chat", "deepseek-reasoner"],
        "hasKey": True,
        "keyMasked": "••••" + deepseek[-4:],
        "builtin": True,
    })
print(json.dumps({
    "providers": summary,
    "defaultProvider": default_provider,
    "defaultModel": default_model,
    "deepseekConfigured": bool(deepseek),
}, ensure_ascii=False))
)PY");
}

bool HarnessConnections::run(const QJsonObject &request, QJsonObject &result) {
    const QString python = QStandardPaths::findExecutable(QStringLiteral("python3"));
    if (python.isEmpty()) {
        setError(tr("Python 3 is required to manage harness connections."));
        return false;
    }
    QJsonObject payload = request;
    payload["profilePatch"] = profilePatchPath(harnessProfile());
    payload["overlay"] = m_overlayPath;
    payload["credentials"] = credentialsPath();
    payload["env"] = m_envPath;

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(python, {QStringLiteral("-c"), pythonHelper()});
    if (!process.waitForStarted(2000)) {
        setError(tr("Could not start the connection helper."));
        return false;
    }
    process.write(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    process.closeWriteChannel();
    if (!process.waitForFinished(15000)) {
        process.kill();
        setError(tr("The connection helper timed out."));
        return false;
    }
    if (process.exitCode() != 0) {
        setError(QString::fromUtf8(process.readAll()).trimmed().left(300));
        return false;
    }
    const QJsonDocument document = QJsonDocument::fromJson(process.readAll());
    if (!document.isObject()) {
        setError(tr("The connection helper returned an invalid result."));
        return false;
    }
    result = document.object();
    setError({});
    return true;
}

void HarnessConnections::refresh() {
    QJsonObject result;
    if (!run(QJsonObject{{"op", "list"}}, result))
        return;
    m_providers = result.value("providers").toArray().toVariantList();
    m_defaultProvider = result.value("defaultProvider").toString();
    m_defaultModelId = result.value("defaultModel").toString();
    m_defaultModel = m_defaultProvider.isEmpty() || m_defaultModelId.isEmpty()
        ? QString() : m_defaultProvider + "/" + m_defaultModelId;
    m_deepseekConfigured = result.value("deepseekConfigured").toBool();
    emit changed();
}

void HarnessConnections::saveDeepSeek(const QString &apiKey) {
    QJsonObject result;
    if (!run(QJsonObject{{"op", "saveDeepSeek"}, {"apiKey", apiKey}, {"makeDefault", true}}, result))
        return;
    refresh();
    emit deepSeekChanged();
}

void HarnessConnections::saveProvider(const QString &idValue, const QString &nameValue,
                                      const QString &baseUrl, const QString &apiKey,
                                      const QString &modelsText, bool makeDefault,
                                      const QString &apiValue) {
    const QString id = idValue.trimmed().toLower();
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
    QJsonArray models;
    for (const QString &line : modelsText.split('\n', Qt::SkipEmptyParts)) {
        QString model = line.trimmed();
        if (model.isEmpty())
            continue;
        QString display = model;
        const int equals = model.indexOf('=');
        if (equals > 0) {
            display = model.mid(equals + 1).trimmed();
            model = model.left(equals).trimmed();
        }
        if (!model.isEmpty())
            models.append(QJsonObject{{"id", model}, {"name", display.isEmpty() ? model : display}});
    }
    if (models.isEmpty()) {
        setError(tr("Add at least one model ID."));
        return;
    }
    const QString api = apiValue == "anthropic-messages"
        ? QStringLiteral("anthropic-messages") : QStringLiteral("openai-completions");
    QJsonObject request{{"op", "saveProvider"}, {"id", id},
                        {"name", nameValue.trimmed().isEmpty() ? id : nameValue.trimmed()},
                        {"baseURL", baseUrl.trimmed()}, {"apiKey", apiKey},
                        {"apiKeyEnv", "HARNESS_" + id.toUpper().replace('-', '_') + "_API_KEY"},
                        {"api", api},
                        {"models", models}, {"makeDefault", makeDefault}};
    QJsonObject result;
    if (!run(request, result))
        return;
    refresh();
}

void HarnessConnections::removeProvider(const QString &id) {
    QJsonObject result;
    if (!run(QJsonObject{{"op", "removeProvider"}, {"id", id}}, result))
        return;
    refresh();
}

void HarnessConnections::setDefault(const QString &provider, const QString &model) {
    QJsonObject result;
    if (!run(QJsonObject{{"op", "setDefault"}, {"provider", provider}, {"model", model}}, result))
        return;
    refresh();
}

void HarnessConnections::clearError() { setError({}); }

QString HarnessConnections::providerApiKey(const QString &id) const {
    QString envName;
    for (const QVariant &value : m_providers) {
        const QVariantMap provider = value.toMap();
        if (provider.value(QStringLiteral("id")).toString() == id) {
            envName = provider.value(QStringLiteral("apiKeyEnv")).toString();
            break;
        }
    }
    if (envName.isEmpty()) return {};
    const QString fromEnvironment = qEnvironmentVariable(envName.toUtf8().constData());
    if (!fromEnvironment.isEmpty()) return fromEnvironment;
    QFile envFile(m_envPath);
    if (envFile.open(QIODevice::ReadOnly)) {
        for (const QString &line : QString::fromUtf8(envFile.readAll()).split('\n')) {
            const int equals = line.indexOf('=');
            if (equals > 0 && line.left(equals) == envName)
                return line.mid(equals + 1).trimmed();
        }
    }
    QFile creds(credentialsPath());
    if (!creds.open(QIODevice::ReadOnly)) return {};
    const QString text = QString::fromUtf8(creds.readAll());
    QRegularExpression expression(QStringLiteral("(?m)^\\s*%1\\s*[:=]\\s*(\\S+)").arg(QRegularExpression::escape(envName)));
    const auto match = expression.match(text);
    if (!match.hasMatch()) return {};
    QString key = match.captured(1).trimmed();
    key.remove(QRegularExpression(QStringLiteral("^[\\\"']|[\\\"']$")));
    return key;
}

void HarnessConnections::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}
