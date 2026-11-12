import QtQuick
import QtQuick.Controls

Button {
    id: control

    property bool filled: true
    property bool danger: false

    implicitHeight: design.controlHeight
    implicitWidth: Math.max(92, contentItem.implicitWidth + design.space7)
    padding: 0
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        radius: design.radiusSm
        color: control.filled
             ? (control.danger ? theme.red : (control.down ? Qt.darker(theme.accent, 1.08)
                                                             : (control.hovered ? Qt.lighter(theme.accent, 1.06) : theme.accent)))
             : (control.hovered ? theme.chip : "transparent")
        border.width: control.activeFocus ? 2 : (control.filled ? 0 : 1)
        border.color: control.activeFocus ? theme.brightText : (control.danger ? theme.red : theme.border)
    }

    contentItem: Text {
        text: control.text
        color: control.filled
             ? (control.danger ? "#ffffff" : theme.accentForeground)
             : (control.danger ? theme.red : theme.text)
        font.pixelSize: design.textLabel
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        leftPadding: design.space4
        rightPadding: design.space4
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
