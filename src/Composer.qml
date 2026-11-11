import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window

import "Palette.js" as Pal

Rectangle {
    id: root

    property bool ready: backend.opencodeState === "ready"
    readonly property bool canSend: ready && !backend.busy && input.text.trim() !== ""

    signal submit(string text)

    implicitHeight: layout.implicitHeight + 20
    color: Pal.raised
    radius: Math.min(12, theme.radius)
    border.width: 1
    border.color: input.activeFocus ? theme.accent : Pal.border
    Behavior on border.color { ColorAnimation { duration: 120 } }

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

    function syncModel() {
        const index = modelCombo.indexOfValue(backend.currentModel)
        if (index >= 0 && index !== modelCombo.currentIndex)
            modelCombo.currentIndex = index
    }

    function syncMode() {
        const index = modeCombo.indexOfValue(backend.currentMode)
        if (index >= 0 && index !== modeCombo.currentIndex)
            modeCombo.currentIndex = index
    }

    Component.onCompleted: {
        syncModel()
        syncMode()
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
                id: input
                anchors.fill: parent
                anchors.margins: 2
                color: Pal.text
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
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 3
                width: parent.width - 6
                visible: input.text.length === 0
                text: !root.ready ? qsTr("OpenCode is not ready") :
                      backend.busy ? qsTr("OpenCode is working…") : qsTr("Ask OpenCode…  Enter to send, Shift+Enter for a new line")
                color: Pal.dim
                font.pixelSize: 13
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ComboBox {
                id: modelCombo
                Layout.preferredWidth: Math.min(190, implicitWidth)
                implicitHeight: 24
                model: backend.modelOptions
                textRole: "name"
                valueRole: "value"
                flat: true
                padding: 0
                spacing: 0
                enabled: root.ready && !backend.busy
                font.pixelSize: 10
                background: Rectangle {
                    color: "transparent"
                    radius: Math.min(5, theme.radius)
                }
                indicator: Item { width: 0; height: 0 }
                contentItem: RowLayout {
                    spacing: 3
                    Text {
                        Layout.fillWidth: true
                        text: modelCombo.displayText
                        color: Pal.muted
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                    Image {
                        source: "icons/chevron.svg"
                        sourceSize.width: 9
                        sourceSize.height: 9
                        opacity: 0.4
                    }
                }
                onActivated: backend.setConfigOption("model", currentValue)
            }

            Rectangle {
                width: 1
                height: 14
                color: Pal.border
            }

            ComboBox {
                id: modeCombo
                Layout.preferredWidth: Math.min(110, implicitWidth)
                implicitHeight: 24
                model: backend.modeOptions
                textRole: "name"
                valueRole: "value"
                flat: true
                padding: 0
                spacing: 0
                enabled: root.ready && !backend.busy
                font.pixelSize: 10
                background: Rectangle {
                    color: "transparent"
                    radius: Math.min(5, theme.radius)
                }
                indicator: Item { width: 0; height: 0 }
                contentItem: RowLayout {
                    spacing: 3
                    Text {
                        Layout.fillWidth: true
                        text: modeCombo.displayText
                        color: Pal.muted
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                    Image {
                        source: "icons/chevron.svg"
                        sourceSize.width: 9
                        sourceSize.height: 9
                        opacity: 0.4
                    }
                }
                onActivated: backend.setConfigOption("mode", currentValue)
            }

            Item { Layout.fillWidth: true }

            Text {
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
                color: Pal.dim
            }

            Rectangle {
                width: 30
                height: 30
                radius: Math.min(8, theme.radius)
                color: backend.busy ? "#2A1518" : (root.canSend ? Pal.teal : Pal.raised)
                border.width: 1
                border.color: backend.busy ? "#5A2A2E" : (root.canSend ? Pal.teal : Pal.border)

                Image {
                    anchors.centerIn: parent
                    source: backend.busy ? "icons/stop.svg" : "icons/send.svg"
                    sourceSize.width: 13
                    sourceSize.height: 13
                    opacity: root.canSend || backend.busy ? 1 : 0.3
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

    Connections {
        target: backend
        function onConfigOptionsChanged() {
            root.syncModel()
            root.syncMode()
        }
    }
}
