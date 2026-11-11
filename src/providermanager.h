#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QVariantList>

class QNetworkReply;
class QTimer;

// Manages OpenAI-compatible providers in the OpenCode configuration, plus the
// list of accounts stored by `opencode auth`. Keys are written locally with
// mode 0600 and only a masked suffix is ever sent to QML.
class ProviderManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList providers READ providers NOTIFY changed)
    Q_PROPERTY(QVariantList accounts READ accounts NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString testMessage READ testMessage NOTIFY testMessageChanged)
    Q_PROPERTY(bool testing READ testing NOTIFY testingChanged)
    Q_PROPERTY(QString configPath READ configPath CONSTANT)

public:
    explicit ProviderManager(const QString &configPath = {}, QObject *parent = nullptr);

    QVariantList providers() const { return m_providers; }
    QVariantList accounts() const { return m_accounts; }
    QString error() const { return m_error; }
    QString testMessage() const { return m_testMessage; }
    bool testing() const { return m_testing; }
    QString configPath() const { return m_configPath; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshAccounts();
    Q_INVOKABLE void saveProvider(const QString &id, const QString &name,
                                  const QString &baseUrl, const QString &apiKey,
                                  const QString &modelsText);
    Q_INVOKABLE void removeProvider(const QString &id);
    Q_INVOKABLE void testProvider(const QString &id);
    Q_INVOKABLE void restartService();
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void clearTest();

    static QString defaultConfigPath();

signals:
    void changed();
    void errorChanged();
    void testMessageChanged();
    void testingChanged();
    void applied();

private:
    bool readConfig(QJsonObject &root) const;
    bool writeConfig(const QJsonObject &root);
    void setError(const QString &error);
    void setTestMessage(const QString &message);
    void setTesting(bool testing);
    void finishTest(QNetworkReply *reply);
    QVariantMap findProvider(const QString &id) const;

    QString m_configPath;
    QVariantList m_providers;
    QVariantList m_accounts;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
    QTimer *m_timeout = nullptr;
    QString m_error;
    QString m_testMessage;
    bool m_testing = false;
};
