import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: win
    width: 1340
    height: 860
    minimumWidth: 720
    minimumHeight: 600
    visible: true
    title: backend.demo ? qsTr("Crasy Harness") : "Crasy Harness — " + backend.workspaceName
    color: theme.background
    background: Rectangle {
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: theme.ambientTop }
            GradientStop { position: 0.45; color: theme.background }
            GradientStop { position: 1.0; color: theme.ambientBottom }
        }
    }
    font.family: backend.uiFont
    font.pixelSize: 13

    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool sidebarUserCollapsed: false
    readonly property bool sidebarCompact: sidebarUserCollapsed || width < 1040
    readonly property real contentWidth: width - (sidebarCompact ? 64 : 212)
    readonly property string viewTitle: {
        if (backend.view === "home") return qsTr("Cockpit")
        if (backend.view === "ecosystem") return qsTr("Projet")
        if (backend.view === "workspace") return qsTr("Fichiers")
        if (backend.view === "ninerouter") return "9Router"
        if (backend.view === "mausbot") return "MausBot"
        if (backend.view === "harness") return qsTr("Harness")
        if (backend.view === "terminal") return qsTr("Terminal agent")
        if (backend.view === "connections") return qsTr("Connexions")
        if (backend.view === "memory") return qsTr("Mémoire")
        if (backend.view === "autopilot") return qsTr("Autopilote")
        if (backend.view === "settings") return qsTr("Réglages")
        return "DeepCrasy H"
    }
    property bool helpVisible: false
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
        const binary = backend.dshBinary !== "" ? backend.dshBinary : "dsh"
        const env = connections.envPath
        const prefix = "set -a; if [ -f " + shellQuote(env) + " ]; then . " + shellQuote(env) + "; fi; set +a; "
        const context = memory.recall(backend.workspace, text, 8)
        const full = context !== ""
              ? "Mémoire persistante du projet (contexte fiable, ne la répète pas) :\n" + context + "\n\nTâche :\n" + text
              : text
        backend.setView("terminal")
        terminal.run(prefix + binary + " headless " + shellQuote(full))
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
        add(qsTr("Navigation"), qsTr("Harness"), "Ctrl+1", "view:harness", "icons/harness.svg")
        add(qsTr("Navigation"), qsTr("Terminal agent"), "Ctrl+2", "view:terminal", "icons/terminal.svg")
        add(qsTr("Navigation"), qsTr("Projet"), "Ctrl+6", "view:ecosystem", "icons/ecosystem.svg")
        add(qsTr("Navigation"), qsTr("Fichiers"), "Ctrl+7", "view:workspace", "icons/files.svg")
        add(qsTr("Navigation"), qsTr("Connexions"), "Ctrl+8", "view:connections", "icons/providers.svg")
        add(qsTr("Navigation"), qsTr("Mémoire"), "Ctrl+Alt+M", "view:memory", "icons/spark.svg")
        add(qsTr("Navigation"), qsTr("Autopilote"), "Ctrl+Alt+A", "view:autopilot", "icons/reload.svg")
        add(qsTr("Navigation"), "9Router", "Ctrl+9", "view:ninerouter", "icons/router.svg")
        add(qsTr("Navigation"), "MausBot", "Ctrl+Shift+M", "view:mausbot", "icons/mausbot.svg")
        add(qsTr("Navigation"), qsTr("Réglages"), "Ctrl+4", "view:settings", "icons/gear.svg")

        add(qsTr("Actions"), qsTr("Restart the harness"), "Ctrl+R", "action:restart-harness", "icons/reload.svg")
        add(qsTr("Actions"), qsTr("Change workspace"), "", "action:workspace", "icons/folder.svg")
        add(qsTr("Actions"), qsTr("Run diagnostics"), "", "action:diagnostics", "icons/plug.svg")
        add(qsTr("Actions"), qsTr("Harness connections"), "", "action:mcp", "icons/plug.svg")
        add(qsTr("Actions"), qsTr("Open the harness in a browser"), "", "action:browser", "icons/external.svg")
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
        } else if (id === "action:restart-harness") {
            backend.restartHarness()
        } else if (id === "action:workspace") {
            backend.pickWorkspace()
        } else if (id === "action:diagnostics") {
            backend.runDiagnostics()
            if (backend.view !== "home")
                win.showToast(qsTr("Diagnostics updated."))
        } else if (id === "action:browser") {
            if (backend.harnessUrl !== "")
                Qt.openUrlExternally(backend.harnessUrl)
        } else if (id === "action:mcp") {
            backend.setView("settings")
        } else if (id === "action:shortcuts") {
            win.helpVisible = true
        }
    }

    // The harness WebEngine lives for the whole session and is unloaded while
    // another view is active.

    Shortcut { sequence: "Ctrl+0"; context: Qt.ApplicationShortcut; onActivated: backend.setView("home") }
    Shortcut { sequence: "Ctrl+1"; context: Qt.ApplicationShortcut; onActivated: backend.setView("harness") }
    Shortcut { sequence: "Ctrl+2"; context: Qt.ApplicationShortcut; onActivated: backend.setView("terminal") }
    Shortcut { sequence: "Ctrl+4"; context: Qt.ApplicationShortcut; onActivated: backend.setView("settings") }
    Shortcut { sequence: "Ctrl+5"; context: Qt.ApplicationShortcut; onActivated: backend.setView("terminal") }
    Shortcut { sequence: "Ctrl+6"; context: Qt.ApplicationShortcut; onActivated: backend.setView("ecosystem") }
    Shortcut { sequence: "Ctrl+7"; context: Qt.ApplicationShortcut; onActivated: backend.setView("workspace") }
    Shortcut { sequence: "Ctrl+8"; context: Qt.ApplicationShortcut; onActivated: backend.setView("connections") }
    Shortcut { sequence: "Ctrl+Alt+M"; context: Qt.ApplicationShortcut; onActivated: backend.setView("memory") }
    Shortcut { sequence: "Ctrl+Alt+A"; context: Qt.ApplicationShortcut; onActivated: backend.setView("autopilot") }
    Shortcut { sequence: "Ctrl+9"; context: Qt.ApplicationShortcut; onActivated: backend.setView("ninerouter") }
    Shortcut { sequence: "Ctrl+Shift+M"; context: Qt.ApplicationShortcut; onActivated: backend.setView("mausbot") }
    Shortcut {
        sequence: "Ctrl+K"
        context: Qt.ApplicationShortcut
        enabled: !win.terminalFocused
        onActivated: commandPalette.opened ? commandPalette.close() : commandPalette.open()
    }
    Shortcut {
        sequence: "Ctrl+R"
        context: Qt.ApplicationShortcut
        enabled: !win.typing && !win.terminalFocused
        onActivated: backend.restartHarness()
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

        // ── header ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: theme.surface

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: (win.sidebarCompact ? 64 : 212) + 20
                anchors.rightMargin: 12
                spacing: 10

                ColumnLayout {
                    spacing: 1
                    Text {
                        text: backend.workspaceName
                        color: theme.brightText
                        font.family: "iA Writer Duospace"
                        font.pixelSize: 14
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.maximumWidth: 300
                        text: win.viewTitle + "  ·  " + qsTr("espace de travail")
                        color: theme.dimText
                        font.pixelSize: 8
                        elide: Text.ElideRight
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    visible: win.width >= 920
                    Layout.preferredWidth: Math.min(300, Math.max(180, win.width * 0.22))
                    Layout.preferredHeight: 30
                    radius: Math.min(8, theme.radius)
                    color: searchMouse.containsMouse ? theme.raisedHover : theme.raised
                    border.width: 1
                    border.color: searchMouse.containsMouse ? theme.borderStrong : theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 9
                        anchors.rightMargin: 8
                        spacing: 7
                        ThemedIcon { source: "icons/search.svg"; width: 12; height: 12; color: theme.dimText }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Rechercher ou exécuter…")
                            color: theme.dimText
                            font.pixelSize: 9
                            elide: Text.ElideRight
                        }
                        Text {
                            text: "Ctrl K"
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: 7
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

                IconButton {
                    visible: win.width < 920
                    icon: "icons/search.svg"
                    hint: qsTr("Command palette (Ctrl+K)")
                    onClicked: commandPalette.open()
                }
                IconButton {
                    icon: "icons/help.svg"
                    hint: qsTr("Keyboard shortcuts (?)")
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
                Layout.leftMargin: win.sidebarCompact ? 64 : 212

                RowLayout {
                    anchors.fill: parent
                    spacing: 0
                    visible: backend.view !== "settings" && backend.view !== "home"
                             && backend.view !== "terminal" && backend.view !== "ecosystem"
                             && backend.view !== "workspace" && backend.view !== "connections"
                             && backend.view !== "memory" && backend.view !== "autopilot"
                             && backend.view !== "ninerouter" && backend.view !== "mausbot"

                    HarnessPane {
                        id: harnessPane
                        visible: backend.view === "harness"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }

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

                MemoryPane {
                    anchors.fill: parent
                    visible: backend.view === "memory"
                }

                AutopilotPane {
                    anchors.fill: parent
                    visible: backend.view === "autopilot"
                }

                NineRouterPane {
                    anchors.fill: parent
                    visible: backend.view === "ninerouter"
                }

                MausBotPane {
                    anchors.fill: parent
                    visible: backend.view === "mausbot"
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
            Layout.preferredHeight: 28
            color: theme.surface

            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: (win.sidebarCompact ? 64 : 212) + 14
                anchors.rightMargin: 14
                spacing: 10

                Text {
                    text: backend.workspace
                    font.family: backend.monoFont
                    font.pixelSize: 9
                    color: theme.dimText
                    elide: Text.ElideMiddle
                    Layout.maximumWidth: win.width * 0.42
                }

                Rectangle {
                    visible: backend.gitAvailable
                    width: 1
                    height: 12
                    color: theme.border
                }
                Row {
                    spacing: 4
                    visible: backend.gitAvailable
                    ThemedIcon { source: "icons/branch.svg"; width: 10; height: 10; anchors.verticalCenter: parent.verticalCenter; color: backend.gitDirtyCount > 0 ? theme.orange : theme.green }
                    Text { text: backend.gitBranch; color: theme.mutedText; font.family: backend.monoFont; font.pixelSize: 9 }
                }

                Item { Layout.fillWidth: true }

                Repeater {
                    model: [
                        { label: "Harness", color: win.stateColor(backend.harnessState) },
                        { label: "Terminal", color: terminal.running ? theme.green : theme.dimText },
                        { label: "9Router", color: nineRouter.running ? theme.green : (nineRouter.starting ? theme.yellow : theme.dimText) },
                        { label: "MausBot", color: mausBot.running ? theme.green : (mausBot.starting ? theme.yellow : theme.dimText) }
                    ]
                    Row {
                        spacing: 5
                        StatusDot { anchors.verticalCenter: parent.verticalCenter; size: 4; color: modelData.color }
                        Text { visible: win.width >= 900; text: modelData.label; color: theme.dimText; font.pixelSize: 9 }
                    }
                }

            }
        }
    }

    AppSidebar {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: win.sidebarCompact ? 64 : 212
        compact: win.sidebarCompact
        z: 40
        onToggleCompact: win.sidebarUserCollapsed = !win.sidebarUserCollapsed
    }

    Timer {
        id: toastTimer
        interval: 3600
        onTriggered: win.toastText = ""
    }

    Connections {
        target: backend
        function onToast(message) { win.showToast(message) }

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

                    Button {
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
