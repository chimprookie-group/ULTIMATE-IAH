#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class CrasyRouter;
class HarnessConnections;

// Central decision core for the harness. It owns the options that drive model
// selection, permissions, retries, fallbacks, review gates, memory/learning and
// stop conditions, and resolves a usable provider from the harness provider
// system, a DeepSeek key or the local 4Crasy router.
class DecisionEngine : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString strategy READ strategy WRITE setStrategy NOTIFY optionsChanged)
    Q_PROPERTY(QString autonomy READ autonomy WRITE setAutonomy NOTIFY optionsChanged)
    Q_PROPERTY(QString providerSource READ providerSource WRITE setProviderSource NOTIFY optionsChanged)
    Q_PROPERTY(QString permissionCeiling READ permissionCeiling WRITE setPermissionCeiling NOTIFY optionsChanged)
    Q_PROPERTY(int maxRounds READ maxRounds WRITE setMaxRounds NOTIFY optionsChanged)
    Q_PROPERTY(int maxRetries READ maxRetries WRITE setMaxRetries NOTIFY optionsChanged)
    Q_PROPERTY(bool allowFallback READ allowFallback WRITE setAllowFallback NOTIFY optionsChanged)
    Q_PROPERTY(bool reviewChanges READ reviewChanges WRITE setReviewChanges NOTIFY optionsChanged)
    Q_PROPERTY(bool useMemory READ useMemory WRITE setUseMemory NOTIFY optionsChanged)
    Q_PROPERTY(bool useLearning READ useLearning WRITE setUseLearning NOTIFY optionsChanged)
    Q_PROPERTY(QString providerLabel READ providerLabel NOTIFY changed)
    Q_PROPERTY(QString modelLabel READ modelLabel NOTIFY changed)
    Q_PROPERTY(QString sourceLabel READ sourceLabel NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)
    Q_PROPERTY(QString lastReason READ lastReason NOTIFY changed)

public:
    struct ProviderInfo {
        QString id;
        QString name;
        QString baseUrl;
        QStringList models;
        QString key;
        bool free = false;
    };

    struct Context {
        QList<ProviderInfo> providers;
        QString defaultProvider;
        QString defaultModel;
        QString deepSeekKey;
        QString routerEndpoint;
        QString routerToken;
        QString routerModel = QStringLiteral("auto");
        QString providerOverride;
        QString modelOverride;
        QString source = QStringLiteral("auto");
        QString strategy = QStringLiteral("balanced");
    };

    struct Endpoint {
        QString providerId;
        QString baseUrl;
        QString apiKey;
        QString model;
        QString label;
        QString source;
        bool valid() const { return !baseUrl.isEmpty() && !apiKey.isEmpty() && !model.isEmpty(); }
    };

    struct Decision {
        QString mode;
        QString permission;
        int retries = 0;
        bool fallback = false;
        bool review = false;
        bool memory = false;
        bool learning = false;
        bool stop = false;
        QString reason;
    };

    explicit DecisionEngine(QObject *parent = nullptr);

    void setConnections(HarnessConnections *connections) { m_connections = connections; }
    void setRouter(CrasyRouter *router) { m_router = router; }
    void setOverrides(const QString &provider, const QString &model) { m_providerOverride = provider; m_modelOverride = model; }

    Context context() const;
    static Endpoint resolve(const Context &context);
    Decision decide(const QString &prompt, const QString &risk,
                    const QString &mode, int round) const;

    QString strategy() const { return m_strategy; }
    QString autonomy() const { return m_autonomy; }
    QString providerSource() const { return m_providerSource; }
    QString permissionCeiling() const { return m_permissionCeiling; }
    int maxRounds() const { return m_maxRounds; }
    int maxRetries() const { return m_maxRetries; }
    bool allowFallback() const { return m_allowFallback; }
    bool reviewChanges() const { return m_reviewChanges; }
    bool useMemory() const { return m_useMemory; }
    bool useLearning() const { return m_useLearning; }
    QString providerLabel() const { return m_endpoint.label; }
    QString modelLabel() const { return m_endpoint.model; }
    QString sourceLabel() const { return m_endpoint.source; }
    QString summary() const;
    QString lastReason() const { return m_lastDecision.reason; }
    void setLastDecision(const Decision &decision) { m_lastDecision = decision; emit changed(); }

    void setStrategy(const QString &value);
    void setAutonomy(const QString &value);
    void setProviderSource(const QString &value);
    void setPermissionCeiling(const QString &value);
    void setMaxRounds(int value);
    void setMaxRetries(int value);
    void setAllowFallback(bool value);
    void setReviewChanges(bool value);
    void setUseMemory(bool value);
    void setUseLearning(bool value);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void resetOptions();
    Q_INVOKABLE QVariantMap status() const;
    Q_INVOKABLE QString explain(const QString &prompt, const QString &risk,
                                const QString &mode, int round) const;

    static QString credential(const QString &name);

signals:
    void optionsChanged();
    void changed();

private:
    static QString pickModel(const ProviderInfo &provider, const Context &context);
    static void normalize(Endpoint &endpoint);
    QString settingsPath() const;
    void load();
    void save() const;
    void touch();

    HarnessConnections *m_connections = nullptr;
    CrasyRouter *m_router = nullptr;
    QString m_providerOverride;
    QString m_modelOverride;
    QString m_strategy = QStringLiteral("balanced");
    QString m_autonomy = QStringLiteral("assisted");
    QString m_providerSource = QStringLiteral("auto");
    QString m_permissionCeiling = QStringLiteral("workspace-write");
    int m_maxRounds = 100;
    int m_maxRetries = 2;
    bool m_allowFallback = true;
    bool m_reviewChanges = true;
    bool m_useMemory = true;
    bool m_useLearning = true;
    Endpoint m_endpoint;
    Decision m_lastDecision;
};
