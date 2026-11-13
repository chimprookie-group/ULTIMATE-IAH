#include "webservice.h"

#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QStandardPaths>
#include <QTimer>

namespace {
constexpr int kCacheTtlMs = 10 * 60 * 1000;

QString jsonString(const QJsonObject &object, const QString &key) {
    return object.value(key).toString();
}
} // namespace

WebService::WebService(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this)) {}

QString WebService::deepSeekKey() const {
    QStringList files = {
        qEnvironmentVariable("DSH_HOME", QDir::homePath() + QStringLiteral("/.dsh"))
            + QStringLiteral("/.credentials.yaml"),
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/agent.env"),
    };
    for (const QString &path : files) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QString text = QString::fromUtf8(file.readAll());
        QRegularExpression expression(QStringLiteral("(?m)^\\s*DEEPSEEK_API_KEY\\s*[:=]\\s*(\\S+)"));
        auto match = expression.match(text);
        if (match.hasMatch()) {
            QString value = match.captured(1).trimmed();
            value.remove(QRegularExpression(QStringLiteral("^[\\\"']|[\\\"']$")));
            return value;
        }
    }
    return {};
}

QJsonObject WebService::errorObject(const QString &code, const QString &message) {
    return QJsonObject{{QStringLiteral("ok"), false},
                       {QStringLiteral("code"), code},
                       {QStringLiteral("error"), message}};
}

