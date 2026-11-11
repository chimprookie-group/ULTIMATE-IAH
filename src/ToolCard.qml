import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts


Item {
    id: root
    property string title: ""
    property string kind: ""
    property string status: "pending"
    property string output: ""
    property string command: ""
    property var locations: []
    property bool expanded: false
    property int exitCode: -1

    width: parent ? parent.width : 0
    implicitHeight: column.implicitHeight + 20

    function kindIcon(kind) {
        if (kind === "execute")
            return "icons/terminal.svg"
        if (kind === "edit")
            return "icons/pencil.svg"
        if (kind === "read")
            return "icons/file.svg"
        return "icons/spark.svg"
    }

    function kindColor(kind) {
        if (kind === "execute")
            return theme.green
        if (kind === "edit")
            return theme.orange
        if (kind === "read")
            return theme.blue
        return theme.accent
    }

    function statusColor(state) {
        if (state === "completed")
            return theme.green
        if (state === "failed")
            return theme.red
        if (state === "in_progress")
            return theme.cyan
        return theme.dimText
    }

    function statusLabel(state) {
        if (state === "completed")
            return qsTr("done")
        if (state === "failed")
            return qsTr("failed")
        if (state === "in_progress")
            return qsTr("running")
        return qsTr("queued")
    }

    Rectangle {
        anchors.fill: parent
        radius: Math.min(9, theme.radius)
        color: theme.raised
        border.width: 1
        border.color: cardMouse.containsMouse ? theme.borderStrong : theme.border
        Behavior on border.color { ColorAnimation { duration: 100 } }

        Rectangle {
            width: 2
            height: parent.height - 16
            radius: 1
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            color: root.kindColor(root.kind)
            opacity: 0.8
        }
    }

    ColumnLayout {
        id: column
        x: 10
        y: 10
        width: parent.width - 20
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                width: 22
                height: 22
                radius: 5
                color: theme.background
                border.width: 1
                border.color: theme.border
                ThemedIcon {
                    anchors.centerIn: parent
                    source: root.kindIcon(root.kind)
                    width: 12
                    height: 12
                    color: root.kindColor(root.kind)
                }
            }

            Text {
                Layout.fillWidth: true
                text: root.title !== "" ? root.title : qsTr("Tool call")
                font.family: backend.monoFont
                font.pixelSize: 12
                color: theme.text
                elide: Text.ElideMiddle
            }

            Rectangle {
                height: 18
                implicitWidth: pill.implicitWidth + 14
                radius: 9
                color: theme.background
                border.width: 1
                border.color: root.statusColor(root.status)

                Row {
                    id: pill
                    anchors.centerIn: parent
                    spacing: 4
                    Rectangle {
                        width: 4
                        height: 4
                        radius: 2
                        anchors.verticalCenter: parent.verticalCenter
                        color: root.statusColor(root.status)
                    }
                    Text {
                        text: root.statusLabel(root.status)
                        font.pixelSize: 8
                        font.letterSpacing: 0.3
                        color: root.statusColor(root.status)
                    }
                }
            }

            ThemedIcon {
                source: "icons/chevron.svg"
                width: 11
                height: 11
                rotation: root.expanded ? 180 : 0
                color: theme.dimText
                Behavior on rotation { NumberAnimation { duration: 120 } }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.command !== ""
            text: "$ " + root.command
            font.family: backend.monoFont
            font.pixelSize: 10
            color: theme.dimText
            elide: Text.ElideRight
            wrapMode: Text.NoWrap
        }

        Text {
            Layout.fillWidth: true
            visible: root.output !== ""
            text: root.output
            font.family: backend.monoFont
            font.pixelSize: 11
            lineHeight: 1.35
            color: theme.mutedText
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            maximumLineCount: root.expanded ? 1000 : 2
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            visible: root.expanded && root.locations.length > 0
            text: {
                const first = root.locations.length > 0 ? root.locations[0] : null
                return first ? (first.path || "") : ""
            }
            font.family: backend.monoFont
            font.pixelSize: 10
            color: theme.dimText
            elide: Text.ElideMiddle
        }

        Text {
            Layout.fillWidth: true
            visible: root.expanded && root.exitCode >= 0
            text: qsTr("exit code %1").arg(root.exitCode)
            font.family: backend.monoFont
            font.pixelSize: 10
            color: theme.dimText
        }
    }

    MouseArea {
        id: cardMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.expanded = !root.expanded
    }
}
