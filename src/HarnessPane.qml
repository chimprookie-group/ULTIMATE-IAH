import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtWebEngine

import "Palette.js" as Pal

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: Pal.bg

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Pal.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 8
                spacing: 8

                Rectangle {
                    width: 5
                    height: 5
                    radius: 2.5
                    color: win.stateColor(backend.harnessState)
                }
                Text {
                    text: qsTr("DeepSeek Harness")
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    color: Pal.text
                }
                Rectangle {
                    implicitWidth: profileLabel.implicitWidth + 12
                    height: 18
                    radius: 9
                    color: Pal.raised
                    border.width: 1
                    border.color: Pal.border
                    Text {
                        id: profileLabel
                        anchors.centerIn: parent
                        text: backend.harnessProfile
                        font.family: backend.monoFont
                        font.pixelSize: 9
                        color: Pal.muted
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
                backgroundColor: Pal.panel
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
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Starting DeepSeek Harness…")
                    color: Pal.text
                    font.pixelSize: 13
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: backend.harnessDetail
                    color: Pal.dim
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
                color: Pal.raised
                border.width: 1
                border.color: Pal.borderStrong

                ColumnLayout {
                    id: failedColumn
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 10

                    RowLayout {
                        spacing: 8
                        Image {
                            source: "icons/alert.svg"
                            sourceSize.width: 16
                            sourceSize.height: 16
                        }
                        Text {
                            text: qsTr("The harness is not running")
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: Pal.text
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.harnessDetail
                        wrapMode: Text.Wrap
                        color: Pal.muted
                        font.pixelSize: 12
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: backend.harnessLog !== ""
                        text: backend.harnessLog.split("\n").slice(-6).join("\n")
                        font.family: backend.monoFont
                        font.pixelSize: 10
                        color: Pal.dim
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
                        Button {
                            text: qsTr("Restart")
                            highlighted: true
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
