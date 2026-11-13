import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property bool dismissed: false
    visible: updates.updateAvailable && !root.dismissed
    implicitHeight: visible ? bannerColumn.implicitHeight + design.space3 * 2 : 0
    radius: design.radiusMd
    color: theme.raised
    border.width: 1
    border.color: theme.accent

    ColumnLayout {
        id: bannerColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: design.space3
        anchors.rightMargin: design.space3
        spacing: design.space1

        RowLayout {
            Layout.fillWidth: true
            spacing: design.space2
            ThemedIcon { source: "icons/4crasy.svg"; width: design.iconSm; height: design.iconSm; color: theme.accent }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    text: qsTr("Ultimate IAH %1 est disponible").arg(updates.latestVersion)
                    color: theme.brightText
                    font.pixelSize: design.textCaption
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: updates.error !== "" ? updates.error : qsTr("Mise à jour vérifiée par SHA256 · installation via pkexec")
                    color: updates.error !== "" ? theme.red : theme.dimText
                    font.pixelSize: design.textMicro
                    elide: Text.ElideRight
                }
            }
            ActionButton {
                text: updates.installing ? qsTr("%1 %").arg(updates.progress) : qsTr("Installer")
                enabled: !updates.installing
                onClicked: updates.install()
            }
            GhostButton {
                text: qsTr("Notes de version")
                flat: true
                font.pixelSize: design.textMicro
                onClicked: Qt.openUrlExternally(updates.releaseUrl)
            }
            IconButton { icon: "icons/close.svg"; hint: qsTr("Plus tard"); onClicked: root.dismissed = true }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: updates.installing
            Layout.preferredHeight: 3
            radius: 2
            color: theme.background
            Rectangle {
                width: parent.width * Math.max(0, Math.min(1, updates.progress / 100))
                height: parent.height
                radius: parent.radius
                color: theme.accent
            }
        }
    }
}