void WebService::setError(const QString &error) {
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

void WebService::clearError() { setError({}); }

void WebService::clearCache() {
    m_cache.clear();
    emit changed();
}

void WebService::rememberSearch(const QString &provider, const QStringList &queries, int count) {
    m_searchProvider = provider;
    m_recent.prepend(QVariantMap{{QStringLiteral("queries"), queries.join(QStringLiteral(" · "))},
                                 {QStringLiteral("provider"), provider},
                                 {QStringLiteral("count"), count},
                                 {QStringLiteral("time"), QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"))}});
    while (m_recent.size() > 12)
        m_recent.removeLast();
    emit changed();
}

QJsonObject WebService::status() const {
    return QJsonObject{
        {QStringLiteral("ok"), true},
        {QStringLiteral("searchProvider"), m_searchProvider},
        {QStringLiteral("searchReady"), searchReady()},
        {QStringLiteral("cacheEntries"), m_cache.size()},
        {QStringLiteral("fetchAvailable"), true},
        {QStringLiteral("searchEndpoint"), qEnvironmentVariable("DEEPSEEK_SEARCH_BASE_URL",
                                                                 QStringLiteral("https://api.deepseek.com/anthropic/v1"))},
    };
}

QJsonObject WebService::search(const QStringList &queries, int maxResults) {
    QStringList unique;
    for (const QString &query : queries) {
        const QString clean = query.trimmed();
        if (!clean.isEmpty() && !unique.contains(clean))
            unique.append(clean);
    }
    if (unique.isEmpty()) {
        const QJsonObject result = errorObject(QStringLiteral("WEB_INVALID_QUERY"),
                                               tr("La recherche nécessite au moins une requête non vide."));
        setError(result.value(QStringLiteral("error")).toString());
        return result;
    }
    if (unique.size() > 4)
        unique = unique.mid(0, 4);
    const QString cacheKey = QStringLiteral("search:") + unique.join(QChar(0x1f));
    const auto cached = m_cache.constFind(cacheKey);
    if (cached != m_cache.constEnd() && QDateTime::currentMSecsSinceEpoch() - cached->createdAt < kCacheTtlMs)
        return cached->value;

    QJsonObject result;
    if (searchReady()) {
        result = deepSeekSearch(unique, maxResults);
        if (result.value(QStringLiteral("ok")).toBool())
            m_searchProvider = QStringLiteral("deepseek");
        else
            result = duckDuckGoSearch(unique, maxResults);
    } else {
        result = duckDuckGoSearch(unique, maxResults);
    }
    if (!result.value(QStringLiteral("ok")).toBool()) {
        setError(result.value(QStringLiteral("error")).toString());
        return result;
    }
    setError({});
    m_searchProvider = result.value(QStringLiteral("provider")).toString(m_searchProvider);
    m_cache.insert(cacheKey, CacheEntry{result, QDateTime::currentMSecsSinceEpoch()});
    rememberSearch(m_searchProvider, unique, result.value(QStringLiteral("sources")).toArray().size());
    return result;
}

QJsonObject WebService::deepSeekSearch(const QStringList &queries, int maxResults) {
    const QString base = qEnvironmentVariable("DEEPSEEK_SEARCH_BASE_URL",
                                               QStringLiteral("https://api.deepseek.com/anthropic/v1"));
    const QString endpoint = base.endsWith(QLatin1Char('/')) ? base + QStringLiteral("messages") : base + QStringLiteral("/messages");
    const QString model = qEnvironmentVariable("DEEPSEEK_SEARCH_MODEL", QStringLiteral("deepseek-v4-flash"));
    QJsonArray sources;
    QString error;
    for (const QString &query : queries) {
        QJsonObject body{
            {QStringLiteral("model"), model},
            {QStringLiteral("max_tokens"), 4096},
            {QStringLiteral("messages"), QJsonArray{QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                                                 {QStringLiteral("content"), QStringLiteral("Perform a web search for the query: ") + query}}}},
            {QStringLiteral("tools"), QJsonArray{QJsonObject{{QStringLiteral("type"), QStringLiteral("web_search_20250305")},
                                                              {QStringLiteral("name"), QStringLiteral("web_search")},
                                                              {QStringLiteral("max_uses"), 5}}}},
        };
        const QJsonObject reply = httpPost(endpoint, QJsonDocument(body).toJson(QJsonDocument::Compact),
            {{QByteArray("x-api-key"), deepSeekKey().toUtf8()},
             {QByteArray("Authorization"), QByteArray("Bearer ") + deepSeekKey().toUtf8()},
             {QByteArray("anthropic-version"), QByteArray("2023-06-01")}}, 30000);
        if (!reply.value(QStringLiteral("ok")).toBool()) {
            error = reply.value(QStringLiteral("error")).toString();
            break;
        }
        const QJsonArray content = QJsonDocument::fromJson(reply.value(QStringLiteral("body")).toString().toUtf8())
                                       .object().value(QStringLiteral("content")).toArray();
        for (const QJsonValue &blockValue : content) {
            const QJsonObject block = blockValue.toObject();
            if (block.value(QStringLiteral("type")).toString() != QLatin1String("web_search_tool_result"))
                continue;
            for (const QJsonValue &resultValue : block.value(QStringLiteral("content")).toArray()) {
                const QJsonObject item = resultValue.toObject();
                const QString url = item.value(QStringLiteral("url")).toString();
                if (url.isEmpty())
                    continue;
                sources.append(QJsonObject{{QStringLiteral("url"), url},
                                           {QStringLiteral("title"), item.value(QStringLiteral("title")).toString(url)},
                                           {QStringLiteral("date"), item.value(QStringLiteral("page_age")).toString()},
                                           {QStringLiteral("snippet"), item.value(QStringLiteral("cited_text")).toString()}});
            }
        }
        if (sources.size() >= maxResults)
            break;
    }
    if (sources.isEmpty())
        return errorObject(QStringLiteral("WEB_PROVIDER_ERROR"),
                           error.isEmpty() ? tr("DeepSeek n'a renvoyé aucun résultat de recherche.") : error);
    if (sources.size() > maxResults) {
        QJsonArray trimmed;
        for (int i = 0; i < maxResults; ++i)
            trimmed.append(sources.at(i));
        sources = trimmed;
    }
    return QJsonObject{{QStringLiteral("ok"), true},
                       {QStringLiteral("provider"), QStringLiteral("deepseek")},
                       {QStringLiteral("queries"), QJsonArray::fromStringList(queries)},
                       {QStringLiteral("sources"), sources}};
}

QJsonObject WebService::duckDuckGoSearch(const QStringList &queries, int maxResults) {
    QJsonArray sources;
    for (const QString &query : queries) {
        const QUrl url(QStringLiteral("https://html.duckduckgo.com/html/"));
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("UltimateIAH/1.10 web_search"));
        QUrlQuery form;
        form.addQueryItem(QStringLiteral("q"), query);
        QNetworkReply *reply = m_network->post(request, form.toString(QUrl::FullyEncoded).toUtf8());
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        timer.setInterval(15000);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start();
        loop.exec();
        if (!reply->isFinished()) {
            reply->abort();
            reply->deleteLater();
            return errorObject(QStringLiteral("WEB_FETCH_TIMEOUT"), tr("Délai de recherche dépassé."));
        }
        const QString html = QString::fromUtf8(reply->readAll());
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        if (status < 200 || status >= 300)
            return errorObject(QStringLiteral("WEB_PROVIDER_ERROR"), tr("DuckDuckGo a répondu HTTP %1.").arg(status));
        const QRegularExpression resultExpression(
            QStringLiteral("<a[^>]+class=\"result__a\"[^>]+href=\"([^\"]+)\"[^>]*>(.*?)</a>.*?<a[^>]+class=\"result__snippet\"[^>]*>(.*?)</a>"),
            QRegularExpression::DotMatchesEverythingOption);
        auto iterator = resultExpression.globalMatch(html);
        while (iterator.hasNext() && sources.size() < maxResults) {
            const auto match = iterator.next();
            QString href = match.captured(1);
            href.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
            QUrl target(href);
            if (target.host().endsWith(QStringLiteral("duckduckgo.com"))) {
                const QString uddg = QUrlQuery(target).queryItemValue(QStringLiteral("uddg"));
                if (!uddg.isEmpty())
                    target = QUrl(QUrl::fromPercentEncoding(uddg.toUtf8()));
            }
            if (!target.isValid() || target.scheme().isEmpty())
                continue;
            sources.append(QJsonObject{{QStringLiteral("url"), target.toString()},
                                       {QStringLiteral("title"), decodeEntities(htmlToText(match.captured(2))).trimmed()},
                                       {QStringLiteral("snippet"), decodeEntities(htmlToText(match.captured(3))).trimmed()},
                                       {QStringLiteral("date"), QString()}});
        }
        if (sources.size() >= maxResults)
            break;
    }
    if (sources.isEmpty())
        return errorObject(QStringLiteral("WEB_PROVIDER_UNAVAILABLE"),
                           tr("Aucun fournisseur de recherche n'est disponible."));
    return QJsonObject{{QStringLiteral("ok"), true},
                       {QStringLiteral("provider"), QStringLiteral("duckduckgo")},
                       {QStringLiteral("queries"), QJsonArray::fromStringList(queries)},
                       {QStringLiteral("sources"), sources}};
}

QJsonObject WebService::httpPost(const QString &url, const QByteArray &body,
                                 const QList<QPair<QByteArray, QByteArray>> &headers,
                                 int timeoutMs) {
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("UltimateIAH/1.10 web_search"));
    for (const auto &header : headers)
        request.setRawHeader(header.first, header.second);
    QNetworkReply *reply = m_network->post(request, body);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMs);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();
    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        return errorObject(QStringLiteral("WEB_FETCH_TIMEOUT"), tr("Délai de recherche dépassé."));
    }
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray data = reply->readAll();
    const QString networkError = reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
    reply->deleteLater();
    if (status < 200 || status >= 300)
        return errorObject(QStringLiteral("WEB_PROVIDER_ERROR"),
                           networkError.isEmpty() ? tr("Le fournisseur de recherche a répondu HTTP %1.").arg(status)
                                                  : networkError);
    return QJsonObject{{QStringLiteral("ok"), true}, {QStringLiteral("status"), status},
                       {QStringLiteral("body"), QString::fromUtf8(data)}};
}

