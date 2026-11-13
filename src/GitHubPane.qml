import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(1180, width - design.space7))
    readonly property var selected: gitHub.selectedRepository
    readonly property bool wide: width >= 1050
    property string query: ""
    property bool showPulls: false
    property var filteredRepos: {
        var result = []
        const needle = query.trim().toLowerCase()
        for (var i = 0; i < gitHub.repositories.length; ++i) {
            const repo = gitHub.repositories[i]
            if (needle === "" || (repo.fullName + " " + repo.description).toLowerCase().indexOf(needle) >= 0)
                result.push(repo)
        }
        return result
    }

    function cloneSelected() {
        if (selected.fullName === undefined) return
        if (gitHub.cloneRepository(selected.fullName))
            Window.window.showToast(qsTr("Dépôt cloné dans le workspace."))
    }

    function createIssue() {
        if (issueTitle.text.trim() === "") return
        if (gitHub.createIssue(issueTitle.text, issueBody.text)) {
            issueTitle.text = ""
            issueBody.text = ""
            issueDialog.close()
            Window.window.showToast(qsTr("Issue créée."))
        }
    }

    function createRepository() {
        if (repoName.text.trim() === "") return
        const result = gitHub.createRepository(repoName.text, repoDescription.text, privateSwitch.checked, cloneSwitch.checked)
        if (result.ok === true) {
            repoDialog.close()
            createName.text = ""
            Window.window.showToast(qsTr("Projet GitHub créé."))
        }
    }

    Component.onCompleted: gitHub.refresh()
    onVisibleChanged: if (visible) gitHub.refresh()

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            width: root.contentWidth
            x: Math.max(design.space5, (parent.width - width) / 2)
            spacing: design.space4

            Item { Layout.fillWidth: true; Layout.preferredHeight: design.space4 }

            PageHeader {
                Layout.fillWidth: true
                title: qsTr("GitHub")
                description: qsTr("Connexion, dépôts, clonage, issues et pull requests, directement dans le harness.")
                icon: "icons/github.svg"
                accent: theme.blue
                ActionButton { text: qsTr("Rafraîchir"); onClicked: gitHub.refresh() }
                ActionButton { text: qsTr("Nouveau projet"); onClicked: repoDialog.open() }
            }

            Text {
                Layout.fillWidth: true
                visible: gitHub.error !== ""
                text: gitHub.error
                color: theme.red
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: connectionColumn.implicitHeight + design.space4
                radius: design.radiusMd
                color: theme.surface
                border.width: 1
                border.color: gitHub.authenticated ? theme.border : theme.orange
                ColumnLayout {
                    id: connectionColumn
                    anchors.fill: parent
                    anchors.margins: design.space3
                    spacing: design.space1
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: design.space2
                        StatusDot { size: 6; color: gitHub.authenticated ? theme.green : theme.orange }
                        Text {
                            Layout.fillWidth: true
                            text: gitHub.authenticated
                                  ? qsTr("Connecté · %1 (%2)").arg(gitHub.login).arg(gitHub.accountName)
                                  : qsTr("GitHub CLI non connecté")
                            color: theme.text
                            font.pixelSize: design.textLabel
                            font.weight: Font.DemiBold
                        }
                        Text { text: gitHub.scopes; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro; elide: Text.ElideRight; Layout.maximumWidth: root.contentWidth * 0.35 }
                        Button {
                            visible: !gitHub.authenticated
                            text: qsTr("Se connecter")
                            font.pixelSize: design.textMicro
                            onClicked: { backend.setView("terminal"); terminal.run("gh auth login --web") }
                        }
                        GhostButton { text: qsTr("Workspace"); flat: true; font.pixelSize: design.textMicro; onClicked: gitHub.openWorkspace() }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: gitHub.repositories.length + qsTr(" dépôt(s) · ") + gitHub.issues.length + qsTr(" issue(s) · ") + gitHub.pulls.length + qsTr(" pull request(s)")
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: design.textMicro
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: design.space3

                Rectangle {
                    visible: root.wide
                    Layout.preferredWidth: 330
                    Layout.fillHeight: true
                    implicitHeight: Math.max(360, root.height - 300)
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
                            Text { text: qsTr("Dépôts"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                            Item { Layout.fillWidth: true }
                            Text { text: root.filteredRepos.length; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                        }
                        SettingsField {
                            Layout.fillWidth: true
                            placeholderText: qsTr("Filtrer les dépôts…")
                            onTextChanged: root.query = text
                        }
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            contentWidth: availableWidth
                            ScrollBar.vertical: AppScrollBar {}
                            Column {
                                width: parent.width
                                spacing: 2
                                Repeater {
                                    model: root.filteredRepos
                                    Rectangle {
                                        width: parent.width
                                        height: 58
                                        radius: design.radiusSm
                                        color: modelData.fullName === gitHub.selectedRepo ? theme.raised : (repoMouse.containsMouse ? theme.chip : "transparent")
                                        border.width: modelData.fullName === gitHub.selectedRepo ? 1 : 0
                                        border.color: theme.borderStrong
                                        MouseArea { id: repoMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: gitHub.selectRepository(modelData.fullName) }
                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: design.space2
                                            spacing: 1
                                            RowLayout {
                                                Layout.fillWidth: true
                                                Text { Layout.fillWidth: true; text: modelData.name; color: theme.text; font.pixelSize: design.textCaption; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                                Text { text: modelData.private ? qsTr("privé") : qsTr("public"); color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                                            }
                                            Text { Layout.fillWidth: true; text: modelData.description !== "" ? modelData.description : qsTr("Sans description"); color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                                            RowLayout {
                                                Layout.fillWidth: true
                                                Text { Layout.fillWidth: true; text: modelData.language !== "" ? modelData.language : "—"; color: theme.mutedText; font.family: backend.monoFont; font.pixelSize: 8 }
                                                Text { text: "★ " + modelData.stars; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                                            }
                                        }
                                    }
                                }
                                Text {
                                    width: parent.width
                                    visible: root.filteredRepos.length === 0
                                    text: gitHub.authenticated ? qsTr("Aucun dépôt.") : qsTr("Connectez gh pour voir vos dépôts.")
                                    color: theme.dimText
                                    font.pixelSize: design.textCaption
                                    wrapMode: Text.Wrap
                                    horizontalAlignment: Text.AlignHCenter
                                    topPadding: design.space5
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    implicitHeight: Math.max(360, root.height - 300)
                    radius: design.radiusLg
                    color: theme.background
                    border.width: 1
                    border.color: theme.border
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 70
                            color: theme.surface
                            radius: design.radiusLg
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: design.space3
                                spacing: 1
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { Layout.fillWidth: true; text: selected.name !== undefined ? selected.fullName : qsTr("Sélectionnez un dépôt"); color: theme.brightText; font.pixelSize: design.textBody; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                    Text { visible: selected.visibility !== undefined; text: selected.visibility !== undefined ? selected.visibility : ""; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                                }
                                Text { Layout.fillWidth: true; text: selected.description !== undefined && selected.description !== "" ? selected.description : qsTr("Aucun dépôt sélectionné."); color: theme.dimText; font.pixelSize: design.textMicro; elide: Text.ElideRight }
                            }
                        }
                        Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: design.space3
                            Layout.rightMargin: design.space3
                            Layout.topMargin: design.space2
                            spacing: design.space2
                            ComboBox {
                                visible: !root.wide
                                Layout.preferredWidth: root.wide ? 190 : 150
                                model: gitHub.repositories
                                textRole: "fullName"
                                font.pixelSize: design.textMicro
                                onActivated: function (index) { gitHub.selectRepository(gitHub.repositories[index].fullName) }
                            }
                            GhostButton { text: qsTr("Ouvrir"); flat: true; font.pixelSize: design.textMicro; enabled: selected.url !== undefined; onClicked: gitHub.openRepository(selected.url) }
                            GhostButton { text: qsTr("Cloner"); flat: true; font.pixelSize: design.textMicro; enabled: selected.fullName !== undefined; onClicked: root.cloneSelected() }
                            GhostButton { text: root.wide ? qsTr("Nouvelle issue") : qsTr("Issue"); flat: true; font.pixelSize: design.textMicro; enabled: selected.fullName !== undefined; onClicked: issueDialog.open() }
                            Item { Layout.fillWidth: true }
                            Button { text: qsTr("Issues"); flat: !root.showPulls; font.pixelSize: design.textMicro; onClicked: root.showPulls = false }
                            Button { text: root.wide ? qsTr("Pull requests") : "PR"; flat: root.showPulls; font.pixelSize: design.textMicro; onClicked: root.showPulls = true }
                        }
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            contentWidth: availableWidth
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            ScrollBar.vertical: AppScrollBar {}
                            Column {
                                width: parent.width
                                spacing: 2
                                Repeater {
                                    model: root.showPulls ? gitHub.pulls : gitHub.issues
                                    Rectangle {
                                        width: parent.width
                                        height: 62
                                        color: issueMouse.containsMouse ? theme.chip : "transparent"
                                        border.width: 1
                                        border.color: theme.border
                                        radius: design.radiusSm
                                        MouseArea { id: issueMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: gitHub.openRepository(modelData.url) }
                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: design.space2
                                            spacing: 1
                                            RowLayout {
                                                Layout.fillWidth: true
                                                Text { text: "#" + modelData.number; color: theme.accent; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                                                Text { Layout.fillWidth: true; text: modelData.title; color: theme.text; font.pixelSize: design.textCaption; elide: Text.ElideRight }
                                                Text { text: root.showPulls ? (modelData.draft ? qsTr("brouillon") : modelData.state) : modelData.state; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8 }
                                            }
                                            Text { Layout.fillWidth: true; text: root.showPulls ? modelData.branch : modelData.author; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: 8; elide: Text.ElideRight }
                                        }
                                    }
                                }
                                Text {
                                    width: parent.width
                                    visible: (root.showPulls ? gitHub.pulls.length : gitHub.issues.length) === 0
                                    text: root.showPulls ? qsTr("Aucune pull request.") : qsTr("Aucune issue.")
                                    color: theme.dimText
                                    font.pixelSize: design.textCaption
                                    horizontalAlignment: Text.AlignHCenter
                                    topPadding: design.space5
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: repoDialog
        modal: true
        width: Math.min(520, root.width - design.space5)
        height: 340
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            Text { text: qsTr("Nouveau projet GitHub"); color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
            SettingsField { id: repoName; Layout.fillWidth: true; placeholderText: qsTr("nom-du-depot") }
            SettingsField { id: repoDescription; Layout.fillWidth: true; placeholderText: qsTr("Description") }
            RowLayout {
                Layout.fillWidth: true
                CheckBox { id: privateSwitch; text: qsTr("Privé"); checked: true }
                CheckBox { id: cloneSwitch; text: qsTr("Cloner dans le workspace"); checked: true }
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GhostButton { text: qsTr("Annuler"); flat: true; onClicked: repoDialog.close() }
                ActionButton { text: qsTr("Créer"); enabled: repoName.text.trim() !== ""; onClicked: root.createRepository() }
            }
        }
    }

    Dialog {
        id: issueDialog
        modal: true
        width: Math.min(560, root.width - design.space5)
        height: 390
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        padding: design.space4
        background: Rectangle { radius: design.radiusLg; color: theme.surface; border.width: 1; border.color: theme.borderStrong }
        contentItem: ColumnLayout {
            spacing: design.space2
            Text { text: qsTr("Nouvelle issue"); color: theme.brightText; font.pixelSize: design.textTitle; font.weight: Font.DemiBold }
            SettingsField { id: issueTitle; Layout.fillWidth: true; placeholderText: qsTr("Titre") }
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: theme.background
                border.width: 1
                border.color: issueBody.activeFocus ? theme.accent : theme.border
                radius: design.radiusSm
                TextArea { id: issueBody; anchors.fill: parent; anchors.margins: 6; color: theme.text; font.pixelSize: design.textCaption; wrapMode: TextArea.Wrap; background: null }
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GhostButton { text: qsTr("Annuler"); flat: true; onClicked: issueDialog.close() }
                ActionButton { text: qsTr("Créer"); enabled: issueTitle.text.trim() !== ""; onClicked: root.createIssue() }
            }
        }
    }
}
