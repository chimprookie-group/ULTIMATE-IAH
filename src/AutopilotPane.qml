import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(design.contentMax, width - design.space7))

    function addJob() {
        if (titleField.text.trim() === "" || promptArea.text.trim() === "") {
            Window.window.showToast(qsTr("Titre et tâche sont obligatoires."))
            return
        }
        const interval = parseInt(intervalField.text) || 0
        autopilot.addJob(titleField.text.trim(), promptArea.text.trim(), interval,
                         backend.workspace, 3)
        if (autopilot.error === "") {
            titleField.text = ""
            promptArea.text = ""
            intervalField.text = "60"
            Window.window.showToast(qsTr("Tâche autonome ajoutée."))
        }
    }

    function statusColor(status) {
        if (status === "running") return theme.yellow
        if (status === "done") return theme.green
        if (status === "failed") return theme.red
        if (status === "paused") return theme.dimText
        return theme.accent
    }

    Component.onCompleted: if (visible) autopilot.refresh()
    onVisibleChanged: if (visible) autopilot.refresh()

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            width: root.contentWidth
            x: Math.max(design.space5, (parent.width - width) / 2)
            spacing: design.space5

            Item { Layout.fillWidth: true; Layout.preferredHeight: design.space4 }

            PageHeader {
                Layout.fillWidth: true
                title: qsTr("Autopilote")
                description: qsTr("Des tâches planifiées, exécutées une à une par Ultimate Agent, avec mémoire et reprise.")
                icon: "icons/autopilot.svg"
                accent: theme.orange
                Switch {
                    checked: autopilot.enabled
                    onToggled: autopilot.setEnabled(checked)
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 44
                radius: Math.min(8, theme.radius)
                color: autopilot.enabled ? Qt.rgba(theme.orange.r, theme.orange.g, theme.orange.b, 0.08) : theme.surface
                border.width: 1
                border.color: autopilot.enabled ? theme.orange : theme.border
                Text {
                    anchors.fill: parent
                    anchors.margins: 10
                    text: autopilot.enabled
                          ? qsTr("Autonomie active : les tâches échues s'exécutent seules, une à la fois, dans %1.").arg(backend.workspace)
                          : qsTr("Autonomie en pause. Les tâches ne s'exécutent que manuellement.")
                    color: autopilot.enabled ? theme.orange : theme.mutedText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }
            }

            Rectangle {
                Layout.fillWidth: true
                visible: autopilot.running
                height: runningText.implicitHeight + 20
                radius: Math.min(8, theme.radius)
                color: theme.surface
                border.width: 1
                border.color: theme.yellow
                ColumnLayout {
                    x: 12
                    y: 10
                    width: parent.width - 24
                    spacing: 4
                    RowLayout {
                        Layout.fillWidth: true
                        StatusDot { color: theme.yellow }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("En cours : %1").arg(autopilot.currentTitle)
                            color: theme.text
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                        }
                        Text { text: qsTr("moteur agent"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                    }
                    Text {
                        id: runningText
                        Layout.fillWidth: true
                        text: autopilot.lastOutput !== "" ? autopilot.lastOutput.slice(-1200) : qsTr("L'agent travaille…")
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: design.textMicro
                        wrapMode: Text.Wrap
                        maximumLineCount: 8
                        elide: Text.ElideRight
                    }
                }
            }

            SettingsSection {
                title: qsTr("Nouvelle tâche autonome")
                Layout.fillWidth: true

                SettingsField {
                    id: titleField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Titre — ex. Vérifier les tests chaque matin")
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    radius: Math.min(6, theme.radius)
                    color: theme.background
                    border.width: 1
                    border.color: promptArea.activeFocus ? theme.accent : theme.border
                    TextArea {
                        id: promptArea
                        anchors.fill: parent
                        anchors.margins: 7
                        color: theme.text
                        font.pixelSize: 10
                        wrapMode: TextArea.Wrap
                        background: null
                    }
                    Text {
                        anchors.fill: parent
                        anchors.margins: 10
                        visible: promptArea.text === ""
                        text: qsTr("La tâche confiée à Ultimate Agent. La mémoire pertinente du projet est ajoutée automatiquement.")
                        color: theme.dimText
                        font.pixelSize: 10
                        wrapMode: Text.Wrap
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    SettingsField {
                        id: intervalField
                        Layout.preferredWidth: 70
                        text: "60"
                        inputMethodHints: Qt.ImhDigitsOnly
                    }
                    Text {
                        text: qsTr("minutes")
                        color: theme.mutedText
                        font.pixelSize: design.textCaption
                    }
                    Text {
                        text: qsTr("0 = manuel · sinon répétition à intervalle régulier")
                        color: theme.dimText
                        font.pixelSize: design.textMicro
                    }
                    Item { Layout.fillWidth: true }
                    ActionButton { text: qsTr("Ajouter"); onClicked: root.addJob() }
                }
                Text {
                    Layout.fillWidth: true
                    visible: autopilot.error !== ""
                    text: autopilot.error
                    color: theme.red
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: design.space5
                spacing: design.space3
                visible: autopilot.jobs.length === 0

                BrandMark {
                    Layout.alignment: Qt.AlignHCenter
                    width: 48
                    height: 42
                    size: 48
                    accent: true
                    markOpacity: 0.7
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Aucune tâche autonome")
                    color: theme.text
                    font.pixelSize: design.textBody
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Ajoutez une tâche et activez l'autonomie pour la faire tourner à intervalle régulier.")
                    color: theme.dimText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }
            }

            Repeater {
                model: autopilot.jobs
                Rectangle {
                    Layout.fillWidth: true
                    height: jobBody.implicitHeight + 22
                    radius: Math.min(10, theme.radius)
                    color: theme.surface
                    border.width: 1
                    border.color: modelData.status === "running" ? theme.yellow : theme.border

                    ColumnLayout {
                        id: jobBody
                        x: 12
                        y: 10
                        width: parent.width - 24
                        spacing: 5

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7
                            StatusDot { color: root.statusColor(modelData.status) }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.title
                                color: theme.text
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                            Text {
                                text: modelData.status
                                color: root.statusColor(modelData.status)
                                font.pixelSize: design.textMicro
                                font.weight: Font.DemiBold
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelData.prompt
                            color: theme.mutedText
                            font.pixelSize: design.textCaption
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: (modelData.intervalMinutes > 0
                                   ? qsTr("Toutes les %1 min").arg(modelData.intervalMinutes)
                                   : qsTr("Manuelle"))
                                  + (modelData.nextRun !== "" ? " · " + qsTr("prochaine : %1").arg(modelData.nextRun.substring(0, 16).replace("T", " ")) : "")
                                  + (modelData.lastError !== "" ? " · " + modelData.lastError : "")
                            color: modelData.lastError !== "" ? theme.red : theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textMicro
                            wrapMode: Text.Wrap
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Button {
                                text: qsTr("Exécuter")
                                flat: true
                                font.pixelSize: design.textMicro
                                enabled: !autopilot.running
                                onClicked: autopilot.runNow(modelData.id)
                            }
                            Button {
                                text: modelData.enabled ? qsTr("Mettre en pause") : qsTr("Reprendre")
                                flat: true
                                font.pixelSize: design.textMicro
                                onClicked: autopilot.toggleJob(modelData.id)
                            }
                            Item { Layout.fillWidth: true }
                            Button {
                                text: qsTr("Supprimer")
                                flat: true
                                font.pixelSize: design.textMicro
                                enabled: !autopilot.running
                                onClicked: autopilot.removeJob(modelData.id)
                            }
                        }
                    }
                }
            }
        }
    }
}
