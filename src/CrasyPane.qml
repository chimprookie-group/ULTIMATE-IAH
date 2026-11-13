import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(1180, width - design.space7))
    property string routePrompt: ""
    property string routeResult: ""
    property string modelQuery: ""

    function useInHarness() {
        connections.saveProvider("4crasy", "4Crasy", crasy.endpoint, crasy.token,
                                 "auto", true)
        ultimateAgent.refreshEndpoint()
        Window.window.showToast(qsTr("4Crasy est maintenant le provider par défaut."))
    }

    function previewRoute() {
        routeResult = crasy.routePreview(routePrompt, "auto")
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
                Layout.fillWidth: true
                RowLayout {
                    Layout.fillWidth: true
                    CheckBox { text: qsTr("Préférer les modèles gratuits"); checked: crasy.preferFree; onToggled: crasy.preferFree = checked }
                    CheckBox { text: qsTr("Découverte auto"); checked: crasy.autoRefresh; onToggled: crasy.autoRefresh = checked }
                    Item { Layout.fillWidth: true }
                    ActionButton { text: qsTr("Découvrir les modèles"); onClicked: crasy.refreshModels() }
                }
                RowLayout {
                    Layout.fillWidth: true
                    SettingsField { id: routeField; Layout.fillWidth: true; placeholderText: qsTr("Tester le routage : « corrige un bug C++ »") ; onTextChanged: root.routePrompt = text }
                    ActionButton { text: qsTr("Router"); onClicked: root.previewRoute() }
                }
                Text { Layout.fillWidth: true; text: root.routeResult; color: theme.accent; font.family: backend.monoFont; font.pixelSize: design.textMicro; visible: root.routeResult !== ""; wrapMode: Text.Wrap }
            }

            SettingsSection {
                title: qsTr("Canaux")
                Layout.fillWidth: true
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
                                Text { text: modelData.baseUrl + " · " + modelData.modelCount + qsTr(" modèle(s)"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8; elide: Text.ElideRight }
                            }
                            Text { text: modelData.hasKey ? "" : qsTr("clé absente"); color: theme.orange; font.pixelSize: 8 }
                            Text { text: modelData.free ? qsTr("free") : qsTr("payant"); color: modelData.free ? theme.green : theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                            CheckBox { checked: modelData.enabled; onToggled: crasy.setChannelEnabled(modelData.id, checked) }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Catalogue")
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; visible: crasy.models.length === 0; text: qsTr("Aucun modèle découvert. Ajoutez une clé OpenRouter/Groq/Gemini, démarrez Ollama ou utilisez DeepSeek."); color: theme.dimText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                Repeater {
                    model: {
                        var result = []
                        const needle = root.modelQuery.toLowerCase()
                        for (var i = 0; i < crasy.models.length && result.length < 80; ++i) {
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
                title: qsTr("Dernières routes")
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
}
