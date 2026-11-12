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
    Accessible.name: label !== "" ? label : hint

    implicitWidth: design.sidebarWidth
    implicitHeight: 38

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

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        width: 12
        height: 2
        radius: 1
        color: theme.accent
        opacity: mouse.containsMouse ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: design.durationFast; easing.type: Easing.OutCubic } }
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
            color: root.active ? theme.accent : (mouse.containsMouse ? (theme.dark ? theme.brightText : theme.accent) : theme.mutedText)
            opacity: root.active || mouse.containsMouse ? 1 : 0.82
            Behavior on color { ColorAnimation { duration: design.durationFast } }
            Behavior on opacity { NumberAnimation { duration: design.durationFast; easing.type: Easing.OutCubic } }
        }

        Text {
            Layout.fillWidth: true
            visible: !root.compact
            text: root.label
            color: root.active ? theme.brightText : (mouse.containsMouse ? (theme.dark ? theme.text : theme.accent) : theme.mutedText)
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

    property bool tipVisible: false
    Timer {
        id: hoverDelay
        interval: 450
        onTriggered: root.tipVisible = mouse.containsMouse
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
