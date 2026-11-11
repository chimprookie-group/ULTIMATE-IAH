#include "iconprovider.h"

#include <QColor>
#include <QPainter>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <QUrl>

IconProvider::IconProvider()
    : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage IconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    const QString file = id.section('?', 0, 0);
    static const QRegularExpression valid(QStringLiteral("^[a-z0-9-]+\\.svg$"));
    if (!valid.match(file).hasMatch())
        return {};

    QColor color(QStringLiteral("#ffffff"));
    const QString query = id.section('?', 1);
    for (const QString &pair : query.split('&')) {
        const QString key = pair.section('=', 0, 0);
        if (key != "color")
            continue;
        const QColor requested(QUrl::fromPercentEncoding(pair.section('=', 1).toUtf8()));
        if (requested.isValid())
            color = requested;
    }

    int logical = requestedSize.isValid() && requestedSize.width() > 0 ? requestedSize.width() : 48;
    logical = qBound(8, logical, 256);
    const int pixels = logical * 2;

    QSvgRenderer renderer(QStringLiteral(":/icons/") + file);
    if (!renderer.isValid())
        return {};

    QImage image(pixels, pixels, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    renderer.render(&painter, QRectF(0, 0, pixels, pixels));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(image.rect(), color);
    painter.end();
    image.setDevicePixelRatio(2.0);

    if (size)
        *size = QSize(logical, logical);
    return image;
}
