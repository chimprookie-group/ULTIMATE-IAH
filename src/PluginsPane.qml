import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(1180, width - design.space7))
    readonly property var artcraft: {
        var result = []
        for (var i = 0; i < plugins.plugins.length; ++i) {
            if (plugins.plugins[i].kind === "artcraft")
                result.push(plugins.plugins[i])
        }
        return result
    }
    readonly property var external: {
        var result = []
        for (var i = 0; i < plugins.plugins.length; ++i) {
            if (plugins.plugins[i].kind !== "artcraft")
                result.push(plugins.plugins[i])
        }
        return result
    }

    Component.onCompleted: plugins.refresh()
    onVisibleChanged: if (visible) plugins.refresh()

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
                title: qsTr("Plugins")
                description: qsTr("Le registre natif des outils et de la suite ArtCraft, exposé directement à Ultimate Agent.")
                icon: "icons/plug.svg"
                accent: theme.accent
                ActionButton { text: qsTr("Rafraîchir"); onClicked: plugins.refresh() }
                ActionButton { text: qsTr("Dossier plugins"); onClicked: plugins.openPluginFolder() }
            }

            Text {
                Layout.fillWidth: true
                visible: plugins.error !== ""
                text: plugins.error
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
                        Text { text: qsTr("Installés"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: plugins.installedCount; color: theme.text; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Actifs"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: plugins.enabledCount; color: theme.green; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Outils agent"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: plugins.toolCount; color: theme.accent; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("ArtCraft"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: root.artcraft.filter(function (p) { return p.installed }).length + " / " + root.artcraft.length; color: theme.text; font.pixelSize: design.textCaption; font.weight: Font.DemiBold }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Suite ArtCraft")
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("14 applications créatives pilotables par CLI et MCP. Chaque outil est enregistré dans Ultimate Agent sous artcraft_<craft>."); color: theme.mutedText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                    Button { text: qsTr("Guide d'installation"); flat: true; font.pixelSize: design.textMicro; onClicked: plugins.openArtcraftInstallGuide() }
                }
                Text { Layout.fillWidth: true; text: plugins.artcraftPath; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideMiddle }

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.contentWidth > 1000 ? 3 : 2
                    columnSpacing: design.space2
                    rowSpacing: design.space2
                    Repeater {
                        model: root.artcraft
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: craftColumn.implicitHeight + design.space4
                            radius: design.radiusMd
                            color: modelData.installed ? theme.background : theme.surface
                            border.width: 1
                            border.color: modelData.installed && modelData.enabled ? theme.borderStrong : theme.border
                            ColumnLayout {
                                id: craftColumn
                                anchors.fill: parent
                                anchors.margins: design.space3
                                spacing: design.space1
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: design.space2
                                    StatusDot { size: 5; color: modelData.installed ? (modelData.enabled ? theme.green : theme.yellow) : theme.dimText }
                                    Text { Layout.fillWidth: true; text: modelData.name; color: theme.text; font.pixelSize: design.textCaption; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                    Text { text: modelData.installed ? "v" + modelData.version : qsTr("absent"); color: modelData.installed ? theme.dimText : theme.orange; font.family: backend.monoFont; font.pixelSize: 8 }
                                }
                                Text { Layout.fillWidth: true; text: modelData.description; color: theme.dimText; font.pixelSize: design.textMicro; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight }
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: modelData.toolName; color: theme.accent; font.family: backend.monoFont; font.pixelSize: 8; elide: Text.ElideRight }
                                    Item { Layout.fillWidth: true }
                                    Button {
                                        text: modelData.enabled ? qsTr("Désactiver") : qsTr("Activer")
                                        flat: true
                                        font.pixelSize: 8
                                        enabled: modelData.installed
                                        onClicked: plugins.setEnabled(modelData.id, !modelData.enabled)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Plugins externes")
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Déposez un manifeste JSON dans le dossier plugins pour enregistrer une commande comme outil agent. Format : id, name, description, command, args."); color: theme.mutedText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                Text { Layout.fillWidth: true; visible: root.external.length === 0; text: qsTr("Aucun plugin externe installé."); color: theme.dimText; font.pixelSize: design.textCaption }
                Repeater {
                    model: root.external
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: externalColumn.implicitHeight + design.space3
                        radius: design.radiusSm
                        color: theme.background
                        border.width: 1
                        border.color: theme.border
                        ColumnLayout {
                            id: externalColumn
                            anchors.fill: parent
                            anchors.margins: design.space2
                            RowLayout {
                                Layout.fillWidth: true
                                StatusDot { size: 4; color: modelData.installed ? theme.green : theme.dimText }
                                Text { Layout.fillWidth: true; text: modelData.name + "  ·  " + modelData.version; color: theme.text; font.pixelSize: design.textCaption }
                                Button { text: modelData.enabled ? qsTr("Désactiver") : qsTr("Activer"); flat: true; font.pixelSize: 8; enabled: modelData.installed; onClicked: plugins.setEnabled(modelData.id, !modelData.enabled) }
                            }
                            Text { Layout.fillWidth: true; text: modelData.description; color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Intégration au harness")
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Ultimate Agent reçoit dynamiquement les outils des plugins actifs. Les notes disposent aussi d'un graphe natif et d'outils agent."); color: theme.mutedText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                Flow {
                    Layout.fillWidth: true
                    spacing: design.space2
                    Button { text: qsTr("Ultimate Agent"); flat: true; onClicked: backend.setView("agent") }
                    Button { text: qsTr("Notes + graphe"); flat: true; onClicked: { backend.setView("notes"); noteStore.graphOpen = true } }
                    Button { text: qsTr("Skills"); flat: true; onClicked: backend.setView("agents") }
                    Button { text: "MCP"; flat: true; onClicked: backend.setView("mcp") }
                }
            }
        }
    }
}
