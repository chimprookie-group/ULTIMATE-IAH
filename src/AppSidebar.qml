import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property bool compact: false
    signal toggleCompact()

    implicitWidth: compact ? 64 : 212

    Rectangle {
        anchors.fill: parent
        color: theme.surface
        Rectangle {
            anchors.right: parent.right
            width: 1
            height: parent.height
            color: theme.border
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Brand
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 66
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: backend.setView("home")
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: root.compact ? 0 : 16
                    anchors.rightMargin: root.compact ? 0 : 12
                    spacing: 9
                    Item {
                        Layout.alignment: root.compact ? Qt.AlignHCenter : Qt.AlignVCenter
                        width: 30
                        height: 30
                        Image {
                            anchors.centerIn: parent
                            source: "assets/icon-128.png"
                            sourceSize.width: 28
                            sourceSize.height: 28
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: !root.compact
                        spacing: 0
                        RowLayout {
                            spacing: 3
                            Text { text: "Crasy"; color: theme.text; font.family: "iA Writer Duospace"; font.pixelSize: 16; font.weight: Font.Bold }
                            Text { text: "H"; color: theme.accent; font.family: "iA Writer Duospace"; font.pixelSize: 16; font.weight: Font.Bold }
                        }
                        Text {
                            text: qsTr("HARNESS · OPENCODE · APPS")
                            color: theme.dimText
                            font.pixelSize: 7
                            font.letterSpacing: 0.9
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.leftMargin: 14; Layout.rightMargin: 14; height: 1; color: theme.border; visible: !root.compact }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 1

                Text {
                    visible: !root.compact
                    Layout.leftMargin: 18
                    Layout.topMargin: 14
                    Layout.bottomMargin: 3
                    text: qsTr("ATELIER")
                    color: theme.dimText
                    font.pixelSize: 7
                    font.letterSpacing: 1.1
                    font.weight: Font.DemiBold
                }

                SidebarItem {
                    Layout.fillWidth: true
                    compact: root.compact
                    icon: "icons/home.svg"
                    label: qsTr("Cockpit")
                    hint: qsTr("Cockpit (Ctrl+0)")
                    active: backend.view === "home"
                    onClicked: backend.setView("home")
                }
                SidebarItem {
                    Layout.fillWidth: true
                    compact: root.compact
                    icon: "icons/ecosystem.svg"
                    label: qsTr("Projet")
                    hint: qsTr("Projet et écosystème (Ctrl+6)")
                    active: backend.view === "ecosystem"
                    onClicked: backend.setView("ecosystem")
                }
                SidebarItem {
                    Layout.fillWidth: true
                    compact: root.compact
                    icon: "icons/files.svg"
                    label: qsTr("Fichiers")
                    hint: qsTr("Fichiers et diffs (Ctrl+7)")
                    active: backend.view === "workspace"
                    onClicked: backend.setView("workspace")
                }
                SidebarItem {
                    Layout.fillWidth: true
                    compact: root.compact
                    icon: "icons/terminal.svg"
                    label: qsTr("Terminal")
                    hint: qsTr("Terminal (Ctrl+5)")
                    active: backend.view === "terminal"
                    onClicked: backend.setView("terminal")
                }

                Text {
                    visible: !root.compact
                    Layout.leftMargin: 18
                    Layout.topMargin: 14
                    Layout.bottomMargin: 3
                    text: qsTr("AGENTS")
                    color: theme.dimText
                    font.pixelSize: 7
                    font.letterSpacing: 1.1
                    font.weight: Font.DemiBold
                }

                SidebarItem {
                    Layout.fillWidth: true
                    compact: root.compact
                    icon: "icons/harness.svg"
                    label: qsTr("Harness")
                    hint: qsTr("DeepSeek Harness (Ctrl+1)")
                    active: backend.view === "harness"
                    statusColor: backend.harnessState === "running" ? theme.green
                               : (backend.harnessState === "starting" ? theme.yellow : theme.dimText)
                    onClicked: backend.setView("harness")
                }
                SidebarItem {
                    Layout.fillWidth: true
                    compact: root.compact
                    icon: "icons/chat.svg"
                    label: "OpenCode"
                    hint: qsTr("OpenCode (Ctrl+2)")
                    active: backend.view === "opencode"
                    statusColor: backend.opencodeState === "ready" ? theme.green
                               : (backend.opencodeState === "starting" ? theme.yellow : theme.dimText)
                    onClicked: backend.setView("opencode")
                }
                SidebarItem {
                    Layout.fillWidth: true
                    compact: root.compact
                    icon: "icons/split.svg"
                    label: qsTr("Split")
                    hint: qsTr("Les deux panes (Ctrl+3)")
                    active: backend.view === "split"
                    onClicked: {
                        if (win.compact) {
                            win.showToast(qsTr("Split view needs a window at least 1180 px wide."))
                            return
                        }
                        backend.setView("split")
                    }
                }

                Text {
                    visible: !root.compact
                    Layout.leftMargin: 18
                    Layout.topMargin: 14
                    Layout.bottomMargin: 3
                    text: qsTr("INTÉGRATIONS")
                    color: theme.dimText
                    font.pixelSize: 7
                    font.letterSpacing: 1.1
                    font.weight: Font.DemiBold
                }

                SidebarItem {
                    Layout.fillWidth: true
                    compact: root.compact
                    icon: "icons/providers.svg"
                    label: qsTr("Providers")
                    hint: qsTr("Providers et clés (Ctrl+8)")
                    active: backend.view === "providers"
                    onClicked: backend.setView("providers")
                }
                SidebarItem {
                    Layout.fillWidth: true
                    compact: root.compact
                    icon: "icons/router.svg"
                    label: "9Router"
                    hint: qsTr("9Router (Ctrl+9)")
                    active: backend.view === "ninerouter"
                    statusColor: nineRouter.running ? theme.green
                               : (nineRouter.starting ? theme.yellow : theme.dimText)
                    onClicked: backend.setView("ninerouter")
                }
                SidebarItem {
                    Layout.fillWidth: true
                    compact: root.compact
                    icon: "icons/mausbot.svg"
                    label: "MausBot"
                    hint: qsTr("MausBot (Ctrl+Shift+M)")
                    active: backend.view === "mausbot"
                    statusColor: mausBot.running ? theme.green
                               : (mausBot.starting ? theme.yellow : theme.dimText)
                    onClicked: backend.setView("mausbot")
                }

                Item { Layout.preferredHeight: 14 }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.leftMargin: 14; Layout.rightMargin: 14; height: 1; color: theme.border }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 7
            spacing: 2
            SidebarItem {
                Layout.fillWidth: true
                compact: root.compact
                icon: "icons/gear.svg"
                label: qsTr("Réglages")
                hint: qsTr("Réglages (Ctrl+4)")
                active: backend.view === "settings"
                onClicked: backend.setView("settings")
            }
            SidebarItem {
                Layout.fillWidth: true
                compact: root.compact
                icon: "icons/chevron.svg"
                label: qsTr("Replier")
                hint: qsTr("Replier la barre latérale")
                onClicked: root.toggleCompact()
            }
        }
    }
}
