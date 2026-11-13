import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property string title: ""
    property string summary: ""
    property bool collapsible: true
    property bool expanded: true
    default property alias content: contentColumn.data

    Layout.fillWidth: true
    implicitHeight: layout.implicitHeight + design.space7
    color: theme.raised
    radius: design.radiusLg
    border.width: 1
    border.color: theme.border

    ColumnLayout {
        id: layout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: design.space4
        spacing: design.space3

        RowLayout {
            Layout.fillWidth: true
            spacing: design.space2

            Rectangle { width: 3; height: 12; radius: 2; color: theme.accent }
            Text {
                text: root.title
                font.pixelSize: design.textLabel
                font.weight: Font.DemiBold
                color: theme.text
            }
            Text {
                Layout.fillWidth: true
                visible: root.summary !== ""
                text: root.summary
                color: theme.dimText
                font.family: backend.monoFont
                font.pixelSize: design.textMicro
                elide: Text.ElideRight
            }
            Item { Layout.fillWidth: root.summary === "" }
            ThemedIcon {
                visible: root.collapsible
                source: "icons/chevron.svg"
                width: 11
                height: 11
                color: headerMouse.containsMouse ? theme.accent : theme.dimText
                rotation: root.expanded ? 90 : 0
                Behavior on rotation { NumberAnimation { duration: design.durationFast } }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.bottomMargin: design.space1
            height: 1
            color: theme.border
        }

        ColumnLayout {
            id: contentColumn
            Layout.fillWidth: true
            visible: !root.collapsible || root.expanded
            spacing: design.space3
        }
    }

    MouseArea {
        id: headerMouse
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: design.space4 + 24
        enabled: root.collapsible
        hoverEnabled: true
        cursorShape: root.collapsible ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (root.collapsible) root.expanded = !root.expanded
    }
}
