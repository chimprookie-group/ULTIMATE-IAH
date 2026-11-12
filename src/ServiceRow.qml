import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string name: ""
    property string detail: ""
    property color stateColor: theme.dimText
    property string actionLabel: ""
    property bool actionEnabled: true
    signal action()

    implicitWidth: 300
    implicitHeight: 46

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: -8
        anchors.rightMargin: -8
        radius: Math.min(8, theme.radius)
        color: hover.hovered ? theme.chip : "transparent"
    }

    RowLayout {
        anchors.fill: parent
        spacing: 9

        Rectangle {
            width: 7
            height: 7
            radius: 4
            color: root.stateColor
            Layout.alignment: Qt.AlignVCenter
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Text {
                Layout.fillWidth: true
                text: root.name
                color: theme.text
                font.pixelSize: 11
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: root.detail
                color: theme.dimText
                font.pixelSize: 8
                elide: Text.ElideRight
            }
        }

        Button {
            visible: root.actionLabel !== ""
            text: root.actionLabel
            flat: true
            enabled: root.actionEnabled
            font.pixelSize: 9
            implicitHeight: 26
            onClicked: root.action()
        }
    }

    HoverHandler { id: hover }
}
