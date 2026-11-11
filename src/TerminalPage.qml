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

    function arm() {
        if (loaded)
            return
        loaded = true
        web.url = "qrc:/terminal/index.html?cwd=" + encodeURIComponent(backend.workspace)
    }

    function applyTheme() {
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
        web.runJavaScript("window.termClear && window.termClear()")
    }

    function restart() {
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
            Layout.preferredHeight: 34
            color: theme.background

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 8
                spacing: 8

                StatusDot {
                    color: terminal.running ? theme.green : theme.dimText
                }
                Text {
                    text: qsTr("Terminal")
                    color: theme.text
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: terminal.shell.substring(terminal.shell.lastIndexOf("/") + 1) + " · " + backend.workspace
                    color: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: 9
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

        WebEngineView {
            id: web
            Layout.fillWidth: true
            Layout.fillHeight: true
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
                terminal.input("clear; echo 'Crasy Harness'; ls -1\n")
        }
    }

    Connections {
        target: theme
        function onChanged() { root.applyTheme() }
    }

    Connections {
        target: backend
        function onWorkspaceChanged() {
            if (!root.loaded)
                return
            web.runJavaScript("window.termReset && window.termReset()")
            terminal.start(backend.workspace)
        }
    }
}
