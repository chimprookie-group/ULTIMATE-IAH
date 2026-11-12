#include "hoverservice.h"

HoverService::HoverService(QObject *parent)
    : QObject(parent) {}

void HoverService::show(const QString &text, qreal x, qreal y, qreal width, qreal height) {
    const QString value = text.trimmed();
    if (value.isEmpty()) {
        hide();
        return;
    }
    if (m_visible && value == m_text && qFuzzyCompare(m_anchorX + 1, x + 1)
        && qFuzzyCompare(m_anchorY + 1, y + 1)) {
        return;
    }
    m_text = value;
    m_anchorX = x;
    m_anchorY = y;
    m_anchorWidth = width;
    m_anchorHeight = height;
    m_visible = true;
    emit changed();
}

void HoverService::hide() {
    if (!m_visible)
        return;
    m_visible = false;
    m_text.clear();
    emit changed();
}
