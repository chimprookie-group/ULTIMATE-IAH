import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A compact preview of an installed Omarchy themeColors. The card paints itself
// in the theme it represents, so the grid is a real comparison, not swatches
// on a neutral card.
Item {
    id: root

    property string themeId: ""
    property string label: ""
    property string mode: "dark"
    property var themeColors: ({})
    property bool active: false
    property bool autoTheme: false
    property string subtitle: ""

    signal chosen(string id)
    activeFocusOnTab: true
    Keys.onReturnPressed: root.chosen(root.themeId)
    Keys.onEnterPressed: root.chosen(root.themeId)

    width: 160
    height: 86

    readonly property color cardBackground: themeColors && themeColors.background !== undefined ? themeColors.background : theme.background
    readonly property color cardForeground: themeColors && themeColors.foreground !== undefined ? themeColors.foreground : theme.text
    readonly property color cardAccent: themeColors && themeColors.accent !== undefined ? themeColors.accent : theme.accent
    readonly property color cardCyan: themeColors && themeColors.cyan !== undefined ? themeColors.cyan : theme.cyan
    readonly property color cardRed: themeColors && themeColors.red !== undefined ? themeColors.red : theme.red
    readonly property color cardGreen: themeColors && themeColors.green !== undefined ? themeColors.green : theme.green

    Rectangle {
        id: card
        anchors.fill: parent
        radius: design.radiusMd
        color: root.cardBackground
        border.width: root.activeFocus ? 2 : (root.active ? 2 : 1)
        border.color: root.activeFocus ? root.cardForeground
                                       : (root.active ? root.cardAccent : Qt.rgba(root.cardForeground.r, root.cardForeground.g, root.cardForeground.b, 0.26))

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: design.space3
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
                font.pixelSize: design.textLabel
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: root.subtitle !== "" ? root.subtitle : (root.mode === "light" ? qsTr("Light") : qsTr("Dark"))
                color: root.cardForeground
                opacity: root.mode === "light" ? 0.68 : 0.55
                font.pixelSize: design.textMicro
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
