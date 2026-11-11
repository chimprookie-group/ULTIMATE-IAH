import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

import "Palette.js" as Pal

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
        color: root.active ? Pal.raised : (mouse.containsMouse ? "#0F191E" : "transparent")
        Behavior on color { ColorAnimation { duration: 100 } }
    }

    Column {
        anchors.centerIn: parent
        spacing: 3

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: root.icon
            sourceSize.width: 19
            sourceSize.height: 19
            opacity: root.active ? 1 : (mouse.containsMouse ? 0.85 : 0.58)
            Behavior on opacity { NumberAnimation { duration: 100 } }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.label
            font.pixelSize: 8
            font.letterSpacing: 0.3
            color: root.active ? Pal.text : Pal.dim
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
