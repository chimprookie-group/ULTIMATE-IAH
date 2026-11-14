#pragma once

#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QVariantList>

class QNetworkAccessManager;
class QNetworkReply;

// Native provider and MCP connections for the DeepSeek Harness. Expands the
// harness profile with llm-pi-ai routes, stores API keys in the IAH credential
// store, and writes a stable overlay consumed by HarnessServer.
class HarnessConnections : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList providers READ providers NOTIFY changed)
    Q_PROPERTY(QString defaultModel READ defaultModel NOTIFY changed)
    Q_PROPERTY(QString defaultProvider READ defaultProvider NOTIFY changed)
    Q_PROPERTY(QString defaultModelId READ defaultModelId NOTIFY changed)
    Q_PROPERTY(bool deepseekConfigured READ deepseekConfigured NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString envPath READ envPath NOTIFY changed)
    Q_PROPERTY(bool detecting READ detecting NOTIFY detectingChanged)
    Q_PROPERTY(QString detectionStatus READ detectionStatus NOTIFY detectionStatusChanged)
    Q_PROPERTY(QVariantList detectedModels READ detectedModels NOTIFY detectedModelsChanged)

public:
    explicit HarnessConnections(const QString &overlayPath = {}, QObject *parent = nullptr);

    QVariantList providers() const { return m_providers; }
    QString defaultModel() const { return m_defaultModel; }
    QString defaultProvider() const { return m_defaultProvider; }
    QString defaultModelId() const { return m_defaultModelId; }
    bool deepseekConfigured() const { return m_deepseekConfigured; }
    QString error() const { return m_error; }
    QString envPath() const { return m_envPath; }
    bool detecting() const { return m_detecting; }
    QString detectionStatus() const { return m_detectionStatus; }
    QVariantList detectedModels() const { return m_detectedModels; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void saveDeepSeek(const QString &apiKey);
    Q_INVOKABLE void saveProvider(const QString &id, const QString &name,
                                  const QString &baseUrl, const QString &apiKey,
                                  const QString &modelsText, bool makeDefault,
                                  const QString &api = QStringLiteral("openai-completions"));
    Q_INVOKABLE void removeProvider(const QString &id);
    Q_INVOKABLE void setDefault(const QString &provider, const QString &model);
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void detectModels(const QString &id);
    Q_INVOKABLE void detectModelsFor(const QString &baseUrl, const QString &apiKey,
                                     const QString &api, const QString &providerId = {});
    Q_INVOKABLE void clearDetectedModels();
    QString providerApiKey(const QString &id) const;

    static QString defaultOverlayPath();
    static QJsonArray parseModels(const QJsonObject &root);

signals:
    void changed();
    void errorChanged();
    void deepSeekChanged();
    void detectingChanged();
    void detectionStatusChanged();
    void detectedModelsChanged();

private:
    bool run(const QJsonObject &request, QJsonObject &result);
    void setError(const QString &error);
    void setDetecting(bool detecting);
    void setDetectionStatus(const QString &status);
    void startModelDetection(const QString &baseUrl, const QString &apiKey, const QString &api,
                             const QString &providerId, const QString &providerName,
                             const QString &apiKeyEnv, bool persist);
    QString harnessProfile() const;
    QString pythonHelper() const;

    QString m_overlayPath;
    QString m_envPath;
    QVariantList m_providers;
    QString m_defaultModel;
    QString m_defaultProvider;
    QString m_defaultModelId;
    QString m_error;
    bool m_deepseekConfigured = false;
    bool m_detecting = false;
    QString m_detectionStatus;
    QVariantList m_detectedModels;
    QNetworkAccessManager *m_network = nullptr;
    QPointer<QNetworkReply> m_detectReply;
    QString m_detectProvider;
};
