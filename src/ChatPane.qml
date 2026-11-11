import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts


Item {
    id: pane

    property bool pinned: true
    property var suggestions: [
        qsTr("Explain this repository"),
        qsTr("Review my uncommitted changes"),
        qsTr("Write tests for the last change")
    ]

    signal openSessions()
    signal changeModelRequested()
    signal connectProviderRequested()

    function focusComposer() {
        composer.focusInput()
    }

    function openModelPicker() {
        composer.openModelPicker()
    }

    function attachFile(path, content) {
        composer.attachFile(path, content)
        backend.setView("opencode")
        Qt.callLater(function () { composer.focusInput() })
    }

    function suggest(text) {
        composer.setText(text)
    }

    function send(text) {
        if (text.trim() === "" || backend.busy)
            return
        backend.sendPrompt(text)
        pinned = true
        list.positionViewAtEnd()
        followEnd()
    }

    function followEnd() {
        if (pinned && !autoScroll.running)
            autoScroll.start()
    }

    function planColor(status) {
        if (status === "completed")
            return theme.green
        if (status === "in_progress")
            return theme.cyan
        return theme.dimText
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── header ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: theme.background

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.border
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
                            color: theme.text
                            elide: Text.ElideRight
                        }
                        Rectangle {
                            visible: backend.busy
                            width: 5
                            height: 5
                            radius: 2.5
                            color: theme.cyan
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
                        color: theme.dimText
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
                anchors.bottomMargin: 8
                clip: true
                spacing: 14
                model: backend.messageModel
                boundsBehavior: Flickable.StopAtBounds
                footer: Item { width: 1; height: 12 }
                onMovementStarted: if (dragging || flicking) pane.pinned = false
                onMovementEnded: {
                    if (!backend.busy || dragging || flicking || atYEnd)
                        pane.pinned = atYEnd
                }
                Component.onCompleted: pane.followEnd()
                onCountChanged: pane.followEnd()
                onContentHeightChanged: pane.followEnd()

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
                            visible: model.role === "user"

                            Rectangle {
                                id: userCard
                                width: parent.width
                                height: userLabel.implicitHeight + 18
                                radius: Math.min(9, theme.radius)
                                color: theme.raised
                                border.width: 1
                                border.color: theme.border

                                Rectangle {
                                    width: 2
                                    height: parent.height - 12
                                    y: 6
                                    radius: 1
                                    color: theme.accent
                                }

                                Text {
                                    id: userLabel
                                    anchors.fill: parent
                                    anchors.margins: 9
                                    anchors.leftMargin: 12
                                    text: model.text !== undefined ? model.text : ""
                                    wrapMode: Text.Wrap
                                    color: theme.text
                                    font.pixelSize: 13
                                }
                            }
                        }

                        Rectangle {
                            visible: model.role === "error"
                            width: parent.width
                            height: errorColumn.implicitHeight + 20
                            radius: Math.min(10, theme.radius)
                            color: theme.chip
                            border.width: 1
                            border.color: theme.red

                            ColumnLayout {
                                id: errorColumn
                                x: 12
                                y: 10
                                width: parent.width - 24
                                spacing: 7

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 7
                                    ThemedIcon {
                                        source: "icons/alert.svg"
                                        width: 14
                                        height: 14
                                        color: model.kind === "usage" ? theme.yellow : theme.red
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: model.kind === "usage"
                                              ? qsTr("OpenCode usage limit")
                                              : (model.kind === "provider"
                                                 ? qsTr("Provider authentication required")
                                                 : qsTr("OpenCode needs attention"))
                                        color: theme.text
                                        font.pixelSize: 11
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: model.text !== undefined ? model.text : ""
                                    color: theme.mutedText
                                    font.pixelSize: 10
                                    wrapMode: Text.Wrap
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    Button {
                                        text: qsTr("Change model")
                                        flat: true
                                        font.pixelSize: 9
                                        onClicked: pane.changeModelRequested()
                                    }
                                    Button {
                                        text: model.kind === "provider" ? qsTr("Manage providers") : qsTr("Connect a provider")
                                        flat: true
                                        font.pixelSize: 9
                                        onClicked: pane.connectProviderRequested()
                                    }
                                    Button {
                                        text: qsTr("Retry")
                                        flat: true
                                        font.pixelSize: 9
                                        onClicked: backend.retryLastPrompt()
                                    }
                                    Item { Layout.fillWidth: true }
                                }
                            }
                        }

                        Text {
                            visible: model.role === "assistant"
                            width: parent.width
                            text: model.text !== undefined ? model.text : ""
                            textFormat: Text.MarkdownText
                            wrapMode: Text.Wrap
                            color: theme.text
                            font.pixelSize: 13
                            lineHeight: 1.4
                            onLinkActivated: function (link) { Qt.openUrlExternally(link) }
                        }

                        Rectangle {
                            id: caret
                            visible: model.role === "assistant" && model.streaming
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
                            visible: model.role === "thought"
                            width: parent.width
                            text: "thinking · " + model.text
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            color: theme.dimText
                            font.pixelSize: 11
                            font.italic: true
                        }

                        ToolCard {
                            visible: model.role === "tool"
                            title: model.title !== undefined ? model.title : ""
                            kind: model.kind !== undefined ? model.kind : ""
                            status: model.status !== undefined ? model.status : "pending"
                            output: model.output !== undefined ? model.output : ""
                            command: (model.input !== undefined && model.input !== null && model.input.command !== undefined) ? model.input.command : ""
                            locations: model.locations !== undefined ? model.locations : []
                        }

                        Rectangle {
                            id: planCard
                            visible: model.role === "plan"
                            property var planEntries: model.entries !== undefined ? model.entries : []
                            width: parent.width
                            height: planColumn.implicitHeight + 20
                            radius: Math.min(9, theme.radius)
                            color: theme.raised
                            border.width: 1
                            border.color: theme.border

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
                                    color: theme.dimText
                                }
                                Repeater {
                                    model: planCard.planEntries
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
                                            color: theme.mutedText
                                            font.pixelSize: 12
                                        }
                                    }
                                }
                            }
                        }

                        Text {
                            visible: model.role === "notice"
                            width: parent.width
                            text: model.text !== undefined ? model.text : ""
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            color: theme.dimText
                            font.pixelSize: 10
                        }
                    }
                }
            }

            // ── empty / not ready ─────────────────────────────────────────
            Item {
                anchors.fill: parent
                visible: backend.messageCount === 0

                Column {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 40, 420)
                    spacing: 13
                    visible: backend.opencodeState === "ready"

                    Item {
                        width: 150
                        height: 150
                        anchors.horizontalCenter: parent.horizontalCenter

                        Rectangle {
                            anchors.centerIn: parent
                            width: 138
                            height: 138
                            radius: 69
                            color: theme.accent
                            opacity: theme.dark ? 0.10 : 0.06
                        }
                        Rectangle {
                            anchors.centerIn: parent
                            width: 104
                            height: 104
                            radius: 52
                            color: theme.cyan
                            opacity: theme.dark ? 0.06 : 0.04
                        }
                        Image {
                            anchors.centerIn: parent
                            source: "assets/icon-256.png"
                            sourceSize.width: 88
                            sourceSize.height: 88
                        }
                    }
                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        text: qsTr("One workspace. Two engines.")
                        color: theme.brightText
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                    }
                    Text {
                        width: parent.width
                        text: qsTr("The harness runs beside this chat. Ask OpenCode to read, edit, or run anything in the shared workspace — tool calls and permissions stay visible.")
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        color: theme.mutedText
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
                                color: suggestionMouse.containsMouse ? theme.raisedHover : theme.raised
                                border.width: 1
                                border.color: suggestionMouse.containsMouse ? theme.accent : theme.border

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 11
                                    anchors.rightMargin: 11
                                    spacing: 8
                                    ThemedIcon {
                                        source: "icons/spark.svg"
                                        width: 12
                                        height: 12
                                        color: theme.cyan
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData
                                        color: theme.mutedText
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }
                                    ThemedIcon {
                                        source: "icons/chevron.svg"
                                        width: 10
                                        height: 10
                                        rotation: -90
                                        color: theme.dimText
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
                        color: theme.text
                        font.pixelSize: 13
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: backend.opencodeDetail
                        color: theme.dimText
                        font.pixelSize: 11
                    }
                    ActionButton {
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: backend.opencodeState === "stopped"
                        text: qsTr("Start")
                        onClicked: backend.restartOpencode()
                    }
                }

                Column {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 60, 420)
                    spacing: 10
                    visible: backend.opencodeState === "failed"

                    ThemedIcon {
                        source: "icons/alert.svg"
                        width: 20
                        height: 20
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: theme.yellow
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("OpenCode is not available")
                        color: theme.text
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                    Text {
                        width: parent.width
                        text: backend.opencodeDetail
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        color: theme.mutedText
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
                        ActionButton {
                            text: qsTr("Retry")
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
            pane.followEnd()
        }
    }

    Timer {
        id: autoScroll
        interval: 40
        repeat: false
        onTriggered: {
            if (!pane.pinned)
                return
            if (list.dragging || list.flicking)
                return
            list.positionViewAtEnd()
        }
    }
}
