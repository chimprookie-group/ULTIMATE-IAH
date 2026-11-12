import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(design.contentMax, width - design.space7))

    function saveDeepSeek() {
        connections.saveDeepSeek(deepseekKey.text)
        if (connections.error === "") {
            deepseekKey.text = ""
            backend.restartHarness()
            Window.window.showToast(qsTr("DeepSeek enregistré. Le harness redémarre."))
        }
    }

    function applyPreset(preset) {
        providerId.text = preset.id
        providerName.text = preset.name
        providerBase.text = preset.base
        providerKey.text = ""
        providerModels.text = preset.models
        providerApi.currentIndex = preset.api === "anthropic-messages" ? 1 : 0
    }

    function saveProvider() {
        connections.saveProvider(providerId.text, providerName.text, providerBase.text,
                                 providerKey.text, providerModels.text, makeDefault.checked,
                                 providerApi.currentIndex === 1 ? "anthropic-messages" : "openai-completions")
        if (connections.error === "") {
            providerId.text = ""
            providerName.text = ""
            providerBase.text = ""
            providerKey.text = ""
            providerModels.text = ""
            backend.restartHarness()
            Window.window.showToast(qsTr("Provider enregistré. Le harness redémarre."))
        }
    }

    function refreshAll() {
        connections.refresh()
        doctorRefresh.restart()
    }

    Component.onCompleted: if (visible) root.refreshAll()
    onVisibleChanged: if (visible) root.refreshAll()

    Timer {
        id: doctorRefresh
        interval: 1100
        onTriggered: connectionDoctor.run()
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
                title: qsTr("Connexions")
                description: qsTr("Les providers et les outils que le harness peut réellement utiliser.")
                icon: "icons/connections.svg"
                accent: theme.green
                ActionButton { text: qsTr("Rafraîchir"); onClicked: connections.refresh() }
            }

            Text {
                Layout.fillWidth: true
                visible: connections.error !== ""
                text: connections.error
                color: theme.red
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }

            SettingsSection {
                title: qsTr("Diagnostic des liaisons")
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space2
                    Text {
                        Layout.fillWidth: true
                        text: connectionDoctor.summary !== "" ? connectionDoctor.summary
                              : qsTr("Vérifiez chaque maillon : DSH, clé DeepSeek, providers, MCP et workspace.")
                        color: connectionDoctor.failed > 0 ? theme.orange : theme.mutedText
                        font.pixelSize: design.textCaption
                        wrapMode: Text.Wrap
                    }
                    ActionButton {
                        text: connectionDoctor.running ? qsTr("Vérification…") : qsTr("Vérifier")
                        enabled: !connectionDoctor.running
                        onClicked: doctorRefresh.restart()
                    }
                }

                Repeater {
                    model: connectionDoctor.checks
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: checkBody.implicitHeight + design.space3
                        radius: design.radiusSm
                        color: modelData.ok ? theme.background : Qt.rgba(theme.orange.r, theme.orange.g, theme.orange.b, 0.07)
                        border.width: 1
                        border.color: modelData.ok ? theme.border : theme.orange
                        RowLayout {
                            id: checkBody
                            anchors.fill: parent
                            anchors.margins: design.space2
                            spacing: design.space2
                            StatusDot { color: modelData.ok ? theme.green : theme.orange }
                            Text {
                                Layout.preferredWidth: 82
                                text: modelData.layer
                                color: theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: design.textMicro
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.label
                                    color: theme.text
                                    font.pixelSize: design.textCaption
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.hint !== "" ? modelData.hint : modelData.detail
                                    color: theme.dimText
                                    font.pixelSize: design.textMicro
                                    elide: Text.ElideRight
                                }
                            }
                            Text {
                                text: modelData.ok ? qsTr("ok") : qsTr("à corriger")
                                color: modelData.ok ? theme.green : theme.orange
                                font.family: backend.monoFont
                                font.pixelSize: design.textMicro
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("DeepSeek")
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: connections.deepseekConfigured
                          ? qsTr("Clé active : propagée automatiquement à DSH et au terminal.")
                          : qsTr("Ajoutez une clé API DeepSeek. Elle sera propagée automatiquement à DSH et au terminal.")
                    color: connections.deepseekConfigured ? theme.green : theme.mutedText
                    font.pixelSize: 10
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    SettingsField {
                        id: deepseekKey
                        Layout.fillWidth: true
                        placeholderText: connections.deepseekConfigured ? qsTr("Remplacer la clé DeepSeek") : qsTr("sk-…")
                        echoMode: TextInput.Password
                    }
                    ActionButton { text: qsTr("Enregistrer"); onClicked: root.saveDeepSeek() }
                }
            }

            SettingsSection {
                title: qsTr("Catalogue")
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Choisissez un preset, puis collez votre clé. Les endpoints locaux n'exigent pas de clé.")
                    color: theme.dimText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 4
                    Repeater {
                        model: [
                            { id: "openai", name: "OpenAI", base: "https://api.openai.com/v1", api: "openai-completions", models: "gpt-4.1=GPT-4.1\no3=o3" },
                            { id: "anthropic", name: "Anthropic", base: "https://api.anthropic.com/v1", api: "anthropic-messages", models: "claude-sonnet-4-20250514=Claude Sonnet 4" },
                            { id: "openrouter", name: "OpenRouter", base: "https://openrouter.ai/api/v1", api: "openai-completions", models: "deepseek/deepseek-chat=DeepSeek Chat\nanthropic/claude-sonnet-4=Claude Sonnet 4\nopenai/gpt-4.1=GPT-4.1" },
                            { id: "gemini", name: "Google Gemini", base: "https://generativelanguage.googleapis.com/v1beta/openai/", api: "openai-completions", models: "gemini-2.5-pro=Gemini 2.5 Pro\ngemini-2.5-flash=Gemini 2.5 Flash" },
                            { id: "groq", name: "Groq", base: "https://api.groq.com/openai/v1", api: "openai-completions", models: "llama-3.3-70b-versatile=Llama 3.3 70B" },
                            { id: "mistral", name: "Mistral", base: "https://api.mistral.ai/v1", api: "openai-completions", models: "mistral-large-latest=Mistral Large" },
                            { id: "xai", name: "xAI", base: "https://api.x.ai/v1", api: "openai-completions", models: "grok-4=Grok 4" },
                            { id: "ollama", name: "Ollama", base: "http://127.0.0.1:11434/v1", api: "openai-completions", models: "qwen3:8b=Qwen3 8B" },
                            { id: "lmstudio", name: "LM Studio", base: "http://127.0.0.1:1234/v1", api: "openai-completions", models: "local-model=Local model" }
                        ]
                        Button {
                            text: modelData.name
                            flat: true
                            font.pixelSize: design.textMicro
                            onClicked: root.applyPreset(modelData)
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Providers du harness")
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    visible: connections.providers.length === 0
                    text: qsTr("Aucun provider configuré. Ajoutez DeepSeek ou un endpoint OpenAI-compatible.")
                    color: theme.dimText
                    font.pixelSize: 10
                    wrapMode: Text.Wrap
                }

                Repeater {
                    model: connections.providers
                    Rectangle {
                        property var providerData: modelData
                        Layout.fillWidth: true
                        height: providerBody.implicitHeight + 22
                        radius: Math.min(10, theme.radius)
                        color: theme.background
                        border.width: 1
                        border.color: theme.border

                        ColumnLayout {
                            id: providerBody
                            x: 12
                            y: 10
                            width: parent.width - 24
                            spacing: 5

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                StatusDot { color: modelData.hasKey ? theme.green : theme.yellow }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.name + "  ·  " + modelData.id
                                    color: theme.text
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: modelData.hasKey ? modelData.keyMasked : qsTr("sans clé")
                                    color: modelData.hasKey ? theme.green : theme.yellow
                                    font.family: backend.monoFont
                                    font.pixelSize: design.textMicro
                                }
                                Button {
                                    visible: !modelData.builtin
                                    text: qsTr("Supprimer")
                                    flat: true
                                    font.pixelSize: design.textMicro
                                    onClicked: {
                                        connections.removeProvider(modelData.id)
                                        backend.restartHarness()
                                    }
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.baseURL + "  ·  " + (modelData.api !== undefined ? modelData.api : "openai-completions")
                                color: theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: design.textMicro
                                elide: Text.ElideMiddle
                            }
                            Flow {
                                Layout.fillWidth: true
                                spacing: 4
                                Repeater {
                                    model: modelData.models
                                    Button {
                                        text: modelData
                                        flat: true
                                        font.pixelSize: design.textMicro
                                        highlighted: connections.defaultModel === (providerData.id + "/" + modelData)
                                        onClicked: {
                                            connections.setDefault(providerData.id, modelData)
                                            backend.restartHarness()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Ajouter un provider OpenAI-compatible")
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    SettingsField { id: providerId; Layout.fillWidth: true; placeholderText: qsTr("ID (ex. deepseek-direct)") }
                    SettingsField { id: providerName; Layout.fillWidth: true; placeholderText: qsTr("Nom affiché") }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    SettingsField {
                        id: providerBase
                        Layout.fillWidth: true
                        placeholderText: qsTr("URL de base (ex. https://api.deepseek.com/v1)")
                    }
                    ComboBox {
                        id: providerApi
                        Layout.preferredWidth: 180
                        model: ["OpenAI-compatible", "Anthropic"]
                        font.pixelSize: design.textCaption
                    }
                }
                SettingsField {
                    id: providerKey
                    Layout.fillWidth: true
                    placeholderText: qsTr("Clé API")
                    echoMode: TextInput.Password
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Modèles, un par ligne. Utilisez id=Nom affiché si besoin.")
                    color: theme.dimText
                    font.pixelSize: design.textCaption
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 86
                    radius: Math.min(6, theme.radius)
                    color: theme.background
                    border.width: 1
                    border.color: providerModels.activeFocus ? theme.accent : theme.border
                    TextArea {
                        id: providerModels
                        anchors.fill: parent
                        anchors.margins: 6
                        color: theme.text
                        placeholderText: "deepseek-chat=DeepSeek Chat\ndeepseek-reasoner=DeepSeek Reasoner"
                        placeholderTextColor: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: design.textCaption
                        wrapMode: TextArea.NoWrap
                        background: null
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    CheckBox {
                        id: makeDefault
                        text: qsTr("Modèle par défaut")
                        checked: true
                        font.pixelSize: design.textCaption
                    }
                    Item { Layout.fillWidth: true }
                    ActionButton { text: qsTr("Ajouter"); onClicked: root.saveProvider() }
                }
            }

            SettingsSection {
                title: qsTr("Composants actifs")
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Les composants actifs se pilotent directement depuis l'application.")
                    color: theme.mutedText
                    font.pixelSize: 10
                    wrapMode: Text.Wrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Button { text: "MCP"; flat: true; onClicked: backend.setView("mcp") }
                    Button { text: qsTr("Mémoire"); flat: true; onClicked: backend.setView("memory") }
                    Button { text: qsTr("Ultimate Agent"); flat: true; onClicked: backend.setView("agent") }
                    Button { text: qsTr("Installation"); flat: true; onClicked: backend.setView("installation") }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: connections.defaultModel !== "" ? qsTr("Défaut : %1").arg(connections.defaultModel) : qsTr("Aucun modèle par défaut")
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: design.textMicro
                    }
                }
            }
        }
    }
}
