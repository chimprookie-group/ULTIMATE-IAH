import QtQuick

// The rider-and-whale mark, with a transparent ground. The white cut is used
// on dark Omarchy themes and the ink cut on light ones. A hairline accent can
// be shown underneath for small personal signatures.
Item {
    id: root

    property int size: 36
    property bool accent: false
    property real markOpacity: 1

    implicitWidth: size
    implicitHeight: size

    Image {
        anchors.fill: parent
        anchors.bottomMargin: root.accent ? 3 : 0
        source: theme.dark ? "assets/logo-ride-light.png" : "assets/logo-ride-dark.png"
        sourceSize.width: Math.max(48, root.size * 2)
        sourceSize.height: Math.max(48, root.size * 2)
        fillMode: Image.PreserveAspectFit
        smooth: true
        asynchronous: true
        opacity: root.markOpacity
    }

    Rectangle {
        visible: root.accent
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        width: Math.max(10, root.size * 0.24)
        height: 2
        radius: 1
        color: theme.accent
        opacity: 0.9
    }
}
