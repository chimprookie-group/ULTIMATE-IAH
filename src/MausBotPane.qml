import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtWebEngine

Item {
    id: root

    property bool mcpOpen: false
    property string pageUrl: ""
    property bool pairAttempted: false
    property bool pairClicked: false
    readonly property var hostWindow: Window.window

    Timer {
        interval: 3000
        repeat: true
        running: mausBot.running && !mausBot.pairing
                 && (!root.pairAttempted || root.pageUrl.indexOf("/pair") >= 0)
        onTriggered: {
            if (root.pageUrl === "")
                return
            if (root.pageUrl.indexOf("/pair") >= 0 && !root.pairClicked) {
                root.pairClicked = true
                web.runJavaScript("(() => { const b = [...document.querySelectorAll('button')].find(x => /^(connect|connexion)$/i.test((x.innerText || '').trim())); if (b) b.click(); })()")
                return
            }
            web.runJavaScript("document.body ? document.body.innerText : ''", function (text) {
                const value = String(text || "")
                const needsPair = value.indexOf("Pairing code") >= 0
                    || value.indexOf("Connect to DeepCrasy") >= 0
                    || value.indexOf("unauthorized") >= 0
                if (needsPair) {
                    root.pairAttempted = true
                    mausBot.pair()
                } else if (value.length > 0) {
                    root.pairAttempted = true
                }
            })
        }
    }

    Component.onCompleted: {
        if (visible) Qt.callLater(openIfNeeded)
    }
    onVisibleChanged: if (visible) openIfNeeded()

    function openIfNeeded() {
        mausBot.refresh()
        if (mausBot.installed && !mausBot.running && !mausBot.starting)
            mausBot.start()
    }

    Connections {
        target: mausBot
        function onMcpConnected() {
            backend.restartOpencode()
            backend.restartHarness()
            root.hostWindow.showToast(qsTr("MausBot MCP connected to OpenCode and the harness."))
        }
        function onMcpDisconnected() {
            backend.restartOpencode()
            backend.restartHarness()
            root.hostWindow.showToast(qsTr("MausBot MCP disconnected."))
        }
        function onRunningChanged() {
            root.pageUrl = mausBot.running ? mausBot.url : ""
        }
        function onPairUrlChanged() {
            if (mausBot.pairUrl !== "") {
                root.pairClicked = false
                root.pageUrl = mausBot.pairUrl
            }
        }
        function onErrorChanged() {
            if (mausBot.error !== "")
                root.hostWindow.showToast(mausBot.error)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            color: theme.surface

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 14
                spacing: 10

                Item {
                    width: 34
                    height: 34
                    Rectangle {
                        anchors.centerIn: parent
                        width: 32
                        height: 32
                        radius: 10
                        color: theme.magenta
                        opacity: 0.14
                    }
                    ThemedIcon {
                        anchors.centerIn: parent
                        source: "icons/mausbot.svg"
                        width: 18
                        height: 18
                        color: theme.magenta
                    }
                }

                ColumnLayout {
                    spacing: 1
                    Text {
                        text: "MausBot"
                        color: theme.brightText
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                    RowLayout {
                        spacing: 6
                        StatusDot {
                            color: mausBot.running ? theme.green
                                   : (mausBot.starting || mausBot.installing ? theme.yellow : theme.dimText)
                        }
                        Text {
                            text: mausBot.detail + (mausBot.version !== "" ? " · v" + mausBot.version : "")
                            color: theme.dimText
                            font.pixelSize: 9
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    visible: mausBot.running
                    text: qsTr("Pair browser")
                    flat: true
                    onClicked: mausBot.pair()
                }
                Button {
                    visible: mausBot.running
                    text: qsTr("MCP bridge")
                    flat: true
                    highlighted: root.mcpOpen
                    onClicked: root.mcpOpen = !root.mcpOpen
                }
                Button {
                    visible: mausBot.running
                    text: qsTr("Open browser")
                    flat: true
                    onClicked: Qt.openUrlExternally(mausBot.url)
                }
                Button {
                    visible: mausBot.running
                    text: qsTr("Stop")
                    flat: true
                    onClicked: mausBot.stop()
                }
                Button {
                    visible: mausBot.installed && !mausBot.running && !mausBot.installing
                    text: mausBot.starting ? qsTr("Starting…") : qsTr("Start")
                    enabled: !mausBot.starting
                    flat: true
                    onClicked: mausBot.start()
                }
                ActionButton {
                    visible: !mausBot.installed
                    text: mausBot.installing ? qsTr("Installing…") : qsTr("Install MausBot")
                    enabled: !mausBot.installing
                    onClicked: mausBot.install()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                WebEngineProfile {
                    id: mausProfile
                    storageName: "mausbot"
                    offTheRecord: false
                    persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
                }

                WebEngineView {
                    id: web
                    anchors.fill: parent
                    visible: mausBot.running
                    backgroundColor: theme.background
                    profile: mausProfile
                    settings.javascriptEnabled: true
                    settings.localStorageEnabled: true
                    url: root.pageUrl

                    onLoadingChanged: function (request) {
                        if (request.status === WebEngineView.LoadFailedStatus && mausBot.running)
                            root.hostWindow.showToast(qsTr("The MausBot page did not load."))
                        if (request.status === WebEngineView.LoadSucceededStatus && mausBot.running) {
                            Qt.callLater(function () {
                                web.runJavaScript("document.body.innerText", function (text) {
                                    const value = String(text || "")
                                    const needsPair = value.indexOf("Pairing code") >= 0
                                        || value.indexOf("Connect to DeepCrasy H") >= 0
                                        || value.indexOf("unauthorized") >= 0
                                    if (needsPair && !root.pairAttempted && !mausBot.pairing) {
                                        root.pairAttempted = true
                                        mausBot.pair()
                                    }
                                })
                            })
                        }
                    }
                }

                Rectangle {
                    visible: !mausBot.running
                    anchors.centerIn: parent
                    width: Math.min(580, parent.width - 60)
                    height: Math.max(280, welcome.implicitHeight + 54)
                    radius: Math.min(14, theme.radius)
                    color: theme.surface
                    border.width: 1
                    border.color: theme.border

                    ColumnLayout {
                        id: welcome
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 9

                        RowLayout {
                            spacing: 10
                            ThemedIcon { source: "icons/mausbot.svg"; width: 24; height: 24; color: theme.magenta }
                            Text {
                                Layout.fillWidth: true
                                text: mausBot.installed ? qsTr("MausBot is not running") : qsTr("Install MausBot")
                                color: theme.brightText
                                font.pixelSize: 17
                                font.weight: Font.Bold
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: mausBot.installed
                                  ? qsTr("Start the local MausBot workspace and manage your team of bots in its embedded app.")
                                  : qsTr("MausBot is a local-first team of AI bots (Claude, Codex, Grok) with their own computers and apps. It installs locally in DeepCrasy H, without admin rights.")
                            color: theme.mutedText
                            font.pixelSize: 10
                            wrapMode: Text.Wrap
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: mausBot.installReport !== ""
                            text: mausBot.installReport
                            color: theme.green
                            font.pixelSize: 9
                            wrapMode: Text.Wrap
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: mausBot.installing
                            text: qsTr("Installing the local package… this can take a minute.")
                            color: theme.yellow
                            font.pixelSize: 9
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: mausBot.installLog !== ""
                            text: mausBot.installLog
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: 8
                            wrapMode: Text.Wrap
                            maximumLineCount: 4
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: mausBot.privilegedLog !== ""
                            text: mausBot.privilegedLog
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: 8
                            wrapMode: Text.Wrap
                            maximumLineCount: 4
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: mausBot.error !== ""
                            text: mausBot.error
                            color: theme.red
                            font.pixelSize: 9
                            wrapMode: Text.Wrap
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Item { Layout.fillWidth: true }
                            Button {
                                visible: mausBot.installed
                                text: mausBot.starting ? qsTr("Starting…") : qsTr("Start MausBot")
                                enabled: !mausBot.starting
                                flat: true
                                onClicked: mausBot.start()
                            }
                            Button {
                                text: mausBot.privilegedBusy ? qsTr("Authorizing…") : qsTr("System dependencies (sudo)")
                                flat: true
                                font.pixelSize: 9
                                enabled: !mausBot.privilegedBusy && !mausBot.installing
                                onClicked: mausBot.installDependencies()
                            }
                            ActionButton {
                                visible: !mausBot.installed
                                text: mausBot.installing ? qsTr("Installing…") : qsTr("Install MausBot")
                                enabled: !mausBot.installing
                                onClicked: mausBot.install()
                            }
                        }
                    }
                }
            }

            Rectangle {
                visible: root.mcpOpen && mausBot.running
                Layout.preferredWidth: 390
                Layout.fillHeight: true
                color: theme.surface

                Rectangle {
                    anchors.left: parent.left
                    width: 1
                    height: parent.height
                    color: theme.border
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 9

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("MCP bridge")
                            color: theme.brightText
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }
                        IconButton {
                            icon: "icons/close.svg"
                            hint: qsTr("Close")
                            onClicked: root.mcpOpen = false
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Expose the MausBot team to OpenCode and the harness through its stdio MCP server. Tools appear as mcp__openmausbot__*.")
                        color: theme.dimText
                        font.pixelSize: 9
                        wrapMode: Text.Wrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        StatusDot { color: mausBot.mcpOpenCode ? theme.green : theme.dimText }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("OpenCode MCP")
                            color: theme.text
                            font.pixelSize: 10
                        }
                        Text {
                            text: mausBot.mcpOpenCode ? qsTr("registered") : qsTr("not registered")
                            color: mausBot.mcpOpenCode ? theme.green : theme.dimText
                            font.pixelSize: 8
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        StatusDot { color: mausBot.mcpHarness ? theme.green : theme.dimText }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Harness MCP")
                            color: theme.text
                            font.pixelSize: 10
                        }
                        Text {
                            text: mausBot.mcpHarness ? qsTr("registered") : qsTr("not registered")
                            color: mausBot.mcpHarness ? theme.green : theme.dimText
                            font.pixelSize: 8
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("MausBot must stay running for its tools. Read-only tools work on loopback; pairing may be required for actions that change your team.")
                        color: theme.dimText
                        font.pixelSize: 8
                        wrapMode: Text.Wrap
                    }

                    ActionButton {
                        Layout.fillWidth: true
                        text: qsTr("Connect to OpenCode + harness")
                        onClicked: mausBot.connectMcp()
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Disconnect MCP")
                        flat: true
                        font.pixelSize: 9
                        onClicked: mausBot.disconnectMcp()
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Available agents: Claude, Codex and Grok CLIs are detected automatically by MausBot.")
                        color: theme.dimText
                        font.pixelSize: 8
                        wrapMode: Text.Wrap
                    }

                    Item { Layout.fillHeight: true }

                    Text {
                        Layout.fillWidth: true
                        visible: mausBot.logTail !== ""
                        text: mausBot.logTail
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: 7
                        wrapMode: Text.Wrap
                        maximumLineCount: 6
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
