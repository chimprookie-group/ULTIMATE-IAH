#include "botserver.h"

#include "botmanager.h"

#include <QFile>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QRegularExpression>
#include <QSharedPointer>

namespace {
QByteArray json(const QJsonObject &object) {
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QByteArray json(const QJsonArray &array) {
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

QByteArray statusText(int status) {
    switch (status) {
    case 200: return "OK";
    case 201: return "Created";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Payload Too Large";
    case 500: return "Internal Server Error";
    default: return "Error";
    }
}
} // namespace

BotServer::BotServer(BotManager *manager, QObject *parent)
    : QObject(parent), m_manager(manager) {
    connect(&m_server, &QTcpServer::newConnection, this, [this] {
        while (m_server.hasPendingConnections())
            handleConnection(m_server.nextPendingConnection());
    });
}

bool BotServer::start(quint16 preferredPort) {
    if (m_server.isListening())
        return true;
    if (!m_server.listen(QHostAddress::LocalHost, preferredPort)) {
        if (preferredPort == 0 || !m_server.listen(QHostAddress::LocalHost, 0)) {
            setError(tr("Impossible de démarrer l'API des bots : %1").arg(m_server.errorString()));
            return false;
        }
    }
    m_url = QStringLiteral("http://127.0.0.1:%1").arg(m_server.serverPort());
    setError({});
    emit stateChanged();
    return true;
}

void BotServer::stop() {
    if (!m_server.isListening())
        return;
    m_server.close();
    m_url.clear();
    emit stateChanged();
}

void BotServer::clearError() {
    setError({});
}

void BotServer::handleConnection(QTcpSocket *socket) {
    if (!socket)
        return;
    const QPointer<QTcpSocket> guard(socket);
    const auto buffer = QSharedPointer<QByteArray>::create();
    const auto handled = QSharedPointer<bool>::create(false);

    connect(socket, &QTcpSocket::readyRead, this, [this, guard, buffer, handled] {
        if (!guard || *handled)
            return;
        buffer->append(guard->readAll());
        if (buffer->size() > 1024 * 1024) {
            *handled = true;
            respond(guard, 413, "application/json", R"({"ok":false,"error":"payload too large"})");
            return;
        }
        const int headerEnd = buffer->indexOf("\r\n\r\n");
        if (headerEnd < 0)
            return;
        const QByteArray headerBlock = buffer->left(headerEnd);
        const QList<QByteArray> lines = headerBlock.split('\n');
        if (lines.isEmpty()) {
            *handled = true;
            respond(guard, 400, "application/json", R"({"ok":false,"error":"invalid request"})");
            return;
        }
        const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
        if (requestLine.size() < 2) {
            *handled = true;
            respond(guard, 400, "application/json", R"({"ok":false,"error":"invalid request line"})");
            return;
        }
        const QByteArray method = requestLine.at(0).toUpper();
        const QByteArray path = requestLine.at(1).split('?').first();
        QHash<QByteArray, QByteArray> headers;
        for (int i = 1; i < lines.size(); ++i) {
            const QByteArray line = lines.at(i).trimmed();
            const int colon = line.indexOf(':');
            if (colon <= 0)
                continue;
            headers.insert(line.left(colon).trimmed().toLower(), line.mid(colon + 1).trimmed());
        }
        int contentLength = 0;
        if (headers.contains("content-length"))
            contentLength = qBound(0, headers.value("content-length").toInt(), 256 * 1024);
        if (buffer->size() < headerEnd + 4 + contentLength)
            return;
        *handled = true;
        const QByteArray body = buffer->mid(headerEnd + 4, contentLength);

        if (path == "/health" && method == "GET") {
            QJsonObject result{{"ok", true}, {"service", "ultimate-iah-bots"}, {"version", 1}};
            respond(guard, 200, "application/json", json(result));
            return;
        }
        if (path == "/" && method == "GET") {
            QFile dashboard(QStringLiteral(":/bot/dashboard.html"));
            if (!dashboard.open(QIODevice::ReadOnly)) {
                respond(guard, 500, "text/plain", "dashboard unavailable");
                return;
            }
            respond(guard, 200, "text/html; charset=utf-8", dashboard.readAll());
            return;
        }
        if (!authorized(headers)) {
            respond(guard, 401, "application/json", R"({"ok":false,"error":"missing or invalid token"})");
            return;
        }

        if (path == "/api/status" && method == "GET") {
            respond(guard, 200, "application/json", json(m_manager->statusJson()));
            return;
        }
        if (path == "/api/bots" && method == "GET") {
            respond(guard, 200, "application/json", json(m_manager->botsJson()));
            return;
        }
        if (path == "/api/bots" && method == "POST") {
            const QJsonObject input = QJsonDocument::fromJson(body).object();
            const QString name = input.value("name").toString().trimmed();
            if (name.isEmpty()) {
                respond(guard, 400, "application/json", R"({"ok":false,"error":"name is required"})");
                return;
            }
            const QJsonObject created = m_manager->createBotFromApi(
                name, input.value("description").toString(),
                input.value("instructions").toString(),
                input.value("permission").toString(QStringLiteral("read-only")));
            respond(guard, created.contains("error") ? 400 : 201, "application/json", json(created));
            return;
        }

        static const QRegularExpression botPath(QStringLiteral("^/api/bots/(\\d+)$"));
        static const QRegularExpression messagePath(QStringLiteral("^/api/bots/(\\d+)/messages$"));
        static const QRegularExpression runPath(QStringLiteral("^/api/runs/(\\d+)$"));

        const QString pathString = QString::fromUtf8(path);
        auto match = botPath.match(pathString);
        if (match.hasMatch() && method == "GET") {
            const QJsonObject bot = m_manager->botJson(match.captured(1).toInt());
            respond(guard, bot.isEmpty() ? 404 : 200, "application/json",
                    bot.isEmpty() ? QByteArray(R"({"ok":false,"error":"bot not found"})") : json(bot));
            return;
        }
        match = messagePath.match(pathString);
        if (match.hasMatch() && method == "GET") {
            respond(guard, 200, "application/json",
                    json(m_manager->messagesJson(match.captured(1).toInt())));
            return;
        }
        if (match.hasMatch() && method == "POST") {
            const QJsonObject input = QJsonDocument::fromJson(body).object();
            const QString text = input.value("text").toString().trimmed();
            if (text.isEmpty()) {
                respond(guard, 400, "application/json", R"({"ok":false,"error":"text is required"})");
                return;
            }
            const QJsonObject result = m_manager->sendBotMessage(
                match.captured(1).toInt(), text, QStringLiteral("api"));
            respond(guard, result.contains("error") ? 400 : 202, "application/json", json(result));
            return;
        }
        match = runPath.match(pathString);
        if (match.hasMatch() && method == "GET") {
            const QJsonObject run = m_manager->runJson(match.captured(1).toInt());
            respond(guard, run.isEmpty() ? 404 : 200, "application/json",
                    run.isEmpty() ? QByteArray(R"({"ok":false,"error":"run not found"})") : json(run));
            return;
        }
        respond(guard, 404, "application/json", R"({"ok":false,"error":"not found"})");
    });
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    connect(socket, &QTcpSocket::errorOccurred, socket, &QTcpSocket::deleteLater);
}

void BotServer::respond(QTcpSocket *socket, int status, const QByteArray &type,
                        const QByteArray &body) {
    if (!socket)
        return;
    QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + " " + statusText(status) + "\r\n";
    response += "Content-Type: " + type + "\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Cache-Control: no-store\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

bool BotServer::authorized(const QHash<QByteArray, QByteArray> &headers) const {
    if (m_token.isEmpty())
        return false;
    QByteArray provided = headers.value("x-bot-token");
    const QByteArray authorization = headers.value("authorization");
    if (authorization.startsWith("Bearer "))
        provided = authorization.mid(7).trimmed();
    return !provided.isEmpty() && provided == m_token.toUtf8();
}

void BotServer::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}
