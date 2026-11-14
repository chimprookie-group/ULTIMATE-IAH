#pragma once

#include <QJsonObject>
#include <QObject>
#include <QVariantList>
#include <QStringList>

class QNetworkAccessManager;

// Native web capability for Ultimate Agent: search + bounded fetch, cache,
// provider reporting and DeepSeek native-search credentials. It is the native
// equivalent of the DSH web service/tool-web/web-fetch-http trio.
class WebService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString searchProvider READ searchProvider NOTIFY changed)
    Q_PROPERTY(bool searchReady READ searchReady NOTIFY changed)
    Q_PROPERTY(int cacheEntries READ cacheEntries NOTIFY changed)
    Q_PROPERTY(QVariantList recentSearches READ recentSearches NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit WebService(QObject *parent = nullptr);

    QString searchProvider() const { return m_searchProvider; }
    bool searchReady() const { return !deepSeekKey().isEmpty(); }
    int cacheEntries() const { return m_cache.size(); }
    QVariantList recentSearches() const { return m_recent; }
    QString error() const { return m_error; }

    QJsonObject search(const QStringList &queries, int maxResults = 8);
    QJsonObject fetch(const QString &url);
    static QString siteOf(const QUrl &url);
    static bool redirectAllowed(const QUrl &from, const QUrl &to,
                                const QStringList &allowedHosts = {});
    QJsonObject status() const;
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE void clearError();

signals:
    void changed();
    void errorChanged();

private:
    struct CacheEntry {
        QJsonObject value;
        qint64 createdAt = 0;
    };

    QString deepSeekKey() const;
    QJsonObject deepSeekSearch(const QStringList &queries, int maxResults);
    QJsonObject duckDuckGoSearch(const QStringList &queries, int maxResults);

protected:
    QJsonObject httpGet(const QString &url, int maxBytes = 5000000, int timeoutMs = 30000);

private:
    QJsonObject httpPost(const QString &url, const QByteArray &body,
                         const QList<QPair<QByteArray, QByteArray>> &headers,
                         int timeoutMs = 30000);
    QString htmlToText(const QString &html) const;
    QString decodeEntities(const QString &text) const;
    QString cleanUrl(const QString &url) const;
    bool blockedHost(const QUrl &url) const;
    static QJsonObject errorObject(const QString &code, const QString &message);
    void setError(const QString &error);
    void rememberSearch(const QString &provider, const QStringList &queries, int count);
    void rememberCache(const QString &key, const QJsonObject &value);

    QNetworkAccessManager *m_network = nullptr;
    QHash<QString, CacheEntry> m_cache;
    QVariantList m_recent;
    QString m_searchProvider = QStringLiteral("deepseek");
    QString m_error;
};
