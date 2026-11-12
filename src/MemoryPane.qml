import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(design.contentMax, width - design.space7))
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
            x: Math.max(design.space5, (parent.width - width) / 2)
            spacing: design.space5

            Item { Layout.fillWidth: true; Layout.preferredHeight: design.space4 }

            PageHeader {
                Layout.fillWidth: true
                title: qsTr("Mémoire")
                description: qsTr("Faits, décisions, préférences et résultats, retrouvés par recherche plein texte.")
                icon: "icons/memory.svg"
                accent: theme.accent
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
                            font.pixelSize: design.textCaption
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Ajouter")
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space3

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text { text: qsTr("Titre"); color: theme.dimText; font.pixelSize: design.textCaption }
                        SettingsField {
                            id: titleField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Titre — ex. Décision : SQLite + FTS5")
                            onAccepted: root.addMemory()
                        }
                    }
                    ColumnLayout {
                        Layout.preferredWidth: 128
                        spacing: 3
                        Text { text: qsTr("Type"); color: theme.dimText; font.pixelSize: design.textCaption }
                        ComboBox {
                            id: kindBox
                            Layout.fillWidth: true
                            model: ["note", "fact", "decision", "preference", "task", "result"]
                            font.pixelSize: design.textCaption
                        }
                    }
                }

                Text { Layout.fillWidth: true; text: qsTr("Contenu"); color: theme.dimText; font.pixelSize: design.textCaption }
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
                    spacing: design.space3

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Text { text: qsTr("Tags"); color: theme.dimText; font.pixelSize: design.textCaption }
                        SettingsField {
                            id: tagsField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Tags, séparés par des virgules")
                        }
                    }
                    CheckBox {
                        id: pinnedBox
                        text: qsTr("Épingler")
                        font.pixelSize: design.textCaption
                        Layout.alignment: Qt.AlignBottom
                    }
                    ActionButton {
                        Layout.alignment: Qt.AlignBottom
                        text: qsTr("Retenir")
                        onClicked: root.addMemory()
                    }
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
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: design.space5
                spacing: design.space3
                visible: root.shown.length === 0

                BrandMark {
                    Layout.alignment: Qt.AlignHCenter
                    width: 54
                    height: 46
                    size: 54
                    accent: true
                    markOpacity: 0.75
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: memory.query !== "" ? qsTr("Aucun résultat") : qsTr("La mémoire est vide")
                    color: theme.text
                    font.pixelSize: design.textBody
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: memory.query !== "" ? qsTr("Essayez un autre mot-clé ou effacez la recherche.")
                                                 : qsTr("Commencez par une décision, une préférence ou un résultat à retenir.")
                    color: theme.dimText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }
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
                                    font.pixelSize: design.textMicro
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
                                font.pixelSize: design.textMicro
                                onClicked: memory.togglePin(modelData.id)
                            }
                            Button {
                                text: qsTr("Supprimer")
                                flat: true
                                font.pixelSize: design.textMicro
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
                            font.pixelSize: design.textMicro
                            elide: Text.ElideMiddle
                        }
                    }
                }
            }
        }
    }
}
