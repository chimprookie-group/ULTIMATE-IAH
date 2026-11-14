import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(design.contentMax, width - design.space7))

    function stateColor(state) {
        if (state === "running")
            return theme.green
        if (state === "starting")
            return theme.yellow
        if (state === "failed")
            return theme.red
        return theme.dimText
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
                title: qsTr("Réglages")
                description: qsTr("Ultimate Agent, son terminal, les agents et votre espace de travail.")
                icon: "icons/sliders.svg"
                accent: theme.mutedText
                ActionButton { text: qsTr("Retour au cockpit"); onClicked: backend.setView("home") }
            }

            SettingsSection {
                title: qsTr("Apparence")
                collapsible: true
                expanded: false
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Ultimate IAH suit le thème Omarchy actif. Choisissez un autre thème installé pour l'application seule.")
                    color: theme.mutedText
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 10

                    Repeater {
                        model: backend.themeOptions
                        ThemeCard {
                            themeId: modelData.id
                            label: modelData.label
                            mode: modelData.mode
                            themeColors: modelData
                            active: backend.themePreference === modelData.id
                            subtitle: modelData.id === backend.systemThemeId ? qsTr("Thème système") : ""
                            onChosen: backend.setTheme(modelData.id)
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Langue")
                collapsible: true
                expanded: false
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Automatique suit Omarchy, puis le navigateur embarqué. Un choix manuel ne s'applique qu'à Ultimate IAH.")
                    color: theme.mutedText
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: [
                            { "id": "", "label": qsTr("Auto") },
                            { "id": "fr", "label": "Français" },
                            { "id": "en", "label": "English" }
                        ]

                        Rectangle {
                            width: label.implicitWidth + 26
                            height: 30
                            radius: 15
                            color: backend.languagePreference === modelData.id ? theme.accent : theme.chip
                            border.width: 1
                            border.color: backend.languagePreference === modelData.id ? theme.accent : theme.border
                            Text {
                                id: label
                                anchors.centerIn: parent
                                text: modelData.label
                                color: backend.languagePreference === modelData.id ? theme.accentForeground : theme.text
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.setLanguage(modelData.id)
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Démarrage")
                collapsible: true
                expanded: false
                Layout.fillWidth: true

                Flow {
                    Layout.fillWidth: true
                    spacing: 6
                    Repeater {
                        model: [
                            { "id": "last", "label": qsTr("Dernière vue") },
                            { "id": "home", "label": qsTr("Cockpit") },
                            { "id": "terminal", "label": qsTr("Terminal agent") },
                            { "id": "ecosystem", "label": qsTr("Projet") },
                            { "id": "workspace", "label": qsTr("Fichiers") },
                            { "id": "memory", "label": qsTr("Mémoire") },
                            { "id": "autopilot", "label": qsTr("Autopilote") },
                            { "id": "mcp", "label": "MCP" },
                            { "id": "agents", "label": qsTr("Agents") },
                            { "id": "agent", "label": qsTr("Ultimate Agent") },
                            { "id": "installation", "label": qsTr("Installation") }
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

            }

            SettingsSection {
                title: qsTr("Espace de travail")
                collapsible: true
                expanded: false
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        Layout.fillWidth: true
                        text: backend.workspace
                        color: theme.text
                        font.family: backend.monoFont
                        font.pixelSize: 10
                        elide: Text.ElideMiddle
                    }
                    ActionButton {
                        text: qsTr("Choisir…")
                        onClicked: backend.pickWorkspace()
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Ultimate Agent et le terminal travaillent dans ce dossier.")
                    color: theme.dimText
                    font.pixelSize: design.textCaption
                }
            }

            SettingsSection {
                title: qsTr("Ultimate Agent")
                collapsible: true
                expanded: false
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Le moteur agent est natif : discussions, images, skills, notes, MCP, missions et bots utilisent les providers configurés dans Providers.")
                    color: theme.mutedText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space2
                    GhostButton { text: qsTr("Providers"); flat: true; onClicked: backend.setView("connections") }
                    GhostButton { text: qsTr("Ultimate Agent"); flat: true; onClicked: backend.setView("agent") }
                    GhostButton { text: qsTr("Diagnostics"); flat: true; onClicked: backend.runDiagnostics() }
                }
            }

            SettingsSection {
                title: qsTr("Providers")
                collapsible: true
                expanded: false
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Les providers d'Ultimate Agent se configurent dans Providers. MCP et la mémoire sont gérés directement par l'application.")
                    color: theme.mutedText
                    font.pixelSize: 10
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    GhostButton { text: qsTr("Providers"); flat: true; onClicked: backend.setView("connections") }
                    GhostButton { text: qsTr("MCP"); flat: true; onClicked: backend.setView("mcp") }
                    GhostButton { text: qsTr("Ultimate Agent"); flat: true; onClicked: backend.setView("agent") }
                    GhostButton { text: qsTr("Agents"); flat: true; onClicked: backend.setView("agents") }
                    GhostButton { text: qsTr("Installation"); flat: true; onClicked: backend.setView("installation") }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: connections.deepseekConfigured ? qsTr("DeepSeek connecté") : qsTr("DeepSeek non configuré")
                        color: connections.deepseekConfigured ? theme.green : theme.dimText
                        font.pixelSize: design.textCaption
                    }
                }
            }

            SettingsSection {
                title: qsTr("Décisions")
                collapsible: true
                expanded: false
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: decisions.summary
                    color: theme.text
                    font.family: backend.monoFont
                    font.pixelSize: design.textMicro
                    wrapMode: Text.Wrap
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Provider résolu : %1 · modèle %2 · source %3")
                          .arg(decisions.providerLabel !== "" ? decisions.providerLabel : qsTr("aucun"))
                          .arg(decisions.modelLabel !== "" ? decisions.modelLabel : qsTr("aucun"))
                          .arg(decisions.sourceLabel !== "" ? decisions.sourceLabel : qsTr("aucune"))
                    color: decisions.providerLabel !== "" ? theme.green : theme.orange
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space2
                    Text { text: qsTr("Stratégie"); color: theme.dimText; font.pixelSize: design.textCaption }
                    ComboBox {
                        Layout.preferredWidth: 150
                        model: [qsTr("Équilibré"), qsTr("Qualité"), qsTr("Gratuit")]
                        currentIndex: decisions.strategy === "quality" ? 1 : (decisions.strategy === "free" ? 2 : 0)
                        onActivated: decisions.strategy = ["balanced", "quality", "free"][index]
                    }
                    Text { text: qsTr("Autonomie"); color: theme.dimText; font.pixelSize: design.textCaption }
                    ComboBox {
                        Layout.preferredWidth: 150
                        model: [qsTr("Manuel"), qsTr("Assisté"), qsTr("Auto")]
                        currentIndex: decisions.autonomy === "manual" ? 0 : (decisions.autonomy === "auto" ? 2 : 1)
                        onActivated: decisions.autonomy = ["manual", "assisted", "auto"][index]
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space2
                    Text { text: qsTr("Provider"); color: theme.dimText; font.pixelSize: design.textCaption }
                    ComboBox {
                        Layout.preferredWidth: 150
                        model: [qsTr("Auto"), qsTr("Harness"), qsTr("DeepSeek"), qsTr("4Crasy")]
                        currentIndex: decisions.providerSource === "harness" ? 1
                                      : (decisions.providerSource === "deepseek" ? 2
                                      : (decisions.providerSource === "4crasy" ? 3 : 0))
                        onActivated: decisions.providerSource = ["auto", "harness", "deepseek", "4crasy"][index]
                    }
                    Text { text: qsTr("Permissions"); color: theme.dimText; font.pixelSize: design.textCaption }
                    ComboBox {
                        Layout.preferredWidth: 170
                        model: [qsTr("Lecture seule"), qsTr("Workspace"), qsTr("Complet")]
                        currentIndex: decisions.permissionCeiling === "read-only" ? 0
                                      : (decisions.permissionCeiling === "full" ? 2 : 1)
                        onActivated: decisions.permissionCeiling = ["read-only", "workspace-write", "full"][index]
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space3
                    Text { text: qsTr("Tours max"); color: theme.dimText; font.pixelSize: design.textCaption }
                    SpinBox {
                        from: 1; to: 500; value: decisions.maxRounds
                        onValueModified: decisions.maxRounds = value
                    }
                    Text { text: qsTr("Retries"); color: theme.dimText; font.pixelSize: design.textCaption }
                    SpinBox {
                        from: 0; to: 10; value: decisions.maxRetries
                        onValueModified: decisions.maxRetries = value
                    }
                    CheckBox {
                        text: qsTr("Fallback")
                        checked: decisions.allowFallback
                        onToggled: decisions.allowFallback = checked
                    }
                    CheckBox {
                        text: qsTr("Revue")
                        checked: decisions.reviewChanges
                        onToggled: decisions.reviewChanges = checked
                    }
                    CheckBox {
                        text: qsTr("Mémoire")
                        checked: decisions.useMemory
                        onToggled: decisions.useMemory = checked
                    }
                    CheckBox {
                        text: qsTr("Apprentissage")
                        checked: decisions.useLearning
                        onToggled: decisions.useLearning = checked
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    GhostButton { text: qsTr("Réinitialiser les décisions"); flat: true; onClicked: decisions.resetOptions() }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: decisions.lastReason
                        color: theme.dimText
                        font.pixelSize: design.textMicro
                        elide: Text.ElideRight
                        Layout.maximumWidth: 420
                    }
                }
            }

            SettingsSection {
                title: qsTr("Mises à jour")
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: updates.statusText
                    color: updates.updateAvailable ? theme.accent : theme.mutedText
                    font.pixelSize: design.textCaption
                }
                Text {
                    Layout.fillWidth: true
                    text: updates.error !== ""
                          ? updates.error
                          : qsTr("Version %1 · dernière vérification : %2").arg(updates.currentVersion).arg(updates.lastChecked !== "" ? updates.lastChecked : qsTr("jamais"))
                    color: updates.error !== "" ? theme.red : theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: design.textMicro
                    wrapMode: Text.Wrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space2
                    GhostButton { text: qsTr("Vérifier maintenant"); flat: true; enabled: !updates.checking && !updates.installing; onClicked: updates.checkNow() }
                    ActionButton { text: updates.installing ? qsTr("Installation… %1 %").arg(updates.progress) : qsTr("Installer la mise à jour"); visible: updates.updateAvailable; enabled: !updates.installing; onClicked: updates.install() }
                    GhostButton { text: qsTr("Notes de version"); flat: true; visible: updates.releaseUrl !== ""; onClicked: Qt.openUrlExternally(updates.releaseUrl) }
                    Item { Layout.fillWidth: true }
                    Text { text: qsTr("skills %1 · ArtCraft %2").arg(skills.defaultCount).arg(plugins.installedCount); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                }
                Rectangle {
                    Layout.fillWidth: true
                    visible: updates.installing
                    Layout.preferredHeight: 3
                    radius: 2
                    color: theme.background
                    Rectangle {
                        width: parent.width * Math.max(0, Math.min(1, updates.progress / 100))
                        height: parent.height
                        radius: parent.radius
                        color: theme.accent
                    }
                }
            }

            SettingsSection {
                title: qsTr("À propos")
                collapsible: true
                expanded: false
                Layout.fillWidth: true
                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space3
                    BrandMark { width: 62; height: 62; size: 62; accent: false }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            Layout.fillWidth: true
                            text: "Ultimate IAH " + backend.appVersion
                            color: theme.text
                            font.pixelSize: design.textBody
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Ultimate Agent, construit pour Omarchy · MIT")
                            color: theme.dimText
                            font.pixelSize: design.textMicro
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Raccourcis : Ctrl+0 cockpit · Ctrl+1 Ultimate Agent · Ctrl+2 terminal · Ctrl+Alt+N notes · Ctrl+K palette · ? aide")
                    color: theme.dimText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
