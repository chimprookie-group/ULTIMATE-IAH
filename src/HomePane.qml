import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    signal openSessions()
    signal openPalette()

    readonly property int contentWidth: Math.max(320, Math.min(940, width - 64))
    readonly property int cardWidth: contentWidth >= 720 ? Math.floor((contentWidth - 12) / 2) : contentWidth

    function stateColor(state) {
        if (state === "running" || state === "ready")
            return theme.green
        if (state === "starting")
            return theme.yellow
        if (state === "failed")
            return theme.red
        return theme.dimText
    }

    function stateLabel(state) {
        if (state === "running")
            return qsTr("Running")
        if (state === "ready")
            return qsTr("Ready")
        if (state === "starting")
            return qsTr("Starting…")
        if (state === "failed")
            return qsTr("Failed")
        return qsTr("Stopped")
    }

    function healthColor() {
        if (backend.harnessHealth === "ok")
            return theme.green
        if (backend.harnessHealth === "failed")
            return theme.red
        if (backend.harnessHealth === "checking")
            return theme.yellow
        return theme.dimText
    }

    function healthLabel() {
        if (backend.harnessHealth === "ok")
            return qsTr("connected")
        if (backend.harnessHealth === "failed")
            return qsTr("unreachable")
        if (backend.harnessHealth === "checking")
            return qsTr("testing…")
        if (backend.harnessHealth === "offline")
            return qsTr("offline")
        return qsTr("not tested")
    }

    function dirtyLabel() {
        if (!backend.gitAvailable)
            return ""
        if (backend.gitDirtyCount === 0)
            return qsTr("clean")
        return qsTr("%n change(s)", "", backend.gitDirtyCount)
    }

    function usageLabel() {
        const usage = backend.usage
        if (usage.used === undefined)
            return ""
        const used = usage.used >= 1000 ? (usage.used / 1000).toFixed(1) + "k" : usage.used
        const size = usage.size ? (usage.size >= 1000 ? Math.round(usage.size / 1000) + "k" : usage.size) : ""
        return size ? qsTr("%1 / %2 tokens").arg(used).arg(size) : qsTr("%1 tokens").arg(used)
    }

    function refresh() {
        backend.refreshGit()
        backend.runDiagnostics()
        if (backend.opencodeState === "ready")
            backend.refreshSessions()
    }

    Component.onCompleted: if (visible) Qt.callLater(refresh)
    onVisibleChanged: if (visible) refresh()

    Connections {
        target: backend

        function onHarnessChanged() {
            if (visible)
                backend.runDiagnostics()
        }

        function onOpencodeChanged() {
            if (!visible)
                return
            backend.runDiagnostics()
            if (backend.opencodeState === "ready" && backend.sessions.length === 0)
                backend.refreshSessions()
        }
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Column {
            id: content
            width: root.contentWidth
            x: Math.max(24, (scroll.availableWidth - width) / 2)
            topPadding: 26
            bottomPadding: 32
            spacing: 14

            // ── hero ─────────────────────────────────────────────────────
            RowLayout {
                width: parent.width
                spacing: 16

                Item {
                    width: 74
                    height: 74
                    Layout.alignment: Qt.AlignTop

                    Rectangle {
                        anchors.centerIn: parent
                        width: 96
                        height: 96
                        radius: 48
                        color: theme.accent
                        opacity: theme.dark ? 0.13 : 0.07
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: 74
                        height: 74
                        radius: 37
                        color: theme.cyan
                        opacity: 0.05
                    }
                    Image {
                        anchors.centerIn: parent
                        source: "assets/icon-256.png"
                        sourceSize.width: 62
                        sourceSize.height: 62
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3

                    Text {
                        text: qsTr("Crasy Harness")
                        color: theme.brightText
                        font.pixelSize: 26
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("DeepSeek Harness × OpenCode") + " · " + backend.workspaceName
                        color: theme.mutedText
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        spacing: 14

                        Row {
                            spacing: 6
                            StatusDot {
                                anchors.verticalCenter: parent.verticalCenter
                                color: root.stateColor(backend.harnessState)
                            }
                            Text {
                                text: qsTr("Harness")
                                color: theme.mutedText
                                font.pixelSize: 10
                            }
                        }
                        Row {
                            spacing: 6
                            StatusDot {
                                anchors.verticalCenter: parent.verticalCenter
                                color: root.stateColor(backend.opencodeState)
                            }
                            Text {
                                text: qsTr("OpenCode")
                                color: theme.mutedText
                                font.pixelSize: 10
                            }
                        }
                        Rectangle {
                            visible: backend.currentModel !== ""
                            implicitWidth: modelLabel.implicitWidth + 14
                            height: 18
                            radius: 9
                            color: theme.chip
                            border.width: 1
                            border.color: theme.border
                            Text {
                                id: modelLabel
                                anchors.centerIn: parent
                                text: backend.currentModel
                                color: theme.mutedText
                                font.family: backend.monoFont
                                font.pixelSize: 8
                            }
                        }
                    }
                }

                ColumnLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignTop

                    ActionButton {
                        text: qsTr("New session")
                        onClicked: {
                            backend.newSession()
                            backend.setView("opencode")
                        }
                    }
                    Button {
                        text: qsTr("Command palette")
                        flat: true
                        onClicked: root.openPalette()
                    }
                }
            }

            // ── quick actions ────────────────────────────────────────────
            Flow {
                width: parent.width
                spacing: 8

                Button {
                    text: qsTr("Open the harness")
                    flat: true
                    onClicked: backend.setView("harness")
                }
                Button {
                    text: qsTr("Split view")
                    flat: true
                    onClicked: backend.setView("split")
                }
                Button {
                    text: qsTr("Terminal")
                    flat: true
                    onClicked: backend.setView("terminal")
                }
                Button {
                    text: qsTr("Change workspace")
                    flat: true
                    onClicked: backend.pickWorkspace()
                }
                Button {
                    text: qsTr("Run diagnostics")
                    flat: true
                    onClicked: backend.runDiagnostics()
                }
            }

            // ── cards ────────────────────────────────────────────────────
            Flow {
                width: parent.width
                spacing: 12

                HomeCard {
                    title: qsTr("Workspace")
                    icon: "icons/branch.svg"
                    cardWidth: root.cardWidth

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 7

                        ThemedIcon {
                            source: "icons/folder.svg"
                            width: 12
                            height: 12
                            color: theme.mutedText
                        }
                        Text {
                            Layout.fillWidth: true
                            text: backend.workspace
                            color: theme.text
                            font.family: backend.monoFont
                            font.pixelSize: 10
                            elide: Text.ElideMiddle
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 7

                        ThemedIcon {
                            source: "icons/branch.svg"
                            width: 11
                            height: 11
                            color: !backend.gitAvailable ? theme.dimText
                                 : (backend.gitDirtyCount > 0 ? theme.orange : theme.green)
                        }
                        Text {
                            Layout.fillWidth: true
                            text: backend.gitAvailable
                                  ? (backend.gitBranch !== "" ? backend.gitBranch : qsTr("repository"))
                                  : qsTr("not a git repository")
                            color: theme.mutedText
                            font.family: backend.monoFont
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                        Text {
                            visible: text !== ""
                            text: root.dirtyLabel()
                            color: backend.gitDirtyCount > 0 ? theme.orange : theme.green
                            font.pixelSize: 9
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            text: qsTr("Change")
                            flat: true
                            onClicked: backend.pickWorkspace()
                        }
                        Button {
                            text: qsTr("Open folder")
                            flat: true
                            onClicked: Qt.openUrlExternally("file://" + backend.workspace)
                        }
                        Item { Layout.fillWidth: true }
                    }
                }

                HomeCard {
                    title: qsTr("DeepSeek Harness")
                    icon: "icons/terminal.svg"
                    accent: theme.blue
                    cardWidth: root.cardWidth

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 7

                        StatusDot {
                            color: root.stateColor(backend.harnessState)
                        }
                        Text {
                            text: root.stateLabel(backend.harnessState)
                            color: theme.text
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }
                        Item { Layout.fillWidth: true }

                        Rectangle {
                            implicitWidth: healthText.implicitWidth + 14
                            height: 18
                            radius: 9
                            color: theme.chip
                            border.width: 1
                            border.color: root.healthColor()
                            Text {
                                id: healthText
                                anchors.centerIn: parent
                                text: root.healthLabel()
                                color: root.healthColor()
                                font.pixelSize: 8
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: backend.harnessProfile + " · " + backend.harnessDetail
                        color: theme.mutedText
                        font.pixelSize: 9
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.harnessAddress
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: 9
                        elide: Text.ElideRight
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            text: qsTr("Open")
                            flat: true
                            onClicked: backend.setView("harness")
                        }
                        Button {
                            text: qsTr("Test")
                            flat: true
                            onClicked: backend.testHarness()
                        }
                        Button {
                            text: qsTr("Restart")
                            flat: true
                            onClicked: backend.restartHarness()
                        }
                        Button {
                            text: qsTr("Browser")
                            flat: true
                            enabled: backend.harnessUrl !== ""
                            onClicked: Qt.openUrlExternally(backend.harnessUrl)
                        }
                    }
                }

                HomeCard {
                    title: qsTr("OpenCode")
                    icon: "icons/chat.svg"
                    accent: theme.cyan
                    cardWidth: root.cardWidth

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 7

                        StatusDot {
                            color: root.stateColor(backend.opencodeState)
                        }
                        Text {
                            Layout.fillWidth: true
                            text: backend.sessionTitle !== "" ? backend.sessionTitle : root.stateLabel(backend.opencodeState)
                            color: theme.text
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                        Text {
                            visible: backend.busy
                            text: qsTr("working…")
                            color: theme.yellow
                            font.pixelSize: 9
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: (backend.currentModel !== "" ? backend.currentModel : qsTr("no model"))
                              + " · " + (backend.currentMode !== "" ? backend.currentMode : qsTr("no mode"))
                        color: theme.mutedText
                        font.pixelSize: 9
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.usageLabel()
                        visible: text !== ""
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: 9
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            text: qsTr("Chat")
                            flat: true
                            onClicked: backend.setView("opencode")
                        }
                        Button {
                            text: qsTr("New")
                            flat: true
                            onClicked: {
                                backend.newSession()
                                backend.setView("opencode")
                            }
                        }
                        Button {
                            text: qsTr("History")
                            flat: true
                            onClicked: root.openSessions()
                        }
                    }
                }

                HomeCard {
                    title: qsTr("Connections")
                    icon: "icons/plug.svg"
                    accent: theme.green
                    cardWidth: root.cardWidth

                    Repeater {
                        model: backend.diagnostics

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7
                            visible: index < 6

                            StatusDot {
                                size: 4
                                color: modelData.ok ? theme.green : theme.red
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.label
                                color: theme.text
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.maximumWidth: 180
                                text: modelData.detail
                                color: theme.dimText
                                font.pixelSize: 8
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: backend.diagnostics.length === 0
                        text: qsTr("Run the diagnostic to check both engines, the portal, and the workspace.")
                        color: theme.mutedText
                        font.pixelSize: 9
                        wrapMode: Text.Wrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Button {
                            text: backend.diagnosticsRunning ? qsTr("Checking…") : qsTr("Run diagnostic")
                            flat: true
                            enabled: !backend.diagnosticsRunning
                            onClicked: backend.runDiagnostics()
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: qsTr("%1 checks").arg(backend.diagnostics.length)
                            color: theme.dimText
                            font.pixelSize: 8
                        }
                    }
                }
            }

            // ── resume ───────────────────────────────────────────────────
            HomeCard {
                title: qsTr("Resume")
                icon: "icons/history.svg"
                accent: theme.magenta
                cardWidth: root.contentWidth

                Repeater {
                    model: backend.sessions

                    Rectangle {
                        Layout.fillWidth: true
                        height: 42
                        visible: index < 3
                        radius: Math.min(8, theme.radius)
                        color: sessionMouse.containsMouse ? theme.chip : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                text: backend.sessionLabel(modelData)
                                color: theme.text
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                            Text {
                                text: modelData.updatedAt !== undefined
                                      ? Qt.formatDateTime(new Date(modelData.updatedAt), "d MMM · HH:mm")
                                      : ""
                                color: theme.dimText
                                font.pixelSize: 8
                            }
                            ThemedIcon {
                                source: "icons/chevron.svg"
                                width: 9
                                height: 9
                                rotation: -90
                                color: theme.dimText
                            }
                        }

                        MouseArea {
                            id: sessionMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                backend.loadSession(modelData.sessionId)
                                backend.setView("opencode")
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: backend.sessions.length === 0
                    text: backend.loadingSessions ? qsTr("Loading sessions…") : qsTr("No sessions in this workspace yet.")
                    color: theme.mutedText
                    font.pixelSize: 9
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Button {
                        text: qsTr("Refresh")
                        flat: true
                        onClicked: backend.refreshSessions()
                    }
                    Button {
                        text: qsTr("All sessions")
                        flat: true
                        onClicked: root.openSessions()
                    }
                    Item { Layout.fillWidth: true }
                }
            }

            Text {
                width: parent.width
                text: qsTr("Ctrl+K opens the command palette · Ctrl+1/2/3 switch views")
                color: theme.dimText
                font.pixelSize: 9
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
