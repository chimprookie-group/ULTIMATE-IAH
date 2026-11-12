import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    property string icon: ""
    property string label: ""
    property string hint: ""
    property bool active: false
    property bool compact: false
    property color statusColor: "transparent"
    signal clicked()

    implicitWidth: 190
    implicitHeight: compact ? 38 : 36

    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: 2
        height: root.active ? 18 : 0
        radius: 1
        color: theme.accent
        opacity: root.active ? 1 : 0
        Behavior on height { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
    }

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        radius: Math.min(8, theme.radius)
        color: root.active ? theme.accentSoft : (mouse.containsMouse ? theme.chip : "transparent")
        Behavior on color { ColorAnimation { duration: 100 } }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.compact ? 0 : 14
        anchors.rightMargin: root.compact ? 0 : 10
        spacing: 9

        ThemedIcon {
            Layout.alignment: root.compact ? Qt.AlignHCenter : Qt.AlignVCenter
            Layout.fillWidth: root.compact
            source: root.icon
            width: 16
            height: 16
            color: root.active ? theme.accent : (mouse.containsMouse ? theme.text : theme.dimText)
        }

        Text {
            Layout.fillWidth: true
            visible: !root.compact
            text: root.label
            color: root.active ? theme.brightText : theme.mutedText
            font.pixelSize: 11
            font.weight: root.active ? Font.DemiBold : Font.Normal
            elide: Text.ElideRight
        }

        Rectangle {
            visible: root.statusColor !== "transparent" && !root.compact
            width: 5
            height: 5
            radius: 3
            color: root.statusColor
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
    ToolTip.delay: 400
}
