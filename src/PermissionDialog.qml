import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts


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
        color: theme.surface
        border.width: 1
        border.color: theme.borderStrong

        ColumnLayout {
            id: card
            x: 20
            y: 20
            width: parent.width - 40
            spacing: 10

            RowLayout {
                spacing: 9

                ThemedIcon {
                    source: "icons/alert.svg"
                    width: 16
                    height: 16
                    color: theme.yellow
                }
                Text {
        renderType: Text.NativeRendering
                    Layout.fillWidth: true
                    text: qsTr("OpenCode asks for permission")
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: theme.text
                }
            }

            Text {
        renderType: Text.NativeRendering
                Layout.fillWidth: true
                visible: root.opened && request.title !== undefined && request.title !== ""
                text: root.opened ? request.title : ""
                font.family: backend.monoFont
                font.pixelSize: 11
                color: theme.mutedText
                wrapMode: Text.Wrap
            }

            Rectangle {
                Layout.fillWidth: true
                visible: root.opened && request.input !== undefined && request.input !== null && request.input.command !== undefined
                height: commandText.implicitHeight + 16
                radius: Math.min(7, theme.radius)
                color: theme.background
                border.width: 1
                border.color: theme.border

                Text {
        renderType: Text.NativeRendering
                    id: commandText
                    anchors.fill: parent
                    anchors.margins: 8
                    text: root.opened && request.input !== undefined && request.input !== null ? "$ " + request.input.command : ""
                    font.family: backend.monoFont
                    font.pixelSize: 10
                    color: theme.mutedText
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }

            Text {
        renderType: Text.NativeRendering
                Layout.fillWidth: true
                text: qsTr("Nothing runs until you choose. Escape refuses.")
                color: theme.dimText
                font.pixelSize: 10
            }

            Repeater {
                model: root.opened && request.options !== undefined ? request.options : []

                ActionButton {
                    Layout.fillWidth: true
                    text: modelData.name
                    filled: modelData.kind === undefined || modelData.kind.indexOf("reject") !== 0
                    danger: modelData.kind !== undefined && modelData.kind.indexOf("reject") === 0
                    onClicked: backend.respondPermission(modelData.optionId)
                }
            }
        }
    }
}
