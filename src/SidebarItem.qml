import QtQuick
import QtQuick.Controls
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

    implicitWidth: design.sidebarWidth
    implicitHeight: 38

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: design.space2
        anchors.rightMargin: design.space2
        radius: design.radiusSm
        color: root.active ? theme.raised : (mouse.containsMouse ? theme.chip : "transparent")
        Behavior on color { ColorAnimation { duration: design.durationFast } }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: design.space2
        anchors.verticalCenter: parent.verticalCenter
        width: 3
        height: root.active ? 18 : 0
        radius: 2
        color: theme.accent
        Behavior on height { NumberAnimation { duration: design.durationFast; easing.type: Easing.OutCubic } }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: root.compact ? 0 : design.space4
        anchors.rightMargin: root.compact ? 0 : design.space3
        spacing: design.space3

        ThemedIcon {
            Layout.alignment: root.compact ? Qt.AlignHCenter : Qt.AlignVCenter
            Layout.fillWidth: root.compact
            source: root.icon
            width: design.iconMd
            height: design.iconMd
            color: root.active ? theme.accent : (mouse.containsMouse ? theme.text : theme.mutedText)
            Behavior on color { ColorAnimation { duration: design.durationFast } }
        }

        Text {
            Layout.fillWidth: true
            visible: !root.compact
            text: root.label
            color: root.active ? theme.brightText : theme.mutedText
            font.pixelSize: design.textLabel
            font.weight: root.active ? Font.DemiBold : Font.Normal
            elide: Text.ElideRight
            Behavior on color { ColorAnimation { duration: design.durationFast } }
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
