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
            ScrollBar.vertical: AppScrollBar {}

        Column {
            width: root.contentWidth
            x: Math.max(24, (parent.width - width) / 2)
            topPadding: 26
            bottomPadding: 32
            spacing: 14

            PageHeader {
                width: parent.width
                title: qsTr("Écosystème")
                description: qsTr("Les composants actifs d'Ultimate IAH, le projet, les notes et le contexte agentique.")
                icon: "icons/ecosystem.svg"
                ActionButton { text: qsTr("Rafraîchir"); onClicked: root.refresh(true) }
            }

            HomeCard {
                title: qsTr("Runtime actif")
                icon: "icons/spark.svg"
                accent: theme.green
                cardWidth: root.contentWidth
                Layout.fillWidth: true

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Repeater {
                        model: [
                            { name: qsTr("Ultimate Agent"), detail: backend.harnessDetail, ok: true, view: "agent", action: qsTr("Ouvrir") },
                            { name: qsTr("Terminal agent"), detail: terminal.running ? (terminal.shell + " · " + backend.workspaceName) : qsTr("arrêté"), ok: terminal.running, view: "terminal", action: qsTr("Ouvrir") },
                            { name: qsTr("DeepSeek API"), detail: connections.deepseekConfigured ? (connections.defaultModel !== "" ? connections.defaultModel : qsTr("clé configurée")) : qsTr("clé API manquante"), ok: connections.deepseekConfigured, view: "connections", action: qsTr("Configurer") },
                            { name: qsTr("Ultimate Agent"), detail: connections.deepseekConfigured ? qsTr("prêt · DeepSeek") : qsTr("à configurer"), ok: connections.deepseekConfigured, view: "agent", action: qsTr("Ouvrir") },
                            { name: "MCP", detail: mcpManager.builtins.length > 0 ? qsTr("Mémoire intégrée · %n serveur(s) personnalisé(s)", "", mcpManager.servers.length) : qsTr("%n serveur(s) personnalisé(s)", "", mcpManager.servers.length), ok: mcpManager.builtins.length > 0 || mcpManager.servers.length > 0, view: "mcp", action: qsTr("Ouvrir") },
                            { name: qsTr("Mémoire persistante"), detail: qsTr("%n entrée(s)", "", memory.stats.total !== undefined ? memory.stats.total : 0), ok: memory.stats.total !== undefined && memory.stats.total > 0, view: "memory", action: qsTr("Ouvrir") },
                            { name: qsTr("Skills"), detail: qsTr("%n skill(s) disponible(s)", "", ecosystem.skills.length), ok: ecosystem.skills.length > 0, view: "ecosystem", action: qsTr("Ouvrir") },
                            { name: qsTr("Bots"), detail: bots.serverRunning ? qsTr("%n bot(s) · API locale", "", bots.bots.length) : qsTr("service arrêté"), ok: bots.serverRunning && bots.bots.length > 0, view: "bots", action: qsTr("Ouvrir") },
                            { name: qsTr("Notes"), detail: qsTr("%n note(s) · Markdown local", "", noteStore.count), ok: noteStore.count > 0, view: "notes", action: qsTr("Ouvrir") },
                            { name: qsTr("Plugins"), detail: qsTr("%n outil(s) · ArtCraft", "", plugins.toolCount), ok: plugins.installedCount > 0, view: "plugins", action: qsTr("Ouvrir") },
                            { name: qsTr("Recherche web"), detail: web.searchReady ? qsTr("DeepSeek Search") : qsTr("DuckDuckGo"), ok: true, view: "plugins", action: qsTr("Gérer") },
                            { name: qsTr("Installation"), detail: installation.summary !== "" ? installation.summary : qsTr("audit disponible"), ok: installation.failed === 0, view: "installation", action: qsTr("Vérifier") }
                        ]
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.topMargin: index === 0 ? 0 : design.space2
                                Layout.bottomMargin: design.space2
                                spacing: design.space2
                                StatusDot { color: modelData.ok ? theme.green : theme.dimText }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 1
                                    Text { Layout.fillWidth: true; text: modelData.name; color: theme.text; font.pixelSize: design.textCaption; font.weight: Font.DemiBold }
                                    Text { Layout.fillWidth: true; text: modelData.detail; color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                                }
                                Button { text: modelData.action; flat: true; font.pixelSize: design.textCaption; onClicked: backend.setView(modelData.view) }
                            }
                            Rectangle { Layout.fillWidth: true; height: 1; color: theme.border; opacity: 0.6 }
                        }
                    }
                }
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
                        text: qsTr("Une tâche prête pour Ultimate Agent, exécutée dans le terminal agent.")
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
