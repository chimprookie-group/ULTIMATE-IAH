import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    signal openPalette()
    signal commandRequested(string command)
    signal agentTaskRequested(string prompt)

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
                        text: qsTr("Le harness, son terminal agent et votre projet. Rien d'autre.")
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
                            { label: qsTr("Terminal"), state: terminal.running ? "running" : "stopped" },
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

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: taskBody.implicitHeight + 32
                radius: Math.min(14, theme.radius)
                color: theme.raised
                border.width: 1
                border.color: taskInput.activeFocus ? theme.borderStrong : theme.border

                ColumnLayout {
                    id: taskBody
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
                            color: root.stateColor(backend.harnessState)
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Text {
                            text: qsTr("HARNESS")
                            color: theme.dimText
                            font.pixelSize: 8
                            font.letterSpacing: 1
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: backend.harnessDetail
                            color: theme.mutedText
                            font.pixelSize: 8
                            elide: Text.ElideRight
                        }
                        Text {
                            text: qsTr("terminal agent")
                            color: theme.dimText
                            font.pixelSize: 8
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
                            visible: taskInput.text === ""
                            text: qsTr("Décrivez une tâche pour le harness…")
                            color: theme.dimText
                            font.pixelSize: 13
                            wrapMode: Text.Wrap
                        }

                        TextArea {
                            id: taskInput
                            anchors.fill: parent
                            color: theme.text
                            placeholderText: ""
                            font.pixelSize: 13
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
                        spacing: 5
                        Button { text: qsTr("Ouvrir le harness"); flat: true; font.pixelSize: 9; onClicked: backend.setView("harness") }
                        Button { text: qsTr("Terminal agent"); flat: true; font.pixelSize: 9; onClicked: backend.setView("terminal") }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: qsTr("dsh headless · workspace")
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: 8
                        }
                        ActionButton {
                            text: qsTr("Exécuter")
                            onClicked: root.runTask()
                        }
                    }
                }
            }

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
                        Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }
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
                            Button { text: qsTr("Projet"); flat: true; font.pixelSize: 9; onClicked: backend.setView("ecosystem") }
                            Button { text: qsTr("Fichiers"); flat: true; font.pixelSize: 9; onClicked: backend.setView("workspace") }
                            Button { text: "Git diff"; flat: true; font.pixelSize: 9; onClicked: root.commandRequested("git diff") }
                            Button { text: qsTr("Changer"); flat: true; font.pixelSize: 9; onClicked: backend.pickWorkspace() }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 7
                        Text {
                            text: qsTr("Agent terminal")
                            color: theme.brightText
                            font.family: "iA Writer Duospace"
                            font.pixelSize: 16
                            font.weight: Font.Bold
                        }
                        Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Le terminal reste votre shell réel. Le harness y reçoit des tâches en langage naturel via dsh headless.")
                            color: theme.mutedText
                            font.pixelSize: 9
                            wrapMode: Text.Wrap
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Button { text: qsTr("Ouvrir le terminal"); flat: true; font.pixelSize: 9; onClicked: backend.setView("terminal") }
                            Item { Layout.fillWidth: true }
                            Button { text: qsTr("Palette"); flat: true; font.pixelSize: 9; onClicked: root.openPalette() }
                        }
                    }
                }

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
                            Text { text: qsTr("LOCAL"); color: theme.dimText; font.pixelSize: 7; font.letterSpacing: 1 }
                        }

                        ServiceRow {
                            Layout.fillWidth: true
                            name: qsTr("Harness")
                            detail: backend.harnessDetail
                            stateColor: root.stateColor(backend.harnessState)
                            actionLabel: backend.harnessState === "running" ? qsTr("Ouvrir") : qsTr("Démarrer")
                            actionEnabled: backend.harnessState !== "starting"
                            onAction: {
                                if (backend.harnessState === "running") backend.setView("harness")
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
                            name: "9Router"
                            detail: nineRouter.running ? nineRouter.detail : (nineRouter.installed ? qsTr("installé, arrêté") : qsTr("non installé"))
                            stateColor: nineRouter.running ? theme.green : (nineRouter.starting ? theme.yellow : theme.dimText)
                            actionLabel: nineRouter.running ? qsTr("Ouvrir") : qsTr("Démarrer")
                            actionEnabled: !nineRouter.starting && !nineRouter.installing
                            onAction: {
                                backend.setView("ninerouter")
                                if (!nineRouter.running) nineRouter.start()
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
                                if (!mausBot.running) mausBot.start()
                            }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.topMargin: 8; Layout.bottomMargin: 6; height: 1; color: theme.border }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            Button { text: qsTr("Projet"); flat: true; font.pixelSize: 9; onClicked: backend.setView("ecosystem") }
                            Button { text: qsTr("Réglages"); flat: true; font.pixelSize: 9; onClicked: backend.setView("settings") }
                            Item { Layout.fillWidth: true }
                            Button { text: qsTr("Palette"); flat: true; font.pixelSize: 9; onClicked: root.openPalette() }
                        }
                    }
                }
            }
        }
    }
}
