import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    signal openPalette()
    signal commandRequested(string command)
    signal agentTaskRequested(string prompt)

    readonly property int contentWidth: Math.max(320, Math.min(design.contentMax, width - design.space7))
    readonly property bool wide: contentWidth >= 760
    property real reveal: 0

    function stateColor(state) {
        if (state === "running" || state === "ready")
            return theme.green
        if (state === "starting")
            return theme.yellow
        if (state === "failed")
            return theme.red
        return theme.dimText
    }

    function stateLabel(state) {
        if (state === "running" || state === "ready")
            return qsTr("actif")
        if (state === "starting")
            return qsTr("démarrage")
        if (state === "failed")
            return qsTr("erreur")
        return qsTr("arrêté")
    }

    function runTask() {
        const text = taskInput.text.trim()
        if (text === "")
            return
        root.agentTaskRequested(text)
        taskInput.text = ""
    }

    function refresh() {
        backend.refreshGit()
    }

    Component.onCompleted: if (visible) { reveal = 1; Qt.callLater(refresh) }
    onVisibleChanged: if (visible) { reveal = 1; refresh() }

    NumberAnimation on reveal {
        from: 0
        to: 1
        duration: design.durationNormal
        easing.type: Easing.OutCubic
        running: true
    }

    BrandMark {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: design.space6
        anchors.bottomMargin: design.space3
        width: Math.min(360, parent.width * 0.32)
        height: width * 0.85
        size: width
        markOpacity: 0.035
        accent: false
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            width: root.contentWidth
            x: Math.max(design.space5, (parent.width - width) / 2)
            spacing: design.space4
            opacity: root.reveal
            transform: Translate { y: (1 - root.reveal) * 8 }

            Item { Layout.fillWidth: true; Layout.preferredHeight: design.space4 }

            PageHeader {
                Layout.fillWidth: true
                title: qsTr("Cockpit")
                description: qsTr("Ultimate Agent, ses discussions, ses notes et votre projet.")
                icon: "icons/home.svg"
                brandMark: true

                RowLayout {
                    spacing: design.space3
                    Repeater {
                        model: [
                            { label: qsTr("Ultimate Agent"), state: "running" },
                            { label: qsTr("Terminal"), state: terminal.running ? "running" : "stopped" },
                            { label: "DeepSeek", state: connections.deepseekConfigured ? "running" : "stopped" },
                            { label: "MCP", state: mcpManager.servers.length > 0 ? "running" : "stopped" },
                            { label: qsTr("Bots"), state: bots.serverRunning ? "running" : "stopped" },
                            { label: qsTr("Notes"), state: noteStore.count > 0 ? "running" : "stopped" },
                            { label: qsTr("Plugins"), state: plugins.installedCount > 0 ? "running" : "stopped" }
                        ]
                        Row {
                            spacing: design.space2
                            StatusDot { anchors.verticalCenter: parent.verticalCenter; size: 5; color: root.stateColor(modelData.state) }
                            Text {
                                visible: root.wide
                                text: modelData.label
                                color: theme.mutedText
                                font.pixelSize: design.textCaption
                            }
                        }
                    }
                }
            }

            // ── Mission input ────────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: taskBody.implicitHeight + design.space6
                radius: design.radiusLg
                color: theme.raised
                border.width: 1
                border.color: taskInput.activeFocus ? theme.borderStrong : theme.border

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 3
                    color: theme.accent
                    radius: design.radiusLg
                }

                ColumnLayout {
                    id: taskBody
                    anchors.fill: parent
                    anchors.leftMargin: design.space4 + 3
                    anchors.rightMargin: design.space4
                    anchors.topMargin: design.space3
                    anchors.bottomMargin: design.space3
                    spacing: design.space2

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: design.space3
                        StatusDot { color: root.stateColor(backend.harnessState) }
                        Text {
                            text: qsTr("AGENT INPUT")
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textMicro
                            font.letterSpacing: 0.9
                        }
                        Text {
                            Layout.fillWidth: true
                            text: backend.harnessDetail
                            color: theme.mutedText
                            font.pixelSize: design.textCaption
                            elide: Text.ElideRight
                        }
                        Text {
                            text: qsTr("runtime natif")
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textMicro
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 58
                        Text {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.topMargin: design.space4
                            visible: taskInput.text === ""
                            text: qsTr("Décrivez une tâche pour Ultimate Agent…")
                            color: theme.dimText
                            font.family: "iA Writer Duospace"
                            font.pixelSize: 18
                            wrapMode: Text.Wrap
                        }
                        TextArea {
                            id: taskInput
                            anchors.fill: parent
                            topPadding: design.space4
                            bottomPadding: design.space2
                            leftPadding: 0
                            rightPadding: 0
                            color: theme.brightText
                            font.family: "iA Writer Duospace"
                            font.pixelSize: 18
                            wrapMode: TextArea.Wrap
                            background: null
                            selectByMouse: true
                            Keys.onPressed: function (event) {
                                if (event.key === Qt.Key_Return && !(event.modifiers & Qt.ShiftModifier)) {
                                    root.runTask()
                                    event.accepted = true
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: design.space2
                        Button { text: qsTr("Ouvrir Ultimate Agent"); flat: true; font.pixelSize: design.textCaption; onClicked: backend.setView("agent") }
                        Button { text: qsTr("Terminal agent"); flat: true; font.pixelSize: design.textCaption; onClicked: backend.setView("terminal") }
                        Item { Layout.fillWidth: true }
                        Text {
                            visible: root.wide
                            text: qsTr("Entrée pour lancer · Maj+Entrée pour une ligne")
                            color: theme.dimText
                            font.pixelSize: design.textMicro
                        }
                        ActionButton { text: qsTr("Exécuter"); onClicked: root.runTask() }
                    }
                }
            }

            // ── Operational grid ─────────────────────────────────────────
            GridLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: design.space7
                columns: root.wide ? 2 : 1
                columnSpacing: design.space4
                rowSpacing: design.space4

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: design.space4
                    Layout.preferredWidth: root.wide ? Math.floor((root.contentWidth - design.space4) * 0.62) : root.contentWidth

                    HomeCard {
                        Layout.fillWidth: true
                        title: qsTr("Projet")
                        icon: "icons/branch.svg"
                        cardWidth: parent.width

                        Text {
                            Layout.fillWidth: true
                            text: backend.workspace
                            color: theme.text
                            font.family: backend.monoFont
                            font.pixelSize: design.textCaption
                            elide: Text.ElideMiddle
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: design.space2
                            Text {
                                text: backend.gitAvailable ? backend.gitBranch : qsTr("hors dépôt git")
                                color: theme.mutedText
                                font.family: backend.monoFont
                                font.pixelSize: design.textMicro
                            }
                            Text {
                                visible: backend.gitAvailable
                                text: backend.gitDirtyCount > 0 ? qsTr("%n changement(s)", "", backend.gitDirtyCount) : qsTr("propre")
                                color: backend.gitDirtyCount > 0 ? theme.orange : theme.green
                                font.pixelSize: design.textMicro
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: "git"
                                color: theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: design.textMicro
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: design.space1
                            Button { text: qsTr("Projet"); flat: true; font.pixelSize: design.textCaption; onClicked: backend.setView("ecosystem") }
                            Button { text: qsTr("Fichiers"); flat: true; font.pixelSize: design.textCaption; onClicked: backend.setView("workspace") }
                            Button { text: "Git diff"; flat: true; font.pixelSize: design.textCaption; onClicked: root.commandRequested("git diff") }
                            Item { Layout.fillWidth: true }
                            Button { text: qsTr("Changer"); flat: true; font.pixelSize: design.textCaption; onClicked: backend.pickWorkspace() }
                        }
                    }

                    HomeCard {
                        Layout.fillWidth: true
                        title: qsTr("Terminal agent")
                        icon: "icons/terminal.svg"
                        cardWidth: parent.width

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Le shell réel du workspace, partagé avec Ultimate Agent et le projet.")
                            color: theme.mutedText
                            font.pixelSize: design.textCaption
                            lineHeight: 1.3
                            wrapMode: Text.Wrap
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: design.space2
                            Button { text: qsTr("Ouvrir le terminal"); flat: true; font.pixelSize: design.textCaption; onClicked: backend.setView("terminal") }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: terminal.running ? terminal.shell : qsTr("arrêté")
                                color: theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: design.textMicro
                            }
                        }
                    }

                    HomeCard {
                        Layout.fillWidth: true
                        title: qsTr("Couche agentique")
                        icon: "icons/agents.svg"
                        accent: theme.green
                        cardWidth: parent.width

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Missions orchestrées, mémoire persistante et tâches autonomes.")
                            color: theme.mutedText
                            font.pixelSize: design.textCaption
                            wrapMode: Text.Wrap
                        }
                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: design.space2
                            rowSpacing: design.space2

                            Repeater {
                                model: [
                                    { label: qsTr("Agents"), view: "agents" },
                                    { label: qsTr("Mémoire"), view: "memory" },
                                    { label: qsTr("Autopilote"), view: "autopilot" },
                                    { label: "MCP", view: "mcp" }
                                ]
                                Button {
                                    id: agentTile
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 30
                                    text: modelData.label
                                    padding: 0
                                    background: Rectangle {
                                        radius: design.radiusSm
                                        color: agentTile.hovered ? theme.raised : theme.chip
                                        border.width: agentTile.activeFocus ? 2 : 1
                                        border.color: agentTile.activeFocus ? theme.accent
                                                                           : (agentTile.hovered ? theme.borderStrong : theme.border)
                                    }
                                    contentItem: Text {
                                        text: agentTile.text
                                        color: theme.text
                                        font.pixelSize: design.textCaption
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: backend.setView(modelData.view)
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: design.space4
                    Layout.preferredWidth: root.wide ? Math.floor((root.contentWidth - design.space4) * 0.38) : root.contentWidth

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: servicesBody.implicitHeight + design.space5
                        radius: design.radiusLg
                        color: theme.surface
                        border.width: 1
                        border.color: theme.border

                        ColumnLayout {
                            id: servicesBody
                            anchors.fill: parent
                            anchors.margins: design.space4
                            spacing: 2

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.bottomMargin: design.space2
                                Text {
                                    text: qsTr("Composants actifs")
                                    color: theme.brightText
                                    font.family: "iA Writer Duospace"
                                    font.pixelSize: design.textTitle
                                    font.weight: Font.Bold
                                }
                                Item { Layout.fillWidth: true }
                                Button {
                                    text: qsTr("Tout voir")
                                    flat: true
                                    font.pixelSize: design.textCaption
                                    onClicked: backend.setView("ecosystem")
                                }
                                Text { text: qsTr("ACTIFS"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; font.letterSpacing: 0.9 }
                            }

                            ServiceRow {
                                Layout.fillWidth: true
                                name: qsTr("Ultimate Agent")
                                detail: backend.harnessDetail
                                stateColor: root.stateColor(backend.harnessState)
                                actionLabel: qsTr("Ouvrir")
                                actionEnabled: backend.harnessState !== "starting"
                                onAction: {
                                    if (backend.harnessState === "running") backend.setView("agent")
                                    else backend.restartHarness()
                                }
                            }
                            ServiceRow {
                                Layout.fillWidth: true
                                name: qsTr("Terminal agent")
                                detail: terminal.running ? (terminal.shell + " · " + backend.workspaceName) : qsTr("arrêté")
                                stateColor: terminal.running ? theme.green : theme.dimText
                                actionLabel: qsTr("Ouvrir")
                                onAction: backend.setView("terminal")
                            }
                            ServiceRow {
                                Layout.fillWidth: true
                                name: qsTr("DeepSeek API")
                                detail: connections.deepseekConfigured
                                        ? (connections.defaultModel !== "" ? connections.defaultModel : qsTr("clé configurée"))
                                        : qsTr("clé API manquante")
                                stateColor: connections.deepseekConfigured ? theme.green : theme.orange
                                actionLabel: qsTr("Ouvrir")
                                onAction: backend.setView("connections")
                            }
                            ServiceRow {
                                Layout.fillWidth: true
                                name: "MCP"
                                detail: mcpManager.builtins.length > 0
                                        ? qsTr("Mémoire intégrée · %n serveur(s) personnalisé(s)", "", mcpManager.servers.length)
                                        : qsTr("%n serveur(s) personnalisé(s)", "", mcpManager.servers.length)
                                stateColor: (mcpManager.builtins.length > 0 || mcpManager.servers.length > 0) ? theme.green : theme.dimText
                                actionLabel: qsTr("Ouvrir")
                                onAction: backend.setView("mcp")
                            }

                            ServiceRow {
                                Layout.fillWidth: true
                                name: qsTr("Bots")
                                detail: bots.serverRunning
                                        ? qsTr("%n bot(s) · service local", "", bots.bots.length)
                                        : qsTr("service arrêté")
                                stateColor: bots.serverRunning ? theme.green : theme.dimText
                                actionLabel: qsTr("Ouvrir")
                                onAction: backend.setView("bots")
                            }

                            ServiceRow {
                                Layout.fillWidth: true
                                name: qsTr("Notes")
                                detail: qsTr("%n note(s) · vault Markdown", "", noteStore.count)
                                stateColor: noteStore.count > 0 ? theme.yellow : theme.dimText
                                actionLabel: qsTr("Ouvrir")
                                onAction: backend.setView("notes")
                            }

                            ServiceRow {
                                Layout.fillWidth: true
                                name: qsTr("Plugins")
                                detail: qsTr("%n outil(s) · ArtCraft", "", plugins.toolCount)
                                stateColor: plugins.installedCount > 0 ? theme.green : theme.dimText
                                actionLabel: qsTr("Ouvrir")
                                onAction: backend.setView("plugins")
                            }

                            Rectangle { Layout.fillWidth: true; Layout.topMargin: design.space2; height: 1; color: theme.border }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: design.space1
                                Button { text: qsTr("Providers"); flat: true; font.pixelSize: design.textCaption; onClicked: backend.setView("connections") }
                                Button { text: qsTr("Réglages"); flat: true; font.pixelSize: design.textCaption; onClicked: backend.setView("settings") }
                                Item { Layout.fillWidth: true }
                                Button { text: qsTr("Palette"); flat: true; font.pixelSize: design.textCaption; onClicked: root.openPalette() }
                            }
                        }
                    }
                }
            }
        }
    }
}
