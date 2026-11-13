import QtQuick
import QtQuick.Controls

// Thin, trackless scrollbar used across Ultimate IAH. The thumb stays quiet at
// rest and becomes clearly visible only while hovered or dragged.
ScrollBar {
    id: root

    policy: ScrollBar.AsNeeded
    padding: 0
    implicitWidth: 8
    implicitHeight: 8

    contentItem: Rectangle {
        implicitWidth: 6
        implicitHeight: 6
        radius: Math.min(width, height) / 2
        color: root.pressed ? theme.accent
                            : (root.hovered ? theme.mutedText : theme.dimText)
        opacity: root.pressed || root.hovered ? 0.95 : 0.55
        Behavior on color { ColorAnimation { duration: design.durationFast } }
        Behavior on opacity { NumberAnimation { duration: design.durationFast; easing.type: Easing.OutCubic } }
    }

    background: Item {}
}
