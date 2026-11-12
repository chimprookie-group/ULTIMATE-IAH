import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root

    property string title: ""
    property string icon: ""
    property color accent: theme.accent
    property int cardWidth: 440

    default property alias content: body.data

    width: cardWidth
    implicitHeight: body.implicitHeight + design.space6
    height: implicitHeight
    radius: design.radiusLg
    color: theme.raised
    border.width: 1
    border.color: hover.hovered ? theme.borderStrong : theme.border
    Behavior on border.color { ColorAnimation { duration: design.durationFast } }

    ColumnLayout {
        id: body
        x: design.space5
        y: design.space4
        width: parent.width - design.space8
        spacing: design.space2

        RowLayout {
            Layout.fillWidth: true
            spacing: design.space2
            Rectangle { width: 3; height: 12; radius: 2; color: root.accent }
            ThemedIcon { source: root.icon; width: design.iconSm; height: design.iconSm; color: root.accent }
            Text {
                Layout.fillWidth: true
                text: root.title
                color: theme.text
                font.pixelSize: design.textBodyLarge
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
        }
    }

    HoverHandler { id: hover }
}
