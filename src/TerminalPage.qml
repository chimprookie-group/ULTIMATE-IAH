import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window
import QtWebChannel
import QtWebEngine

Item {
    id: root

    property bool loaded: false
    readonly property var web: webLoader.item

    function arm() {
        if (loaded)
            return
        loaded = true
        if (web)
            web.url = "qrc:/terminal/index.html?cwd=" + encodeURIComponent(backend.workspace)
    }

    function applyTheme() {
        if (!web)
            return
        const colors = {
            "background": theme.background,
            "foreground": theme.text,
            "cursor": theme.accent,
            "cursorAccent": theme.background,
            "selectionBackground": theme.menuSelected,
            "black": theme.background,
            "red": theme.red,
            "green": theme.green,
            "yellow": theme.yellow,
            "blue": theme.blue,
            "magenta": theme.magenta,
            "cyan": theme.cyan,
            "white": theme.text,
            "brightBlack": theme.dimText,
            "brightRed": theme.red,
            "brightGreen": theme.green,
            "brightYellow": theme.yellow,
            "brightBlue": theme.blue,
            "brightMagenta": theme.magenta,
            "brightCyan": theme.cyan,
            "brightWhite": theme.brightText,
            "dimText": theme.dimText,
            "fontFamily": backend.monoFont
        }
        web.runJavaScript("window.setTerminalTheme && window.setTerminalTheme(" + JSON.stringify(colors) + ")")
    }

    function clear() {
        if (web)
            web.runJavaScript("window.termClear && window.termClear()")
    }

    function shellQuote(value) {
        return "'" + String(value).replace(/'/g, "'\\''") + "'"
    }

    function runAgentTask() {
        const text = agentInput.text.trim()
        if (text === "")
            return
        arm()
        const binary = backend.dshBinary !== "" ? backend.dshBinary : "dsh"
        const env = connections.envPath
        const prefix = "set -a; if [ -f " + shellQuote(env) + " ]; then . " + shellQuote(env) + "; fi; set +a; "
        const context = memory.recall(backend.workspace, text, 8)
        const full = context !== ""
              ? "Mémoire persistante du projet (contexte fiable, ne la répète pas) :\n" + context + "\n\nTâche :\n" + text
              : text
        terminal.run(prefix + binary + " headless " + shellQuote(full))
        agentInput.text = ""
        Qt.callLater(function () {
            if (!web)
                return
            web.forceActiveFocus()
            web.runJavaScript("window.termFocus && window.termFocus()")
        })
    }

    function restart() {
        if (web)
            web.runJavaScript("window.termReset && window.termReset()")
        terminal.start(backend.workspace)
    }

    onVisibleChanged: {
        if (visible) {
            const wasLoaded = loaded
            arm()
            if (wasLoaded && !terminal.running)
                terminal.start(backend.workspace)
            Qt.callLater(function () {
                if (!web)
                    return
                web.forceActiveFocus()
                web.runJavaScript("window.termFocus && window.termFocus()")
            })
        } else {
            Window.window.terminalFocused = false
        }
    }

    Component.onCompleted: {
        terminalChannel.registerObjects({"terminal": terminal})
        if (visible)
            arm()
    }

    WebChannel {
        id: terminalChannel
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: theme.background

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: design.space4
                anchors.rightMargin: design.space2
                spacing: design.space2

                StatusDot {
                    color: terminal.running ? theme.green : theme.dimText
                }
                Text {
                    text: qsTr("Terminal")
                    color: theme.text
                    font.pixelSize: design.textBody
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: terminal.shell.substring(terminal.shell.lastIndexOf("/") + 1) + " · " + backend.workspace
                    color: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: design.textCaption
                    elide: Text.ElideMiddle
                }

                IconButton {
                    icon: "icons/trash.svg"
                    hint: qsTr("Clear the terminal")
                    onClicked: root.clear()
                }
                IconButton {
                    icon: "icons/reload.svg"
                    hint: qsTr("Restart the shell")
                    onClicked: root.restart()
                }
            }
        }

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
                anchors.leftMargin: design.space4
                anchors.rightMargin: design.space2
                spacing: design.space2

                ThemedIcon {
                    source: "icons/harness.svg"
                    width: 14
                    height: 14
                    color: theme.accent
                }
                TextField {
                    id: agentInput
                    Layout.fillWidth: true
                    placeholderText: qsTr("Confier une tâche au harness dans ce terminal…")
                    color: theme.text
                    font.pixelSize: design.textLabel
                    selectByMouse: true
                    onAccepted: root.runAgentTask()
                    background: Rectangle {
                        radius: Math.min(6, theme.radius)
                        color: theme.raised
                        border.width: 1
                        border.color: agentInput.activeFocus ? theme.accent : theme.border
                    }
                }
                ActionButton {
                    text: qsTr("Exécuter")
                    onClicked: root.runAgentTask()
                }
            }
        }

        Loader {
            id: webLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            active: root.loaded
            sourceComponent: terminalViewComponent
        }
    }

    Component {
        id: terminalViewComponent

        WebEngineView {
            id: web
            webChannel: terminalChannel
            backgroundColor: theme.background
            focus: true

            onActiveFocusChanged: {
                if (activeFocus)
                    Window.window.terminalFocused = true
                else if (Window.window.terminalFocused)
                    Window.window.terminalFocused = false
            }

            onLoadingChanged: function (request) {
                if (request.status === WebEngineView.LoadSucceededStatus) {
                    web.forceActiveFocus()
                    root.applyTheme()
                    web.runJavaScript("window.termFocus && window.termFocus()")
                } else if (request.status === WebEngineView.LoadFailedStatus) {
                    Window.window.showToast(qsTr("The terminal page could not load."))
                }
            }

            onRenderProcessTerminated: function (status) {
                Window.window.showToast(qsTr("The terminal renderer stopped."))
            }
        }
    }

    Connections {
        target: terminal
        function onStarted() {
            root.applyTheme()
            if (Qt.application.arguments.indexOf("--terminal-demo") !== -1)
                terminal.input("clear; echo 'Ultimate IAH'; ls -1\n")
        }
    }

    Connections {
        target: theme
        function onChanged() { root.applyTheme() }
    }

    Connections {
        target: backend
        function onWorkspaceChanged() {
            if (!root.loaded || !web)
                return
            web.runJavaScript("window.termReset && window.termReset()")
            terminal.start(backend.workspace)
        }
    }
}
