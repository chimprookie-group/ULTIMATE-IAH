import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    signal agentTaskRequested(string prompt)

    property string selectedPath: ""
    property string selectedStatus: ""
    property int selectedSize: 0
    property bool showingDiff: false
    property string shownText: ""
    readonly property bool markdownPreview: !showingDiff
        && /\.(md|markdown|mdx)$/i.test(selectedPath)
    readonly property string previewText: markdownPreview
        ? shownText.replace(/!\[[^\]]*\]\([^)]*\)/g, "") : shownText

    readonly property int leftWidth: Math.max(230, Math.min(320, width * 0.3))
    readonly property var selectedFile: {
        for (let i = 0; i < workspaceModel.files.length; ++i) {
            if (workspaceModel.files[i].path === selectedPath)
                return workspaceModel.files[i]
        }
        return null
    }

    function refresh() {
        workspaceModel.refresh(backend.workspace)
        selectedPath = ""
        selectedStatus = ""
        selectedSize = 0
        shownText = ""
        showingDiff = false
        if (workspaceModel.files.length > 0) {
            let candidate = workspaceModel.files[0]
            for (let i = 0; i < workspaceModel.files.length; ++i) {
                if (workspaceModel.files[i].name === "README.md") {
                    candidate = workspaceModel.files[i]
                    break
                }
            }
            selectFile(candidate)
        }
    }

    function selectFile(file) {
        selectedPath = file.path
        selectedStatus = file.status !== undefined ? file.status : ""
        selectedSize = file.size !== undefined ? file.size : 0
        showingDiff = false
        shownText = workspaceModel.preview(file.path)
    }

    function toggleDiff() {
        if (selectedPath === "")
            return
        showingDiff = !showingDiff
        shownText = showingDiff ? workspaceModel.diff(selectedPath)
                                : workspaceModel.preview(selectedPath)
    }

    function sizeLabel(bytes) {
        if (bytes >= 1048576)
            return (bytes / 1048576).toFixed(1) + " MB"
        if (bytes >= 1024)
            return Math.round(bytes / 1024) + " kB"
        return bytes + " B"
    }

    function statusLabel(status) {
        if (status === "untracked")
            return qsTr("untracked")
        if (status === "added")
            return qsTr("added")
        if (status === "deleted")
            return qsTr("deleted")
        if (status === "renamed")
            return qsTr("renamed")
        if (status === "modified" || status === "changed")
            return qsTr("modified")
        return qsTr("tracked")
    }

    function statusColor(status) {
        if (status === "untracked")
            return theme.yellow
        if (status === "deleted")
            return theme.red
        if (status === "modified" || status === "changed")
            return theme.orange
        if (status === "added" || status === "renamed")
            return theme.green
        return theme.dimText
    }

    function askAboutFile() {
        if (selectedPath === "")
            return
        if (showingDiff) {
            agentTaskRequested(qsTr("Explain the changes in the diff of %1 in this project. Describe the intent, the risks, and what to verify.").arg(selectedPath))
        } else {
            agentTaskRequested(qsTr("Explain the file %1 in this project. Read it, describe its role, its dependencies, and anything worth watching.").arg(selectedPath))
        }
    }

    function copyPath() {
        if (selectedPath === "")
            return
        workspaceModel.copyToClipboard(selectedPath)
        Window.window.showToast(qsTr("Path copied."))
    }

    function openFile() {
        if (selectedPath === "")
            return
        Qt.openUrlExternally("file://" + workspaceModel.absolutePath(selectedPath))
    }

    Component.onCompleted: if (visible) refresh()
    onVisibleChanged: if (visible) refresh()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: theme.background

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: design.space5
                anchors.rightMargin: design.space4
                spacing: design.space2

                Text {
                    text: qsTr("Fichiers")
                    color: theme.text
                    font.pixelSize: design.textBody
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: backend.workspace + " · " + qsTr("%n file(s)", "", workspaceModel.count)
                    color: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: design.textCaption
                    elide: Text.ElideMiddle
                }
                Button {
                    text: qsTr("Refresh")
                    flat: true
                    onClicked: root.refresh()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.preferredWidth: root.leftWidth
                Layout.fillHeight: true
                color: theme.background

                Rectangle {
                    anchors.right: parent.right
                    width: 1
                    height: parent.height
                    color: theme.border
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    SettingsField {
                        id: filterField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Filter files…")
                        onTextChanged: workspaceModel.setFilter(text)
                    }

                    ListView {
                        id: fileList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 1
                        model: workspaceModel.files
                        boundsBehavior: Flickable.StopAtBounds

                        ScrollBar.vertical: AppScrollBar {}

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 36
                            radius: Math.min(5, theme.radius)
                            color: root.selectedPath === modelData.path
                                   ? theme.chip
                                   : (fileMouse.containsMouse ? theme.raised : "transparent")

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 7
                                anchors.rightMargin: 7
                                spacing: 7

                                StatusDot {
                                    color: root.statusColor(modelData.status)
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        color: root.selectedPath === modelData.path ? theme.text : theme.mutedText
                                        font.pixelSize: 10
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        visible: modelData.dir !== ""
                                        text: modelData.dir
                                        color: theme.dimText
                                        font.family: backend.monoFont
                                        font.pixelSize: design.textMicro
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            MouseArea {
                                id: fileMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.selectFile(modelData)
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: workspaceModel.count === 0
                            text: qsTr("No file found.")
                            color: theme.dimText
                            font.pixelSize: 10
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: theme.surface

                Text {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 60, 420)
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    visible: root.selectedPath === ""
                    text: qsTr("Select a file to preview it, or open its Git diff.")
                    color: theme.dimText
                    font.pixelSize: 11
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 9
                    visible: root.selectedPath !== ""

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 7

                        Text {
                            Layout.fillWidth: true
                            text: root.selectedPath
                            color: theme.text
                            font.family: backend.monoFont
                            font.pixelSize: 10
                            elide: Text.ElideMiddle
                        }
                        Text {
                            text: root.statusLabel(root.selectedStatus)
                            color: root.statusColor(root.selectedStatus)
                            font.pixelSize: design.textMicro
                        }
                        Text {
                            text: root.sizeLabel(root.selectedSize)
                            color: theme.dimText
                            font.pixelSize: design.textMicro
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 5

                        Rectangle {
                            width: previewLabel.implicitWidth + 18
                            height: 25
                            radius: 12
                            color: !root.showingDiff ? theme.accent : theme.chip
                            border.width: 1
                            border.color: !root.showingDiff ? theme.accent : theme.border
                            Text {
                                id: previewLabel
                                anchors.centerIn: parent
                                text: qsTr("Preview")
                                color: !root.showingDiff ? theme.accentForeground : theme.text
                                font.pixelSize: design.textCaption
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.showingDiff = false
                                    root.shownText = workspaceModel.preview(root.selectedPath)
                                }
                            }
                        }
                        Rectangle {
                            width: diffLabel.implicitWidth + 18
                            height: 25
                            radius: 12
                            color: root.showingDiff ? theme.accent : theme.chip
                            border.width: 1
                            border.color: root.showingDiff ? theme.accent : theme.border
                            Text {
                                id: diffLabel
                                anchors.centerIn: parent
                                text: qsTr("Diff")
                                color: root.showingDiff ? theme.accentForeground : theme.text
                                font.pixelSize: design.textCaption
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.toggleDiff()
                            }
                        }
                        Button {
                            text: qsTr("Confier au harness")
                            flat: true
                            font.pixelSize: design.textCaption
                            onClicked: root.askAboutFile()
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: qsTr("Copy path")
                            flat: true
                            font.pixelSize: design.textCaption
                            onClicked: root.copyPath()
                        }
                        Button {
                            text: qsTr("Open")
                            flat: true
                            font.pixelSize: design.textCaption
                            onClicked: root.openFile()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: Math.min(6, theme.radius)
                        color: theme.background
                        border.width: 1
                        border.color: theme.border
                        clip: true

                        Flickable {
                            id: previewFlick
                            anchors.fill: parent
                            anchors.margins: 10
                            clip: true
                            contentWidth: root.markdownPreview ? codeText.width : codeText.implicitWidth
                            contentHeight: codeText.implicitHeight
                            boundsBehavior: Flickable.StopAtBounds

                            ScrollBar.vertical: AppScrollBar {}
                            ScrollBar.horizontal: AppScrollBar {
                                policy: root.markdownPreview ? ScrollBar.AlwaysOff : ScrollBar.AsNeeded
                            }

                            Text {
                                id: codeText
                                width: root.markdownPreview ? Math.min(previewFlick.width, 760) : implicitWidth
                                text: root.previewText
                                color: theme.text
                                font.family: root.markdownPreview ? backend.uiFont : backend.monoFont
                                font.pixelSize: root.markdownPreview ? 12 : 10
                                lineHeight: root.markdownPreview ? 1.45 : 1.35
                                textFormat: root.markdownPreview ? Text.MarkdownText : Text.PlainText
                                wrapMode: root.markdownPreview ? Text.Wrap : Text.NoWrap
                            }
                        }
                    }
                }
            }
        }
    }
}
