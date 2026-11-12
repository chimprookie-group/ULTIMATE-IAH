import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property string icon: ""
    property string label: ""
    property string detail: ""
    property bool compact: false
    signal clicked()

    implicitHeight: compact ? 52 : 64
    Accessible.name: label

    Rectangle {
        anchors.fill: parent
        radius: design.radiusMd
        color: mouse.containsMouse ? theme.raised : theme.chip
        border.width: 1
        border.color: mouse.containsMouse ? theme.borderStrong : theme.border
        Behavior on color { ColorAnimation { duration: design.durationFast } }
        Behavior on border.color { ColorAnimation { duration: design.durationFast } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: design.space3
            anchors.rightMargin: design.space2
            spacing: design.space2

            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: root.compact ? 26 : 30
                implicitHeight: root.compact ? 26 : 30
                radius: design.radiusSm
                color: Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.12)
                ThemedIcon {
                    anchors.centerIn: parent
                    source: root.icon
                    width: root.compact ? 14 : 16
                    height: width
                    color: theme.accent
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1
                Text {
                    Layout.fillWidth: true
                    text: root.label
                    color: theme.text
                    font.pixelSize: root.compact ? design.textCaption : design.textLabel
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    visible: !root.compact
                    text: root.detail
                    color: theme.dimText
                    font.pixelSize: design.textMicro
                    elide: Text.ElideRight
                }
            }

            ThemedIcon {
                Layout.alignment: Qt.AlignVCenter
                source: "icons/chevron.svg"
                width: 10
                height: 10
                color: mouse.containsMouse ? theme.accent : theme.dimText
            }
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.clicked()
        }
    }
}
