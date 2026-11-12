import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    signal closeRequested()
    signal openRequested()

    color: theme.surface
    border.width: 0
    Rectangle {
        anchors.left: parent.left
        width: 1
        height: parent.height
        color: theme.borderStrong
    }

    UltimateAgentPane {
        anchors.fill: parent
        compact: true
        closeButton: true
        expandButton: true
        onCloseRequested: root.closeRequested()
        onExpandRequested: root.openRequested()
    }
}
