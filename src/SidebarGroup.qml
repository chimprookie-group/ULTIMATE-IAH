import QtQuick
import QtQuick.Layouts

Item {
    id: root
    property string title: ""
    property bool compact: false
    Layout.fillWidth: true
    Layout.topMargin: design.space5
    Layout.bottomMargin: design.space1
    implicitHeight: compact ? design.space2 : 18

    RowLayout {
        anchors.fill: parent
        visible: !root.compact
        anchors.leftMargin: design.space4
        anchors.rightMargin: design.space4
        spacing: design.space2

        Text {
            text: root.title
            color: theme.dimText
            font.family: backend.monoFont
            font.pixelSize: design.textMicro
            font.letterSpacing: 0.8
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: theme.border
            opacity: 0.7
        }
    }
}
