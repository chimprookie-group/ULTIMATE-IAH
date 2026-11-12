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

            PageHeader {
                width: parent.width
                title: qsTr("Projet")
                description: qsTr("Le projet, ses outils, ses skills et le contexte à confier au harness.")
                icon: "icons/branch.svg"
                ActionButton { text: qsTr("Rafraîchir"); onClicked: root.refresh(true) }
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
                            font.pixelSize: design.textCaption
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.cardWidth >= root.contentWidth ? 1 : 2
                columnSpacing: 12
                rowSpacing: 12

                HomeCard {
                    title: qsTr("Projet")
                    icon: "icons/branch.svg"
                    cardWidth: root.cardWidth
                    Layout.fillWidth: true
                    Layout.fillHeight: true

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
                        font.pixelSize: design.textCaption
                        elide: Text.ElideMiddle
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.gitAvailable
                              ? (backend.gitBranch + " · " + (backend.gitDirtyCount > 0 ? qsTr("%n changement(s)", "", backend.gitDirtyCount) : qsTr("propre")))
                              : qsTr("hors dépôt git")
                        color: backend.gitDirtyCount > 0 ? theme.orange : theme.mutedText
                        font.family: backend.monoFont
                        font.pixelSize: design.textCaption
                    }
                    Item { Layout.fillHeight: true }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 4
                        Repeater {
                            model: ecosystem.project.commands !== undefined ? ecosystem.project.commands : []
                            Button {
                                text: modelData.label
                                flat: true
                                font.pixelSize: design.textCaption
                                onClicked: root.commandRequested(modelData.command)
                            }
                        }
                        Button {
                            text: qsTr("Git diff")
                            flat: true
                            font.pixelSize: design.textCaption
                            onClicked: root.commandRequested("git diff")
                        }
                    }
                }

                HomeCard {
                    title: qsTr("Outils")
                    icon: "icons/terminal.svg"
                    cardWidth: root.cardWidth
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Repeater {
                        model: ecosystem.tools
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7
                            StatusDot { size: 4; color: modelData.available ? theme.green : theme.dimText }
                            Text { Layout.fillWidth: true; text: modelData.label !== undefined ? modelData.label : modelData.id; color: theme.text; font.pixelSize: 10; elide: Text.ElideRight }
                            Text { text: modelData.version; color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight; Layout.maximumWidth: 150 }
                        }
                    }
                }

                HomeCard {
                    title: qsTr("Skills")
                    icon: "icons/spark.svg"
                    cardWidth: root.cardWidth
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Text {
                        Layout.fillWidth: true
                        visible: ecosystem.skills.length === 0
                        text: qsTr("Aucun skill dans ~/.agents/skills.")
                        color: theme.dimText
                        font.pixelSize: design.textCaption
                        wrapMode: Text.Wrap
                    }

                    Repeater {
                        model: ecosystem.skills
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 5
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 7
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.name !== undefined ? modelData.name : modelData.id
                                    color: theme.text
                                    font.pixelSize: 10
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                }
                                Button {
                                    text: qsTr("Confier")
                                    flat: true
                                    font.pixelSize: design.textCaption
                                    Layout.alignment: Qt.AlignTop
                                    onClicked: root.sendTask(qsTr("Utilise le skill « %1 » depuis %2 pour m'aider sur ce projet. Dis-moi d'abord ce qu'il apporte et ce dont tu as besoin.").arg(modelData.name).arg(modelData.path))
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.description
                                color: theme.dimText
                                font.pixelSize: design.textMicro
                                lineHeight: 1.25
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: theme.border
                                opacity: 0.6
                            }
                        }
                    }
                }

                HomeCard {
                    title: qsTr("Contexte et recettes")
                    icon: "icons/plug.svg"
                    accent: theme.green
                    cardWidth: root.cardWidth
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Une tâche prête pour le harness, exécutée dans le terminal agent.")
                        color: theme.dimText
                        font.pixelSize: design.textCaption
                        wrapMode: Text.Wrap
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 6
                        rowSpacing: 6
                        Repeater {
                            model: [
                                { label: qsTr("Expliquer"), prompt: qsTr("Explique l'architecture de ce projet, ses modules principaux et les flux de données. Cite les fichiers importants.") },
                                { label: qsTr("Relire"), prompt: qsTr("Relis mes changements non commités : bugs, régressions, cas limites et sécurité. Propose des correctifs concrets.") },
                                { label: qsTr("Tests"), prompt: qsTr("Regarde le projet et propose les tests manquants les plus utiles. Écris-les puis lance la suite.") },
                                { label: qsTr("Corriger"), prompt: qsTr("Reproduis le problème que je vais décrire, trouve la cause racine, corrige et vérifie avec un vrai test.") },
                                { label: qsTr("Sécurité"), prompt: qsTr("Audite la sécurité du projet : entrées non validées, secrets, permissions, dépendances et chemins d'exécution. Classe les risques.") },
                                { label: qsTr("Performance"), prompt: qsTr("Trouve les goulots d'étranglement de performance. Mesure quand c'est possible, puis propose des optimisations classées par impact.") }
                            ]
                            Button {
                                id: recipeButton
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                text: modelData.label
                                font.pixelSize: design.textCaption
                                padding: 0
                                background: Rectangle {
                                    radius: design.radiusSm
                                    color: recipeButton.hovered ? theme.raised : theme.chip
                                    border.width: recipeButton.activeFocus ? 2 : 1
                                    border.color: recipeButton.activeFocus ? theme.accent
                                                                          : (recipeButton.hovered ? theme.borderStrong : theme.border)
                                }
                                contentItem: Text {
                                    text: recipeButton.text
                                    color: theme.text
                                    font.pixelSize: design.textCaption
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: root.sendTask(modelData.prompt)
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                    ActionButton {
                        Layout.alignment: Qt.AlignRight
                        text: qsTr("Injecter le contexte")
                        onClicked: root.sendTask(root.contextBrief())
                    }
                }
            }        }
    }
}
