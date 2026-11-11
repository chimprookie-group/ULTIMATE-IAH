import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import "Palette.js" as Pal

// A floating picker for model and mode choices. The stock Material ComboBox
// popup was too narrow for names like "opencode/DeepSeek V4.1 Flash Free" and
// could run past the bottom of the window; this one has room, a search field,
// full names, provider context, and keyboard navigation.
Popup {
    id: popup

    property string title: ""
    property var options: []
    property string currentValue: ""
    property bool searchable: true
    property int preferredWidth: 420
    property var filtered: []
    property var hostWindow: null
    property bool typed: false

    signal selected(string value)

    width: Math.min(preferredWidth, (hostWindow ? hostWindow.width : 1200) - 48)
    // Shrink to the number of choices, but never past the window.
    height: contentItem ? Math.min(Math.min(480, contentItem.implicitHeight),
                                   (hostWindow ? hostWindow.height : 800) - 150) : 480
    padding: 0
    margins: 0
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    y: -height - 8

    function providerOf(value) {
        const slash = String(value).indexOf("/")
        return slash > 0 ? String(value).slice(0, slash) : ""
    }

    function displayName(option) {
        const name = String(option.name !== undefined ? option.name : option.value)
        const slash = name.indexOf("/")
        return slash > 0 ? name.slice(slash + 1) : name
    }

    function secondary(option) {
        if (option.description)
            return option.description
        const value = String(option.value !== undefined ? option.value : "")
        return value !== popup.displayName(option) ? value : ""
    }

    function providerColor(value) {
        const provider = providerOf(value)
        if (provider === "opencode")
            return Pal.teal
        if (provider === "anthropic")
            return "#E5A97A"
        if (provider === "openai")
            return "#6FD3AE"
        if (provider === "google")
            return "#7FA8F0"
        return Pal.orange
    }

    function indexOf(value) {
        for (let i = 0; i < filtered.length; ++i)
            if (filtered[i].value === value)
                return i
        return -1
    }

    function refresh() {
        const query = search.text.trim().toLowerCase()
        const out = []
        for (let i = 0; i < options.length; ++i) {
            const option = options[i]
            const haystack = String(option.name || "") + " " + String(option.value || "") + " " + String(option.description || "")
            if (query === "" || haystack.toLowerCase().indexOf(query) >= 0)
                out.push(option)
        }
        filtered = out
        if (out.length === 0) {
            list.currentIndex = -1
            return
        }
        if (!typed) {
            const index = indexOf(currentValue)
            list.currentIndex = index >= 0 ? index : 0
        } else if (list.currentIndex < 0 || list.currentIndex >= out.length) {
            list.currentIndex = 0
        }
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
            choose(filtered[list.currentIndex].value)
    }

    function choose(value) {
        popup.selected(value)
        popup.close()
    }

    onOptionsChanged: {
        if (opened)
            refresh()
    }

    onCurrentValueChanged: {
        if (opened && !typed)
            refresh()
    }

    onOpened: {
        typed = false
        search.text = ""
        refresh()
        Qt.callLater(function () {
            if (popup.searchable)
                search.forceActiveFocus()
            else
                list.forceActiveFocus()
            const index = popup.indexOf(popup.currentValue)
            if (index >= 0) {
                list.currentIndex = index
                list.positionViewAtIndex(index, ListView.Center)
            }
        })
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 110 }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 80 }
    }

    background: Rectangle {
        color: Pal.raised
        radius: Math.min(13, theme.radius)
        border.width: 1
        border.color: Pal.borderStrong
    }

    contentItem: Item {
        implicitWidth: popup.width
        implicitHeight: contentColumn.implicitHeight + 28

        ColumnLayout {
            id: contentColumn
            x: 14
            y: 14
            width: popup.width - 28
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: popup.title.toUpperCase()
                    color: Pal.muted
                    font.pixelSize: 9
                    font.letterSpacing: 1.3
                    font.weight: Font.DemiBold
                }
                Item { Layout.fillWidth: true }
                Rectangle {
                    width: countLabel.implicitWidth + 14
                    height: 18
                    radius: 9
                    color: Pal.raised
                    border.width: 1
                    border.color: Pal.border
                    Text {
                        id: countLabel
                        anchors.centerIn: parent
                        text: popup.filtered.length
                        color: Pal.dim
                        font.pixelSize: 9
                    }
                }
            }

            Rectangle {
                visible: popup.searchable
                Layout.fillWidth: true
                height: 34
                radius: Math.min(8, theme.radius)
                color: Pal.bg
                border.width: 1
                border.color: search.activeFocus ? theme.accent : Pal.border

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    Image {
                        source: "icons/search.svg"
                        sourceSize.width: 13
                        sourceSize.height: 13
                        opacity: 0.5
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        TextInput {
                            id: search
                            anchors.fill: parent
                            color: Pal.text
                            font.pixelSize: 12
                            selectionColor: theme.accent
                            selectedTextColor: theme.accentForeground
                            clip: true
                            onTextChanged: {
                                popup.typed = search.text.length > 0
                                popup.refresh()
                            }
                            Keys.onPressed: function (event) {
                                if (event.key === Qt.Key_Down) {
                                    popup.move(1)
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Up) {
                                    popup.move(-1)
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                    popup.chooseCurrent()
                                    event.accepted = true
                                } else if (event.key === Qt.Key_Escape) {
                                    popup.close()
                                    event.accepted = true
                                }
                            }
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            visible: search.text.length === 0
                            text: qsTr("Search models…")
                            color: Pal.dim
                            font.pixelSize: 12
                        }
                    }
                }
            }

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(288, Math.max(1, popup.filtered.length) * 48 - 2)
                clip: true
                model: popup.filtered
                currentIndex: -1
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds
                focus: !popup.searchable
                Keys.onPressed: function (event) {
                    if (event.key === Qt.Key_Down) {
                        popup.move(1)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Up) {
                        popup.move(-1)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        popup.chooseCurrent()
                        event.accepted = true
                    } else if (event.key === Qt.Key_Escape) {
                        popup.close()
                        event.accepted = true
                    }
                }
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: Rectangle {
                    id: row
                    width: ListView.view.width
                    height: 46
                    radius: Math.min(8, theme.radius)
                    color: ListView.isCurrentItem ? Pal.menuSelected : (rowMouse.containsMouse ? Pal.menuHover : "transparent")
                    border.width: ListView.isCurrentItem ? 1 : 0
                    border.color: Pal.borderStrong

                    Rectangle {
                        visible: ListView.isCurrentItem
                        width: 2
                        height: parent.height - 16
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 1
                        color: Pal.teal
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10

                        Rectangle {
                            width: 6
                            height: 6
                            radius: 3
                            color: popup.providerColor(modelData.value)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                Layout.fillWidth: true
                                text: popup.displayName(modelData)
                                color: Pal.text
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: popup.secondary(modelData)
                                color: Pal.dim
                                font.family: backend.monoFont
                                font.pixelSize: 9
                                elide: Text.ElideRight
                            }
                        }

                        Image {
                            source: "icons/check.svg"
                            sourceSize.width: 13
                            sourceSize.height: 13
                            visible: modelData.value === popup.currentValue
                            opacity: 0.85
                        }
                    }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: list.currentIndex = index
                        onClicked: popup.choose(modelData.value)
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: popup.filtered.length === 0
                text: qsTr("No match. Try another name.")
                color: Pal.dim
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("↑↓ navigate · Enter select · Esc close")
                color: Pal.dim
                font.pixelSize: 9
                horizontalAlignment: Text.AlignRight
            }
        }
    }
}
