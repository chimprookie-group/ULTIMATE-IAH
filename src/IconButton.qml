import QtQuick
import QtQuick.Controls

Item {
    id: root
    property string icon: ""
    property string hint: ""
    property bool active: false
    signal clicked()

    implicitWidth: 32
    implicitHeight: 32

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
        scale: mouse.containsMouse ? 1.08 : 1.0
        Behavior on color { ColorAnimation { duration: design.durationFast } }
        Behavior on scale { NumberAnimation { duration: design.durationFast; easing.type: Easing.OutCubic } }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ToolTip.visible: mouse.containsMouse && root.hint !== ""
    ToolTip.text: root.hint
    ToolTip.delay: 450
}
