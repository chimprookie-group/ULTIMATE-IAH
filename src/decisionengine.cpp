#include "decisionengine.h"

#include "crasyrouter.h"
#include "harnessconnections.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
bool looksFree(const QString &value) {
    const QString lower = value.toLower();
    return lower.contains(QLatin1String("free")) || lower.contains(QLatin1String("ollama"));
}
} // namespace

DecisionEngine::DecisionEngine(QObject *parent)
    : QObject(parent) {
    load();
}

QString DecisionEngine::settingsPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/decisions.json");
}

void DecisionEngine::load() {
    QFile file(settingsPath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    m_strategy = root.value(QStringLiteral("strategy")).toString(m_strategy);
    m_autonomy = root.value(QStringLiteral("autonomy")).toString(m_autonomy);
    m_providerSource = root.value(QStringLiteral("providerSource")).toString(m_providerSource);
    m_permissionCeiling = root.value(QStringLiteral("permissionCeiling")).toString(m_permissionCeiling);
    m_maxRounds = qBound(1, root.value(QStringLiteral("maxRounds")).toInt(m_maxRounds), 500);
    m_maxRetries = qBound(0, root.value(QStringLiteral("maxRetries")).toInt(m_maxRetries), 10);
    m_allowFallback = root.value(QStringLiteral("allowFallback")).toBool(m_allowFallback);
    m_reviewChanges = root.value(QStringLiteral("reviewChanges")).toBool(m_reviewChanges);
    m_useMemory = root.value(QStringLiteral("useMemory")).toBool(m_useMemory);
    m_useLearning = root.value(QStringLiteral("useLearning")).toBool(m_useLearning);
}

void DecisionEngine::save() const {
    QDir().mkpath(QFileInfo(settingsPath()).absolutePath());
    QSaveFile file(settingsPath());
    if (!file.open(QIODevice::WriteOnly))
        return;
    const QJsonObject root{
        {QStringLiteral("strategy"), m_strategy},
        {QStringLiteral("autonomy"), m_autonomy},
        {QStringLiteral("providerSource"), m_providerSource},
        {QStringLiteral("permissionCeiling"), m_permissionCeiling},
        {QStringLiteral("maxRounds"), m_maxRounds},
        {QStringLiteral("maxRetries"), m_maxRetries},
        {QStringLiteral("allowFallback"), m_allowFallback},
        {QStringLiteral("reviewChanges"), m_reviewChanges},
        {QStringLiteral("useMemory"), m_useMemory},
        {QStringLiteral("useLearning"), m_useLearning},
    };
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.commit();
}

void DecisionEngine::touch() {
    save();
    refresh();
    emit optionsChanged();
}

QString DecisionEngine::pickModel(const ProviderInfo &provider, const Context &context) {
    if (!context.modelOverride.isEmpty() && provider.models.contains(context.modelOverride))
        return context.modelOverride;
    if (!context.defaultModel.isEmpty() && provider.models.contains(context.defaultModel))
        return context.defaultModel;
    QStringList preferences;
    if (context.strategy == QLatin1String("quality"))
        preferences = {QStringLiteral("pro"), QStringLiteral("reasoner"), QStringLiteral("sonnet"),
                       QStringLiteral("gpt-4"), QStringLiteral("max"), QStringLiteral("4.1")};
    else if (context.strategy == QLatin1String("free"))
        preferences = {QStringLiteral("flash"), QStringLiteral("mini"), QStringLiteral("free"),
                       QStringLiteral("chat"), QStringLiteral("auto")};
    else
        preferences = {QStringLiteral("chat"), QStringLiteral("flash"), QStringLiteral("auto"),
                       QStringLiteral("mini")};
    for (const QString &preference : preferences) {
        for (const QString &model : provider.models) {
            if (model.toLower().contains(preference))
                return model;
        }
    }
    return provider.models.value(0);
}

void DecisionEngine::normalize(Endpoint &endpoint) {
    while (endpoint.baseUrl.endsWith(QLatin1Char('/')))
        endpoint.baseUrl.chop(1);
    if (endpoint.baseUrl == QLatin1String("https://api.deepseek.com"))
        endpoint.baseUrl += QStringLiteral("/v1");
}

DecisionEngine::Context DecisionEngine::context() const {
    Context result;
    result.source = m_providerSource;
    result.strategy = m_strategy;
    result.providerOverride = m_providerOverride;
    result.modelOverride = m_modelOverride;
    result.deepSeekKey = credential(QStringLiteral("DEEPSEEK_API_KEY"));
    if (m_connections) {
        result.defaultProvider = m_connections->defaultProvider();
        result.defaultModel = m_connections->defaultModelId();
        for (const QVariant &value : m_connections->providers()) {
            const QVariantMap map = value.toMap();
            ProviderInfo provider;
            provider.id = map.value(QStringLiteral("id")).toString();
            provider.name = map.value(QStringLiteral("name")).toString();
            if (provider.name.isEmpty())
                provider.name = provider.id;
            provider.baseUrl = map.value(QStringLiteral("baseURL")).toString();
            provider.models = map.value(QStringLiteral("models")).toStringList();
            provider.key = m_connections->providerApiKey(provider.id);
            provider.free = looksFree(provider.name) || looksFree(provider.id);
            result.providers.append(provider);
        }
    }
    if (m_router) {
        result.routerEndpoint = m_router->endpoint();
        result.routerToken = m_router->token();
    }
    return result;
}

DecisionEngine::Endpoint DecisionEngine::resolve(const Context &context) {
    Endpoint endpoint;
    const auto fromProvider = [&context](const ProviderInfo &provider) {
        Endpoint candidate;
        if (provider.baseUrl.isEmpty() || provider.key.isEmpty() || provider.models.isEmpty())
            return candidate;
        candidate.providerId = provider.id;
        candidate.baseUrl = provider.baseUrl;
        candidate.apiKey = provider.key;
        candidate.model = pickModel(provider, context);
        candidate.label = provider.name.isEmpty() ? provider.id : provider.name;
        candidate.source = QStringLiteral("harness");
        normalize(candidate);
        return candidate;
    };
    const auto fromDeepSeek = [&context]() {
        Endpoint candidate;
        if (context.deepSeekKey.isEmpty())
            return candidate;
        candidate.providerId = QStringLiteral("deepseek");
        candidate.baseUrl = QStringLiteral("https://api.deepseek.com/v1");
        candidate.apiKey = context.deepSeekKey;
        candidate.model = context.strategy == QLatin1String("quality")
            ? QStringLiteral("deepseek-reasoner") : QStringLiteral("deepseek-chat");
        candidate.label = QStringLiteral("DeepSeek API");
        candidate.source = QStringLiteral("deepseek");
        return candidate;
    };
    const auto fromRouter = [&context]() {
        Endpoint candidate;
        if (context.routerEndpoint.isEmpty() || context.routerToken.isEmpty())
            return candidate;
        candidate.providerId = QStringLiteral("4crasy");
        candidate.baseUrl = context.routerEndpoint;
        candidate.apiKey = context.routerToken;
        candidate.model = context.routerModel.isEmpty() ? QStringLiteral("auto") : context.routerModel;
        candidate.label = QStringLiteral("4Crasy");
        candidate.source = QStringLiteral("4crasy");
        return candidate;
    };
    const auto defaultProvider = [&]() {
        for (const ProviderInfo &provider : context.providers) {
            if (!context.defaultProvider.isEmpty() && provider.id == context.defaultProvider) {
                Endpoint candidate = fromProvider(provider);
                if (candidate.valid())
                    return candidate;
            }
        }
        return Endpoint{};
    };
    const auto firstKeyed = [&]() {
        for (const ProviderInfo &provider : context.providers) {
            Endpoint candidate = fromProvider(provider);
            if (candidate.valid())
                return candidate;
        }
        return Endpoint{};
    };

    if (!context.providerOverride.isEmpty()) {
        for (const ProviderInfo &provider : context.providers) {
            if (provider.id != context.providerOverride)
                continue;
            Endpoint candidate = fromProvider(provider);
            if (candidate.valid())
                return candidate;
        }
    }

    if (context.source == QLatin1String("deepseek")) {
        endpoint = fromDeepSeek();
        if (endpoint.valid())
            return endpoint;
    } else if (context.source == QLatin1String("harness")) {
        endpoint = defaultProvider();
        if (!endpoint.valid())
            endpoint = firstKeyed();
        if (endpoint.valid())
            return endpoint;
    } else if (context.source == QLatin1String("4crasy")) {
        endpoint = fromRouter();
        if (endpoint.valid())
            return endpoint;
    } else {
        endpoint = defaultProvider();
        if (!endpoint.valid())
            endpoint = firstKeyed();
        if (endpoint.valid())
            return endpoint;
        endpoint = fromDeepSeek();
        if (endpoint.valid())
            return endpoint;
        endpoint = fromRouter();
        if (endpoint.valid())
            return endpoint;
    }
    return {};
}

DecisionEngine::Endpoint DecisionEngine::fallback(const QStringList &usedSources) const {
    return fallbackFrom(context(), usedSources);
}

DecisionEngine::Endpoint DecisionEngine::fallbackFrom(const Context &context,
                                                      const QStringList &usedSources) {
    Context candidate = context;
    const QStringList order{QStringLiteral("harness"), QStringLiteral("deepseek"),
                            QStringLiteral("4crasy")};
    for (const QString &source : order) {
        if (usedSources.contains(source))
            continue;
        candidate.source = source;
        const Endpoint endpoint = resolve(candidate);
        if (endpoint.valid())
            return endpoint;
    }
    return {};
}

void DecisionEngine::refresh() {
    m_endpoint = resolve(context());
    emit changed();
}

DecisionEngine::Decision DecisionEngine::decide(const QString &prompt, const QString &risk,
                                                const QString &mode, int round) const {
    Decision decision;
    decision.mode = mode.isEmpty() ? QStringLiteral("build") : mode;
    decision.permission = m_permissionCeiling;
    if (m_permissionCeiling == QLatin1String("read-only")) {
        decision.permission = QStringLiteral("read-only");
    } else if (m_autonomy != QLatin1String("auto") && risk == QLatin1String("high")) {
        decision.permission = QStringLiteral("workspace-write");
    } else if (m_autonomy == QLatin1String("auto") && m_permissionCeiling == QLatin1String("full")) {
        decision.permission = QStringLiteral("full");
    }
    decision.retries = m_allowFallback ? m_maxRetries : 0;
    decision.fallback = m_allowFallback;
    decision.review = m_reviewChanges && (risk == QLatin1String("high") || decision.mode == QLatin1String("build"));
    decision.memory = m_useMemory;
    decision.learning = m_useLearning;
    decision.stop = round >= m_maxRounds;
    QStringList reasons;
    reasons << QStringLiteral("mode=%1").arg(decision.mode)
            << QStringLiteral("permission=%1").arg(decision.permission)
            << QStringLiteral("strategy=%1").arg(m_strategy);
    if (decision.fallback)
        reasons << QStringLiteral("retries=%1").arg(decision.retries);
    if (decision.review)
        reasons << QStringLiteral("review=on");
    if (decision.stop)
        reasons << QStringLiteral("stop=round-limit");
    if (!prompt.isEmpty())
        reasons << QStringLiteral("prompt=%1").arg(prompt.left(60).simplified());
    decision.reason = reasons.join(QStringLiteral(" · "));
    return decision;
}

QString DecisionEngine::explain(const QString &prompt, const QString &risk,
                                const QString &mode, int round) const {
    const Decision decision = decide(prompt, risk, mode, round);
    const Endpoint endpoint = resolve(context());
    return QStringLiteral("%1\nprovider=%2 model=%3 source=%4\n%5")
        .arg(decision.reason,
             endpoint.valid() ? endpoint.label : QStringLiteral("none"),
             endpoint.valid() ? endpoint.model : QStringLiteral("none"),
             endpoint.valid() ? endpoint.source : QStringLiteral("none"),
             summary());
}

QString DecisionEngine::summary() const {
    const Endpoint endpoint = m_endpoint.valid() ? m_endpoint : resolve(context());
    return QStringLiteral("%1 · %2 · %3 · rounds %4 · retries %5 · fallback %6 · review %7 · memory %8 · learning %9")
        .arg(endpoint.valid() ? endpoint.label : tr("Aucun provider"),
             endpoint.valid() ? endpoint.model : tr("aucun modèle"),
             m_strategy,
             QString::number(m_maxRounds),
             QString::number(m_maxRetries),
             m_allowFallback ? QStringLiteral("on") : QStringLiteral("off"),
             m_reviewChanges ? QStringLiteral("on") : QStringLiteral("off"),
             m_useMemory ? QStringLiteral("on") : QStringLiteral("off"),
             m_useLearning ? QStringLiteral("on") : QStringLiteral("off"));
}

QVariantMap DecisionEngine::status() const {
    const Endpoint endpoint = m_endpoint.valid() ? m_endpoint : resolve(context());
    return QVariantMap{
        {QStringLiteral("provider"), endpoint.valid() ? endpoint.label : QString()},
        {QStringLiteral("model"), endpoint.valid() ? endpoint.model : QString()},
        {QStringLiteral("source"), endpoint.valid() ? endpoint.source : QString()},
        {QStringLiteral("baseUrl"), endpoint.valid() ? endpoint.baseUrl : QString()},
        {QStringLiteral("strategy"), m_strategy},
        {QStringLiteral("autonomy"), m_autonomy},
        {QStringLiteral("providerSource"), m_providerSource},
        {QStringLiteral("permissionCeiling"), m_permissionCeiling},
        {QStringLiteral("maxRounds"), m_maxRounds},
        {QStringLiteral("maxRetries"), m_maxRetries},
        {QStringLiteral("allowFallback"), m_allowFallback},
        {QStringLiteral("reviewChanges"), m_reviewChanges},
        {QStringLiteral("useMemory"), m_useMemory},
        {QStringLiteral("useLearning"), m_useLearning},
        {QStringLiteral("summary"), summary()},
        {QStringLiteral("lastReason"), m_lastDecision.reason},
    };
}

void DecisionEngine::resetOptions() {
    m_strategy = QStringLiteral("balanced");
    m_autonomy = QStringLiteral("assisted");
    m_providerSource = QStringLiteral("auto");
    m_permissionCeiling = QStringLiteral("workspace-write");
    m_maxRounds = 100;
    m_maxRetries = 2;
    m_allowFallback = true;
    m_reviewChanges = true;
    m_useMemory = true;
    m_useLearning = true;
    touch();
}

QString DecisionEngine::credential(const QString &name) {
    const QString env = qEnvironmentVariable(name.toUtf8().constData());
    if (!env.isEmpty())
        return env;
    const QStringList files{
        QDir::homePath() + QStringLiteral("/.dsh/.credentials.yaml"),
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/agent.env"),
    };
    for (const QString &path : files) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QString text = QString::fromUtf8(file.readAll());
        const QRegularExpression expression(
            QStringLiteral("(?m)^\\s*%1\\s*[:=]\\s*(\\S+)")
                .arg(QRegularExpression::escape(name)));
        const auto match = expression.match(text);
        if (match.hasMatch())
            return match.captured(1).trimmed().remove(QLatin1Char('"')).remove(QLatin1Char('\''));
    }
    return {};
}

