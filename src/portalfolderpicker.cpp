#include "portalfolderpicker.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QUrl>
#include <QVariantMap>

PortalFolderPicker::PortalFolderPicker(QObject *parent)
    : QObject(parent) {
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(120000);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        disconnectResponse();
        emit cancelled(tr("The folder chooser timed out."));
    });
}

void PortalFolderPicker::open(const QString &title) {
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        emit cancelled(tr("The desktop portal bus is unavailable."));
        return;
    }

    // The token must be unique per request and is part of the object path the
    // portal will call back on.
    const QString token = QStringLiteral("deepcrasy%1_%2")
                              .arg(QCoreApplication::applicationPid())
                              .arg(++m_counter);
    QVariantMap options;
    options.insert("handle_token", token);
    options.insert("directory", true);
    options.insert("multiple", false);
    options.insert("modal", true);
    if (!title.isEmpty())
        options.insert("accept_label", title);

    QDBusMessage message = QDBusMessage::createMethodCall(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.FileChooser",
        "OpenFile");
    // An empty parent window is allowed; the portal simply has no transient
    // parent, which beats guessing a Wayland handle.
    message << QString() << title << options;

    const QDBusMessage reply = bus.call(message, QDBus::Block, 5000);
    if (reply.type() != QDBusMessage::ReplyMessage) {
        emit cancelled(tr("The desktop portal did not open a folder chooser."));
        return;
    }

    const QString path = reply.arguments().value(0).toString();
    if (path.isEmpty()) {
        emit cancelled(tr("The desktop portal returned no request handle."));
        return;
    }

    disconnectResponse();
    m_requestPath = path;
    const bool connected = bus.connect(
        "org.freedesktop.portal.Desktop", path,
        "org.freedesktop.portal.Request", "Response",
        this, SLOT(handleResponse(uint, QVariantMap)));
    if (!connected) {
        m_requestPath.clear();
        emit cancelled(tr("Could not listen for the folder chooser's answer."));
        return;
    }
    m_timeout.start();
}

void PortalFolderPicker::disconnectResponse() {
    if (m_requestPath.isEmpty())
        return;
    QDBusConnection::sessionBus().disconnect(
        "org.freedesktop.portal.Desktop", m_requestPath,
        "org.freedesktop.portal.Request", "Response",
        this, SLOT(handleResponse(uint, QVariantMap)));
    m_requestPath.clear();
}

void PortalFolderPicker::handleResponse(uint response, const QVariantMap &results) {
    m_timeout.stop();
    disconnectResponse();

    if (response != 0) {
        emit cancelled(tr("Folder selection was cancelled."));
        return;
    }
    const QStringList uris = results.value("uris").toStringList();
    if (uris.isEmpty()) {
        emit cancelled(tr("The folder chooser returned no folder."));
        return;
    }
    const QString path = QUrl(uris.first()).toLocalFile();
    if (path.isEmpty()) {
        emit cancelled(tr("The chosen folder is not local."));
        return;
    }
    emit picked(path);
}
