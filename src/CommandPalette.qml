import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// Ctrl+K: every action, view, theme, language, and recent session in one
// searchable list.
Item {
    id: root

    property bool opened: false
    property var commands: []
    property var filtered: []

    signal chosen(string id)

    visible: opened
    z: 160

    function open() {
        opened = true
        search.text = ""
        refresh()
        Qt.callLater(function () {
            search.forceActiveFocus()
            list.currentIndex = filtered.length > 0 ? 0 : -1
        })
    }

    function close() {
        opened = false
    }

    function fold(value) {
        return String(value).toLowerCase()
            .replace(/[àâä]/g, "a")
            .replace(/[éèêë]/g, "e")
            .replace(/[îï]/g, "i")
            .replace(/[ôö]/g, "o")
            .replace(/[ùûü]/g, "u")
            .replace(/ç/g, "c")
    }

    function refresh() {
        const query = fold(search.text.trim())
        const out = []
        for (let i = 0; i < commands.length; ++i) {
            const command = commands[i]
            const haystack = String(command.label || "") + " " + String(command.group || "") + " " + String(command.hint || "")
            if (query === "" || fold(haystack).indexOf(query) >= 0)
                out.push(command)
        }
        filtered = out
        if (out.length === 0)
            list.currentIndex = -1
        else if (list.currentIndex < 0 || list.currentIndex >= out.length)
            list.currentIndex = 0
    }

    function move(delta) {
        if (filtered.length === 0)
            return
        let index = list.currentIndex + delta
        index = Math.max(0, Math.min(filtered.length - 1, index))
        list.currentIndex = index
        list.positionViewAtIndex(index, ListView.Contain)
    }

    function chooseCurrent() {
        if (list.currentIndex >= 0 && list.currentIndex < filtered.length)
            choose(filtered[list.currentIndex].id)
    }

    function choose(id) {
        root.chosen(id)
        root.close()
    }

    onCommandsChanged: if (opened) refresh()

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.6
    }
    MouseArea {
        anchors.fill: parent
        onClicked: root.close()
    }

    Rectangle {
        width: Math.min(680, parent.width - design.space8)
        height: Math.min(580, parent.height - design.space8 * 2)
        anchors.centerIn: parent
        radius: design.radiusLg
        color: theme.surface
        border.width: 1
        border.color: theme.borderStrong

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: design.space5
            spacing: design.space3

            RowLayout {
                Layout.fillWidth: true
                spacing: design.space2

                BrandMark { width: 26; height: 26; size: 26; accent: true }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Commandes")
                    color: theme.brightText
                    font.family: "iA Writer Duospace"
                    font.pixelSize: design.textTitle
                    font.weight: Font.DemiBold
                }
                Rectangle {
                    implicitWidth: 38
                    height: 22
                    radius: design.radiusSm
                    color: theme.chip
                    border.width: 1
                    border.color: theme.border
                    Text {
                        anchors.centerIn: parent
                        text: "Esc"
                        color: theme.mutedText
                        font.family: backend.monoFont
                        font.pixelSize: design.textMicro
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 44
                radius: design.radiusSm
                color: theme.background
                border.width: 1
                border.color: search.activeFocus ? theme.accent : theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: design.space3
                    anchors.rightMargin: design.space3
                    spacing: design.space2

                    ThemedIcon {
                        source: "icons/search.svg"
                        width: design.iconMd
                        height: design.iconMd
                        color: theme.mutedText
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        TextInput {
                            id: search
                            anchors.fill: parent
                            color: theme.text
                            font.pixelSize: design.textBodyLarge
                            selectionColor: theme.accent
                            selectedTextColor: theme.accentForeground
                            clip: true
                            onTextChanged: root.refresh()
                            Keys.onPressed: function (event) {
                                if (event.key === Qt.Key_Down) {
                                    root.move(1)
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Up) {
                                    root.move(-1)
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                    root.chooseCurrent()
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Escape) {
                                    root.close()
                                    event.accepted = true
                                }
                            }
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            visible: search.text.length === 0
                            text: qsTr("Search an action, a theme, a session…")
                            color: theme.dimText
                            font.pixelSize: design.textBodyLarge
                        }
                    }
                }
            }

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.filtered
                currentIndex: -1
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: AppScrollBar {}

                delegate: Rectangle {
                    id: row
                    width: ListView.view.width
                    height: 46
                    radius: design.radiusSm
                    color: ListView.isCurrentItem ? theme.menuSelected : (rowMouse.containsMouse ? theme.menuHover : "transparent")
                    border.width: ListView.isCurrentItem ? 1 : 0
                    border.color: theme.borderStrong

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: design.space3
                        anchors.rightMargin: design.space3
                        spacing: design.space3

                        ThemedIcon {
                            source: modelData.icon !== undefined ? modelData.icon : "icons/spark.svg"
                            width: design.iconMd
                            height: design.iconMd
                            color: theme.mutedText
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelData.label
                            color: theme.text
                            font.pixelSize: design.textBody
                            elide: Text.ElideRight
                        }
                        Text {
                            visible: modelData.hint !== undefined && modelData.hint !== ""
                            text: modelData.hint !== undefined ? modelData.hint : ""
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textMicro
                        }
                        Text {
                            text: modelData.group !== undefined ? modelData.group : ""
                            color: theme.dimText
                            font.pixelSize: design.textMicro
                        }
                    }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: list.currentIndex = index
                        onClicked: root.choose(modelData.id)
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.filtered.length === 0
                text: qsTr("No match.")
                color: theme.dimText
                font.pixelSize: 10
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("↑↓ naviguer · Entrée exécuter · Échap fermer")
                color: theme.dimText
                font.pixelSize: design.textMicro
                horizontalAlignment: Text.AlignRight
            }
        }
    }
}
