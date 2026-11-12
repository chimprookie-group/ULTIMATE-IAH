import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var items: []
    property string currentId: ""
    signal selected(string id)
    signal renameRequested(string id, string title)
    signal deleteRequested(string id, string title)
    signal pinRequested(string id)

    ListView {
        anchors.fill: parent
        clip: true
        spacing: 2
        model: root.items
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: AppScrollBar {}

        delegate: Rectangle {
            id: row
            width: ListView.view.width
            height: 64
            radius: design.radiusSm
            color: modelData.id === root.currentId
                   ? theme.raised
                   : (rowMouse.containsMouse ? theme.chip : "transparent")
            border.width: modelData.id === root.currentId ? 1 : 0
            border.color: theme.borderStrong
            Behavior on color { ColorAnimation { duration: design.durationFast } }

            MouseArea {
                id: rowMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.selected(modelData.id)
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: design.space2
                anchors.rightMargin: design.space1
                anchors.topMargin: 8
                anchors.bottomMargin: 8
                spacing: 2

                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space1
                    ThemedIcon {
                        visible: modelData.pinned
                        source: "icons/pin.svg"
                        width: 9
                        height: 9
                        color: theme.accent
                    }
                    Text {
                        Layout.fillWidth: true
                        text: modelData.title
                        color: modelData.id === root.currentId ? theme.brightText : theme.text
                        font.pixelSize: design.textCaption
                        font.weight: modelData.id === root.currentId ? Font.DemiBold : Font.Normal
                        elide: Text.ElideRight
                    }
                    Text {
                        text: Qt.formatDateTime(new Date(modelData.updatedAt), "dd/MM")
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: 8
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: modelData.preview !== "" ? modelData.preview : qsTr("Discussion vide")
                    color: theme.dimText
                    font.pixelSize: design.textMicro
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: design.space1
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("%n message(s)", "", modelData.messageCount)
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: 8
                    }
                    IconButton {
                        implicitWidth: 20
                        implicitHeight: 20
                        icon: "icons/pin.svg"
                        active: modelData.pinned
                        hint: modelData.pinned ? qsTr("Désépingler") : qsTr("Épingler")
                        onClicked: root.pinRequested(modelData.id)
                    }
                    IconButton {
                        implicitWidth: 20
                        implicitHeight: 20
                        icon: "icons/pencil.svg"
                        hint: qsTr("Renommer")
                        onClicked: root.renameRequested(modelData.id, modelData.title)
                    }
                    IconButton {
                        implicitWidth: 20
                        implicitHeight: 20
                        icon: "icons/close.svg"
                        hint: qsTr("Supprimer la discussion")
                        onClicked: root.deleteRequested(modelData.id, modelData.title)
                    }
                }
            }
        }
    }

    Text {
        anchors.centerIn: parent
        width: parent.width - design.space5
        visible: root.items.length === 0
        text: qsTr("Aucune discussion. Commencez une nouvelle conversation.")
        color: theme.dimText
        font.pixelSize: design.textMicro
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
    }
}
