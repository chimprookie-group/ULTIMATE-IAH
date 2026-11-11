import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window


Rectangle {
    id: root

    property bool ready: backend.opencodeState === "ready"
    property bool active: true
    property var attachments: []
    property var history: []
    property int historyIndex: -1
    readonly property var modelChoices: {
        const list = ecosystem.models.slice()
        const current = backend.currentModel
        if (current !== "") {
            let found = false
            for (let i = 0; i < list.length; ++i) {
                if (list[i].value === current) {
                    found = true
                    break
                }
            }
            if (!found) {
                const slash = current.indexOf("/")
                list.unshift({
                    "value": current,
                    "name": slash > 0 ? current.slice(slash + 1) : current,
                    "provider": slash > 0 ? current.slice(0, slash) : "",
                    "free": current.indexOf("free") >= 0
                })
            }
        }
        return list.length > 0 ? list : backend.modelOptions
    }
    readonly property bool canSend: ready && !backend.busy
                                   && (input.text.trim() !== "" || attachments.length > 0)

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

    function openModelPicker() {
        if (modelPicker.enabled)
            modelPicker.open()
    }

    function setText(value) {
        input.text = value
        focusInput()
    }

    function attachFile(path, content) {
        if (path === "")
            return
        for (let i = 0; i < attachments.length; ++i) {
            if (attachments[i].path === path) {
                focusInput()
                return
            }
        }
        if (attachments.length >= 5) {
            Window.window.showToast(qsTr("Five files maximum per message."))
            return
        }
        const next = attachments.slice()
        next.push({
            "path": path,
            "name": String(path).split("/").pop(),
            "content": String(content).substring(0, 14000)
        })
        attachments = next
        focusInput()
    }

    function removeAttachment(index) {
        const next = attachments.slice()
        next.splice(index, 1)
        attachments = next
    }

    function composeDraft() {
        let context = ""
        for (let i = 0; i < attachments.length; ++i) {
            const file = attachments[i]
            context += '<file path="' + file.path + '">\n' + file.content + '\n</file>\n\n'
        }
        const typed = input.text.trim()
        if (typed !== "")
            return context + typed
        return context + (attachments.length > 0 ? qsTr("Analyze these files.") : "")
    }

    function submitDraft() {
        if (!canSend)
            return
        const text = composeDraft()
        if (text.trim() === "")
            return
        const typed = input.text.trim()
        if (typed !== "" && (history.length === 0 || history[history.length - 1] !== typed)) {
            const next = history.slice()
            next.push(typed)
            while (next.length > 50)
                next.shift()
            history = next
        }
        historyIndex = -1
        root.submit(text)
        input.text = ""
        attachments = []
    }

    function historyBack() {
        if (history.length === 0)
            return
        if (historyIndex < 0)
            historyIndex = history.length - 1
        else if (historyIndex > 0)
            historyIndex--
        input.text = history[historyIndex]
        input.cursorPosition = input.text.length
    }

    function historyForward() {
        if (historyIndex < 0)
            return
        if (historyIndex < history.length - 1) {
            historyIndex++
            input.text = history[historyIndex]
            input.cursorPosition = input.text.length
        } else {
            historyIndex = -1
            input.text = ""
        }
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
        Qt.callLater(function () { ecosystem.refreshModels() })
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
                color: theme.text
                selectionColor: theme.accent
                selectedTextColor: theme.accentForeground
                font.pixelSize: 13
                wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
                readOnly: !root.ready
                onActiveFocusChanged: Window.window.typing = activeFocus

                Keys.onPressed: function (event) {
                    if (event.key === Qt.Key_Up && input.text.indexOf('\n') < 0 && !(event.modifiers & Qt.ShiftModifier)) {
                        root.historyBack()
                        event.accepted = true
                        return
                    }
                    if (event.key === Qt.Key_Down && input.text.indexOf('\n') < 0 && !(event.modifiers & Qt.ShiftModifier)) {
                        root.historyForward()
                        event.accepted = true
                        return
                    }
                    if (event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter)
                        return
                    if (event.modifiers & Qt.ShiftModifier && !(event.modifiers & Qt.ControlModifier))
                        return
                    event.accepted = true
                    if (!root.canSend)
                        return
                    root.submitDraft()
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
                color: theme.dimText
                font.pixelSize: 13
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: 4
            visible: root.attachments.length > 0

            Repeater {
                model: root.attachments

                Rectangle {
                    width: Math.min(240, Math.max(110, modelData.name.length * 6 + 38))
                    height: 22
                    radius: 11
                    color: theme.chip
                    border.width: 1
                    border.color: theme.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 5
                        spacing: 4

                        ThemedIcon {
                            source: "icons/file.svg"
                            width: 9
                            height: 9
                            color: theme.cyan
                        }
                        Text {
                            id: attachmentLabel
                            Layout.fillWidth: true
                            text: modelData.name
                            color: theme.mutedText
                            font.pixelSize: 8
                            elide: Text.ElideMiddle
                        }
                        Text {
                            text: "×"
                            color: theme.dimText
                            font.pixelSize: 12
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.removeAttachment(index)
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            PickerChip {
                id: profilePicker
                visible: backend.profiles.length > 0
                enabled: root.ready && !backend.busy
                opacity: enabled ? 1 : 0.5
                title: qsTr("Profile")
                options: backend.profiles
                currentValue: backend.activeProfileId
                searchable: true
                emptyText: qsTr("Profile")
                dotColor: theme.magenta
                onSelected: function (value) {
                    backend.applyProfile(value)
                    input.forceActiveFocus()
                }
            }

            PickerChip {
                id: modelPicker
                enabled: root.ready && !backend.busy
                opacity: enabled ? 1 : 0.5
                title: qsTr("Model")
                options: root.modelChoices
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
                            root.submitDraft()
                        }
                    }
                }
            }
        }
    }
}
