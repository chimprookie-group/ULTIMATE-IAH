#pragma once

#include <QJsonObject>
#include <QObject>
#include <QVariantList>

// Native provider and MCP connections for the DeepSeek Harness. Expands the
// harness profile with llm-pi-ai routes, stores API keys in the DSH credential
// store, and writes a stable overlay consumed by HarnessServer.
class HarnessConnections : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList providers READ providers NOTIFY changed)
    Q_PROPERTY(QString defaultModel READ defaultModel NOTIFY changed)
    Q_PROPERTY(bool deepseekConfigured READ deepseekConfigured NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString envPath READ envPath NOTIFY changed)

public:
    explicit HarnessConnections(const QString &overlayPath = {}, QObject *parent = nullptr);

    QVariantList providers() const { return m_providers; }
    QString defaultModel() const { return m_defaultModel; }
    bool deepseekConfigured() const { return m_deepseekConfigured; }
    QString error() const { return m_error; }
    QString envPath() const { return m_envPath; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void saveDeepSeek(const QString &apiKey);
    Q_INVOKABLE void saveProvider(const QString &id, const QString &name,
                                  const QString &baseUrl, const QString &apiKey,
                                  const QString &modelsText, bool makeDefault,
                                  const QString &api = QStringLiteral("openai-completions"));
    Q_INVOKABLE void removeProvider(const QString &id);
    Q_INVOKABLE void setDefault(const QString &provider, const QString &model);
    Q_INVOKABLE void clearError();

    static QString defaultOverlayPath();

signals:
    void changed();
    void errorChanged();

private:
    bool run(const QJsonObject &request, QJsonObject &result);
    void setError(const QString &error);
    QString harnessProfile() const;
    QString pythonHelper() const;

    QString m_overlayPath;
    QString m_envPath;
    QVariantList m_providers;
    QString m_defaultModel;
    QString m_error;
    bool m_deepseekConfigured = false;
};
