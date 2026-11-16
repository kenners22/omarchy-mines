import QtQuick

// A key, drawn as a key: the label in a thin outlined box.
Rectangle {
    id: cap

    property string label

    implicitWidth: Math.max(22, text.implicitWidth + 12)
    implicitHeight: 22
    radius: Math.min(4, theme.radius)
    color: "transparent"
    border.width: 1
    border.color: theme.faint

    Text {
        id: text
        anchors.centerIn: parent
        text: cap.label
        color: theme.dim
        font.family: theme.fontFamily
        font.pixelSize: 12
    }
}
