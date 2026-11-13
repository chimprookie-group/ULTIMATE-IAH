import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: root

    property bool compact: false
    property bool closeButton: false
    property bool expandButton: false
    signal closeRequested()
    signal expandRequested()

    readonly property bool wide: !compact && width >= 980
    readonly property int outerMargin: compact ? design.space3 : design.space6
    readonly property bool empty: ultimateAgent.messages.length === 0
    readonly property bool canSend: (composerField.text.trim() !== "" || ultimateAgent.pendingAttachments.length > 0)
                                    && !ultimateAgent.busy
    property string imagePreview: ""
    property string renameId: ""
    property string renameValue: ""
    property string deleteId: ""
    property string deleteValue: ""
    property int commandIndex: 0
    property int mentionIndex: 0
    property var workspaceFiles: []
    property var expandedGroups: ({})

    function groupOk(items) {
        for (var i = 0; i < items.length; ++i) {
            if (!items[i].ok) return false
        }
        return true
    }

    function groupSummary(items) {
        var names = []
        for (var i = 0; i < items.length; ++i) {
            if (names.indexOf(items[i].tool) < 0) names.push(items[i].tool)
        }
        return names.join(", ")
    }

    function toolRun(start) {
        var result = []
        var messages = ultimateAgent.messages
        for (var i = start; i < messages.length && messages[i].role === "tool"; ++i) {
            var item = {}
            for (var field in messages[i]) item[field] = messages[i][field]
            item.index = i
            result.push(item)
        }
        return result
    }

    function toggleGroup(key) {
        var copy = {}
        for (var name in expandedGroups) copy[name] = expandedGroups[name]
        copy[key] = !copy[key]
        expandedGroups = copy
    }



    readonly property var commands: [
        { id: "new", label: "/new", detail: qsTr("Nouvelle discussion") },
        { id: "clear", label: "/clear", detail: qsTr("Vider la conversation") },
        { id: "compact", label: "/compact", detail: qsTr("Compacter le contexte") },
        { id: "export", label: "/export", detail: qsTr("Copier le transcript Markdown") },
        { id: "history", label: "/history", detail: qsTr("Ouvrir les discussions") },
        { id: "model", label: "/model", detail: qsTr("Choisir le provider et le modèle") },
        { id: "permission", label: "/permission", detail: qsTr("Basculer lecture seule / accès complet") },
        { id: "effort", label: "/effort", detail: qsTr("Changer l'effort de raisonnement") },
        { id: "skills", label: "/skills", detail: qsTr("Choisir les compétences") },
        { id: "stop", label: "/stop", detail: qsTr("Arrêter l'agent") },
        { id: "help", label: "/help", detail: qsTr("Afficher les commandes") }
    ]
    readonly property bool commandMode: composerField.text.startsWith("/")
                                          && composerField.text.indexOf(" ") < 0
    readonly property var mentionMatch: composerField.text.match(/(?:^|\s)@([^\s@]*)$/)
    readonly property bool mentionMode: mentionMatch !== null && !root.commandMode
    readonly property string mentionQuery: mentionMatch ? mentionMatch[1].toLowerCase() : ""
    readonly property var filteredMentions: {
        if (!root.mentionMode)
            return []
        var result = []
        for (var i = 0; i < root.workspaceFiles.length && result.length < 20; ++i) {
            const path = root.workspaceFiles[i]
            if (root.mentionQuery === "" || path.toLowerCase().indexOf(root.mentionQuery) >= 0)
                result.push(path)
        }
        return result
    }

    function refreshWorkspaceFiles() {
        workspaceFiles = ultimateAgent.workspaceFiles(500)
    }

    function selectMention(path) {
        composerField.text = composerField.text.replace(/@([^\s@]*)$/, "@" + path + " ")
        composerField.forceActiveFocus()
    }

    onMentionModeChanged: if (mentionMode && workspaceFiles.length === 0) refreshWorkspaceFiles()
    readonly property var filteredCommands: {
        if (!root.commandMode)
            return []
        const query = composerField.text.substring(1).toLowerCase()
        var result = []
        for (var i = 0; i < root.commands.length; ++i) {
            const command = root.commands[i]
            if (command.id.indexOf(query) === 0 || command.detail.toLowerCase().indexOf(query) >= 0)
                result.push(command)
        }
        return result
    }

    function executeCommand(command) {
        if (command === undefined)
            return
        composerField.text = ""
        if (command.id === "new") ultimateAgent.newConversation()
        else if (command.id === "clear") ultimateAgent.clearConversation()
        else if (command.id === "compact") ultimateAgent.compactConversation()
        else if (command.id === "export") {
            ultimateAgent.copyText(ultimateAgent.conversationMarkdown(ultimateAgent.conversationId))
            Window.window.showToast(qsTr("Transcript copié."))
        } else if (command.id === "history") root.openHistory()
        else if (command.id === "model") modelDialog.open()
        else if (command.id === "permission")
            ultimateAgent.permissionMode = ultimateAgent.permissionMode === "read-only" ? "workspace-write" : "read-only"
        else if (command.id === "effort") {
            const efforts = ["", "low", "medium", "high"]
            const index = efforts.indexOf(ultimateAgent.reasoningEffort)
            ultimateAgent.reasoningEffort = efforts[(index + 1) % efforts.length]
        } else if (command.id === "skills") optionsDialog.open()
        else if (command.id === "stop") ultimateAgent.cancel()
        else if (command.id === "help") helpDialog.open()
        composerField.forceActiveFocus()
    }

    function send(text) {
        const value = String(text).trim()
        if ((value === "" && ultimateAgent.pendingAttachments.length === 0) || ultimateAgent.busy)
            return
        ultimateAgent.sendMessage(value)
        composerField.text = ""
    }

    function quick(text) {
        root.send(text)
    }

    property bool followOutput: true
    property bool showScrollButton: false
    property real lastContentY: 0

    function nearBottom() {
        if (messageList.count === 0)
            return true
        return messageList.contentHeight - (messageList.contentY + messageList.height) < 80
    }

    function scrollToBottom(force) {
        if (!force && !root.followOutput)
            return
        Qt.callLater(function () {
            if (messageList.count > 0)
                messageList.positionViewAtEnd()
        })
    }

    function scrollToBottomAnimated() {
        root.followOutput = true
        const target = Math.max(0, messageList.contentHeight - messageList.height)
        if (target <= 0 || Math.abs(messageList.contentY - target) < 2)
            return
        scrollAnimation.stop()
        scrollAnimation.from = messageList.contentY
        scrollAnimation.to = target
        scrollAnimation.start()
    }

    function openRename(id, title) {
        root.renameId = id
        root.renameValue = title
        renameField.text = title
        renameDialog.open()
    }

    function openDelete(id, title) {
        root.deleteId = id
        root.deleteValue = title
        deleteDialog.open()
    }

    function openHistory() {
        if (root.wide)
            return
        historyDialog.open()
    }

    function formatTokens(value) {
        if (value >= 1000000) return (value / 1000000).toFixed(1) + " M"
        if (value >= 1000) return (value / 1000).toFixed(1) + " k"
        return String(value)
    }

    function reasoningLabel(seconds) {
        if (seconds >= 60) return qsTr("Réflexion approfondie depuis %1m %2s").arg(Math.floor(seconds / 60)).arg(seconds % 60)
        if (seconds > 0) return qsTr("Réflexion approfondie · %1 s").arg(seconds)
        return qsTr("Réflexion approfondie")
    }

    function todoCount(status) {
        var count = 0
        for (var i = 0; i < ultimateAgent.todos.length; ++i) {
            if (ultimateAgent.todos[i].status === status) ++count
        }
        return count
    }

    function todoSummary() {
        const running = todoCount("in_progress")
        const waiting = todoCount("pending")
        if (running > 0) return qsTr("%1 en cours · %2 en attente").arg(running).arg(waiting)
        return qsTr("%1 en attente").arg(waiting)
    }

    function effortIndex() {
        if (ultimateAgent.reasoningEffort === "low") return 1
        if (ultimateAgent.reasoningEffort === "medium") return 2
        if (ultimateAgent.reasoningEffort === "high") return 3
        return 0
    }

    function modeLabel(mode) {
        if (mode === "diagnostic") return qsTr("Diagnostic")
        if (mode === "build") return qsTr("Construction")
        if (mode === "review") return qsTr("Revue")
        return qsTr("Assistant")
    }

    Component.onCompleted: if (visible) { ultimateAgent.refreshEndpoint(); skills.refresh(); root.refreshWorkspaceFiles() }
    onVisibleChanged: if (visible) { ultimateAgent.refreshEndpoint(); skills.refresh(); root.refreshWorkspaceFiles() }

    Connections {
        target: ultimateAgent
        function onMessagesChanged() {
            root.scrollToBottom()
            if (root.empty)
                Qt.callLater(function () { welcomeScroll.contentY = 0 })
        }
        function onBusyChanged() { if (!ultimateAgent.busy) root.scrollToBottom() }
        function onConversationIdChanged() {
            root.followOutput = true
            root.showScrollButton = false
            root.scrollToBottom(true)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── header ──────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.compact ? 58 : 64
            color: theme.surface
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: theme.border }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.compact ? design.space3 : design.space5
                anchors.rightMargin: root.compact ? design.space2 : design.space4
                spacing: design.space2
                BrandMark { Layout.alignment: Qt.AlignVCenter; width: root.compact ? 24 : 28; height: width; size: width; accent: false }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1
                    RowLayout {
                        spacing: design.space2
                        Text {
                            text: qsTr("Ultimate Agent")
                            color: theme.brightText
                            font.pixelSize: root.compact ? design.textLabel : design.textBody
                            font.weight: Font.DemiBold
                        }
                        StatusDot {
                            size: 5
                            color: ultimateAgent.busy ? theme.yellow
                                 : (ultimateAgent.error !== "" ? theme.red
                                 : (ultimateAgent.modelLabel === "Non configuré" ? theme.orange : theme.green))
                        }
                        Rectangle {
                            implicitWidth: modeChip.implicitWidth + 12
                            implicitHeight: 18
                            radius: 9
                            color: theme.chip
                            Text {
                                id: modeChip
                                anchors.centerIn: parent
                                text: root.modeLabel(ultimateAgent.mode)
                                color: theme.mutedText
                                font.family: backend.monoFont
                                font.pixelSize: 8
                            }
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: ultimateAgent.busy ? ultimateAgent.status
                                                 : ultimateAgent.conversationTitle + "  ·  " + ultimateAgent.modelLabel
                        color: ultimateAgent.error !== "" ? theme.red : theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: design.textMicro
                        elide: Text.ElideRight
                    }
                }
                IconButton { visible: !root.wide; icon: "icons/history.svg"; hint: qsTr("Discussions"); onClicked: root.openHistory() }
                IconButton { icon: "icons/plus.svg"; hint: qsTr("Nouvelle conversation"); onClicked: ultimateAgent.newConversation() }
                IconButton { icon: "icons/sliders.svg"; hint: qsTr("Options, compétences et mode"); onClicked: optionsDialog.open() }
                IconButton { visible: root.expandButton; icon: "icons/external.svg"; hint: qsTr("Ouvrir en grand"); onClicked: root.expandRequested() }
                IconButton { visible: root.closeButton; icon: "icons/close.svg"; hint: qsTr("Fermer"); onClicked: root.closeRequested() }
            }
        }

        // ── error ───────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: root.outerMargin
            Layout.rightMargin: root.outerMargin
            Layout.topMargin: ultimateAgent.error !== "" ? design.space2 : 0
            Layout.preferredHeight: ultimateAgent.error !== "" ? errorText.implicitHeight + design.space3 : 0
            visible: ultimateAgent.error !== ""
            radius: design.radiusSm
            color: Qt.rgba(theme.red.r, theme.red.g, theme.red.b, 0.09)
            border.width: 1
            border.color: Qt.rgba(theme.red.r, theme.red.g, theme.red.b, 0.55)
            Text {
                id: errorText
                anchors.fill: parent
                anchors.margins: design.space2
                text: ultimateAgent.error
                color: theme.red
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }
        }

        // ── discussions + conversation ──────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: 1200
            Layout.leftMargin: root.outerMargin
            Layout.rightMargin: root.outerMargin
            Layout.topMargin: design.space3
            Layout.bottomMargin: design.space3
            spacing: design.space3

            Rectangle {
                visible: root.wide
                Layout.preferredWidth: 254
                Layout.fillHeight: true
                radius: design.radiusLg
                color: theme.surface
                border.width: 1
                border.color: theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: design.space2
                    spacing: design.space2
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: design.space1
                        Text { text: qsTr("Discussions"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; font.letterSpacing: 0.8 }
                        Item { Layout.fillWidth: true }
                        Text { text: ultimateAgent.conversations.length; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        IconButton { implicitWidth: 22; implicitHeight: 22; icon: "icons/plus.svg"; hint: qsTr("Nouvelle conversation"); onClicked: ultimateAgent.newConversation() }
                    }
                    SettingsField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Rechercher…")
                        text: ultimateAgent.conversationQuery
                        onTextChanged: ultimateAgent.conversationQuery = text
                    }
                    AgentConversationList {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        items: ultimateAgent.conversations
                        currentId: ultimateAgent.conversationId
                        onSelected: function (id) { ultimateAgent.selectConversation(id) }
                        onRenameRequested: function (id, title) { root.openRename(id, title) }
                        onDeleteRequested: function (id, title) { root.openDelete(id, title) }
                        onPinRequested: function (id) { ultimateAgent.pinConversation(id) }
                    }
                    GhostButton {
                        Layout.fillWidth: true
                        text: qsTr("Nouvelle discussion")
                        flat: true
                        font.pixelSize: design.textCaption
                        onClicked: ultimateAgent.newConversation()
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: messageList
                        anchors.fill: parent
                        visible: !root.empty
                        anchors.leftMargin: design.space2
                        anchors.rightMargin: design.space3
                        anchors.topMargin: design.space3
                        anchors.bottomMargin: design.space2
                        clip: true
                        spacing: design.space3
                        model: ultimateAgent.messages
                        reuseItems: true
                        cacheBuffer: 600
                        boundsBehavior: Flickable.StopAtBounds
                        onContentHeightChanged: if (root.followOutput) root.scrollToBottom()
                        onMovementEnded: root.followOutput = root.nearBottom()
                        onFlickEnded: root.followOutput = root.nearBottom()
                        onContentYChanged: {
                            if (root.nearBottom())
                                root.followOutput = true
                            else if (contentY < root.lastContentY - 2 && !scrollAnimation.running)
                                root.followOutput = false
                            root.showScrollButton = !root.nearBottom() && count > 0
                            root.lastContentY = contentY
                        }
                        ScrollBar.vertical: AppScrollBar { policy: ultimateAgent.messages.length > 0 ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }

                        delegate: Item {
                            id: delegateItem
                            readonly property bool hiddenTool: modelData.role === "tool"
                                && index > 0 && ultimateAgent.messages[index - 1].role === "tool"
                            width: ListView.view.width - design.space3
                            height: hiddenTool ? 0 : Math.max(userBlock.visible ? userBlock.height : 0,
                                             Math.max(assistantBlock.visible ? assistantBlock.height : 0,
                                                      Math.max(reasoningBlock.visible ? reasoningBlock.height : 0,
                                                               stepsBlock.visible ? stepsBlock.height : 0)))

                            Column {
                                id: userBlock
                                visible: modelData.role === "user"
                                anchors.right: parent.right
                                width: Math.min(parent.width * 0.8, 620)
                                spacing: design.space1

                                Repeater {
                                    model: modelData.images !== undefined ? modelData.images : []
                                    Rectangle {
                                        anchors.right: parent.right
                                        width: Math.min(userBlock.width, 280)
                                        height: 180
                                        radius: design.radiusMd
                                        color: theme.chip
                                        border.width: 1
                                        border.color: theme.border
                                        clip: true
                                        Image {
                                            anchors.fill: parent
                                            anchors.margins: 2
                                            source: "file://" + modelData.path
                                            sourceSize.width: 560
                                            asynchronous: true
                                            fillMode: Image.PreserveAspectFit
                                            cache: false
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.imagePreview = "file://" + modelData.path
                                        }
                                    }
                                }

                                Rectangle {
                                    id: userBubble
                                    visible: modelData.text !== ""
                                    anchors.right: parent.right
                                    width: Math.min(userBlock.width, 520)
                                    height: userText.implicitHeight + design.space3
                                    radius: design.radiusMd
                                    color: Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.18)
                                    border.width: 1
                                    border.color: Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.5)
                                    Text {
                                        id: userText
                                        width: parent.width - design.space4
                                        anchors.centerIn: parent
                                        text: modelData.role === "user" ? modelData.text : ""
                                        color: theme.text
                                        font.pixelSize: design.textBody
                                        wrapMode: Text.Wrap
                                        textFormat: Text.PlainText
                                    }
                                }
                            }

                            ColumnLayout {
                                id: assistantBlock
                                visible: modelData.role === "assistant"
                                anchors.left: parent.left
                                width: parent.width
                                spacing: design.space1
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: design.space2
                                    BrandMark { width: 18; height: 18; size: 18; accent: false }
                                    Text { text: qsTr("Ultimate Agent"); color: theme.accent; font.pixelSize: design.textCaption; font.weight: Font.DemiBold }
                                    Text { Layout.fillWidth: true; text: modelData.time; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                                    IconButton { implicitWidth: 20; implicitHeight: 20; icon: "icons/copy.svg"; hint: qsTr("Copier"); onClicked: ultimateAgent.copyText(modelData.text) }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.role === "assistant" ? modelData.text : ""
                                    color: theme.text
                                    font.pixelSize: design.textBody
                                    lineHeight: 1.38
                                    wrapMode: Text.Wrap
                                    textFormat: Text.MarkdownText
                                    onLinkActivated: function (link) { Qt.openUrlExternally(link) }
                                }
                            }

                            Rectangle {
                                id: stepsBlock
                                readonly property var items: root.toolRun(index)
                                visible: modelData.role === "tool" && !delegateItem.hiddenTool
                                width: parent.width
                                implicitHeight: stepsColumn.implicitHeight + design.space3
                                height: implicitHeight
                                radius: design.radiusMd
                                color: theme.chip
                                border.width: 1
                                border.color: theme.border
                                property bool expanded: root.expandedGroups[index] === true
                                ColumnLayout {
                                    id: stepsColumn
                                    anchors.fill: parent
                                    anchors.margins: design.space2
                                    spacing: 2
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: design.space2
                                        StatusDot { size: 5; color: root.groupOk(stepsBlock.items) ? theme.green : theme.red }
                                        Text {
                                            text: qsTr("%1 étape(s)").arg(stepsBlock.items.length)
                                            color: theme.text
                                            font.family: backend.monoFont
                                            font.pixelSize: design.textCaption
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: root.groupSummary(stepsBlock.items)
                                            color: theme.dimText
                                            font.pixelSize: design.textMicro
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: stepsBlock.expanded ? qsTr("Masquer") : qsTr("Voir")
                                            color: theme.accent
                                            font.pixelSize: design.textMicro
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: root.toggleGroup(index)
                                            }
                                        }
                                    }
                                    Repeater {
                                        model: stepsBlock.expanded ? stepsBlock.items : []
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 1
                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: design.space2
                                                StatusDot { size: 4; color: modelData.ok ? theme.green : theme.red }
                                                Text { text: modelData.tool || ""; color: theme.text; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                                                Text { Layout.fillWidth: true; text: modelData.detail || ""; color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                                                Text {
                                                    text: modelData.expanded ? qsTr("Masquer") : qsTr("Détails")
                                                    color: theme.accent
                                                    font.pixelSize: design.textMicro
                                                    MouseArea {
                                                        anchors.fill: parent
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: ultimateAgent.toggleToolDetails(modelData.index)
                                                    }
                                                }
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                visible: !!modelData.expanded
                                                text: modelData.payload || ""
                                                color: theme.dimText
                                                font.family: backend.monoFont
                                                font.pixelSize: design.textMicro
                                                wrapMode: Text.WrapAnywhere
                                                maximumLineCount: 12
                                                elide: Text.ElideRight
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                id: reasoningBlock
                                visible: modelData.role === "reasoning"
                                width: parent.width
                                implicitHeight: reasoningColumn.implicitHeight + design.space3
                                height: implicitHeight
                                radius: design.radiusMd
                                color: "transparent"
                                border.width: 1
                                border.color: theme.border
                                ColumnLayout {
                                    id: reasoningColumn
                                    anchors.fill: parent
                                    anchors.margins: design.space2
                                    spacing: 3
                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: design.space2
                                        ThemedIcon { source: "icons/spark.svg"; width: 13; height: 13; color: theme.mutedText }
                                        Text {
                                            Layout.fillWidth: true
                                            text: root.reasoningLabel(modelData.seconds || 0)
                                            color: theme.mutedText
                                            font.family: backend.monoFont
                                            font.pixelSize: design.textMicro
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: modelData.expanded ? qsTr("Masquer") : qsTr("Voir")
                                            color: theme.accent
                                            font.pixelSize: design.textMicro
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: ultimateAgent.toggleToolDetails(model.index)
                                            }
                                        }
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        visible: !!modelData.expanded
                                        text: modelData.text || ""
                                        color: theme.dimText
                                        font.pixelSize: design.textCaption
                                        wrapMode: Text.Wrap
                                        maximumLineCount: 40
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                        }
                    }

                    ScrollView {
                        id: welcomeScroll
                        anchors.fill: parent
                        visible: root.empty
                        clip: true
                        Component.onCompleted: contentItem.contentY = 0
                        onVisibleChanged: if (visible) contentItem.contentY = 0
                        contentWidth: availableWidth
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                        ScrollBar.vertical: AppScrollBar {}

                        Column {
                            width: Math.min(root.compact ? parent.width - design.space6 : 760, parent.width - (root.compact ? design.space6 : design.space7))
                            x: Math.max(0, (parent.width - width) / 2)
                            topPadding: root.compact ? design.space4 : design.space6
                            bottomPadding: design.space5
                            spacing: design.space5

                            RowLayout {
                                width: parent.width
                                spacing: design.space4
                                BrandMark { Layout.alignment: Qt.AlignTop; width: root.compact ? 46 : 64; height: width; size: width; accent: false }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: design.space2
                                    Text {
                                        Layout.fillWidth: true
                                        text: qsTr("Que veux-tu faire ?")
                                        color: theme.brightText
                                        font.family: "iA Writer Duospace"
                                        font.pixelSize: root.compact ? 22 : 28
                                        font.weight: Font.Bold
                                        wrapMode: Text.Wrap
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: qsTr("Ultimate Agent connaît l'état réel d'Ultimate IAH et agit sur les providers, MCP, la mémoire, les skills, les notes, le terminal et l'installation.")
                                        color: theme.mutedText
                                        font.pixelSize: root.compact ? design.textCaption : design.textBody
                                        lineHeight: 1.32
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }

                            Flow {
                                width: parent.width
                                spacing: design.space2
                                Repeater {
                                    model: [qsTr("Providers"), qsTr("MCP"), qsTr("Mémoire"), qsTr("Skills"), qsTr("Notes"), qsTr("Terminal"), qsTr("Images")]
                                    Rectangle {
                                        width: chip.implicitWidth + design.space3
                                        height: 22
                                        radius: 11
                                        color: theme.chip
                                        border.width: 1
                                        border.color: theme.border
                                        Text { id: chip; anchors.centerIn: parent; text: modelData; color: theme.mutedText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                                    }
                                }
                            }

                            RowLayout {
                                width: parent.width
                                Text { Layout.fillWidth: true; text: qsTr("Actions rapides"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; font.letterSpacing: 0.9 }
                                GhostButton { text: qsTr("Graphe Notes"); flat: true; font.pixelSize: design.textMicro; onClicked: { backend.setView("notes"); noteStore.graphOpen = true } }
                                Text { text: qsTr("Compétences →"); color: theme.accent; font.pixelSize: design.textMicro; MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: optionsDialog.open() } }
                            }

                            GridLayout {
                                width: parent.width
                                columns: root.compact ? 1 : 2
                                columnSpacing: design.space2
                                rowSpacing: design.space2
                                Repeater {
                                    model: [
                                        { label: qsTr("Diagnostiquer"), detail: qsTr("État réel du système"), icon: "icons/shield.svg",
                                          prompt: qsTr("Fais un diagnostic complet d'Ultimate IAH et de son écosystème. Utilise les outils réels, puis résume les problèmes et propose les corrections.") },
                                        { label: qsTr("Réparer"), detail: qsTr("Providers · MCP · workspace"), icon: "icons/plug.svg",
                                          prompt: qsTr("Vérifie les providers, MCP, le workspace et les composants d'Ultimate IAH. Répare ce qui peut l'être sans casser mes données, puis vérifie le résultat.") },
                                        { label: qsTr("Construire"), detail: qsTr("Implémenter et vérifier"), icon: "icons/spark.svg",
                                          prompt: qsTr("Implémente la prochaine étape utile de mon projet dans le workspace, puis lance les vérifications réelles et rapporte les preuves.") },
                                        { label: qsTr("Revue"), detail: qsTr("Trouver les régressions"), icon: "icons/branch.svg",
                                          prompt: qsTr("Fais une revue rigoureuse des changements récents : erreurs, régressions, risques et correctifs exacts classés par gravité.") },
                                        { label: qsTr("Providers"), detail: qsTr("Catalogue et modèles"), icon: "icons/connections.svg",
                                          prompt: qsTr("Audite les providers, les modèles par défaut et la configuration d'Ultimate Agent. Corrige ce qui est incohérent.") },
                                        { label: qsTr("Expliquer"), detail: qsTr("Capacités et limites"), icon: "icons/help.svg",
                                          prompt: qsTr("Explique précisément ce que tu peux faire dans Ultimate IAH, tes outils, tes limites et les options disponibles.") }
                                    ]
                                    AgentQuickAction {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: root.compact ? 52 : 64
                                        compact: root.compact
                                        icon: modelData.icon
                                        label: modelData.label
                                        detail: modelData.detail
                                        onClicked: root.quick(modelData.prompt)
                                    }
                                }
                            }

                            Rectangle {
                                width: parent.width
                                implicitHeight: contextColumn.implicitHeight + design.space4
                                radius: design.radiusMd
                                color: theme.chip
                                border.width: 1
                                border.color: theme.border
                                ColumnLayout {
                                    id: contextColumn
                                    anchors.fill: parent
                                    anchors.margins: design.space3
                                    spacing: design.space2
                                    Text { text: qsTr("Contexte actif"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; font.letterSpacing: 0.8 }
                                    Repeater {
                                        model: [
                                            { label: qsTr("Workspace"), value: backend.workspaceName, ok: true },
                                            { label: qsTr("Modèle"), value: connections.defaultModel !== "" ? connections.defaultModel : ultimateAgent.modelLabel, ok: connections.defaultModel !== "" },
                                            { label: qsTr("Mémoire"), value: qsTr("%n entrée(s)", "", memory.stats.total !== undefined ? memory.stats.total : 0), ok: memory.stats.total !== undefined && memory.stats.total > 0 },
                                            { label: qsTr("Skills actifs"), value: ultimateAgent.activeSkills.length > 0 ? ultimateAgent.activeSkills.join(", ") : qsTr("Aucun"), ok: ultimateAgent.activeSkills.length > 0 }
                                        ]
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: design.space2
                                            StatusDot { size: 4; color: modelData.ok ? theme.green : theme.dimText }
                                            Text { text: modelData.label; color: theme.dimText; font.pixelSize: design.textMicro; Layout.preferredWidth: root.compact ? 96 : 112; elide: Text.ElideRight }
                                            Text { Layout.fillWidth: true; text: modelData.value; color: theme.text; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                width: parent.width
                                spacing: design.space2
                                ThemedIcon { source: "icons/shield.svg"; width: 12; height: 12; color: theme.dimText }
                                Text { Layout.fillWidth: true; text: qsTr("Il peut agir sur votre machine. Les actions destructives demandent une confirmation explicite. Joignez des images avec le trombone."); color: theme.dimText; font.pixelSize: design.textMicro; wrapMode: Text.Wrap }
                            }
                        }
                    }

                    Rectangle {
                        id: scrollToEndButton
                        visible: root.showScrollButton && !root.empty
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.rightMargin: design.space3
                        anchors.bottomMargin: design.space3
                        width: 38
                        height: 38
                        radius: 19
                        color: theme.raised
                        border.width: 1
                        border.color: theme.borderStrong
                        ThemedIcon {
                            anchors.centerIn: parent
                            source: "icons/chevron.svg"
                            rotation: 90
                            width: 14
                            height: 14
                            color: theme.accent
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.scrollToBottomAnimated()
                        }
                    }
                }

                // Todo plan
                Rectangle {
                    id: todoPanel
                    Layout.fillWidth: true
                    Layout.leftMargin: design.space2
                    Layout.rightMargin: design.space2
                    Layout.bottomMargin: design.space1
                    visible: ultimateAgent.todos.length > 0
                    implicitHeight: todoColumn.implicitHeight + design.space2
                    radius: design.radiusMd
                    color: theme.raised
                    border.width: 1
                    border.color: theme.border
                    property bool expanded: true

                    ColumnLayout {
                        id: todoColumn
                        anchors.fill: parent
                        anchors.margins: design.space2
                        spacing: 2
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: design.space2
                            ThemedIcon { source: "icons/check.svg"; width: 13; height: 13; color: theme.accent }
                            Text { text: qsTr("À faire"); color: theme.brightText; font.pixelSize: design.textCaption; font.weight: Font.DemiBold }
                            Text { Layout.fillWidth: true; text: root.todoSummary(); color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                            Text {
                                text: todoPanel.expanded ? qsTr("Masquer") : qsTr("Afficher")
                                color: theme.accent
                                font.pixelSize: design.textMicro
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: todoPanel.expanded = !todoPanel.expanded }
                            }
                        }
                        Repeater {
                            model: todoPanel.expanded ? ultimateAgent.todos : []
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: design.space2
                                Rectangle {
                                    width: 6; height: 6; radius: 3
                                    color: modelData.status === "completed" ? theme.green
                                         : (modelData.status === "in_progress" ? theme.yellow : theme.dimText)
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.title
                                    color: modelData.status === "completed" ? theme.dimText : theme.text
                                    font.pixelSize: design.textMicro
                                    elide: Text.ElideRight
                                    font.strikeout: modelData.status === "completed"
                                }
                            }
                        }
                    }
                }

                // Pending image strip
                Flow {
                    Layout.fillWidth: true
                    Layout.leftMargin: design.space2
                    Layout.rightMargin: design.space2
                    spacing: design.space1
                    visible: ultimateAgent.pendingAttachments.length > 0
                    Repeater {
                        model: ultimateAgent.pendingAttachments
                        Rectangle {
                            width: 58
                            height: 58
                            radius: design.radiusSm
                            color: theme.chip
                            border.width: 1
                            border.color: theme.border
                            Image { anchors.fill: parent; anchors.margins: 2; source: "file://" + modelData.path; asynchronous: true; fillMode: Image.PreserveAspectCrop; cache: false }
                            IconButton { anchors.top: parent.top; anchors.right: parent.right; implicitWidth: 18; implicitHeight: 18; icon: "icons/close.svg"; hint: qsTr("Retirer l'image"); onClicked: ultimateAgent.removeAttachment(index) }
                        }
                    }
                }

                // Composer
                Rectangle {
                    id: composerBox
                    Layout.fillWidth: true
                    Layout.leftMargin: design.space1
                    Layout.rightMargin: design.space1
                    Layout.preferredHeight: composerBody.implicitHeight + design.space4
                    radius: design.radiusLg
                    color: theme.raised
                    border.width: 1
                    border.color: composerField.activeFocus ? theme.borderStrong : theme.border
                    Behavior on border.color { ColorAnimation { duration: design.durationFast } }

                    DropArea {
                        anchors.fill: parent
                        onDropped: function (drop) {
                            for (var i = 0; i < drop.urls.length; ++i)
                                ultimateAgent.addAttachment(drop.urls[i])
                        }
                    }

                    RowLayout {
                        id: composerBody
                        anchors.fill: parent
                        anchors.leftMargin: design.space2
                        anchors.rightMargin: design.space2
                        anchors.topMargin: 6
                        anchors.bottomMargin: design.space3
                        spacing: design.space1

                        Rectangle {
                            id: attachButton
                            Layout.alignment: Qt.AlignVCenter
                            implicitWidth: 38
                            implicitHeight: 38
                            radius: design.radiusMd
                            color: attachMouse.containsMouse ? theme.chip : "transparent"
                            border.width: 1
                            border.color: attachMouse.containsMouse ? theme.borderStrong : "transparent"
                            ThemedIcon {
                                anchors.centerIn: parent
                                source: "icons/paperclip.svg"
                                width: 17
                                height: 17
                                color: attachMouse.containsMouse ? theme.accent : theme.mutedText
                            }
                            MouseArea {
                                id: attachMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                Accessible.name: qsTr("Joindre une image")
                                onClicked: imageDialog.open()
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            TextArea {
                                id: composerField
                                anchors.fill: parent
                                topPadding: 9
                                bottomPadding: 7
                                leftPadding: 6
                                rightPadding: 6
                                color: theme.text
                                font.pixelSize: design.textBody
                                wrapMode: TextArea.Wrap
                                background: null
                                selectByMouse: true
                                Keys.onPressed: function (event) {
                                    if (root.commandMode) {
                                        if (event.key === Qt.Key_Down) {
                                            root.commandIndex = Math.min(root.filteredCommands.length - 1, root.commandIndex + 1)
                                            event.accepted = true
                                            return
                                        }
                                        if (event.key === Qt.Key_Up) {
                                            root.commandIndex = Math.max(0, root.commandIndex - 1)
                                            event.accepted = true
                                            return
                                        }
                                        if (event.key === Qt.Key_Escape) {
                                            composerField.text = ""
                                            event.accepted = true
                                            return
                                        }
                                        if (event.key === Qt.Key_Return && !(event.modifiers & Qt.ShiftModifier)) {
                                            root.executeCommand(root.filteredCommands[root.commandIndex])
                                            event.accepted = true
                                            return
                                        }
                                    }
                                    if (root.mentionMode) {
                                        if (event.key === Qt.Key_Down) {
                                            root.mentionIndex = Math.min(root.filteredMentions.length - 1, root.mentionIndex + 1)
                                            event.accepted = true
                                            return
                                        }
                                        if (event.key === Qt.Key_Up) {
                                            root.mentionIndex = Math.max(0, root.mentionIndex - 1)
                                            event.accepted = true
                                            return
                                        }
                                        if (event.key === Qt.Key_Escape) {
                                            composerField.text = composerField.text.replace(/@([^\s@]*)$/, "")
                                            event.accepted = true
                                            return
                                        }
                                        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Tab)
                                            && root.filteredMentions.length > 0) {
                                            root.selectMention(root.filteredMentions[root.mentionIndex])
                                            event.accepted = true
                                            return
                                        }
                                    }
                                    if (event.key === Qt.Key_Return && !(event.modifiers & Qt.ShiftModifier)) {
                                        root.send(composerField.text)
                                        event.accepted = true
                                    }
                                }
                                onTextChanged: { root.commandIndex = 0; root.mentionIndex = 0 }
                            }
                            Text {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                anchors.topMargin: 9
                                visible: composerField.text === ""
                                text: qsTr("Envoyez un message ou exécutez une tâche, des commandes /, des fichiers @ ou des sessions")
                                color: theme.dimText
                                font.pixelSize: design.textBody
                                elide: Text.ElideRight
                            }
                        }

                        Rectangle {
                            Layout.alignment: Qt.AlignVCenter
                            implicitWidth: 38
                            implicitHeight: 38
                            radius: design.radiusMd
                            color: ultimateAgent.busy ? theme.red : (root.canSend ? theme.accent : theme.chip)
                            border.width: 1
                            border.color: ultimateAgent.busy ? theme.red : (root.canSend ? theme.accent : theme.borderStrong)
                            Behavior on color { ColorAnimation { duration: design.durationFast } }
                            Behavior on border.color { ColorAnimation { duration: design.durationFast } }
                            ThemedIcon {
                                anchors.centerIn: parent
                                source: ultimateAgent.busy ? "icons/stop.svg" : "icons/send.svg"
                                width: 16
                                height: 16
                                color: ultimateAgent.busy ? "#ffffff" : (root.canSend ? theme.accentForeground : theme.dimText)
                            }
                            MouseArea {
                                anchors.fill: parent
                                enabled: root.canSend || ultimateAgent.busy
                                hoverEnabled: true
                                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: ultimateAgent.busy ? ultimateAgent.cancel() : root.send(composerField.text)
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: design.space2
                    spacing: design.space2

                    AppComboBox {
                        implicitHeight: 30
                        font.pixelSize: design.textMicro
                        Layout.preferredWidth: 132
                        model: [qsTr("Accès complet"), qsTr("Lecture seule")]
                        currentIndex: ultimateAgent.permissionMode === "read-only" ? 1 : 0
                        onActivated: function (index) {
                            ultimateAgent.permissionMode = index === 1 ? "read-only" : "workspace-write"
                        }
                    }
                    AppComboBox {
                        visible: ultimateAgent.modelChoices.length > 0
                        implicitHeight: 30
                        font.pixelSize: design.textMicro
                        Layout.preferredWidth: 230
                        model: ultimateAgent.modelChoices
                        textRole: "label"
                        currentIndex: {
                            const choices = ultimateAgent.modelChoices
                            for (var i = 0; i < choices.length; ++i) {
                                if (choices[i].value === ultimateAgent.modelChoice)
                                    return i
                            }
                            return 0
                        }
                        onActivated: function (index) {
                            if (model[index] !== undefined)
                                ultimateAgent.modelChoice = model[index].value
                        }
                    }
                    AppComboBox {
                        implicitHeight: 30
                        font.pixelSize: design.textMicro
                        Layout.preferredWidth: 108
                        model: [qsTr("Auto"), qsTr("Faible"), qsTr("Moyen"), qsTr("Élevé")]
                        currentIndex: root.effortIndex()
                        onActivated: function (index) {
                            ultimateAgent.reasoningEffort = ["", "low", "medium", "high"][index]
                        }
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.leftMargin: design.space2
                    Layout.rightMargin: design.space2
                    Layout.topMargin: 4
                    Layout.bottomMargin: design.space2
                    text: qsTr("%1 tours, %2 étapes").arg(ultimateAgent.turnCount).arg(ultimateAgent.stepCount)
                          + " · " + qsTr("%1 jetons/s").arg(Math.round(ultimateAgent.tokensPerSecond))
                          + " · " + qsTr("Jeton %1").arg(root.formatTokens(ultimateAgent.promptTokens + ultimateAgent.completionTokens))
                          + " · " + qsTr("Taux de réussite du cache : %1 %").arg(Math.round(ultimateAgent.cacheRate))
                          + " · " + ultimateAgent.contextPercent + "%"
                    color: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: 9
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }
            }
        }
    }

    Dialog {
        id: modelDialog
        modal: true
        width: Math.min(460, root.width - design.space5)
        height: Math.min(520, root.height - design.space6)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            Text { text: qsTr("Provider et modèle"); color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical: AppScrollBar {}
                ColumnLayout {
                    width: Math.min(420, parent.width)
                    Repeater {
                        model: ultimateAgent.modelChoices
                        GhostButton {
                            Layout.fillWidth: true
                            text: modelData.label
                            flat: ultimateAgent.modelChoice !== modelData.value
                            highlighted: ultimateAgent.modelChoice === modelData.value
                            font.pixelSize: design.textCaption
                            onClicked: { ultimateAgent.modelChoice = modelData.value; modelDialog.close() }
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; visible: ultimateAgent.modelChoices.length === 0; text: qsTr("Aucun provider configuré."); color: theme.dimText; font.pixelSize: design.textMicro }
                GhostButton { text: qsTr("Fermer"); flat: true; onClicked: modelDialog.close() }
            }
        }
    }

    Dialog {
        id: helpDialog
        modal: true
        width: Math.min(520, root.width - design.space5)
        height: Math.min(560, root.height - design.space6)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            Text { text: qsTr("Commandes"); color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical: AppScrollBar {}
                ColumnLayout {
                    width: Math.min(460, parent.width)
                    Repeater {
                        model: root.commands
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: modelData.label; color: theme.accent; font.family: backend.monoFont; font.pixelSize: design.textCaption; Layout.preferredWidth: 90 }
                            Text { Layout.fillWidth: true; text: modelData.detail; color: theme.mutedText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GhostButton { text: qsTr("Fermer"); flat: true; onClicked: helpDialog.close() }
            }
        }
    }

    Popup {
        id: mentionPopup
        parent: composerBox
        width: composerBox.width
        height: Math.min(root.filteredMentions.length * 30 + 12, 260)
        padding: 6
        closePolicy: Popup.NoAutoClose
        focus: false
        visible: root.mentionMode && root.filteredMentions.length > 0
        x: 0
        y: -height - 6
        background: Rectangle {
            radius: design.radiusMd
            color: theme.raised
            border.width: 1
            border.color: theme.borderStrong
        }
        contentItem: ListView {
            id: mentionList
            clip: true
            model: root.filteredMentions
            spacing: 1
            currentIndex: root.mentionIndex
            onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: AppScrollBar { policy: ScrollBar.AsNeeded }
            delegate: Rectangle {
                width: ListView.view.width
                height: 28
                radius: design.radiusSm
                color: index === root.mentionIndex ? theme.menuSelected : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: design.space2
                    anchors.rightMargin: design.space2
                    spacing: design.space2
                    ThemedIcon { source: "icons/file.svg"; width: 12; height: 12; color: theme.dimText }
                    Text { Layout.fillWidth: true; text: modelData; color: theme.text; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.selectMention(modelData)
                }
            }
        }
    }

    Popup {
        id: commandPopup
        parent: composerBox
        width: composerBox.width
        height: Math.min(root.filteredCommands.length * 35 + 12, 280)
        padding: 6
        closePolicy: Popup.NoAutoClose
        focus: false
        visible: root.commandMode && root.filteredCommands.length > 0
        x: 0
        y: -height - 6
        background: Rectangle {
            radius: design.radiusMd
            color: theme.raised
            border.width: 1
            border.color: theme.borderStrong
        }
        contentItem: ListView {
            clip: true
            model: root.filteredCommands
            spacing: 1
            currentIndex: root.commandIndex
            onCurrentIndexChanged: if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: AppScrollBar { policy: ScrollBar.AsNeeded }
            delegate: Rectangle {
                width: ListView.view.width
                height: 34
                radius: design.radiusSm
                color: index === root.commandIndex ? theme.menuSelected : "transparent"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: design.space2
                    anchors.rightMargin: design.space2
                    spacing: design.space2
                    Text { text: modelData.label; color: theme.accent; font.family: backend.monoFont; font.pixelSize: design.textCaption }
                    Text { Layout.fillWidth: true; text: modelData.detail; color: theme.mutedText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.executeCommand(modelData)
                }
            }
        }
    }

    NumberAnimation {
        id: scrollAnimation
        target: messageList
        property: "contentY"
        duration: 180
        easing.type: Easing.OutCubic
    }

    FileDialog {
        id: imageDialog
        title: qsTr("Joindre une image")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.webp *.gif)")]
        onAccepted: ultimateAgent.addAttachment(selectedFile)
    }

    Dialog {
        id: historyDialog
        modal: true
        width: Math.min(380, root.width - design.space5)
        height: Math.min(620, root.height - design.space6)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Discussions"); color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                IconButton { icon: "icons/plus.svg"; hint: qsTr("Nouvelle conversation"); onClicked: { ultimateAgent.newConversation(); historyDialog.close() } }
                IconButton { icon: "icons/close.svg"; hint: qsTr("Fermer"); onClicked: historyDialog.close() }
            }
            SettingsField {
                Layout.fillWidth: true
                placeholderText: qsTr("Rechercher…")
                text: ultimateAgent.conversationQuery
                onTextChanged: ultimateAgent.conversationQuery = text
            }
            AgentConversationList {
                Layout.fillWidth: true
                Layout.fillHeight: true
                items: ultimateAgent.conversations
                currentId: ultimateAgent.conversationId
                onSelected: function (id) { ultimateAgent.selectConversation(id); historyDialog.close() }
                onRenameRequested: function (id, title) { historyDialog.close(); root.openRename(id, title) }
                onDeleteRequested: function (id, title) { historyDialog.close(); root.openDelete(id, title) }
                onPinRequested: function (id) { ultimateAgent.pinConversation(id) }
            }
            GhostButton {
                Layout.fillWidth: true
                text: qsTr("Nouvelle discussion")
                flat: true
                onClicked: { ultimateAgent.newConversation(); historyDialog.close() }
            }
        }
    }

    Dialog {
        id: optionsDialog
        modal: true
        width: Math.min(560, root.width - design.space5)
        height: Math.min(680, root.height - design.space6)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space5
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ScrollView {
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical: AppScrollBar {}
            ColumnLayout {
                width: Math.min(520, parent.width)
                spacing: design.space3
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("Options de l'agent"); color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                    IconButton { icon: "icons/close.svg"; hint: qsTr("Fermer"); onClicked: optionsDialog.close() }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("Mode de travail"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                    Text { text: qsTr("Contexte %1 %").arg(ultimateAgent.contextPercent); color: ultimateAgent.contextPercent > 80 ? theme.orange : theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                    GhostButton { text: qsTr("Compacter"); flat: true; font.pixelSize: design.textMicro; onClicked: ultimateAgent.compactConversation() }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space1
                    Repeater {
                        model: [
                            { id: "assistant", label: qsTr("Assistant") },
                            { id: "plan", label: qsTr("Plan") },
                            { id: "diagnostic", label: qsTr("Diagnostic") },
                            { id: "build", label: qsTr("Construction") },
                            { id: "review", label: qsTr("Revue") }
                        ]
                        GhostButton {
                            Layout.fillWidth: true
                            text: modelData.label
                            flat: ultimateAgent.mode !== modelData.id
                            highlighted: ultimateAgent.mode === modelData.id
                            font.pixelSize: design.textMicro
                            onClicked: ultimateAgent.mode = modelData.id
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }
                RowLayout {
                    Layout.fillWidth: true
                    Text { Layout.fillWidth: true; text: qsTr("Compétences"); color: theme.text; font.pixelSize: design.textLabel; font.weight: Font.DemiBold }
                    Text { text: ultimateAgent.activeSkills.length + " " + qsTr("active(s)"); color: theme.dimText; font.pixelSize: design.textMicro }
                }
                Text { Layout.fillWidth: true; text: qsTr("Les compétences sélectionnées sont injectées dans le prompt de cette conversation."); color: theme.dimText; font.pixelSize: design.textMicro; wrapMode: Text.Wrap }
                Repeater {
                    model: ultimateAgent.availableSkills
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: skillRow.implicitHeight + design.space2
                        radius: design.radiusSm
                        color: ultimateAgent.activeSkills.indexOf(modelData.id) >= 0 ? theme.chip : theme.background
                        border.width: 1
                        border.color: ultimateAgent.activeSkills.indexOf(modelData.id) >= 0 ? theme.accent : theme.border
                        RowLayout {
                            id: skillRow
                            anchors.fill: parent
                            anchors.margins: design.space2
                            spacing: design.space2
                            ThemedIcon { source: "icons/skill.svg"; width: 14; height: 14; color: theme.accent }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Text { Layout.fillWidth: true; text: modelData.name; color: theme.text; font.pixelSize: design.textCaption; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                Text { Layout.fillWidth: true; text: modelData.description; color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                            }
                            Text { text: modelData.scope; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                            GhostButton {
                                text: ultimateAgent.activeSkills.indexOf(modelData.id) >= 0 ? qsTr("Retirer") : qsTr("Activer")
                                flat: true
                                font.pixelSize: design.textMicro
                                onClicked: ultimateAgent.toggleSkill(modelData.id)
                            }
                        }
                    }
                }
                Text { Layout.fillWidth: true; visible: ultimateAgent.availableSkills.length === 0; text: qsTr("Aucune compétence installée. Créez-en dans Agents → Skills."); color: theme.dimText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
                Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }
                Text { text: qsTr("Conversation"); color: theme.text; font.pixelSize: design.textLabel; font.weight: Font.DemiBold }
                RowLayout {
                    Layout.fillWidth: true
                    GhostButton { text: qsTr("Copier le transcript"); flat: true; font.pixelSize: design.textMicro; onClicked: { ultimateAgent.copyText(ultimateAgent.conversationMarkdown(ultimateAgent.conversationId)); Window.window.showToast(qsTr("Transcript copié.")) } }
                    Item { Layout.fillWidth: true }
                    GhostButton { text: qsTr("Vider"); flat: true; font.pixelSize: design.textMicro; onClicked: { ultimateAgent.clearConversation(); optionsDialog.close() } }
                    GhostButton { text: qsTr("Supprimer"); flat: true; font.pixelSize: design.textMicro; onClicked: { optionsDialog.close(); root.openDelete(ultimateAgent.conversationId, ultimateAgent.conversationTitle) } }
                }
                Text { Layout.fillWidth: true; text: qsTr("Les images sont copiées dans les données de l'application. Quatre images de 12 Mo maximum par message."); color: theme.dimText; font.pixelSize: design.textMicro; wrapMode: Text.Wrap }
            }
        }
    }

    Dialog {
        id: renameDialog
        modal: true
        width: Math.min(420, root.width - design.space5)
        height: 170
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            Text { text: qsTr("Renommer la discussion"); color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
            SettingsField { id: renameField; Layout.fillWidth: true; placeholderText: qsTr("Titre de la discussion") }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GhostButton { text: qsTr("Annuler"); flat: true; onClicked: renameDialog.close() }
                ActionButton { text: qsTr("Renommer"); onClicked: { ultimateAgent.renameConversation(root.renameId, renameField.text); renameDialog.close() } }
            }
        }
    }

    Dialog {
        id: deleteDialog
        modal: true
        width: Math.min(420, root.width - design.space5)
        height: 190
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            Text { text: qsTr("Supprimer la discussion"); color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
            Text { Layout.fillWidth: true; text: qsTr("Supprimer « %1 » et tout son historique ? Cette action est définitive.").arg(root.deleteValue); color: theme.mutedText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GhostButton { text: qsTr("Annuler"); flat: true; onClicked: deleteDialog.close() }
                ActionButton { text: qsTr("Supprimer"); danger: true; onClicked: { ultimateAgent.deleteConversation(root.deleteId); deleteDialog.close() } }
            }
        }
    }

    Dialog {
        id: imageViewer
        modal: true
        width: Math.min(root.width - design.space5, 1100)
        height: Math.min(root.height - design.space5, 800)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: 0
        onClosed: root.imagePreview = ""
        background: Rectangle { color: "#09090c"; radius: design.radiusMd }
        contentItem: Item {
            Image {
                anchors.fill: parent
                anchors.margins: design.space2
                source: root.imagePreview
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }
            IconButton { anchors.top: parent.top; anchors.right: parent.right; icon: "icons/close.svg"; hint: qsTr("Fermer"); onClicked: imageViewer.close() }
        }
    }

    Connections {
        target: root
        function onImagePreviewChanged() { if (root.imagePreview !== "") imageViewer.open() }
    }
}
