import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(1180, width - design.space7))
    readonly property var current: bots.selectedBot
    readonly property var currentId: bots.selectedBotId
    function permissionLabel(permission) {
        return permission === "workspace-write" ? qsTr("Écriture workspace") : qsTr("Lecture seule")
    }
    function permissionShort(permission) {
        return permission === "workspace-write" ? "RW" : "RO"
    }
    property bool editing: false
    property int editingId: -1
    property bool scheduleOpen: false
    property int deleteId: -1
    property string deleteName: ""

    function requestDelete(id) {
        const bot = root.botById(id)
        deleteId = id
        deleteName = bot.name !== undefined ? bot.name : ""
        deleteDialog.open()
    }

    function openEditor(id) {
        editingId = id
        const bot = id > 0 ? root.botById(id) : ({})
        nameField.text = bot.name !== undefined ? bot.name : ""
        descriptionField.text = bot.description !== undefined ? bot.description : ""
        instructionsField.text = bot.instructions !== undefined ? bot.instructions : ""
        permissionBox.currentIndex = bot.permission === "workspace-write" ? 1 : 0
        enabledBox.checked = bot.enabled !== undefined ? bot.enabled : true
        editor.title = id > 0 ? qsTr("Modifier le bot") : qsTr("Nouveau bot")
        editor.open()
    }

    function botById(id) {
        for (let i = 0; i < bots.bots.length; ++i) {
            if (bots.bots[i].id === id)
                return bots.bots[i]
        }
        return ({})
    }

    function saveEditor() {
        if (nameField.text.trim() === "")
            return
        if (editingId > 0)
            bots.updateBot(editingId, nameField.text, descriptionField.text,
                           instructionsField.text,
                           permissionBox.currentIndex === 1 ? "workspace-write" : "read-only",
                           enabledBox.checked)
        else
            bots.createBot(nameField.text, descriptionField.text, instructionsField.text,
                           permissionBox.currentIndex === 1 ? "workspace-write" : "read-only")
        editor.close()
    }

    function openSchedule() {
        scheduleTitle.text = ""
        schedulePrompt.text = ""
        scheduleInterval.value = 60
        scheduleDialog.open()
    }

    function createSchedule() {
        if (scheduleTitle.text.trim() === "" || schedulePrompt.text.trim() === "")
            return
        bots.createSchedule(root.currentId, scheduleTitle.text, schedulePrompt.text,
                            scheduleInterval.value)
        scheduleDialog.close()
    }

    function send() {
        const text = composerField.text.trim()
        if (text === "" || root.currentId <= 0)
            return
        bots.sendMessage(root.currentId, text)
        composerField.text = ""
    }

    function scrollMessages() {
        Qt.callLater(function () {
            if (bots.messages.length === 0 || !messageScroll.contentItem)
                return
            const flick = messageScroll.contentItem
            flick.contentY = Math.max(0, flick.contentHeight - flick.height)
        })
    }

    Component.onCompleted: {
        bots.refresh()
        Qt.callLater(function () {
            if (root.currentId > 0)
                composerField.forceActiveFocus()
        })
    }

    Connections {
        target: bots
        function onMessagesChanged() { root.scrollMessages() }
        function onSelectionChanged() {
            root.scrollMessages()
            Qt.callLater(function () { if (root.currentId > 0) composerField.forceActiveFocus() })
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(56, headerColumn.implicitHeight + design.space4)
            clip: true
            contentWidth: availableWidth
            ScrollBar.vertical: AppScrollBar {}

            ColumnLayout {
                id: headerColumn
                width: root.contentWidth
                x: Math.max(design.space5, (parent.width - width) / 2)
                spacing: design.space3

                Item { Layout.fillWidth: true; Layout.preferredHeight: design.space4 }

                PageHeader {
                    Layout.fillWidth: true
                    title: qsTr("Bots")
                    description: qsTr("Des opérateurs persistants pour l'écosystème Ultimate IAH.")
                    icon: "icons/agents.svg"
                    accent: theme.accent
                    ActionButton { text: qsTr("Nouveau bot"); onClicked: root.openEditor(-1) }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(58, serviceRow.implicitHeight + design.space3)
                    radius: design.radiusMd
                    color: theme.surface
                    border.width: 1
                    border.color: theme.border

                    RowLayout {
                        id: serviceRow
                        anchors.fill: parent
                        anchors.margins: design.space3
                        spacing: design.space2

                        StatusDot { size: 6; color: bots.serverRunning ? theme.green : theme.red }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                text: bots.serverRunning ? qsTr("Service de bots actif") : qsTr("Service de bots arrêté")
                                color: theme.text
                                font.pixelSize: design.textCaption
                                font.weight: Font.DemiBold
                            }
                            Text {
                                Layout.fillWidth: true
                                text: bots.serverRunning
                                      ? bots.serverUrl + "  ·  jeton " + bots.tokenMasked
                                      : (bots.error !== "" ? bots.error : qsTr("API locale non démarrée"))
                                color: theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: design.textMicro
                                elide: Text.ElideRight
                            }
                        }
                        GhostButton { text: qsTr("Copier le jeton"); flat: true; font.pixelSize: design.textMicro; onClicked: bots.copyToken() }
                        GhostButton {
                            text: qsTr("Dashboard")
                            flat: true
                            font.pixelSize: design.textMicro
                            enabled: bots.serverRunning
                            onClicked: Qt.openUrlExternally(bots.serverUrl)
                        }
                        GhostButton {
                            text: bots.serverRunning ? qsTr("Arrêter") : qsTr("Démarrer")
                            flat: true
                            font.pixelSize: design.textMicro
                            onClicked: bots.serverRunning ? bots.stopServer() : bots.startServer()
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Math.max(design.space5, (root.width - root.contentWidth) / 2)
            Layout.rightMargin: Math.max(design.space5, (root.width - root.contentWidth) / 2)
            Layout.bottomMargin: design.space3
            spacing: design.space3

            // Bot roster + schedules
            Rectangle {
                Layout.preferredWidth: Math.min(280, root.contentWidth * 0.32)
                Layout.fillHeight: true
                radius: design.radiusLg
                color: theme.surface
                border.width: 1
                border.color: theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: design.space3
                    spacing: design.space2

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("Roster"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; font.letterSpacing: 0.8 }
                        Item { Layout.fillWidth: true }
                        Text { text: bots.bots.length; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: availableWidth
                        ScrollBar.vertical: AppScrollBar {}

                        ColumnLayout {
                            width: parent.width
                            spacing: design.space1

                            Repeater {
                                model: bots.bots
                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: botColumn.implicitHeight + design.space3
                                    radius: design.radiusMd
                                    color: modelData.id === root.currentId ? theme.raised : (botMouse.containsMouse ? theme.chip : "transparent")
                                    border.width: modelData.id === root.currentId ? 1 : 0
                                    border.color: theme.borderStrong

                                    ColumnLayout {
                                        id: botColumn
                                        anchors.fill: parent
                                        anchors.margins: design.space2
                                        spacing: 1
                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: design.space2
                                            StatusDot { size: 5; color: modelData.enabled ? theme.green : theme.dimText }
                                            Text { Layout.fillWidth: true; text: modelData.name; color: theme.text; font.pixelSize: design.textLabel; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                            Text {
                                                text: root.permissionShort(modelData.permission)
                                                color: modelData.permission === "workspace-write" ? theme.orange : theme.dimText
                                                font.family: backend.monoFont
                                                font.pixelSize: 9
                                            }
                                        }
                                        Text { Layout.fillWidth: true; text: modelData.description; color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                                    }
                                    MouseArea {
                                        id: botMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: bots.selectBot(modelData.id)
                                    }
                                }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: theme.border; Layout.topMargin: design.space2 }
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: qsTr("Planifications"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                                Item { Layout.fillWidth: true }
                                IconButton { icon: "icons/plus.svg"; hint: qsTr("Planifier une tâche"); onClicked: root.openSchedule() }
                            }
                            Repeater {
                                model: bots.schedules
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: design.space1
                                    StatusDot { size: 4; color: modelData.enabled ? theme.green : theme.dimText }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 0
                                        Text { Layout.fillWidth: true; text: modelData.title; color: theme.text; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                                        Text { Layout.fillWidth: true; text: qsTr("toutes les %1 min").arg(modelData.intervalMinutes); color: theme.dimText; font.pixelSize: 8 }
                                    }
                                    GhostButton { text: modelData.enabled ? qsTr("Pause") : qsTr("Reprendre"); flat: true; font.pixelSize: 8; onClicked: bots.toggleSchedule(modelData.id, !modelData.enabled) }
                                    IconButton { icon: "icons/close.svg"; hint: qsTr("Supprimer"); onClicked: bots.removeSchedule(modelData.id) }
                                }
                            }
                        }
                    }

                    GhostButton {
                        Layout.fillWidth: true
                        text: qsTr("Nouveau bot")
                        flat: true
                        font.pixelSize: design.textCaption
                        onClicked: root.openEditor(-1)
                    }
                }
            }

            // Chat panel
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: design.radiusLg
                color: theme.background
                border.width: 1
                border.color: theme.border

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 52
                        Layout.leftMargin: design.space4
                        Layout.rightMargin: design.space3
                        spacing: design.space2

                        BrandMark { width: 26; height: 26; size: 26; accent: false }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                Layout.fillWidth: true
                                text: root.current.name !== undefined ? root.current.name : qsTr("Aucun bot sélectionné")
                                color: theme.brightText
                                font.pixelSize: design.textBody
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.current.description !== undefined ? root.current.description : qsTr("Créez ou sélectionnez un bot.")
                                color: theme.dimText
                                font.pixelSize: design.textMicro
                                elide: Text.ElideRight
                            }
                        }
                        Text {
                            visible: root.currentId > 0
                            text: root.permissionLabel(root.current.permission)
                            color: root.current.permission === "workspace-write" ? theme.orange : theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textMicro
                        }
                        IconButton { visible: root.currentId > 0; icon: "icons/pencil.svg"; hint: qsTr("Modifier"); onClicked: root.openEditor(root.currentId) }
                        IconButton { visible: root.currentId > 0; icon: "icons/trash.svg"; hint: qsTr("Supprimer"); onClicked: root.requestDelete(root.currentId) }
                        IconButton { visible: root.currentId > 0; icon: "icons/reload.svg"; hint: qsTr("Vider la conversation"); onClicked: bots.clearConversation(root.currentId) }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ScrollView {
                            id: messageScroll
                            anchors.fill: parent
                            anchors.margins: design.space4
                            clip: true
                            contentWidth: availableWidth
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            ScrollBar.vertical: AppScrollBar {
                                policy: bots.messages.length > 0 ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                            }

                            Column {
                                width: parent.width
                                topPadding: design.space2
                                bottomPadding: design.space2
                                spacing: design.space3

                                Item { width: 1; height: design.space3 }

                                Repeater {
                                    model: bots.messages
                                    Item {
                                        width: parent.width
                                        height: bubble.height

                                        Rectangle {
                                            id: bubble
                                            width: Math.min(parent.width, Math.max(180, parent.width * (modelData.role === "user" ? 0.78 : 0.96)))
                                            anchors.right: modelData.role === "user" ? parent.right : undefined
                                            anchors.left: modelData.role === "assistant" ? parent.left : undefined
                                            height: messageText.implicitHeight + design.space3
                                            radius: design.radiusMd
                                            color: modelData.role === "user"
                                                   ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.18)
                                                   : (modelData.kind === "error" ? Qt.rgba(theme.red.r, theme.red.g, theme.red.b, 0.10) : theme.raised)
                                            border.width: 1
                                            border.color: modelData.role === "user"
                                                          ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.48)
                                                          : (modelData.kind === "error" ? theme.red : theme.border)

                                            Text {
                                                id: messageText
                                                x: design.space2
                                                y: design.space1
                                                width: parent.width - design.space4
                                                text: modelData.content
                                                color: theme.text
                                                font.pixelSize: design.textCaption
                                                wrapMode: Text.Wrap
                                                textFormat: modelData.role === "assistant" ? Text.MarkdownText : Text.PlainText
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Column {
                            anchors.centerIn: parent
                            spacing: design.space2
                            visible: bots.messages.length === 0 && root.currentId > 0
                            BrandMark { width: 42; height: 42; size: 42; accent: false; anchors.horizontalCenter: parent.horizontalCenter }
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Confiez une première tâche à %1.").arg(root.current.name !== undefined ? root.current.name : ""); color: theme.mutedText; font.pixelSize: design.textCaption }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: design.space3
                        Layout.rightMargin: design.space3
                        Layout.preferredHeight: 26
                        spacing: design.space2
                        Text {
                            Layout.fillWidth: true
                            text: bots.running ? qsTr("%1 · file %2").arg(bots.status).arg(bots.queueCount) : qsTr("Prêt")
                            color: bots.running ? theme.yellow : theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textMicro
                            elide: Text.ElideRight
                        }
                        Text {
                            visible: bots.error !== ""
                            text: bots.error
                            color: theme.red
                            font.pixelSize: design.textMicro
                            elide: Text.ElideLeft
                            Layout.maximumWidth: parent.width * 0.5
                        }
                        GhostButton {
                            visible: bots.running
                            text: qsTr("Annuler")
                            flat: true
                            font.pixelSize: design.textMicro
                            onClicked: bots.cancel()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: composerRow.implicitHeight + design.space4
                        Layout.leftMargin: design.space3
                        Layout.rightMargin: design.space3
                        Layout.bottomMargin: design.space3
                        radius: design.radiusLg
                        color: theme.raised
                        border.width: 1
                        border.color: composerField.activeFocus ? theme.borderStrong : theme.border

                        RowLayout {
                            id: composerRow
                            anchors.fill: parent
                            anchors.leftMargin: design.space2
                            anchors.rightMargin: design.space2
                            anchors.topMargin: 6
                            anchors.bottomMargin: design.space3
                            spacing: design.space2
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
                                    enabled: root.currentId > 0
                                    Keys.onPressed: function (event) {
                                        if (event.key === Qt.Key_Return && !(event.modifiers & Qt.ShiftModifier)) {
                                            root.send(); event.accepted = true
                                        }
                                    }
                                }
                                Text {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.leftMargin: 6
                                    anchors.rightMargin: 6
                                    anchors.topMargin: 9
                                    visible: composerField.text === "" && root.currentId > 0
                                    text: qsTr("Confier une tâche à %1…").arg(root.current.name !== undefined ? root.current.name : "")
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
                                color: root.currentId > 0 && composerField.text.trim() !== "" ? theme.accent : theme.chip
                                border.width: 1
                                border.color: root.currentId > 0 && composerField.text.trim() !== "" ? theme.accent : theme.borderStrong
                                ThemedIcon { anchors.centerIn: parent; source: "icons/send.svg"; width: 16; height: 16; color: root.currentId > 0 && composerField.text.trim() !== "" ? theme.accentForeground : theme.dimText }
                                MouseArea { anchors.fill: parent; enabled: root.currentId > 0; cursorShape: Qt.PointingHandCursor; onClicked: root.send() }
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: editor
        modal: true
        width: Math.min(580, root.width - design.space7)
        height: Math.min(620, root.height - design.space7)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space5
        title: qsTr("Nouveau bot")
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            Text { text: editor.title; color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
            SettingsField { id: nameField; Layout.fillWidth: true; placeholderText: qsTr("Nom du bot") }
            SettingsField { id: descriptionField; Layout.fillWidth: true; placeholderText: qsTr("Rôle court") }
            Text { text: qsTr("Instructions"); color: theme.dimText; font.pixelSize: design.textMicro }
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: theme.background
                radius: design.radiusSm
                border.width: 1
                border.color: instructionsField.activeFocus ? theme.accent : theme.border
                TextArea {
                    id: instructionsField
                    anchors.fill: parent
                    anchors.margins: 6
                    color: theme.text
                    font.pixelSize: design.textCaption
                    wrapMode: TextArea.Wrap
                    background: null
                    selectByMouse: true
                }
            }
            RowLayout {
                Layout.fillWidth: true
                AppComboBox {
                    id: permissionBox
                    Layout.fillWidth: true
                    model: [qsTr("Lecture seule"), qsTr("Écriture workspace")]
                }
                CheckBox { id: enabledBox; text: qsTr("Actif"); checked: true; font.pixelSize: design.textCaption }
            }
            RowLayout {
                Layout.fillWidth: true
                GhostButton { visible: editingId > 0; text: qsTr("Supprimer"); flat: true; onClicked: { editor.close(); root.requestDelete(editingId) } }
                Item { Layout.fillWidth: true }
                GhostButton { text: qsTr("Annuler"); flat: true; onClicked: editor.close() }
                ActionButton { text: qsTr("Enregistrer"); onClicked: root.saveEditor() }
            }
        }
    }

    Dialog {
        id: deleteDialog
        modal: true
        width: Math.min(400, root.width - design.space7)
        height: 190
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space5
        title: qsTr("Supprimer le bot")
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space3
            Text { text: deleteDialog.title; color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
            Text {
                Layout.fillWidth: true
                text: qsTr("Supprimer « %1 » et tout son historique ? Cette action est définitive.").arg(root.deleteName)
                color: theme.mutedText
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GhostButton { text: qsTr("Annuler"); flat: true; onClicked: deleteDialog.close() }
                ActionButton { text: qsTr("Supprimer"); danger: true; onClicked: { if (root.deleteId > 0) bots.removeBot(root.deleteId); deleteDialog.close() } }
            }
        }
    }

    Dialog {
        id: scheduleDialog
        modal: true
        width: Math.min(500, root.width - design.space7)
        height: 330
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space5
        title: qsTr("Planifier une tâche")
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            Text { text: scheduleDialog.title; color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
            SettingsField { id: scheduleTitle; Layout.fillWidth: true; placeholderText: qsTr("Titre de la planification") }
            SettingsField { id: schedulePrompt; Layout.fillWidth: true; placeholderText: qsTr("Tâche à exécuter") }
            RowLayout {
                Text { text: qsTr("Toutes les"); color: theme.dimText; font.pixelSize: design.textCaption }
                SpinBox { id: scheduleInterval; from: 1; to: 10080; value: 60; editable: true }
                Text { text: qsTr("minutes"); color: theme.dimText; font.pixelSize: design.textCaption }
                Item { Layout.fillWidth: true }
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GhostButton { text: qsTr("Annuler"); flat: true; onClicked: scheduleDialog.close() }
                ActionButton { text: qsTr("Planifier"); onClicked: root.createSchedule() }
            }
        }
    }
}
