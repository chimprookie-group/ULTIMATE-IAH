import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    signal close()

    property var rows: [
        { keys: "Ctrl+0", what: qsTr("Ultimate IAH cockpit") },
        { keys: "Ctrl+K", what: qsTr("Command palette") },
        { keys: "Ctrl+1", what: qsTr("DeepSeek Harness") },
        { keys: "Ctrl+2", what: qsTr("Terminal agent") },
        { keys: "Ctrl+6", what: qsTr("Ecosystem") },
        { keys: "Ctrl+7", what: qsTr("Files and diffs") },
        { keys: "Ctrl+8", what: qsTr("Harness connections") },
        { keys: "Ctrl+Alt+C", what: qsTr("Serveurs MCP") },
        { keys: "Ctrl+Alt+G", what: qsTr("Agents et missions") },
        { keys: "Ctrl+Alt+U", what: qsTr("Ultimate Agent") },
        { keys: "Ctrl+Alt+I", what: qsTr("Installation complète") },
        { keys: "Ctrl+Alt+M", what: qsTr("Mémoire persistante") },
        { keys: "Ctrl+Alt+A", what: qsTr("Autopilote") },
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
        opacity: 0.64
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.close()
    }

    Rectangle {
        id: card
        width: Math.min(640, parent.width - design.space7)
        height: Math.min(parent.height - design.space7, 780)
        anchors.centerIn: parent
        radius: design.radiusLg
        color: theme.surface
        border.width: 1
        border.color: theme.borderStrong

        // Absorb clicks on the card background so only the dim area closes it.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {}
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: design.space5
            spacing: design.space3

            RowLayout {
                Layout.fillWidth: true
                spacing: design.space2

                BrandMark { width: 28; height: 28; size: 28; accent: false }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Raccourcis")
                    font.family: "iA Writer Duospace"
                    font.pixelSize: design.textTitle
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
                text: qsTr("Tout se pilote au clavier. Le harness garde sa page ; le terminal reste un vrai shell et exécute les tâches via dsh headless.")
                wrapMode: Text.Wrap
                color: theme.mutedText
                font.pixelSize: design.textCaption
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.border
            }

            ScrollView {
                id: shortcutScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                focus: visible
                Component.onCompleted: contentY = 0
                onVisibleChanged: {
                    if (visible) {
                        contentY = 0
                        forceActiveFocus()
                    }
                }
                Keys.onDownPressed: contentY = Math.min(contentHeight - height, contentY + 40)
                Keys.onUpPressed: contentY = Math.max(0, contentY - 40)
                Keys.onPageDownPressed: contentY = Math.min(contentHeight - height, contentY + height * 0.85)
                Keys.onPageUpPressed: contentY = Math.max(0, contentY - height * 0.85)
                Keys.onHomePressed: contentY = 0
                Keys.onEndPressed: contentY = Math.max(0, contentHeight - height)
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical: AppScrollBar {}

                Column {
                    width: parent.width
                    spacing: design.space2

                    Repeater {
                        model: root.rows
                        RowLayout {
                            width: parent.width
                            spacing: design.space3

                            Rectangle {
                                Layout.preferredWidth: 112
                                height: design.controlHeightSm
                                radius: Math.min(5, theme.radius)
                                color: theme.raised
                                border.width: 1
                                border.color: theme.border
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.keys
                                    font.family: backend.monoFont
                                    font.pixelSize: design.textCaption
                                    color: theme.text
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.what
                                color: theme.mutedText
                                font.pixelSize: design.textLabel
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: design.space2

                ThemedIcon {
                    source: "icons/command.svg"
                    width: 11
                    height: 11
                    color: theme.dimText
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Molette pour parcourir · Échap pour fermer")
                    color: theme.dimText
                    font.pixelSize: design.textMicro
                }
                Button {
                    text: "↑"
                    flat: true
                    font.pixelSize: design.textLabel
                    enabled: shortcutScroll.contentY > 0
                    onClicked: shortcutScroll.contentY = Math.max(0, shortcutScroll.contentY - 180)
                }
                Button {
                    text: "↓"
                    flat: true
                    font.pixelSize: design.textLabel
                    enabled: shortcutScroll.contentY < shortcutScroll.contentHeight - shortcutScroll.height
                    onClicked: shortcutScroll.contentY = Math.min(
                        shortcutScroll.contentHeight - shortcutScroll.height,
                        shortcutScroll.contentY + 180)
                }
                Button {
                    text: qsTr("Fermer")
                    flat: true
                    font.pixelSize: design.textCaption
                    onClicked: root.close()
                }
            }
        }
    }
}
