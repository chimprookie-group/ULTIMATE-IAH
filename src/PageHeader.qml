import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Shared page heading. No all-caps eyebrow: a small index marker, the display
// title and one plain-language description carry the hierarchy.
Item {
    id: root

    property string title: ""
    property string description: ""
    property string icon: ""
    property string index: ""
    property color accent: theme.accent
    property bool brandMark: false

    default property alias actions: actionRow.data

    implicitWidth: parent ? parent.width : 800
    implicitHeight: Math.max(58, titleColumn.implicitHeight)

    RowLayout {
        anchors.fill: parent
        spacing: design.space4

        Item {
            width: 42
            height: 42
            Layout.alignment: Qt.AlignTop
            Rectangle {
                anchors.fill: parent
                visible: !root.brandMark
                radius: design.radiusMd
                color: theme.raised
                border.width: 1
                border.color: theme.border
            }
            BrandMark {
                anchors.fill: parent
                visible: root.brandMark
                size: 42
                accent: false
            }
            ThemedIcon {
                anchors.centerIn: parent
                visible: !root.brandMark
                source: root.icon
                width: design.iconLg
                height: design.iconLg
                color: root.accent
            }
        }

        ColumnLayout {
            id: titleColumn
            Layout.fillWidth: true
            spacing: 3

            RowLayout {
                Layout.fillWidth: true
                spacing: design.space2
                Text {
                    text: root.index
                    visible: root.index !== ""
                    color: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: design.textCaption
                }
                Text {
                    Layout.fillWidth: true
                    text: root.title
                    color: theme.brightText
                    font.family: "iA Writer Duospace"
                    font.pixelSize: design.textDisplay
                    font.weight: Font.Bold
                    elide: Text.ElideRight
                }
            }
            Text {
                Layout.fillWidth: true
                visible: root.description !== ""
                text: root.description
                color: theme.mutedText
                font.pixelSize: design.textBody
                lineHeight: 1.25
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        RowLayout {
            id: actionRow
            Layout.alignment: Qt.AlignTop
            spacing: design.space2
        }
    }
}
