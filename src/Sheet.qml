import QtQuick

// A card over the window, for the shortcuts, the best times, a custom board,
// and the question before quitting. The window behind is dimmed, and a click
// on it puts the card away. A card taller than the window scrolls.
Item {
    id: sheet

    property bool open: false
    property string title
    property int cardWidth: 460
    default property alias content: body.data

    signal dismissed()

    function scroll(by) {
        const furthest = Math.max(0, scroller.contentHeight - scroller.height)
        scroller.contentY = Math.max(0, Math.min(furthest, scroller.contentY + by))
    }

    anchors.fill: parent
    visible: opacity > 0
    opacity: open ? 1 : 0
    onOpenChanged: if (open) scroller.contentY = 0

    Behavior on opacity { NumberAnimation { duration: 110 } }

    Rectangle {
        anchors.fill: parent
        color: theme.background
        opacity: .82
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        hoverEnabled: true
        onClicked: sheet.dismissed()
        onWheel: (wheel) => wheel.accepted = true
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(sheet.width - 32, sheet.cardWidth)
        height: Math.min(sheet.height - 32, column.implicitHeight + 48)
        radius: theme.radius
        color: theme.panel
        border.width: 1
        border.color: theme.faint

        // Clicks on the card stay on the card.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onWheel: (wheel) => wheel.accepted = true
        }

        Flickable {
            id: scroller

            anchors.fill: parent
            anchors.margins: 24
            contentWidth: width
            contentHeight: column.implicitHeight
            clip: contentHeight > height
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: column
                width: scroller.width
                spacing: 18

                Text {
                    width: parent.width
                    text: sheet.title
                    color: theme.foreground
                    font.family: theme.fontFamily
                    font.pixelSize: 17
                    font.bold: true
                    wrapMode: Text.WordWrap
                }
                Column {
                    id: body
                    width: parent.width
                    spacing: 12
                }
            }
        }

        // How far down the card has been scrolled, when it scrolls at all.
        Rectangle {
            visible: scroller.interactive
            anchors.right: parent.right
            anchors.rightMargin: 8
            y: 24 + scroller.visibleArea.yPosition * scroller.height
            width: 3
            height: scroller.visibleArea.heightRatio * scroller.height
            radius: Math.min(1.5, theme.radius)
            color: theme.dim
            opacity: .6
        }
    }
}
