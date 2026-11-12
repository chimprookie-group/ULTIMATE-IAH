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

        ColumnLayout {
            width: root.contentWidth
            x: Math.max(design.space5, (parent.width - width) / 2)
            spacing: design.space5

            Item { Layout.fillWidth: true; Layout.preferredHeight: design.space4 }

            PageHeader {
                Layout.fillWidth: true
                title: qsTr("Réglages")
                description: qsTr("Le harness, son terminal, les agents et votre espace de travail.")
                icon: "icons/sliders.svg"
                accent: theme.mutedText
                ActionButton { text: qsTr("Retour au cockpit"); onClicked: backend.setView("home") }
            }

            SettingsSection {
                title: qsTr("Apparence")
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Ultimate HAI suit le thème Omarchy actif. Choisissez un autre thème installé pour l'application seule.")
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
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Automatique suit Omarchy, puis le navigateur embarqué. Un choix manuel ne s'applique qu'à Ultimate HAI.")
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
                Layout.fillWidth: true

                Flow {
                    Layout.fillWidth: true
                    spacing: 6
                    Repeater {
                        model: [
                            { "id": "last", "label": qsTr("Dernière vue") },
                            { "id": "home", "label": qsTr("Cockpit") },
                            { "id": "harness", "label": qsTr("Harness") },
                            { "id": "terminal", "label": qsTr("Terminal agent") },
                            { "id": "ecosystem", "label": qsTr("Projet") },
                            { "id": "workspace", "label": qsTr("Fichiers") },
                            { "id": "memory", "label": qsTr("Mémoire") },
                            { "id": "autopilot", "label": qsTr("Autopilote") },
                            { "id": "mcp", "label": "MCP" },
                            { "id": "agents", "label": qsTr("Agents") }
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

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text { text: qsTr("Démarrer le harness avec l'application"); color: theme.text; font.pixelSize: 11 }
                        Text { text: qsTr("Lance dsh web avec la fenêtre."); color: theme.dimText; font.pixelSize: 9 }
                    }
                    Switch {
                        checked: backend.autoStartHarness
                        onToggled: backend.setAutoStartHarness(checked)
                    }
                }
            }

            SettingsSection {
                title: qsTr("Espace de travail")
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
                    text: qsTr("Le harness et le terminal agent travaillent dans ce dossier.")
                    color: theme.dimText
                    font.pixelSize: 9
                }
            }

            SettingsSection {
                title: qsTr("Harness")
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    SettingsField {
                        id: dshField
                        Layout.fillWidth: true
                        text: backend.dshBinary
                        placeholderText: qsTr("Binaire dsh")
                        onEditingFinished: backend.setDshBinary(text)
                    }
                    Rectangle {
                        width: badge.implicitWidth + 16
                        height: 24
                        radius: 12
                        color: theme.background
                        border.width: 1
                        border.color: backend.dshAvailable ? theme.green : theme.red
                        Text {
                            id: badge
                            anchors.centerIn: parent
                            text: backend.dshAvailable ? qsTr("trouvé") : qsTr("absent")
                            color: backend.dshAvailable ? theme.green : theme.red
                            font.pixelSize: 9
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    SettingsField {
                        id: profileField
                        Layout.fillWidth: true
                        text: backend.harnessProfile
                        placeholderText: qsTr("Profil harness")
                        onEditingFinished: backend.setHarnessProfile(text)
                    }
                    Text {
                        text: backend.harnessState + " · " + backend.harnessDetail
                        color: root.stateColor(backend.harnessState)
                        font.pixelSize: 9
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Button { text: qsTr("Tester"); flat: true; onClicked: backend.testHarness() }
                    Button { text: qsTr("Redémarrer"); flat: true; onClicked: backend.restartHarness() }
                    Button { text: qsTr("Diagnostics"); flat: true; onClicked: backend.runDiagnostics() }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: backend.harnessHealth
                        color: backend.harnessHealth === "ok" ? theme.green : theme.dimText
                        font.pixelSize: 9
                    }
                }
            }

            SettingsSection {
                title: qsTr("Connexions")
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Les providers du harness se configurent dans le harness embarqué ou via 9Router. MausBot ajoute son équipe par MCP.")
                    color: theme.mutedText
                    font.pixelSize: 10
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Button { text: qsTr("Connexions du harness"); flat: true; onClicked: backend.setView("connections") }
                    Button { text: qsTr("MCP"); flat: true; onClicked: backend.setView("mcp") }
                    Button { text: qsTr("Agents"); flat: true; onClicked: backend.setView("agents") }
                    Button { text: "9Router"; flat: true; onClicked: backend.setView("ninerouter") }
                    Button { text: "MausBot"; flat: true; onClicked: backend.setView("mausbot") }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: mausBot.mcpHarness ? qsTr("MCP harness connecté") : qsTr("MCP harness non connecté")
                        color: mausBot.mcpHarness ? theme.green : theme.dimText
                        font.pixelSize: 9
                    }
                }
            }

            SettingsSection {
                title: qsTr("À propos")
                Layout.fillWidth: true
                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space3
                    BrandMark { width: 52; height: 52; size: 52; accent: true }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            Layout.fillWidth: true
                            text: "Ultimate HAI " + backend.appVersion
                            color: theme.text
                            font.pixelSize: design.textBody
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Harness-only, construit pour Omarchy · MIT")
                            color: theme.dimText
                            font.pixelSize: design.textMicro
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Raccourcis : Ctrl+0 cockpit · Ctrl+1 harness · Ctrl+2 terminal agent · Ctrl+4 réglages · Ctrl+K palette · ? aide")
                    color: theme.dimText
                    font.pixelSize: 9
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
