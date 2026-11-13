import QtQuick
import QtQuick.Controls

// Themed checkbox: compact rounded box, accent fill and a clear check mark.
CheckBox {
    id: control

    implicitHeight: 28
    spacing: design.space2
    font.pixelSize: design.textCaption
    focusPolicy: Qt.StrongFocus

    indicator: Rectangle {
        x: 1
        y: (control.height - height) / 2
        width: 17
        height: 17
        radius: 5
        color: control.checked ? theme.accent : (control.hovered ? theme.chip : theme.raised)
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? theme.brightText : (control.checked ? theme.accent : theme.mutedText)
        Behavior on color { ColorAnimation { duration: design.durationFast } }
        Behavior on border.color { ColorAnimation { duration: design.durationFast } }

        ThemedIcon {
            anchors.centerIn: parent
            visible: control.checked
            source: "icons/check.svg"
            width: 11
            height: 11
            color: theme.accentForeground
        }
    }

    contentItem: Text {
        leftPadding: 23
        text: control.text
        color: control.enabled ? theme.text : theme.dimText
        font.family: backend.uiFont
        font.pixelSize: control.font.pixelSize
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
