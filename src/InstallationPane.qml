import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property int contentWidth: Math.max(320, Math.min(design.contentMax, width - design.space7))

    function refresh() {
        installation.check()
        nineRouter.probe()
        mausBot.probe()
        probeTimer.restart()
    }

    Timer {
        id: probeTimer
        interval: 1400
        repeat: false
        onTriggered: installation.check()
    }

    Component.onCompleted: if (visible) refresh()
    onVisibleChanged: if (visible) refresh()

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical: AppScrollBar {}

        ColumnLayout {
            width: root.contentWidth
            x: Math.max(design.space5, (parent.width - width) / 2)
            spacing: design.space5

            Item { Layout.fillWidth: true; Layout.preferredHeight: design.space4 }

            PageHeader {
                Layout.fillWidth: true
                title: qsTr("Installation")
                description: qsTr("Tout l'écosystème : DSH, Node, Python, 9Router, MausBot, MCP et l'entrée de bureau.")
                icon: "icons/shield.svg"
                accent: theme.green
                ActionButton { text: qsTr("Vérifier tout"); onClicked: root.refresh() }
            }

            SettingsSection {
                title: qsTr("Audit de l'installation")
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: installation.summary !== "" ? installation.summary
                          : qsTr("Vérification des composants système et des services locaux.")
                    color: installation.failed > 0 ? theme.orange : theme.mutedText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }

                Repeater {
                    model: installation.checks
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: row.implicitHeight + design.space3
                        radius: design.radiusSm
                        color: modelData.ok ? theme.background : Qt.rgba(theme.orange.r, theme.orange.g, theme.orange.b, 0.07)
                        border.width: 1
                        border.color: modelData.ok ? theme.border : theme.orange
                        RowLayout {
                            id: row
                            anchors.fill: parent
                            anchors.margins: design.space2
                            spacing: design.space2
                            StatusDot { color: modelData.ok ? theme.green : theme.orange }
                            Text {
                                Layout.preferredWidth: 86
                                text: modelData.layer
                                color: theme.dimText
                                font.family: backend.monoFont
                                font.pixelSize: design.textMicro
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.label
                                    color: theme.text
                                    font.pixelSize: design.textCaption
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.hint !== "" ? modelData.hint : modelData.detail
                                    color: theme.dimText
                                    font.pixelSize: design.textMicro
                                    elide: Text.ElideRight
                                }
                            }
                            Text {
                                text: modelData.ok ? qsTr("prêt") : qsTr("manquant")
                                color: modelData.ok ? theme.green : theme.orange
                                font.pixelSize: design.textMicro
                            }
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Installer ce qui manque")
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Les dépendances système passent par pkexec. DSH, 9Router et MausBot s'installent localement dans les données de l'application.")
                    color: theme.dimText
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: design.space2

                    ActionButton {
                        text: qsTr("Dépendances système")
                        enabled: !installation.installing && !nineRouter.privilegedBusy
                        onClicked: installation.installSystemDependencies()
                    }
                    ActionButton {
                        text: qsTr("Installer DSH")
                        enabled: !installation.installing && !backend.dshAvailable
                        onClicked: installation.installDsh()
                    }
                    ActionButton {
                        text: qsTr("Installer 9Router")
                        enabled: !installation.installing && !nineRouter.installed && !nineRouter.installing
                        onClicked: installation.installNineRouter()
                    }
                    ActionButton {
                        text: qsTr("Installer MausBot")
                        enabled: !installation.installing && !mausBot.installed && !mausBot.installing
                        onClicked: installation.installMausBot()
                    }
                    ActionButton {
                        text: qsTr("Démarrer les services")
                        enabled: !installation.installing
                        onClicked: installation.startServices()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: installation.error !== ""
                    text: installation.error
                    color: theme.red
                    font.pixelSize: design.textCaption
                    wrapMode: Text.Wrap
                }

                Text {
                    Layout.fillWidth: true
                    visible: installation.installing
                    text: installation.actionLabel + "…"
                    color: theme.yellow
                    font.pixelSize: design.textCaption
                }

                Rectangle {
                    Layout.fillWidth: true
                    visible: installation.actionLog !== "" || nineRouter.installLog !== ""
                             || nineRouter.privilegedLog !== "" || mausBot.installLog !== ""
                    implicitHeight: Math.min(220, logColumn.implicitHeight + design.space4)
                    radius: design.radiusSm
                    color: theme.background
                    border.width: 1
                    border.color: theme.border
                    ColumnLayout {
                        id: logColumn
                        anchors.fill: parent
                        anchors.margins: design.space2
                        spacing: 2
                        Text {
                            Layout.fillWidth: true
                            text: installation.actionLog + nineRouter.installLog
                                  + nineRouter.privilegedLog + mausBot.installLog
                            color: theme.dimText
                            font.family: backend.monoFont
                            font.pixelSize: design.textMicro
                            wrapMode: Text.Wrap
                            maximumLineCount: 14
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