QString WebService::decodeEntities(const QString &text) const {
    QString value = text;
    static const QList<QPair<QString, QString>> entities = {
        {QStringLiteral("&amp;"), QStringLiteral("&")}, {QStringLiteral("&lt;"), QStringLiteral("<")},
        {QStringLiteral("&gt;"), QStringLiteral(">")}, {QStringLiteral("&quot;"), QStringLiteral("\"")},
        {QStringLiteral("&#39;"), QStringLiteral("'")}, {QStringLiteral("&nbsp;"), QStringLiteral(" ")},
    };
    for (const auto &entity : entities)
        value.replace(entity.first, entity.second);
    return value;
}

QString WebService::htmlToText(const QString &html) const {
    QString value = html;
    value.remove(QRegularExpression(QStringLiteral("<script[\\s\\S]*?</script>"), QRegularExpression::CaseInsensitiveOption));
    value.remove(QRegularExpression(QStringLiteral("<style[\\s\\S]*?</style>"), QRegularExpression::CaseInsensitiveOption));
    value.remove(QRegularExpression(QStringLiteral("<head[\\s\\S]*?</head>"), QRegularExpression::CaseInsensitiveOption));
    value.remove(QRegularExpression(QStringLiteral("<[^>]+>")));
    value = decodeEntities(value);
    value.replace(QRegularExpression(QStringLiteral("[ \\t]+")), QStringLiteral(" "));
    value.replace(QRegularExpression(QStringLiteral("\\n\\s*\\n+")), QStringLiteral("\n\n"));
    return value.trimmed();
}

bool WebService::blockedHost(const QUrl &url) const {
    const QString host = url.host().toLower();
    if (host.isEmpty() || host == QLatin1String("localhost") || host.endsWith(QLatin1String(".local"))
        || host.endsWith(QLatin1String(".internal")))
        return true;
    QHostAddress address;
    if (!address.setAddress(host))
        return false;
    if (address.isLoopback() || address.isLinkLocal() || address.isSiteLocal())
        return true;
    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        const quint32 value = address.toIPv4Address();
        return (value >> 24) == 0 || (value >> 24) == 127 || (value >> 24) >= 224
            || (value & 0xffff0000U) == 0xa9fe0000U
            || (value & 0xffc00000U) == 0x64400000U;
    }
    if (address.protocol() == QAbstractSocket::IPv6Protocol) {
        const Q_IPV6ADDR ipv6 = address.toIPv6Address();
        if ((ipv6[0] & 0xfe) == 0xfc || (ipv6[0] == 0xfe && (ipv6[1] & 0xc0) == 0x80))
            return true;
    }
    return false;
}

