import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Shared page heading. No all-caps eyebrow: a small index marker, the display
// title and one plain-language description carry the hierarchy.
//
// Layout is deliberate rather than layout-managed: when the actions fit beside
// the title they stay on one line exactly as before, and when the window is too
// narrow for both they move to their own line under the title instead of being
// pushed past the right edge and clipped.
Item {
    id: root

    property string title: ""
    property string description: ""
    property string icon: ""
    property string index: ""
    property color accent: theme.accent
    property bool brandMark: false

    default property alias actions: actionFlow.data

    readonly property real iconBox: 42
    readonly property real gap: design.space4
    // Sum the action buttons explicitly: a Flow's own implicitWidth is not a
    // reliable single-row width, and basing the heading on it made the actions
    // column-stack and push the whole page down.
    readonly property real actionsWidth: {
        var total = 0
        var count = actionFlow.children.length
        for (var i = 0; i < count; ++i) {
            var child = actionFlow.children[i]
            if (!child.visible || child.implicitWidth <= 0)
                continue
            total += (total > 0 ? actionFlow.spacing : 0) + child.implicitWidth
        }
        return total
    }
    // The actions only leave the heading line when they would leave less than
    // this for the title. Kept small on purpose: pages that fitted before must
    // keep their exact previous layout.
    readonly property real titleMin: 96
    readonly property bool stacked: actionsWidth > 0
                                     && width - iconBox - gap - actionsWidth - gap < titleMin

    implicitWidth: parent ? parent.width : 800
    implicitHeight: stacked ? heading.height + design.space3 + actionFlow.height
                            : Math.max(58, heading.height)

    Item {
        id: heading
        width: root.stacked || root.actionsWidth === 0
               ? root.width
               : root.width - root.actionsWidth - root.gap
        height: Math.max(58, titleColumn.implicitHeight)

        Item {
            id: iconSlot
            width: root.iconBox
            height: root.iconBox
            anchors.top: parent.top
            Rectangle {
                anchors.fill: parent
                visible: !root.brandMark
                radius: design.radiusMd
                color: theme.raised
                border.width: 1
                border.color: theme.border
            }
            BrandMark {
                anchors.fill: parent
                visible: root.brandMark
                size: root.iconBox
                accent: false
            }
            ThemedIcon {
                anchors.centerIn: parent
                visible: !root.brandMark
                source: root.icon
                width: design.iconLg
                height: design.iconLg
                color: root.accent
            }
        }

        // Same title block as before: only the action placement is responsive,
        // so wide layouts keep their exact previous geometry.
        ColumnLayout {
            id: titleColumn
            anchors.left: iconSlot.right
            anchors.leftMargin: root.gap
            anchors.right: parent.right
            anchors.top: parent.top
            spacing: 3

            RowLayout {
                Layout.fillWidth: true
                spacing: design.space2
                Text {
                    text: root.index
                    visible: root.index !== ""
                    color: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: design.textCaption
                }
                Text {
                    Layout.fillWidth: true
                    text: root.title
                    color: theme.brightText
                    font.family: "iA Writer Duospace"
                    font.pixelSize: design.textDisplay
                    font.weight: Font.Bold
                    elide: Text.ElideRight
                }
            }
            Text {
                Layout.fillWidth: true
                visible: root.description !== ""
                text: root.description
                color: theme.mutedText
                font.pixelSize: design.textBody
                lineHeight: 1.25
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }
    }

    Flow {
        id: actionFlow
        width: root.width
        // Position from the placed children rather than a computed sum: a
        // summed implicit width is a couple of pixels short of what the buttons
        // actually occupy, which left the group slightly off the right edge.
        x: root.stacked ? 0 : Math.max(0, root.width - childrenRect.width)
        y: root.stacked ? heading.height + design.space3 : 0
        spacing: design.space2
    }
}
