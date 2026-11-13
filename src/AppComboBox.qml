import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Fully themed combo box. The native Material popup is intentionally replaced:
// menu surfaces follow the application theme, corners and hover are custom,
// while model/textRole/valueRole behavior remains the Qt ComboBox contract.
ComboBox {
    id: control

    implicitHeight: design.controlHeight
    font.pixelSize: design.textLabel
    leftPadding: design.space3
    rightPadding: design.space3
    focusPolicy: Qt.StrongFocus

    background: Rectangle {
        radius: design.radiusSm
        color: control.hovered || control.popup.visible ? theme.chip : theme.background
        border.width: control.activeFocus || control.popup.visible ? 1 : 1
        border.color: control.activeFocus || control.popup.visible ? theme.accent : theme.border
        Behavior on color { ColorAnimation { duration: design.durationFast } }
        Behavior on border.color { ColorAnimation { duration: design.durationFast } }
    }

    contentItem: Text {
        leftPadding: control.leftPadding
        rightPadding: control.indicator.width + design.space2
        text: control.displayText
        color: control.enabled ? theme.text : theme.dimText
        font.family: backend.uiFont
        font.pixelSize: control.font.pixelSize
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: ThemedIcon {
        x: control.width - width - design.space3
        y: (control.height - height) / 2
        width: 10
        height: 10
        source: "icons/chevron.svg"
        rotation: control.popup.visible ? 270 : 90
        color: control.popup.visible || control.hovered ? theme.accent : theme.dimText
        Behavior on rotation { NumberAnimation { duration: design.durationFast } }
    }

    delegate: ItemDelegate {
        id: item
        width: control.width - design.space2 * 2
        height: 34
        leftPadding: design.space3
        rightPadding: design.space3
        highlighted: control.highlightedIndex === index

        contentItem: Text {
            text: control.textRole
                  ? (Array.isArray(control.model) ? model[control.textRole] : modelData[control.textRole])
                  : modelData
            color: item.highlighted ? theme.brightText : theme.text
            font.family: backend.uiFont
            font.pixelSize: design.textLabel
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            radius: design.radiusSm
            color: item.highlighted ? theme.menuSelected
                 : (item.hovered ? theme.menuHover : "transparent")
            Behavior on color { ColorAnimation { duration: design.durationFast } }
        }

        HoverHandler { cursorShape: Qt.PointingHandCursor }
    }

    popup: Popup {
        y: control.height + 4
        width: control.width
        padding: design.space1
        implicitHeight: Math.min(contentItem.implicitHeight + design.space2, 330)
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight + design.space2
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            radius: design.radiusMd
            color: theme.raised
            border.width: 1
            border.color: theme.borderStrong
            layer.enabled: true
            layer.effect: null
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: design.durationFast }
            NumberAnimation { property: "scale"; from: 0.98; to: 1; duration: design.durationFast; easing.type: Easing.OutCubic }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: design.durationFast }
        }
    }

    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
