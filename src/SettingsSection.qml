import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    property string title: ""
    default property alias content: column.data

    Layout.fillWidth: true
    implicitHeight: column.implicitHeight + design.space8
    color: theme.raised
    radius: design.radiusLg
    border.width: 1
    border.color: theme.border

    ColumnLayout {
        id: column
        x: design.space5
        y: design.space4
        width: parent.width - design.space8
        spacing: design.space3

        RowLayout {
            Layout.fillWidth: true
            spacing: design.space2
            Rectangle { width: 3; height: 12; radius: 2; color: theme.accent }
            Text {
                Layout.fillWidth: true
                text: root.title
                font.pixelSize: design.textLabel
                font.weight: Font.DemiBold
                color: theme.text
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.bottomMargin: design.space1
            height: 1
            color: theme.border
        }
    }
}
