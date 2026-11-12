import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts


Rectangle {
    id: root
    property string title: ""
    default property alias content: column.data

    Layout.fillWidth: true
    implicitHeight: column.implicitHeight + 40
    color: theme.raised
    radius: Math.min(12, theme.radius)
    border.width: 1
    border.color: theme.border

    ColumnLayout {
        id: column
        x: 18
        y: 17
        width: parent.width - 36
        spacing: 10

        Text {
            text: root.title
            font.pixelSize: 12
            font.weight: Font.DemiBold
            color: theme.text
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.bottomMargin: 2
            height: 1
            color: theme.border
        }
    }
}
