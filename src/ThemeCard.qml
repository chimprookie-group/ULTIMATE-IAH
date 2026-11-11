import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A compact preview of an installed Omarchy palette. The card paints itself
// in the theme it represents, so the grid is a real comparison, not swatches
// on a neutral card.
Item {
    id: root

    property string themeId: ""
    property string label: ""
    property string mode: "dark"
    property var palette: ({})
    property bool active: false
    property bool autoTheme: false
    property string subtitle: ""

    signal chosen(string id)

    width: 160
    height: 86

    readonly property color cardBackground: palette && palette.background !== undefined ? palette.background : theme.background
    readonly property color cardForeground: palette && palette.foreground !== undefined ? palette.foreground : theme.text
    readonly property color cardAccent: palette && palette.accent !== undefined ? palette.accent : theme.accent
    readonly property color cardCyan: palette && palette.cyan !== undefined ? palette.cyan : theme.cyan
    readonly property color cardRed: palette && palette.red !== undefined ? palette.red : theme.red
    readonly property color cardGreen: palette && palette.green !== undefined ? palette.green : theme.green

    Rectangle {
        id: card
        anchors.fill: parent
        radius: Math.min(10, theme.radius)
        color: root.cardBackground
        border.width: root.active ? 2 : 1
        border.color: root.active ? root.cardAccent : Qt.rgba(root.cardForeground.r, root.cardForeground.g, root.cardForeground.b, 0.26)
        scale: hover.containsMouse ? 1.015 : 1.0
        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                spacing: 3

                Repeater {
                    model: [root.cardBackground, root.cardAccent, root.cardCyan, root.cardRed, root.cardGreen]
                    Rectangle {
                        width: 18
                        height: 8
                        radius: 3
                        color: modelData
                        border.width: 1
                        border.color: Qt.rgba(root.cardForeground.r, root.cardForeground.g, root.cardForeground.b, 0.22)
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    visible: root.active
                    width: 16
                    height: 16
                    radius: 8
                    color: root.cardAccent
                    ThemedIcon {
                        anchors.centerIn: parent
                        source: "icons/check.svg"
                        width: 9
                        height: 9
                        color: root.cardBackground
                    }
                }
            }

            Item { Layout.fillHeight: true }

            Text {
                Layout.fillWidth: true
                text: root.label
                color: root.cardForeground
                font.pixelSize: 11
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: root.subtitle !== "" ? root.subtitle : (root.mode === "light" ? qsTr("Light") : qsTr("Dark"))
                color: root.cardForeground
                opacity: 0.55
                font.pixelSize: 9
                elide: Text.ElideRight
            }
        }
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.chosen(root.themeId)
    }
}
