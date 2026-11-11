import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    signal commandRequested(string command)
    signal promptRequested(string prompt)
    signal openSessions()

    readonly property int contentWidth: Math.max(320, Math.min(980, width - 64))
    readonly property int cardWidth: contentWidth >= 760 ? Math.floor((contentWidth - 12) / 2) : contentWidth

    function refresh(rescanTools) {
        if (rescanTools)
            ecosystem.rescan()
        else
            ecosystem.refresh(backend.workspace)
        backend.refreshGit()
        mcp.refresh()
        if (backend.opencodeState === "ready")
            backend.refreshSessions()
    }

    function availableTools() {
        let count = 0
        for (let i = 0; i < ecosystem.tools.length; ++i) {
            if (ecosystem.tools[i].available)
                count++
        }
        return count
    }

    function mcpStatusLabel(status) {
        if (status === "connected")
            return qsTr("Connected")
        if (status === "failed")
            return qsTr("Failed")
        if (status === "pending")
            return qsTr("Pending")
        if (status === "disabled")
            return qsTr("Disabled")
        return qsTr("Not tested")
    }

    function mcpColor(status) {
        if (status === "connected")
            return theme.green
        if (status === "failed")
            return theme.red
        if (status === "disabled")
            return theme.dimText
        return theme.yellow
    }

    function contextBrief() {
        let text = ecosystem.brief()
        if (backend.gitAvailable) {
            const state = backend.gitDirtyCount > 0
                    ? qsTr("%n change(s)", "", backend.gitDirtyCount)
                    : qsTr("clean")
            text += "\n" + qsTr("- Git: %1").arg(backend.gitBranch + " · " + state)
        }
        if (backend.currentModel !== "")
            text += "\n" + qsTr("- OpenCode model: %1 · %2").arg(backend.currentModel).arg(backend.currentMode)
        if (mcp.servers.length > 0)
            text += "\n" + qsTr("- MCP servers: %n", "", mcp.servers.length)
        return text
    }

    function recipePrompt(id) {
        if (id === "explain")
            return qsTr("Explain this project's architecture, its main modules and the data flow between them. Cite the important files.")
        if (id === "review")
            return qsTr("Review my uncommitted changes: look for bugs, regressions, edge cases and security issues. Propose concrete fixes.")
        if (id === "test")
            return qsTr("Look at the project and propose the most useful missing tests. Write them, then run the test suite.")
        if (id === "fix")
            return qsTr("Reproduce the problem I'm about to describe, find the root cause, fix it, and verify with a real test or command.")
        if (id === "security")
            return qsTr("Audit the project's security: unvalidated input, secrets, permissions, dependencies and execution paths. Rank risks by severity.")
        if (id === "perf")
            return qsTr("Find the project's performance bottlenecks. Measure where possible, then propose optimizations ranked by impact.")
        return ""
    }

    function useSkill(skill) {
        promptRequested(qsTr("Use the « %1 » skill from %2 to help me on this project. Start by telling me what it brings and what you need.").arg(skill.name).arg(skill.path))
    }

    function useSession(session) {
        backend.loadSession(session.sessionId, backend.workspace)
        backend.setView("opencode")
    }

    Component.onCompleted: if (visible) Qt.callLater(refresh)
    onVisibleChanged: if (visible) refresh()

    ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Column {
            id: content
            width: root.contentWidth
            x: Math.max(24, (scroll.availableWidth - width) / 2)
            topPadding: 26
            bottomPadding: 32
            spacing: 14

            RowLayout {
                width: parent.width
                spacing: 14

                Item {
                    width: 56
                    height: 56
                    Layout.alignment: Qt.AlignTop

                    Rectangle {
                        anchors.centerIn: parent
                        width: 54
                        height: 54
                        radius: 27
                        color: theme.accent
                        opacity: theme.dark ? 0.12 : 0.07
                    }
                    ThemedIcon {
                        anchors.centerIn: parent
                        source: "icons/ecosystem.svg"
                        width: 25
                        height: 25
                        color: theme.accent
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3

                    Text {
                        text: qsTr("Ecosystem")
                        color: theme.brightText
                        font.pixelSize: 25
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("The project, the tools it can use, and the skills that know how to use them.")
                        color: theme.mutedText
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                    }
                }

                Button {
                    text: qsTr("Refresh")
                    flat: true
                    onClicked: root.refresh(true)
                }
            }

            Flow {
                width: parent.width
                spacing: 6

                Repeater {
                    model: [
                        { "label": ecosystem.project.label !== undefined ? ecosystem.project.label : qsTr("workspace") },
                        { "label": qsTr("%n tool(s) ready", "", root.availableTools()) },
                        { "label": qsTr("%n skill(s)", "", ecosystem.skills.length) },
                        { "label": qsTr("%n model(s)", "", backend.modelOptions.length) },
                        { "label": qsTr("%n MCP server(s)", "", mcp.servers.length) }
                    ]

                    Rectangle {
                        width: chipLabel.implicitWidth + 18
                        height: 24
                        radius: 12
                        color: theme.chip
                        border.width: 1
                        border.color: theme.border
                        Text {
                            id: chipLabel
                            anchors.centerIn: parent
                            text: modelData.label
                            color: theme.mutedText
                            font.pixelSize: 9
                        }
                    }
                }
            }

            Flow {
                width: parent.width
                spacing: 12

                HomeCard {
                    title: qsTr("Project")
                    icon: "icons/branch.svg"
                    accent: theme.green
                    cardWidth: root.cardWidth

                    Text {
                        Layout.fillWidth: true
                        text: ecosystem.project.label !== undefined ? ecosystem.project.label : qsTr("Scanning…")
                        color: theme.text
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                    }
                    Text {
                        Layout.fillWidth: true
                        text: ecosystem.project.workspace !== undefined ? ecosystem.project.workspace : backend.workspace
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: 9
                        elide: Text.ElideMiddle
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 5

                        Repeater {
                            model: ecosystem.project.commands !== undefined ? ecosystem.project.commands : []

                            Button {
                                text: modelData.label
                                flat: true
                                font.pixelSize: 10
                                onClicked: root.commandRequested(modelData.command)
                            }
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: ecosystem.project.files !== undefined
                        text: ecosystem.project.files !== undefined ? ecosystem.project.files.join(" · ") : ""
                        color: theme.dimText
                        font.pixelSize: 8
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Commands run in the integrated terminal.")
                        color: theme.dimText
                        font.pixelSize: 8
                    }
                }

                HomeCard {
                    title: qsTr("Context and recipes")
                    icon: "icons/spark.svg"
                    accent: theme.cyan
                    cardWidth: root.cardWidth

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Give the agent the state of the workspace in one click, or start from a proven workflow.")
                        color: theme.mutedText
                        font.pixelSize: 10
                        wrapMode: Text.Wrap
                    }

                    ActionButton {
                        text: qsTr("Inject project context")
                        onClicked: root.promptRequested(root.contextBrief())
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 5

                        Repeater {
                            model: [
                                { "id": "explain", "label": qsTr("Explain") },
                                { "id": "review", "label": qsTr("Review") },
                                { "id": "test", "label": qsTr("Tests") },
                                { "id": "fix", "label": qsTr("Fix") },
                                { "id": "security", "label": qsTr("Security") },
                                { "id": "perf", "label": qsTr("Performance") }
                            ]

                            Button {
                                text: modelData.label
                                flat: true
                                font.pixelSize: 10
                                onClicked: root.promptRequested(root.recipePrompt(modelData.id))
                            }
                        }
                    }
                }

                HomeCard {
                    title: qsTr("Tools")
                    icon: "icons/terminal.svg"
                    accent: theme.blue
                    cardWidth: root.cardWidth

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("%n of %1 installed", "", root.availableTools()).arg(ecosystem.tools.length)
                        color: theme.dimText
                        font.pixelSize: 9
                    }

                    Repeater {
                        model: ecosystem.tools

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7

                            StatusDot {
                                color: modelData.available ? theme.green : theme.dimText
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.label
                                color: modelData.available ? theme.text : theme.dimText
                                font.pixelSize: 9
                            }
                            Text {
                                visible: modelData.version !== ""
                                text: modelData.version
                                color: theme.dimText
                                font.pixelSize: 8
                                elide: Text.ElideRight
                                Layout.maximumWidth: 210
                            }
                        }
                    }
                }

                HomeCard {
                    title: qsTr("Skills")
                    icon: "icons/spark.svg"
                    accent: theme.magenta
                    cardWidth: root.cardWidth

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("%n available", "", ecosystem.skills.length)
                        color: theme.dimText
                        font.pixelSize: 9
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: ecosystem.skills.length === 0
                        text: qsTr("No skill found in ~/.agents/skills.")
                        color: theme.mutedText
                        font.pixelSize: 9
                        wrapMode: Text.Wrap
                    }

                    Repeater {
                        model: ecosystem.skills.slice(0, 6)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    color: theme.text
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.description
                                    color: theme.dimText
                                    font.pixelSize: 8
                                    elide: Text.ElideRight
                                }
                            }
                            Button {
                                text: qsTr("Use")
                                flat: true
                                font.pixelSize: 9
                                onClicked: root.useSkill(modelData)
                            }
                        }
                    }
                }

                HomeCard {
                    title: qsTr("Models")
                    icon: "icons/spark.svg"
                    accent: theme.orange
                    cardWidth: root.cardWidth

                    Text {
                        Layout.fillWidth: true
                        text: backend.currentModel !== "" ? backend.currentModel : qsTr("No model selected")
                        color: theme.text
                        font.family: backend.monoFont
                        font.pixelSize: 10
                        elide: Text.ElideMiddle
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("%n available · mode %1", "", backend.modelOptions.length).arg(backend.currentMode)
                        color: theme.dimText
                        font.pixelSize: 9
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 5

                        Repeater {
                            model: backend.modelOptions.slice(0, 8)

                            Button {
                                text: modelData.name
                                flat: true
                                font.pixelSize: 9
                                enabled: backend.opencodeState === "ready"
                                onClicked: backend.setConfigOption("model", modelData.value)
                            }
                        }
                    }
                }

                HomeCard {
                    title: qsTr("MCP and sessions")
                    icon: "icons/plug.svg"
                    accent: theme.green
                    cardWidth: root.cardWidth

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("%n MCP server(s)", "", mcp.servers.length)
                        color: theme.dimText
                        font.pixelSize: 9
                    }
                    Repeater {
                        model: mcp.servers.slice(0, 4)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7
                            StatusDot {
                                color: root.mcpColor(modelData.status)
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.name
                                color: theme.text
                                font.pixelSize: 9
                                elide: Text.ElideRight
                            }
                            Text {
                                text: root.mcpStatusLabel(modelData.status)
                                color: root.mcpColor(modelData.status)
                                font.pixelSize: 8
                            }
                        }
                    }
                    Button {
                        text: qsTr("Manage MCP")
                        flat: true
                        onClicked: backend.setView("settings")
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: theme.border
                    }

                    Repeater {
                        model: backend.sessions.slice(0, 3)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 7
                            Text {
                                Layout.fillWidth: true
                                text: backend.sessionLabel(modelData)
                                color: theme.mutedText
                                font.pixelSize: 9
                                elide: Text.ElideRight
                            }
                            Button {
                                text: qsTr("Open")
                                flat: true
                                font.pixelSize: 8
                                onClicked: root.useSession(modelData)
                            }
                        }
                    }
                    Button {
                        text: qsTr("All sessions")
                        flat: true
                        onClicked: root.openSessions()
                    }
                }
            }
        }
    }
}
