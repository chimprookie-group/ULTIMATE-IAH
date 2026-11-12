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

    function openExternal(url) {
        const value = String(url)
        if (value === "" || value.startsWith("about:"))
            return
        if (value.indexOf("http://127.0.0.1") === 0 || value.indexOf("http://localhost") === 0) {
            root.setPageUrl(value)
            return
        }
        Qt.openUrlExternally(value)
        root.hostWindow.showToast(qsTr("Lien ouvert dans le navigateur."))
    }
    property bool armed: false
    property var profile: null
    readonly property var hostWindow: Window.window
    readonly property var web: pageLoader.item

    function ensureProfile() {
        if (!profile)
            profile = profileComponent.createObject(root)
    }

    function setPageUrl(value) {
        if (value !== "")
            ensureProfile()
        root.pageUrl = value
    }

    Component {
        id: profileComponent

        WebEngineProfile {
            storageName: "mausbot"
            offTheRecord: false
            persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
        }
    }

    Timer {
        interval: 3000
        repeat: true
        running: root.visible && mausBot.running && !mausBot.pairing
                 && (!root.pairAttempted || root.pageUrl.indexOf("/pair") >= 0)
        onTriggered: {
            if (root.pageUrl === "" || !web)
                return
            if (root.pageUrl.indexOf("/pair") >= 0 && !root.pairClicked) {
                root.pairClicked = true
                web.runJavaScript("(() => { const b = [...document.querySelectorAll('button')].find(x => /^(connect|connexion)$/i.test((x.innerText || '').trim())); if (b) b.click(); })()")
                return
            }
            web.runJavaScript("document.body ? document.body.innerText : ''", function (text) {
                const value = String(text || "")
                const needsPair = value.indexOf("Pairing code") >= 0
                    || value.indexOf("Connect to Ultimate HAI") >= 0
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
        if (visible) {
            armed = true
            Qt.callLater(openIfNeeded)
        }
    }
    onVisibleChanged: {
        if (visible) {
            armed = true
            openIfNeeded()
        }
    }

    function openIfNeeded() {
        mausBot.refresh()
        if (mausBot.running && root.pageUrl === "")
            root.setPageUrl(mausBot.url)
        if (mausBot.installed && !mausBot.running && !mausBot.starting)
            mausBot.start()
    }

    Connections {
        target: mausBot
        function onMcpConnected() {
            backend.restartHarness()
            root.hostWindow.showToast(qsTr("MausBot MCP connected to the harness."))
        }
        function onMcpDisconnected() {
            backend.restartHarness()
            root.hostWindow.showToast(qsTr("MausBot MCP disconnected."))
        }
        function onRunningChanged() {
            root.setPageUrl(mausBot.running ? mausBot.url : "")
        }
        function onPairUrlChanged() {
            if (mausBot.pairUrl !== "") {
                root.pairClicked = false
                root.setPageUrl(mausBot.pairUrl)
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

                Loader {
                    id: pageLoader
                    anchors.fill: parent
                    active: root.armed && root.pageUrl !== ""
                    sourceComponent: pageViewComponent
                }

                Component {
                    id: pageViewComponent

                    WebEngineView {
                        id: web
                        visible: mausBot.running
                        backgroundColor: theme.background
                        profile: root.profile
                        settings.javascriptEnabled: true
                        settings.localStorageEnabled: true
                        url: root.visible ? root.pageUrl : ""

                        onNewWindowRequested: function (request) {
                            const value = String(request.requestedUrl)
                            if (value.indexOf("http://127.0.0.1") === 0 || value.indexOf("http://localhost") === 0)
                                request.openIn(web)
                            else
                                root.openExternal(request.requestedUrl)
                        }
                        onNavigationRequested: function (request) {
                            const value = String(request.url)
                            if (request.isMainFrame && value.indexOf("http") === 0
                                    && value.indexOf("http://127.0.0.1") !== 0
                                    && value.indexOf("http://localhost") !== 0) {
                                Qt.openUrlExternally(request.url)
                                request.reject()
                            } else {
                                request.accept()
                            }
                        }
                        onLoadingChanged: function (request) {
                            if (request.status === WebEngineView.LoadFailedStatus && mausBot.running)
                                root.hostWindow.showToast(qsTr("The MausBot page did not load."))
                            if (request.status === WebEngineView.LoadSucceededStatus && mausBot.running) {
                                Qt.callLater(function () {
                                    if (!web)
                                        return
                                    web.runJavaScript("document.body.innerText", function (text) {
                                        const value = String(text || "")
                                        const needsPair = value.indexOf("Pairing code") >= 0
                                            || value.indexOf("Connect to Ultimate HAI") >= 0
                                            || value.indexOf("Connect to DeepCrasy") >= 0
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
                                  : qsTr("MausBot is a local-first team of AI bots (Claude, Codex, Grok) with their own computers and apps. It installs locally in Ultimate HAI, without admin rights.")
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
                        text: qsTr("Expose the MausBot team to the harness through its stdio MCP server. Tools appear as mcp__openmausbot__*.")
                        color: theme.dimText
                        font.pixelSize: 9
                        wrapMode: Text.Wrap
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
                        text: qsTr("Connect to harness")
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
