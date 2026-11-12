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
        color: root.active ? theme.brightText : (mouse.containsMouse ? theme.text : theme.mutedText)
        opacity: root.active || mouse.containsMouse ? 1 : 0.82
        Behavior on color { ColorAnimation { duration: design.durationFast } }
        Behavior on opacity { NumberAnimation { duration: design.durationFast; easing.type: Easing.OutCubic } }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
