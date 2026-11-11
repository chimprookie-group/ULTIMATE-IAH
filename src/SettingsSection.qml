import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts


Rectangle {
    id: root
    property string title: ""
    default property alias content: column.data

    Layout.fillWidth: true
    implicitHeight: column.implicitHeight + 36
    color: theme.raised
    radius: Math.min(10, theme.radius)
    border.width: 1
    border.color: theme.border

    ColumnLayout {
        id: column
        x: 16
        y: 16
        width: parent.width - 32
        spacing: 10

        Text {
        renderType: Text.NativeRendering
            text: root.title.toUpperCase()
            font.pixelSize: 9
            font.letterSpacing: 1.3
            font.weight: Font.DemiBold
            color: theme.mutedText
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.bottomMargin: 2
            height: 1
            color: theme.border
        }
    }
}
