import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property bool compact: false
    signal toggleCompact()
    signal commandRequested()

    implicitWidth: compact ? design.sidebarCompactWidth : design.sidebarWidth

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
            Layout.preferredHeight: 74
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: backend.setView("home")
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: root.compact ? 0 : design.space4
                    anchors.rightMargin: root.compact ? 0 : design.space3
                    spacing: design.space3

                    Rectangle {
                        Layout.alignment: root.compact ? Qt.AlignHCenter : Qt.AlignVCenter
                        width: 36
                        height: 36
                        radius: design.radiusMd
                        color: theme.raised
                        border.width: 1
                        border.color: theme.border
                        BrandMark {
                            anchors.centerIn: parent
                            size: 30
                            accent: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: !root.compact
                        spacing: 1
                        RowLayout {
                            spacing: design.space1
                            Text { text: "Ultimate"; color: theme.brightText; font.family: "iA Writer Duospace"; font.pixelSize: design.textTitle; font.weight: Font.Bold }
                            Text { text: "HAI"; color: theme.accent; font.family: "iA Writer Duospace"; font.pixelSize: design.textTitle; font.weight: Font.Bold }
                        }
                        Text {
                            text: qsTr("AGENTIC CONTROL")
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: 7
                            font.letterSpacing: 1.0
                        }
                    }
                }
            }
        }

        // Workspace identity
        Rectangle {
            visible: !root.compact
            Layout.fillWidth: true
            Layout.leftMargin: design.space3
            Layout.rightMargin: design.space3
            Layout.preferredHeight: 44
            radius: design.radiusSm
            color: workspaceMouse.containsMouse ? theme.chip : "transparent"
            border.width: 1
            border.color: theme.border

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: design.space3
                anchors.rightMargin: design.space2
                spacing: design.space2
                ThemedIcon { source: "icons/folder.svg"; width: design.iconSm; height: design.iconSm; color: theme.dimText }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Text { Layout.fillWidth: true; text: backend.workspaceName; color: theme.text; font.pixelSize: design.textCaption; font.weight: Font.DemiBold; elide: Text.ElideRight }
                    Text {
                        Layout.fillWidth: true
                        text: backend.gitAvailable ? (backend.gitBranch + (backend.gitDirtyCount > 0 ? "  ·  " + backend.gitDirtyCount : "")) : qsTr("espace de travail")
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: design.textMicro
                        elide: Text.ElideRight
                    }
                }
                ThemedIcon { source: "icons/chevron.svg"; width: 10; height: 10; color: theme.dimText }
            }
            MouseArea {
                id: workspaceMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: backend.setView("ecosystem")
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical: AppScrollBar {}
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 0

                SidebarGroup { title: qsTr("OPÉRER"); compact: root.compact }
                SidebarItem { Layout.fillWidth: true; compact: root.compact; icon: "icons/home.svg"; label: qsTr("Cockpit"); hint: qsTr("Cockpit (Ctrl+0)"); active: backend.view === "home"; onClicked: backend.setView("home") }
                SidebarItem { Layout.fillWidth: true; compact: root.compact; icon: "icons/branch.svg"; label: qsTr("Écosystème"); hint: qsTr("Écosystème (Ctrl+6)"); active: backend.view === "ecosystem"; onClicked: backend.setView("ecosystem") }
                SidebarItem { Layout.fillWidth: true; compact: root.compact; icon: "icons/files.svg"; label: qsTr("Fichiers"); hint: qsTr("Fichiers et diffs (Ctrl+7)"); active: backend.view === "workspace"; onClicked: backend.setView("workspace") }

                SidebarGroup { title: qsTr("AGENT"); compact: root.compact }
                SidebarItem { Layout.fillWidth: true; compact: root.compact; icon: "icons/spark.svg"; label: qsTr("Ultimate Agent"); hint: qsTr("Ultimate Agent (Ctrl+Alt+U)"); active: backend.view === "agent"; onClicked: backend.setView("agent") }

                SidebarGroup { title: qsTr("INTELLIGENCE"); compact: root.compact }
                SidebarItem { Layout.fillWidth: true; compact: root.compact; icon: "icons/memory.svg"; label: qsTr("Mémoire"); hint: qsTr("Mémoire persistante (Ctrl+Alt+M)"); active: backend.view === "memory"; onClicked: backend.setView("memory") }
                SidebarItem { Layout.fillWidth: true; compact: root.compact; icon: "icons/autopilot.svg"; label: qsTr("Autopilote"); hint: qsTr("Tâches autonomes (Ctrl+Alt+A)"); active: backend.view === "autopilot"; onClicked: backend.setView("autopilot") }
                SidebarItem { Layout.fillWidth: true; compact: root.compact; icon: "icons/agents.svg"; label: qsTr("Agents"); hint: qsTr("Missions plan → exécution → revue (Ctrl+Alt+G)"); active: backend.view === "agents"; onClicked: backend.setView("agents") }

                SidebarGroup { title: qsTr("RUNTIME"); compact: root.compact }
                SidebarItem {
                    Layout.fillWidth: true; compact: root.compact; icon: "icons/harness.svg"; label: qsTr("Harness"); hint: qsTr("DeepSeek Harness (Ctrl+1)")
                    active: backend.view === "harness"
                    statusColor: backend.harnessState === "running" ? theme.green : (backend.harnessState === "starting" ? theme.yellow : theme.dimText)
                    onClicked: backend.setView("harness")
                }
                SidebarItem {
                    Layout.fillWidth: true; compact: root.compact; icon: "icons/terminal.svg"; label: qsTr("Terminal agent"); hint: qsTr("Terminal agent (Ctrl+2)")
                    active: backend.view === "terminal"
                    statusColor: terminal.running ? theme.green : theme.dimText
                    onClicked: backend.setView("terminal")
                }

                SidebarGroup { title: qsTr("SYSTÈME"); compact: root.compact }
                SidebarItem { Layout.fillWidth: true; compact: root.compact; icon: "icons/connections.svg"; label: qsTr("Connexions"); hint: qsTr("DeepSeek et providers (Ctrl+8)"); active: backend.view === "connections"; onClicked: backend.setView("connections") }
                SidebarItem { Layout.fillWidth: true; compact: root.compact; icon: "icons/mcp.svg"; label: "MCP"; hint: qsTr("Serveurs MCP (Ctrl+Alt+C)"); active: backend.view === "mcp"; onClicked: backend.setView("mcp") }
                SidebarItem { Layout.fillWidth: true; compact: root.compact; icon: "icons/shield.svg"; label: qsTr("Installation"); hint: qsTr("Audit et installation (Ctrl+Alt+I)"); active: backend.view === "installation"; onClicked: backend.setView("installation") }

                Item { Layout.preferredHeight: design.space5 }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.leftMargin: design.space3; Layout.rightMargin: design.space3; height: 1; color: theme.border }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: design.space2
            spacing: design.space1

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: root.compact ? 26 : design.controlHeight
                Layout.leftMargin: root.compact ? 21 : design.space1
                Layout.rightMargin: root.compact ? 21 : design.space1
                radius: design.radiusSm
                color: commandMouse.containsMouse ? theme.chip : "transparent"
                border.width: 1
                border.color: theme.border
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: root.compact ? 0 : design.space3
                    anchors.rightMargin: root.compact ? 0 : design.space2
                    spacing: design.space2
                    ThemedIcon {
                        Layout.alignment: root.compact ? Qt.AlignHCenter : Qt.AlignVCenter
                        Layout.fillWidth: root.compact
                        source: "icons/command.svg"
                        width: design.iconSm; height: design.iconSm
                        color: commandMouse.containsMouse ? theme.accent : theme.mutedText
                        Behavior on color { ColorAnimation { duration: design.durationFast } }
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: !root.compact
                        text: qsTr("Commandes")
                        color: theme.mutedText
                        font.pixelSize: design.textCaption
                    }
                    Rectangle {
                        visible: !root.compact
                        width: 38; height: 19; radius: design.radiusSm
                        color: theme.background
                        border.width: 1; border.color: theme.border
                        Text { anchors.centerIn: parent; text: "Ctrl K"; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                    }
                }
                MouseArea {
                    id: commandMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.commandRequested()
                }
            }

            SidebarItem { Layout.fillWidth: true; compact: root.compact; icon: "icons/sliders.svg"; label: qsTr("Réglages"); hint: qsTr("Réglages (Ctrl+4)"); active: backend.view === "settings"; onClicked: backend.setView("settings") }
            SidebarItem {
                Layout.fillWidth: true; compact: root.compact
                icon: root.compact ? "icons/chevron.svg" : "icons/chevron-left.svg"
                label: qsTr("Replier")
                hint: qsTr("Replier la barre latérale")
                onClicked: root.toggleCompact()
            }
        }
    }
}
