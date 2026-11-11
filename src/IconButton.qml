import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

import "Palette.js" as Pal

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
        color: root.active ? Pal.raisedHover : (mouse.containsMouse ? Pal.raised : "transparent")
        border.width: root.active ? 1 : 0
        border.color: Pal.borderStrong
    }

    Image {
        anchors.centerIn: parent
        source: root.icon
        sourceSize.width: 15
        sourceSize.height: 15
        opacity: root.active ? 1 : (mouse.containsMouse ? 0.8 : 0.55)
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
