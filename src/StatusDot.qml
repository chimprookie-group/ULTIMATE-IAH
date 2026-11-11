import QtQuick

// A small status light with a soft halo, tinted by the active theme.
Item {
    id: root

    property color color: theme.green
    property int size: 5

    implicitWidth: size + 6
    implicitHeight: size + 6

    Rectangle {
        anchors.centerIn: parent
        width: root.size + 6
        height: root.size + 6
        radius: width / 2
        color: root.color
        opacity: 0.16
    }

    Rectangle {
        anchors.centerIn: parent
        width: root.size
        height: root.size
        radius: width / 2
        color: root.color
    }
}
