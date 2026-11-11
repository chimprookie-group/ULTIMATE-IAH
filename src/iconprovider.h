#pragma once

#include <QQuickImageProvider>

// Renders an icon from the compiled SVG set with a flat theme color. Doing the
// tint in Qt avoids QML effect quirks (white leftovers, black boxes) and keeps
// alpha edges clean on light and dark palettes alike.
class IconProvider : public QQuickImageProvider {
public:
    IconProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};
