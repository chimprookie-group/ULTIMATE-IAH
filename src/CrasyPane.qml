import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(1180, width - design.space7))
    property string routePrompt: ""
    property string routeResult: ""
    property string modelQuery: ""
    property string testResult: ""
    property var editingChannel: ({})

    function useInHarness() {
        connections.saveProvider("4crasy", "4Crasy", crasy.endpoint, crasy.token,
                                 "auto", true)
        ultimateAgent.refreshEndpoint()
        Window.window.showToast(qsTr("4Crasy est maintenant le provider par défaut."))
    }

    function previewRoute() {
        routeResult = crasy.routePreview(routePrompt, "auto")
    }

    function openAddChannel() {
        editingChannel = ({})
        channelId.text = ""
        channelId.editable = true
        channelName.text = ""
        channelBase.text = "https://"
        channelEnv.text = ""
        channelKey.text = ""
        channelPriority.value = 10
        channelFree.checked = true
        channelEnabled.checked = true
        testResult = ""
        deleteChannel.visible = false
        channelDialog.channelTitle = qsTr("Ajouter un canal")
        channelDialog.open()
    }

    function openEditChannel(channel) {
        editingChannel = channel
        channelId.text = channel.id
        channelId.editable = false
        channelName.text = channel.name
        channelBase.text = channel.baseUrl
        channelEnv.text = channel.apiKeyEnv
        channelKey.text = ""
        channelPriority.value = channel.priority
        channelFree.checked = channel.free
        channelEnabled.checked = channel.enabled
        testResult = channel.hasKey ? "" : qsTr("Clé API absente")
        deleteChannel.visible = true
        channelDialog.channelTitle = qsTr("Configurer %1").arg(channel.name)
        channelDialog.open()
    }

    function saveChannel() {
        const ok = crasy.saveChannel(channelId.text, channelName.text, channelBase.text,
                                     channelEnv.text, channelKey.text, channelPriority.value,
                                     channelFree.checked, channelEnabled.checked)
        if (ok) {
            channelDialog.close()
            Window.window.showToast(qsTr("Canal enregistré."))
        }
    }

    function saveRouting() {
        crasy.saveSettings(parseInt(portField.text) || crasy.port, freeOnly.checked, fallbackSpin.value,
                           autoStart.checked,
                           strategyBox.currentIndex === 0 ? "free"
                           : (strategyBox.currentIndex === 2 ? "quality" : "balanced"))
        Window.window.showToast(qsTr("Configuration 4Crasy enregistrée."))
    }

    Component.onCompleted: crasy.refresh()
    onVisibleChanged: if (visible) crasy.refresh()

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
                title: "4Crasy"
                description: qsTr("Routeur de modèles gratuits, fallback automatique et API OpenAI-compatible intégrée au harness.")
                icon: "icons/4crasy.svg"
                accent: theme.accent
                ActionButton { text: qsTr("Rafraîchir"); onClicked: crasy.refresh() }
                ActionButton { text: qsTr("Démarrer l'API"); visible: !crasy.running; onClicked: crasy.start() }
                ActionButton { text: qsTr("Arrêter"); visible: crasy.running; onClicked: crasy.stop() }
            }

            Text {
                Layout.fillWidth: true
                visible: crasy.error !== ""
                text: crasy.error
                color: theme.red
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.contentWidth > 820 ? 4 : 2
                columnSpacing: design.space2
                rowSpacing: design.space2
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("API"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: crasy.running ? qsTr("en ligne") : qsTr("arrêtée"); color: crasy.running ? theme.green : theme.dimText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Modèles libres"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: crasy.freeModelCount; color: theme.accent; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Requêtes"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: crasy.requestCount; color: theme.text; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Canaux"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: crasy.channels.filter(function (c) { return c.enabled }).length + " / " + crasy.channels.length; color: theme.text; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Passerelle locale")
                collapsible: true
                expanded: true
                Layout.fillWidth: true
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: crasy.endpoint; color: theme.text; font.family: backend.monoFont; font.pixelSize: design.textCaption; elide: Text.ElideMiddle }
                    StatusDot { size: 5; color: crasy.running ? theme.green : theme.dimText }
                    Text { text: crasy.running ? qsTr("prêt") : qsTr("démarrage requis"); color: crasy.running ? theme.green : theme.dimText; font.pixelSize: design.textMicro }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("Jeton Bearer : ••••••••%1").arg(crasy.token.substring(Math.max(0, crasy.token.length - 4))); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                    Button { text: qsTr("Copier le jeton"); flat: true; font.pixelSize: design.textMicro; onClicked: { crasy.copyToken(); Window.window.showToast(qsTr("Jeton 4Crasy copié.")) } }
                    ActionButton { text: qsTr("Utiliser dans le harness"); enabled: crasy.running; onClicked: root.useInHarness() }
                }
                Text { Layout.fillWidth: true; text: qsTr("L'API expose /v1/models et /v1/chat/completions. Elle n'écoute que sur la loopback et exige le jeton."); color: theme.dimText; font.pixelSize: design.textMicro; wrapMode: Text.Wrap }
            }

            SettingsSection {
                title: qsTr("Routage automatique")
                collapsible: true
                expanded: true
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Choisissez la stratégie, le nombre de secours et la priorité des modèles gratuits."); color: theme.dimText; font.pixelSize: design.textMicro; wrapMode: Text.Wrap }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space3
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("Stratégie"); color: theme.dimText; font.pixelSize: design.textMicro }
                        ComboBox {
                            id: strategyBox
                            Layout.fillWidth: true
                            model: [qsTr("Free"), qsTr("Balanced"), qsTr("Quality")]
                            currentIndex: crasy.strategy === "free" ? 0 : (crasy.strategy === "quality" ? 2 : 1)
                            onActivated: crasy.strategy = currentIndex === 0 ? "free" : (currentIndex === 2 ? "quality" : "balanced")
                        }
                    }
                    ColumnLayout {
                        Text { text: qsTr("Fallbacks"); color: theme.dimText; font.pixelSize: design.textMicro }
                        SpinBox { id: fallbackSpin; from: 0; to: 5; value: crasy.maxFallbacks; editable: true; Layout.preferredWidth: 82 }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("Port API"); color: theme.dimText; font.pixelSize: design.textMicro }
                        SettingsField { id: portField; Layout.fillWidth: true; text: crasy.port; inputMethodHints: Qt.ImhDigitsOnly }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    CheckBox { id: freeOnly; text: qsTr("Uniquement les modèles gratuits"); checked: crasy.freeOnly }
                    CheckBox { id: autoStart; text: qsTr("Démarrer avec l'application"); checked: crasy.autoStart }
                    Item { Layout.fillWidth: true }
                    Button { text: qsTr("Enregistrer"); onClicked: root.saveRouting() }
                }
                Text { Layout.fillWidth: true; text: crasy.configPath; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8; elide: Text.ElideMiddle }
            }

            SettingsSection {
                title: qsTr("Canaux")
                summary: qsTr("%1 actif(s) · %2 modèle(s)").arg(crasy.channels.filter(function (c) { return c.enabled }).length).arg(crasy.models.length)
                collapsible: true
                expanded: false
                Layout.fillWidth: true
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("Chaque canal peut être configuré, testé, priorisé ou retiré."); color: theme.dimText; font.pixelSize: design.textMicro }
                    Button { text: qsTr("Ajouter un canal"); flat: true; font.pixelSize: design.textMicro; onClicked: root.openAddChannel() }
                    Button { text: qsTr("Découvrir"); flat: true; font.pixelSize: design.textMicro; onClicked: crasy.refreshModels() }
                }
                Repeater {
                    model: crasy.channels
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: channelRow.implicitHeight + design.space2
                        radius: design.radiusSm
                        color: theme.background
                        border.width: 1
                        border.color: modelData.enabled ? theme.border : theme.borderStrong
                        RowLayout {
                            id: channelRow
                            anchors.fill: parent
                            anchors.margins: design.space2
                            spacing: design.space2
                            StatusDot { size: 4; color: modelData.enabled && modelData.hasKey ? theme.green : theme.dimText }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Text { text: modelData.name; color: theme.text; font.pixelSize: design.textMicro; font.weight: Font.DemiBold }
                                Text { text: modelData.baseUrl + " · p" + modelData.priority + " · " + modelData.modelCount + qsTr(" modèle(s)"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8; elide: Text.ElideRight }
                            }
                            Text { text: modelData.hasKey ? "" : qsTr("clé absente"); color: theme.orange; font.pixelSize: 8 }
                            Text { text: modelData.free ? qsTr("free") : qsTr("payant"); color: modelData.free ? theme.green : theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                            CheckBox { checked: modelData.enabled; onToggled: crasy.setChannelEnabled(modelData.id, checked) }
                            Button { text: qsTr("Configurer"); flat: true; font.pixelSize: 8; onClicked: root.openEditChannel(modelData) }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Catalogue")
                summary: crasy.models.length + qsTr(" modèle(s) · ") + crasy.freeModelCount + qsTr(" libre(s)")
                collapsible: true
                expanded: false
                Layout.fillWidth: true
                SettingsField { Layout.fillWidth: true; placeholderText: qsTr("Filtrer les modèles…"); onTextChanged: root.modelQuery = text }
                Text { Layout.fillWidth: true; visible: crasy.models.length === 0; text: qsTr("Aucun modèle découvert. Ajoutez une clé, démarrez Ollama ou utilisez DeepSeek."); color: theme.dimText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                Repeater {
                    model: {
                        var result = []
                        const needle = root.modelQuery.toLowerCase()
                        for (var i = 0; i < crasy.models.length && result.length < 100; ++i) {
                            const model = crasy.models[i]
                            if (needle === "" || model.id.toLowerCase().indexOf(needle) >= 0)
                                result.push(model)
                        }
                        return result
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.fillWidth: true; text: modelData.id; color: theme.text; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                        Text { text: modelData.channel; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                        Text { text: modelData.free ? qsTr("free") : qsTr("payant"); color: modelData.free ? theme.green : theme.dimText; font.pixelSize: 8 }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Tester le routage")
                collapsible: true
                expanded: true
                Layout.fillWidth: true
                RowLayout {
                    Layout.fillWidth: true
                    SettingsField { id: routeField; Layout.fillWidth: true; placeholderText: qsTr("Ex. corrige un bug C++ et vérifie les tests"); onTextChanged: root.routePrompt = text }
                    ActionButton { text: qsTr("Router"); onClicked: root.previewRoute() }
                }
                Text { Layout.fillWidth: true; text: root.routeResult; color: theme.accent; font.family: backend.monoFont; font.pixelSize: design.textMicro; visible: root.routeResult !== ""; wrapMode: Text.Wrap }
            }

            SettingsSection {
                title: qsTr("Dernières routes")
                summary: crasy.recentRoutes.length + qsTr(" requête(s)")
                collapsible: true
                expanded: false
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; visible: crasy.recentRoutes.length === 0; text: qsTr("Aucune requête routée pour l'instant."); color: theme.dimText; font.pixelSize: design.textCaption }
                Repeater {
                    model: crasy.recentRoutes
                    RowLayout {
                        Layout.fillWidth: true
                        StatusDot { size: 4; color: modelData.ok ? theme.green : theme.red }
                        Text { Layout.fillWidth: true; text: modelData.time + "  " + modelData.channel + "  " + modelData.model; color: theme.text; font.family: backend.monoFont; font.pixelSize: 8; elide: Text.ElideRight }
                        Text { text: modelData.status; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                    }
                }
            }
        }
    }

    Dialog {
        id: channelDialog
        property string channelTitle: qsTr("Canal")
        modal: true
        width: Math.min(650, root.width - design.space5)
        height: 570
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            Text { text: channelDialog.channelTitle; color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
            RowLayout {
                Layout.fillWidth: true
                SettingsField { id: channelId; Layout.fillWidth: true; placeholderText: qsTr("id-du-canal") }
                SettingsField { id: channelName; Layout.fillWidth: true; placeholderText: qsTr("Nom affiché") }
            }
            SettingsField { id: channelBase; Layout.fillWidth: true; placeholderText: qsTr("URL de base https://…") }
            RowLayout {
                Layout.fillWidth: true
                SettingsField { id: channelEnv; Layout.fillWidth: true; placeholderText: qsTr("Variable de clé, ex. OPENROUTER_API_KEY") }
                SettingsField { id: channelKey; Layout.fillWidth: true; placeholderText: qsTr("Clé API (optionnelle)"); echoMode: TextInput.Password }
            }
            RowLayout {
                Layout.fillWidth: true
                Text { text: qsTr("Priorité"); color: theme.dimText; font.pixelSize: design.textMicro }
                SpinBox { id: channelPriority; from: 0; to: 100; value: 10; editable: true }
                CheckBox { id: channelFree; text: qsTr("Gratuit") }
                CheckBox { id: channelEnabled; text: qsTr("Actif") }
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Tester"); flat: true; onClicked: root.testResult = crasy.testChannel(channelId.text) }
            }
            Text { Layout.fillWidth: true; text: root.testResult; color: theme.accent; font.pixelSize: design.textMicro; wrapMode: Text.Wrap; visible: root.testResult !== "" }
            Text { Layout.fillWidth: true; text: qsTr("Une clé enregistrée est conservée si le champ reste vide. Le fichier est protégé en 0600."); color: theme.dimText; font.pixelSize: 8; wrapMode: Text.Wrap }
            Item { Layout.fillHeight: true }
            RowLayout {
                Layout.fillWidth: true
                Button { id: deleteChannel; text: qsTr("Supprimer"); flat: true; visible: false; onClicked: { crasy.removeChannel(channelId.text); channelDialog.close() } }
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Annuler"); flat: true; onClicked: channelDialog.close() }
                ActionButton { text: qsTr("Enregistrer"); onClicked: root.saveChannel() }
            }
        }
    }
}
