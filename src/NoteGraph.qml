import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property var graphData: ({ nodes: [], edges: [] })
    property string selectedPath: ""
    signal nodeActivated(string path)

    property var positions: []

    function rebuild() {
        var nodes = graphData.nodes !== undefined ? graphData.nodes : []
        var result = []
        var count = Math.max(1, nodes.length)
        var radius = Math.max(40, Math.min(width, height) * 0.37)
        for (var i = 0; i < nodes.length; ++i) {
            var angle = -Math.PI / 2 + (i / count) * Math.PI * 2
            result.push({
                path: nodes[i].id,
                title: nodes[i].title,
                x: width / 2 + Math.cos(angle) * radius,
                y: height / 2 + Math.sin(angle) * radius
            })
        }
        root.positions = result
        canvas.requestPaint()
    }

    function nodeAt(px, py) {
        for (var i = 0; i < positions.length; ++i) {
            var dx = positions[i].x - px
            var dy = positions[i].y - py
            if (dx * dx + dy * dy <= 20 * 20)
                return positions[i].path
        }
        return ""
    }

    onGraphDataChanged: rebuild()
    onWidthChanged: rebuild()
    onHeightChanged: rebuild()
    Component.onCompleted: rebuild()

    Canvas {
        id: canvas
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            var positions = root.positions
            var byPath = ({})
            for (var i = 0; i < positions.length; ++i)
                byPath[positions[i].path] = positions[i]

            ctx.strokeStyle = theme.border
            ctx.lineWidth = 1
            var edges = root.graphData.edges !== undefined ? root.graphData.edges : []
            for (var e = 0; e < edges.length; ++e) {
                var a = byPath[edges[e].source]
                var b = byPath[edges[e].target]
                if (!a || !b)
                    continue
                ctx.beginPath()
                ctx.moveTo(a.x, a.y)
                ctx.lineTo(b.x, b.y)
                ctx.stroke()
            }

            for (var n = 0; n < positions.length; ++n) {
                var node = positions[n]
                var active = node.path === root.selectedPath
                ctx.beginPath()
                ctx.arc(node.x, node.y, active ? 8 : 5, 0, Math.PI * 2)
                ctx.fillStyle = active ? theme.accent : theme.mutedText
                ctx.fill()
                ctx.strokeStyle = active ? theme.accent : theme.borderStrong
                ctx.lineWidth = active ? 2 : 1
                ctx.stroke()
                if (active) {
                    ctx.fillStyle = theme.text
                    ctx.font = "11px sans-serif"
                    ctx.fillText(node.title, node.x + 12, node.y + 4)
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: function (mouse) {
            var path = root.nodeAt(mouse.x, mouse.y)
            if (path !== "")
                root.nodeActivated(path)
        }
    }

    Text {
        anchors.centerIn: parent
        visible: root.positions.length === 0
        text: qsTr("Le graphe apparaîtra avec vos premières notes.")
        color: theme.dimText
        font.pixelSize: design.textCaption
    }
}
