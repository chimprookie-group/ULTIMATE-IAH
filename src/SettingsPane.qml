import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts


Item {
    id: root

    readonly property var systemPalette: {
        const themes = backend.themeOptions
        for (let i = 0; i < themes.length; ++i) {
            if (themes[i].id === backend.systemThemeId)
                return themes[i]
        }
        return {
            "background": theme.background,
            "foreground": theme.text,
            "accent": theme.accent,
            "cyan": theme.cyan,
            "red": theme.red,
            "green": theme.green
        }
    }

    function refresh() {
        backend.refreshThemes()
        backend.runDiagnostics()
        backend.refreshGit()
        mcp.refresh()
    }

    function mcpColor(status) {
        if (status === "connected")
            return theme.green
        if (status === "failed")
            return theme.red
        if (status === "disabled")
            return theme.dimText
        return theme.yellow
    }

    function mcpLabel(status) {
        if (status === "connected")
            return qsTr("Connected")
        if (status === "failed")
            return qsTr("Failed")
        if (status === "pending")
            return qsTr("Pending")
        if (status === "disabled")
            return qsTr("Disabled")
        return qsTr("Not tested")
    }

    Component.onCompleted: if (visible) refresh()
    onVisibleChanged: if (visible) refresh()

    property string mcpType: "local"
    property string mcpScope: "global"

    readonly property var providers: {
        const options = backend.modelOptions
        const counts = {}
        for (let i = 0; i < options.length; ++i) {
            const value = String(options[i].value !== undefined ? options[i].value : "")
            const provider = value.indexOf("/") > 0 ? value.slice(0, value.indexOf("/")) : "local"
            counts[provider] = (counts[provider] || 0) + 1
        }
        const list = []
        for (const provider in counts)
            list.push({"name": provider, "count": counts[provider]})
        list.sort(function (a, b) { return a.name.localeCompare(b.name) })
        return list
    }

    function healthColor() {
        if (backend.harnessHealth === "ok")
            return theme.green
        if (backend.harnessHealth === "failed")
            return theme.red
        if (backend.harnessHealth === "checking")
            return theme.yellow
        return theme.dimText
    }

    function healthLabel() {
        if (backend.harnessHealth === "ok")
            return qsTr("connected")
        if (backend.harnessHealth === "failed")
            return qsTr("unreachable")
        if (backend.harnessHealth === "checking")
            return qsTr("testing…")
        if (backend.harnessHealth === "offline")
            return qsTr("offline")
        return qsTr("not tested")
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

    ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Column {
            id: content
            width: Math.min(880, scroll.availableWidth - 64)
            x: Math.max(32, (scroll.availableWidth - width) / 2)
            topPadding: 24
            bottomPadding: 32
            spacing: 14

            RowLayout {
                width: content.width
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: qsTr("Settings")
                        font.pixelSize: 19
                        font.weight: Font.DemiBold
                        color: theme.text
                    }
                    Text {
                        text: qsTr("One workspace, two engines, nothing hidden.")
                        font.pixelSize: 12
                        color: theme.mutedText
                    }
                }

                ActionButton {
                    text: qsTr("Back to chat")
                    onClicked: backend.setView("opencode")
                }
            }

            SettingsSection {
                title: qsTr("Appearance")
                width: content.width

                Text {
                    Layout.fillWidth: true
                    text: qsTr("The app follows Omarchy's current theme. Pick another installed theme to use it here without changing the desktop.")
                    wrapMode: Text.Wrap
                    color: theme.mutedText
                    font.pixelSize: 11
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    ThemeCard {
                        themeId: ""
                        label: qsTr("Follow Omarchy")
                        mode: theme.dark ? "dark" : "light"
                        themeColors: root.systemPalette
                        active: backend.themePreference === ""
                        autoTheme: true
                        subtitle: qsTr("Current · %1").arg(backend.systemTheme)
                        onChosen: backend.setTheme("")
                    }

                    Repeater {
                        model: backend.themeOptions
                        ThemeCard {
                            themeId: modelData.id
                            label: modelData.label
                            mode: modelData.mode
                            themeColors: modelData
                            active: backend.themePreference === modelData.id
                            onChosen: backend.setTheme(modelData.id)
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Language")
                width: content.width

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Automatic follows Omarchy, then the language the embedded browser is using. A manual choice applies to DeepCrasy H only.")
                    wrapMode: Text.Wrap
                    color: theme.mutedText
                    font.pixelSize: 11
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: [
                            { "id": "", "label": qsTr("Auto") },
                            { "id": "fr", "label": "Français" },
                            { "id": "en", "label": "English" }
                        ]

                        Rectangle {
                            width: languageLabel.implicitWidth + 28
                            height: 30
                            radius: 15
                            color: backend.languagePreference === modelData.id ? theme.accent : theme.chip
                            border.width: 1
                            border.color: backend.languagePreference === modelData.id ? theme.accent : theme.border

                            Text {
                                id: languageLabel
                                anchors.centerIn: parent
                                text: modelData.label
                                color: backend.languagePreference === modelData.id ? theme.accentForeground : theme.text
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.setLanguage(modelData.id)
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }
                }
            }

            SettingsSection {
                title: qsTr("Options")
                width: content.width

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Startup view")
                    color: theme.text
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: [
                            { "id": "last", "label": qsTr("Last view") },
                            { "id": "home", "label": qsTr("Home") },
                            { "id": "opencode", "label": qsTr("Chat") },
                            { "id": "harness", "label": qsTr("Harness") },
                            { "id": "split", "label": qsTr("Split") },
                            { "id": "terminal", "label": qsTr("Terminal") },
                            { "id": "ecosystem", "label": qsTr("Ecosystem") },
                            { "id": "workspace", "label": qsTr("Workspace") }
                        ]

                        Rectangle {
                            width: startupLabel.implicitWidth + 24
                            height: 28
                            radius: 14
                            color: backend.startupView === modelData.id ? theme.accent : theme.chip
                            border.width: 1
                            border.color: backend.startupView === modelData.id ? theme.accent : theme.border

                            Text {
                                id: startupLabel
                                anchors.centerIn: parent
                                text: modelData.label
                                color: backend.startupView === modelData.id ? theme.accentForeground : theme.text
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.setStartupView(modelData.id)
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Last view remembers where you were. The other choices open there every time.")
                    wrapMode: Text.Wrap
                    color: theme.dimText
                    font.pixelSize: 10
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: theme.border
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            text: qsTr("Launch the harness on startup")
                            color: theme.text
                            font.pixelSize: 11
                        }
                        Text {
                            text: qsTr("Starts dsh web with the window.")
                            color: theme.dimText
                            font.pixelSize: 9
                        }
                    }
                    Switch {
                        checked: backend.autoStartHarness
                        onToggled: backend.setAutoStartHarness(checked)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            text: qsTr("Launch OpenCode on startup")
                            color: theme.text
                            font.pixelSize: 11
                        }
                        Text {
                            text: qsTr("Starts opencode acp with the window.")
                            color: theme.dimText
                            font.pixelSize: 9
                        }
                    }
                    Switch {
                        checked: backend.autoStartOpencode
                        onToggled: backend.setAutoStartOpencode(checked)
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: theme.border
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("Split ratio")
                        color: theme.text
                        font.pixelSize: 11
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: Math.round(backend.splitRatio * 100) + " %"
                        color: theme.mutedText
                        font.pixelSize: 10
                    }
                }
                Slider {
                    Layout.fillWidth: true
                    from: 0.45
                    to: 0.68
                    stepSize: 0.01
                    value: backend.splitRatio
                    onMoved: backend.setSplitRatio(value)
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            text: qsTr("Notify when a turn finishes")
                            color: theme.text
                            font.pixelSize: 11
                        }
                        Text {
                            text: qsTr("Desktop notification when the window is not focused.")
                            color: theme.dimText
                            font.pixelSize: 9
                        }
                    }
                    Switch {
                        checked: backend.notifyOnFinish
                        onToggled: backend.setNotifyOnFinish(checked)
                    }
                }
            }

            SettingsSection {
                title: qsTr("Work profiles")
                width: content.width

                Text {
                    Layout.fillWidth: true
                    text: qsTr("A profile remembers the current model and mode. Apply it before starting a task instead of re-selecting everything.")
                    wrapMode: Text.Wrap
                    color: theme.mutedText
                    font.pixelSize: 11
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    SettingsField {
                        id: profileName
                        Layout.fillWidth: true
                        placeholderText: qsTr("Profile name")
                    }
                    ActionButton {
                        text: qsTr("Save current profile")
                        onClicked: {
                            backend.saveProfile(profileName.text)
                            profileName.text = ""
                        }
                    }
                }

                Repeater {
                    model: backend.profiles

                    Rectangle {
                        Layout.fillWidth: true
                        height: 54
                        radius: Math.min(8, theme.radius)
                        color: backend.activeProfileId === modelData.id ? theme.chip : theme.background
                        border.width: 1
                        border.color: backend.activeProfileId === modelData.id ? theme.accent : theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    color: theme.text
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.description
                                    color: theme.dimText
                                    font.family: backend.monoFont
                                    font.pixelSize: 9
                                    elide: Text.ElideMiddle
                                }
                            }

                            Button {
                                text: qsTr("Apply")
                                flat: true
                                font.pixelSize: 9
                                onClicked: backend.applyProfile(modelData.id)
                            }
                            IconButton {
                                icon: "icons/trash.svg"
                                hint: qsTr("Delete this profile")
                                onClicked: backend.deleteProfile(modelData.id)
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: backend.profiles.length === 0
                    text: qsTr("No profile saved yet.")
                    color: theme.dimText
                    font.pixelSize: 9
                }
            }

            SettingsSection {
                title: qsTr("Connections")
                width: content.width

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        text: qsTr("Test the harness")
                        flat: true
                        onClicked: backend.testHarness()
                    }
                    Text {
                        text: root.healthLabel()
                        color: root.healthColor()
                        font.pixelSize: 10
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: qsTr("Open in browser")
                        flat: true
                        enabled: backend.harnessUrl !== ""
                        onClicked: Qt.openUrlExternally(backend.harnessUrl)
                    }
                }

                Repeater {
                    model: backend.diagnostics

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        StatusDot {
                            size: 4
                            color: modelData.ok ? theme.green : theme.red
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelData.label
                            color: theme.text
                            font.pixelSize: 10
                        }
                        Text {
                            Layout.maximumWidth: 340
                            text: modelData.detail
                            color: theme.dimText
                            font.pixelSize: 9
                            elide: Text.ElideRight
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: backend.diagnostics.length === 0
                    text: qsTr("No diagnostic yet.")
                    color: theme.mutedText
                    font.pixelSize: 10
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        text: qsTr("Run diagnostic")
                        flat: true
                        enabled: !backend.diagnosticsRunning
                        onClicked: backend.runDiagnostics()
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: qsTr("%1 checks").arg(backend.diagnostics.length)
                        color: theme.dimText
                        font.pixelSize: 9
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: theme.border
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("OpenCode providers")
                    color: theme.text
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: root.providers

                        Rectangle {
                            width: providerLabel.implicitWidth + 16
                            height: 24
                            radius: 12
                            color: theme.chip
                            border.width: 1
                            border.color: theme.border

                            Text {
                                id: providerLabel
                                anchors.centerIn: parent
                                text: modelData.name + " · " + modelData.count
                                color: theme.mutedText
                                font.pixelSize: 9
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: root.providers.length === 0
                    text: qsTr("No model loaded yet.")
                    color: theme.dimText
                    font.pixelSize: 9
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        text: qsTr("Restart the harness")
                        flat: true
                        onClicked: backend.restartHarness()
                    }
                    Button {
                        text: qsTr("Connect a provider")
                        flat: true
                        onClicked: {
                            const raw = backend.currentModel
                            const provider = raw.indexOf("/") > 0 ? raw.slice(0, raw.indexOf("/")).replace(/[^A-Za-z0-9._-]/g, "") : ""
                            const login = provider !== "" ? "opencode auth login " + provider : "opencode auth login"
                            terminal.run(login + " && opencode service restart")
                            backend.setView("terminal")
                        }
                    }
                    Button {
                        text: qsTr("Restart OpenCode")
                        flat: true
                        onClicked: backend.restartOpencode()
                    }
                    Item { Layout.fillWidth: true }
                }
            }

            SettingsSection {
                title: qsTr("MCP servers")
                width: content.width

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Tool servers exposed to OpenCode. Add a local command or a remote URL, then restart OpenCode to load it.")
                    wrapMode: Text.Wrap
                    color: theme.mutedText
                    font.pixelSize: 11
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: qsTr("Scope")
                        color: theme.mutedText
                        font.pixelSize: 9
                    }

                    Repeater {
                        model: [
                            { "id": "global", "label": qsTr("Global") },
                            { "id": "project", "label": qsTr("Project") }
                        ]

                        Rectangle {
                            width: scopeLabel.implicitWidth + 20
                            height: 26
                            radius: 13
                            color: root.mcpScope === modelData.id ? theme.accent : theme.chip
                            border.width: 1
                            border.color: root.mcpScope === modelData.id ? theme.accent : theme.border

                            Text {
                                id: scopeLabel
                                anchors.centerIn: parent
                                text: modelData.label
                                color: root.mcpScope === modelData.id ? theme.accentForeground : theme.text
                                font.pixelSize: 9
                                font.weight: Font.DemiBold
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.mcpScope = modelData.id
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        Layout.fillWidth: true
                        text: root.mcpScope === "project" ? mcp.projectConfigPath : mcp.configPath
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: 8
                        elide: Text.ElideMiddle
                    }
                    Text {
                        visible: mcp.loading
                        text: qsTr("Checking…")
                        color: theme.yellow
                        font.pixelSize: 9
                    }
                    Button {
                        text: qsTr("Refresh")
                        flat: true
                        enabled: !mcp.loading
                        onClicked: mcp.refresh()
                    }
                    Button {
                        text: qsTr("Restart OpenCode")
                        flat: true
                        onClicked: backend.restartOpencode()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    visible: mcp.authRunning || mcp.authMessage !== ""
                    Layout.preferredHeight: authColumn.implicitHeight + 18
                    radius: Math.min(8, theme.radius)
                    color: theme.chip
                    border.width: 1
                    border.color: mcp.authRunning ? theme.accent : theme.border

                    ColumnLayout {
                        id: authColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 9
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            StatusDot {
                                color: mcp.authRunning ? theme.yellow : (mcp.authOk ? theme.green : theme.red)
                            }
                            Text {
                                Layout.fillWidth: true
                                text: mcp.authRunning
                                      ? (mcp.authAction === "logout"
                                         ? qsTr("Logging out of %1…").arg(mcp.authName)
                                         : qsTr("Authenticating %1…").arg(mcp.authName))
                                      : mcp.authMessage
                                color: theme.text
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                            Button {
                                visible: mcp.authRunning
                                text: qsTr("Cancel")
                                flat: true
                                onClicked: mcp.cancelAuth()
                            }
                            Button {
                                visible: !mcp.authRunning
                                text: qsTr("Close")
                                flat: true
                                onClicked: mcp.clearAuth()
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: mcp.authRunning && mcp.authAction !== "logout"
                            text: qsTr("Finish the flow in your browser, then come back here.")
                            color: theme.mutedText
                            font.pixelSize: 9
                            wrapMode: Text.Wrap
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: mcp.authAction !== "logout" && mcp.authUrl !== ""
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                text: mcp.authUrl
                                color: theme.accent
                                font.family: backend.monoFont
                                font.pixelSize: 8
                                elide: Text.ElideMiddle
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: Qt.openUrlExternally(mcp.authUrl)
                                }
                            }
                            Button {
                                text: qsTr("Open")
                                flat: true
                                onClicked: Qt.openUrlExternally(mcp.authUrl)
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: !mcp.authRunning && !mcp.authOk && mcp.authDetail !== ""
                            text: mcp.authDetail
                            color: theme.red
                            font.family: backend.monoFont
                            font.pixelSize: 8
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            wrapMode: Text.WrapAnywhere
                        }
                    }
                }

                Repeater {
                    model: mcp.servers

                    Rectangle {
                        Layout.fillWidth: true
                        height: serverColumn.implicitHeight + 18
                        radius: Math.min(8, theme.radius)
                        color: theme.chip
                        border.width: 1
                        border.color: theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 9
                            spacing: 8

                            StatusDot {
                                color: root.mcpColor(modelData.status)
                            }

                            ColumnLayout {
                                id: serverColumn
                                Layout.fillWidth: true
                                spacing: 2

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Text {
                                        text: modelData.name
                                        color: theme.text
                                        font.pixelSize: 10
                                        font.weight: Font.DemiBold
                                    }
                                    Rectangle {
                                        width: typeLabel.implicitWidth + 12
                                        height: 16
                                        radius: 8
                                        color: theme.background
                                        border.width: 1
                                        border.color: theme.border
                                        Text {
                                            id: typeLabel
                                            anchors.centerIn: parent
                                            text: modelData.type === "remote" ? qsTr("Remote") : qsTr("Local")
                                            color: theme.mutedText
                                            font.pixelSize: 8
                                        }
                                    }
                                    Rectangle {
                                        width: scopeBadge.implicitWidth + 12
                                        height: 16
                                        radius: 8
                                        color: modelData.scope === "project" ? theme.accentSoft : theme.background
                                        border.width: 1
                                        border.color: modelData.scope === "project" ? theme.accent : theme.border
                                        Text {
                                            id: scopeBadge
                                            anchors.centerIn: parent
                                            text: modelData.scope === "project" ? qsTr("Project") : qsTr("Global")
                                            color: modelData.scope === "project" ? theme.accent : theme.mutedText
                                            font.pixelSize: 8
                                        }
                                    }
                                    Rectangle {
                                        visible: modelData.type === "remote"
                                        width: oauthBadge.implicitWidth + 12
                                        height: 16
                                        radius: 8
                                        color: theme.background
                                        border.width: 1
                                        border.color: root.mcpColor(modelData.auth !== undefined ? modelData.auth : "unknown")
                                        Text {
                                            id: oauthBadge
                                            anchors.centerIn: parent
                                            text: qsTr("OAuth")
                                            color: root.mcpColor(modelData.auth !== undefined ? modelData.auth : "unknown")
                                            font.pixelSize: 8
                                        }
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: root.mcpLabel(modelData.status)
                                        color: root.mcpColor(modelData.status)
                                        font.pixelSize: 8
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.target
                                    color: theme.mutedText
                                    font.family: backend.monoFont
                                    font.pixelSize: 8
                                    elide: Text.ElideMiddle
                                }

                                Text {
                                    Layout.fillWidth: true
                                    visible: modelData.detail !== undefined && modelData.detail !== ""
                                    text: modelData.detail !== undefined ? modelData.detail : ""
                                    color: theme.red
                                    font.pixelSize: 8
                                    elide: Text.ElideRight
                                }
                            }

                            IconButton {
                                visible: modelData.type === "remote"
                                icon: "icons/key.svg"
                                hint: qsTr("Authenticate with OAuth")
                                onClicked: mcp.authenticate(modelData.name, modelData.scope)
                            }

                            IconButton {
                                visible: modelData.type === "remote"
                                icon: "icons/logout.svg"
                                hint: qsTr("Log out of this MCP server")
                                onClicked: mcp.logoutServer(modelData.name, modelData.scope)
                            }

                            IconButton {
                                icon: "icons/trash.svg"
                                hint: qsTr("Remove this MCP server")
                                onClicked: mcp.removeServer(modelData.name, modelData.scope)
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: mcp.servers.length === 0
                    text: qsTr("No MCP server configured yet.")
                    color: theme.mutedText
                    font.pixelSize: 10
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: theme.border
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    SettingsField {
                        id: mcpName
                        Layout.preferredWidth: 170
                        placeholderText: qsTr("Server name")
                    }

                    RowLayout {
                        spacing: 4

                        Repeater {
                            model: [
                                { "id": "local", "label": qsTr("Local") },
                                { "id": "remote", "label": qsTr("Remote") }
                            ]

                            Rectangle {
                                width: mcpTypeLabel.implicitWidth + 20
                                height: 26
                                radius: 13
                                color: root.mcpType === modelData.id ? theme.accent : theme.chip
                                border.width: 1
                                border.color: root.mcpType === modelData.id ? theme.accent : theme.border

                                Text {
                                    id: mcpTypeLabel
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    color: root.mcpType === modelData.id ? theme.accentForeground : theme.text
                                    font.pixelSize: 9
                                    font.weight: Font.DemiBold
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.mcpType = modelData.id
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    SettingsField {
                        id: mcpTarget
                        Layout.fillWidth: true
                        placeholderText: root.mcpType === "remote"
                            ? qsTr("https://example.com/mcp")
                            : qsTr("npx -y @modelcontextprotocol/server-filesystem /home")
                    }
                    ActionButton {
                        text: qsTr("Add")
                        onClicked: mcp.addServer(mcpName.text, root.mcpType, mcpTarget.text, root.mcpScope)
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: mcp.error !== ""
                    text: mcp.error
                    color: theme.red
                    font.pixelSize: 9
                    wrapMode: Text.Wrap
                }
            }

            SettingsSection {
                title: qsTr("Workspace")
                width: content.width

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.fillWidth: true
                        text: backend.workspace
                        font.family: backend.monoFont
                        font.pixelSize: 11
                        color: theme.text
                        elide: Text.ElideMiddle
                    }
                    Button {
                        text: qsTr("Browse…")
                        onClicked: backend.pickWorkspace()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Both engines start in this folder. Changing it restarts the harness and opens a new OpenCode session there.")
                    wrapMode: Text.Wrap
                    color: theme.dimText
                    font.pixelSize: 10
                }
            }

            SettingsSection {
                title: qsTr("DeepSeek Harness")
                width: content.width

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.preferredWidth: 86
                        text: qsTr("Profile")
                        color: theme.mutedText
                        font.pixelSize: 11
                    }
                    TextField {
                        id: profileField
                        Layout.preferredWidth: 220
                        text: backend.harnessProfile
                        font.family: backend.monoFont
                        font.pixelSize: 11
                        color: theme.text
                        selectionColor: theme.accent
                        leftPadding: 9
                        rightPadding: 9
                        background: Rectangle {
                            implicitHeight: 32
                            radius: Math.min(6, theme.radius)
                            color: theme.background
                            border.width: 1
                            border.color: profileField.activeFocus ? theme.accent : theme.border
                        }
                        onEditingFinished: backend.setHarnessProfile(text)
                    }
                    Text {
                        text: qsTr("dsh profile booted, web by default")
                        color: theme.dimText
                        font.pixelSize: 10
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.preferredWidth: 86
                        text: qsTr("Binary")
                        color: theme.mutedText
                        font.pixelSize: 11
                    }
                    TextField {
                        id: dshField
                        Layout.fillWidth: true
                        text: backend.dshBinary
                        font.family: backend.monoFont
                        font.pixelSize: 11
                        color: theme.text
                        selectionColor: theme.accent
                        leftPadding: 9
                        rightPadding: 9
                        background: Rectangle {
                            implicitHeight: 32
                            radius: Math.min(6, theme.radius)
                            color: theme.background
                            border.width: 1
                            border.color: dshField.activeFocus ? theme.accent : theme.border
                        }
                        onEditingFinished: backend.setDshBinary(text)
                    }
                    Rectangle {
                        implicitWidth: dshBadge.implicitWidth + 14
                        height: 18
                        radius: 9
                        color: theme.background
                        border.width: 1
                        border.color: backend.dshAvailable ? theme.green : theme.red
                        Text {
                            id: dshBadge
                            anchors.centerIn: parent
                            text: backend.dshAvailable ? qsTr("found") : qsTr("missing")
                            color: backend.dshAvailable ? theme.green : theme.red
                            font.pixelSize: 9
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        width: 5
                        height: 5
                        radius: 2.5
                        color: root.stateColor(backend.harnessState)
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.harnessState + " · " + backend.harnessDetail
                        color: theme.mutedText
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                    Button {
                        text: qsTr("Restart")
                        onClicked: backend.restartHarness()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: backend.harnessState === "failed" && backend.harnessLog !== ""
                    text: backend.harnessLog.split("\n").slice(-10).join("\n")
                    font.family: backend.monoFont
                    font.pixelSize: 10
                    color: theme.dimText
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    maximumLineCount: 12
                    elide: Text.ElideRight
                }
            }

            SettingsSection {
                title: qsTr("OpenCode")
                width: content.width

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.preferredWidth: 86
                        text: qsTr("Binary")
                        color: theme.mutedText
                        font.pixelSize: 11
                    }
                    TextField {
                        id: opencodeField
                        Layout.fillWidth: true
                        text: backend.opencodeBinary
                        font.family: backend.monoFont
                        font.pixelSize: 11
                        color: theme.text
                        selectionColor: theme.accent
                        leftPadding: 9
                        rightPadding: 9
                        background: Rectangle {
                            implicitHeight: 32
                            radius: Math.min(6, theme.radius)
                            color: theme.background
                            border.width: 1
                            border.color: opencodeField.activeFocus ? theme.accent : theme.border
                        }
                        onEditingFinished: backend.setOpencodeBinary(text)
                    }
                    Rectangle {
                        implicitWidth: opencodeBadge.implicitWidth + 14
                        height: 18
                        radius: 9
                        color: theme.background
                        border.width: 1
                        border.color: backend.opencodeAvailable ? theme.green : theme.red
                        Text {
                            id: opencodeBadge
                            anchors.centerIn: parent
                            text: backend.opencodeAvailable ? qsTr("found") : qsTr("missing")
                            color: backend.opencodeAvailable ? theme.green : theme.red
                            font.pixelSize: 9
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        width: 5
                        height: 5
                        radius: 2.5
                        color: root.stateColor(backend.opencodeState)
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.opencodeState + " · " + backend.opencodeDetail
                        color: theme.mutedText
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                    Button {
                        text: qsTr("Restart")
                        onClicked: backend.restartOpencode()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Talked to over the Agent Client Protocol (opencode acp). Sessions, models, modes, tool calls, and permissions are native.")
                    wrapMode: Text.Wrap
                    color: theme.dimText
                    font.pixelSize: 10
                }
            }

            SettingsSection {
                title: qsTr("About")
                width: content.width

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        Layout.fillWidth: true
                        text: "DeepCrasy H " + backend.appVersion
                        color: theme.text
                        font.pixelSize: 12
                    }
                    Text {
                        text: qsTr("Built for Omarchy · MIT")
                        color: theme.dimText
                        font.pixelSize: 10
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Harness and OpenCode stay separate processes. DeepCrasy H only owns their lifecycle and the window around them.")
                    wrapMode: Text.Wrap
                    color: theme.dimText
                    font.pixelSize: 10
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Shortcuts: Ctrl+1 harness · Ctrl+2 chat · Ctrl+3 split · Ctrl+4 settings · Ctrl+L composer · ? help")
                    wrapMode: Text.Wrap
                    color: theme.dimText
                    font.pixelSize: 10
                }
            }
        }
    }
}
