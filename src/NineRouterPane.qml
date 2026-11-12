import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtWebEngine

Item {
    id: root

    property bool integrationOpen: false
    property string query: ""
    property var selectedModels: []
    property bool makeDefault: true
    property string password: ""
    property bool showPassword: false
    property bool showHint: true
    property bool armed: false
    property var profile: null
    readonly property var hostWindow: Window.window
    readonly property var web: dashboardLoader.item

    function ensureProfile() {
        if (!profile) {
            profile = profileComponent.createObject(root)
        }
    }

    function openLink(url) {
        const value = String(url)
        if (value === "" || value.startsWith("about:"))
            return
        if (value.indexOf("http://127.0.0.1") === 0 || value.indexOf("http://localhost") === 0) {
            if (dashboard)
                dashboard.url = value
            return
        }
        Qt.openUrlExternally(value)
        root.hostWindow.showToast(qsTr("Lien ouvert dans le navigateur."))
    }

    Component {
        id: profileComponent

        WebEngineProfile {
            storageName: "nine-router"
            offTheRecord: false
            persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
        }
    }

    readonly property var filteredModels: {
        const needle = query.trim().toLowerCase()
        const source = nineRouter.models
        const result = []
        for (let i = 0; i < source.length; ++i) {
            const model = source[i]
            if (needle === "" || String(model.id).toLowerCase().indexOf(needle) >= 0
                    || String(model.ownedBy).toLowerCase().indexOf(needle) >= 0)
                result.push(model)
        }
        return result
    }

    readonly property int selectedCount: selectedModels.length

    function recommendedModels() {
        const picks = []
        const source = nineRouter.models
        const prefixes = ["oc/", "kr/", "ocg/", "cc/", "cx/", "ccg/"]
        for (let p = 0; p < prefixes.length && picks.length < 12; ++p) {
            for (let i = 0; i < source.length && picks.length < 12; ++i) {
                const id = String(source[i].id)
                if (id.indexOf(prefixes[p]) === 0 && picks.indexOf(id) < 0)
                    picks.push(id)
            }
        }
        if (picks.length === 0) {
            for (let i = 0; i < source.length && picks.length < 8; ++i)
                picks.push(source[i].id)
        }
        return picks
    }

    function ensureSelection() {
        if (selectedModels.length === 0 && nineRouter.models.length > 0)
            selectedModels = recommendedModels()
    }

    function toggleModel(id) {
        const next = selectedModels.slice()
        const index = next.indexOf(id)
        if (index >= 0)
            next.splice(index, 1)
        else
            next.push(id)
        selectedModels = next
    }

    function selectVisible() {
        const next = selectedModels.slice()
        for (let i = 0; i < filteredModels.length; ++i) {
            if (next.indexOf(filteredModels[i].id) < 0)
                next.push(filteredModels[i].id)
        }
        selectedModels = next
    }

    function applyHarness() {
        if (selectedModels.length === 0) {
            root.hostWindow.showToast(qsTr("Select at least one model."))
            return
        }
        nineRouter.applyToHarness(selectedModels, makeDefault)
    }

    Component.onCompleted: {
        if (visible) {
            Qt.callLater(function () { nineRouter.refresh() })
            if (nineRouter.running) {
                ensureProfile()
                armed = true
            }
        }
    }
    onVisibleChanged: {
        if (visible) {
            nineRouter.refresh()
            if (nineRouter.running) {
                ensureProfile()
                armed = true
            }
        }
    }

    Connections {
        target: nineRouter

        function onRunningChanged() {
            if (nineRouter.running && root.visible) {
                root.ensureProfile()
                root.armed = true
            }
        }

        function onModelsChanged() { root.ensureSelection() }

        function onHarnessApplied() {
            connections.refresh()
            backend.restartHarness()
            root.hostWindow.showToast(qsTr("9Router est maintenant disponible dans le harness."))
        }

        function onHarnessDisconnected() {
            connections.refresh()
            backend.restartHarness()
            root.hostWindow.showToast(qsTr("9Router retiré du harness."))
        }

        function onErrorChanged() {
            if (nineRouter.error !== "")
                root.hostWindow.showToast(nineRouter.error)
        }

        function onAuthRequiredChanged() {
            if (nineRouter.authRequired && nineRouter.running)
                root.integrationOpen = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 58
            color: theme.surface
            border.width: 0

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 14
                spacing: 10

                Item {
                    width: 34
                    height: 34
                    Rectangle {
                        anchors.centerIn: parent
                        width: 32
                        height: 32
                        radius: 16
                        color: theme.accent
                        opacity: 0.12
                    }
                    ThemedIcon {
                        anchors.centerIn: parent
                        source: "icons/router.svg"
                        width: 17
                        height: 17
                        color: theme.accent
                    }
                }

                ColumnLayout {
                    spacing: 1
                    Text {
                        text: "9Router"
                        color: theme.brightText
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                    RowLayout {
                        spacing: 6
                        StatusDot {
                            color: nineRouter.running ? theme.green
                                   : (nineRouter.starting || nineRouter.installing ? theme.yellow : theme.dimText)
                        }
                        Text {
                            text: nineRouter.detail + (nineRouter.version !== "" ? " · v" + nineRouter.version : "")
                            color: theme.dimText
                            font.pixelSize: 9
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    visible: nineRouter.running
                    text: qsTr("Integration")
                    flat: true
                    highlighted: root.integrationOpen
                    onClicked: root.integrationOpen = !root.integrationOpen
                }
                Button {
                    visible: nineRouter.running
                    text: qsTr("Dashboard")
                    flat: true
                    onClicked: Qt.openUrlExternally(nineRouter.dashboardUrl)
                }
                Button {
                    visible: nineRouter.installed && !nineRouter.running && !nineRouter.installing
                    text: nineRouter.starting ? qsTr("Starting…") : qsTr("Start")
                    enabled: !nineRouter.starting
                    flat: true
                    onClicked: nineRouter.start()
                }
                Button {
                    visible: nineRouter.running
                    text: qsTr("Stop")
                    flat: true
                    onClicked: nineRouter.stop()
                }
                ActionButton {
                    visible: !nineRouter.installed
                    text: nineRouter.installing ? qsTr("Installing…") : qsTr("Install 9Router")
                    enabled: !nineRouter.installing
                    onClicked: nineRouter.install()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Loader {
                    id: dashboardLoader
                    anchors.fill: parent
                    active: root.armed && nineRouter.running
                    sourceComponent: dashboardViewComponent
                }

                Component {
                    id: dashboardViewComponent

                    WebEngineView {
                        id: dashboard
                        backgroundColor: theme.background
                        profile: root.profile
                        settings.javascriptEnabled: true
                        settings.localStorageEnabled: true
                        url: (nineRouter.running && root.visible) ? nineRouter.dashboardUrl : ""

                        onLoadingChanged: function (request) {
                            if (request.status === WebEngineView.LoadFailedStatus && nineRouter.running)
                                root.hostWindow.showToast(qsTr("The 9Router dashboard did not load."))
                        }
                        onNewWindowRequested: function (request) {
                            const value = String(request.requestedUrl)
                            if (value.indexOf("http://127.0.0.1") === 0 || value.indexOf("http://localhost") === 0)
                                request.openIn(dashboard)
                            else
                                root.openLink(request.requestedUrl)
                        }
                        onNavigationRequested: function (request) {
                            const value = String(request.url)
                            if (request.isMainFrame && value.indexOf("http") === 0
                                    && value.indexOf("http://127.0.0.1") !== 0
                                    && value.indexOf("http://localhost") !== 0) {
                                Qt.openUrlExternally(request.url)
                                request.reject()
                            } else {
                                request.accept()
                            }
                        }
                    }
                }

                Rectangle {
                    visible: !nineRouter.running
                    anchors.centerIn: parent
                    width: Math.min(560, parent.width - 60)
                    height: Math.max(270, welcome.implicitHeight + 52)
                    radius: Math.min(14, theme.radius)
                    color: theme.surface
                    border.width: 1
                    border.color: theme.border

                    ColumnLayout {
                        id: welcome
                        anchors.fill: parent
                        anchors.margins: 26
                        spacing: 10

                        RowLayout {
                            spacing: 10
                            ThemedIcon {
                                source: "icons/router.svg"
                                width: 24
                                height: 24
                                color: theme.accent
                            }
                            Text {
                                Layout.fillWidth: true
                                text: nineRouter.installed ? qsTr("9Router is not running") : qsTr("Install 9Router")
                                color: theme.brightText
                                font.pixelSize: 17
                                font.weight: Font.Bold
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: nineRouter.installed
                                  ? qsTr("Start the local router, then configure your providers in its dashboard. Ultimate HAI exposes its models to the harness.")
                                  : qsTr("9Router is a local AI router that connects 40+ providers and keeps one OpenAI-compatible endpoint. Install it locally inside Ultimate HAI; no root access and no global npm change.")
                            color: theme.mutedText
                            font.pixelSize: 10
                            wrapMode: Text.Wrap
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: nineRouter.installing
                            text: qsTr("Installing the local package… this can take a minute.")
                            color: theme.yellow
                            font.pixelSize: 9
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: nineRouter.installLog !== ""
                            text: nineRouter.installLog
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: 8
                            wrapMode: Text.Wrap
                            maximumLineCount: 4
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: nineRouter.error !== ""
                            text: nineRouter.error
                            color: theme.red
                            font.pixelSize: 9
                            wrapMode: Text.Wrap
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: nineRouter.installReport !== ""
                            text: nineRouter.installReport
                            color: theme.green
                            font.pixelSize: 9
                            wrapMode: Text.Wrap
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: nineRouter.privilegedLog !== ""
                            text: nineRouter.privilegedLog
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: 8
                            wrapMode: Text.Wrap
                            maximumLineCount: 4
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Item { Layout.fillWidth: true }
                            Button {
                                visible: nineRouter.installed
                                text: nineRouter.starting ? qsTr("Starting…") : qsTr("Start 9Router")
                                enabled: !nineRouter.starting
                                flat: true
                                onClicked: nineRouter.start()
                            }
                            Button {
                                text: nineRouter.privilegedBusy ? qsTr("Authorizing…") : qsTr("System dependencies (sudo)")
                                flat: true
                                font.pixelSize: 9
                                enabled: !nineRouter.privilegedBusy && !nineRouter.installing
                                onClicked: nineRouter.installDependencies()
                            }
                            ActionButton {
                                visible: !nineRouter.installed
                                text: nineRouter.installing ? qsTr("Installing…") : qsTr("Install 9Router")
                                enabled: !nineRouter.installing
                                onClicked: nineRouter.install()
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: !nineRouter.installed && nineRouter.privilegedLog === ""
                            text: qsTr("The local install needs no root. pkexec is only used for Node/npm/PyYAML dependencies, and the password stays with polkit.")
                            color: theme.dimText
                            font.pixelSize: 8
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }

            Rectangle {
                id: drawer
                visible: root.integrationOpen && nineRouter.running
                Layout.preferredWidth: 390
                Layout.fillHeight: true
                color: theme.surface
                border.width: 0

                Rectangle {
                    anchors.left: parent.left
                    width: 1
                    height: parent.height
                    color: theme.border
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Integration")
                            color: theme.brightText
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }
                        IconButton {
                            icon: "icons/close.svg"
                            hint: qsTr("Close")
                            onClicked: root.integrationOpen = false
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("The dashboard is where you connect providers. Here you choose the models exposed to the harness.")
                        color: theme.dimText
                        font.pixelSize: 9
                        wrapMode: Text.Wrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text {
                            text: qsTr("Key")
                            color: theme.dimText
                            font.pixelSize: 9
                        }
                        Text {
                            Layout.fillWidth: true
                            text: nineRouter.keyMasked !== "" ? nineRouter.keyMasked : qsTr("not created")
                            color: nineRouter.keyMasked !== "" ? theme.green : theme.yellow
                            font.family: backend.monoFont
                            font.pixelSize: 9
                        }
                        Button {
                            text: qsTr("Refresh")
                            flat: true
                            font.pixelSize: 9
                            onClicked: nineRouter.refresh()
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Dashboard login: 123456 on a fresh install. Change it in 9Router Settings.")
                        color: theme.dimText
                        font.pixelSize: 8
                        wrapMode: Text.Wrap
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        visible: nineRouter.authRequired
                        height: authColumn.implicitHeight + 20
                        radius: Math.min(8, theme.radius)
                        color: theme.raised
                        border.width: 1
                        border.color: theme.yellow

                        ColumnLayout {
                            id: authColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 5
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("9Router asks for its dashboard password.")
                                color: theme.yellow
                                font.pixelSize: 9
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                TextField {
                                    id: passwordField
                                    Layout.fillWidth: true
                                    placeholderText: qsTr("Password")
                                    echoMode: TextInput.Password
                                    color: theme.text
                                    font.pixelSize: 10
                                    onTextChanged: root.password = text
                                    background: Rectangle {
                                        implicitHeight: 28
                                        radius: 5
                                        color: theme.background
                                        border.width: 1
                                        border.color: passwordField.activeFocus ? theme.accent : theme.border
                                    }
                                }
                                Button {
                                    text: qsTr("Log in")
                                    flat: true
                                    font.pixelSize: 9
                                    onClicked: nineRouter.login(root.password)
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("A fresh install uses 123456; change it in 9Router Settings.")
                                color: theme.dimText
                                font.pixelSize: 8
                                wrapMode: Text.Wrap
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("%1 models · %2 selected").arg(nineRouter.models.length).arg(root.selectedCount)
                        color: theme.dimText
                        font.pixelSize: 9
                    }

                    TextField {
                        id: modelSearch
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search models…")
                        color: theme.text
                        font.pixelSize: 10
                        onTextChanged: root.query = text
                        background: Rectangle {
                            implicitHeight: 30
                            radius: 6
                            color: theme.background
                            border.width: 1
                            border.color: modelSearch.activeFocus ? theme.accent : theme.border
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Button {
                            text: qsTr("Recommended")
                            flat: true
                            font.pixelSize: 9
                            onClicked: root.selectedModels = root.recommendedModels()
                        }
                        Button {
                            text: qsTr("Select visible")
                            flat: true
                            font.pixelSize: 9
                            onClicked: root.selectVisible()
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: qsTr("Clear")
                            flat: true
                            font.pixelSize: 9
                            onClicked: root.selectedModels = []
                        }
                    }

                    ListView {
                        id: modelList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 0
                        model: root.filteredModels

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 30
                            color: modelMouse.containsMouse ? theme.raisedHover : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 2
                                anchors.rightMargin: 4
                                spacing: 4

                                CheckBox {
                                    checked: root.selectedModels.indexOf(modelData.id) >= 0
                                    onToggled: root.toggleModel(modelData.id)
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.id
                                        color: theme.text
                                        font.family: backend.monoFont
                                        font.pixelSize: 9
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        visible: modelData.ownedBy !== ""
                                        text: modelData.ownedBy
                                        color: theme.dimText
                                        font.pixelSize: 7
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            MouseArea {
                                id: modelMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                acceptedButtons: Qt.NoButton
                            }
                        }
                    }

                    CheckBox {
                        Layout.fillWidth: true
                        text: qsTr("Make it the harness default model")
                        checked: root.makeDefault
                        onToggled: root.makeDefault = checked
                        font.pixelSize: 9
                    }

                    ActionButton {
                        Layout.fillWidth: true
                        text: qsTr("Appliquer au harness")
                        onClicked: root.applyHarness()
                    }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Disconnect from harness")
                        flat: true
                        font.pixelSize: 9
                        onClicked: nineRouter.disconnectHarness()
                    }
                }
            }
        }
    }
}
