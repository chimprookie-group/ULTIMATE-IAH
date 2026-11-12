import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    signal openSessions()
    signal openPalette()
    signal promptRequested(string prompt)
    signal commandRequested(string command)
    signal modelPickerRequested()

    readonly property int contentWidth: Math.max(320, Math.min(1080, width - 56))
    readonly property bool wide: contentWidth >= 860

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

    function sendPrompt() {
        const text = promptInput.text.trim()
        if (text === "")
            return
        root.promptRequested(text)
        promptInput.text = ""
    }

    function refresh() {
        backend.refreshGit()
        if (backend.opencodeState === "ready")
            backend.refreshSessions()
    }

    Component.onCompleted: {
        if (visible) {
            reveal = 1
            Qt.callLater(refresh)
        }
    }
    onVisibleChanged: {
        if (visible) {
            reveal = 1
            refresh()
        }
    }

    NumberAnimation on reveal {
        from: 0
        to: 1
        duration: 180
        easing.type: Easing.OutCubic
        running: true
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            id: content
            width: root.contentWidth
            x: Math.max(20, (scroll.availableWidth - width) / 2)
            spacing: 18
            opacity: root.reveal
            transform: Translate { y: (1 - root.reveal) * 8 }

            // Header
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 26
                spacing: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: qsTr("Cockpit")
                        color: theme.brightText
                        font.family: "iA Writer Duospace"
                        font.pixelSize: 31
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Vos agents, votre projet et vos intégrations dans un seul geste.")
                        color: theme.mutedText
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                    }
                }

                Row {
                    spacing: 8
                    Repeater {
                        model: [
                            { label: qsTr("Harness"), state: backend.harnessState },
                            { label: "OpenCode", state: backend.opencodeState },
                            { label: "9Router", state: nineRouter.running ? "running" : (nineRouter.starting ? "starting" : "stopped") },
                            { label: "MausBot", state: mausBot.running ? "running" : (mausBot.starting ? "starting" : "stopped") }
                        ]
                        Row {
                            spacing: 4
                            StatusDot { anchors.verticalCenter: parent.verticalCenter; size: 5; color: root.stateColor(modelData.state) }
                            Text { visible: root.wide; text: modelData.label; color: theme.dimText; font.pixelSize: 9 }
                        }
                    }
                }
            }

            // Command deck
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: commandBody.implicitHeight + 32
                radius: Math.min(14, theme.radius)
                color: theme.raised
                border.width: 1
                border.color: promptInput.activeFocus ? theme.borderStrong : theme.border

                ColumnLayout {
                    id: commandBody
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            width: 7
                            height: 7
                            radius: 4
                            color: root.stateColor(backend.opencodeState)
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Text {
                            text: "OPENCODE"
                            color: theme.dimText
                            font.pixelSize: 8
                            font.letterSpacing: 1
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: backend.currentModel !== "" ? backend.currentModel : qsTr("aucun modèle")
                                  + "  ·  " + (backend.currentMode !== "" ? backend.currentMode : qsTr("aucun mode"))
                            color: theme.mutedText
                            font.family: backend.monoFont
                            font.pixelSize: 8
                            elide: Text.ElideRight
                        }
                        Text {
                            text: backend.sessionTitle !== "" ? backend.sessionTitle : qsTr("Nouvelle session")
                            color: theme.dimText
                            font.pixelSize: 8
                            elide: Text.ElideRight
                            Layout.maximumWidth: 220
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 58

                        Text {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.topMargin: 2
                            visible: promptInput.text === ""
                            text: qsTr("Demandez à OpenCode…")
                            color: theme.dimText
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                        }

                        TextArea {
                            id: promptInput
                            anchors.fill: parent
                            color: theme.text
                            placeholderText: ""
                            font.pixelSize: 13
                            wrapMode: TextArea.Wrap
                            background: null
                            selectByMouse: true
                            Keys.onPressed: function (event) {
                                if (event.key === Qt.Key_Return && !(event.modifiers & Qt.ShiftModifier)) {
                                    root.sendPrompt()
                                    event.accepted = true
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 5

                        Button {
                            text: qsTr("Nouvelle session")
                            flat: true
                            font.pixelSize: 9
                            onClicked: {
                                backend.newSession()
                                backend.setView("opencode")
                            }
                        }
                        Button {
                            text: qsTr("Changer de modèle")
                            flat: true
                            font.pixelSize: 9
                            onClicked: root.modelPickerRequested()
                        }
                        Button {
                            text: qsTr("Session complète")
                            flat: true
                            font.pixelSize: 9
                            onClicked: backend.setView("opencode")
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            visible: backend.busy
                            text: qsTr("OpenCode travaille…")
                            color: theme.yellow
                            font.pixelSize: 8
                        }
                        ActionButton {
                            text: qsTr("Envoyer")
                            onClicked: root.sendPrompt()
                        }
                    }
                }
            }

            // Asymmetric workspace
            GridLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 26
                columns: root.wide ? 2 : 1
                columnSpacing: 26
                rowSpacing: 18

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: 18

                    // Project
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 7

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: qsTr("Projet")
                                color: theme.brightText
                                font.family: "iA Writer Duospace"
                                font.pixelSize: 16
                                font.weight: Font.Bold
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: backend.gitAvailable
                                      ? (backend.gitBranch + "  ·  " + (backend.gitDirtyCount === 0 ? qsTr("propre") : qsTr("%n changement(s)", "", backend.gitDirtyCount)))
                                      : qsTr("hors dépôt git")
                                color: backend.gitDirtyCount > 0 ? theme.orange : theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: 8
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: theme.border
                        }

                        Text {
                            Layout.fillWidth: true
                            text: backend.workspace
                            color: theme.mutedText
                            font.family: backend.monoFont
                            font.pixelSize: 9
                            elide: Text.ElideMiddle
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Button { text: qsTr("Écosystème"); flat: true; font.pixelSize: 9; onClicked: backend.setView("ecosystem") }
                            Button { text: qsTr("Fichiers"); flat: true; font.pixelSize: 9; onClicked: backend.setView("workspace") }
                            Button { text: "Git diff"; flat: true; font.pixelSize: 9; onClicked: root.commandRequested("git diff") }
                            Button { text: qsTr("Changer"); flat: true; font.pixelSize: 9; onClicked: backend.pickWorkspace() }
                        }
                    }

                    // Sessions
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 7

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: qsTr("Sessions récentes")
                                color: theme.brightText
                                font.family: "iA Writer Duospace"
                                font.pixelSize: 16
                                font.weight: Font.Bold
                            }
                            Item { Layout.fillWidth: true }
                            Button { text: qsTr("Tout voir"); flat: true; font.pixelSize: 9; onClicked: root.openSessions() }
                        }
                        Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

                        Repeater {
                            model: backend.sessions
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                visible: index < 3
                                height: 38
                                radius: Math.min(8, theme.radius)
                                color: sessionHover.hovered ? theme.chip : "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 6
                                    anchors.rightMargin: 6
                                    spacing: 8
                                    Text {
                                        Layout.fillWidth: true
                                        text: backend.sessionLabel(modelData)
                                        color: theme.text
                                        font.pixelSize: 10
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: modelData.updatedAt !== undefined ? Qt.formatDateTime(new Date(modelData.updatedAt), "d MMM · HH:mm") : ""
                                        color: theme.dimText
                                        font.pixelSize: 8
                                    }
                                }
                                HoverHandler { id: sessionHover }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        backend.loadSession(modelData.sessionId)
                                        backend.setView("opencode")
                                    }
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: backend.sessions.length === 0
                            text: backend.opencodeState === "ready" ? qsTr("Aucune session pour ce projet.") : qsTr("OpenCode démarre pour charger les sessions.")
                            color: theme.dimText
                            font.pixelSize: 9
                        }
                    }
                }

                // Services spine
                Rectangle {
                    Layout.preferredWidth: root.wide ? 316 : root.contentWidth
                    Layout.fillWidth: !root.wide
                    Layout.alignment: Qt.AlignTop
                    implicitHeight: servicesBody.implicitHeight + 28
                    radius: Math.min(14, theme.radius)
                    color: theme.surface
                    border.width: 1
                    border.color: theme.border

                    ColumnLayout {
                        id: servicesBody
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.bottomMargin: 8
                            Text {
                                text: qsTr("Services")
                                color: theme.brightText
                                font.family: "iA Writer Duospace"
                                font.pixelSize: 16
                                font.weight: Font.Bold
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: qsTr("LOCAL")
                                color: theme.dimText
                                font.pixelSize: 7
                                font.letterSpacing: 1
                            }
                        }

                        ServiceRow {
                            Layout.fillWidth: true
                            name: qsTr("Harness")
                            detail: backend.harnessDetail
                            stateColor: root.stateColor(backend.harnessState)
                            actionLabel: backend.harnessState === "running" ? qsTr("Ouvrir") : qsTr("Démarrer")
                            actionEnabled: backend.harnessState !== "starting"
                            onAction: {
                                if (backend.harnessState === "running")
                                    backend.setView("harness")
                                else
                                    backend.restartHarness()
                            }
                        }
                        ServiceRow {
                            Layout.fillWidth: true
                            name: "OpenCode"
                            detail: backend.opencodeState === "ready" ? (backend.currentModel !== "" ? backend.currentModel : qsTr("prêt")) : backend.opencodeDetail
                            stateColor: root.stateColor(backend.opencodeState)
                            actionLabel: backend.opencodeState === "ready" ? qsTr("Ouvrir") : qsTr("Démarrer")
                            actionEnabled: backend.opencodeState !== "starting"
                            onAction: {
                                if (backend.opencodeState === "ready")
                                    backend.setView("opencode")
                                else
                                    backend.restartOpencode()
                            }
                        }
                        ServiceRow {
                            Layout.fillWidth: true
                            name: "9Router"
                            detail: nineRouter.running ? nineRouter.detail : (nineRouter.installed ? qsTr("installé, arrêté") : qsTr("non installé"))
                            stateColor: nineRouter.running ? theme.green : (nineRouter.starting ? theme.yellow : theme.dimText)
                            actionLabel: nineRouter.running ? qsTr("Ouvrir") : qsTr("Démarrer")
                            actionEnabled: !nineRouter.starting && !nineRouter.installing
                            onAction: {
                                backend.setView("ninerouter")
                                if (!nineRouter.running)
                                    nineRouter.start()
                            }
                        }
                        ServiceRow {
                            Layout.fillWidth: true
                            name: "MausBot"
                            detail: mausBot.running ? mausBot.detail : (mausBot.installed ? qsTr("installé, arrêté") : qsTr("non installé"))
                            stateColor: mausBot.running ? theme.green : (mausBot.starting ? theme.yellow : theme.dimText)
                            actionLabel: mausBot.running ? qsTr("Ouvrir") : qsTr("Démarrer")
                            actionEnabled: !mausBot.starting && !mausBot.installing
                            onAction: {
                                backend.setView("mausbot")
                                if (!mausBot.running)
                                    mausBot.start()
                            }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.topMargin: 8; Layout.bottomMargin: 6; height: 1; color: theme.border }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            Button { text: qsTr("Providers"); flat: true; font.pixelSize: 9; onClicked: backend.setView("providers") }
                            Button { text: qsTr("Terminal"); flat: true; font.pixelSize: 9; onClicked: backend.setView("terminal") }
                            Item { Layout.fillWidth: true }
                            Button { text: qsTr("Palette"); flat: true; font.pixelSize: 9; onClicked: root.openPalette() }
                        }
                    }
                }
            }
        }
    }
}
