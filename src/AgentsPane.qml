import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(design.contentMax, width - design.space7))
    property bool rolesInitialized: false

    function startMission() {
        if (missionTask.text.trim() === "") {
            Window.window.showToast(qsTr("Décrivez la mission."))
            return
        }
        const title = missionTitle.text.trim() !== "" ? missionTitle.text.trim() : qsTr("Mission du %1").arg(new Date().toLocaleString(Qt.locale(), "dd/MM HH:mm"))
        missions.startMissionWithAgents(title, missionTask.text.trim(), backend.workspace,
                                        plannerBox.currentValue, builderBox.currentValue, reviewerBox.currentValue)
        if (missions.error === "") {
            missionTitle.text = ""
            missionTask.text = ""
        }
    }

    function addProfile() {
        agents.addProfile(profileName.text, "custom", profilePrompt.text)
        if (agents.error === "") {
            profileName.text = ""
            profilePrompt.text = ""
            Window.window.showToast(qsTr("Profil d'agent ajouté."))
        }
    }

    function latestOutput(steps) {
        for (let i = steps.length - 1; i >= 0; --i)
            if (steps[i].output !== "")
                return steps[i].output
        return ""
    }

    function statusColor(status) {
        if (status === "running") return theme.yellow
        if (status === "done") return theme.green
        if (status === "failed") return theme.red
        if (status === "cancelled") return theme.dimText
        return theme.accent
    }

    function addSkill() {
        skills.createSkill(skillName.text, skillDescription.text, skillInstructions.text)
        if (skills.error === "") {
            skillName.text = ""
            skillDescription.text = ""
            skillInstructions.text = ""
            Window.window.showToast(qsTr("Skill ajouté."))
        }
    }

    Component.onCompleted: { agents.refresh(); missions.refresh(); skills.refresh() }
    onVisibleChanged: if (visible) { agents.refresh(); missions.refresh(); skills.refresh() }

    Connections {
        target: agents
        function onChanged() {
            if (root.rolesInitialized || plannerBox.count < 3)
                return
            plannerBox.currentIndex = 0
            builderBox.currentIndex = Math.min(1, builderBox.count - 1)
            reviewerBox.currentIndex = Math.min(2, reviewerBox.count - 1)
            root.rolesInitialized = true
        }
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
            spacing: design.space5

            Item { Layout.fillWidth: true; Layout.preferredHeight: design.space4 }

            PageHeader {
                Layout.fillWidth: true
                title: qsTr("Agents")
                description: qsTr("Planifier, exécuter, relire : une mission orchestrée par plusieurs rôles, avec mémoire et vérification.")
                icon: "icons/agents.svg"
                accent: theme.accent
            }

            Rectangle {
                Layout.fillWidth: true
                visible: missions.running
                height: runningColumn.implicitHeight + 20
                radius: Math.min(8, theme.radius)
                color: theme.surface
                border.width: 1
                border.color: theme.yellow
                ColumnLayout {
                    id: runningColumn
                    x: 12
                    y: 10
                    width: parent.width - 24
                    spacing: 4
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 7
                        StatusDot { color: theme.yellow }
                        Text {
                            Layout.fillWidth: true
                            text: missions.currentTitle + "  ·  " + missions.currentStep
                            color: theme.text
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                        }
                        Button { text: qsTr("Annuler"); flat: true; font.pixelSize: design.textMicro; onClicked: missions.cancel() }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: missions.lastOutput !== "" ? missions.lastOutput.slice(-1200) : qsTr("L'agent travaille…")
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
                title: qsTr("Nouvelle mission")
                Layout.fillWidth: true

                SettingsField {
                    id: missionTitle
                    Layout.fillWidth: true
                    placeholderText: qsTr("Titre — ex. Corriger le login et vérifier")
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    radius: Math.min(6, theme.radius)
                    color: theme.background
                    border.width: 1
                    border.color: missionTask.activeFocus ? theme.accent : theme.border
                    TextArea {
                        id: missionTask
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
                        visible: missionTask.text === ""
                        text: qsTr("La tâche complète. Le planificateur, l'exécutant et le réviseur travaillent dans ce projet.")
                        color: theme.dimText
                        font.pixelSize: 10
                        wrapMode: Text.Wrap
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space3

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text { text: qsTr("Planification"); color: theme.dimText; font.pixelSize: design.textCaption }
                        ComboBox { id: plannerBox; Layout.fillWidth: true; model: agents.profiles; textRole: "name"; valueRole: "id"; font.pixelSize: 8 }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text { text: qsTr("Exécution"); color: theme.dimText; font.pixelSize: design.textCaption }
                        ComboBox {
                            id: builderBox
                            Layout.fillWidth: true
                            model: agents.profiles
                            textRole: "name"
                            valueRole: "id"
                            font.pixelSize: design.textMicro
                            currentIndex: 1
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text { text: qsTr("Revue"); color: theme.dimText; font.pixelSize: design.textCaption }
                        ComboBox {
                            id: reviewerBox
                            Layout.fillWidth: true
                            model: agents.profiles
                            textRole: "name"
                            valueRole: "id"
                            font.pixelSize: design.textMicro
                            currentIndex: 2
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Séquence : Plan → Exécution → Revue · mémoire injectée à chaque étape")
                        color: theme.dimText
                        font.pixelSize: design.textMicro
                        wrapMode: Text.Wrap
                    }
                    ActionButton {
                        text: qsTr("Lancer la mission")
                        enabled: !missions.running
                        onClicked: root.startMission()
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: missions.error !== ""
                    text: missions.error
                    color: theme.red
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }
            }

            Text {
                Layout.fillWidth: true
                visible: missions.missions.length === 0
                text: qsTr("Aucune mission. Le premier plan apparaîtra ici.")
                color: theme.dimText
                font.pixelSize: design.textCaption
            }

            Repeater {
                model: missions.missions
                Rectangle {
                    Layout.fillWidth: true
                    height: missionBody.implicitHeight + 22
                    radius: Math.min(10, theme.radius)
                    color: theme.surface
                    border.width: 1
                    border.color: modelData.status === "running" ? theme.yellow : theme.border

                    ColumnLayout {
                        id: missionBody
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
                            Button {
                                text: qsTr("Supprimer")
                                flat: true
                                font.pixelSize: design.textMicro
                                enabled: !missions.running
                                onClicked: missions.removeMission(modelData.id)
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelData.task
                            color: theme.mutedText
                            font.pixelSize: design.textCaption
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Repeater {
                                model: modelData.steps
                                Rectangle {
                                    width: stepLabel.implicitWidth + 16
                                    height: 20
                                    radius: 10
                                    color: step.status === "done" ? Qt.rgba(theme.green.r, theme.green.g, theme.green.b, 0.14)
                                         : (step.status === "running" ? Qt.rgba(theme.yellow.r, theme.yellow.g, theme.yellow.b, 0.14) : theme.chip)
                                    Text {
                                        id: stepLabel
                                        anchors.centerIn: parent
                                        text: step.title
                                        color: step.status === "done" ? theme.green
                                             : (step.status === "running" ? theme.yellow : theme.dimText)
                                        font.pixelSize: design.textMicro
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                visible: modelData.error !== ""
                                text: modelData.error
                                color: theme.red
                                font.pixelSize: design.textMicro
                                elide: Text.ElideRight
                                Layout.maximumWidth: 360
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: root.latestOutput(modelData.steps) !== ""
                            text: root.latestOutput(modelData.steps)
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textMicro
                            wrapMode: Text.Wrap
                            maximumLineCount: 5
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Profils d'agents")
                Layout.fillWidth: true

                Repeater {
                    model: agents.profiles
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 7
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text { Layout.fillWidth: true; text: modelData.name + "  ·  " + modelData.role; color: theme.text; font.pixelSize: 10; font.weight: Font.DemiBold }
                            Text { Layout.fillWidth: true; text: modelData.description; color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                        }
                        Button {
                            visible: !modelData.builtin
                            text: qsTr("Supprimer")
                            flat: true
                            font.pixelSize: design.textMicro
                            onClicked: agents.removeProfile(modelData.id)
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    SettingsField { id: profileName; Layout.preferredWidth: 170; placeholderText: qsTr("Nom du profil") }
                    SettingsField { id: profilePrompt; Layout.fillWidth: true; placeholderText: qsTr("Prompt du rôle personnalisé") }
                    ActionButton { text: qsTr("Ajouter"); onClicked: root.addProfile() }
                }
                Text {
                    Layout.fillWidth: true
                    visible: agents.error !== ""
                    text: agents.error
                    color: theme.red
                    font.pixelSize: design.textCaption
                }
            }

            SettingsSection {
                title: qsTr("Skills")
                collapsible: true
                expanded: false
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Des instructions réutilisables, découvertes par l'agent dans ~/.agents/skills et dans le projet.")
                    color: theme.dimText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }

                Repeater {
                    model: skills.skills
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 7
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text { Layout.fillWidth: true; text: modelData.name + "  ·  " + modelData.scope; color: theme.text; font.pixelSize: 10; font.weight: Font.DemiBold }
                            Text { Layout.fillWidth: true; text: modelData.description; color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                        }
                        Button {
                            text: qsTr("Utiliser")
                            flat: true
                            font.pixelSize: design.textMicro
                            implicitHeight: design.controlHeightSm
                            onClicked: missionTask.text = qsTr("Utilise le skill « %1 » (%2) pour cette mission.").arg(modelData.name).arg(modelData.path)
                        }
                        Button {
                            visible: modelData.scope === "user"
                            text: qsTr("Supprimer")
                            flat: true
                            font.pixelSize: design.textMicro
                            implicitHeight: design.controlHeightSm
                            onClicked: skills.removeSkill(modelData.id)
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    SettingsField { id: skillName; Layout.preferredWidth: 170; placeholderText: qsTr("Nom du skill") }
                    SettingsField { id: skillDescription; Layout.fillWidth: true; placeholderText: qsTr("Description courte") }
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    radius: Math.min(6, theme.radius)
                    color: theme.background
                    border.width: 1
                    border.color: skillInstructions.activeFocus ? theme.accent : theme.border
                    TextArea {
                        id: skillInstructions
                        anchors.fill: parent
                        anchors.margins: 6
                        color: theme.text
                        placeholderText: qsTr("Instructions du skill…")
                        placeholderTextColor: theme.dimText
                        font.pixelSize: design.textCaption
                        wrapMode: TextArea.Wrap
                        background: null
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        visible: skills.error !== ""
                        text: skills.error
                        color: theme.red
                        font.pixelSize: design.textMicro
                    }
                    Button { text: qsTr("Bibliothèque par défaut"); flat: true; font.pixelSize: design.textMicro; onClicked: { skills.seedDefaults(true); Window.window.showToast(qsTr("Bibliothèque de skills restaurée.")) } }
                    ActionButton { text: qsTr("Créer le skill"); onClicked: root.addSkill() }
                }
            }
        }
    }
}
