import QtQuick
import QtQuick.Controls
import QtQuick.Window

// A compact DSH-style hover label. It is window-level so it cannot be clipped
// by the sidebar ScrollView, and it is positioned against its anchor rather
// than against the mouse pointer.
Popup {
    id: root

    property Item anchorItem: null
    property string text: ""

    parent: anchorItem
    modal: false
    focus: false
    dim: false
    closePolicy: Popup.NoAutoClose
    padding: 0
    width: Math.min(230, label.implicitWidth + design.space4)
    height: design.controlHeightSm
    visible: false

    readonly property real sceneX: anchorItem
        ? anchorItem.mapToItem(null, anchorItem.width, 0).x
        : 0
    readonly property real windowWidth: {
        const win = anchorItem && anchorItem.Window.window
        return win ? win.width : 0
    }

    x: {
        if (!anchorItem)
            return 0
        if (windowWidth > 0 && sceneX + width + design.space2 > windowWidth)
            return -width - design.space2
        return anchorItem.width + design.space2
    }
    y: anchorItem ? anchorItem.height + design.space1 : 0

    background: Rectangle {
        radius: design.radiusSm
        color: theme.raised
        border.width: 1
        border.color: theme.borderStrong
    }

    contentItem: Text {
        id: label
        anchors.fill: parent
        anchors.leftMargin: design.space2
        anchors.rightMargin: design.space2
        text: root.text
        color: theme.text
        font.pixelSize: design.textCaption
        font.family: backend.uiFont
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }
}
