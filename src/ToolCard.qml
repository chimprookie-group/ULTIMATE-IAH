import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import "Palette.js" as Pal

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

    function statusColor(state) {
        if (state === "completed")
            return Pal.green
        if (state === "failed")
            return Pal.red
        if (state === "in_progress")
            return Pal.teal
        return Pal.dim
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
        color: Pal.raised
        border.width: 1
        border.color: cardMouse.containsMouse ? Pal.borderStrong : Pal.border
        Behavior on border.color { ColorAnimation { duration: 100 } }
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
                color: Pal.bg
                border.width: 1
                border.color: Pal.border
                Image {
                    anchors.centerIn: parent
                    source: root.kindIcon(root.kind)
                    sourceSize.width: 12
                    sourceSize.height: 12
                    opacity: 0.75
                }
            }

            Text {
                Layout.fillWidth: true
                text: root.title !== "" ? root.title : qsTr("Tool call")
                font.family: backend.monoFont
                font.pixelSize: 12
                color: Pal.text
                elide: Text.ElideMiddle
            }

            Rectangle {
                height: 18
                implicitWidth: pill.implicitWidth + 14
                radius: 9
                color: Pal.bg
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

            Image {
                source: "icons/chevron.svg"
                sourceSize.width: 11
                sourceSize.height: 11
                rotation: root.expanded ? 180 : 0
                opacity: 0.45
                Behavior on rotation { NumberAnimation { duration: 120 } }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.command !== ""
            text: "$ " + root.command
            font.family: backend.monoFont
            font.pixelSize: 10
            color: Pal.dim
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
            color: Pal.muted
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
            color: Pal.dim
            elide: Text.ElideMiddle
        }

        Text {
            Layout.fillWidth: true
            visible: root.expanded && root.exitCode >= 0
            text: qsTr("exit code %1").arg(root.exitCode)
            font.family: backend.monoFont
            font.pixelSize: 10
            color: Pal.dim
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
