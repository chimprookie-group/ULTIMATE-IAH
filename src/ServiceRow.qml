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
    implicitHeight: 48

    Rectangle {
        anchors.fill: parent
        radius: design.radiusSm
        color: hover.hovered ? theme.chip : "transparent"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: design.space2
        anchors.rightMargin: design.space2
        spacing: design.space3

        Rectangle { width: 6; height: 6; radius: 3; color: root.stateColor; Layout.alignment: Qt.AlignVCenter }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Text {
                Layout.fillWidth: true
                text: root.name
                color: theme.text
                font.pixelSize: design.textLabel
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: root.detail
                color: theme.dimText
                font.pixelSize: design.textCaption
                elide: Text.ElideRight
            }
        }

        Button {
            visible: root.actionLabel !== ""
            text: root.actionLabel
            flat: true
            enabled: root.actionEnabled
            font.pixelSize: design.textCaption
            implicitHeight: design.controlHeightSm
            onClicked: root.action()
        }
    }

    HoverHandler { id: hover }
}
