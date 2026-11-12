import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: root

    property bool compact: false
    property bool closeButton: false
    property bool expandButton: false
    signal closeRequested()
    signal expandRequested()

    readonly property int contentWidth: Math.max(320, Math.min(920, width - (compact ? design.space4 * 2 : design.space7)))
    readonly property bool empty: ultimateAgent.messages.length === 0
    readonly property bool canSend: composerField.text.trim() !== "" || ultimateAgent.busy

    function send(text) {
        const value = String(text).trim()
        if (value === "" || ultimateAgent.busy)
            return
        ultimateAgent.sendMessage(value)
    }

    function quick(text) {
        root.send(text)
    }

    function scrollToBottom() {
        Qt.callLater(function () {
            if (messageList.count > 0)
                messageList.positionViewAtEnd()
        })
    }

    Component.onCompleted: if (visible) ultimateAgent.refreshEndpoint()
    onVisibleChanged: if (visible) ultimateAgent.refreshEndpoint()

    Connections {
        target: ultimateAgent
        function onMessagesChanged() {
            root.scrollToBottom()
            if (root.empty)
                Qt.callLater(function () { welcomeScroll.contentY = 0 })
        }
        function onBusyChanged() { if (!ultimateAgent.busy) root.scrollToBottom() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── panel header ────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.compact ? 58 : 62
            color: theme.surface
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: theme.border }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: root.compact ? design.space3 : design.space5
                anchors.rightMargin: root.compact ? design.space2 : design.space4
                spacing: design.space2

                BrandMark {
                    Layout.alignment: Qt.AlignVCenter
                    width: root.compact ? 24 : 28
                    height: width
                    size: root.compact ? 24 : 28
                    accent: false
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1
                    RowLayout {
                        spacing: design.space2
                        Text {
                            text: qsTr("Ultimate Agent")
                            color: theme.brightText
                            font.pixelSize: root.compact ? design.textLabel : design.textBody
                            font.weight: Font.DemiBold
                        }
                        StatusDot {
                            size: 5
                            color: ultimateAgent.busy ? theme.yellow
                                 : (ultimateAgent.error !== "" ? theme.red
                                 : (ultimateAgent.modelLabel === "Non configuré" ? theme.orange : theme.green))
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: ultimateAgent.busy
                              ? ultimateAgent.status
                              : (ultimateAgent.endpointLabel !== "" ? ultimateAgent.endpointLabel + "  ·  " + ultimateAgent.modelLabel
                                                                     : qsTr("Aucun provider configuré"))
                        color: ultimateAgent.error !== "" ? theme.red : theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: design.textMicro
                        elide: Text.ElideRight
                    }
                }

                IconButton {
                    icon: ultimateAgent.busy ? "icons/stop.svg" : "icons/plus.svg"
                    hint: ultimateAgent.busy ? qsTr("Arrêter le traitement") : qsTr("Nouvelle conversation")
                    onClicked: ultimateAgent.busy ? ultimateAgent.cancel() : ultimateAgent.clearConversation()
                }
                IconButton {
                    visible: root.expandButton
                    icon: "icons/external.svg"
                    hint: qsTr("Ouvrir en grand")
                    onClicked: root.expandRequested()
                }
                IconButton {
                    visible: root.closeButton
                    icon: "icons/close.svg"
                    hint: qsTr("Fermer")
                    onClicked: root.closeRequested()
                }
            }
        }

        // ── error ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: root.compact ? design.space3 : design.space5
            Layout.rightMargin: root.compact ? design.space3 : design.space5
            Layout.topMargin: ultimateAgent.error !== "" ? design.space2 : 0
            Layout.preferredHeight: ultimateAgent.error !== "" ? errorText.implicitHeight + design.space3 : 0
            visible: ultimateAgent.error !== ""
            radius: design.radiusSm
            color: Qt.rgba(theme.red.r, theme.red.g, theme.red.b, 0.09)
            border.width: 1
            border.color: Qt.rgba(theme.red.r, theme.red.g, theme.red.b, 0.55)
            Text {
                id: errorText
                anchors.fill: parent
                anchors.margins: design.space2
                text: ultimateAgent.error
                color: theme.red
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }
        }

        // ── conversation / welcome ──────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: messageList
                anchors.fill: parent
                visible: !root.empty
                anchors.leftMargin: Math.max(design.space5, (parent.width - root.contentWidth) / 2)
                anchors.rightMargin: Math.max(design.space5, (parent.width - root.contentWidth) / 2)
                anchors.topMargin: design.space4
                anchors.bottomMargin: design.space3
                clip: true
                spacing: design.space3
                model: ultimateAgent.messages
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: AppScrollBar {}

                delegate: Item {
                    width: ListView.view.width - design.space5
                    implicitHeight: messageBody.implicitHeight

                    ColumnLayout {
                        id: messageBody
                        width: parent.width
                        spacing: design.space1

                        Rectangle {
                            visible: modelData.role === "user"
                            Layout.alignment: Qt.AlignRight
                            Layout.maximumWidth: Math.max(210, parent.width * 0.78)
                            implicitWidth: Math.min(parent.width * 0.78, userText.implicitWidth + design.space5)
                            implicitHeight: userText.implicitHeight + design.space3
                            radius: design.radiusMd
                            color: Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.13)
                            border.width: 1
                            border.color: Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.4)
                            Text {
                                id: userText
                                anchors.fill: parent
                                anchors.margins: design.space2
                                text: modelData.role === "user" ? modelData.text : ""
                                color: theme.text
                                font.pixelSize: design.textBody
                                wrapMode: Text.Wrap
                                textFormat: Text.PlainText
                            }
                        }

                        ColumnLayout {
                            visible: modelData.role === "assistant"
                            Layout.fillWidth: true
                            Layout.maximumWidth: Math.min(parent.width, 900)
                            Layout.alignment: Qt.AlignLeft
                            spacing: design.space1
                            RowLayout {
                                spacing: design.space2
                                BrandMark { width: 18; height: 18; size: 18; accent: false }
                                Text { text: qsTr("Ultimate Agent"); color: theme.accent; font.pixelSize: design.textCaption; font.weight: Font.DemiBold }
                                Text { Layout.fillWidth: true; text: modelData.time; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.role === "assistant" ? modelData.text : ""
                                color: theme.text
                                font.pixelSize: design.textBody
                                lineHeight: 1.38
                                wrapMode: Text.Wrap
                                textFormat: Text.MarkdownText
                                onLinkActivated: function (link) { Qt.openUrlExternally(link) }
                            }
                        }

                        Rectangle {
                            visible: modelData.role === "tool"
                            Layout.fillWidth: true
                            implicitHeight: toolColumn.implicitHeight + design.space2
                            radius: design.radiusMd
                            color: theme.chip
                            border.width: 1
                            border.color: modelData.ok ? theme.border : Qt.rgba(theme.red.r, theme.red.g, theme.red.b, 0.55)
                            ColumnLayout {
                                id: toolColumn
                                anchors.fill: parent
                                anchors.margins: design.space2
                                spacing: 3
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: design.space2
                                    StatusDot { size: 5; color: (modelData.role === "tool" && modelData.ok) ? theme.green : theme.red }
                                    Text {
                                        text: modelData.role === "tool" ? modelData.tool : ""
                                        color: theme.text
                                        font.family: backend.monoFont
                                        font.pixelSize: design.textCaption
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.role === "tool" ? modelData.detail : ""
                                        color: theme.dimText
                                        font.pixelSize: design.textCaption
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: modelData.expanded ? qsTr("Masquer") : qsTr("Détails")
                                        color: theme.accent
                                        font.pixelSize: design.textMicro
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: ultimateAgent.toggleToolDetails(model.index)
                                        }
                                    }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    visible: modelData.role === "tool" && modelData.expanded
                                    text: modelData.role === "tool" ? modelData.payload : ""
                                    color: theme.dimText
                                    font.family: backend.monoFont
                                    font.pixelSize: design.textMicro
                                    wrapMode: Text.WrapAnywhere
                                    maximumLineCount: 18
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }

            // Welcome state is scrollable: nothing gets clipped in a short dock.
            ScrollView {
                id: welcomeScroll
                anchors.fill: parent
                visible: root.empty
                clip: true
                Component.onCompleted: contentY = 0
                onVisibleChanged: if (visible) contentY = 0
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical: AppScrollBar {}

                Column {
                    width: Math.min(root.compact ? parent.width - design.space6 : 760,
                                    parent.width - (root.compact ? design.space6 : design.space7))
                    x: Math.max(0, (parent.width - width) / 2)
                    topPadding: root.compact ? design.space4 : design.space6
                    bottomPadding: design.space5
                    spacing: design.space5

                    RowLayout {
                        width: parent.width
                        spacing: design.space4

                        BrandMark {
                            Layout.alignment: Qt.AlignTop
                            width: root.compact ? 46 : 64
                            height: width
                            size: root.compact ? 46 : 64
                            accent: false
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: design.space2
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Que veux-tu faire ?")
                                color: theme.brightText
                                font.family: "iA Writer Duospace"
                                font.pixelSize: root.compact ? 22 : 28
                                font.weight: Font.Bold
                                wrapMode: Text.Wrap
                            }
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("Ultimate Agent connaît l'état réel d'Ultimate IAH et peut agir sur DSH, les providers, MCP, la mémoire, les skills, le terminal et l'installation.")
                                color: theme.mutedText
                                font.pixelSize: root.compact ? design.textCaption : design.textBody
                                lineHeight: 1.32
                                wrapMode: Text.Wrap
                            }
                        }
                    }

                    Flow {
                        width: parent.width
                        spacing: design.space2
                        Repeater {
                            model: [
                                qsTr("DSH"), qsTr("DeepSeek"), "MCP", qsTr("Mémoire"),
                                qsTr("Skills"), qsTr("Terminal"), qsTr("Installation")
                            ]
                            Rectangle {
                                width: chip.implicitWidth + design.space3
                                height: 22
                                radius: 11
                                color: theme.chip
                                border.width: 1
                                border.color: theme.border
                                Text {
                                    id: chip
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: theme.mutedText
                                    font.family: backend.monoFont
                                    font.pixelSize: design.textMicro
                                }
                            }
                        }
                    }

                    Text {
                        width: parent.width
                        text: qsTr("Actions rapides")
                        color: theme.dimText
                        font.family: backend.monoFont
                        font.pixelSize: design.textMicro
                        font.letterSpacing: 0.9
                    }

                    GridLayout {
                        width: parent.width
                        columns: 2
                        columnSpacing: design.space2
                        rowSpacing: design.space2
                        Repeater {
                            model: [
                                { label: qsTr("Diagnostiquer"), detail: qsTr("État réel du système"), icon: "icons/shield.svg",
                                  prompt: qsTr("Fais un diagnostic complet d'Ultimate IAH et de son écosystème. Utilise les outils réels, puis résume les problèmes et propose les corrections.") },
                                { label: qsTr("Réparer"), detail: qsTr("DSH · DeepSeek · MCP"), icon: "icons/plug.svg",
                                  prompt: qsTr("Vérifie les connexions DSH, DeepSeek et MCP. Répare ce qui peut l'être sans casser mes données, puis vérifie le résultat.") },
                                { label: qsTr("Expliquer"), detail: qsTr("Capacités et limites"), icon: "icons/spark.svg",
                                  prompt: qsTr("Explique précisément ce que tu peux faire dans Ultimate IAH, quels outils tu as, et comment tu peux m'aider sur ce projet.") },
                                { label: qsTr("Installer"), detail: qsTr("Composants manquants"), icon: "icons/power.svg",
                                  prompt: qsTr("Audite l'installation complète, repère les composants manquants et propose ou lance les installations nécessaires.") }
                            ]
                            AgentQuickAction {
                                Layout.fillWidth: true
                                Layout.preferredHeight: root.compact ? 52 : 64
                                compact: root.compact
                                icon: modelData.icon
                                label: modelData.label
                                detail: modelData.detail
                                onClicked: root.quick(modelData.prompt)
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        implicitHeight: contextColumn.implicitHeight + design.space4
                        radius: design.radiusMd
                        color: theme.chip
                        border.width: 1
                        border.color: theme.border

                        ColumnLayout {
                            id: contextColumn
                            anchors.fill: parent
                            anchors.margins: design.space3
                            spacing: design.space2

                            Text {
                                text: qsTr("Contexte actif")
                                color: theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: design.textMicro
                                font.letterSpacing: 0.8
                            }
                            Repeater {
                                model: [
                                    { label: qsTr("Workspace"), value: backend.workspaceName, ok: true },
                                    { label: qsTr("Modèle"), value: connections.defaultModel !== "" ? connections.defaultModel : ultimateAgent.modelLabel, ok: connections.defaultModel !== "" },
                                    { label: qsTr("Mémoire"), value: qsTr("%n entrée(s)", "", memory.stats.total !== undefined ? memory.stats.total : 0), ok: memory.stats.total !== undefined && memory.stats.total > 0 }
                                ]
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: design.space2
                                    StatusDot { size: 4; color: modelData.ok ? theme.green : theme.dimText }
                                    Text {
                                        text: modelData.label
                                        color: theme.dimText
                                        font.pixelSize: design.textMicro
                                        Layout.preferredWidth: root.compact ? 96 : 112
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.value
                                        color: theme.text
                                        font.family: backend.monoFont
                                        font.pixelSize: design.textMicro
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: theme.border
                        opacity: 0.7
                    }

                    RowLayout {
                        width: parent.width
                        spacing: design.space2
                        ThemedIcon { source: "icons/shield.svg"; width: 12; height: 12; color: theme.dimText }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Il peut agir sur votre machine. Les actions destructives demandent une confirmation explicite.")
                            color: theme.dimText
                            font.pixelSize: design.textMicro
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }

        // ── composer ────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: composerRow.implicitHeight + design.space3
            Layout.leftMargin: Math.max(root.compact ? design.space3 : design.space5, (parent.width - root.contentWidth) / 2)
            Layout.rightMargin: Math.max(root.compact ? design.space3 : design.space5, (parent.width - root.contentWidth) / 2)
            Layout.bottomMargin: design.space3
            radius: design.radiusLg
            color: theme.raised
            border.width: 1
            border.color: composerField.activeFocus ? theme.borderStrong : theme.border
            Behavior on border.color { ColorAnimation { duration: design.durationFast } }

            RowLayout {
                id: composerRow
                anchors.fill: parent
                anchors.margins: 6
                spacing: design.space2

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42

                    TextArea {
                        id: composerField
                        anchors.fill: parent
                        topPadding: 10
                        bottomPadding: 8
                        leftPadding: 6
                        rightPadding: 6
                        color: theme.text
                        font.pixelSize: design.textBody
                        wrapMode: TextArea.Wrap
                        background: null
                        selectByMouse: true
                        Keys.onPressed: function (event) {
                            if (event.key === Qt.Key_Return && !(event.modifiers & Qt.ShiftModifier)) {
                                root.send(composerField.text)
                                composerField.text = ""
                                event.accepted = true
                            }
                        }
                    }
                    Text {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        anchors.topMargin: 10
                        visible: composerField.text === ""
                        text: qsTr("Demandez, diagnostiquez ou agissez…")
                        color: theme.dimText
                        font.pixelSize: design.textBody
                        elide: Text.ElideRight
                    }
                }

                Rectangle {
                    Layout.alignment: Qt.AlignBottom
                    implicitWidth: 38
                    implicitHeight: 38
                    radius: design.radiusMd
                    color: ultimateAgent.busy ? theme.red : (root.canSend ? theme.accent : theme.chip)
                    Behavior on color { ColorAnimation { duration: design.durationFast } }
                    ThemedIcon {
                        anchors.centerIn: parent
                        source: ultimateAgent.busy ? "icons/stop.svg" : "icons/send.svg"
                        width: 16
                        height: 16
                        color: ultimateAgent.busy ? "#ffffff" : (root.canSend ? theme.accentForeground : theme.dimText)
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: root.canSend
                        hoverEnabled: true
                        cursorShape: root.canSend ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: {
                            if (ultimateAgent.busy) {
                                ultimateAgent.cancel()
                            } else {
                                root.send(composerField.text)
                                composerField.text = ""
                            }
                        }
                    }
                }
            }
        }
    }
}
