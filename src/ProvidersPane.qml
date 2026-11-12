import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(340, Math.min(1000, width - 64))
    readonly property int cardWidth: contentWidth >= 780 ? Math.floor((contentWidth - 12) / 2) : contentWidth

    property bool editorOpen: false
    property bool editing: false
    property string draftId: ""
    property string draftName: ""
    property string draftBase: ""
    property string draftKey: ""
    property string draftModels: ""
    property string testTarget: ""
    property string confirmRemoveId: ""

    readonly property var presets: [
        { "id": "deepseek-direct", "name": "DeepSeek", "base": "https://api.deepseek.com/v1",
          "models": "deepseek-chat=DeepSeek Chat\ndeepseek-reasoner=DeepSeek Reasoner" },
        { "id": "openai-direct", "name": "OpenAI", "base": "https://api.openai.com/v1",
          "models": "gpt-4o-mini=GPT-4o mini\ngpt-4.1=GPT-4.1" },
        { "id": "groq", "name": "Groq", "base": "https://api.groq.com/openai/v1",
          "models": "llama-3.3-70b-versatile=Llama 3.3 70B" },
        { "id": "mistral", "name": "Mistral", "base": "https://api.mistral.ai/v1",
          "models": "mistral-large-latest=Mistral Large" },
        { "id": "openrouter-api", "name": "OpenRouter", "base": "https://openrouter.ai/api/v1",
          "models": "openai/gpt-4o-mini=GPT-4o mini" },
        { "id": "ollama", "name": "Ollama", "base": "http://127.0.0.1:11434/v1",
          "models": "llama3.2=Llama 3.2" },
        { "id": "custom", "name": "Custom", "base": "", "models": "" }
    ]

    function refresh() {
        providers.refresh()
        providers.refreshAccounts()
    }

    function openAdd() {
        providers.clearError()
        providers.clearTest()
        editing = false
        draftId = ""
        draftName = ""
        draftBase = ""
        draftKey = ""
        draftModels = ""
        editorOpen = true
        Qt.callLater(function () { idField.forceActiveFocus() })
    }

    function closeEditor() {
        draftKey = ""
        editorOpen = false
    }

    function applyPreset(preset) {
        draftId = preset.id === "custom" ? "" : preset.id
        draftName = preset.id === "custom" ? "" : preset.name
        draftBase = preset.base
        draftModels = preset.models
        draftKey = ""
        providers.clearError()
    }

    function openEdit(provider) {
        providers.clearError()
        providers.clearTest()
        editing = true
        draftId = provider.id
        draftName = provider.name
        draftBase = provider.baseURL
        draftKey = ""
        let models = ""
        for (let i = 0; i < provider.models.length; ++i) {
            const model = provider.models[i]
            models += model.id + "=" + model.name + (i + 1 < provider.models.length ? "\n" : "")
        }
        draftModels = models
        editorOpen = true
    }

    function modelsText(provider) {
        let out = ""
        for (let i = 0; i < provider.models.length; ++i) {
            const model = provider.models[i]
            out += (i > 0 ? "\n" : "") + model.id
        }
        return out
    }

    function save() {
        providers.saveProvider(draftId, draftName, draftBase, draftKey, draftModels)
        if (providers.error === "") {
            draftKey = ""
            editorOpen = false
            providers.restartService()
            backend.restartOpencode()
            Window.window.showToast(qsTr("Provider saved. OpenCode is restarting."))
        }
    }

    function removeProvider(provider) {
        if (confirmRemoveId !== provider.id) {
            confirmRemoveId = provider.id
            Window.window.showToast(qsTr("Click again to remove this provider."))
            return
        }
        confirmRemoveId = ""
        providers.removeProvider(provider.id)
        providers.restartService()
        backend.restartOpencode()
    }

    function applyProvider() {
        providers.restartService()
        backend.restartOpencode()
        Window.window.showToast(qsTr("OpenCode is restarting with the new providers."))
    }

    function useModel(provider, model) {
        if (backend.opencodeState !== "ready") {
            Window.window.showToast(qsTr("OpenCode is not ready yet."))
            return
        }
        backend.setConfigOption("model", provider.id + "/" + model.id)
        Window.window.showToast(qsTr("Model selected: %1").arg(model.id))
    }

    function testProvider(provider) {
        testTarget = provider.id
        providers.testProvider(provider.id)
    }

    function reconnectAccount(account) {
        const name = String(account.name).replace(/["\\$`]/g, "")
        terminal.run("opencode auth logout \"" + name + "\"; opencode auth login \"" + name + "\" && opencode service restart")
        backend.setView("terminal")
    }

    function connectAccount() {
        terminal.run("opencode auth login && opencode service restart")
        backend.setView("terminal")
    }

    Shortcut {
        sequence: "Escape"
        enabled: root.editorOpen
        onActivated: root.closeEditor()
    }

    Component.onCompleted: if (visible) Qt.callLater(refresh)
    onVisibleChanged: if (visible) refresh()

    ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Column {
            id: content
            width: root.contentWidth
            x: Math.max(24, (scroll.availableWidth - width) / 2)
            topPadding: 26
            bottomPadding: 32
            spacing: 14

            RowLayout {
                width: parent.width
                spacing: 14

                Item {
                    width: 56
                    height: 56
                    Layout.alignment: Qt.AlignTop
                    Rectangle {
                        anchors.centerIn: parent
                        width: 54
                        height: 54
                        radius: 27
                        color: theme.accent
                        opacity: theme.dark ? 0.12 : 0.07
                    }
                    ThemedIcon {
                        anchors.centerIn: parent
                        source: "icons/providers.svg"
                        width: 24
                        height: 24
                        color: theme.accent
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Text {
                        text: qsTr("Providers")
                        color: theme.brightText
                        font.pixelSize: 25
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Any OpenAI-compatible API, plus your OpenCode accounts. One place, no terminal required.")
                        color: theme.mutedText
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                    }
                }

                Button {
                    text: qsTr("Refresh")
                    flat: true
                    onClicked: root.refresh()
                }
                ActionButton {
                    text: qsTr("Add provider")
                    onClicked: root.openAdd()
                }
            }

            Text {
                width: parent.width
                text: qsTr("Keys are stored locally in %1 (mode 600) and only a masked suffix is shown.").arg(providers.configPath)
                color: theme.dimText
                font.family: backend.monoFont
                font.pixelSize: 8
                elide: Text.ElideMiddle
            }

            Flow {
                width: parent.width
                spacing: 12

                Repeater {
                    model: providers.providers

                    HomeCard {
                        property var providerEntry: modelData
                        title: modelData.name
                        icon: "icons/providers.svg"
                        accent: modelData.custom ? theme.green : theme.blue
                        cardWidth: root.cardWidth

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Rectangle {
                                width: 32
                                height: 32
                                radius: 16
                                color: theme.accent
                                opacity: 0.12
                                Text {
                                    anchors.centerIn: parent
                                    text: String(modelData.name).slice(0, 1).toUpperCase()
                                    color: theme.accent
                                    font.pixelSize: 14
                                    font.weight: Font.Bold
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.id
                                    color: theme.text
                                    font.family: backend.monoFont
                                    font.pixelSize: 9
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.baseURL
                                    color: theme.dimText
                                    font.family: backend.monoFont
                                    font.pixelSize: 8
                                    elide: Text.ElideMiddle
                                }
                            }
                            Text {
                                text: modelData.hasKey ? modelData.keyMasked : qsTr("no key")
                                color: modelData.hasKey ? theme.green : theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: 8
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 4

                            Repeater {
                                model: modelData.models
                                Button {
                                    text: modelData.name
                                    flat: true
                                    font.pixelSize: 9
                                    onClicked: root.useModel(providerEntry, modelData)
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: root.testTarget === modelData.id
                            text: providers.testing ? qsTr("Testing…") : providers.testMessage
                            color: providers.testing ? theme.dimText
                                   : ((providers.testMessage.indexOf("Connected") >= 0
                                       || providers.testMessage.indexOf("Connecté") >= 0)
                                      ? theme.green : theme.red)
                            font.pixelSize: 8
                            wrapMode: Text.Wrap
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 3

                            Button {
                                text: qsTr("Test")
                                flat: true
                                font.pixelSize: 9
                                enabled: !providers.testing
                                onClicked: root.testProvider(modelData)
                            }
                            Button {
                                text: qsTr("Edit")
                                flat: true
                                font.pixelSize: 9
                                onClicked: root.openEdit(modelData)
                            }
                            Item { Layout.fillWidth: true }
                            Button {
                                text: root.confirmRemoveId === modelData.id ? qsTr("Confirm") : qsTr("Remove")
                                flat: true
                                font.pixelSize: 9
                                onClicked: root.removeProvider(modelData)
                            }
                        }
                    }
                }
            }

            Text {
                width: parent.width
                visible: providers.providers.length === 0
                text: qsTr("No custom provider yet. Add DeepSeek, OpenAI, Groq, Mistral, OpenRouter, Ollama or any OpenAI-compatible endpoint.")
                color: theme.mutedText
                font.pixelSize: 10
                wrapMode: Text.Wrap
            }

            HomeCard {
                title: qsTr("OpenCode accounts")
                icon: "icons/plug.svg"
                accent: theme.cyan
                cardWidth: root.contentWidth

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Accounts connected through OpenCode auth (OAuth or built-in providers).")
                    color: theme.mutedText
                    font.pixelSize: 9
                    wrapMode: Text.Wrap
                }

                Repeater {
                    model: providers.accounts
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        StatusDot { color: theme.yellow }
                        Text {
                            Layout.fillWidth: true
                            text: modelData.name
                            color: theme.text
                            font.pixelSize: 10
                        }
                        Text {
                            text: modelData.status
                            color: theme.dimText
                            font.pixelSize: 8
                        }
                        Button {
                            text: qsTr("Reconnect")
                            flat: true
                            font.pixelSize: 9
                            onClicked: root.reconnectAccount(modelData)
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: providers.accounts.length === 0
                    text: qsTr("No OpenCode account connected.")
                    color: theme.dimText
                    font.pixelSize: 9
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Button {
                        text: qsTr("Connect an account")
                        flat: true
                        font.pixelSize: 9
                        onClicked: root.connectAccount()
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: qsTr("Apply to OpenCode")
                        flat: true
                        font.pixelSize: 9
                        onClicked: root.applyProvider()
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.editorOpen
        color: "#000000"
        opacity: 0.58
        z: 190
        MouseArea {
            anchors.fill: parent
            onClicked: root.closeEditor()
        }
    }

    Rectangle {
        visible: root.editorOpen
        width: Math.min(680, root.width - 60)
        height: Math.min(600, root.height - 40)
        anchors.centerIn: parent
        radius: Math.min(14, theme.radius)
        color: theme.surface
        border.width: 1
        border.color: theme.border
        z: 200

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 9

            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: root.editing ? qsTr("Edit provider") : qsTr("Add provider")
                    color: theme.brightText
                    font.pixelSize: 18
                    font.weight: Font.Bold
                }
                IconButton {
                    icon: "icons/close.svg"
                    hint: qsTr("Close")
                    onClicked: root.closeEditor()
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 4
                visible: !root.editing

                Repeater {
                    model: root.presets
                    Button {
                        text: modelData.name
                        flat: true
                        font.pixelSize: 9
                        onClicked: root.applyPreset(modelData)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                SettingsField {
                    id: idField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Provider ID (e.g. deepseek-direct)")
                    text: root.draftId
                    enabled: !root.editing
                    onTextChanged: root.draftId = text
                }
                SettingsField {
                    id: nameField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Display name")
                    text: root.draftName
                    onTextChanged: root.draftName = text
                }
            }

            SettingsField {
                id: baseField
                Layout.fillWidth: true
                placeholderText: qsTr("Base URL (e.g. https://api.deepseek.com/v1)")
                text: root.draftBase
                onTextChanged: root.draftBase = text
            }

            SettingsField {
                id: keyField
                Layout.fillWidth: true
                placeholderText: root.editing ? qsTr("Leave empty to keep the current key") : qsTr("API key")
                echoMode: TextInput.Password
                text: root.draftKey
                onTextChanged: root.draftKey = text
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Models, one per line. Use id=Display Name when needed.")
                color: theme.mutedText
                font.pixelSize: 9
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 112
                radius: Math.min(6, theme.radius)
                color: theme.background
                border.width: 1
                border.color: modelsArea.activeFocus ? theme.accent : theme.border

                Text {
                    anchors.fill: parent
                    anchors.margins: 8
                    visible: modelsArea.text === ""
                    text: "deepseek-chat=DeepSeek Chat\ndeepseek-reasoner=DeepSeek Reasoner"
                    color: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: 10
                    wrapMode: Text.Wrap
                    enabled: false
                }

                TextArea {
                    id: modelsArea
                    anchors.fill: parent
                    anchors.margins: 6
                    color: theme.text
                    placeholderText: "deepseek-chat=DeepSeek Chat\ndeepseek-reasoner=DeepSeek Reasoner"
                    placeholderTextColor: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: 10
                    wrapMode: TextArea.NoWrap
                    background: null
                    text: root.draftModels
                    onTextChanged: root.draftModels = text
                }
            }

            Text {
                Layout.fillWidth: true
                visible: providers.error !== ""
                text: providers.error
                color: theme.red
                font.pixelSize: 9
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("Cancel")
                    flat: true
                    onClicked: root.closeEditor()
                }
                ActionButton {
                    text: qsTr("Save and apply")
                    onClicked: root.save()
                }
            }
        }
    }
}
