import QtQuick

// A palette-tinted icon. The actual SVG rendering happens in IconProvider, so
// the tint is exact on every theme and no QML effect is involved.
Item {
    id: root

    property string source: ""
    property color color: theme.text
    property real iconOpacity: 1

    implicitWidth: 18
    implicitHeight: 18

    readonly property string colorKey: {
        function hex(v) {
            const value = Math.round(v * 255).toString(16)
            return value.length < 2 ? "0" + value : value
        }
        return "#" + hex(root.color.r) + hex(root.color.g) + hex(root.color.b)
    }

    Image {
        anchors.fill: parent
        source: root.source === "" ? "" :
                "image://themeicon/" + root.source.substring(root.source.lastIndexOf("/") + 1) +
                "?color=" + encodeURIComponent(root.colorKey)
        fillMode: Image.PreserveAspectFit
        smooth: true
        opacity: root.iconOpacity
    }
}
