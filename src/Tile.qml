import QtQuick
import Mines

// One cell of the board. It draws `shown`, which trails the cell's real face
// by the model's delay, so an opened area or the mines at the end of a game
// ripple outward from the move instead of landing all at once.
Item {
    id: tile

    required property int index
    required property int face
    required property int adjacent
    required property int delay

    property int cell: 32
    property int gap: 2
    property bool down: false

    property int shown: Game.Hidden
    readonly property bool sunk: shown === Game.Open || shown === Game.Mine || shown === Game.Blast

    x: (index % game.columns) * cell
    y: Math.floor(index / game.columns) * cell
    width: cell - gap
    height: cell - gap

    Component.onCompleted: shown = face
    onFaceChanged: {
        if (delay > 0) {
            wait.interval = delay
            wait.restart()
        } else {
            wait.stop()
            shown = face
        }
    }

    Timer {
        id: wait
        onTriggered: tile.shown = tile.face
    }

    Rectangle {
        anchors.fill: parent
        radius: Math.min(4, theme.radius, tile.cell / 6)
        color: tile.shown === Game.Blast ? theme.danger
             : tile.sunk ? theme.well
             : tile.down && tile.shown === Game.Hidden ? theme.tilePressed : theme.tile

        Behavior on color { ColorAnimation { duration: 110 } }
    }

    Loader {
        anchors.centerIn: parent
        active: tile.shown === Game.Open && tile.adjacent > 0
        sourceComponent: Text {
            text: tile.adjacent
            color: theme.numbers[tile.adjacent - 1]
            font.family: theme.fontFamily
            font.bold: true
            font.pixelSize: Math.max(6, Math.round(tile.cell * .56))

            NumberAnimation on opacity { from: 0; to: 1; duration: 110 }
        }
    }

    Loader {
        anchors.fill: parent
        active: tile.shown === Game.Flag || tile.shown === Game.Misflag
        sourceComponent: Item {
            // A flag that turned out wrong fades, with a stroke through it.
            FlagMark {
                id: flag
                anchors.centerIn: parent
                width: Math.round(tile.cell * .72)
                height: width
                opacity: tile.shown === Game.Misflag ? .35 : 1

                NumberAnimation on scale { from: .4; to: 1; duration: 150; easing.type: Easing.OutBack }
            }
            Rectangle {
                visible: tile.shown === Game.Misflag
                anchors.centerIn: parent
                width: tile.cell * .78
                height: Math.max(2, Math.round(tile.cell / 14))
                rotation: -45
                color: theme.danger
                antialiasing: true
            }
        }
    }

    Loader {
        anchors.fill: parent
        active: tile.shown === Game.Mine || tile.shown === Game.Blast
        sourceComponent: Item {
            MineMark {
                anchors.centerIn: parent
                width: Math.round(tile.cell * .76)
                height: width
                body: tile.shown === Game.Blast ? theme.background : theme.foreground

                NumberAnimation on scale { from: .5; to: 1; duration: 160; easing.type: Easing.OutBack }
            }
        }
    }
}
