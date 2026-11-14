#pragma once

#include <QJsonObject>
#include <QObject>
#include <QTcpServer>
#include <QVariantList>
#include <QVariantMap>

class QNetworkAccessManager;
class QNetworkReply;
class QTcpSocket;

// 4Crasy: native free-model router. It exposes an OpenAI-compatible loopback
// API, discovers free models, chooses a channel per task and falls back when a
// provider is unavailable. Provider keys stay in the environment/keyring side
// of the existing connections; the router only stores its own access token.
class CrasyRouter : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY stateChanged)
    Q_PROPERTY(bool listening READ listening NOTIFY stateChanged)
    Q_PROPERTY(bool healthy READ healthy NOTIFY stateChanged)
    Q_PROPERTY(QString endpoint READ endpoint NOTIFY stateChanged)
    Q_PROPERTY(QString token READ token NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QVariantList channels READ channels NOTIFY changed)
    Q_PROPERTY(QVariantList models READ models NOTIFY changed)
    Q_PROPERTY(int freeModelCount READ freeModelCount NOTIFY changed)
    Q_PROPERTY(int requestCount READ requestCount NOTIFY changed)
    Q_PROPERTY(QVariantList recentRoutes READ recentRoutes NOTIFY changed)
    Q_PROPERTY(bool autoRefresh READ autoRefresh WRITE setAutoRefresh NOTIFY changed)
    Q_PROPERTY(bool preferFree READ preferFree WRITE setPreferFree NOTIFY changed)
    Q_PROPERTY(bool freeOnly READ freeOnly WRITE setFreeOnly NOTIFY changed)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY changed)
    Q_PROPERTY(int maxFallbacks READ maxFallbacks WRITE setMaxFallbacks NOTIFY changed)
    Q_PROPERTY(QString strategy READ strategy WRITE setStrategy NOTIFY changed)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY changed)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)
    Q_PROPERTY(QString configState READ configState NOTIFY changed)

public:
    explicit CrasyRouter(const QString &configDir = {}, QObject *parent = nullptr);
    ~CrasyRouter() override;

    bool running() const { return m_server.isListening(); }
    bool listening() const { return m_server.isListening(); }
    bool healthy() const;
    QString endpoint() const;
    QString token() const { return m_token; }
    QString error() const { return m_error; }
    QVariantList channels() const { return m_channels; }
    QVariantList models() const { return m_models; }
    int freeModelCount() const;
    int requestCount() const { return m_requestCount; }
    QVariantList recentRoutes() const { return m_recentRoutes; }
    bool autoRefresh() const { return m_autoRefresh; }
    bool preferFree() const { return m_preferFree; }
    bool freeOnly() const { return m_freeOnly; }
    int port() const { return m_port; }
    int maxFallbacks() const { return m_maxFallbacks; }
    QString strategy() const { return m_strategy; }
    bool autoStart() const { return m_autoStart; }
    QString configState() const { return m_configState; }

    Q_INVOKABLE bool start(quint16 preferredPort = 8788);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshModels();
    Q_INVOKABLE bool setChannelEnabled(const QString &id, bool enabled);
    Q_INVOKABLE bool saveChannel(const QString &id, const QString &name, const QString &baseUrl,
                                 const QString &apiKeyEnv, const QString &apiKey,
                                 int priority, bool free, bool enabled);
    Q_INVOKABLE bool removeChannel(const QString &id);
    Q_INVOKABLE QString testChannel(const QString &id);
    Q_INVOKABLE void resetChannels();
    Q_INVOKABLE void saveSettings(int port, bool freeOnly, int maxFallbacks,
                                  bool autoStart, const QString &strategy);
    Q_INVOKABLE QString routePreview(const QString &prompt = {},
                                     const QString &requestedModel = QStringLiteral("auto"));
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void copyToken() const;
    void setAutoRefresh(bool enabled);
    void setPreferFree(bool enabled);
    void setFreeOnly(bool enabled);
    void setPort(int value);
    void setMaxFallbacks(int value);
    void setStrategy(const QString &value);
    void setAutoStart(bool value);

    QJsonObject status() const;
    static QVariantList parseModels(const QJsonArray &array, const QString &channel);

signals:
    void stateChanged();
    void changed();
    void errorChanged();

private:
    struct Channel {
        QString id;
        QString name;
        QString baseUrl;
        QString apiKeyEnv;
        int priority = 5;
        bool enabled = true;
        bool free = true;
        QString modelsPath;
        QString health;
        QString apiKey;
        QStringList modelIds;
        QStringList freeModelIds;
    };

    void loadConfig();
    bool saveConfig() const;
    void rebuildChannelView();
    void addModel(const QString &channel, const QJsonObject &model);
    QString resolveKey(const QString &envName) const;
    QString chooseChannel(const QJsonObject &request, const QString &requestedModel,
                          QJsonObject *selection = nullptr) const;
    bool proxyChat(QTcpSocket *socket, const QJsonObject &request, const QByteArray &raw);
    void sendProxy(QTcpSocket *socket, const Channel &channel, const QJsonObject &request,
                   const QString &model, const QJsonArray &fallbacks, int attempt);
    void handleConnection(QTcpSocket *socket);
    void respond(QTcpSocket *socket, int status, const QByteArray &type, const QByteArray &body);
    bool authorized(const QHash<QByteArray, QByteArray> &headers) const;
    void setError(const QString &error);
    QString configPath() const;
    QString tokenPath() const;
    static QByteArray httpStatus(int code);

    QTcpServer m_server;
    QNetworkAccessManager *m_network = nullptr;
    QString m_configDir;
    QList<Channel> m_channelData;
    QVariantList m_channels;
    QVariantList m_models;
    QVariantList m_recentRoutes;
    QString m_token;
    QString m_error;
    int m_requestCount = 0;
    qint64 m_lastServedAt = 0;
    bool m_autoRefresh = true;
    bool m_preferFree = true;
    bool m_freeOnly = false;
    int m_port = 8788;
    int m_maxFallbacks = 3;
    QString m_strategy = QStringLiteral("balanced");
    bool m_autoStart = false;
    QString m_configState = QStringLiteral("loaded");
};
