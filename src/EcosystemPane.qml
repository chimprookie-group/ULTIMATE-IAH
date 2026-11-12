import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    signal commandRequested(string command)
    signal agentTaskRequested(string prompt)

    readonly property int contentWidth: Math.max(320, Math.min(980, width - 64))
    readonly property int cardWidth: contentWidth >= 760 ? Math.floor((contentWidth - 12) / 2) : contentWidth

    function refresh(rescanTools) {
        if (rescanTools)
            ecosystem.rescan()
        else
            ecosystem.refresh(backend.workspace)
        backend.refreshGit()
    }

    function availableTools() {
        let count = 0
        for (let i = 0; i < ecosystem.tools.length; ++i)
            if (ecosystem.tools[i].available)
                count++
        return count
    }

    function contextBrief() {
        let text = ecosystem.brief()
        if (backend.gitAvailable) {
            const state = backend.gitDirtyCount > 0
                    ? qsTr("%n changement(s)", "", backend.gitDirtyCount)
                    : qsTr("propre")
            text += "\n" + qsTr("- Git : %1").arg(backend.gitBranch + " · " + state)
        }
        return text
    }

    function sendTask(text) {
        if (text !== "")
            root.agentTaskRequested(text)
    }

    Component.onCompleted: if (visible) Qt.callLater(refresh)
    onVisibleChanged: if (visible) refresh()

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Column {
            width: root.contentWidth
            x: Math.max(24, (parent.width - width) / 2)
            topPadding: 26
            bottomPadding: 32
            spacing: 14

            RowLayout {
                width: parent.width
                spacing: 14
                Item {
                    width: 56
                    height: 56
                    Rectangle {
                        anchors.centerIn: parent
                        width: 54
                        height: 54
                        radius: 27
                        color: theme.accent
                        opacity: 0.1
                    }
                    ThemedIcon { anchors.centerIn: parent; source: "icons/ecosystem.svg"; width: 24; height: 24; color: theme.accent }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: qsTr("Projet")
                        color: theme.brightText
                        font.family: "iA Writer Duospace"
                        font.pixelSize: 26
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Le projet, ses outils, ses skills et le contexte à confier au harness.")
                        color: theme.mutedText
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                    }
                }
                Button { text: qsTr("Rafraîchir"); flat: true; onClicked: root.refresh(true) }
            }

            Flow {
                width: parent.width
                spacing: 6
                Repeater {
                    model: [
                        { "label": ecosystem.project.label !== undefined ? ecosystem.project.label : qsTr("workspace") },
                        { "label": qsTr("%n outil(s) prêt(s)", "", root.availableTools()) },
                        { "label": qsTr("%n skill(s)", "", ecosystem.skills.length) }
                    ]
                    Rectangle {
                        width: chip.implicitWidth + 18
                        height: 24
                        radius: 12
                        color: theme.chip
                        border.width: 1
                        border.color: theme.border
                        Text {
                            id: chip
                            anchors.centerIn: parent
                            text: modelData.label
                            color: theme.mutedText
                            font.pixelSize: 9
                        }
                    }
                }
            }

            Flow {
                width: parent.width
                spacing: 12

                HomeCard {
                    title: qsTr("Projet")
                    icon: "icons/branch.svg"
                    cardWidth: root.cardWidth

                    Text {
                        Layout.fillWidth: true
                        text: ecosystem.project.name !== undefined ? ecosystem.project.name : backend.workspaceName
                        color: theme.text
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.workspace
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: 9
                        elide: Text.ElideMiddle
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.gitAvailable
                              ? (backend.gitBranch + " · " + (backend.gitDirtyCount > 0 ? qsTr("%n changement(s)", "", backend.gitDirtyCount) : qsTr("propre")))
                              : qsTr("hors dépôt git")
                        color: backend.gitDirtyCount > 0 ? theme.orange : theme.mutedText
                        font.family: backend.monoFont
                        font.pixelSize: 9
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 4
                        Repeater {
                            model: ecosystem.project.commands !== undefined ? ecosystem.project.commands : []
                            Button {
                                text: modelData.label
                                flat: true
                                font.pixelSize: 9
                                onClicked: root.commandRequested(modelData.command)
                            }
                        }
                    }
                    Button {
                        text: qsTr("Git diff")
                        flat: true
                        font.pixelSize: 9
                        onClicked: root.commandRequested("git diff")
                    }
                }

                HomeCard {
                    title: qsTr("Outils")
                    icon: "icons/terminal.svg"
                    cardWidth: root.cardWidth

                    Repeater {
                        model: ecosystem.tools
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7
                            StatusDot { size: 4; color: modelData.available ? theme.green : theme.dimText }
                            Text { Layout.fillWidth: true; text: modelData.label !== undefined ? modelData.label : modelData.id; color: theme.text; font.pixelSize: 10; elide: Text.ElideRight }
                            Text { text: modelData.version; color: theme.dimText; font.pixelSize: 8; elide: Text.ElideRight; Layout.maximumWidth: 150 }
                        }
                    }
                }

                HomeCard {
                    title: qsTr("Skills")
                    icon: "icons/spark.svg"
                    cardWidth: root.cardWidth

                    Text {
                        Layout.fillWidth: true
                        visible: ecosystem.skills.length === 0
                        text: qsTr("Aucun skill dans ~/.agents/skills.")
                        color: theme.dimText
                        font.pixelSize: 9
                        wrapMode: Text.Wrap
                    }

                    Repeater {
                        model: ecosystem.skills
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text { Layout.fillWidth: true; text: modelData.name !== undefined ? modelData.name : modelData.id; color: theme.text; font.pixelSize: 10; elide: Text.ElideRight }
                                Text { Layout.fillWidth: true; text: modelData.description; color: theme.dimText; font.pixelSize: 8; elide: Text.ElideRight }
                            }
                            Button {
                                text: qsTr("Confier")
                                flat: true
                                font.pixelSize: 9
                                onClicked: root.sendTask(qsTr("Utilise le skill « %1 » depuis %2 pour m'aider sur ce projet. Dis-moi d'abord ce qu'il apporte et ce dont tu as besoin.").arg(modelData.name).arg(modelData.path))
                            }
                        }
                    }
                }

                HomeCard {
                    title: qsTr("Contexte et recettes")
                    icon: "icons/plug.svg"
                    accent: theme.green
                    cardWidth: root.cardWidth

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Une tâche prête pour le harness, exécutée dans le terminal agent.")
                        color: theme.dimText
                        font.pixelSize: 9
                        wrapMode: Text.Wrap
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 4
                        Button { text: qsTr("Expliquer"); flat: true; font.pixelSize: 9; onClicked: root.sendTask(qsTr("Explique l'architecture de ce projet, ses modules principaux et les flux de données. Cite les fichiers importants.")) }
                        Button { text: qsTr("Relire"); flat: true; font.pixelSize: 9; onClicked: root.sendTask(qsTr("Relis mes changements non commités : bugs, régressions, cas limites et sécurité. Propose des correctifs concrets.")) }
                        Button { text: qsTr("Tests"); flat: true; font.pixelSize: 9; onClicked: root.sendTask(qsTr("Regarde le projet et propose les tests manquants les plus utiles. Écris-les puis lance la suite.")) }
                        Button { text: qsTr("Corriger"); flat: true; font.pixelSize: 9; onClicked: root.sendTask(qsTr("Reproduis le problème que je vais décrire, trouve la cause racine, corrige et vérifie avec un vrai test.")) }
                        Button { text: qsTr("Sécurité"); flat: true; font.pixelSize: 9; onClicked: root.sendTask(qsTr("Audite la sécurité du projet : entrées non validées, secrets, permissions, dépendances et chemins d'exécution. Classe les risques.")) }
                        Button { text: qsTr("Performance"); flat: true; font.pixelSize: 9; onClicked: root.sendTask(qsTr("Trouve les goulots d'étranglement de performance. Mesure quand c'est possible, puis propose des optimisations classées par impact.")) }
                    }
                    Button {
                        text: qsTr("Injecter le contexte")
                        flat: true
                        font.pixelSize: 9
                        onClicked: root.sendTask(root.contextBrief())
                    }
                }
            }
        }
    }
}
