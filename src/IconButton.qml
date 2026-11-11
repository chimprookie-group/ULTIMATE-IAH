import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material


Item {
    id: root
    property string icon: ""
    property string hint: ""
    property bool active: false
    signal clicked()

    implicitWidth: 30
    implicitHeight: 30

    Rectangle {
        anchors.fill: parent
        radius: Math.min(7, theme.radius)
        color: root.active ? theme.chip : (mouse.containsMouse ? theme.raised : "transparent")
        border.width: root.active ? 1 : 0
        border.color: theme.borderStrong
    }

    ThemedIcon {
        anchors.centerIn: parent
        source: root.icon
        width: 15
        height: 15
        color: root.active ? theme.brightText : (mouse.containsMouse ? theme.text : theme.mutedText)
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
