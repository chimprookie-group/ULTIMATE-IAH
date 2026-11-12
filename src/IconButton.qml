import QtQuick
import QtQuick.Controls

Item {
    id: root
    property string icon: ""
    property string hint: ""
    property bool active: false
    signal clicked()
    Accessible.name: hint

    implicitWidth: 28
    implicitHeight: 28

    ThemedIcon {
        anchors.centerIn: parent
        source: root.icon
        width: design.iconMd
        height: design.iconMd
        color: root.active ? theme.brightText : (mouse.containsMouse ? theme.accent : theme.mutedText)
        opacity: root.active || mouse.containsMouse ? 1 : 0.82
        Behavior on color { ColorAnimation { duration: design.durationFast } }
        Behavior on opacity { NumberAnimation { duration: design.durationFast; easing.type: Easing.OutCubic } }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        width: 10
        height: 2
        radius: 1
        color: theme.accent
        opacity: mouse.containsMouse ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: design.durationFast; easing.type: Easing.OutCubic } }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: hoverTimer.restart()
        onExited: {
            hoverTimer.stop()
            hoverService.hide()
        }
        onClicked: {
            hoverTimer.stop()
            hoverService.hide()
            root.clicked()
        }
    }

    Timer {
        id: hoverTimer
        interval: 450
        onTriggered: {
            if (root.hint === "")
                return
            const origin = root.mapToItem(null, 0, 0)
            hoverService.show(root.hint, origin.x, origin.y, root.width, root.height)
        }
    }
}
