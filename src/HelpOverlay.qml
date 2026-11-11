import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import "Palette.js" as Pal

Item {
    id: root

    signal close()

    property var rows: [
        { keys: "Ctrl+1", what: qsTr("DeepSeek Harness") },
        { keys: "Ctrl+2", what: qsTr("OpenCode chat") },
        { keys: "Ctrl+3", what: qsTr("Split view") },
        { keys: "Ctrl+4", what: qsTr("Settings") },
        { keys: "Ctrl+Enter", what: qsTr("Send the message") },
        { keys: "Shift+Enter", what: qsTr("New line in the composer") },
        { keys: "Ctrl+N", what: qsTr("New OpenCode session") },
        { keys: "Ctrl+M", what: qsTr("Choose the model") },
        { keys: "Ctrl+L", what: qsTr("Focus the composer") },
        { keys: "Ctrl+R", what: qsTr("Restart the harness") },
        { keys: "?", what: qsTr("This help") },
        { keys: "Q", what: qsTr("Quit") }
    ]

    z: 150
    visible: false

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.62
    }
    MouseArea {
        anchors.fill: parent
        onClicked: root.close()
    }

    Rectangle {
        width: Math.min(600, parent.width - 64)
        height: card.implicitHeight + 44
        anchors.centerIn: parent
        radius: Math.min(14, theme.radius)
        color: Pal.panel
        border.width: 1
        border.color: Pal.borderStrong

        ColumnLayout {
            id: card
            x: 22
            y: 22
            width: parent.width - 44
            spacing: 10

            RowLayout {
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Keyboard")
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    color: Pal.text
                }
                IconButton {
                    icon: "icons/close.svg"
                    hint: qsTr("Close (Esc)")
                    onClicked: root.close()
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Everything works from the keyboard. The harness keeps its page when you switch views; OpenCode keeps its transcript.")
                wrapMode: Text.Wrap
                color: Pal.muted
                font.pixelSize: 11
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Pal.border
            }

            Repeater {
                model: root.rows
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    Rectangle {
                        Layout.preferredWidth: 104
                        height: 21
                        radius: Math.min(5, theme.radius)
                        color: Pal.raised
                        border.width: 1
                        border.color: Pal.border
                        Text {
                            anchors.centerIn: parent
                            text: modelData.keys
                            font.family: backend.monoFont
                            font.pixelSize: 10
                            color: Pal.text
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: modelData.what
                        color: Pal.muted
                        font.pixelSize: 12
                    }
                }
            }
        }
    }
}
