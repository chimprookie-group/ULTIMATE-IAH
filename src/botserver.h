#pragma once

#include <QObject>
#include <QTcpServer>

class BotManager;

// Loopback HTTP API and dashboard for the bot system. Every API request needs
// the service token; the dashboard asks for it once and keeps it in memory.
// Only one request per connection, bounded body, no directory serving.
class BotServer : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY stateChanged)
    Q_PROPERTY(bool listening READ listening NOTIFY stateChanged)
    Q_PROPERTY(bool healthy READ healthy NOTIFY stateChanged)
    Q_PROPERTY(QString lastActivityAt READ lastActivityAt NOTIFY stateChanged)
    Q_PROPERTY(QString url READ url NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(int port READ port NOTIFY stateChanged)

public:
    explicit BotServer(BotManager *manager, QObject *parent = nullptr);

    bool running() const { return m_server.isListening(); }
    bool listening() const { return m_server.isListening(); }
    bool healthy() const;
    QString lastActivityAt() const;
    QString url() const { return m_url; }
    QString error() const { return m_error; }
    int port() const { return m_server.serverPort(); }

    bool start(quint16 preferredPort = 8790);
    void stop();
    void setToken(const QString &token) { m_token = token; }
    QString token() const { return m_token; }
    void clearError();

signals:
    void stateChanged();
    void errorChanged();

private:
    void handleConnection(QTcpSocket *socket);
    void respond(QTcpSocket *socket, int status, const QByteArray &type,
                 const QByteArray &body);
    bool authorized(const QHash<QByteArray, QByteArray> &headers) const;
    void setError(const QString &error);

    BotManager *m_manager = nullptr;
    QTcpServer m_server;
    QString m_url;
    QString m_token;
    QString m_error;
    qint64 m_lastServedAt = 0;
};
