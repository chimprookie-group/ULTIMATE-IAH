import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: win
    width: 1340
    height: 860
    minimumWidth: 760
    minimumHeight: 620
    visible: true
    title: backend.demo ? qsTr("Ultimate IAH") : "Ultimate IAH — " + backend.workspaceName
    color: theme.background
    background: Rectangle { color: theme.background }
    font.family: backend.uiFont
    font.pixelSize: 13

    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool sidebarUserCollapsed: false
    property bool agentDockOpen: false
    readonly property bool sidebarCompact: sidebarUserCollapsed || width < 1120
    readonly property real sidebarWidth: sidebarCompact ? design.sidebarCompactWidth : design.sidebarWidth
    readonly property real contentWidth: width - sidebarWidth
    readonly property string viewTitle: {
        if (backend.view === "home") return qsTr("Cockpit")
        if (backend.view === "ecosystem") return qsTr("Écosystème")
        if (backend.view === "workspace") return qsTr("Fichiers")
        if (backend.view === "terminal") return qsTr("Terminal agent")
        if (backend.view === "connections") return qsTr("Providers")
        if (backend.view === "memory") return qsTr("Mémoire")
        if (backend.view === "mcp") return "MCP"
        if (backend.view === "agents") return qsTr("Agents")
        if (backend.view === "agent") return qsTr("Ultimate Agent")
        if (backend.view === "bots") return qsTr("Bots")
        if (backend.view === "notes") return qsTr("Notes")
        if (backend.view === "github") return qsTr("GitHub")
        if (backend.view === "crasy") return "4Crasy"
        if (backend.view === "learning") return qsTr("Apprentissage")
        if (backend.view === "plugins") return qsTr("Plugins")
        if (backend.view === "installation") return qsTr("Installation")
        if (backend.view === "autopilot") return qsTr("Autopilote")
        if (backend.view === "settings") return qsTr("Réglages")
        return "Ultimate IAH"
    }
    property bool helpVisible: false
    onHelpVisibleChanged: if (helpVisible) hoverService.hide()
    onActiveChanged: if (!active) hoverService.hide()
    property bool quitConfirm: false
    property string toastText: ""
    // True while a text field has focus. "?" and "Q" are ordinary characters
    // there, so their shortcuts must stand down.
    property bool typing: false
    property bool forceQuit: false
    property bool terminalFocused: false

    readonly property bool compact: width < 1180

    function showToast(message) {
        win.toastText = message
        toastTimer.restart()
    }

    function shellQuote(value) {
        return "'" + String(value).replace(/'/g, "'\\''") + "'"
    }

    function runAgentTask(prompt) {
        const text = String(prompt).trim()
        if (text === "")
            return
        const context = memory.recall(backend.workspace, text, 8)
        const full = context !== ""
              ? qsTr("Mémoire persistante du projet :\n") + context + qsTr("\n\nTâche :\n") + text
              : text
        backend.setView("agent")
        agentDockOpen = false
        ultimateAgent.sendMessage(full)
    }

    function requestQuit() {
        Qt.quit()
    }

    function stateColor(state) {
        if (state === "running" || state === "ready")
            return theme.green
        if (state === "starting")
            return theme.yellow
        if (state === "failed")
            return theme.red
        return theme.dimText
    }

    readonly property var commands: {
        const list = []
        const add = function (group, label, hint, id, icon) {
            list.push({"group": group, "label": label, "hint": hint, "id": id, "icon": icon})
        }

        add(qsTr("Navigation"), qsTr("Home"), "Ctrl+0", "view:home", "icons/home.svg")
        add(qsTr("Navigation"), qsTr("Cockpit"), "Ctrl+0", "view:home", "icons/home.svg")
        add(qsTr("Navigation"), qsTr("Ultimate Agent"), "Ctrl+1", "view:agent", "icons/spark.svg")
        add(qsTr("Navigation"), qsTr("Terminal agent"), "Ctrl+2", "view:terminal", "icons/terminal.svg")
        add(qsTr("Navigation"), qsTr("Écosystème"), "Ctrl+6", "view:ecosystem", "icons/ecosystem.svg")
        add(qsTr("Navigation"), qsTr("Fichiers"), "Ctrl+7", "view:workspace", "icons/files.svg")
        add(qsTr("Navigation"), qsTr("Providers"), "Ctrl+8", "view:connections", "icons/providers.svg")
        add(qsTr("Navigation"), qsTr("MCP"), "Ctrl+Alt+C", "view:mcp", "icons/plug.svg")
        add(qsTr("Agents"), qsTr("Agents"), "Ctrl+Alt+G", "view:agents", "icons/agents.svg")
        add(qsTr("Agent"), qsTr("Ultimate Agent"), "Ctrl+Alt+U", "view:agent", "icons/spark.svg")
        add(qsTr("Agent"), qsTr("Bots"), "Ctrl+Alt+B", "view:bots", "icons/agents.svg")
        add(qsTr("Connaissance"), qsTr("Notes"), "Ctrl+Alt+N", "view:notes", "icons/notes.svg")
        add(qsTr("Opérer"), qsTr("GitHub"), "Ctrl+Alt+H", "view:github", "icons/github.svg")
        add(qsTr("Automatisation"), qsTr("Apprentissage"), "Ctrl+Alt+L", "view:learning", "icons/spark.svg")
        add(qsTr("Exécution"), "4Crasy", "Ctrl+Alt+4", "view:crasy", "icons/4crasy.svg")
        add(qsTr("Système"), qsTr("Plugins"), "Ctrl+Alt+P", "view:plugins", "icons/plug.svg")
        add(qsTr("Système"), qsTr("Installation"), "Ctrl+Alt+I", "view:installation", "icons/shield.svg")
        add(qsTr("Navigation"), qsTr("Mémoire"), "Ctrl+Alt+M", "view:memory", "icons/spark.svg")
        add(qsTr("Navigation"), qsTr("Autopilote"), "Ctrl+Alt+A", "view:autopilot", "icons/reload.svg")
        add(qsTr("Navigation"), qsTr("Réglages"), "Ctrl+4", "view:settings", "icons/gear.svg")

        add(qsTr("Actions"), qsTr("Change workspace"), "", "action:workspace", "icons/folder.svg")
        add(qsTr("Actions"), qsTr("Run diagnostics"), "", "action:diagnostics", "icons/plug.svg")
        add(qsTr("Actions"), qsTr("Providers"), "Ctrl+8", "view:connections", "icons/providers.svg")
        add(qsTr("Actions"), qsTr("Show shortcuts"), "?", "action:shortcuts", "icons/help.svg")

        add(qsTr("Appearance"), qsTr("Follow Omarchy"), "", "theme:", "icons/palette.svg")
        for (let i = 0; i < backend.themeOptions.length; ++i) {
            const option = backend.themeOptions[i]
            add(qsTr("Appearance"), option.label,
                option.mode === "light" ? qsTr("Light") : qsTr("Dark"),
                "theme:" + option.id, "icons/palette.svg")
        }

        add(qsTr("Language"), qsTr("Auto"), "", "lang:", "icons/globe.svg")
        add(qsTr("Language"), "Français", "", "lang:fr", "icons/globe.svg")
        add(qsTr("Language"), "English", "", "lang:en", "icons/globe.svg")

        return list
    }

    function runCommand(id) {
        if (id.indexOf("view:") === 0) {
            backend.setView(id.substring(5))
        } else if (id.indexOf("theme:") === 0) {
            backend.setTheme(id.substring(6))
        } else if (id.indexOf("lang:") === 0) {
            backend.setLanguage(id.substring(5))
        } else if (id === "action:workspace") {
            backend.pickWorkspace()
        } else if (id === "action:diagnostics") {
            backend.runDiagnostics()
            if (backend.view !== "home")
                win.showToast(qsTr("Diagnostics updated."))
        } else if (id === "action:shortcuts") {
            win.helpVisible = true
        }
    }

    Shortcut { sequence: "Ctrl+0"; context: Qt.ApplicationShortcut; onActivated: backend.setView("home") }
    Shortcut { sequence: "Ctrl+1"; context: Qt.ApplicationShortcut; onActivated: backend.setView("agent") }
    Shortcut { sequence: "Ctrl+2"; context: Qt.ApplicationShortcut; onActivated: backend.setView("terminal") }
    Shortcut { sequence: "Ctrl+4"; context: Qt.ApplicationShortcut; onActivated: backend.setView("settings") }
    Shortcut { sequence: "Ctrl+5"; context: Qt.ApplicationShortcut; onActivated: backend.setView("terminal") }
    Shortcut { sequence: "Ctrl+6"; context: Qt.ApplicationShortcut; onActivated: backend.setView("ecosystem") }
    Shortcut { sequence: "Ctrl+7"; context: Qt.ApplicationShortcut; onActivated: backend.setView("workspace") }
    Shortcut { sequence: "Ctrl+8"; context: Qt.ApplicationShortcut; onActivated: backend.setView("connections") }
    Shortcut { sequence: "Ctrl+Alt+C"; context: Qt.ApplicationShortcut; onActivated: backend.setView("mcp") }
    Shortcut { sequence: "Ctrl+Alt+G"; context: Qt.ApplicationShortcut; onActivated: backend.setView("agents") }
    Shortcut {
        sequence: "Ctrl+Alt+U"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (win.agentDockOpen) {
                win.agentDockOpen = false
            } else {
                if (backend.view === "agent")
                    backend.setView("home")
                win.agentDockOpen = true
            }
        }
    }
    Shortcut { sequence: "Ctrl+Alt+I"; context: Qt.ApplicationShortcut; onActivated: backend.setView("installation") }
    Shortcut { sequence: "Ctrl+Alt+B"; context: Qt.ApplicationShortcut; onActivated: backend.setView("bots") }
    Shortcut { sequence: "Ctrl+Alt+N"; context: Qt.ApplicationShortcut; onActivated: backend.setView("notes") }
    Shortcut { sequence: "Ctrl+Alt+H"; context: Qt.ApplicationShortcut; onActivated: backend.setView("github") }
    Shortcut { sequence: "Ctrl+Alt+4"; context: Qt.ApplicationShortcut; onActivated: backend.setView("crasy") }
    Shortcut { sequence: "Ctrl+Alt+L"; context: Qt.ApplicationShortcut; onActivated: backend.setView("learning") }
    Shortcut { sequence: "Ctrl+Alt+P"; context: Qt.ApplicationShortcut; onActivated: backend.setView("plugins") }
    Shortcut { sequence: "Ctrl+Alt+M"; context: Qt.ApplicationShortcut; onActivated: backend.setView("memory") }
    Shortcut { sequence: "Ctrl+Alt+A"; context: Qt.ApplicationShortcut; onActivated: backend.setView("autopilot") }
    Shortcut {
        sequence: "Ctrl+K"
        context: Qt.ApplicationShortcut
        enabled: !win.terminalFocused
        onActivated: commandPalette.opened ? commandPalette.close() : commandPalette.open()
    }
    Shortcut { sequence: "Ctrl+Q"; context: Qt.ApplicationShortcut; onActivated: win.requestQuit() }
    Shortcut {
        sequence: "F1"
        context: Qt.ApplicationShortcut
        enabled: !win.terminalFocused
        onActivated: win.helpVisible = !win.helpVisible
    }
    Shortcut {
        sequence: "?"
        context: Qt.ApplicationShortcut
        enabled: !win.typing && !win.helpVisible && !commandPalette.opened && !win.terminalFocused
        onActivated: win.helpVisible = true
    }
    Shortcut {
        sequence: "Q"
        context: Qt.ApplicationShortcut
        enabled: !win.typing && !win.helpVisible && !commandPalette.opened && !win.terminalFocused
        onActivated: win.requestQuit()
    }
    Shortcut {
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        enabled: win.helpVisible || win.quitConfirm || commandPalette.opened
        onActivated: {
            if (commandPalette.opened)
                commandPalette.close()
            else if (win.helpVisible)
                win.helpVisible = false
            else if (win.quitConfirm)
                win.quitConfirm = false
        }
    }

    Connections {
        target: ultimateAgent
        function onBusyChanged() {
            if (ultimateAgent.busy)
                win.agentDockOpen = true
        }
    }

    Component.onCompleted: {
        if (Qt.application.arguments.indexOf("--palette") !== -1)
            Qt.callLater(function () { commandPalette.open() })
    }

    onClosing: function (close) {
        close.accepted = true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── command header ────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: design.headerHeight
            color: theme.background

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: win.sidebarWidth + design.space4
                anchors.rightMargin: design.space4
                spacing: design.space3

                IconButton {
                    icon: "icons/menu.svg"
                    hint: qsTr("Replier la barre latérale")
                    onClicked: win.sidebarUserCollapsed = !win.sidebarUserCollapsed
                }

                ColumnLayout {
                    spacing: 0
                    Text {
                        text: backend.workspaceName
                        color: theme.brightText
                        font.family: "iA Writer Duospace"
                        font.pixelSize: design.textBodyLarge
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.maximumWidth: 280
                        text: "Ultimate IAH  /  " + win.viewTitle
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: design.textMicro
                        elide: Text.ElideRight
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    visible: win.width >= 940
                    Layout.preferredWidth: Math.min(320, Math.max(220, win.width * 0.24))
                    Layout.preferredHeight: design.controlHeight
                    radius: design.radiusSm
                    color: searchMouse.containsMouse ? theme.raised : theme.chip
                    Behavior on color { ColorAnimation { duration: design.durationFast } }
                    border.width: 1
                    border.color: searchMouse.containsMouse ? theme.borderStrong : theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: design.space3
                        anchors.rightMargin: design.space2
                        spacing: design.space2
                        ThemedIcon { source: "icons/search.svg"; width: design.iconSm; height: design.iconSm; color: theme.dimText }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Commandes, vues, thèmes…")
                            color: theme.dimText
                            font.pixelSize: design.textCaption
                            elide: Text.ElideRight
                        }
                        Rectangle {
                            width: 42; height: 20; radius: design.radiusSm
                            color: theme.background
                            border.width: 1; border.color: theme.border
                            Text {
                                anchors.centerIn: parent
                                text: "Ctrl K"
                                color: theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: design.textMicro
                            }
                        }
                    }
                    MouseArea {
                        id: searchMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: commandPalette.open()
                    }
                }

                Rectangle {
                    visible: win.width >= 1180
                    height: design.controlHeight
                    width: liveRow.implicitWidth + design.space3 * 2
                    radius: design.radiusSm
                    color: theme.chip
                    Behavior on color { ColorAnimation { duration: design.durationFast } }
                    border.width: 1
                    border.color: theme.border
                    RowLayout {
                        id: liveRow
                        anchors.centerIn: parent
                        spacing: design.space2
                        StatusDot { size: 5; color: theme.green }
                        Text {
                            text: qsTr("agent")
                            color: theme.mutedText
                            font.family: backend.monoFont
                            font.pixelSize: design.textMicro
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.setView("agent")
                    }
                }

                IconButton {
                    visible: win.width < 940
                    icon: "icons/search.svg"
                    hint: qsTr("Command palette (Ctrl+K)")
                    onClicked: commandPalette.open()
                }
                IconButton {
                    icon: "icons/spark.svg"
                    hint: qsTr("Ultimate Agent (Ctrl+Alt+U)")
                    onClicked: {
                        if (win.agentDockOpen) {
                            win.agentDockOpen = false
                        } else {
                            if (backend.view === "agent")
                                backend.setView("home")
                            win.agentDockOpen = true
                        }
                    }
                }
                IconButton {
                    icon: "icons/help.svg"
                    hint: qsTr("Raccourcis clavier (?)")
                    onClicked: win.helpVisible = true
                }
            }
        }

        // ── rail + content ────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: win.sidebarWidth
                Layout.rightMargin: win.agentDockOpen && backend.view !== "agent"
                                     && win.width >= 1280 ? agentDock.width : 0

                TerminalPage {
                    anchors.fill: parent
                    visible: backend.view === "terminal"
                }

                EcosystemPane {
                    id: ecosystemPane
                    anchors.fill: parent
                    visible: backend.view === "ecosystem"
                    onCommandRequested: function (command) {
                        terminal.run(command)
                        backend.setView("terminal")
                    }
                    onAgentTaskRequested: function (prompt) {
                        win.runAgentTask(prompt)
                    }
                }

                ConnectionsPane {
                    anchors.fill: parent
                    visible: backend.view === "connections"
                }

                McpPane {
                    anchors.fill: parent
                    visible: backend.view === "mcp"
                }

                UltimateAgentPane {
                    anchors.fill: parent
                    visible: backend.view === "agent"
                }

                BotsPane {
                    anchors.fill: parent
                    visible: backend.view === "bots"
                }

                NotesPane {
                    anchors.fill: parent
                    visible: backend.view === "notes"
                }

                GitHubPane {
                    anchors.fill: parent
                    visible: backend.view === "github"
                }

                CrasyPane {
                    anchors.fill: parent
                    visible: backend.view === "crasy"
                }

                LearningPane {
                    anchors.fill: parent
                    visible: backend.view === "learning"
                }

                PluginsPane {
                    anchors.fill: parent
                    visible: backend.view === "plugins"
                }

                AgentsPane {
                    anchors.fill: parent
                    visible: backend.view === "agents"
                }

                InstallationPane {
                    anchors.fill: parent
                    visible: backend.view === "installation"
                }

                MemoryPane {
                    anchors.fill: parent
                    visible: backend.view === "memory"
                }

                AutopilotPane {
                    anchors.fill: parent
                    visible: backend.view === "autopilot"
                }

                WorkspacePane {
                    anchors.fill: parent
                    visible: backend.view === "workspace"
                    onAgentTaskRequested: function (prompt) {
                        win.runAgentTask(prompt)
                    }
                }

                HomePane {
                    anchors.fill: parent
                    visible: backend.view === "home"
                    onOpenPalette: commandPalette.open()
                    onCommandRequested: function (command) {
                        terminal.run(command)
                        backend.setView("terminal")
                    }
                    onAgentTaskRequested: function (prompt) {
                        win.runAgentTask(prompt)
                    }
                }

                SettingsPane {
                    anchors.fill: parent
                    visible: backend.view === "settings"
                }
            }
        }

        // ── status bar ────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: design.statusHeight
            color: theme.background

            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: win.sidebarWidth + design.space4
                anchors.rightMargin: design.space4
                spacing: design.space3

                ThemedIcon {
                    Layout.alignment: Qt.AlignVCenter
                    source: "icons/folder.svg"
                    width: design.iconSm
                    height: design.iconSm
                    color: theme.dimText
                }
                Text {
                    Layout.alignment: Qt.AlignVCenter
                    text: backend.workspace
                    font.family: backend.monoFont
                    font.pixelSize: design.textMicro
                    color: theme.dimText
                    elide: Text.ElideMiddle
                    Layout.maximumWidth: win.width * 0.38
                }

                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    visible: backend.gitAvailable
                    width: 1
                    height: 12
                    color: theme.border
                }
                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: design.space1
                    visible: backend.gitAvailable
                    ThemedIcon {
                        Layout.alignment: Qt.AlignVCenter
                        source: "icons/branch.svg"
                        width: 11
                        height: 11
                        color: backend.gitDirtyCount > 0 ? theme.orange : theme.green
                    }
                    Text {
                        Layout.alignment: Qt.AlignVCenter
                        text: backend.gitBranch
                        color: theme.mutedText
                        font.family: backend.monoFont
                        font.pixelSize: design.textMicro
                    }
                }

                Item { Layout.fillWidth: true }

                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: design.space3
                    Repeater {
                        model: [
                            { label: qsTr("Agent"), color: ultimateAgent.busy ? theme.yellow : theme.green },
                            { label: qsTr("Terminal"), color: terminal.running ? theme.green : theme.dimText },
                            { label: qsTr("API"), color: connections.deepseekConfigured ? theme.green : theme.orange }
                        ]
                        RowLayout {
                            Layout.alignment: Qt.AlignVCenter
                            spacing: design.space1
                            StatusDot { Layout.alignment: Qt.AlignVCenter; size: 4; color: modelData.color }
                            Text {
                                Layout.alignment: Qt.AlignVCenter
                                visible: win.width >= 600
                                text: modelData.label
                                color: theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: design.textMicro
                            }
                        }
                    }
                }
            }
        }
    }

    AppSidebar {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: win.sidebarWidth
        compact: win.sidebarCompact
        z: 40
        onToggleCompact: win.sidebarUserCollapsed = !win.sidebarUserCollapsed
        onCommandRequested: commandPalette.open()
    }

    Timer {
        id: toastTimer
        interval: 3600
        onTriggered: win.toastText = ""
    }

    Connections {
        target: backend
        function onToast(message) { win.showToast(message) }
        function onViewChanged() { hoverService.hide() }

    }

    Rectangle {
        id: globalHoverTip
        visible: hoverService.visible && hoverService.text !== ""
        z: 500
        width: Math.min(240, hoverMeasure.implicitWidth + design.space4)
        height: design.controlHeightSm
        radius: design.radiusSm
        color: theme.raised
        border.width: 1
        border.color: theme.borderStrong

        readonly property real anchorRight: hoverService.anchorX + hoverService.anchorWidth + design.space2
        x: anchorRight + width + design.space2 <= win.width
           ? anchorRight
           : Math.max(design.space2, hoverService.anchorX - width - design.space2)
        y: Math.max(design.space2,
                    Math.min(hoverService.anchorY + (hoverService.anchorHeight - height) / 2,
                             win.height - height - design.space2))

        Text {
            id: hoverMeasure
            visible: false
            text: hoverService.text
            font.pixelSize: design.textCaption
            font.family: backend.uiFont
        }
        Text {
            anchors.fill: parent
            anchors.leftMargin: design.space2
            anchors.rightMargin: design.space2
            text: hoverService.text
            color: theme.text
            font.pixelSize: design.textCaption
            font.family: backend.uiFont
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }

    UltimateAgentDock {
        id: agentDock
        visible: win.agentDockOpen && backend.view !== "agent"
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.topMargin: design.headerHeight
        anchors.bottomMargin: design.statusHeight
        width: Math.min(400, win.width - win.sidebarWidth - 24)
        z: 90
        onCloseRequested: win.agentDockOpen = false
        onOpenRequested: {
            win.agentDockOpen = false
            backend.setView("agent")
        }
    }

    HelpOverlay {
        anchors.fill: parent
        visible: win.helpVisible
        onClose: win.helpVisible = false
    }

    CommandPalette {
        id: commandPalette
        anchors.fill: parent
        commands: win.commands
        onChosen: win.runCommand(id)
    }

    // ── quit confirmation ────────────────────────────────────────────────
    Item {
        anchors.fill: parent
        visible: win.quitConfirm
        z: 200

        Rectangle {
            anchors.fill: parent
            color: "#000000"
            opacity: 0.6
        }
        MouseArea {
            anchors.fill: parent
            onClicked: win.quitConfirm = false
        }

        Rectangle {
            width: 380
            height: quitColumn.implicitHeight + 40
            anchors.centerIn: parent
            radius: Math.min(12, theme.radius)
            color: theme.raised
            border.width: 1
            border.color: theme.borderStrong

            ColumnLayout {
                id: quitColumn
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12

                Text {
                    text: qsTr("A turn is still running")
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    color: theme.text
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Une tâche est encore en cours.")
                    wrapMode: Text.Wrap
                    color: theme.mutedText
                    font.pixelSize: 12
                }
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: 8

                    GhostButton {
                        text: qsTr("Keep working")
                        flat: true
                        onClicked: win.quitConfirm = false
                    }
                    ActionButton {
                        text: qsTr("Quit")
                        onClicked: {
                            win.forceQuit = true
                            Qt.quit()
                        }
                    }
                }
            }
        }
    }

    // ── toast ────────────────────────────────────────────────────────────
    Rectangle {
        visible: win.toastText !== ""
        z: 300
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 44
        width: Math.min(560, toastLabel.implicitWidth + 28)
        height: 34
        radius: 17
        color: theme.raised
        border.width: 1
        border.color: theme.borderStrong

        Text {
            id: toastLabel
            anchors.centerIn: parent
            text: win.toastText
            color: theme.text
            font.pixelSize: 12
            elide: Text.ElideRight
            width: parent.width - 28
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
