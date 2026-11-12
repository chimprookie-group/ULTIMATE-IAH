import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtWebEngine


Item {
    id: root

    property bool armed: false
    property var profile: null
    readonly property var web: webLoader.item
    readonly property bool webLoading: web ? web.loading : false
    readonly property real webProgress: web ? web.loadProgress : 0

    function ensureProfile() {
        if (!profile)
            profile = profileComponent.createObject(root)
    }

    Component {
        id: profileComponent

        WebEngineProfile {
            storageName: "deepcrasy-h"
            offTheRecord: false
            persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: theme.background

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 8
                spacing: 8

                StatusDot {
                    color: win.stateColor(backend.harnessState)
                }
                Text {
                    text: qsTr("DeepSeek Harness")
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    color: theme.text
                }
                Rectangle {
                    implicitWidth: profileLabel.implicitWidth + 12
                    height: 18
                    radius: 9
                    color: theme.raised
                    border.width: 1
                    border.color: theme.border
                    Text {
                        id: profileLabel
                        anchors.centerIn: parent
                        text: backend.harnessProfile
                        font.family: backend.monoFont
                        font.pixelSize: 9
                        color: theme.mutedText
                    }
                }

                Item { Layout.fillWidth: true }

                IconButton {
                    icon: "icons/reload.svg"
                    hint: qsTr("Reload the harness page")
                    onClicked: { if (web) web.reload() }
                }
                IconButton {
                    icon: "icons/external.svg"
                    hint: qsTr("Open the harness in a browser")
                    onClicked: {
                        if (backend.harnessUrl !== "")
                            Qt.openUrlExternally(backend.harnessUrl)
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Loader {
                id: webLoader
                anchors.fill: parent
                active: root.armed
                sourceComponent: harnessViewComponent
            }

            Rectangle {
                visible: root.webLoading
                anchors.top: parent.top
                height: 2
                width: parent.width * Math.max(0, root.webProgress) / 100
                color: theme.accent
            }

            // Starting
            Column {
                anchors.centerIn: parent
                spacing: 14
                visible: backend.harnessState === "starting" || backend.harnessState === "stopped"

                BusyIndicator {
                    running: visible
                    anchors.horizontalCenter: parent.horizontalCenter
                    implicitWidth: 30
                    implicitHeight: 30
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Starting DeepSeek Harness…")
                    color: theme.text
                    font.pixelSize: 13
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: backend.harnessDetail
                    color: theme.dimText
                    font.pixelSize: 11
                }
            }

            // Failed
            Rectangle {
                visible: backend.harnessState === "failed"
                width: Math.min(560, parent.width - 80)
                height: failedColumn.implicitHeight + 40
                anchors.centerIn: parent
                radius: Math.min(12, theme.radius)
                color: theme.raised
                border.width: 1
                border.color: theme.borderStrong

                ColumnLayout {
                    id: failedColumn
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 10

                    RowLayout {
                        spacing: 8
                        ThemedIcon {
                            source: "icons/alert.svg"
                            width: 16
                            height: 16
                            color: theme.yellow
                        }
                        Text {
                            text: qsTr("The harness is not running")
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: theme.text
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.harnessDetail
                        wrapMode: Text.Wrap
                        color: theme.mutedText
                        font.pixelSize: 12
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: backend.harnessLog !== ""
                        text: backend.harnessLog.split("\n").slice(-6).join("\n")
                        font.family: backend.monoFont
                        font.pixelSize: 10
                        color: theme.dimText
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        maximumLineCount: 8
                        elide: Text.ElideRight
                    }
                    RowLayout {
                        spacing: 8
                        Layout.alignment: Qt.AlignRight

                        Button {
                            text: qsTr("Settings")
                            flat: true
                            onClicked: backend.setView("settings")
                        }
                        ActionButton {
                            text: qsTr("Restart")
                            onClicked: backend.restartHarness()
                        }
                    }
                }
            }
        }
    }

    Component {
        id: harnessViewComponent

        WebEngineView {
            id: web
            visible: backend.harnessState === "running"
            backgroundColor: theme.surface
            profile: root.profile
            settings.javascriptEnabled: true
            settings.localStorageEnabled: true
            settings.fullScreenSupportEnabled: true

            onLoadingChanged: function (request) {
                if (request.status === WebEngineView.LoadFailedStatus)
                    win.showToast(qsTr("The harness page did not load."))
            }
        }
    }

    function loadHarness() {
        if (!root.visible || backend.harnessUrl === "")
            return
        root.ensureProfile()
        root.armed = true
        if (web && web.url.toString() !== backend.harnessUrl)
            web.url = backend.harnessUrl
    }

    onVisibleChanged: {
        if (!root.visible) {
            if (web)
                web.url = ""
        } else {
            loadHarness()
        }
    }

    Component.onCompleted: loadHarness()

    Connections {
        target: backend
        function onHarnessChanged() { loadHarness() }
    }
}
