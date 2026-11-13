import QtQuick
import QtQuick.Controls

// Small outlined action used for secondary and configuration actions. It keeps
// the same hit target and focus ring as ActionButton without looking like
// plain text.
Button {
    id: control

    implicitHeight: design.controlHeightSm
    implicitWidth: Math.max(64, contentItem.implicitWidth + design.space4)
    padding: 0
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        radius: design.radiusSm
        color: control.down ? theme.chip : (control.hovered ? theme.chip : theme.surface)
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? theme.accent : (control.hovered ? theme.borderStrong : theme.border)
        Behavior on color { ColorAnimation { duration: design.durationFast } }
        Behavior on border.color { ColorAnimation { duration: design.durationFast } }
    }

    contentItem: Text {
        text: control.text
        color: control.enabled ? (control.hovered ? theme.accent : theme.text) : theme.dimText
        font.family: control.font.family
        font.pixelSize: control.font.pixelSize > 0 ? control.font.pixelSize : design.textMicro
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        leftPadding: design.space3
        rightPadding: design.space3
    }

    HoverHandler { cursorShape: Qt.CursorShape.PointingHandCursor }
}
