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
    implicitHeight: body.implicitHeight + 36
    height: implicitHeight
    radius: Math.min(14, theme.radius)
    color: theme.raised
    border.width: 1
    border.color: hover.hovered ? theme.borderStrong : theme.border
    Behavior on border.color { ColorAnimation { duration: 100 } }

    ColumnLayout {
        id: body
        x: 18
        y: 17
        width: parent.width - 36
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 7

            ThemedIcon {
                source: root.icon
                width: 13
                height: 13
                color: root.accent
            }
            Text {
                Layout.fillWidth: true
                text: root.title
                color: theme.text
                font.pixelSize: 12
                font.weight: Font.DemiBold
                elide: Text.ElideRight
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
