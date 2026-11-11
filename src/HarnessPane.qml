import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtWebEngine


Item {
    id: root

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
        renderType: Text.NativeRendering
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
        renderType: Text.NativeRendering
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
                    onClicked: web.reload()
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

            WebEngineProfile {
                id: harnessProfile
                storageName: "deepcrasy-h"
                offTheRecord: false
                persistentCookiesPolicy: WebEngineProfile.ForcePersistentCookies
            }

            WebEngineView {
                id: web
                anchors.fill: parent
                visible: backend.harnessState === "running"
                backgroundColor: theme.surface
                profile: harnessProfile
                settings.javascriptEnabled: true
                settings.localStorageEnabled: true
                settings.fullScreenSupportEnabled: true

                onLoadingChanged: function (request) {
                    if (request.status === WebEngineView.LoadFailedStatus)
                        win.showToast(qsTr("The harness page did not load."))
                }
            }

            Rectangle {
                visible: web.loading
                anchors.top: parent.top
                height: 2
                width: parent.width * Math.max(0, web.loadProgress) / 100
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
        renderType: Text.NativeRendering
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Starting DeepSeek Harness…")
                    color: theme.text
                    font.pixelSize: 13
                }
                Text {
        renderType: Text.NativeRendering
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
        renderType: Text.NativeRendering
                            text: qsTr("The harness is not running")
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: theme.text
                        }
                    }
                    Text {
        renderType: Text.NativeRendering
                        Layout.fillWidth: true
                        text: backend.harnessDetail
                        wrapMode: Text.Wrap
                        color: theme.mutedText
                        font.pixelSize: 12
                    }
                    Text {
        renderType: Text.NativeRendering
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

    Component.onCompleted: {
        if (backend.harnessUrl !== "")
            web.url = backend.harnessUrl
    }

    Connections {
        target: backend
        function onHarnessChanged() {
            if (backend.harnessUrl !== "" && web.url.toString() !== backend.harnessUrl)
                web.url = backend.harnessUrl
        }
    }
}
