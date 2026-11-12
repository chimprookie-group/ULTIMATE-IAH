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

    Rectangle {
        anchors.fill: parent
        radius: design.radiusSm
        color: root.active ? theme.raised : (mouse.containsMouse ? theme.chip : "transparent")
        border.width: root.active ? 1 : 0
        border.color: theme.border
        Behavior on color { ColorAnimation { duration: design.durationFast } }
    }

    ThemedIcon {
        anchors.centerIn: parent
        source: root.icon
        width: design.iconMd
        height: design.iconMd
        color: root.active ? theme.brightText : (mouse.containsMouse ? theme.text : theme.mutedText)
        Behavior on color { ColorAnimation { duration: design.durationFast } }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
