import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(design.contentMax, width - design.space7))

    function resetForm() {
        nameField.text = ""
        commandField.text = ""
        argsArea.text = ""
        envArea.text = ""
        urlField.text = ""
        headersArea.text = ""
        transportBox.currentIndex = 0
    }

    function addServer() {
        mcpManager.addServer(nameField.text, transportBox.currentText === "HTTP" ? "streamable-http" : "stdio",
                             commandField.text, argsArea.text, envArea.text,
                             urlField.text, headersArea.text)
        if (mcpManager.error === "") {
            root.resetForm()
            Window.window.showToast(qsTr("Serveur MCP ajouté. Redémarrez le harness."))
        }
    }

    function presetFilesystem() {
        nameField.text = "filesystem"
        transportBox.currentIndex = 0
        commandField.text = "npx"
        argsArea.text = "-y\n@modelcontextprotocol/server-filesystem\n" + backend.workspace
    }

    function presetFetch() {
        nameField.text = "fetch"
        transportBox.currentIndex = 0
        commandField.text = "uvx"
        argsArea.text = "mcp-server-fetch"
    }

    function presetGit() {
        nameField.text = "git"
        transportBox.currentIndex = 0
        commandField.text = "uvx"
        argsArea.text = "mcp-server-git\n--repository\n" + backend.workspace
    }

    Component.onCompleted: if (visible) mcpManager.refresh()
    onVisibleChanged: if (visible) mcpManager.refresh()

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
                title: qsTr("MCP")
                description: qsTr("Les outils et sources de données que le harness peut appeler, en stdio ou en HTTP.")
                icon: "icons/mcp.svg"
                accent: theme.green
                ActionButton {
                    text: qsTr("Redémarrer le harness")
                    onClicked: backend.restartHarness()
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Les serveurs MCP sont exécutés par le harness avec les permissions de votre session. N'activez que des commandes que vous contrôlez.")
                color: theme.orange
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }

            SettingsSection {
                title: qsTr("Intégrés")
                Layout.fillWidth: true

                Repeater {
                    model: mcpManager.builtins
                    Rectangle {
                        Layout.fillWidth: true
                        height: 54
                        radius: Math.min(8, theme.radius)
                        color: theme.surface
                        border.width: 1
                        border.color: theme.border
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8
                            StatusDot { color: modelData.active ? theme.green : theme.dimText }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text { text: modelData.label; color: theme.text; font.pixelSize: 10; font.weight: Font.DemiBold }
                                Text { Layout.fillWidth: true; text: modelData.detail; color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                            }
                            Text {
                                text: modelData.active ? qsTr("actif") : qsTr("inactif")
                                color: modelData.active ? theme.green : theme.dimText
                                font.pixelSize: design.textMicro
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Ajouter un serveur")
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space3

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text { text: qsTr("Nom"); color: theme.dimText; font.pixelSize: design.textCaption }
                        SettingsField {
                            id: nameField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Nom — ex. filesystem")
                        }
                    }
                    ColumnLayout {
                        Layout.preferredWidth: 160
                        Layout.maximumWidth: 180
                        spacing: 3
                        Text { text: qsTr("Transport"); color: theme.dimText; font.pixelSize: design.textCaption }
                        ComboBox {
                            id: transportBox
                            Layout.fillWidth: true
                            model: ["stdio", "HTTP"]
                            font.pixelSize: design.textCaption
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    visible: transportBox.currentIndex === 0
                    Text { text: qsTr("Commande"); color: theme.dimText; font.pixelSize: design.textCaption }
                    SettingsField {
                        id: commandField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Commande — ex. npx, uvx")
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    visible: transportBox.currentIndex === 1
                    Text { text: qsTr("URL du serveur"); color: theme.dimText; font.pixelSize: design.textCaption }
                    SettingsField {
                        id: urlField
                        Layout.fillWidth: true
                        placeholderText: qsTr("URL MCP — ex. https://serveur/mcp")
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 8
                    rowSpacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 88
                        visible: transportBox.currentIndex === 0
                        radius: Math.min(6, theme.radius)
                        color: theme.background
                        border.width: 1
                        border.color: argsArea.activeFocus ? theme.accent : theme.border
                        TextArea {
                            id: argsArea
                            anchors.fill: parent
                            anchors.margins: 6
                            color: theme.text
                            font.family: backend.monoFont
                            font.pixelSize: design.textCaption
                            wrapMode: TextArea.Wrap
                            background: null
                        }
                        Text {
                            anchors.fill: parent
                            anchors.margins: 9
                            visible: argsArea.text === ""
                            text: qsTr("Arguments, un par ligne")
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textCaption
                            wrapMode: Text.Wrap
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 88
                        visible: transportBox.currentIndex === 0
                        radius: Math.min(6, theme.radius)
                        color: theme.background
                        border.width: 1
                        border.color: envArea.activeFocus ? theme.accent : theme.border
                        TextArea {
                            id: envArea
                            anchors.fill: parent
                            anchors.margins: 6
                            color: theme.text
                            font.family: backend.monoFont
                            font.pixelSize: design.textCaption
                            wrapMode: TextArea.Wrap
                            background: null
                        }
                        Text {
                            anchors.fill: parent
                            anchors.margins: 9
                            visible: envArea.text === ""
                            text: qsTr("Env KEY=VALUE, une par ligne")
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textCaption
                            wrapMode: Text.Wrap
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 66
                        visible: transportBox.currentIndex === 1
                        radius: Math.min(6, theme.radius)
                        color: theme.background
                        border.width: 1
                        border.color: headersArea.activeFocus ? theme.accent : theme.border
                        TextArea {
                            id: headersArea
                            anchors.fill: parent
                            anchors.margins: 6
                            color: theme.text
                            font.family: backend.monoFont
                            font.pixelSize: design.textCaption
                            wrapMode: TextArea.Wrap
                            background: null
                        }
                        Text {
                            anchors.fill: parent
                            anchors.margins: 9
                            visible: headersArea.text === ""
                            text: qsTr("Headers Authorization=…, un par ligne")
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textCaption
                            wrapMode: Text.Wrap
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text { text: qsTr("Exemples :"); color: theme.dimText; font.pixelSize: 8 }
                    Button { text: "Filesystem"; flat: true; font.pixelSize: design.textMicro; onClicked: root.presetFilesystem() }
                    Button { text: "Fetch"; flat: true; font.pixelSize: design.textMicro; onClicked: root.presetFetch() }
                    Button { text: "Git"; flat: true; font.pixelSize: design.textMicro; onClicked: root.presetGit() }
                    Item { Layout.fillWidth: true }
                    ActionButton { text: qsTr("Ajouter"); onClicked: root.addServer() }
                }
                Text {
                    Layout.fillWidth: true
                    visible: mcpManager.error !== ""
                    text: mcpManager.error
                    color: theme.red
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }
            }

            Text {
                Layout.fillWidth: true
                visible: mcpManager.servers.length === 0
                text: qsTr("Aucun serveur personnalisé. Ajoutez-en un ou utilisez un exemple.")
                color: theme.dimText
                font.pixelSize: design.textCaption
            }

            Repeater {
                model: mcpManager.servers
                Rectangle {
                    property var serverData: modelData
                    Layout.fillWidth: true
                    height: 78
                    radius: Math.min(8, theme.radius)
                    color: theme.surface
                    border.width: 1
                    border.color: modelData.enabled ? theme.green : theme.border
                    ColumnLayout {
                        x: 12
                        y: 9
                        width: parent.width - 24
                        spacing: 3
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7
                            StatusDot { color: modelData.enabled ? theme.green : theme.dimText }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.name + "  ·  " + modelData.transport
                                color: theme.text
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                            }
                            Switch {
                                checked: modelData.enabled
                                onToggled: {
                                    mcpManager.setEnabled(modelData.id, checked)
                                    backend.restartHarness()
                                }
                            }
                            Button {
                                text: qsTr("Supprimer")
                                flat: true
                                font.pixelSize: design.textMicro
                                onClicked: {
                                    mcpManager.removeServer(modelData.id)
                                    backend.restartHarness()
                                }
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelData.transport === "stdio"
                                  ? (modelData.command + " " + modelData.args.join(" "))
                                  : modelData.url
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textMicro
                            elide: Text.ElideMiddle
                        }
                    }
                }
            }
        }
    }
}
