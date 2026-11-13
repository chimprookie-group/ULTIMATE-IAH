import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ProviderCatalog.js" as Catalog

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(design.contentMax, width - design.space7))
    property string filterText: ""
    property string selectedCategory: qsTr("Tous")
    property string editingProviderId: ""
    property bool showDiagnostics: false
    property string editorNote: ""
    readonly property var visiblePresets: {
        var result = []
        var query = root.filterText.trim().toLowerCase()
        var category = root.selectedCategory
        for (var i = 0; i < Catalog.providers.length; ++i) {
            var preset = Catalog.providers[i]
            if (category !== qsTr("Tous") && preset.category !== category)
                continue
            if (query !== "" && (preset.name + " " + preset.id + " " + preset.base + " " + preset.note).toLowerCase().indexOf(query) < 0)
                continue
            result.push(preset)
        }
        return result
    }
    readonly property var categories: [qsTr("Tous")].concat(Catalog.categories())
    readonly property bool hasDefault: connections.defaultProvider !== ""

    function defaultLabel() {
        if (connections.defaultProvider === "")
            return qsTr("Aucun")
        for (var i = 0; i < connections.providers.length; ++i) {
            var provider = connections.providers[i]
            if (provider.id === connections.defaultProvider)
                return provider.name + " · " + connections.defaultModelId
        }
        return connections.defaultProvider + " · " + connections.defaultModelId
    }

    function stateColor(state) {
        if (state === "running") return theme.green
        if (state === "starting") return theme.yellow
        return theme.dimText
    }

    function openPreset(preset) {
        connections.clearError()
        editingProviderId = ""
        providerId.text = preset.id
        providerName.text = preset.name
        providerBase.text = preset.base
        providerKey.text = ""
        providerModels.text = preset.models
        providerApi.currentIndex = preset.api === "anthropic-messages" ? 1 : 0
        makeDefault.checked = true
        root.editorNote = preset.note
        editor.title = qsTr("Configurer %1").arg(preset.name)
        editor.open()
    }

    function openEdit(provider) {
        connections.clearError()
        editingProviderId = provider.id
        providerId.text = provider.id
        providerName.text = provider.name
        providerBase.text = provider.baseURL
        providerKey.text = ""
        providerModels.text = provider.models.join("\n")
        providerApi.currentIndex = provider.api === "anthropic-messages" ? 1 : 0
        makeDefault.checked = connections.defaultProvider === provider.id
        root.editorNote = provider.baseURL
        editor.title = qsTr("Modifier %1").arg(provider.name)
        editor.open()
    }

    function openCustom() {
        connections.clearError()
        editingProviderId = ""
        providerId.text = ""
        providerName.text = ""
        providerBase.text = ""
        providerKey.text = ""
        providerModels.text = ""
        providerApi.currentIndex = 0
        makeDefault.checked = true
        root.editorNote = qsTr("Endpoint OpenAI-compatible personnalisé.")
        editor.title = qsTr("Nouveau provider")
        editor.open()
    }

    function saveProvider() {
        if (providerName.text.trim() === "" || providerBase.text.trim() === "" || providerModels.text.trim() === "")
            return
        connections.clearError()
        connections.saveProvider(providerId.text, providerName.text, providerBase.text,
                                 providerKey.text, providerModels.text, makeDefault.checked,
                                 providerApi.currentIndex === 1 ? "anthropic-messages" : "openai-completions")
        if (connections.error === "") {
            editor.close()
            Window.window.showToast(qsTr("Provider enregistré."))
        }
    }

    function saveDeepSeek() {
        if (deepseekKey.text.trim() === "")
            return
        connections.clearError()
        connections.saveDeepSeek(deepseekKey.text)
        if (connections.error === "") {
            deepseekKey.text = ""
            Window.window.showToast(qsTr("DeepSeek enregistré."))
        }
    }

    function setDefault(provider, model) {
        connections.setDefault(provider, model)
        if (connections.error === "") {
            Window.window.showToast(qsTr("Modèle par défaut : %1/%2").arg(provider).arg(model))
        }
    }

    Component.onCompleted: if (visible) { connections.refresh(); connectionDoctor.run() }
    onVisibleChanged: if (visible) { connections.refresh(); connectionDoctor.run() }

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            width: root.contentWidth
            x: Math.max(design.space5, (parent.width - width) / 2)
            spacing: design.space4

            Item { Layout.fillWidth: true; Layout.preferredHeight: design.space4 }

            PageHeader {
                Layout.fillWidth: true
                title: qsTr("Providers")
                description: qsTr("Le centre unique des providers et des modèles de l'écosystème.")
                icon: "icons/providers.svg"
                accent: theme.orange
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

            GridLayout {
                Layout.fillWidth: true
                columns: root.contentWidth > 760 ? 4 : 2
                columnSpacing: design.space2
                rowSpacing: design.space2

                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("DeepSeek"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: connections.deepseekConfigured ? qsTr("Clé active") : qsTr("Non configuré"); color: connections.deepseekConfigured ? theme.green : theme.orange; font.pixelSize: design.textCaption; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Providers"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: connections.providers.length; color: theme.text; font.pixelSize: design.textCaption; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Modèle par défaut"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { Layout.fillWidth: true; text: root.defaultLabel(); color: connections.defaultModel !== "" ? theme.green : theme.orange; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideMiddle }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Runtime"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: backend.harnessState; color: root.stateColor(backend.harnessState); font.pixelSize: design.textCaption; font.weight: Font.DemiBold }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Clé DeepSeek")
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: connections.deepseekConfigured
                          ? qsTr("Clé active : propagée automatiquement à Ultimate Agent et au terminal.")
                          : qsTr("Ajoutez une clé API DeepSeek. Elle sera propagée automatiquement à Ultimate Agent et au terminal.")
                    color: connections.deepseekConfigured ? theme.green : theme.mutedText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space2
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
                title: qsTr("Catalogue de providers")
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Choisissez un preset : le formulaire s'ouvre déjà rempli. Vous pouvez ajuster l'URL, les modèles et la clé avant d'enregistrer.")
                    color: theme.mutedText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space2
                    SettingsField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Rechercher un provider ou un modèle…")
                        onTextChanged: root.filterText = text
                    }
                    AppComboBox {
                        Layout.preferredWidth: 150
                        model: root.categories
                        onActivated: root.selectedCategory = currentText
                    }
                    ActionButton { text: qsTr("Personnalisé"); onClicked: root.openCustom() }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("%n preset(s) disponible(s)", "", root.visiblePresets.length)
                    color: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: design.textMicro
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.contentWidth > 820 ? 3 : 2
                    columnSpacing: design.space2
                    rowSpacing: design.space2
                    Repeater {
                        model: root.visiblePresets
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 96
                            radius: design.radiusMd
                            color: presetMouse.containsMouse ? theme.raised : theme.background
                            border.width: 1
                            border.color: activeFocus || presetMouse.containsMouse ? theme.borderStrong : theme.border
                            activeFocusOnTab: true
                            Keys.onReturnPressed: root.openPreset(modelData)
                            Keys.onSpacePressed: root.openPreset(modelData)
                            Behavior on color { ColorAnimation { duration: design.durationFast } }

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: design.space3
                                spacing: 2
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: design.space1
                                    Text { Layout.fillWidth: true; text: modelData.name; color: theme.text; font.pixelSize: design.textCaption; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                    Text { text: modelData.category; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                                }
                                Text { Layout.fillWidth: true; text: modelData.note; color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { Layout.fillWidth: true; text: modelData.models.split("\n").length + " " + qsTr("modèle(s)"); color: theme.dimText; font.pixelSize: 9 }
                                    Text { text: qsTr("Configurer"); color: theme.accent; font.pixelSize: design.textMicro }
                                }
                            }
                            MouseArea {
                                id: presetMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.openPreset(modelData)
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Providers configurés")
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    visible: connections.providers.length === 0
                    text: qsTr("Aucun provider configuré. Choisissez un preset ou ajoutez un endpoint personnalisé.")
                    color: theme.dimText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }

                Repeater {
                    model: connections.providers
                    Rectangle {
                        property var providerData: modelData
                        Layout.fillWidth: true
                        implicitHeight: providerBody.implicitHeight + design.space4
                        radius: design.radiusMd
                        color: theme.background
                        border.width: 1
                        border.color: root.hasDefault && connections.defaultProvider === modelData.id ? theme.borderStrong : theme.border

                        ColumnLayout {
                            id: providerBody
                            anchors.fill: parent
                            anchors.margins: design.space3
                            spacing: design.space1

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: design.space2
                                StatusDot { color: modelData.hasKey || modelData.builtin ? theme.green : theme.yellow }
                                Text { Layout.fillWidth: true; text: modelData.name; color: theme.text; font.pixelSize: design.textLabel; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                Rectangle {
                                    visible: root.hasDefault && connections.defaultProvider === modelData.id
                                    implicitWidth: defaultLabel.implicitWidth + 12
                                    implicitHeight: 18
                                    radius: 9
                                    color: Qt.rgba(theme.green.r, theme.green.g, theme.green.b, 0.12)
                                    Text { id: defaultLabel; anchors.centerIn: parent; text: qsTr("par défaut"); color: theme.green; font.pixelSize: 8 }
                                }
                                Text { text: modelData.hasKey ? modelData.keyMasked : qsTr("sans clé"); color: modelData.hasKey ? theme.green : theme.yellow; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                                GhostButton { visible: !modelData.builtin; text: qsTr("Modifier"); flat: true; font.pixelSize: design.textMicro; onClicked: root.openEdit(modelData) }
                                GhostButton { visible: !modelData.builtin; text: qsTr("Supprimer"); flat: true; font.pixelSize: design.textMicro; onClicked: { connections.removeProvider(modelData.id) } }
                            }
                            Text { Layout.fillWidth: true; text: modelData.baseURL + "  ·  " + (modelData.api !== undefined ? modelData.api : "openai-completions"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideMiddle }
                            Flow {
                                Layout.fillWidth: true
                                spacing: 4
                                visible: !modelData.builtin
                                Repeater {
                                    model: modelData.models
                                    GhostButton {
                                        text: modelData
                                        flat: true
                                        font.pixelSize: design.textMicro
                                        highlighted: connections.defaultProvider === providerData.id && connections.defaultModelId === modelData
                                        onClicked: root.setDefault(providerData.id, modelData)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Diagnostic des liaisons")
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: connectionDoctor.summary !== "" ? connectionDoctor.summary : qsTr("Vérifiez la clé DeepSeek, les providers, MCP et le workspace."); color: connectionDoctor.failed > 0 ? theme.orange : theme.mutedText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                    GhostButton { text: root.showDiagnostics ? qsTr("Masquer") : qsTr("Afficher"); flat: true; font.pixelSize: design.textCaption; onClicked: root.showDiagnostics = !root.showDiagnostics }
                    ActionButton { text: connectionDoctor.running ? qsTr("Vérification…") : qsTr("Vérifier"); enabled: !connectionDoctor.running; onClicked: connectionDoctor.run() }
                }
                Repeater {
                    model: root.showDiagnostics ? connectionDoctor.checks : []
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: checkBody.implicitHeight + design.space2
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
                            Text { Layout.preferredWidth: 72; text: modelData.layer; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Text { Layout.fillWidth: true; text: modelData.label; color: theme.text; font.pixelSize: design.textCaption }
                                Text { Layout.fillWidth: true; text: modelData.hint !== "" ? modelData.hint : modelData.detail; color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                            }
                            Text { text: modelData.ok ? qsTr("ok") : qsTr("à corriger"); color: modelData.ok ? theme.green : theme.orange; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Composants de l'écosystème")
                Layout.fillWidth: true
                Flow {
                    Layout.fillWidth: true
                    spacing: design.space2
                    GhostButton { text: "MCP"; flat: true; onClicked: backend.setView("mcp") }
                    GhostButton { text: qsTr("Mémoire"); flat: true; onClicked: backend.setView("memory") }
                    GhostButton { text: qsTr("Bots"); flat: true; onClicked: backend.setView("bots") }
                    GhostButton { text: qsTr("Ultimate Agent"); flat: true; onClicked: backend.setView("agent") }
                    GhostButton { text: qsTr("Installation"); flat: true; onClicked: backend.setView("installation") }
                }
            }
        }
    }

    Dialog {
        id: editor
        modal: true
        width: Math.min(640, root.width - design.space7)
        height: Math.min(680, root.height - design.space7)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space5
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            Text { text: editor.title; color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold; elide: Text.ElideRight }
            Text { Layout.fillWidth: true; text: root.editorNote; color: theme.dimText; font.pixelSize: design.textMicro; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight }
            RowLayout {
                Layout.fillWidth: true
                spacing: design.space2
                SettingsField { id: providerId; Layout.fillWidth: true; placeholderText: qsTr("ID (minuscules)") ; readOnly: root.editingProviderId !== "" }
                SettingsField { id: providerName; Layout.fillWidth: true; placeholderText: qsTr("Nom affiché") }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: design.space2
                SettingsField { id: providerBase; Layout.fillWidth: true; placeholderText: qsTr("URL de base https://…") }
                AppComboBox { id: providerApi; Layout.preferredWidth: 170; model: ["OpenAI-compatible", "Anthropic Messages"]; font.pixelSize: design.textCaption }
            }
            SettingsField { id: providerKey; Layout.fillWidth: true; placeholderText: qsTr("Clé API (optionnelle pour un endpoint local)"); echoMode: TextInput.Password }
            Text { text: qsTr("Modèles, un par ligne : id=Nom affiché"); color: theme.dimText; font.pixelSize: design.textMicro }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 112
                color: theme.background
                radius: design.radiusSm
                border.width: 1
                border.color: providerModels.activeFocus ? theme.accent : theme.border
                TextArea {
                    id: providerModels
                    anchors.fill: parent
                    anchors.margins: 6
                    color: theme.text
                    placeholderText: "model-id=Nom affiché"
                    placeholderTextColor: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: design.textCaption
                    wrapMode: TextArea.NoWrap
                    background: null
                }
            }
            RowLayout {
                Layout.fillWidth: true
                AppCheckBox { id: makeDefault; text: qsTr("Définir comme modèle par défaut"); checked: true; font.pixelSize: design.textCaption }
                Item { Layout.fillWidth: true }
                Text { visible: connections.error !== ""; Layout.maximumWidth: 240; text: connections.error; color: theme.red; font.pixelSize: design.textMicro; elide: Text.ElideRight }
            }
            RowLayout {
                Layout.fillWidth: true
                GhostButton { visible: root.editingProviderId !== ""; text: qsTr("Supprimer"); flat: true; onClicked: { connections.removeProvider(root.editingProviderId); editor.close() } }
                Item { Layout.fillWidth: true }
                GhostButton { text: qsTr("Annuler"); flat: true; onClicked: editor.close() }
                ActionButton { text: qsTr("Enregistrer"); onClicked: root.saveProvider() }
            }
        }
    }
}
