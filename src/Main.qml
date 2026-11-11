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
    title: backend.demo ? qsTr("DeepCrasy H") : "DeepCrasy H — " + backend.workspaceName
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

    property bool helpVisible: false
    property bool quitConfirm: false
    property string toastText: ""
    // True while a text field has focus. "?" and "Q" are ordinary characters
    // there, so their shortcuts must stand down.
    property bool typing: false
    property bool forceQuit: false
    property bool terminalFocused: false

    // Split only works when both panes get a usable width. In a tiled
    // half-screen window it would squeeze the chat below 400 px, so fall back
    // to the chat and say why.
    readonly property bool compact: width < 1180

    function showToast(message) {
        win.toastText = message
        toastTimer.restart()
    }

    function requestQuit() {
        if (backend.busy && !win.forceQuit)
            win.quitConfirm = true
        else
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
        add(qsTr("Navigation"), qsTr("Harness"), "Ctrl+1", "view:harness", "icons/harness.svg")
        add(qsTr("Navigation"), "OpenCode", "Ctrl+2", "view:opencode", "icons/chat.svg")
        add(qsTr("Navigation"), qsTr("Terminal"), "Ctrl+5", "view:terminal", "icons/terminal.svg")
        add(qsTr("Navigation"), qsTr("Split"), "Ctrl+3", "view:split", "icons/split.svg")
        add(qsTr("Navigation"), qsTr("Settings"), "Ctrl+4", "view:settings", "icons/gear.svg")

        add(qsTr("Actions"), qsTr("New OpenCode session"), "Ctrl+N", "action:new-session", "icons/plus.svg")
        add(qsTr("Actions"), qsTr("Restart the harness"), "Ctrl+R", "action:restart-harness", "icons/reload.svg")
        add(qsTr("Actions"), qsTr("Restart OpenCode"), "", "action:restart-opencode", "icons/reload.svg")
        add(qsTr("Actions"), qsTr("Change workspace"), "", "action:workspace", "icons/folder.svg")
        add(qsTr("Actions"), qsTr("Run diagnostics"), "", "action:diagnostics", "icons/plug.svg")
        add(qsTr("Actions"), qsTr("MCP servers"), "", "action:mcp", "icons/plug.svg")
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

        for (let i = 0; i < Math.min(6, backend.sessions.length); ++i) {
            const session = backend.sessions[i]
            const title = backend.sessionLabel(session)
            const date = session.updatedAt !== undefined
                ? Qt.formatDateTime(new Date(session.updatedAt), "d MMM")
                : ""
            add(qsTr("Sessions"), title, date, "session:" + session.sessionId, "icons/history.svg")
        }

        return list
    }

    function runCommand(id) {
        if (id.indexOf("view:") === 0) {
            backend.setView(id.substring(5))
        } else if (id.indexOf("theme:") === 0) {
            backend.setTheme(id.substring(6))
        } else if (id.indexOf("lang:") === 0) {
            backend.setLanguage(id.substring(5))
        } else if (id.indexOf("session:") === 0) {
            backend.loadSession(id.substring(8))
            backend.setView("opencode")
        } else if (id === "action:new-session") {
            backend.newSession()
            backend.setView("opencode")
        } else if (id === "action:restart-harness") {
            backend.restartHarness()
        } else if (id === "action:restart-opencode") {
            backend.restartOpencode()
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

    // One WebEngineView and one ChatPane live for the whole session, so the
    // harness keeps its page and OpenCode keeps its transcript when the user
    // moves between single and split layouts.

    Shortcut { sequence: "Ctrl+0"; context: Qt.ApplicationShortcut; onActivated: backend.setView("home") }
    Shortcut { sequence: "Ctrl+1"; context: Qt.ApplicationShortcut; onActivated: backend.setView("harness") }
    Shortcut { sequence: "Ctrl+2"; context: Qt.ApplicationShortcut; onActivated: backend.setView("opencode") }
    Shortcut { sequence: "Ctrl+3"; context: Qt.ApplicationShortcut; onActivated: backend.setView("split") }
    Shortcut { sequence: "Ctrl+4"; context: Qt.ApplicationShortcut; onActivated: backend.setView("settings") }
    Shortcut { sequence: "Ctrl+5"; context: Qt.ApplicationShortcut; onActivated: backend.setView("terminal") }
    Shortcut {
        sequence: "Ctrl+N"
        context: Qt.ApplicationShortcut
        enabled: !win.terminalFocused
        onActivated: backend.newSession()
    }
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
    Shortcut {
        sequence: "Ctrl+L"
        context: Qt.ApplicationShortcut
        enabled: !win.terminalFocused
        onActivated: chatPane.focusComposer()
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
        enabled: win.helpVisible || sessionsPanel.opened || win.quitConfirm
                 || commandPalette.opened || backend.permission.requestId !== undefined
        onActivated: {
            if (commandPalette.opened)
                commandPalette.close()
            else if (backend.permission.requestId !== undefined)
                backend.respondPermission("")
            else if (win.helpVisible)
                win.helpVisible = false
            else if (sessionsPanel.opened)
                sessionsPanel.close()
            else if (win.quitConfirm)
                win.quitConfirm = false
        }
    }

    Component.onCompleted: {
        if (Qt.application.arguments.indexOf("--palette") !== -1)
            Qt.callLater(function () { commandPalette.open() })
        if (Qt.application.arguments.indexOf("--auth-demo") !== -1)
            Qt.callLater(function () { mcp.authenticate("crasy-remote", "global") })
    }

    onCompactChanged: {
        if (compact && backend.view === "split") {
            backend.setView("opencode")
            showToast(qsTr("Split view needs a window at least 1180 px wide."))
        }
    }

    onClosing: function (close) {
        if (backend.busy && !win.forceQuit) {
            close.accepted = false
            win.quitConfirm = true
        }
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
                anchors.leftMargin: 16
                anchors.rightMargin: 12
                spacing: 12

                Item {
                    width: 30
                    height: 30
                    Rectangle {
                        anchors.centerIn: parent
                        width: 44
                        height: 44
                        radius: 22
                        color: theme.accent
                        opacity: theme.dark ? 0.13 : 0.07
                    }
                    Image {
                        anchors.centerIn: parent
                        source: "assets/icon-128.png"
                        sourceSize.width: 27
                        sourceSize.height: 27
                    }
                }

                ColumnLayout {
                    spacing: 0
                    RowLayout {
                        spacing: 4
                        Text {
                            text: "DeepCrasy"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: theme.text
                        }
                        Text {
                            text: "H"
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            color: theme.cyan
                        }
                    }
                    Text {
                        text: qsTr("HARNESS × OPENCODE")
                        font.pixelSize: 8
                        font.letterSpacing: 1.5
                        color: theme.dimText
                    }
                }

                Rectangle {
                    Layout.leftMargin: 4
                    Layout.rightMargin: 4
                    width: 1
                    height: 22
                    color: theme.border
                }

                Rectangle {
                    id: workspaceChip
                    Layout.preferredHeight: 28
                    implicitWidth: chipRow.implicitWidth + 18
                    color: chipMouse.containsMouse ? theme.raisedHover : theme.raised
                    border.width: 1
                    border.color: theme.border
                    radius: Math.min(7, theme.radius)

                    Row {
                        id: chipRow
                        anchors.centerIn: parent
                        spacing: 7
                        ThemedIcon {
                            source: "icons/folder.svg"
                            width: 13
                            height: 13
                            anchors.verticalCenter: parent.verticalCenter
                            color: theme.mutedText
                        }
                        Text {
                            text: backend.workspaceName
                            font.pixelSize: 12
                            color: theme.text
                            width: Math.min(implicitWidth, 200)
                            elide: Text.ElideRight
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        ThemedIcon {
                            source: "icons/chevron.svg"
                            width: 11
                            height: 11
                            anchors.verticalCenter: parent.verticalCenter
                            color: theme.dimText
                        }
                    }

                    MouseArea {
                        id: chipMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.pickWorkspace()
                    }
                    ToolTip.visible: chipMouse.containsMouse
                    ToolTip.text: backend.workspace
                    ToolTip.delay: 600
                }

                Item { Layout.fillWidth: true }

                IconButton {
                    icon: "icons/search.svg"
                    hint: qsTr("Command palette (Ctrl+K)")
                    onClicked: commandPalette.open()
                }
                IconButton {
                    icon: "icons/split.svg"
                    hint: qsTr("Split view (Ctrl+3)")
                    active: backend.view === "split"
                    onClicked: {
                        if (win.compact) {
                            win.showToast(qsTr("Split view needs a window at least 1180 px wide."))
                            return
                        }
                        backend.setView(backend.view === "split" ? "opencode" : "split")
                    }
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

            Rectangle {
                Layout.preferredWidth: 58
                Layout.fillHeight: true
                color: theme.surface

                Rectangle {
                    anchors.right: parent.right
                    width: 1
                    height: parent.height
                    color: theme.border
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 2

                    RailButton {
                        icon: "icons/home.svg"
                        label: qsTr("Home")
                        hint: qsTr("Crasy Harness (Ctrl+0)")
                        active: backend.view === "home"
                        onClicked: backend.setView("home")
                    }
                    RailButton {
                        icon: "icons/harness.svg"
                        label: qsTr("Harness")
                        hint: qsTr("DeepSeek Harness (Ctrl+1)")
                        active: backend.view === "harness"
                        onClicked: backend.setView("harness")
                    }
                    RailButton {
                        icon: "icons/chat.svg"
                        label: qsTr("OpenCode")
                        hint: qsTr("OpenCode chat (Ctrl+2)")
                        active: backend.view === "opencode"
                        onClicked: backend.setView("opencode")
                    }
                    RailButton {
                        icon: "icons/terminal.svg"
                        label: qsTr("Terminal")
                        hint: qsTr("Terminal (Ctrl+5)")
                        active: backend.view === "terminal"
                        onClicked: backend.setView("terminal")
                    }
                    RailButton {
                        icon: "icons/split.svg"
                        label: qsTr("Split")
                        hint: qsTr("Both panes (Ctrl+3)")
                        active: backend.view === "split"
                        onClicked: {
                            if (win.compact) {
                                win.showToast(qsTr("Split view needs a window at least 1180 px wide."))
                                return
                            }
                            backend.setView("split")
                        }
                    }

                    Item { Layout.fillHeight: true }

                    RailButton {
                        icon: "icons/gear.svg"
                        label: qsTr("Settings")
                        hint: qsTr("Workspace and engines (Ctrl+4)")
                        active: backend.view === "settings"
                        onClicked: backend.setView("settings")
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    anchors.fill: parent
                    spacing: 0
                    visible: backend.view !== "settings" && backend.view !== "home" && backend.view !== "terminal"

                    HarnessPane {
                        id: harnessPane
                        visible: backend.view === "harness" || backend.view === "split"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: backend.view === "split" ? win.width * backend.splitRatio : win.width
                    }

                    Rectangle {
                        visible: backend.view === "split"
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true
                        color: theme.border
                    }

                    ChatPane {
                        id: chatPane
                        visible: backend.view === "opencode" || backend.view === "split"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: backend.view === "split" ? win.width * (1 - backend.splitRatio) : win.width
                        onOpenSessions: sessionsPanel.open()
                    }
                }

                TerminalPage {
                    anchors.fill: parent
                    visible: backend.view === "terminal"
                }

                HomePane {
                    anchors.fill: parent
                    visible: backend.view === "home"
                    onOpenSessions: sessionsPanel.open()
                    onOpenPalette: commandPalette.open()
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
            Layout.preferredHeight: 27
            color: theme.surface

            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 14

                Text {
                    Layout.fillWidth: true
                    text: backend.workspace
                    font.family: backend.monoFont
                    font.pixelSize: 10
                    color: theme.dimText
                    elide: Text.ElideMiddle
                }

                Row {
                    spacing: 5
                    StatusDot {
                        anchors.verticalCenter: parent.verticalCenter
                        color: win.stateColor(backend.harnessState)
                    }
                    Text {
                        text: qsTr("Harness")
                        font.pixelSize: 10
                        color: theme.mutedText
                    }
                }

                Row {
                    spacing: 5
                    StatusDot {
                        anchors.verticalCenter: parent.verticalCenter
                        color: win.stateColor(backend.opencodeState)
                    }
                    Text {
                        text: qsTr("OpenCode")
                        font.pixelSize: 10
                        color: theme.mutedText
                    }
                }

                Text {
                    visible: backend.currentModel !== ""
                    Layout.maximumWidth: 220
                    text: backend.currentModel
                    font.family: backend.monoFont
                    font.pixelSize: 10
                    color: theme.dimText
                    elide: Text.ElideRight
                }
            }
        }
    }

    Timer {
        id: toastTimer
        interval: 3600
        onTriggered: win.toastText = ""
    }

    Connections {
        target: backend
        function onToast(message) { win.showToast(message) }

        function onBusyChanged() {
            if (!backend.busy && backend.notifyOnFinish && backend.sessionTitle !== "" && !win.active)
                backend.notify(qsTr("OpenCode finished"), backend.sessionTitle)
        }
    }

    SessionsPanel {
        id: sessionsPanel
        anchors.fill: parent
    }

    PermissionDialog {
        anchors.fill: parent
        request: backend.permission
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
                    text: qsTr("OpenCode is working. Quitting now stops the answer where it is.")
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
