import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts


Item {
    id: root

    readonly property var systemPalette: {
        const themes = backend.themeOptions
        for (let i = 0; i < themes.length; ++i) {
            if (themes[i].id === backend.systemThemeId)
                return themes[i]
        }
        return {
            "background": theme.background,
            "foreground": theme.text,
            "accent": theme.accent,
            "cyan": theme.cyan,
            "red": theme.red,
            "green": theme.green
        }
    }

    onVisibleChanged: {
        if (visible)
            backend.refreshThemes()
    }

    function stateColor(state) {
        if (state === "running" || state === "ready")
            return theme.green
        if (state === "starting")
            return theme.yellow
        if (state === "failed")
            return theme.red
        return theme.dimText
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Column {
            id: content
            width: Math.min(880, scroll.availableWidth - 64)
            x: Math.max(32, (scroll.availableWidth - width) / 2)
            topPadding: 24
            bottomPadding: 32
            spacing: 14

            RowLayout {
                width: content.width
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: qsTr("Settings")
                        font.pixelSize: 19
                        font.weight: Font.DemiBold
                        color: theme.text
                    }
                    Text {
                        text: qsTr("One workspace, two engines, nothing hidden.")
                        font.pixelSize: 12
                        color: theme.mutedText
                    }
                }

                ActionButton {
                    text: qsTr("Back to chat")
                    onClicked: backend.setView("opencode")
                }
            }

            SettingsSection {
                title: qsTr("Appearance")
                width: content.width

                Text {
                    Layout.fillWidth: true
                    text: qsTr("The app follows Omarchy's current theme. Pick another installed theme to use it here without changing the desktop.")
                    wrapMode: Text.Wrap
                    color: theme.mutedText
                    font.pixelSize: 11
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    ThemeCard {
                        themeId: ""
                        label: qsTr("Follow Omarchy")
                        mode: theme.dark ? "dark" : "light"
                        palette: root.systemPalette
                        active: backend.themePreference === ""
                        autoTheme: true
                        subtitle: qsTr("Current · %1").arg(backend.systemTheme)
                        onChosen: backend.setTheme("")
                    }

                    Repeater {
                        model: backend.themeOptions
                        ThemeCard {
                            themeId: modelData.id
                            label: modelData.label
                            mode: modelData.mode
                            palette: modelData
                            active: backend.themePreference === modelData.id
                            onChosen: backend.setTheme(modelData.id)
                        }
                    }
                }
            }

            SettingsSection {
                title: qsTr("Workspace")
                width: content.width

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.fillWidth: true
                        text: backend.workspace
                        font.family: backend.monoFont
                        font.pixelSize: 11
                        color: theme.text
                        elide: Text.ElideMiddle
                    }
                    Button {
                        text: qsTr("Browse…")
                        onClicked: backend.pickWorkspace()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Both engines start in this folder. Changing it restarts the harness and opens a new OpenCode session there.")
                    wrapMode: Text.Wrap
                    color: theme.dimText
                    font.pixelSize: 10
                }
            }

            SettingsSection {
                title: qsTr("DeepSeek Harness")
                width: content.width

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.preferredWidth: 86
                        text: qsTr("Profile")
                        color: theme.mutedText
                        font.pixelSize: 11
                    }
                    TextField {
                        id: profileField
                        Layout.preferredWidth: 220
                        text: backend.harnessProfile
                        font.family: backend.monoFont
                        font.pixelSize: 11
                        color: theme.text
                        selectionColor: theme.accent
                        leftPadding: 9
                        rightPadding: 9
                        background: Rectangle {
                            implicitHeight: 32
                            radius: Math.min(6, theme.radius)
                            color: theme.background
                            border.width: 1
                            border.color: profileField.activeFocus ? theme.accent : theme.border
                        }
                        onEditingFinished: backend.setHarnessProfile(text)
                    }
                    Text {
                        text: qsTr("dsh profile booted, web by default")
                        color: theme.dimText
                        font.pixelSize: 10
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.preferredWidth: 86
                        text: qsTr("Binary")
                        color: theme.mutedText
                        font.pixelSize: 11
                    }
                    TextField {
                        id: dshField
                        Layout.fillWidth: true
                        text: backend.dshBinary
                        font.family: backend.monoFont
                        font.pixelSize: 11
                        color: theme.text
                        selectionColor: theme.accent
                        leftPadding: 9
                        rightPadding: 9
                        background: Rectangle {
                            implicitHeight: 32
                            radius: Math.min(6, theme.radius)
                            color: theme.background
                            border.width: 1
                            border.color: dshField.activeFocus ? theme.accent : theme.border
                        }
                        onEditingFinished: backend.setDshBinary(text)
                    }
                    Rectangle {
                        implicitWidth: dshBadge.implicitWidth + 14
                        height: 18
                        radius: 9
                        color: theme.background
                        border.width: 1
                        border.color: backend.dshAvailable ? theme.green : theme.red
                        Text {
                            id: dshBadge
                            anchors.centerIn: parent
                            text: backend.dshAvailable ? qsTr("found") : qsTr("missing")
                            color: backend.dshAvailable ? theme.green : theme.red
                            font.pixelSize: 9
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        width: 5
                        height: 5
                        radius: 2.5
                        color: root.stateColor(backend.harnessState)
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.harnessState + " · " + backend.harnessDetail
                        color: theme.mutedText
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                    Button {
                        text: qsTr("Restart")
                        onClicked: backend.restartHarness()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: backend.harnessState === "failed" && backend.harnessLog !== ""
                    text: backend.harnessLog.split("\n").slice(-10).join("\n")
                    font.family: backend.monoFont
                    font.pixelSize: 10
                    color: theme.dimText
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    maximumLineCount: 12
                    elide: Text.ElideRight
                }
            }

            SettingsSection {
                title: qsTr("OpenCode")
                width: content.width

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        Layout.preferredWidth: 86
                        text: qsTr("Binary")
                        color: theme.mutedText
                        font.pixelSize: 11
                    }
                    TextField {
                        id: opencodeField
                        Layout.fillWidth: true
                        text: backend.opencodeBinary
                        font.family: backend.monoFont
                        font.pixelSize: 11
                        color: theme.text
                        selectionColor: theme.accent
                        leftPadding: 9
                        rightPadding: 9
                        background: Rectangle {
                            implicitHeight: 32
                            radius: Math.min(6, theme.radius)
                            color: theme.background
                            border.width: 1
                            border.color: opencodeField.activeFocus ? theme.accent : theme.border
                        }
                        onEditingFinished: backend.setOpencodeBinary(text)
                    }
                    Rectangle {
                        implicitWidth: opencodeBadge.implicitWidth + 14
                        height: 18
                        radius: 9
                        color: theme.background
                        border.width: 1
                        border.color: backend.opencodeAvailable ? theme.green : theme.red
                        Text {
                            id: opencodeBadge
                            anchors.centerIn: parent
                            text: backend.opencodeAvailable ? qsTr("found") : qsTr("missing")
                            color: backend.opencodeAvailable ? theme.green : theme.red
                            font.pixelSize: 9
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        width: 5
                        height: 5
                        radius: 2.5
                        color: root.stateColor(backend.opencodeState)
                    }
                    Text {
                        Layout.fillWidth: true
                        text: backend.opencodeState + " · " + backend.opencodeDetail
                        color: theme.mutedText
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                    Button {
                        text: qsTr("Restart")
                        onClicked: backend.restartOpencode()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Talked to over the Agent Client Protocol (opencode acp). Sessions, models, modes, tool calls, and permissions are native.")
                    wrapMode: Text.Wrap
                    color: theme.dimText
                    font.pixelSize: 10
                }
            }

            SettingsSection {
                title: qsTr("About")
                width: content.width

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        Layout.fillWidth: true
                        text: "DeepCrasy H " + backend.appVersion
                        color: theme.text
                        font.pixelSize: 12
                    }
                    Text {
                        text: qsTr("Built for Omarchy · MIT")
                        color: theme.dimText
                        font.pixelSize: 10
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Harness and OpenCode stay separate processes. DeepCrasy H only owns their lifecycle and the window around them.")
                    wrapMode: Text.Wrap
                    color: theme.dimText
                    font.pixelSize: 10
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Shortcuts: Ctrl+1 harness · Ctrl+2 chat · Ctrl+3 split · Ctrl+4 settings · Ctrl+L composer · ? help")
                    wrapMode: Text.Wrap
                    color: theme.dimText
                    font.pixelSize: 10
                }
            }
        }
    }
}
