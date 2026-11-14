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
    readonly property int artcraftInstalled: {
        var count = 0
        for (var i = 0; i < plugins.plugins.length; ++i) {
            if (plugins.plugins[i].kind === "artcraft" && plugins.plugins[i].installed)
                ++count
        }
        return count
    }
    readonly property var coreCapabilities: {
        var result = []
        for (var i = 0; i < plugins.capabilities.length; ++i) {
            if (plugins.capabilities[i].category === "Noyau")
                result.push(plugins.capabilities[i])
        }
        return result
    }
    readonly property var experimentalCapabilities: {
        var result = []
        for (var i = 0; i < plugins.capabilities.length; ++i) {
            if (plugins.capabilities[i].category !== "Noyau")
                result.push(plugins.capabilities[i])
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
                columns: root.contentWidth > 1000 ? 5 : 2
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
                        Text { text: qsTr("Outils plugins"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: plugins.toolCount; color: theme.accent; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Capacités DSH"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: plugins.activeCapabilities + " / " + plugins.capabilities.length; color: theme.text; font.pixelSize: design.textCaption; font.weight: Font.DemiBold }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Suite ArtCraft")
                collapsible: true
                expanded: false
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("14 applications créatives pilotables par CLI et MCP. Chaque outil est enregistré dans Ultimate Agent sous artcraft_<craft>."); color: theme.mutedText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                    GhostButton { text: qsTr("Guide d'installation"); flat: true; font.pixelSize: design.textMicro; onClicked: plugins.openArtcraftInstallGuide() }
                }
                Text { Layout.fillWidth: true; text: plugins.artcraftPath; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideMiddle }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space2
                    Text {
                        text: (plugins.artcraftVersion !== "" ? "v" + plugins.artcraftVersion : qsTr("suite absente"))
                              + " · " + root.artcraftInstalled + "/" + plugins.artcraft.length
                        color: theme.mutedText
                        font.family: backend.monoFont
                        font.pixelSize: design.textMicro
                    }
                    Item { Layout.fillWidth: true }
                    GhostButton {
                        text: qsTr("Compléter")
                        flat: true
                        font.pixelSize: design.textMicro
                        enabled: !plugins.artcraftBusy
                        onClicked: plugins.installOrUpdateArtcraft(false)
                    }
                    ActionButton {
                        text: plugins.artcraftBusy ? qsTr("En cours…") : qsTr("Mettre à jour")
                        enabled: !plugins.artcraftBusy
                        onClicked: plugins.installOrUpdateArtcraft(true)
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: plugins.artcraftStatus !== ""
                    text: plugins.artcraftStatus
                    color: plugins.error !== "" ? theme.red : theme.dimText
                    font.pixelSize: design.textMicro
                    wrapMode: Text.Wrap
                }

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
                                    GhostButton {
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
                title: qsTr("Recherche web")
                collapsible: true
                expanded: false
                Layout.fillWidth: true
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("web_search et web_fetch sont enregistrés dans Ultimate Agent. DeepSeek Search est prioritaire ; DuckDuckGo sert de secours sans clé."); color: theme.mutedText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                    StatusDot { size: 5; color: web.searchReady ? theme.green : theme.orange }
                    Text { text: web.searchReady ? qsTr("DeepSeek prêt") : qsTr("secours DuckDuckGo"); color: web.searchReady ? theme.green : theme.orange; font.pixelSize: design.textMicro }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("Provider : %1 · cache : %2 entrée(s)").arg(web.searchProvider).arg(web.cacheEntries); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                    GhostButton { text: qsTr("Vider le cache"); flat: true; font.pixelSize: design.textMicro; onClicked: web.clearCache() }
                }
                Repeater {
                    model: web.recentSearches
                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.fillWidth: true; text: modelData.queries; color: theme.text; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                        Text { text: modelData.provider + " · " + modelData.count; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                    }
                }
                Text { Layout.fillWidth: true; visible: web.recentSearches.length === 0; text: qsTr("Aucune recherche récente."); color: theme.dimText; font.pixelSize: design.textCaption }
            }

            SettingsSection {
                title: qsTr("Noyau DSH")
                collapsible: true
                expanded: false
                Layout.fillWidth: true
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("Les capacités structurantes du harness, actives par défaut comme dans le profil DSH web."); color: theme.mutedText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                    Text { text: plugins.activeCapabilities + " / " + plugins.capabilities.length; color: theme.green; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: root.contentWidth > 1000 ? 3 : 2
                    columnSpacing: design.space2
                    rowSpacing: design.space2
                    Repeater {
                        model: root.coreCapabilities
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: dshCoreColumn.implicitHeight + design.space3
                            radius: design.radiusSm
                            color: theme.background
                            border.width: 1
                            border.color: modelData.implemented ? theme.border : theme.borderStrong
                            ColumnLayout {
                                id: dshCoreColumn
                                anchors.fill: parent
                                anchors.margins: design.space2
                                spacing: 2
                                RowLayout {
                                    Layout.fillWidth: true
                                    StatusDot { size: 4; color: modelData.implemented ? theme.green : theme.yellow }
                                    Text { Layout.fillWidth: true; text: modelData.name; color: theme.text; font.pixelSize: design.textMicro; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                    Text { text: modelData.implemented ? qsTr("natif") : qsTr("partiel"); color: modelData.implemented ? theme.green : theme.orange; font.pixelSize: 8 }
                                }
                                Text { Layout.fillWidth: true; text: modelData.description; color: theme.dimText; font.pixelSize: 8; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight }
                                Text { Layout.fillWidth: true; text: modelData.tool !== "" ? modelData.tool : modelData.view; color: theme.accent; font.family: backend.monoFont; font.pixelSize: 8; elide: Text.ElideRight }
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Extensions expérimentales")
                collapsible: true
                expanded: false
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Désactivées par défaut, comme dans DSH. Elles sont visibles pour garder la carte complète de l’écosystème."); color: theme.mutedText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                GridLayout {
                    Layout.fillWidth: true
                    columns: root.contentWidth > 1000 ? 3 : 2
                    columnSpacing: design.space2
                    rowSpacing: design.space2
                    Repeater {
                        model: root.experimentalCapabilities
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: expColumn.implicitHeight + design.space3
                            radius: design.radiusSm
                            color: theme.surface
                            border.width: 1
                            border.color: theme.border
                            ColumnLayout {
                                id: expColumn
                                anchors.fill: parent
                                anchors.margins: design.space2
                                spacing: 1
                                RowLayout {
                                    Layout.fillWidth: true
                                    StatusDot { size: 4; color: theme.dimText }
                                    Text { Layout.fillWidth: true; text: modelData.name; color: theme.text; font.pixelSize: design.textMicro; font.weight: Font.DemiBold }
                                    Text { text: qsTr("désactivé"); color: theme.dimText; font.pixelSize: 8 }
                                }
                                Text { Layout.fillWidth: true; text: modelData.description; color: theme.dimText; font.pixelSize: 8; elide: Text.ElideRight }
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Plugins externes")
                collapsible: true
                expanded: false
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
                                GhostButton { text: modelData.enabled ? qsTr("Désactiver") : qsTr("Activer"); flat: true; font.pixelSize: 8; enabled: modelData.installed; onClicked: plugins.setEnabled(modelData.id, !modelData.enabled) }
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
                    GhostButton { text: qsTr("Ultimate Agent"); flat: true; onClicked: backend.setView("agent") }
                    GhostButton { text: qsTr("Notes + graphe"); flat: true; onClicked: { backend.setView("notes"); noteStore.graphOpen = true } }
                    GhostButton { text: qsTr("Skills"); flat: true; onClicked: backend.setView("agents") }
                    GhostButton { text: "MCP"; flat: true; onClicked: backend.setView("mcp") }
                }
            }
        }
    }
}