QString WebService::cleanUrl(const QString &url) const {
    QUrl parsed(url.trimmed());
    if (!parsed.isValid() || (parsed.scheme() != QLatin1String("http") && parsed.scheme() != QLatin1String("https")))
        return {};
    if (parsed.toString().size() > 2048 || blockedHost(parsed))
        return {};
    return parsed.toString();
}

QJsonObject WebService::httpGet(const QString &url, int maxBytes, int timeoutMs) {
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("UltimateIAH/1.10 web_fetch"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    QNetworkReply *reply = m_network->get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMs);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();
    if (!reply->isFinished()) {
        reply->abort();
        reply->deleteLater();
        return errorObject(QStringLiteral("WEB_FETCH_TIMEOUT"), tr("Délai de récupération dépassé."));
    }
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    QByteArray data = reply->read(maxBytes + 1);
    const QString networkError = reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
    const QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    reply->deleteLater();
    return QJsonObject{{QStringLiteral("ok"), status >= 200 && status < 400},
                       {QStringLiteral("status"), status},
                       {QStringLiteral("redirect"), redirect.toString()},
                       {QStringLiteral("contentType"), contentType},
                       {QStringLiteral("truncated"), data.size() > maxBytes},
                       {QStringLiteral("body"), QString::fromUtf8(data.left(maxBytes))},
                       {QStringLiteral("error"), networkError}};
}

QJsonObject WebService::fetch(const QString &url) {
    QUrl requested(url.trimmed());
    if (requested.isValid() && blockedHost(requested))
        return errorObject(QStringLiteral("WEB_PRIVATE_HOST"),
                           tr("Les adresses locales et privées sont bloquées pour web_fetch."));
    const QString clean = cleanUrl(url);
    if (clean.isEmpty())
        return errorObject(QStringLiteral("WEB_INVALID_URL"), tr("URL HTTP(S) invalide."));
    const QString cacheKey = QStringLiteral("fetch:") + clean;
    const auto cached = m_cache.constFind(cacheKey);
    if (cached != m_cache.constEnd() && QDateTime::currentMSecsSinceEpoch() - cached->createdAt < kCacheTtlMs)
        return cached->value;

    QString current = clean;
    QUrl origin = QUrl(clean);
    QJsonObject reply;
    for (int hop = 0; hop <= 5; ++hop) {
        reply = httpGet(current, 5000000, 30000);
        if (!reply.value(QStringLiteral("redirect")).toString().isEmpty()
            && reply.value(QStringLiteral("status")).toInt() >= 300
            && reply.value(QStringLiteral("status")).toInt() < 400) {
            const QUrl target = QUrl(current).resolved(QUrl(reply.value(QStringLiteral("redirect")).toString()));
            if (target.host() != origin.host() || target.scheme() != origin.scheme()
                || blockedHost(target))
                return errorObject(QStringLiteral("WEB_REDIRECT_BLOCKED"), tr("Redirection vers une autre origine refusée."));
            current = target.toString();
            continue;
        }
        break;
    }
    if (!reply.value(QStringLiteral("ok")).toBool() && reply.value(QStringLiteral("error")).toString().isEmpty())
        return errorObject(QStringLiteral("WEB_PROVIDER_ERROR"), tr("La page n'a pas pu être récupérée."));
    const int status = reply.value(QStringLiteral("status")).toInt();
    if (status < 200 || status >= 300)
        return errorObject(QStringLiteral("WEB_PROVIDER_ERROR"), tr("La page a répondu HTTP %1.").arg(status));
    const QString type = reply.value(QStringLiteral("contentType")).toString().toLower();
    if (!(type.contains(QStringLiteral("text/")) || type.contains(QStringLiteral("json")) || type.contains(QStringLiteral("xml"))
          || type.contains(QStringLiteral("javascript")) || type.contains(QStringLiteral("xhtml"))))
        return errorObject(QStringLiteral("WEB_UNSUPPORTED_CONTENT_TYPE"), tr("Type de contenu non pris en charge : %1.").arg(type));
    const QString body = htmlToText(reply.value(QStringLiteral("body")).toString());
    QJsonObject result{{QStringLiteral("ok"), true},
                       {QStringLiteral("url"), current},
                       {QStringLiteral("status"), status},
                       {QStringLiteral("contentType"), type},
                       {QStringLiteral("truncated"), reply.value(QStringLiteral("truncated")).toBool() || body.size() > 200000},
                       {QStringLiteral("body"), body.left(200000)}};
    m_cache.insert(cacheKey, CacheEntry{result, QDateTime::currentMSecsSinceEpoch()});
    emit changed();
    return result;
}
