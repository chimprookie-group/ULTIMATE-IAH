import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(1180, width - design.space7))

    Component.onCompleted: if (visible) learning.refresh()
    onVisibleChanged: if (visible) learning.refresh()

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
                title: qsTr("Apprentissage")
                description: qsTr("Boucle d’auto-amélioration locale : trajectoires, leçons et skills proposés.")
                icon: "icons/spark.svg"
                accent: theme.accent
                ActionButton { text: qsTr("Analyser"); onClicked: learning.refresh() }
                ActionButton { text: qsTr("Effacer les données"); onClicked: { learning.clear(); Window.window.showToast(qsTr("Données d’apprentissage effacées.")) } }
            }

            Text {
                Layout.fillWidth: true
                visible: learning.error !== ""
                text: learning.error
                color: theme.red
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: learningRow.implicitHeight + design.space3
                radius: design.radiusMd
                color: theme.surface
                border.width: 1
                border.color: theme.border
                RowLayout {
                    id: learningRow
                    anchors.fill: parent
                    anchors.margins: design.space3
                    spacing: design.space3
                    ThemedIcon { source: "icons/spark.svg"; width: 18; height: 18; color: theme.accent }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text { text: learning.enabled ? qsTr("Auto-apprentissage actif") : qsTr("Auto-apprentissage en pause"); color: theme.text; font.pixelSize: design.textLabel; font.weight: Font.DemiBold }
                        Text { Layout.fillWidth: true; text: qsTr("Les leçons sont injectées dans les prochains tours. Les nouveaux skills restent soumis à validation."); color: theme.dimText; font.pixelSize: design.textMicro; wrapMode: Text.Wrap }
                    }
                    AppSwitch { checked: learning.enabled; onToggled: learning.enabled = checked }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.contentWidth > 820 ? 4 : 2
                columnSpacing: design.space2
                rowSpacing: design.space2
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Événements"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: learning.stats.events !== undefined ? learning.stats.events : 0; color: theme.text; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Réussites"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: learning.stats.successes !== undefined ? learning.stats.successes : 0; color: theme.green; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Leçons"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: learning.stats.lessons !== undefined ? learning.stats.lessons : 0; color: theme.accent; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 74; radius: design.radiusMd; color: theme.surface; border.width: 1; border.color: theme.border
                    ColumnLayout { anchors.fill: parent; anchors.margins: design.space3; spacing: 1
                        Text { text: qsTr("Skills proposés"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: learning.stats.proposals !== undefined ? learning.stats.proposals : 0; color: theme.orange; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Leçons actives")
                summary: learning.lessons.length + qsTr(" leçon(s)")
                collapsible: true
                expanded: true
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; visible: learning.lessons.length === 0; text: qsTr("Les premières leçons apparaîtront après quelques tours d’agent, surtout après une erreur ou une séquence d’outils réussie."); color: theme.dimText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                Repeater {
                    model: learning.lessons
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: lessonColumn.implicitHeight + design.space2
                        radius: design.radiusSm
                        color: theme.background
                        border.width: 1
                        border.color: theme.border
                        ColumnLayout {
                            id: lessonColumn
                            anchors.fill: parent
                            anchors.margins: design.space2
                            spacing: 2
                            RowLayout {
                                Layout.fillWidth: true
                                StatusDot { size: 4; color: modelData.kind === "failure" ? theme.orange : theme.green }
                                Text { Layout.fillWidth: true; text: modelData.title; color: theme.text; font.pixelSize: design.textMicro; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                Text { text: Math.round(modelData.confidence * 100) + "% · " + modelData.uses + "×"; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                            }
                            Text { Layout.fillWidth: true; text: modelData.body; color: theme.dimText; font.pixelSize: 8; wrapMode: Text.Wrap; maximumLineCount: 3; elide: Text.ElideRight }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Skills proposés")
                summary: learning.proposals.length + qsTr(" proposition(s)")
                collapsible: true
                expanded: false
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; visible: learning.proposals.length === 0; text: qsTr("Aucun skill en attente. Une proposition apparaît lorsqu’une leçon devient suffisamment fiable."); color: theme.dimText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                Repeater {
                    model: learning.proposals
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: proposalColumn.implicitHeight + design.space2
                        radius: design.radiusSm
                        color: theme.background
                        border.width: 1
                        border.color: theme.border
                        ColumnLayout {
                            id: proposalColumn
                            anchors.fill: parent
                            anchors.margins: design.space2
                            spacing: 2
                            Text { Layout.fillWidth: true; text: modelData.title; color: theme.text; font.pixelSize: design.textMicro; font.weight: Font.DemiBold }
                            Text { Layout.fillWidth: true; text: modelData.body; color: theme.dimText; font.pixelSize: 8; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight }
                            RowLayout {
                                Layout.fillWidth: true
                                Item { Layout.fillWidth: true }
                                GhostButton { text: qsTr("Rejeter"); font.pixelSize: 8; onClicked: learning.rejectProposal(modelData.id) }
                                ActionButton { text: qsTr("Créer le skill"); onClicked: { learning.approveProposal(modelData.id); Window.window.showToast(qsTr("Skill créé.")) } }
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Derniers apprentissages")
                summary: learning.events.length + qsTr(" événement(s)")
                collapsible: true
                expanded: false
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; visible: learning.events.length === 0; text: qsTr("Aucun événement d’apprentissage."); color: theme.dimText; font.pixelSize: design.textCaption }
                Repeater {
                    model: learning.events
                    RowLayout {
                        Layout.fillWidth: true
                        StatusDot { size: 4; color: modelData.outcome === "success" ? theme.green : theme.orange }
                        Text { Layout.fillWidth: true; text: modelData.createdAt + "  " + modelData.prompt; color: theme.text; font.family: backend.monoFont; font.pixelSize: 8; elide: Text.ElideRight }
                        Text { text: modelData.toolCount + qsTr(" outil(s)"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Politique d’apprentissage")
                collapsible: true
                expanded: false
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Tout reste local dans learning.db. Les secrets sont expurgés. Les leçons modifient le contexte des prochains tours ; les fichiers de skills ne sont créés qu’après validation explicite. Aucun code, provider ou réglage système n’est modifié automatiquement."); color: theme.mutedText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                Text { Layout.fillWidth: true; text: learning.databasePath; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8; elide: Text.ElideMiddle }
            }
        }
    }
}
