import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window


Rectangle {
    id: root

    property bool ready: backend.opencodeState === "ready"
    property bool active: true
    readonly property bool canSend: ready && !backend.busy && input.text.trim() !== ""

    signal submit(string text)

    implicitHeight: layout.implicitHeight + 20
    color: theme.raised
    radius: Math.min(12, theme.radius)
    border.width: 1
    border.color: input.activeFocus ? theme.accent : theme.border
    Behavior on border.color { ColorAnimation { duration: 120 } }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: root.radius + 2
        color: "transparent"
        border.width: 1
        border.color: theme.accent
        opacity: input.activeFocus ? 0.35 : 0
        Behavior on opacity { NumberAnimation { duration: 140 } }
    }

    function focusInput() {
        input.forceActiveFocus()
    }

    function setText(value) {
        input.text = value
        focusInput()
    }

    function takeText() {
        const value = input.text
        input.text = ""
        return value
    }

    Shortcut {
        sequence: "Ctrl+M"
        context: Qt.ApplicationShortcut
        enabled: root.active && root.ready && !backend.busy
        onActivated: modelPicker.open()
    }

    Component.onCompleted: {
        // Used by the README/test screenshot: open the picker so the custom
        // menu, not the stock Material popup, is what gets captured.
        if (Qt.application.arguments.indexOf("--picker") !== -1)
            Qt.callLater(modelPicker.open)
    }

    ColumnLayout {
        id: layout
        x: 10
        y: 10
        width: parent.width - 20
        spacing: 6

        Item {
            id: inputFrame
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(148, Math.max(26, input.contentHeight + 6))

            TextEdit {
        renderType: TextEdit.NativeRendering
                id: input
                anchors.fill: parent
                anchors.margins: 2
                color: theme.text
                selectionColor: theme.accent
                selectedTextColor: theme.accentForeground
                font.pixelSize: 13
                wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
                readOnly: !root.ready
                onActiveFocusChanged: Window.window.typing = activeFocus

                Keys.onPressed: function (event) {
                    if (event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter)
                        return
                    if (event.modifiers & Qt.ShiftModifier && !(event.modifiers & Qt.ControlModifier))
                        return
                    event.accepted = true
                    if (!root.canSend)
                        return
                    root.submit(input.text.trim())
                    input.text = ""
                }
            }

            Text {
        renderType: Text.NativeRendering
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 3
                width: parent.width - 6
                visible: input.text.length === 0
                text: !root.ready ? qsTr("OpenCode is not ready") :
                      backend.busy ? qsTr("OpenCode is working…") : qsTr("Ask OpenCode…  Enter to send, Shift+Enter for a new line")
                color: theme.dimText
                font.pixelSize: 13
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            PickerChip {
                id: modelPicker
                enabled: root.ready && !backend.busy
                opacity: enabled ? 1 : 0.5
                title: qsTr("Model")
                options: backend.modelOptions
                currentValue: backend.currentModel
                searchable: true
                emptyText: qsTr("Model")
                dotColor: theme.cyan
                onSelected: function (value) {
                    backend.setConfigOption("model", value)
                    input.forceActiveFocus()
                }
            }

            PickerChip {
                id: modePicker
                enabled: root.ready && !backend.busy
                opacity: enabled ? 1 : 0.5
                title: qsTr("Mode")
                options: backend.modeOptions
                currentValue: backend.currentMode
                searchable: false
                emptyText: qsTr("Mode")
                dotColor: theme.orange
                onSelected: function (value) {
                    backend.setConfigOption("mode", value)
                    input.forceActiveFocus()
                }
            }

            Item { Layout.fillWidth: true }

            Text {
        renderType: Text.NativeRendering
                visible: backend.usage.used !== undefined
                text: {
                    const usage = backend.usage
                    if (usage.used === undefined)
                        return ""
                    const used = usage.used >= 1000 ? (usage.used / 1000).toFixed(1) + "k" : usage.used
                    const size = usage.size ? (usage.size >= 1000 ? Math.round(usage.size / 1000) + "k" : usage.size) : ""
                    return size ? qsTr("%1 / %2 tokens").arg(used).arg(size) : qsTr("%1 tokens").arg(used)
                }
                font.family: backend.monoFont
                font.pixelSize: 10
                color: theme.dimText
            }

            Rectangle {
                width: 30
                height: 30
                radius: Math.min(8, theme.radius)
                color: backend.busy ? theme.red : (root.canSend ? theme.accent : theme.raised)
                border.width: 1
                border.color: backend.busy ? theme.red : (root.canSend ? theme.accent : theme.border)

                ThemedIcon {
                    anchors.centerIn: parent
                    source: backend.busy ? "icons/stop.svg" : "icons/send.svg"
                    width: 13
                    height: 13
                    color: backend.busy ? "#ffffff" : (root.canSend ? theme.accentForeground : theme.dimText)
                    iconOpacity: root.canSend || backend.busy ? 1 : 0.7
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (backend.busy) {
                            backend.cancelPrompt()
                        } else if (root.canSend) {
                            root.submit(input.text.trim())
                            input.text = ""
                        }
                    }
                }
            }
        }
    }
}
