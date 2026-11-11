import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import "Palette.js" as Pal

Item {
    id: root

    property bool opened: false

    function open() {
        opened = true
        backend.refreshSessions()
    }

    function close() {
        opened = false
    }

    visible: opened
    z: 120

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.45
    }
    MouseArea {
        anchors.fill: parent
        onClicked: root.close()
    }

    Rectangle {
        width: Math.min(430, parent.width * 0.92)
        height: parent.height
        x: root.opened ? parent.width - width : parent.width
        color: Pal.panel

        Behavior on x { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

        Rectangle {
            anchors.left: parent.left
            width: 1
            height: parent.height
            color: Pal.border
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 10

            RowLayout {
                Layout.fillWidth: true

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1
                    Text {
                        text: qsTr("Sessions")
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        color: Pal.text
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.workspaceName
                        color: Pal.dim
                        font.pixelSize: 10
                        elide: Text.ElideMiddle
                    }
                }

                IconButton {
                    icon: "icons/reload.svg"
                    hint: qsTr("Refresh")
                    onClicked: backend.refreshSessions()
                }
                IconButton {
                    icon: "icons/close.svg"
                    hint: qsTr("Close")
                    onClicked: root.close()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Pal.border
            }

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 6
                model: backend.sessions

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 58
                    radius: Math.min(8, theme.radius)
                    color: sessionMouse.containsMouse ? Pal.raisedHover : Pal.raised
                    border.width: 1
                    border.color: Pal.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 9
                        spacing: 2

                        Text {
                            Layout.fillWidth: true
                            text: modelData.title !== undefined && modelData.title !== "" ? modelData.title : qsTr("Untitled session")
                            color: Pal.text
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                text: modelData.updatedAt !== undefined
                                      ? Qt.formatDateTime(new Date(modelData.updatedAt), "d MMM · HH:mm")
                                      : ""
                                color: Pal.dim
                                font.pixelSize: 10
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.sessionId !== undefined ? modelData.sessionId : ""
                                color: Pal.dim
                                font.family: backend.monoFont
                                font.pixelSize: 8
                                elide: Text.ElideMiddle
                            }
                        }
                    }

                    MouseArea {
                        id: sessionMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            backend.loadSession(modelData.sessionId)
                            root.close()
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: backend.loadingSessions
                text: qsTr("Loading sessions…")
                color: Pal.dim
                font.pixelSize: 10
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                visible: !backend.loadingSessions && backend.sessions.length === 0
                text: qsTr("No sessions in this workspace yet. Send a message to start one.")
                color: Pal.dim
                font.pixelSize: 10
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
