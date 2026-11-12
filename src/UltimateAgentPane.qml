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
    readonly property int contentWidth: Math.max(320, Math.min(design.contentMax, width - (compact ? design.space4 * 2 : design.space7)))
    readonly property bool empty: ultimateAgent.messages.length === 0

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
        function onMessagesChanged() { root.scrollToBottom() }
        function onBusyChanged() { if (!ultimateAgent.busy) root.scrollToBottom() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: theme.background
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: theme.border }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: design.space5
                anchors.rightMargin: design.space4
                spacing: design.space2

                StatusDot { color: ultimateAgent.busy ? theme.yellow : (ultimateAgent.error !== "" ? theme.red : theme.green) }
                Text {
                    text: ultimateAgent.busy ? ultimateAgent.status : qsTr("Ultimate Agent")
                    color: theme.text
                    font.pixelSize: design.textBody
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: root.compact ? ultimateAgent.modelLabel
                                       : (ultimateAgent.endpointLabel + "  ·  " + ultimateAgent.modelLabel)
                    color: theme.dimText
                    font.family: backend.monoFont
                    font.pixelSize: design.textMicro
                    elide: Text.ElideRight
                }
                Button {
                    text: ultimateAgent.busy ? qsTr("Stop") : qsTr("New conversation")
                    flat: true
                    font.pixelSize: design.textCaption
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

        Rectangle {
            Layout.fillWidth: true
            visible: ultimateAgent.error !== ""
            Layout.preferredHeight: errorText.implicitHeight + design.space3
            color: Qt.rgba(theme.red.r, theme.red.g, theme.red.b, 0.08)
            border.width: 1
            border.color: theme.red
            Text {
                id: errorText
                anchors.fill: parent
                anchors.margins: design.space3
                text: ultimateAgent.error
                color: theme.red
                font.pixelSize: design.textCaption
                wrapMode: Text.Wrap
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: messageList
                anchors.fill: parent
                anchors.leftMargin: Math.max(design.space5, (parent.width - root.contentWidth) / 2)
                anchors.rightMargin: Math.max(design.space5, (parent.width - root.contentWidth) / 2)
                anchors.topMargin: root.empty ? 0 : Math.max(design.space4,
                    parent.height - messageList.contentHeight - design.space3)
                anchors.bottomMargin: design.space3
                clip: true
                spacing: design.space3
                model: ultimateAgent.messages
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: Item {
                    width: ListView.view.width
                    implicitHeight: messageBody.implicitHeight

                    ColumnLayout {
                        id: messageBody
                        width: parent.width
                        spacing: design.space1

                        // User message: aligned right, compact card.
                        Rectangle {
                            visible: modelData.role === "user"
                            Layout.alignment: Qt.AlignRight
                            Layout.maximumWidth: Math.max(220, parent.width * 0.76)
                            implicitWidth: Math.min(parent.width * 0.76, userText.implicitWidth + design.space5)
                            implicitHeight: userText.implicitHeight + design.space3
                            radius: design.radiusMd
                            color: Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.12)
                            border.width: 1
                            border.color: Qt.rgba(theme.accent.r, theme.accent.g, theme.accent.b, 0.35)
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

                        // Assistant message: full-width readable Markdown.
                        ColumnLayout {
                            visible: modelData.role === "assistant"
                            Layout.fillWidth: true
                            Layout.maximumWidth: Math.min(parent.width, 900)
                            Layout.alignment: Qt.AlignLeft
                            spacing: design.space1
                            RowLayout {
                                spacing: design.space2
                                BrandMark { width: 20; height: 18; size: 20; accent: true }
                                Text { text: qsTr("Ultimate Agent"); color: theme.accent; font.pixelSize: design.textCaption; font.weight: Font.DemiBold }
                                Text { Layout.fillWidth: true; text: modelData.time; color: theme.dimText; font.family: backend.monoFont; font.pixelSize: design.textMicro }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.role === "assistant" ? modelData.text : ""
                                color: theme.text
                                font.pixelSize: design.textBody
                                lineHeight: 1.35
                                wrapMode: Text.Wrap
                                textFormat: Text.MarkdownText
                                onLinkActivated: function (link) { Qt.openUrlExternally(link) }
                            }
                        }

                        // Tool step: compact, expandable evidence.
                        Rectangle {
                            visible: modelData.role === "tool"
                            Layout.fillWidth: true
                            implicitHeight: toolColumn.implicitHeight + design.space2
                            radius: design.radiusSm
                            color: theme.raised
                            border.width: 1
                            border.color: modelData.ok ? theme.border : theme.red
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

            // Empty state / prompt suggestions
            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(680, parent.width - design.space7)
                spacing: design.space4
                visible: root.empty

                BrandMark {
                    Layout.alignment: Qt.AlignHCenter
                    width: 64
                    height: 56
                    size: 64
                    accent: true
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Ultimate Agent")
                    color: theme.brightText
                    font.family: "iA Writer Duospace"
                    font.pixelSize: design.textHeadline
                    font.weight: Font.Bold
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Ton opérateur intégré : connaît l'état réel d'Ultimate HAI et agit sur DSH, les providers, 9Router, MausBot, MCP, la mémoire, les skills, le terminal et l'installation.")
                    color: theme.mutedText
                    font.pixelSize: design.textBody
                    wrapMode: Text.Wrap
                    lineHeight: 1.3
                }
                GridLayout {
                    Layout.fillWidth: true
                    columns: root.compact ? 1 : 2
                    columnSpacing: design.space2
                    rowSpacing: design.space2
                    Repeater {
                        model: [
                            { label: qsTr("Diagnostique tout"), prompt: qsTr("Fais un diagnostic complet d'Ultimate HAI et de son écosystème. Utilise les outils réels, puis résume les problèmes et propose les corrections.") },
                            { label: qsTr("Répare les connexions"), prompt: qsTr("Vérifie les connexions DSH, DeepSeek, 9Router, MausBot et MCP. Répare ce qui peut l'être sans casser mes données, puis vérifie le résultat.") },
                            { label: qsTr("Explique ton fonctionnement"), prompt: qsTr("Explique précisément ce que tu peux faire dans Ultimate HAI, quels outils tu as, et comment tu peux m'aider sur ce projet.") },
                            { label: qsTr("Installe ce qui manque"), prompt: qsTr("Audite l'installation complète, repère les composants manquants et propose ou lance les installations nécessaires.") }
                        ]
                        Button {
                            id: suggestion
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            text: modelData.label
                            padding: design.space2
                            background: Rectangle {
                                radius: design.radiusSm
                                color: suggestion.hovered ? theme.raised : theme.chip
                                border.width: 1
                                border.color: suggestion.hovered ? theme.borderStrong : theme.border
                            }
                            contentItem: Text {
                                text: suggestion.text
                                color: theme.text
                                font.pixelSize: design.textCaption
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                wrapMode: Text.Wrap
                            }
                            onClicked: root.quick(modelData.prompt)
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Il peut agir sur votre système. Les actions destructives demandent une confirmation explicite.")
                    color: theme.dimText
                    font.pixelSize: design.textMicro
                    wrapMode: Text.Wrap
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: composer.implicitHeight + design.space4
            Layout.leftMargin: Math.max(design.space5, (parent.width - root.contentWidth) / 2)
            Layout.rightMargin: Math.max(design.space5, (parent.width - root.contentWidth) / 2)
            Layout.bottomMargin: design.space3
            radius: design.radiusLg
            color: theme.raised
            border.width: 1
            border.color: composerField.activeFocus ? theme.borderStrong : theme.border

            RowLayout {
                id: composer
                anchors.fill: parent
                anchors.margins: design.space2
                spacing: design.space2

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 46

                    TextArea {
                        id: composerField
                        anchors.fill: parent
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
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.topMargin: 8
                        visible: composerField.text === ""
                        text: qsTr("Demandez n'importe quoi sur Ultimate HAI…")
                        color: theme.dimText
                        font.pixelSize: design.textBody
                        wrapMode: Text.Wrap
                        elide: Text.ElideRight
                    }
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignBottom
                    spacing: 2
                    Text {
                        visible: !root.empty && !root.compact
                        text: composerField.activeFocus ? qsTr("Entrée envoyer · Maj+Entrée ligne") : qsTr("Ultimate Agent peut agir sur votre machine.")
                        color: theme.dimText
                        font.pixelSize: design.textMicro
                        horizontalAlignment: Text.AlignRight
                    }
                    ActionButton {
                        Layout.alignment: Qt.AlignRight
                        text: ultimateAgent.busy ? qsTr("Arrêter") : qsTr("Envoyer")
                        enabled: ultimateAgent.busy || composerField.text.trim() !== ""
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