void DecisionEngine::setStrategy(const QString &value) {
    const QString next = (value == QLatin1String("quality") || value == QLatin1String("free"))
        ? value : QStringLiteral("balanced");
    if (next == m_strategy)
        return;
    m_strategy = next;
    touch();
}

void DecisionEngine::setAutonomy(const QString &value) {
    const QString next = (value == QLatin1String("manual") || value == QLatin1String("auto"))
        ? value : QStringLiteral("assisted");
    if (next == m_autonomy)
        return;
    m_autonomy = next;
    touch();
}

void DecisionEngine::setProviderSource(const QString &value) {
    const QString next = (value == QLatin1String("deepseek") || value == QLatin1String("harness")
                          || value == QLatin1String("4crasy"))
        ? value : QStringLiteral("auto");
    if (next == m_providerSource)
        return;
    m_providerSource = next;
    touch();
}

void DecisionEngine::setPermissionCeiling(const QString &value) {
    const QString next = (value == QLatin1String("read-only") || value == QLatin1String("full"))
        ? value : QStringLiteral("workspace-write");
    if (next == m_permissionCeiling)
        return;
    m_permissionCeiling = next;
    touch();
}

void DecisionEngine::setMaxRounds(int value) {
    const int next = qBound(1, value, 500);
    if (next == m_maxRounds)
        return;
    m_maxRounds = next;
    touch();
}

void DecisionEngine::setMaxRetries(int value) {
    const int next = qBound(0, value, 10);
    if (next == m_maxRetries)
        return;
    m_maxRetries = next;
    touch();
}

void DecisionEngine::setAllowFallback(bool value) {
    if (value == m_allowFallback)
        return;
    m_allowFallback = value;
    touch();
}

void DecisionEngine::setReviewChanges(bool value) {
    if (value == m_reviewChanges)
        return;
    m_reviewChanges = value;
    touch();
}

void DecisionEngine::setUseMemory(bool value) {
    if (value == m_useMemory)
        return;
    m_useMemory = value;
    touch();
}

void DecisionEngine::setUseLearning(bool value) {
    if (value == m_useLearning)
        return;
    m_useLearning = value;
    touch();
}
