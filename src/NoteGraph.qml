import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property var graphData: ({ nodes: [], edges: [] })
    property string selectedPath: ""
    property string hoverPath: ""
    signal nodeActivated(string path)

    property var positions: []
    property var degrees: ({})

    function seed(value) {
        var hash = 2166136261
        for (var i = 0; i < value.length; ++i) {
            hash ^= value.charCodeAt(i)
            hash = Math.imul(hash, 16777619)
        }
        return Math.abs(hash % 1000) / 1000
    }

    function rebuild() {
        var nodes = graphData.nodes !== undefined ? graphData.nodes : []
        var edges = graphData.edges !== undefined ? graphData.edges : []
        var count = nodes.length
        var result = []
        var degree = ({})
        for (var i = 0; i < count; ++i) {
            result.push({
                path: nodes[i].id,
                title: nodes[i].title,
                tags: nodes[i].tags,
                x: width * (0.2 + seed(nodes[i].id) * 0.6),
                y: height * (0.2 + seed(nodes[i].title + "y") * 0.6)
            })
        }
        for (var e = 0; e < edges.length; ++e) {
            degree[edges[e].source] = (degree[edges[e].source] || 0) + 1
            degree[edges[e].target] = (degree[edges[e].target] || 0) + 1
        }
        var byPath = ({})
        for (var p = 0; p < result.length; ++p)
            byPath[result[p].path] = result[p]

        var iterations = Math.min(90, 24 + count * 3)
        for (var iter = 0; iter < iterations; ++iter) {
            var force = []
            for (var n = 0; n < count; ++n)
                force.push({ x: 0, y: 0 })
            for (var a = 0; a < count; ++a) {
                for (var b = a + 1; b < count; ++b) {
                    var dx = result[a].x - result[b].x
                    var dy = result[a].y - result[b].y
                    var distance = Math.max(36, Math.sqrt(dx * dx + dy * dy))
                    var push = 420 / (distance * distance)
                    force[a].x += dx / distance * push
                    force[a].y += dy / distance * push
                    force[b].x -= dx / distance * push
                    force[b].y -= dy / distance * push
                }
            }
            for (var edge = 0; edge < edges.length; ++edge) {
                var source = byPath[edges[edge].source]
                var target = byPath[edges[edge].target]
                if (!source || !target)
                    continue
                var ex = target.x - source.x
                var ey = target.y - source.y
                var ed = Math.max(1, Math.sqrt(ex * ex + ey * ey))
                var pull = (ed - 150) * 0.012
                var si = result.indexOf(source)
                var ti = result.indexOf(target)
                if (si >= 0 && ti >= 0) {
                    force[si].x += ex / ed * pull
                    force[si].y += ey / ed * pull
                    force[ti].x -= ex / ed * pull
                    force[ti].y -= ey / ed * pull
                }
            }
            for (var k = 0; k < count; ++k) {
                result[k].x = Math.max(42, Math.min(width - 42, result[k].x + force[k].x))
                result[k].y = Math.max(42, Math.min(height - 42, result[k].y + force[k].y))
            }
        }
        root.degrees = degree
        root.positions = result
        canvas.requestPaint()
    }

    function nodeAt(px, py) {
        for (var i = 0; i < positions.length; ++i) {
            var dx = positions[i].x - px
            var dy = positions[i].y - py
            var radius = 8 + Math.min(8, (degrees[positions[i].path] || 0) * 1.5)
            if (dx * dx + dy * dy <= radius * radius)
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
            var byPath = ({})
            for (var i = 0; i < root.positions.length; ++i)
                byPath[root.positions[i].path] = root.positions[i]

            var edges = root.graphData.edges !== undefined ? root.graphData.edges : []
            for (var e = 0; e < edges.length; ++e) {
                var a = byPath[edges[e].source]
                var b = byPath[edges[e].target]
                if (!a || !b)
                    continue
                var active = a.path === root.selectedPath || b.path === root.selectedPath
                ctx.beginPath()
                ctx.moveTo(a.x, a.y)
                ctx.lineTo(b.x, b.y)
                ctx.strokeStyle = active ? theme.accent : theme.border
                ctx.lineWidth = active ? 1.4 : 0.7
                ctx.stroke()
            }

            for (var n = 0; n < root.positions.length; ++n) {
                var node = root.positions[n]
                var isSelected = node.path === root.selectedPath
                var isHovered = node.path === root.hoverPath
                var degree = root.degrees[node.path] || 0
                var radius = 6 + Math.min(9, degree * 1.4)
                ctx.beginPath()
                ctx.arc(node.x, node.y, isSelected ? radius + 2 : radius, 0, Math.PI * 2)
                ctx.fillStyle = isSelected ? theme.accent : (isHovered ? theme.text : theme.mutedText)
                ctx.fill()
                ctx.strokeStyle = isSelected ? theme.accent : theme.borderStrong
                ctx.lineWidth = isSelected || isHovered ? 2 : 1
                ctx.stroke()
                ctx.fillStyle = isSelected || isHovered ? theme.text : theme.dimText
                ctx.font = (isSelected ? "600 " : "") + (isSelected ? "11px" : "9px") + " sans-serif"
                ctx.fillText(node.title, node.x + radius + 5, node.y + 3)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onPositionChanged: function (mouse) {
            var path = root.nodeAt(mouse.x, mouse.y)
            if (path !== root.hoverPath) {
                root.hoverPath = path
                canvas.requestPaint()
            }
        }
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
