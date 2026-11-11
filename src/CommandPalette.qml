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
        width: Math.min(640, parent.width - 64)
        height: Math.min(560, parent.height - 100)
        anchors.centerIn: parent
        radius: Math.min(14, theme.radius)
        color: theme.surface
        border.width: 1
        border.color: theme.borderStrong

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            RowLayout {
                Layout.fillWidth: true

                Text {
        renderType: Text.NativeRendering
                    Layout.fillWidth: true
                    text: qsTr("Commands")
                    color: theme.brightText
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }
                Rectangle {
                    implicitWidth: 34
                    height: 20
                    radius: 5
                    color: theme.chip
                    border.width: 1
                    border.color: theme.border
                    Text {
        renderType: Text.NativeRendering
                        anchors.centerIn: parent
                        text: "Esc"
                        color: theme.mutedText
                        font.family: backend.monoFont
                        font.pixelSize: 8
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 38
                radius: Math.min(8, theme.radius)
                color: theme.background
                border.width: 1
                border.color: search.activeFocus ? theme.accent : theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 11
                    anchors.rightMargin: 11
                    spacing: 8

                    ThemedIcon {
                        source: "icons/search.svg"
                        width: 14
                        height: 14
                        color: theme.mutedText
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        TextInput {
                            id: search
                            anchors.fill: parent
                            color: theme.text
                            font.pixelSize: 13
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
        renderType: Text.NativeRendering
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            visible: search.text.length === 0
                            text: qsTr("Search an action, a theme, a session…")
                            color: theme.dimText
                            font.pixelSize: 13
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
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: Rectangle {
                    id: row
                    width: ListView.view.width
                    height: 42
                    radius: Math.min(8, theme.radius)
                    color: ListView.isCurrentItem ? theme.menuSelected : (rowMouse.containsMouse ? theme.menuHover : "transparent")
                    border.width: ListView.isCurrentItem ? 1 : 0
                    border.color: theme.borderStrong

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 9

                        ThemedIcon {
                            source: modelData.icon !== undefined ? modelData.icon : "icons/spark.svg"
                            width: 14
                            height: 14
                            color: theme.mutedText
                        }
                        Text {
        renderType: Text.NativeRendering
                            Layout.fillWidth: true
                            text: modelData.label
                            color: theme.text
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                        Text {
        renderType: Text.NativeRendering
                            visible: modelData.hint !== undefined && modelData.hint !== ""
                            text: modelData.hint !== undefined ? modelData.hint : ""
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: 8
                        }
                        Text {
        renderType: Text.NativeRendering
                            text: modelData.group !== undefined ? modelData.group : ""
                            color: theme.dimText
                            font.pixelSize: 8
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
        renderType: Text.NativeRendering
                Layout.fillWidth: true
                visible: root.filtered.length === 0
                text: qsTr("No match.")
                color: theme.dimText
                font.pixelSize: 10
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
        renderType: Text.NativeRendering
                Layout.fillWidth: true
                text: qsTr("↑↓ navigate · Enter run · Esc close")
                color: theme.dimText
                font.pixelSize: 8
                horizontalAlignment: Text.AlignRight
            }
        }
    }
}
