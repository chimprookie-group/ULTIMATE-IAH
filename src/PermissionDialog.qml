import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import "Palette.js" as Pal

Item {
    id: root

    property var request: ({})

    readonly property bool opened: request !== undefined && request !== null && request.requestId !== undefined
    visible: opened
    z: 180

    function dismiss() {
        backend.respondPermission("")
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.64
    }
    MouseArea {
        anchors.fill: parent
        onClicked: root.dismiss()
    }

    Rectangle {
        width: Math.min(580, parent.width - 64)
        height: card.implicitHeight + 40
        anchors.centerIn: parent
        radius: Math.min(12, theme.radius)
        color: Pal.panel
        border.width: 1
        border.color: Pal.borderStrong

        ColumnLayout {
            id: card
            x: 20
            y: 20
            width: parent.width - 40
            spacing: 10

            RowLayout {
                spacing: 9

                Image {
                    source: "icons/alert.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("OpenCode asks for permission")
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: Pal.text
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.opened && request.title !== undefined && request.title !== ""
                text: root.opened ? request.title : ""
                font.family: backend.monoFont
                font.pixelSize: 11
                color: Pal.muted
                wrapMode: Text.Wrap
            }

            Rectangle {
                Layout.fillWidth: true
                visible: root.opened && request.input !== undefined && request.input !== null && request.input.command !== undefined
                height: commandText.implicitHeight + 16
                radius: Math.min(7, theme.radius)
                color: Pal.bg
                border.width: 1
                border.color: Pal.border

                Text {
                    id: commandText
                    anchors.fill: parent
                    anchors.margins: 8
                    text: root.opened && request.input !== undefined && request.input !== null ? "$ " + request.input.command : ""
                    font.family: backend.monoFont
                    font.pixelSize: 10
                    color: Pal.muted
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Nothing runs until you choose. Escape refuses.")
                color: Pal.dim
                font.pixelSize: 10
            }

            Repeater {
                model: root.opened && request.options !== undefined ? request.options : []

                Button {
                    Layout.fillWidth: true
                    text: modelData.name
                    highlighted: modelData.kind !== undefined && modelData.kind.indexOf("reject") !== 0
                    onClicked: backend.respondPermission(modelData.optionId)
                }
            }
        }
    }
}
