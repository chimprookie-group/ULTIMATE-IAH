import QtQuick
import QtQuick.Controls

// Compact themed switch used by automation panels.
Switch {
    id: control

    implicitHeight: 28
    spacing: design.space2
    font.pixelSize: design.textCaption

    indicator: Rectangle {
        x: 1
        y: (control.height - height) / 2
        width: 34
        height: 18
        radius: 9
        color: control.checked ? theme.accentSoft : theme.chip
        border.width: 1
        border.color: control.checked ? theme.accent : theme.borderStrong
        Behavior on color { ColorAnimation { duration: design.durationFast } }
        Behavior on border.color { ColorAnimation { duration: design.durationFast } }

        Rectangle {
            x: control.checked ? parent.width - width - 2 : 2
            y: 2
            width: 12
            height: 12
            radius: 6
            color: control.checked ? theme.accent : theme.mutedText
            Behavior on x { NumberAnimation { duration: design.durationFast; easing.type: Easing.OutCubic } }
        }
    }

    contentItem: Text {
        leftPadding: 43
        text: control.text
        color: control.enabled ? theme.text : theme.dimText
        font.family: backend.uiFont
        font.pixelSize: control.font.pixelSize
        verticalAlignment: Text.AlignVCenter
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
