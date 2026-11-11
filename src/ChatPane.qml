import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import "Palette.js" as Pal

Item {
    id: pane

    property bool pinned: true
    property var suggestions: [
        qsTr("Explain this repository"),
        qsTr("Review my uncommitted changes"),
        qsTr("Write tests for the last change")
    ]

    signal openSessions()

    function focusComposer() {
        composer.focusInput()
    }

    function suggest(text) {
        composer.setText(text)
    }

    function send(text) {
        if (text.trim() === "" || backend.busy)
            return
        backend.sendPrompt(text)
        pinned = true
        Qt.callLater(list.positionViewAtEnd)
    }

    function planColor(status) {
        if (status === "completed")
            return Pal.green
        if (status === "in_progress")
            return Pal.teal
        return Pal.dim
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── header ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: Pal.bg

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Pal.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Math.max(18, (pane.width - 880) / 2)
                anchors.rightMargin: Math.max(8, (pane.width - 880) / 2 - 8)
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    RowLayout {
                        spacing: 6
                        Text {
                            Layout.fillWidth: true
                            text: backend.sessionTitle !== "" ? backend.sessionTitle : qsTr("New session")
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            color: Pal.text
                            elide: Text.ElideRight
                        }
                        Rectangle {
                            visible: backend.busy
                            width: 5
                            height: 5
                            radius: 2.5
                            color: Pal.teal
                            SequentialAnimation on opacity {
                                running: visible
                                loops: Animation.Infinite
                                NumberAnimation { to: 0.25; duration: 600 }
                                NumberAnimation { to: 1.0; duration: 600 }
                            }
                        }
                    }
                    Text {
                        text: backend.sessionId !== "" ? backend.sessionId : qsTr("no session yet")
                        font.family: backend.monoFont
                        font.pixelSize: 9
                        color: Pal.dim
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                IconButton {
                    icon: "icons/history.svg"
                    hint: qsTr("Previous sessions")
                    onClicked: pane.openSessions()
                }
                IconButton {
                    icon: "icons/plus.svg"
                    hint: qsTr("New session (Ctrl+N)")
                    onClicked: backend.newSession()
                }
            }
        }

        // ── transcript ────────────────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                anchors.fill: parent
                anchors.leftMargin: Math.max(18, (parent.width - 880) / 2)
                anchors.rightMargin: Math.max(16, (parent.width - 880) / 2)
                anchors.topMargin: 12
                anchors.bottomMargin: 4
                clip: true
                spacing: 14
                model: backend.messages
                boundsBehavior: Flickable.StopAtBounds
                onMovementEnded: pane.pinned = atYEnd
                Component.onCompleted: Qt.callLater(positionViewAtEnd)
                onCountChanged: if (pane.pinned) Qt.callLater(positionViewAtEnd)
                onContentHeightChanged: if (pane.pinned && backend.busy) Qt.callLater(positionViewAtEnd)

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: Item {
                    id: row
                    width: ListView.view ? ListView.view.width : 0
                    implicitHeight: body.implicitHeight

                    Column {
                        id: body
                        width: parent.width
                        spacing: 7

                        Item {
                            width: parent.width
                            height: userCard.height
                            visible: modelData.role === "user"

                            Rectangle {
                                id: userCard
                                width: parent.width
                                height: userLabel.implicitHeight + 18
                                radius: Math.min(9, theme.radius)
                                color: Pal.raised
                                border.width: 1
                                border.color: Pal.border

                                Rectangle {
                                    width: 2
                                    height: parent.height - 12
                                    y: 6
                                    radius: 1
                                    color: Pal.teal
                                }

                                Text {
                                    id: userLabel
                                    anchors.fill: parent
                                    anchors.margins: 9
                                    anchors.leftMargin: 12
                                    text: modelData.text !== undefined ? modelData.text : ""
                                    wrapMode: Text.Wrap
                                    color: Pal.text
                                    font.pixelSize: 13
                                }
                            }
                        }

                        Text {
                            visible: modelData.role === "assistant"
                            width: parent.width
                            text: modelData.text !== undefined ? modelData.text : ""
                            textFormat: Text.MarkdownText
                            wrapMode: Text.Wrap
                            color: Pal.text
                            font.pixelSize: 13
                            lineHeight: 1.4
                            onLinkActivated: function (link) { Qt.openUrlExternally(link) }
                        }

                        Rectangle {
                            id: caret
                            visible: modelData.role === "assistant" && modelData.streaming
                            width: 34
                            height: 2
                            radius: 1
                            color: theme.accent
                            SequentialAnimation on opacity {
                                running: caret.visible
                                loops: Animation.Infinite
                                NumberAnimation { to: 0.2; duration: 700; easing.type: Easing.InOutSine }
                                NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutSine }
                            }
                        }

                        Text {
                            visible: modelData.role === "thought"
                            width: parent.width
                            text: "thinking · " + modelData.text
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            color: Pal.dim
                            font.pixelSize: 11
                            font.italic: true
                        }

                        ToolCard {
                            visible: modelData.role === "tool"
                            title: modelData.title !== undefined ? modelData.title : ""
                            kind: modelData.kind !== undefined ? modelData.kind : ""
                            status: modelData.status !== undefined ? modelData.status : "pending"
                            output: modelData.output !== undefined ? modelData.output : ""
                            command: (modelData.input !== undefined && modelData.input !== null && modelData.input.command !== undefined) ? modelData.input.command : ""
                            locations: modelData.locations !== undefined ? modelData.locations : []
                        }

                        Rectangle {
                            visible: modelData.role === "plan"
                            width: parent.width
                            height: planColumn.implicitHeight + 20
                            radius: Math.min(9, theme.radius)
                            color: Pal.raised
                            border.width: 1
                            border.color: Pal.border

                            ColumnLayout {
                                id: planColumn
                                x: 10
                                y: 10
                                width: parent.width - 20
                                spacing: 5

                                Text {
                                    text: qsTr("PLAN")
                                    font.pixelSize: 8
                                    font.letterSpacing: 1.2
                                    color: Pal.dim
                                }
                                Repeater {
                                    model: modelData.entries !== undefined ? modelData.entries : []
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 7
                                        Rectangle {
                                            width: 5
                                            height: 5
                                            radius: 2.5
                                            Layout.alignment: Qt.AlignTop
                                            Layout.topMargin: 5
                                            color: pane.planColor(modelData.status)
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: modelData.content
                                            wrapMode: Text.Wrap
                                            color: Pal.muted
                                            font.pixelSize: 12
                                        }
                                    }
                                }
                            }
                        }

                        Text {
                            visible: modelData.role === "notice"
                            width: parent.width
                            text: modelData.text !== undefined ? modelData.text : ""
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            color: Pal.dim
                            font.pixelSize: 10
                        }
                    }
                }
            }

            // ── empty / not ready ─────────────────────────────────────────
            Item {
                anchors.fill: parent
                visible: backend.messages.length === 0

                Column {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 40, 420)
                    spacing: 13
                    visible: backend.opencodeState === "ready"

                    Image {
                        source: "assets/icon-256.png"
                        sourceSize.width: 88
                        sourceSize.height: 88
                        anchors.horizontalCenter: parent.horizontalCenter
                        opacity: 0.95
                    }
                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        text: qsTr("One workspace. Two engines.")
                        color: Pal.text
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }
                    Text {
                        width: parent.width
                        text: qsTr("The harness runs beside this chat. Ask OpenCode to read, edit, or run anything in the shared workspace — tool calls and permissions stay visible.")
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        color: Pal.muted
                        font.pixelSize: 12
                        lineHeight: 1.35
                    }

                    Item { width: 1; height: 4 }

                    Column {
                        width: parent.width
                        spacing: 7

                        Repeater {
                            model: pane.suggestions
                            Rectangle {
                                width: parent.width
                                height: 38
                                radius: Math.min(8, theme.radius)
                                color: suggestionMouse.containsMouse ? Pal.raisedHover : Pal.raised
                                border.width: 1
                                border.color: Pal.border

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 11
                                    anchors.rightMargin: 11
                                    spacing: 8
                                    Image {
                                        source: "icons/spark.svg"
                                        sourceSize.width: 12
                                        sourceSize.height: 12
                                        opacity: 0.55
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData
                                        color: Pal.muted
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }
                                    Image {
                                        source: "icons/chevron.svg"
                                        sourceSize.width: 10
                                        sourceSize.height: 10
                                        rotation: -90
                                        opacity: 0.35
                                    }
                                }

                                MouseArea {
                                    id: suggestionMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: pane.suggest(modelData)
                                }
                            }
                        }
                    }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 12
                    visible: backend.opencodeState === "starting" || backend.opencodeState === "stopped"

                    BusyIndicator {
                        running: visible
                        anchors.horizontalCenter: parent.horizontalCenter
                        implicitWidth: 28
                        implicitHeight: 28
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: backend.opencodeState === "stopped" ? qsTr("OpenCode is stopped") : qsTr("Starting OpenCode…")
                        color: Pal.text
                        font.pixelSize: 13
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: backend.opencodeDetail
                        color: Pal.dim
                        font.pixelSize: 11
                    }
                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: backend.opencodeState === "stopped"
                        text: qsTr("Start")
                        highlighted: true
                        onClicked: backend.restartOpencode()
                    }
                }

                Column {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 60, 420)
                    spacing: 10
                    visible: backend.opencodeState === "failed"

                    Image {
                        source: "icons/alert.svg"
                        sourceSize.width: 20
                        sourceSize.height: 20
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("OpenCode is not available")
                        color: Pal.text
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                    Text {
                        width: parent.width
                        text: backend.opencodeDetail
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        color: Pal.muted
                        font.pixelSize: 12
                    }
                    RowLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 8

                        Button {
                            text: qsTr("Settings")
                            flat: true
                            onClicked: backend.setView("settings")
                        }
                        Button {
                            text: qsTr("Retry")
                            highlighted: true
                            onClicked: backend.restartOpencode()
                        }
                    }
                }
            }
        }

        Composer {
            id: composer
            active: pane.visible
            Layout.fillWidth: true
            Layout.leftMargin: Math.max(14, (pane.width - 908) / 2)
            Layout.rightMargin: Math.max(14, (pane.width - 908) / 2)
            Layout.topMargin: 6
            Layout.bottomMargin: 12
            onSubmit: function (text) { pane.send(text) }
        }
    }

    Connections {
        target: backend
        function onMessagesChanged() {
            if (pane.pinned)
                Qt.callLater(list.positionViewAtEnd)
        }
    }
}
