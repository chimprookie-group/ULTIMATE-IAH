import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

TextField {
    color: theme.text
    placeholderTextColor: theme.dimText
    font.family: backend.monoFont
    font.pixelSize: 10
    selectionColor: theme.accent
    selectedTextColor: theme.accentForeground
    leftPadding: 9
    rightPadding: 9

    background: Rectangle {
        implicitHeight: 32
        radius: Math.min(6, theme.radius)
        color: theme.background
        border.width: 1
        border.color: parent.activeFocus ? theme.accent : theme.border
    }
}
