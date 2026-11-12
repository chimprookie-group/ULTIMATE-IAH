import QtQuick
import QtQuick.Controls

// A themed action button. Material's highlighted Button leaves the text
// unreadable on light palettes, so primary actions get their own fill and
// border that follow the accent color.
Button {
    id: control

    property bool filled: true
    property bool danger: false

    implicitHeight: 34
    implicitWidth: Math.max(88, label.implicitWidth + 30)
    padding: 0
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        radius: Math.min(9, theme.radius)
        color: control.filled
             ? (control.danger ? theme.red : (control.down ? Qt.darker(theme.accent, 1.1)
                                                             : (control.hovered ? Qt.lighter(theme.accent, 1.08) : theme.accent)))
             : (control.hovered ? theme.chip : "transparent")
        border.width: control.filled ? 0 : 1
        border.color: control.danger ? theme.red : theme.border
    }

    contentItem: Text {
        id: label
        text: control.text
        color: control.filled
             ? (control.danger ? "#ffffff" : theme.accentForeground)
             : (control.danger ? theme.red : theme.text)
        font.pixelSize: 11
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        leftPadding: 14
        rightPadding: 14
    }
}
