#pragma once

#include <QObject>
#include <QString>

// Window-level hover state. A single non-interactive overlay renders the
// tooltip, so it never grabs the pointer and can never get stuck on the last
// hovered icon. Coordinates are scene coordinates of the anchor item.
class HoverService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString text READ text NOTIFY changed)
    Q_PROPERTY(bool visible READ visible NOTIFY changed)
    Q_PROPERTY(qreal anchorX READ anchorX NOTIFY changed)
    Q_PROPERTY(qreal anchorY READ anchorY NOTIFY changed)
    Q_PROPERTY(qreal anchorWidth READ anchorWidth NOTIFY changed)
    Q_PROPERTY(qreal anchorHeight READ anchorHeight NOTIFY changed)

public:
    explicit HoverService(QObject *parent = nullptr);

    QString text() const { return m_text; }
    bool visible() const { return m_visible; }
    qreal anchorX() const { return m_anchorX; }
    qreal anchorY() const { return m_anchorY; }
    qreal anchorWidth() const { return m_anchorWidth; }
    qreal anchorHeight() const { return m_anchorHeight; }

    Q_INVOKABLE void show(const QString &text, qreal x, qreal y, qreal width, qreal height);
    Q_INVOKABLE void hide();

signals:
    void changed();

private:
    QString m_text;
    bool m_visible = false;
    qreal m_anchorX = 0;
    qreal m_anchorY = 0;
    qreal m_anchorWidth = 0;
    qreal m_anchorHeight = 0;
};
