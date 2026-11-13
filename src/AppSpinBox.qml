import QtQuick
import QtQuick.Controls

// Numeric stepper with the same surface language as AppComboBox. Replaces the
// Material SpinBox so the up/down controls never look like native gray blocks.
SpinBox {
    id: control

    implicitHeight: design.controlHeight
    implicitWidth: 118
    editable: true
    font.pixelSize: design.textLabel
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        radius: design.radiusSm
        color: control.activeFocus ? theme.chip : theme.background
        border.width: 1
        border.color: control.activeFocus ? theme.accent : theme.border
        Behavior on color { ColorAnimation { duration: design.durationFast } }
        Behavior on border.color { ColorAnimation { duration: design.durationFast } }
    }

    contentItem: TextInput {
        x: 34
        width: control.width - 68
        height: control.height
        text: control.textFromValue(control.value, control.locale)
        color: control.enabled ? theme.text : theme.dimText
        font.family: backend.uiFont
        font.pixelSize: design.textLabel
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !control.editable
        selectByMouse: true
        inputMethodHints: Qt.ImhDigitsOnly
        onEditingFinished: control.value = control.valueFromText(text, control.locale)
    }

    down.indicator: Rectangle {
        x: 0
        y: 0
        width: 34
        height: control.height
        radius: design.radiusSm
        color: downMouse.containsMouse ? theme.chip : "transparent"
        Text {
            anchors.centerIn: parent
            text: "−"
            color: control.enabled ? theme.mutedText : theme.dimText
            font.pixelSize: 16
            font.family: backend.uiFont
        }
        MouseArea {
            id: downMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: control.decrease()
        }
    }

    up.indicator: Rectangle {
        x: control.width - width
        y: 0
        width: 34
        height: control.height
        radius: design.radiusSm
        color: upMouse.containsMouse ? theme.chip : "transparent"
        Text {
            anchors.centerIn: parent
            text: "+"
            color: control.enabled ? theme.mutedText : theme.dimText
            font.pixelSize: 16
            font.family: backend.uiFont
        }
        MouseArea {
            id: upMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: control.increase()
        }
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
