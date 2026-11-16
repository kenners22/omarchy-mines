import QtQuick
import Mines

// The board and everything done to it: the grid of tiles, the mouse, the
// keyboard cursor, and the cover drawn over a paused game.
Item {
    id: field

    property int cell: 32
    readonly property int gap: cell >= 18 ? 2 : 1

    // The keyboard cursor, and whether the last input was a key. While it
    // was, keys act on the cursor; after the pointer moves they act on the
    // cell under it, so hover and F flags without a right click.
    property int cursor: 0
    property bool keyboard: false
    property int hover: -1
    readonly property int current: keyboard ? cursor : hover

    // The cell under a held button, and whether the press is a chord (which
    // shows the cells around it pressed too).
    property int press: -1
    property bool pressChord: false

    signal resumeRequested()

    implicitWidth: game.columns * cell + gap
    implicitHeight: game.rows * cell + gap

    function near(a, b) {
        const columns = game.columns
        return Math.abs(a % columns - b % columns) <= 1
            && Math.abs(Math.floor(a / columns) - Math.floor(b / columns)) <= 1
    }

    function centre() {
        cursor = Math.floor(game.rows / 2) * game.columns + Math.floor(game.columns / 2)
    }

    // The first key after the mouse only shows where the cursor is, unless
    // the pointer is resting on a cell, in which case it starts from there.
    function wake() {
        if (keyboard)
            return true
        keyboard = true
        if (hover < 0)
            return false
        cursor = hover
        return true
    }

    function move(dx, dy) {
        if (!wake())
            return
        const column = Math.max(0, Math.min(game.columns - 1, cursor % game.columns + dx))
        const row = Math.max(0, Math.min(game.rows - 1, Math.floor(cursor / game.columns) + dy))
        cursor = row * game.columns + column
    }

    function jump(column, row) {
        if (!wake())
            return
        const c = column < 0 ? cursor % game.columns : Math.min(column, game.columns - 1)
        const r = row < 0 ? Math.floor(cursor / game.columns) : Math.min(row, game.rows - 1)
        cursor = r * game.columns + c
    }

    function act() { if (current >= 0) game.act(current); else wake() }
    function flag() { if (current >= 0) game.flag(current); else wake() }
    function chord() { if (current >= 0) game.chord(current); else wake() }

    Component.onCompleted: centre()

    Connections {
        target: game
        function onBoardChanged() {
            field.centre()
            field.hover = -1
            field.press = -1
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Math.min(4, theme.radius)
        color: theme.grout
    }

    Item {
        x: field.gap
        y: field.gap

        Repeater {
            model: game
            delegate: Tile {
                cell: field.cell
                gap: field.gap
                down: field.press >= 0
                      && (field.pressChord ? field.near(index, field.press) : index === field.press)
            }
        }

        Rectangle {
            visible: !field.keyboard && field.hover >= 0 && !game.over && !game.covered
            x: (field.hover % game.columns) * field.cell
            y: Math.floor(field.hover / game.columns) * field.cell
            width: field.cell - field.gap
            height: field.cell - field.gap
            radius: Math.min(4, theme.radius, field.cell / 6)
            color: theme.foreground
            opacity: .07
        }

        Rectangle {
            visible: field.keyboard && !game.covered
            x: (field.cursor % game.columns) * field.cell - field.gap
            y: Math.floor(field.cursor / game.columns) * field.cell - field.gap
            width: field.cell + field.gap
            height: field.cell + field.gap
            radius: Math.min(4, theme.radius, field.cell / 6) + field.gap
            color: "transparent"
            border.width: field.gap
            border.color: theme.accent

            Behavior on x { NumberAnimation { duration: 50 } }
            Behavior on y { NumberAnimation { duration: 50 } }
        }
    }

    MouseArea {
        id: pointer

        // Armed from a press until its release; a chord when the middle
        // button, or left and right together, started it.
        property bool armed: false
        property bool chording: false

        // Where the pointer last was while the mouse was in charge. A hand
        // brushing the touchpad mid-game mustn't hand the keys to whatever
        // cell the pointer happens to be over, so it takes a real move from
        // here to leave the keyboard cursor.
        property real restX: 0
        property real restY: 0

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        hoverEnabled: true
        enabled: !game.covered

        function cellAt(x, y) {
            const column = Math.floor((x - field.gap / 2) / field.cell)
            const row = Math.floor((y - field.gap / 2) / field.cell)
            return column < 0 || row < 0 || column >= game.columns || row >= game.rows
                ? -1 : row * game.columns + column
        }

        function track(mouse) {
            field.hover = cellAt(mouse.x, mouse.y)
            if (armed) {
                field.press = field.hover
                field.pressChord = chording || game.faceAt(field.hover) === Game.Open
            }
        }

        function disarm() {
            armed = false
            chording = false
            field.press = -1
            field.pressChord = false
        }

        onPositionChanged: (mouse) => {
            if (field.keyboard && !armed
                    && Math.abs(mouse.x - restX) < 12 && Math.abs(mouse.y - restY) < 12) {
                track(mouse)
                return
            }
            field.keyboard = false
            restX = mouse.x
            restY = mouse.y
            track(mouse)
        }
        onExited: if (!armed) field.hover = -1
        onPressed: (mouse) => {
            field.keyboard = false
            restX = mouse.x
            restY = mouse.y
            field.hover = cellAt(mouse.x, mouse.y)
            const both = (mouse.buttons & Qt.LeftButton) && (mouse.buttons & Qt.RightButton)
            if (mouse.button === Qt.RightButton && !both) {
                game.flag(field.hover)
                return
            }
            armed = true
            chording = both || mouse.button === Qt.MiddleButton
            track(mouse)
        }

        // A move happens on release, on the cell the pointer ends up over,
        // so a press can still be dragged off the board and called off.
        onReleased: (mouse) => {
            if (!armed)
                return
            const target = cellAt(mouse.x, mouse.y)
            const chord = chording
            disarm()
            if (target < 0)
                return
            if (chord)
                game.chord(target)
            else
                game.act(target)
        }
        onCanceled: disarm()
    }

    // A paused game hides its board, so a pause is no free look at it.
    Rectangle {
        anchors.fill: parent
        radius: Math.min(4, theme.radius)
        color: theme.well
        border.width: 1
        border.color: theme.grout
        visible: opacity > 0
        opacity: game.covered ? 1 : 0

        Behavior on opacity { NumberAnimation { duration: 120 } }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            hoverEnabled: true
            onClicked: field.resumeRequested()
        }

        Column {
            anchors.centerIn: parent
            width: parent.width - 24
            spacing: 8

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Paused")
                color: theme.foreground
                font.family: theme.fontFamily
                font.pixelSize: 20
                font.bold: true
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: game.paused
                text: qsTr("Press P or click to carry on")
                color: theme.dim
                font.family: theme.fontFamily
                font.pixelSize: 13
            }
        }
    }
}
