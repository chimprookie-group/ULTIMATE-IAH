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
                            width: ListView.view.width
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
                    Item { Layout.fillHeight: true }
                    ActionButton { Layout.fillWidth: true; text: qsTr("Ouvrir le graphe"); onClicked: graphDialog.open() }
                }
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
