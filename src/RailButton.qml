import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material


Item {
    id: root
    property string icon: ""
    property string label: ""
    property string hint: ""
    property bool active: false
    signal clicked()

    implicitWidth: 46
    implicitHeight: 50

    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: 2
        height: root.active ? 22 : 0
        radius: 1
        color: theme.accent
        opacity: root.active ? 1 : 0
        Behavior on height { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 3
        radius: Math.min(8, theme.radius)
        color: root.active ? theme.accentSoft : (mouse.containsMouse ? theme.chip : "transparent")
        Behavior on color { ColorAnimation { duration: 100 } }
    }

    Column {
        anchors.centerIn: parent
        spacing: 3

        ThemedIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            source: root.icon
            width: 19
            height: 19
            color: root.active ? theme.accent : (mouse.containsMouse ? theme.text : theme.dimText)
        }
        Text {
        renderType: Text.NativeRendering
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            font.pixelSize: 8
            font.letterSpacing: 0.3
            color: root.active ? theme.brightText : theme.mutedText
        }
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
