import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts


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
        color: theme.surface

        Behavior on x { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

        Rectangle {
            anchors.left: parent.left
            width: 1
            height: parent.height
            color: theme.border
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
        renderType: Text.NativeRendering
                        text: qsTr("Sessions")
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        color: theme.text
                    }
                    Text {
        renderType: Text.NativeRendering
                        Layout.fillWidth: true
                        text: backend.workspaceName
                        color: theme.dimText
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
                color: theme.border
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
                    color: sessionMouse.containsMouse ? theme.raisedHover : theme.raised
                    border.width: 1
                    border.color: theme.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 9
                        spacing: 2

                        Text {
        renderType: Text.NativeRendering
                            Layout.fillWidth: true
                            text: backend.sessionLabel(modelData)
                            color: theme.text
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
        renderType: Text.NativeRendering
                                text: modelData.updatedAt !== undefined
                                      ? Qt.formatDateTime(new Date(modelData.updatedAt), "d MMM · HH:mm")
                                      : ""
                                color: theme.dimText
                                font.pixelSize: 10
                            }
                            Text {
        renderType: Text.NativeRendering
                                Layout.fillWidth: true
                                text: modelData.sessionId !== undefined ? modelData.sessionId : ""
                                color: theme.dimText
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
        renderType: Text.NativeRendering
                Layout.fillWidth: true
                visible: backend.loadingSessions
                text: qsTr("Loading sessions…")
                color: theme.dimText
                font.pixelSize: 10
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
        renderType: Text.NativeRendering
                Layout.fillWidth: true
                visible: !backend.loadingSessions && backend.sessions.length === 0
                text: qsTr("No sessions in this workspace yet. Send a message to start one.")
                color: theme.dimText
                font.pixelSize: 10
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
