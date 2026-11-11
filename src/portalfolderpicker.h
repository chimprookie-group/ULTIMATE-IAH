#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

// Thin wrapper over the XDG desktop portal's file chooser. Qt's own dialogs do
// not follow the desktop's look and, on Wayland, the compositor cannot attach
// them to the window; the portal can.
class PortalFolderPicker : public QObject {
    Q_OBJECT

public:
    explicit PortalFolderPicker(QObject *parent = nullptr);

    Q_INVOKABLE void open(const QString &title = {});

signals:
    void picked(const QString &path);
    void cancelled(const QString &reason);

private slots:
    void handleResponse(uint response, const QVariantMap &results);

private:
    void disconnectResponse();

    QString m_requestPath;
    QTimer m_timeout;
    int m_counter = 0;
};
