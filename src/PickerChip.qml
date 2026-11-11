import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window


// The visible half of the model/mode picker: a quiet pill that opens a bigger
// ChoiceMenu above it. Long provider prefixes stay in the menu; the pill shows
// the name the user actually recognizes.
Item {
    id: root

    property string title: ""
    property var options: []
    property string currentValue: ""
    property bool searchable: true
    property string emptyText: ""
    property color dotColor: theme.cyan

    signal selected(string value)
    signal opened()

    readonly property var hostWindow: Window.window

    implicitWidth: chip.width
    implicitHeight: chip.height

    readonly property var currentOption: {
        for (let i = 0; i < options.length; ++i)
            if (options[i].value === currentValue)
                return options[i]
        return null
    }

    readonly property string labelText: {
        if (!currentOption)
            return emptyText
        const name = String(currentOption.name !== undefined ? currentOption.name : currentOption.value)
        const slash = name.indexOf("/")
        return slash > 0 ? name.slice(slash + 1) : name
    }

    function open() {
        // Keep the panel inside the window when the chip sits near the right
        // edge (the mode chip is the usual suspect).
        if (hostWindow) {
            const globalX = root.mapToItem(null, 0, 0).x
            menu.x = Math.min(0, hostWindow.width - menu.width - 16 - globalX)
        }
        menu.open()
    }

    Rectangle {
        id: chip
        width: Math.min(230, pillRow.implicitWidth + 26)
        height: 26
        radius: 13
        color: mouse.containsMouse || menu.visible ? theme.chipHover : theme.chip
        border.width: 1
        border.color: mouse.containsMouse || menu.visible ? theme.borderStrong : theme.border

        RowLayout {
            id: pillRow
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 8
            spacing: 6

            Rectangle {
                width: 5
                height: 5
                radius: 2.5
                color: root.dotColor
            }

            Text {
                id: label
                Layout.maximumWidth: 170
                text: root.labelText
                color: theme.text
                font.pixelSize: 11
                elide: Text.ElideRight
            }

            ThemedIcon {
                source: "icons/chevron.svg"
                width: 9
                height: 9
                color: theme.mutedText
            }
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.open()
        }

        ToolTip.visible: mouse.containsMouse && root.currentValue !== ""
        ToolTip.text: root.currentValue
        ToolTip.delay: 700
    }

    ChoiceMenu {
        id: menu
        title: root.title
        options: root.options
        currentValue: root.currentValue
        searchable: root.searchable
        hostWindow: root.hostWindow
        onOpened: root.opened()
        onSelected: root.selected(value)
    }
}
