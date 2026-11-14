import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: root

    readonly property bool wide: width >= 1120
    readonly property bool compactRail: width < 880
    readonly property var note: noteStore.selectedNote
    readonly property bool hasNote: noteStore.selectedPath !== ""
    readonly property int leftWidth: compactRail ? 214 : 248
    readonly property int rightWidth: 258

    property string loadedPath: ""
    property string deletePath: ""
    property string deleteTitle: ""
    property bool dirty: false
    property bool loading: false
    property bool previewMode: false

    function loadSelected() {
        if (!root.hasNote) {
            loadedPath = ""
            titleField.text = ""
            editor.text = ""
            dirty = false
            return
        }
        if (root.loadedPath === noteStore.selectedPath)
            return
        root.loading = true
        root.loadedPath = noteStore.selectedPath
        titleField.text = root.note.title !== undefined ? root.note.title : ""
        editor.text = root.note.content !== undefined ? root.note.content : ""
        root.dirty = false
        root.loading = false
        autosave.stop()
    }

    function saveNow() {
        if (!root.hasNote || !root.dirty)
            return
        if (noteStore.saveNote(noteStore.selectedPath, editor.text)) {
            root.dirty = false
            Window.window.showToast(qsTr("Note enregistrée."))
        }
    }

    function newNote() {
        noteStore.createNote(qsTr("Nouvelle note"))
        Qt.callLater(function () { editor.forceActiveFocus(); editor.cursorPosition = editor.text.length })
    }

    function newTask() {
        noteStore.createTaskNote(qsTr("Nouvelle tâche"), "", backend.workspace)
        Qt.callLater(function () { editor.forceActiveFocus(); editor.cursorPosition = editor.text.length })
    }

    function delegateNote() {
        if (!root.hasNote)
            return
        root.saveNow()
        const prompt = qsTr("Traite cette note comme une tâche du harness. Statut : %1. Titre : %2.\n\n%3")
            .arg(root.note.status !== undefined && root.note.status !== "" ? root.note.status : "todo")
            .arg(root.note.title)
            .arg(editor.text)
        ultimateAgent.sendMessage(prompt)
        backend.setView("agent")
    }

    function filteredPaths() {
        var paths = []
        for (var i = 0; i < noteStore.filteredNotes.length; ++i)
            paths.push(noteStore.filteredNotes[i].path)
        return paths
    }

    function setTaskStatus(status) {
        if (root.hasNote)
            noteStore.setNoteStatus(noteStore.selectedPath, status)
    }

    function dailyNote() {
        noteStore.dailyNote()
        Qt.callLater(function () { editor.forceActiveFocus(); editor.cursorPosition = editor.text.length })
    }

    function renderMarkdown(text) {
        if (text === undefined || text === null)
            return ""
        return String(text).replace(/\[\[([^\]|#]+)(?:#([^\]|]+))?(?:\|([^\]]+))?\]\]/g,
            function (_, target, heading, label) {
                var name = label ? label : target
                if (heading)
                    name += " › " + heading
                return "[" + name + "](wiki:" + encodeURIComponent(target) + ")"
            })
    }

    function offsetForLine(lineNumber) {
        var text = editor.text
        var position = 0
        for (var line = 1; line < lineNumber; ++line) {
            var index = text.indexOf("\n", position)
            if (index < 0)
                return text.length
            position = index + 1
        }
        return position
    }

    function insertAttachment() {
        if (!root.hasNote) {
            root.newNote()
            return
        }
        imageDialog.open()
    }

    function openDelete(path, title) {
        root.deletePath = path
        root.deleteTitle = title
        deleteDialog.open()
    }

    function togglePin() {
        if (root.hasNote)
            noteStore.togglePin(noteStore.selectedPath)
    }

    Component.onCompleted: { noteStore.refresh(); loadSelected() }
    onVisibleChanged: if (visible) { noteStore.refresh(); loadSelected() }

    Connections {
        target: noteStore
        function onGraphOpenChanged() {
            if (noteStore.graphOpen)
                graphDialog.open()
            else if (graphDialog.opened)
                graphDialog.close()
        }
        function onSelectedChanged() { root.loadSelected() }
        function onChanged() {
            if (!root.dirty && root.loadedPath !== "" && !root.hasNote) {
                root.loadedPath = ""
                titleField.text = ""
                editor.text = ""
            }
        }
        function onErrorChanged() {
            if (noteStore.error !== "")
                Window.window.showToast(noteStore.error)
        }
    }

    Timer {
        id: autosave
        interval: 650
        repeat: false
        onTriggered: root.saveNow()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        PageHeader {
            Layout.fillWidth: true
            Layout.leftMargin: design.space6
            Layout.rightMargin: design.space6
            Layout.topMargin: design.space4
            title: qsTr("Notes")
            description: qsTr("Un vault Markdown local, relié par des wikilinks, des tags et un graphe.")
            icon: "icons/notes.svg"
            accent: theme.yellow
            ActionButton { text: qsTr("Nouvelle note"); onClicked: root.newNote() }
            ActionButton { text: qsTr("Nouvelle tâche"); onClicked: root.newTask() }
            ActionButton { text: qsTr("Modèles"); onClicked: templatesDialog.open() }
            ActionButton { text: qsTr("Quotidienne"); onClicked: root.dailyNote() }
            ActionButton { text: qsTr("Graphe"); onClicked: graphDialog.open() }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: design.space4
            Layout.rightMargin: design.space4
            Layout.bottomMargin: design.space4
            spacing: design.space2

            // ── notes rail ───────────────────────────────────────────────
            Rectangle {
                Layout.preferredWidth: root.leftWidth
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
                        Text { text: qsTr("Vault"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; font.letterSpacing: 0.8 }
                        Item { Layout.fillWidth: true }
                        Text { text: noteStore.count; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        IconButton { implicitWidth: 22; implicitHeight: 22; icon: "icons/database.svg"; hint: qsTr("Sauvegarde et export"); onClicked: backupDialog.open() }
                        IconButton { implicitWidth: 22; implicitHeight: 22; icon: "icons/branch.svg"; hint: qsTr("Synchronisation Git"); onClicked: { noteStore.gitRefresh(); gitDialog.open() } }
                        IconButton { implicitWidth: 22; implicitHeight: 22; icon: "icons/plug.svg"; hint: qsTr("Plugins de notes"); onClicked: notePluginsDialog.open() }
                        IconButton { implicitWidth: 22; implicitHeight: 22; icon: "icons/plus.svg"; hint: qsTr("Nouvelle note"); onClicked: root.newNote() }
                    }

                    SettingsField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Rechercher une note…")
                        text: noteStore.query
                        onTextChanged: noteStore.query = text
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 4
                        Repeater {
                            model: noteStore.tags.slice(0, 7)
                            Rectangle {
                                width: tagLabel.implicitWidth + 12
                                height: 20
                                radius: 10
                                color: noteStore.activeTag === modelData.name ? Qt.rgba(theme.yellow.r, theme.yellow.g, theme.yellow.b, 0.16) : theme.chip
                                border.width: 1
                                border.color: noteStore.activeTag === modelData.name ? theme.yellow : theme.border
                                Text { id: tagLabel; anchors.centerIn: parent; text: "#" + modelData.name; color: theme.mutedText; font.family: backend.monoFont; font.pixelSize: 8 }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: noteStore.activeTag = noteStore.activeTag === modelData.name ? "" : modelData.name }
                            }
                        }
                        Text {
                            visible: noteStore.activeTag !== ""
                            text: qsTr("effacer")
                            color: theme.accent
                            font.pixelSize: 9
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: noteStore.activeTag = "" }
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 4
                        Repeater {
                            model: noteStore.folders.slice(0, 6)
                            Rectangle {
                                width: folderLabel.implicitWidth + 12
                                height: 20
                                radius: 10
                                color: noteStore.activeFolder === modelData.path ? Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.16) : theme.chip
                                border.width: 1
                                border.color: noteStore.activeFolder === modelData.path ? theme.accent : theme.border
                                Text { id: folderLabel; anchors.centerIn: parent; text: (modelData.path === "" ? qsTr("Racine") : modelData.name) + " " + modelData.count; color: theme.mutedText; font.family: backend.monoFont; font.pixelSize: 8 }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: noteStore.activeFolder = noteStore.activeFolder === modelData.path ? "" : modelData.path }
                            }
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 4
                        Repeater {
                            model: [
                                { label: qsTr("Toutes"), value: "" },
                                { label: qsTr("À faire"), value: "todo" },
                                { label: qsTr("En cours"), value: "doing" },
                                { label: qsTr("Terminé"), value: "done" }
                            ]
                            Rectangle {
                                width: statusLabel.implicitWidth + 12
                                height: 20
                                radius: 10
                                color: noteStore.activeStatus === modelData.value ? Qt.rgba(theme.yellow.r, theme.yellow.g, theme.yellow.b, 0.16) : theme.chip
                                border.width: 1
                                border.color: noteStore.activeStatus === modelData.value ? theme.yellow : theme.border
                                Text { id: statusLabel; anchors.centerIn: parent; text: modelData.label; color: theme.mutedText; font.family: backend.monoFont; font.pixelSize: 8 }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: noteStore.activeStatus = modelData.value }
                            }
                        }
                    }

                    ListView {
                        id: noteList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 2
                        model: noteStore.filteredNotes
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: AppScrollBar {}

                        delegate: Rectangle {
                            // Reserve the scrollbar gutter: an attached ScrollBar overlays the
                            // delegate, which used to clip the note list on the right edge.
                            width: ListView.view.width - design.space2
                            height: 70
                            radius: design.radiusSm
                            color: modelData.path === noteStore.selectedPath
                                   ? theme.raised
                                   : (noteMouse.containsMouse ? theme.chip : "transparent")
                            border.width: modelData.path === noteStore.selectedPath ? 1 : 0
                            border.color: theme.borderStrong

                            MouseArea {
                                id: noteMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: noteStore.selectNote(modelData.path)
                            }

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: design.space2
                                spacing: 2
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    ThemedIcon { visible: modelData.pinned; source: "icons/pin.svg"; width: 9; height: 9; color: theme.yellow }
                                    Text { Layout.fillWidth: true; text: modelData.title; color: theme.text; font.pixelSize: design.textCaption; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                    Rectangle {
                                        visible: modelData.status !== undefined && modelData.status !== ""
                                        width: 6; height: 6; radius: 3
                                        color: modelData.status === "done" ? theme.green
                                               : (modelData.status === "doing" ? theme.orange : theme.yellow)
                                    }
                                    Text { text: new Date(modelData.updatedAt).toLocaleDateString(Qt.locale(), "dd/MM"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                                }
                                Text { Layout.fillWidth: true; text: modelData.excerpt !== "" ? modelData.excerpt : qsTr("Note vide"); color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                                Text { Layout.fillWidth: true; text: modelData.path; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8; elide: Text.ElideMiddle }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.compactRail ? noteStore.vaultPath : qsTr("%n note(s) · Markdown local", "", noteStore.count)
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: 8
                        elide: Text.ElideMiddle
                    }
                }
            }

            // ── editor ───────────────────────────────────────────────────
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

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        color: theme.surface
                        radius: design.radiusLg
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: theme.border }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: design.space3
                            anchors.rightMargin: design.space2
                            spacing: design.space2
                            SettingsField {
                                id: titleField
                                Layout.fillWidth: true
                                placeholderText: ""
                                font.pixelSize: design.textBody
                                font.weight: Font.DemiBold
                                enabled: root.hasNote
                                onEditingFinished: {
                                    if (root.hasNote && text.trim() !== "" && text !== root.note.title)
                                        noteStore.renameNote(noteStore.selectedPath, text)
                                }
                            }
                            Text { visible: root.dirty; text: qsTr("non enregistré"); color: theme.yellow; font.pixelSize: 9 }
                            IconButton { icon: "icons/pin.svg"; hint: qsTr("Épingler"); active: root.note.pinned === true; enabled: root.hasNote; onClicked: root.togglePin() }
                            IconButton { icon: "icons/file.svg"; hint: qsTr("Insérer une image"); enabled: root.hasNote; onClicked: root.insertAttachment() }
                            IconButton {
                                icon: root.previewMode ? "icons/pencil.svg" : "icons/eye.svg"
                                hint: root.previewMode ? qsTr("Éditer") : qsTr("Aperçu")
                                enabled: root.hasNote
                                onClicked: root.previewMode = !root.previewMode
                            }
                            IconButton { icon: "icons/copy.svg"; hint: qsTr("Dupliquer la note"); enabled: root.hasNote; onClicked: noteStore.duplicateNote(noteStore.selectedPath) }
                            IconButton { icon: "icons/close.svg"; hint: qsTr("Supprimer la note"); enabled: root.hasNote; onClicked: root.openDelete(noteStore.selectedPath, root.note.title) }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: root.hasNote

                        TextArea {
                            id: editor
                            anchors.fill: parent
                            anchors.margins: design.space4
                            visible: !root.previewMode
                            color: theme.text
                            font.family: backend.monoFont
                            font.pixelSize: design.textCaption
                            wrapMode: TextArea.Wrap
                            selectByMouse: true
                            background: null
                            onTextChanged: {
                                if (root.loading || !root.hasNote)
                                    return
                                root.dirty = true
                                autosave.restart()
                            }
                            Keys.onPressed: function (event) {
                                if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_S) {
                                    root.saveNow(); event.accepted = true
                                }
                            }
                        }

                        ScrollView {
                            anchors.fill: parent
                            visible: root.previewMode
                            clip: true
                            contentWidth: availableWidth
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            ScrollBar.vertical: AppScrollBar {}
                            Text {
                                width: parent.width - design.space6
                                x: design.space4
                                y: design.space4
                                text: root.renderMarkdown(root.note.content !== undefined ? root.note.content : "")
                                color: theme.text
                                font.pixelSize: design.textBody
                                lineHeight: 1.42
                                wrapMode: Text.Wrap
                                textFormat: Text.MarkdownText
                                onLinkActivated: function (link) {
                                    if (link.indexOf("wiki:") === 0) {
                                        noteStore.openWikiLink(decodeURIComponent(link.substring(5)))
                                    } else {
                                        Qt.openUrlExternally(link)
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: !root.hasNote
                        Column {
                            anchors.centerIn: parent
                            spacing: design.space2
                            BrandMark { width: 48; height: 48; size: 48; anchors.horizontalCenter: parent.horizontalCenter; accent: false }
                            Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Sélectionnez une note ou créez-en une."); color: theme.mutedText; font.pixelSize: design.textCaption }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        color: theme.surface
                        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: theme.border }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: design.space3
                            anchors.rightMargin: design.space3
                            spacing: design.space2
                            Text { text: root.hasNote ? qsTr("%n mot(s)", "", root.note.wordCount !== undefined ? root.note.wordCount : 0) : qsTr("Markdown"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                            Item { Layout.fillWidth: true }
                            Text { text: root.hasNote ? qsTr("Ctrl+S pour enregistrer") : ""; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                        }
                    }
                }
            }

            // ── context rail ─────────────────────────────────────────────
            Rectangle {
                visible: root.wide
                Layout.preferredWidth: root.rightWidth
                Layout.fillHeight: true
                radius: design.radiusLg
                color: theme.surface
                border.width: 1
                border.color: theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: design.space3
                    spacing: design.space3
                    Text { text: qsTr("Contexte"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; font.letterSpacing: 0.8 }
                    Repeater {
                        model: [
                            { label: qsTr("Chemin"), value: noteStore.selectedPath !== "" ? noteStore.selectedPath : "—" },
                            { label: qsTr("Mots"), value: root.note.wordCount !== undefined ? String(root.note.wordCount) : "—" },
                            { label: qsTr("Modifié"), value: root.hasNote && root.note.updatedAt !== undefined ? Qt.formatDateTime(new Date(root.note.updatedAt), "dd/MM HH:mm") : "—" }
                        ]
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Text { text: modelData.label; color: theme.dimText; font.pixelSize: design.textMicro }
                            Text { Layout.fillWidth: true; text: modelData.value; color: theme.text; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideMiddle }
                        }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }
                    Text { text: qsTr("Tags"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 4
                        Repeater {
                            model: root.note.tags !== undefined ? root.note.tags : []
                            Rectangle {
                                width: contextTag.implicitWidth + 10
                                height: 18
                                radius: 9
                                color: theme.chip
                                Text { id: contextTag; anchors.centerIn: parent; text: "#" + modelData; color: theme.mutedText; font.family: backend.monoFont; font.pixelSize: 8 }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: noteStore.activeTag = modelData }
                            }
                        }
                        Text { visible: root.note.tags === undefined || root.note.tags.length === 0; text: qsTr("Aucun"); color: theme.dimText; font.pixelSize: 9 }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }
                    Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }
                    Text { text: qsTr("Sommaire"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                    Repeater {
                        model: noteStore.headings.slice(0, 8)
                        Text {
                            Layout.fillWidth: true
                            Layout.leftMargin: (modelData.level - 1) * 8
                            text: modelData.text
                            color: theme.mutedText
                            font.pixelSize: design.textMicro
                            elide: Text.ElideRight
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    editor.cursorPosition = root.offsetForLine(modelData.line)
                                    editor.forceActiveFocus()
                                }
                            }
                        }
                    }
                    Text { Layout.fillWidth: true; visible: noteStore.headings.length === 0; text: qsTr("Aucun titre. Utilisez # Titre."); color: theme.dimText; font.pixelSize: 9 }
                    Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.fillWidth: true; text: qsTr("Liens sortants"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: noteStore.outgoingLinks.length; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                    }
                    Repeater {
                        model: noteStore.outgoingLinks.slice(0, 6)
                        Text {
                            Layout.fillWidth: true
                            text: (modelData.exists ? "→ " : "? ") + modelData.title
                            color: modelData.exists ? theme.accent : theme.dimText
                            font.pixelSize: design.textMicro
                            elide: Text.ElideRight
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (modelData.exists)
                                        noteStore.selectNote(modelData.path)
                                    else
                                        noteStore.openWikiLink(modelData.target)
                                }
                            }
                        }
                    }
                    Text { Layout.fillWidth: true; visible: noteStore.outgoingLinks.length === 0; text: qsTr("Aucun lien. Reliez avec [[Titre]]."); color: theme.dimText; font.pixelSize: 9 }

                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.fillWidth: true; text: qsTr("Backlinks"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        Text { text: noteStore.backlinks.length; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                    }
                    Repeater {
                        model: noteStore.backlinks
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: backlinkColumn.implicitHeight + 12
                            radius: design.radiusSm
                            color: backlinkMouse.containsMouse ? theme.chip : theme.background
                            border.width: 1
                            border.color: theme.border
                            ColumnLayout {
                                id: backlinkColumn
                                anchors.fill: parent
                                anchors.margins: 6
                                spacing: 1
                                Text { Layout.fillWidth: true; text: modelData.title; color: theme.text; font.pixelSize: design.textMicro; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                Text { Layout.fillWidth: true; text: modelData.excerpt; color: theme.dimText; font.pixelSize: 8; elide: Text.ElideRight }
                            }
                            MouseArea { id: backlinkMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: noteStore.selectNote(modelData.path) }
                        }
                    }
                    Text { Layout.fillWidth: true; visible: noteStore.backlinks.length === 0; text: qsTr("Aucun backlink. Reliez vos notes avec [[Titre]]."); color: theme.dimText; font.pixelSize: 9; wrapMode: Text.Wrap }
                    Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }
                    Text { text: qsTr("Harness"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                    Text { Layout.fillWidth: true; text: backend.workspaceName; color: theme.mutedText; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideMiddle }
                    Text { Layout.fillWidth: true; text: ultimateAgent.modelLabel; color: theme.dimText; font.pixelSize: 9 }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        GhostButton { text: qsTr("À faire"); flat: true; font.pixelSize: 8; enabled: root.hasNote; onClicked: root.setTaskStatus("todo") }
                        GhostButton { text: qsTr("En cours"); flat: true; font.pixelSize: 8; enabled: root.hasNote; onClicked: root.setTaskStatus("doing") }
                        GhostButton { text: qsTr("Terminé"); flat: true; font.pixelSize: 8; enabled: root.hasNote; onClicked: root.setTaskStatus("done") }
                    }
                    Item { Layout.fillHeight: true }
                    ActionButton { Layout.fillWidth: true; text: qsTr("Confier à l'agent"); enabled: root.hasNote; onClicked: root.delegateNote() }
                    ActionButton { Layout.fillWidth: true; text: qsTr("Ouvrir le graphe"); onClicked: graphDialog.open() }
                }
            }
        }
    }

    Dialog {
        id: backupDialog
        modal: true
        width: Math.min(root.width - design.space6, 520)
        height: 300
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Sauvegarde du vault"); color: theme.brightText; font.pixelSize: design.textBodyLarge; font.weight: Font.DemiBold }
                IconButton { icon: "icons/close.svg"; hint: qsTr("Fermer"); onClicked: backupDialog.close() }
            }
            Text { Layout.fillWidth: true; text: noteStore.vaultPath; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideMiddle }
            Text {
                Layout.fillWidth: true
                text: qsTr("Archive tar.gz dans .backups, ou export Markdown avec manifeste JSON.")
                color: theme.dimText
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }
            RowLayout {
                Layout.fillWidth: true
                ActionButton {
                    text: qsTr("Sauvegarder maintenant")
                    onClicked: {
                        const path = noteStore.backupVault()
                        backupStatus.text = path !== "" ? qsTr("Archive : %1").arg(path) : noteStore.error
                    }
                }
                GhostButton { text: qsTr("Exporter…"); flat: true; onClicked: exportFolderDialog.open() }
                GhostButton {
                    text: qsTr("Purger (garder 10)")
                    flat: true
                    onClicked: {
                        const removed = noteStore.pruneBackups(10)
                        backupStatus.text = qsTr("%1 ancienne(s) archive(s) supprimée(s).").arg(removed)
                    }
                }
                GhostButton { text: qsTr("Ouvrir .backups"); flat: true; onClicked: Qt.openUrlExternally("file://" + noteStore.vaultPath + "/.backups") }
            }
            Text {
                id: backupStatus
                Layout.fillWidth: true
                color: theme.dimText
                font.family: backend.monoFont
                font.pixelSize: design.textMicro
                wrapMode: Text.Wrap
            }
            Item { Layout.fillHeight: true }
        }
    }

    FolderDialog {
        id: exportFolderDialog
        title: qsTr("Exporter le vault")
        onAccepted: {
            const path = noteStore.exportVault(selectedFolder)
            backupStatus.text = path !== "" ? qsTr("Export : %1").arg(path) : noteStore.error
            backupDialog.open()
        }
    }

    Dialog {
        id: gitDialog
        modal: true
        width: Math.min(root.width - design.space6, 560)
        height: 430
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Synchronisation Git"); color: theme.brightText; font.pixelSize: design.textBodyLarge; font.weight: Font.DemiBold }
                StatusDot { size: 5; color: noteStore.gitAvailable ? (noteStore.gitDirty ? theme.orange : theme.green) : theme.dimText }
                IconButton { icon: "icons/close.svg"; hint: qsTr("Fermer"); onClicked: gitDialog.close() }
            }
            Text { Layout.fillWidth: true; text: noteStore.vaultPath; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideMiddle }
            Text {
                Layout.fillWidth: true
                text: qsTr("Branche : %1 · %2 · %3")
                      .arg(noteStore.gitBranch !== "" ? noteStore.gitBranch : qsTr("aucune"))
                      .arg(noteStore.gitDirty ? qsTr("modifications") : qsTr("propre"))
                      .arg(noteStore.gitLastCommit !== "" ? noteStore.gitLastCommit : qsTr("aucun commit"))
                color: theme.mutedText
                font.family: backend.monoFont
                font.pixelSize: design.textMicro
                wrapMode: Text.Wrap
            }
            SettingsField {
                id: remoteField
                Layout.fillWidth: true
                placeholderText: qsTr("Dépôt distant (https:// ou git@)")
                text: noteStore.gitRemote
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                SettingsField {
                    id: tokenField
                    Layout.fillWidth: true
                    placeholderText: noteStore.gitTokenConfigured ? qsTr("Jeton configuré (laisser vide pour conserver)") : qsTr("Jeton Git (HTTPS)")
                    echoMode: TextInput.Password
                }
                GhostButton {
                    text: qsTr("Enregistrer le jeton")
                    flat: true
                    onClicked: {
                        if (tokenField.text !== "") {
                            noteStore.gitSetToken(tokenField.text)
                            tokenField.text = ""
                        }
                    }
                }
                GhostButton {
                    text: qsTr("Effacer")
                    flat: true
                    enabled: noteStore.gitTokenConfigured
                    onClicked: noteStore.gitClearToken()
                }
            }
            Text {
                Layout.fillWidth: true
                visible: noteStore.gitConflicts.length > 0
                text: qsTr("Conflits : choisissez le côté à garder, puis synchronisez à nouveau.")
                color: theme.orange
                font.pixelSize: design.textMicro
                wrapMode: Text.Wrap
            }
            Repeater {
                model: noteStore.gitConflicts
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text { Layout.fillWidth: true; text: modelData; color: theme.text; font.family: backend.monoFont; font.pixelSize: 8; elide: Text.ElideMiddle }
                    GhostButton { text: qsTr("Local"); flat: true; font.pixelSize: 8; onClicked: noteStore.gitResolve(modelData, "ours") }
                    GhostButton { text: qsTr("Distant"); flat: true; font.pixelSize: 8; onClicked: noteStore.gitResolve(modelData, "theirs") }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                visible: noteStore.gitConflicts.length > 0
                GhostButton { text: qsTr("Continuer"); flat: true; font.pixelSize: 8; enabled: noteStore.gitConflicts.length === 0; onClicked: noteStore.gitContinue() }
                GhostButton { text: qsTr("Abandonner la fusion"); flat: true; font.pixelSize: 8; onClicked: noteStore.gitAbort() }
                Item { Layout.fillWidth: true }
            }
            RowLayout {
                Layout.fillWidth: true
                GhostButton { text: qsTr("Init"); flat: true; onClicked: { noteStore.gitInit(); noteStore.gitRefresh() } }
                GhostButton { text: qsTr("Enregistrer le dépôt"); flat: true; onClicked: { noteStore.gitSetRemote(remoteField.text); noteStore.gitRefresh() } }
                Item { Layout.fillWidth: true }
                GhostButton { text: qsTr("Tirer"); flat: true; onClicked: { noteStore.gitPull(); noteStore.gitRefresh() } }
                GhostButton { text: qsTr("Pousser"); flat: true; onClicked: { noteStore.gitPush(); noteStore.gitRefresh() } }
                ActionButton { text: qsTr("Synchroniser"); onClicked: { noteStore.gitSync(); noteStore.gitRefresh() } }
            }
            Text {
                Layout.fillWidth: true
                text: noteStore.error
                color: theme.red
                font.pixelSize: design.textMicro
                wrapMode: Text.Wrap
                visible: noteStore.error !== ""
            }
            Item { Layout.fillHeight: true }
        }
        onOpened: { noteStore.gitRefresh(); remoteField.text = noteStore.gitRemote }
    }

    Dialog {
        id: notePluginsDialog
        modal: true
        width: Math.min(root.width - design.space6, 620)
        height: Math.min(root.height - design.space6, 560)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Plugins de notes"); color: theme.brightText; font.pixelSize: design.textBodyLarge; font.weight: Font.DemiBold }
                Text { text: notePlugins.plugins.length + qsTr(" plugins"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                IconButton { icon: "icons/close.svg"; hint: qsTr("Fermer"); onClicked: notePluginsDialog.close() }
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Actions intégrées au harness et commandes externes dans ~/.config/Crasy/deepcrasy-h/note-plugins/.")
                color: theme.dimText
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: notePlugins.plugins
                spacing: 6
                section.property: "category"
                section.delegate: Text {
                    width: ListView.view.width
                    height: 24
                    text: section
                    color: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: design.textMicro
                    verticalAlignment: Text.AlignBottom
                }
                delegate: Rectangle {
                    width: ListView.view.width
                    height: pluginColumn.implicitHeight + 14
                    radius: design.radiusSm
                    color: theme.background
                    border.width: 1
                    border.color: theme.border
                    ColumnLayout {
                        id: pluginColumn
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 3
                        RowLayout {
                            Layout.fillWidth: true
                            Text { Layout.fillWidth: true; text: modelData.name; color: theme.text; font.pixelSize: design.textCaption; font.weight: Font.DemiBold; elide: Text.ElideRight }
                            Text { text: (modelData.category !== undefined ? modelData.category + " · " : "") + (modelData.builtin ? qsTr("intégré") : qsTr("externe")); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                            CheckBox {
                                text: qsTr("Actif")
                                checked: modelData.enabled
                                onToggled: notePlugins.setEnabled(modelData.id, checked)
                            }
                        }
                        Text { Layout.fillWidth: true; text: modelData.description; color: theme.dimText; font.pixelSize: design.textMicro; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            GhostButton {
                                text: qsTr("Exécuter")
                                flat: true
                                font.pixelSize: 8
                                enabled: modelData.enabled && root.hasNote
                                onClicked: {
                                    const result = notePlugins.run(modelData.id, noteStore.selectedPath)
                                    if (result.ok)
                                        noteStore.refresh()
                                }
                            }
                            GhostButton {
                                text: qsTr("Appliquer à %1").arg(noteStore.filteredNotes.length)
                                flat: true
                                font.pixelSize: 8
                                visible: noteStore.filteredNotes.length > 1
                                enabled: modelData.enabled && !modelData.readOnly
                                onClicked: {
                                    notePlugins.runBatch(modelData.id, root.filteredPaths())
                                    noteStore.refresh()
                                }
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                visible: !modelData.builtin
                                text: modelData.sandbox ? qsTr("sandbox") : qsTr("sans sandbox")
                                color: theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: 8
                            }
                            ComboBox {
                                Layout.preferredWidth: 116
                                Layout.preferredHeight: 26
                                font.pixelSize: 9
                                model: [qsTr("Manuel"), "15 min", "1 h", "6 h", "24 h"]
                                currentIndex: modelData.scheduleMinutes === 15 ? 1
                                              : (modelData.scheduleMinutes === 60 ? 2
                                              : (modelData.scheduleMinutes === 360 ? 3
                                              : (modelData.scheduleMinutes === 1440 ? 4 : 0)))
                                onActivated: notePlugins.setSchedule(modelData.id, [0, 15, 60, 360, 1440][index])
                            }
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                GhostButton {
                    text: qsTr("Annuler")
                    flat: true
                    font.pixelSize: 8
                    enabled: notePlugins.canUndo
                    onClicked: {
                        if (notePlugins.undoLast())
                            noteStore.refresh()
                    }
                }
                GhostButton {
                    text: qsTr("Rétablir")
                    flat: true
                    font.pixelSize: 8
                    enabled: notePlugins.canRedo
                    onClicked: {
                        if (notePlugins.redoLast())
                            noteStore.refresh()
                    }
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: notePlugins.vaults.length + qsTr(" vault(s) planifié(s)")
                    color: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: 8
                }
            }
            Text {
                Layout.fillWidth: true
                visible: notePlugins.error !== "" || notePlugins.lastOutput !== ""
                text: notePlugins.error !== "" ? notePlugins.error : notePlugins.lastOutput
                color: notePlugins.error !== "" ? theme.red : theme.dimText
                font.family: backend.monoFont
                font.pixelSize: design.textMicro
                wrapMode: Text.Wrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
        }
    }

    Dialog {
        id: templatesDialog
        modal: true
        width: Math.min(root.width - design.space6, 460)
        height: Math.min(root.height - design.space6, 420)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Nouvelle note depuis un modèle"); color: theme.brightText; font.pixelSize: design.textBodyLarge; font.weight: Font.DemiBold }
                IconButton { icon: "icons/close.svg"; hint: qsTr("Fermer"); onClicked: templatesDialog.close() }
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Placez vos modèles dans Templates/. {{title}} et {{date}} sont remplacés.")
                color: theme.dimText
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: noteStore.templates()
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 36
                    radius: design.radiusSm
                    color: templateMouse.containsMouse ? theme.chip : theme.background
                    border.width: 1
                    border.color: theme.border
                    Text { anchors.fill: parent; anchors.leftMargin: 10; verticalAlignment: Text.AlignVCenter; text: modelData.name; color: theme.text; font.pixelSize: design.textCaption }
                    MouseArea {
                        id: templateMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            noteStore.createFromTemplate(modelData.path, modelData.name)
                            templatesDialog.close()
                            Qt.callLater(function () { editor.forceActiveFocus() })
                        }
                    }
                }
            }
            Text { Layout.fillWidth: true; visible: noteStore.templates().length === 0; text: qsTr("Aucun modèle pour l'instant."); color: theme.dimText; font.pixelSize: design.textCaption }
        }
    }

    FileDialog {
        id: imageDialog
        title: qsTr("Insérer une image")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.webp *.gif)")]
        onAccepted: {
            const markdown = noteStore.importAttachment(selectedFile)
            if (markdown !== "") {
                editor.insert(editor.cursorPosition, markdown + "\n")
                root.dirty = true
                autosave.restart()
            }
        }
    }

    Dialog {
        id: graphDialog
        modal: true
        width: Math.min(root.width - design.space6, 1000)
        height: Math.min(root.height - design.space6, 760)
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        onClosed: noteStore.graphOpen = false
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            RowLayout {
                Layout.fillWidth: true
                Text { Layout.fillWidth: true; text: qsTr("Graphe des notes"); color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
                Text { text: qsTr("%n note(s) · %1 lien(s)", "", noteStore.count).arg(noteStore.graph.edges !== undefined ? noteStore.graph.edges.length : 0); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                IconButton { icon: "icons/reload.svg"; hint: qsTr("Rafraîchir"); onClicked: noteStore.refresh() }
                IconButton { icon: "icons/close.svg"; hint: qsTr("Fermer"); onClicked: graphDialog.close() }
            }
            NoteGraph {
                Layout.fillWidth: true
                Layout.fillHeight: true
                graphData: noteStore.graph
                selectedPath: noteStore.selectedPath
                onNodeActivated: function (path) { noteStore.selectNote(path); graphDialog.close() }
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
            Text { text: qsTr("Déplacer la note vers la corbeille ?"); color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
            Text { Layout.fillWidth: true; text: qsTr("« %1 » sera déplacée dans la corbeille du vault.").arg(root.deleteTitle); color: theme.mutedText; font.pixelSize: design.textCaption; wrapMode: Text.Wrap }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GhostButton { text: qsTr("Annuler"); flat: true; onClicked: deleteDialog.close() }
                ActionButton { text: qsTr("Supprimer"); danger: true; onClicked: { noteStore.deleteNote(root.deletePath); deleteDialog.close() } }
            }
        }
    }
}
