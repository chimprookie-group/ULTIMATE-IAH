import QtQuick
import QtQuick.Layouts

// A themed panel in the cockpit. Children added to the card land in its body
// column, under the title and divider.
Rectangle {
    id: root

    property string title: ""
    property string icon: ""
    property color accent: theme.accent
    property int cardWidth: 440

    default property alias content: body.data

    width: cardWidth
    implicitHeight: body.implicitHeight + 32
    height: implicitHeight
    radius: Math.min(12, theme.radius)
    color: theme.raised
    border.width: 1
    border.color: hover.hovered ? theme.borderStrong : theme.border
    Behavior on border.color { ColorAnimation { duration: 100 } }

    ColumnLayout {
        id: body
        x: 16
        y: 16
        width: parent.width - 32
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 7

            ThemedIcon {
                source: root.icon
                width: 14
                height: 14
                color: root.accent
            }
            Text {
        renderType: Text.NativeRendering
                text: root.title.toUpperCase()
                color: theme.mutedText
                font.pixelSize: 9
                font.letterSpacing: 1.2
                font.weight: Font.DemiBold
            }
            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: theme.border
        }
    }

    HoverHandler {
        id: hover
    }
}
