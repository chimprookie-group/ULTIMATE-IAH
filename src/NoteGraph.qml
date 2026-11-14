import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Modern, responsive force-directed note graph.
// - organic settling animation, curved links, tag colours and glows
// - wheel zoom around the cursor, drag to pan, double-click to fit
// - search and tag filter, responsive auto-fit on resize
Item {
    id: root

    property var graphData: ({ nodes: [], edges: [] })
    property string selectedPath: ""
    property string hoverPath: ""
    property string searchText: ""
    property string tagFilter: ""
    property var nodes: []
    property var tags: []
    property real viewScale: 1.0
    property real panX: 0
    property real panY: 0
    property bool autoFit: true
    property bool running: false
    property int tickCount: 0

    signal nodeActivated(string path)

    readonly property var palette: [theme.accent, theme.green, theme.orange,
                                   theme.blue, theme.yellow, theme.red, theme.mutedText]

    function seed(value) {
        var hash = 2166136261
        for (var i = 0; i < value.length; ++i) {
            hash ^= value.charCodeAt(i)
            hash = Math.imul(hash, 16777619)
        }
        return Math.abs(hash % 10000) / 10000
    }

    function colorFor(tags) {
        if (!tags || tags.length === 0)
            return theme.mutedText
        return root.palette[Math.abs(String(tags[0]).length + tags[0].charCodeAt(0)) % root.palette.length]
    }

    function matches(node) {
        const query = root.searchText.trim().toLowerCase()
        const tag = root.tagFilter
        if (tag !== "" && (!node.tags || node.tags.indexOf(tag) < 0))
            return false
        if (query === "")
            return true
        if (String(node.title).toLowerCase().indexOf(query) >= 0)
            return true
        if (node.tags) {
            for (var i = 0; i < node.tags.length; ++i)
                if (String(node.tags[i]).toLowerCase().indexOf(query) >= 0)
                    return true
        }
        return false
    }

    function rebuild() {
        const source = graphData.nodes !== undefined ? graphData.nodes : []
        const edges = graphData.edges !== undefined ? graphData.edges : []
        const degree = ({})
        for (var e = 0; e < edges.length; ++e) {
            degree[edges[e].source] = (degree[edges[e].source] || 0) + 1
            degree[edges[e].target] = (degree[edges[e].target] || 0) + 1
        }
        const tagSet = ({})
        const result = []
        for (var i = 0; i < source.length; ++i) {
            const node = source[i]
            const angle = seed(node.id) * Math.PI * 2
            const radius = 120 + seed(node.title) * 180
            result.push({
                path: node.id,
                title: node.title,
                tags: node.tags !== undefined ? node.tags : [],
                degree: degree[node.id] || 0,
                x: Math.cos(angle) * radius,
                y: Math.sin(angle) * radius,
                color: colorFor(node.tags)
            })
            if (node.tags) {
                for (var t = 0; t < node.tags.length; ++t)
                    tagSet[node.tags[t]] = true
            }
        }
        var tagList = []
        for (var key in tagSet)
            tagList.push(key)
        tagList.sort()
        root.tags = tagList
        root.nodes = result
        root.tickCount = 0
        root.autoFit = true
        root.running = result.length > 0
        physics.stop()
        if (result.length > 0) {
            root.fit()
            physics.start()
        }
        canvas.requestPaint()
    }

    function tick() {
        const list = root.nodes
        const count = list.length
        if (count === 0) {
            physics.stop()
            return
        }
        const forces = []
        for (var i = 0; i < count; ++i)
            forces.push({ x: 0, y: 0 })
        for (var a = 0; a < count; ++a) {
            for (var b = a + 1; b < count; ++b) {
                var dx = list[a].x - list[b].x
                var dy = list[a].y - list[b].y
                var distance = Math.max(18, Math.sqrt(dx * dx + dy * dy))
                var push = 2600 / (distance * distance)
                forces[a].x += dx / distance * push
                forces[a].y += dy / distance * push
                forces[b].x -= dx / distance * push
                forces[b].y -= dy / distance * push
            }
        }
        const edges = root.graphData.edges !== undefined ? root.graphData.edges : []
        for (var e = 0; e < edges.length; ++e) {
            const source = list.find(function (node) { return node.path === edges[e].source })
            const target = list.find(function (node) { return node.path === edges[e].target })
            if (!source || !target)
                continue
            var ex = target.x - source.x
            var ey = target.y - source.y
            var ed = Math.max(1, Math.sqrt(ex * ex + ey * ey))
            var pull = (ed - 120) * 0.006
            source.x += ex / ed * pull
            source.y += ey / ed * pull
            target.x -= ex / ed * pull
            target.y -= ey / ed * pull
        }
        for (var k = 0; k < count; ++k) {
            list[k].x += Math.max(-12, Math.min(12, forces[k].x)) - list[k].x * 0.012
            list[k].y += Math.max(-12, Math.min(12, forces[k].y)) - list[k].y * 0.012
            list[k].x = Math.max(-520, Math.min(520, list[k].x))
            list[k].y = Math.max(-520, Math.min(520, list[k].y))
        }
        root.tickCount++
        if (root.autoFit && root.tickCount % 24 === 0)
            root.fit()
        if (root.tickCount > 180) {
            physics.stop()
            root.running = false
            root.settle()
        }
        canvas.requestPaint()
    }

    // Gently recentre and fit once the simulation has cooled down.
    function settle() {
        if (root.autoFit)
            root.fit()
        canvas.requestPaint()
    }

    function bounds() {
        if (root.nodes.length === 0)
            return { minX: -1, minY: -1, maxX: 1, maxY: 1 }
        var minX = root.nodes[0].x, maxX = root.nodes[0].x
        var minY = root.nodes[0].y, maxY = root.nodes[0].y
        for (var i = 1; i < root.nodes.length; ++i) {
            minX = Math.min(minX, root.nodes[i].x)
            maxX = Math.max(maxX, root.nodes[i].x)
            minY = Math.min(minY, root.nodes[i].y)
            maxY = Math.max(maxY, root.nodes[i].y)
        }
        return { minX: minX, minY: minY, maxX: maxX, maxY: maxY }
    }

    function fit() {
        if (root.width <= 0 || root.height <= 0 || root.nodes.length === 0)
            return
        const box = root.bounds()
        const padding = Math.max(56, Math.min(120, root.width * 0.08))
        const spanX = Math.max(60, box.maxX - box.minX)
        const spanY = Math.max(60, box.maxY - box.minY)
        root.viewScale = Math.max(0.35, Math.min(2.4,
            Math.min((root.width - padding * 2) / spanX,
                     (root.height - padding * 2) / spanY)))
        root.panX = root.width / 2 - ((box.minX + box.maxX) / 2) * root.viewScale
        root.panY = root.height / 2 - ((box.minY + box.maxY) / 2) * root.viewScale
        root.autoFit = true
        canvas.requestPaint()
    }

    function screenX(node) { return node.x * root.viewScale + root.panX }
    function screenY(node) { return node.y * root.viewScale + root.panY }
    function nodeRadius(node) {
        return (5.5 + Math.min(9, node.degree * 1.5)) * Math.max(0.8, Math.min(1.6, root.viewScale))
    }

    function nodeAt(px, py) {
        for (var i = root.nodes.length - 1; i >= 0; --i) {
            const node = root.nodes[i]
            const dx = root.screenX(node) - px
            const dy = root.screenY(node) - py
            const radius = root.nodeRadius(node) + 6
            if (dx * dx + dy * dy <= radius * radius)
                return node.path
        }
        return ""
    }

    function zoomAt(factor, cx, cy) {
        const next = Math.max(0.3, Math.min(4.0, root.viewScale * factor))
        const ratio = next / root.viewScale
        root.panX = cx - (cx - root.panX) * ratio
        root.panY = cy - (cy - root.panY) * ratio
        root.viewScale = next
        root.autoFit = false
        canvas.requestPaint()
    }

    onWidthChanged: if (root.autoFit) root.fit()
    onHeightChanged: if (root.autoFit) root.fit()
    onGraphDataChanged: root.rebuild()
    Component.onCompleted: root.rebuild()

    Timer {
        id: physics
        interval: 12
        repeat: true
        running: false
        onTriggered: root.tick()
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const w = width
            const h = height
            const background = ctx.createRadialGradient(w * 0.5, h * 0.42, 20, w * 0.5, h * 0.5, Math.max(w, h) * 0.8)
            background.addColorStop(0, "#171a21")
            background.addColorStop(1, "#0b0d11")
            ctx.fillStyle = background
            ctx.fillRect(0, 0, w, h)

            const step = 34 * root.viewScale
            if (step > 14) {
                ctx.fillStyle = "rgba(255,255,255,0.035)"
                const startX = root.panX % step
                const startY = root.panY % step
                for (var gx = startX; gx < w; gx += step)
                    for (var gy = startY; gy < h; gy += step)
                        ctx.fillRect(gx, gy, 1.2, 1.2)
            }

            const byPath = ({})
            for (var i = 0; i < root.nodes.length; ++i)
                byPath[root.nodes[i].path] = root.nodes[i]
            const edges = root.graphData.edges !== undefined ? root.graphData.edges : []
            const highlighted = {}
            if (root.selectedPath !== "") {
                highlighted[root.selectedPath] = true
                for (var he = 0; he < edges.length; ++he) {
                    if (edges[he].source === root.selectedPath)
                        highlighted[edges[he].target] = true
                    if (edges[he].target === root.selectedPath)
                        highlighted[edges[he].source] = true
                }
            }

            for (var e = 0; e < edges.length; ++e) {
                const source = byPath[edges[e].source]
                const target = byPath[edges[e].target]
                if (!source || !target)
                    continue
                const active = edges[e].source === root.selectedPath || edges[e].target === root.selectedPath
                const dimmed = (root.searchText !== "" || root.tagFilter !== "")
                                 && (!root.matches(source) || !root.matches(target))
                const ax = root.screenX(source), ay = root.screenY(source)
                const bx = root.screenX(target), by = root.screenY(target)
                const mx = (ax + bx) / 2, my = (ay + by) / 2
                const dx = bx - ax, dy = by - ay
                const length = Math.max(1, Math.sqrt(dx * dx + dy * dy))
                const bend = Math.min(38, length * 0.16)
                const cx = mx - dy / length * bend
                const cy = my + dx / length * bend
                ctx.beginPath()
                ctx.moveTo(ax, ay)
                ctx.quadraticCurveTo(cx, cy, bx, by)
                if (active) {
                    ctx.strokeStyle = "rgba(255,214,10,0.22)"
                    ctx.lineWidth = 6
                    ctx.stroke()
                }
                ctx.strokeStyle = active ? theme.accent
                                         : (dimmed ? "rgba(255,255,255,0.05)" : "rgba(255,255,255,0.16)")
                ctx.lineWidth = active ? 1.8 : 1
                ctx.stroke()
            }

            for (var n = 0; n < root.nodes.length; ++n) {
                const node = root.nodes[n]
                const x = root.screenX(node), y = root.screenY(node)
                const isSelected = node.path === root.selectedPath
                const isHovered = node.path === root.hoverPath
                const isMatch = root.matches(node)
                const alpha = isMatch ? 1 : 0.16
                const radius = root.nodeRadius(node)

                if (isSelected || isHovered) {
                    const glow = ctx.createRadialGradient(x, y, 1, x, y, radius * 3.4)
                    glow.addColorStop(0, "rgba(255,214,10,0.28)")
                    glow.addColorStop(1, "rgba(255,214,10,0)")
                    ctx.fillStyle = glow
                    ctx.beginPath()
                    ctx.arc(x, y, radius * 3.4, 0, Math.PI * 2)
                    ctx.fill()
                }

                ctx.globalAlpha = alpha
                ctx.beginPath()
                ctx.arc(x, y, radius, 0, Math.PI * 2)
                ctx.fillStyle = isSelected ? theme.accent : node.color
                ctx.fill()
                ctx.lineWidth = isSelected || isHovered ? 2.2 : 1.2
                ctx.strokeStyle = isSelected ? theme.text
                                             : (isHovered ? "rgba(255,255,255,0.85)" : "rgba(0,0,0,0.35)")
                ctx.stroke()
                ctx.globalAlpha = 1

                if (isSelected || isHovered || node.degree >= 2 || root.viewScale > 1.4) {
                    ctx.font = (isSelected ? "600 11px" : "10px") + " sans-serif"
                    const label = node.title
                    const textWidth = ctx.measureText(label).width
                    ctx.globalAlpha = isMatch ? 1 : 0.2
                    ctx.fillStyle = "rgba(10,12,16,0.78)"
                    const left = x + radius + 6
                    const top = y - 8
                    const radiusPill = 6
                    ctx.beginPath()
                    ctx.moveTo(left + radiusPill, top)
                    ctx.lineTo(left + textWidth + 12 - radiusPill, top)
                    ctx.quadraticCurveTo(left + textWidth + 12, top, left + textWidth + 12, top + radiusPill)
                    ctx.lineTo(left + textWidth + 12, top + 17 - radiusPill)
                    ctx.quadraticCurveTo(left + textWidth + 12, top + 17, left + textWidth + 12 - radiusPill, top + 17)
                    ctx.lineTo(left + radiusPill, top + 17)
                    ctx.quadraticCurveTo(left, top + 17, left, top + 17 - radiusPill)
                    ctx.lineTo(left, top + radiusPill)
                    ctx.quadraticCurveTo(left, top, left + radiusPill, top)
                    ctx.closePath()
                    ctx.fill()
                    ctx.fillStyle = isSelected ? theme.accent : theme.text
                    ctx.fillText(label, left + 6, y + 4)
                    ctx.globalAlpha = 1
                }
            }
        }
    }

    MouseArea {
        id: surface
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        property real lastX: 0
        property real lastY: 0
        property bool moved: false

        onPressed: function (mouse) {
            lastX = mouse.x
            lastY = mouse.y
            moved = false
        }
        onPositionChanged: function (mouse) {
            if (pressed) {
                const dx = mouse.x - lastX
                const dy = mouse.y - lastY
                if (Math.abs(dx) + Math.abs(dy) > 2) {
                    root.panX += dx
                    root.panY += dy
                    root.autoFit = false
                    moved = true
                    lastX = mouse.x
                    lastY = mouse.y
                    canvas.requestPaint()
                }
                return
            }
            const path = root.nodeAt(mouse.x, mouse.y)
            if (path !== root.hoverPath) {
                root.hoverPath = path
                canvas.requestPaint()
            }
        }
        onClicked: function (mouse) {
            if (moved)
                return
            const path = root.nodeAt(mouse.x, mouse.y)
            if (path !== "")
                root.nodeActivated(path)
        }
        onDoubleClicked: root.fit()
    }

    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: function (event) {
            const factor = event.angleDelta.y > 0 ? 1.12 : 0.89
            root.zoomAt(factor, event.x, event.y)
        }
    }

    // Glass toolbar, responsive: tags hide on narrow widths.
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 12
        width: Math.min(root.width - 24, toolbar.implicitWidth + 20)
        height: toolbar.implicitHeight + 14
        radius: 12
        color: Qt.rgba(0.05, 0.06, 0.08, 0.82)
        border.width: 1
        border.color: "#1AFFFFFF"

        RowLayout {
            id: toolbar
            anchors.fill: parent
            anchors.margins: 7
            spacing: 6

            Rectangle {
                Layout.preferredWidth: Math.min(190, Math.max(90, root.width * 0.24))
                Layout.preferredHeight: 28
                radius: 8
                color: "#0FFFFFFF"
                border.width: 1
                border.color: search.activeFocus ? theme.accent : "#14FFFFFF"
                TextField {
                    id: search
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 6
                    placeholderText: qsTr("Filtrer…")
                    text: root.searchText
                    color: theme.text
                    font.pixelSize: 10
                    background: null
                    onTextChanged: root.searchText = text
                }
            }

            ComboBox {
                visible: root.width > 620
                Layout.preferredWidth: 120
                Layout.preferredHeight: 28
                model: [qsTr("Tous les tags")].concat(root.tags)
                currentIndex: root.tagFilter === "" ? 0 : Math.max(0, root.tags.indexOf(root.tagFilter) + 1)
                onActivated: root.tagFilter = index === 0 ? "" : root.tags[index - 1]
            }

            Item { Layout.fillWidth: true }

            Text {
                text: root.nodes.length + qsTr(" notes")
                color: theme.dimText
                font.family: backend.monoFont
                font.pixelSize: 9
            }

            IconButton { implicitWidth: 26; implicitHeight: 26; icon: "icons/minus.svg"; hint: qsTr("Dézoomer"); onClicked: root.zoomAt(0.85, root.width / 2, root.height / 2) }
            IconButton { implicitWidth: 26; implicitHeight: 26; icon: "icons/plus.svg"; hint: qsTr("Zoomer"); onClicked: root.zoomAt(1.18, root.width / 2, root.height / 2) }
            IconButton { implicitWidth: 26; implicitHeight: 26; icon: "icons/target.svg"; hint: qsTr("Ajuster"); onClicked: root.fit() }
        }
    }

    Text {
        anchors.centerIn: parent
        visible: root.nodes.length === 0
        text: qsTr("Le graphe apparaîtra avec vos premières notes.")
        color: theme.dimText
        font.pixelSize: design.textCaption
    }

    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 14
        visible: root.nodes.length > 0
        text: qsTr("Molette : zoom · glisser : déplacer · double-clic : ajuster")
        color: "#59FFFFFF"
        font.pixelSize: 9
    }
}
