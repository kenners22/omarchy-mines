import QtQuick

// The four levels as one segmented row; the level in play is filled with the
// accent.
Rectangle {
    id: tabs

    property string current
    property var levels: []

    signal picked(string level)

    implicitHeight: 36
    radius: theme.radius
    color: "transparent"
    border.width: 1
    border.color: theme.faint

    Row {
        anchors.fill: parent
        anchors.margins: 3
        spacing: 3

        Repeater {
            model: tabs.levels

            Rectangle {
                id: tab

                required property var modelData
                readonly property bool selected: modelData.level === tabs.current

                width: (parent.width - 3 * (tabs.levels.length - 1)) / tabs.levels.length
                height: parent.height
                radius: Math.max(0, theme.radius - 3)
                color: selected ? theme.accent
                     : area.pressed ? theme.tilePressed
                     : area.containsMouse ? theme.tile : "transparent"

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: tab.modelData.name
                    color: tab.selected ? theme.accentForeground
                         : area.containsMouse ? theme.foreground : theme.dim
                    font.family: theme.fontFamily
                    font.pixelSize: 13
                    font.bold: tab.selected
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 9
                }

                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: tabs.picked(tab.modelData.level)
                }
            }
        }
    }
}
