import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(340, Math.min(1000, width - 64))
    readonly property var shown: memory.query !== "" ? memory.results : memory.entries

    function addMemory() {
        if (titleField.text.trim() === "") {
            Window.window.showToast(qsTr("Donnez un titre à la mémoire."))
            return
        }
        memory.remember(backend.workspace, kindBox.currentText, titleField.text.trim(),
                        bodyArea.text, tagsField.text, pinnedBox.checked, 3, "user")
        if (memory.error === "") {
            titleField.text = ""
            bodyArea.text = ""
            tagsField.text = ""
            pinnedBox.checked = false
            Window.window.showToast(qsTr("Mémoire enregistrée."))
        }
    }

    function checkIntegrity() {
        const report = memory.integrityCheck()
        Window.window.showToast(report.ok
            ? qsTr("Mémoire saine · %1 entrées").arg(report.total)
            : qsTr("Intégrité à vérifier : %1 / %2").arg(report.database).arg(report.fts))
    }

    Component.onCompleted: if (visible) memory.refresh()
    onVisibleChanged: if (visible) memory.refresh()

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
                    Rectangle { anchors.centerIn: parent; width: 52; height: 52; radius: 26; color: theme.accent; opacity: 0.12 }
                    ThemedIcon { anchors.centerIn: parent; source: "icons/spark.svg"; width: 22; height: 22; color: theme.accent }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: qsTr("Mémoire")
                        color: theme.brightText
                        font.family: "iA Writer Duospace"
                        font.pixelSize: 26
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Faits, décisions, préférences et résultats — retrouvés par recherche plein texte.")
                        color: theme.mutedText
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                    }
                }
                ActionButton {
                    text: qsTr("Sauvegarder")
                    onClicked: {
                        const path = memory.backupNow()
                        Window.window.showToast(path !== ""
                            ? qsTr("Sauvegarde mémoire créée.")
                            : qsTr("La sauvegarde a échoué."))
                    }
                }
                ActionButton { text: qsTr("Vérifier"); onClicked: root.checkIntegrity() }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 6
                Repeater {
                    model: [
                        { "label": qsTr("%n entrée(s)", "", memory.stats.total) },
                        { "label": qsTr("%n épinglée(s)", "", memory.stats.pinned) },
                        { "label": qsTr("%n projet(s)", "", memory.stats.projects) },
                        { "label": (memory.stats.bytes / 1024).toFixed(1) + " Ko" }
                    ]
                    Rectangle {
                        width: stat.implicitWidth + 18
                        height: 24
                        radius: 12
                        color: theme.chip
                        border.width: 1
                        border.color: theme.border
                        Text {
                            id: stat
                            anchors.centerIn: parent
                            text: modelData.label
                            color: theme.mutedText
                            font.pixelSize: 9
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Ajouter")
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    SettingsField {
                        id: titleField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Titre — ex. Décision : SQLite + FTS5")
                        onAccepted: root.addMemory()
                    }
                    ComboBox {
                        id: kindBox
                        Layout.preferredWidth: 128
                        model: ["note", "fact", "decision", "preference", "task", "result"]
                        font.pixelSize: 9
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 86
                    radius: Math.min(6, theme.radius)
                    color: theme.background
                    border.width: 1
                    border.color: bodyArea.activeFocus ? theme.accent : theme.border
                    TextArea {
                        id: bodyArea
                        anchors.fill: parent
                        anchors.margins: 7
                        color: theme.text
                        font.family: backend.monoFont
                        font.pixelSize: 10
                        wrapMode: TextArea.Wrap
                        background: null
                    }
                    Text {
                        anchors.fill: parent
                        anchors.margins: 10
                        visible: bodyArea.text === ""
                        text: qsTr("Le contenu à retenir…")
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: 10
                        wrapMode: Text.Wrap
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    SettingsField {
                        id: tagsField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Tags, séparés par des virgules")
                    }
                    CheckBox {
                        id: pinnedBox
                        text: qsTr("Épingler")
                        font.pixelSize: 9
                    }
                    ActionButton { text: qsTr("Retenir"); onClicked: root.addMemory() }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                SettingsField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Rechercher dans la mémoire…")
                    text: memory.query
                    onTextChanged: searchTimer.restart()
                    onAccepted: memory.search(text, "")
                }
                Timer {
                    id: searchTimer
                    interval: 280
                    onTriggered: memory.search(searchField.text, "")
                }
                ActionButton {
                    text: qsTr("Effacer")
                    visible: memory.query !== ""
                    onClicked: { searchField.text = ""; memory.clearSearch() }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: memory.error !== ""
                text: memory.error
                color: theme.red
                font.pixelSize: 9
                wrapMode: Text.Wrap
            }

            Text {
                Layout.fillWidth: true
                visible: root.shown.length === 0
                text: memory.query !== "" ? qsTr("Aucun résultat.") : qsTr("La mémoire est vide. Commencez par une décision ou une préférence.")
                color: theme.dimText
                font.pixelSize: 10
            }

            Repeater {
                model: root.shown
                Rectangle {
                    Layout.fillWidth: true
                    height: entryBody.implicitHeight + 22
                    radius: Math.min(10, theme.radius)
                    color: theme.surface
                    border.width: 1
                    border.color: modelData.pinned ? theme.accent : theme.border

                    ColumnLayout {
                        id: entryBody
                        x: 12
                        y: 10
                        width: parent.width - 24
                        spacing: 5

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7
                            Rectangle {
                                width: kindLabel.implicitWidth + 12
                                height: 18
                                radius: 9
                                color: theme.chip
                                Text {
                                    id: kindLabel
                                    anchors.centerIn: parent
                                    text: modelData.kind
                                    color: theme.accent
                                    font.pixelSize: 8
                                    font.weight: Font.DemiBold
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.title
                                color: theme.text
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                            Button {
                                text: modelData.pinned ? qsTr("Épinglée") : qsTr("Épingler")
                                flat: true
                                font.pixelSize: 8
                                onClicked: memory.togglePin(modelData.id)
                            }
                            Button {
                                text: qsTr("Supprimer")
                                flat: true
                                font.pixelSize: 8
                                onClicked: memory.removeEntry(modelData.id)
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: modelData.body !== ""
                            text: modelData.body
                            color: theme.mutedText
                            font.pixelSize: 10
                            wrapMode: Text.Wrap
                            maximumLineCount: 5
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: (modelData.project !== "" ? modelData.project : qsTr("global"))
                                  + (modelData.tags !== "" ? " · " + modelData.tags : "")
                                  + " · " + modelData.updatedAt.substring(0, 10)
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: 8
                            elide: Text.ElideMiddle
                        }
                    }
                }
            }
        }
    }
}
