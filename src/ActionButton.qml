import QtQuick

// A button in the game's own flat style: outlined, or filled with the accent
// when it is the thing to press next. An optional key shows its shortcut.
// Given less room than it asks for, the label shrinks before it is cut.
Rectangle {
    id: button

    property string text
    property string key
    property bool primary: false

    signal clicked()

    implicitWidth: label.implicitWidth + (cap.visible ? cap.width + 10 : 0) + 28
    implicitHeight: 36
    radius: theme.radius
    color: primary ? theme.accent
         : area.pressed ? theme.tilePressed
         : area.containsMouse ? theme.tile : "transparent"
    border.width: primary ? 0 : 1
    border.color: theme.faint
    opacity: primary && area.containsMouse ? .88 : 1

    Text {
        id: label
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.right: cap.visible ? cap.left : parent.right
        anchors.rightMargin: cap.visible ? 10 : 14
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignHCenter
        text: button.text
        color: button.primary ? theme.accentForeground : theme.foreground
        elide: Text.ElideRight
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: 10
        font.family: theme.fontFamily
        font.pixelSize: 13
        font.bold: true
    }

    Rectangle {
        id: cap
        visible: button.key !== ""
        anchors.right: parent.right
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(20, keyText.implicitWidth + 10)
        height: 20
        radius: Math.min(4, theme.radius)
        color: "transparent"
        border.width: 1
        border.color: button.primary ? theme.accentForeground : theme.faint
        opacity: button.primary ? .55 : 1

        Text {
            id: keyText
            anchors.centerIn: parent
            text: button.key
            color: button.primary ? theme.accentForeground : theme.dim
            font.family: theme.fontFamily
            font.pixelSize: 11
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: button.clicked()
    }
}
