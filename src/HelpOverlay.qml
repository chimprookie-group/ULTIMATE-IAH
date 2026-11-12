import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts


Item {
    id: root

    signal close()

    property var rows: [
        { keys: "Ctrl+0", what: qsTr("Crasy Harness cockpit") },
        { keys: "Ctrl+K", what: qsTr("Command palette") },
        { keys: "Ctrl+1", what: qsTr("DeepSeek Harness") },
        { keys: "Ctrl+2", what: qsTr("Terminal agent") },
        { keys: "Ctrl+6", what: qsTr("Project") },
        { keys: "Ctrl+7", what: qsTr("Files and diffs") },
        { keys: "Ctrl+9", what: qsTr("9Router") },
        { keys: "Ctrl+Shift+M", what: qsTr("MausBot and its MCP bridge") },
        { keys: "Ctrl+4", what: qsTr("Settings") },
        { keys: "Ctrl+Enter", what: qsTr("Run a harness task in the terminal") },
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
        color: theme.surface
        border.width: 1
        border.color: theme.borderStrong

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
                    color: theme.text
                }
                IconButton {
                    icon: "icons/close.svg"
                    hint: qsTr("Close (Esc)")
                    onClicked: root.close()
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Everything works from the keyboard. The harness keeps its page; the terminal stays your real shell and runs harness tasks with dsh headless.")
                wrapMode: Text.Wrap
                color: theme.mutedText
                font.pixelSize: 11
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.border
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
                        color: theme.raised
                        border.width: 1
                        border.color: theme.border
                        Text {
                            anchors.centerIn: parent
                            text: modelData.keys
                            font.family: backend.monoFont
                            font.pixelSize: 10
                            color: theme.text
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: modelData.what
                        color: theme.mutedText
                        font.pixelSize: 12
                    }
                }
            }
        }
    }
}
