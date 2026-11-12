import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    signal close()

    property var rows: [
        { keys: "Ctrl+0", what: qsTr("Ultimate IAH cockpit") },
        { keys: "Ctrl+K", what: qsTr("Command palette") },
        { keys: "Ctrl+1", what: qsTr("Ultimate Agent") },
        { keys: "Ctrl+2", what: qsTr("Terminal agent") },
        { keys: "Ctrl+6", what: qsTr("Ecosystem") },
        { keys: "Ctrl+7", what: qsTr("Files and diffs") },
        { keys: "Ctrl+8", what: qsTr("Providers") },
        { keys: "Ctrl+Alt+C", what: qsTr("Serveurs MCP") },
        { keys: "Ctrl+Alt+G", what: qsTr("Agents et missions") },
        { keys: "Ctrl+Alt+U", what: qsTr("Ultimate Agent") },
        { keys: "Ctrl+Alt+I", what: qsTr("Installation complète") },
        { keys: "Ctrl+Alt+B", what: qsTr("Bots de l'écosystème") },
        { keys: "Ctrl+Alt+N", what: qsTr("Notes et vault Markdown") },
        { keys: "Ctrl+Alt+M", what: qsTr("Mémoire persistante") },
        { keys: "Ctrl+Alt+A", what: qsTr("Autopilote") },
        { keys: "Ctrl+4", what: qsTr("Settings") },


        { keys: "?", what: qsTr("This help") },
        { keys: "Q", what: qsTr("Quit") }
    ]

    z: 150
    visible: false
    focus: visible
    onVisibleChanged: if (visible) forceActiveFocus()
    Keys.onPressed: function (event) {
        const maximum = Math.max(0, shortcutScroll.contentHeight - shortcutScroll.height)
        if (event.key === Qt.Key_Down) {
            shortcutScroll.contentY = Math.min(maximum, shortcutScroll.contentY + 40); event.accepted = true
        } else if (event.key === Qt.Key_Up) {
            shortcutScroll.contentY = Math.max(0, shortcutScroll.contentY - 40); event.accepted = true
        } else if (event.key === Qt.Key_PageDown) {
            shortcutScroll.contentY = Math.min(maximum, shortcutScroll.contentY + shortcutScroll.height * 0.85); event.accepted = true
        } else if (event.key === Qt.Key_PageUp) {
            shortcutScroll.contentY = Math.max(0, shortcutScroll.contentY - shortcutScroll.height * 0.85); event.accepted = true
        }
    }

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
                text: qsTr("Tout se pilote au clavier. Ultimate Agent est le centre de l'application ; le terminal reste un vrai shell.")
                wrapMode: Text.Wrap
                color: theme.mutedText
                font.pixelSize: design.textCaption
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.border
            }

            Flickable {
                id: shortcutScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: shortcutColumn.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick
                focus: visible
                focusPolicy: Qt.StrongFocus
                activeFocusOnTab: true
                Component.onCompleted: contentY = 0
                onVisibleChanged: {
                    if (visible) {
                        contentY = 0
                        forceActiveFocus()
                    }
                }
                Keys.onPressed: function (event) {
                    const maximum = Math.max(0, contentHeight - height)
                    if (event.key === Qt.Key_Down) {
                        contentY = Math.min(maximum, contentY + 40); event.accepted = true
                    } else if (event.key === Qt.Key_Up) {
                        contentY = Math.max(0, contentY - 40); event.accepted = true
                    } else if (event.key === Qt.Key_PageDown) {
                        contentY = Math.min(maximum, contentY + height * 0.85); event.accepted = true
                    } else if (event.key === Qt.Key_PageUp) {
                        contentY = Math.max(0, contentY - height * 0.85); event.accepted = true
                    }
                }
                ScrollBar.vertical: AppScrollBar {}

                Column {
                    id: shortcutColumn
                    width: shortcutScroll.width
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
                                font.pixelSize: design.textCaption
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
