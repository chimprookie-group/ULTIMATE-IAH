import QtQuick
import QtQuick.Controls

TextField {
    color: theme.text
    placeholderTextColor: theme.dimText
    font.family: backend.monoFont
    font.pixelSize: design.textLabel
    selectionColor: theme.accent
    selectedTextColor: theme.accentForeground
    leftPadding: design.space3
    rightPadding: design.space3

    background: Rectangle {
        implicitHeight: design.controlHeight
        radius: design.radiusSm
        color: theme.background
        border.width: 1
        border.color: parent.activeFocus ? theme.accent : theme.border
    }
}
