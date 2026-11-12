import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(340, Math.min(1000, width - 64))

    function saveDeepSeek() {
        connections.saveDeepSeek(deepseekKey.text)
        if (connections.error === "") {
            deepseekKey.text = ""
            backend.restartHarness()
            Window.window.showToast(qsTr("DeepSeek enregistré. Le harness redémarre."))
        }
    }

    function saveProvider() {
        connections.saveProvider(providerId.text, providerName.text, providerBase.text,
                                 providerKey.text, providerModels.text, makeDefault.checked)
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

    Component.onCompleted: if (visible) connections.refresh()
    onVisibleChanged: if (visible) connections.refresh()

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: root.contentWidth
            x: Math.max(24, (parent.width - width) / 2)
            spacing: 14

            Item { Layout.fillWidth: true; Layout.preferredHeight: 26 }

            RowLayout {
                Layout.fillWidth: true
                spacing: 14
                Item {
                    width: 54
                    height: 54
                    Rectangle { anchors.centerIn: parent; width: 52; height: 52; radius: 26; color: theme.green; opacity: 0.12 }
                    ThemedIcon { anchors.centerIn: parent; source: "icons/providers.svg"; width: 22; height: 22; color: theme.green }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: qsTr("Connexions")
                        color: theme.brightText
                        font.family: "iA Writer Duospace"
                        font.pixelSize: 26
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Les providers et les outils que le harness peut réellement utiliser.")
                        color: theme.mutedText
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                    }
                }
                ActionButton { text: qsTr("Rafraîchir"); onClicked: connections.refresh() }
            }

            Text {
                Layout.fillWidth: true
                visible: connections.error !== ""
                text: connections.error
                color: theme.red
                font.pixelSize: 9
                wrapMode: Text.Wrap
            }

            SettingsSection {
                title: qsTr("DeepSeek")
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: connections.deepseekConfigured
                          ? qsTr("Une clé DeepSeek est enregistrée dans le credential store du harness.")
                          : qsTr("Ajoutez une clé API DeepSeek. Elle est stockée dans les credentials DSH, jamais dans QML.")
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
                                    font.pixelSize: 8
                                }
                                Button {
                                    visible: !modelData.builtin
                                    text: qsTr("Supprimer")
                                    flat: true
                                    font.pixelSize: 8
                                    onClicked: {
                                        connections.removeProvider(modelData.id)
                                        backend.restartHarness()
                                    }
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.baseURL
                                color: theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: 8
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
                                        font.pixelSize: 8
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
                SettingsField {
                    id: providerBase
                    Layout.fillWidth: true
                    placeholderText: qsTr("URL de base (ex. https://api.deepseek.com/v1)")
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
                    font.pixelSize: 9
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
                        font.pixelSize: 9
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
                        font.pixelSize: 9
                    }
                    Item { Layout.fillWidth: true }
                    ActionButton { text: qsTr("Ajouter"); onClicked: root.saveProvider() }
                }
            }

            SettingsSection {
                title: qsTr("Intégrations")
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: qsTr("9Router peut exposer ses modèles au harness. MausBot expose son équipe par MCP.")
                    color: theme.mutedText
                    font.pixelSize: 10
                    wrapMode: Text.Wrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Button { text: "9Router"; flat: true; onClicked: backend.setView("ninerouter") }
                    Button { text: "MausBot"; flat: true; onClicked: backend.setView("mausbot") }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: connections.defaultModel !== "" ? qsTr("Défaut : %1").arg(connections.defaultModel) : qsTr("Aucun modèle par défaut")
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: 8
                    }
                }
            }
        }
    }
}
