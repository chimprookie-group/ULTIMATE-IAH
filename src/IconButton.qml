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

    property bool tipVisible: false
    Timer {
        id: hoverDelay
        interval: 450
        onTriggered: root.tipVisible = mouse.containsMouse
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
        onEntered: hoverDelay.restart()
        onExited: {
            hoverDelay.stop()
            root.tipVisible = false
        }
        onClicked: root.clicked()
    }

    HoverTip {
        anchorItem: root
        text: root.hint
        visible: root.tipVisible && root.hint !== ""
    }
}
