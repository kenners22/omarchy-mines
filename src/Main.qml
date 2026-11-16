import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import Mines

ApplicationWindow {
    id: win

    minimumWidth: 500
    minimumHeight: 460
    title: "Mines"
    color: theme.background
    font.family: theme.fontFamily

    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent
    Material.foreground: theme.foreground
    Material.background: theme.panel
    Material.roundedScale: theme.radius

    // Which card is up, if any: "help", "best", "custom", or "quit". When a
    // card goes away so does the keyboard focus it held, or a number field
    // left behind would keep swallowing the game's keys.
    property string sheet: ""
    onSheetChanged: if (sheet !== "custom") root.forceActiveFocus()
    readonly property bool boardKeys: sheet === ""
    readonly property bool moveKeys: sheet === "" || sheet === "help" || sheet === "best"

    readonly property var levels: [
        { level: "beginner", name: qsTr("Beginner") },
        { level: "intermediate", name: qsTr("Intermediate") },
        { level: "expert", name: qsTr("Expert") },
        { level: "custom", name: qsTr("Custom") }
    ]

    function show(name) {
        sheet = sheet === name ? "" : name
    }

    function pick(level) {
        if (level === "custom") {
            sheet = "custom"
            customSheet.load()
        } else {
            sheet = ""
            game.setLevel(level)
        }
    }

    // A game in play is asked about once; nothing else is lost by leaving.
    function quit() {
        if (game.playing && sheet !== "quit")
            sheet = "quit"
        else
            Qt.quit()
    }

    // The movement keys move the cursor, or scroll a card that is too tall
    // for the window.
    function step(dx, dy) {
        if (sheet === "help")
            helpSheet.scroll(dy * 48)
        else if (sheet === "best")
            bestSheet.scroll(dy * 48)
        else
            field.move(dx, dy)
    }

    function primary() {
        if (game.paused)
            game.paused = false
        else
            field.act()
    }

    function confirm() {
        if (sheet === "quit")
            Qt.quit()
        else if (sheet === "custom")
            customSheet.apply()
        else if (sheet !== "")
            sheet = ""
        else
            primary()
    }

    // The window opens at a size that suits the board, once, and is left
    // alone after that: a tiling compositor decides its size anyway.
    Component.onCompleted: {
        width = Math.max(620, Math.min(1320, game.columns * 36 + 120))
        height = Math.max(600, Math.min(900, game.rows * 36 + 230))
        visible = true
    }

    // The clock stops while the window is out of focus or a card is up.
    Binding {
        target: game
        property: "suspended"
        value: !win.active || win.sheet !== ""
    }

    Shortcut { sequences: ["?", "F1"]; context: Qt.ApplicationShortcut; enabled: win.sheet !== "custom"; onActivated: win.show("help") }
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; enabled: win.sheet !== ""; onActivated: win.sheet = "" }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; enabled: win.sheet !== "custom"; onActivated: win.quit() }
    Shortcut { sequences: ["Return", "Enter"]; context: Qt.ApplicationShortcut; autoRepeat: false; onActivated: win.confirm() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; autoRepeat: false; enabled: win.boardKeys; onActivated: win.primary() }
    Shortcut { sequence: "B"; context: Qt.ApplicationShortcut; enabled: win.sheet === "" || win.sheet === "best"; onActivated: win.show("best") }

    Shortcut { sequence: "N"; context: Qt.ApplicationShortcut; enabled: win.boardKeys; onActivated: game.newGame() }
    Shortcut { sequence: "P"; context: Qt.ApplicationShortcut; enabled: win.boardKeys; onActivated: game.paused = !game.paused }
    Shortcut { sequence: "F"; context: Qt.ApplicationShortcut; autoRepeat: false; enabled: win.boardKeys; onActivated: field.flag() }
    Shortcut { sequence: "C"; context: Qt.ApplicationShortcut; autoRepeat: false; enabled: win.boardKeys; onActivated: field.chord() }
    Shortcut { sequence: "1"; context: Qt.ApplicationShortcut; enabled: win.boardKeys; onActivated: win.pick("beginner") }
    Shortcut { sequence: "2"; context: Qt.ApplicationShortcut; enabled: win.boardKeys; onActivated: win.pick("intermediate") }
    Shortcut { sequence: "3"; context: Qt.ApplicationShortcut; enabled: win.boardKeys; onActivated: win.pick("expert") }
    Shortcut { sequence: "4"; context: Qt.ApplicationShortcut; enabled: win.boardKeys; onActivated: win.pick("custom") }

    Shortcut { sequences: ["Left", "H"]; context: Qt.ApplicationShortcut; enabled: win.moveKeys; onActivated: win.step(-1, 0) }
    Shortcut { sequences: ["Right", "L"]; context: Qt.ApplicationShortcut; enabled: win.moveKeys; onActivated: win.step(1, 0) }
    Shortcut { sequences: ["Up", "K"]; context: Qt.ApplicationShortcut; enabled: win.moveKeys; onActivated: win.step(0, -1) }
    Shortcut { sequences: ["Down", "J"]; context: Qt.ApplicationShortcut; enabled: win.moveKeys; onActivated: win.step(0, 1) }
    Shortcut { sequence: "Home"; context: Qt.ApplicationShortcut; enabled: win.boardKeys; onActivated: field.jump(0, -1) }
    Shortcut { sequence: "End"; context: Qt.ApplicationShortcut; enabled: win.boardKeys; onActivated: field.jump(game.columns - 1, -1) }
    Shortcut { sequence: "PgUp"; context: Qt.ApplicationShortcut; enabled: win.boardKeys; onActivated: field.jump(-1, 0) }
    Shortcut { sequence: "PgDown"; context: Qt.ApplicationShortcut; enabled: win.boardKeys; onActivated: field.jump(-1, game.rows - 1) }

    // Right-to-left languages mirror the window; the Translator sets the
    // application's layout direction. The board itself stays as it is.
    Item {
        id: root

        readonly property int margin: 24

        // Everything in the deck that isn't the board, and the largest whole
        // cell size that still fits the board in what is left.
        readonly property int chrome: tabs.height + 18 + status.height + 14 + 12 + footer.height
        readonly property int cell: Math.max(6, Math.min(46, Math.floor(Math.min(
            (width - 2 * margin - 2) / game.columns,
            (height - 2 * margin - chrome - 2) / game.rows))))

        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        Column {
            id: deck

            anchors.centerIn: parent
            width: Math.max(field.width, Math.min(root.width - 2 * root.margin, 500))

            LevelTabs {
                id: tabs
                width: parent.width
                current: game.level
                levels: win.levels
                onPicked: (level) => win.pick(level)
            }

            Item { width: 1; height: 18 }

            Item {
                id: status

                width: parent.width
                height: 48

                Row {
                    id: counter
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    MineMark {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 26
                        height: 26
                        body: theme.dim
                        core: theme.background
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (translator.language, Number(game.minesLeft).toLocaleString(Qt.locale(), "f", 0))
                        color: theme.foreground
                        font.family: theme.fontFamily
                        font.pixelSize: 30
                        font.bold: true
                    }
                }

                ActionButton {
                    anchors.centerIn: parent
                    width: Math.min(implicitWidth, parent.width - 2 * Math.max(counter.width, clock.width) - 24)
                    text: game.won ? qsTr("Cleared! Play again") : game.lost ? qsTr("Mine hit. Try again") : qsTr("New game")
                    key: "N"
                    primary: game.over
                    onClicked: game.newGame()
                }

                Text {
                    id: clock
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: (translator.language, game.formatTime(game.elapsed, game.over))
                    color: game.won ? theme.accent : theme.foreground
                    font.family: theme.fontFamily
                    font.pixelSize: 30
                    font.bold: true
                }
            }

            Item { width: 1; height: 14 }

            Minefield {
                id: field
                objectName: "field"
                anchors.horizontalCenter: parent.horizontalCenter
                width: implicitWidth
                height: implicitHeight
                cell: root.cell
                onResumeRequested: game.paused = false
            }

            Item { width: 1; height: 12 }

            Item {
                id: footer

                width: parent.width
                height: 22

                // Something wrong with the best times is said here, in place
                // of the figures it would otherwise put in doubt.
                Text {
                    visible: game.issue !== ""
                    anchors.left: parent.left
                    anchors.right: hint.left
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: game.issue
                    color: theme.danger
                    elide: Text.ElideMiddle
                    font.family: theme.fontFamily
                    font.pixelSize: 12
                }

                // The board's size, the best time, and the win count, as
                // many of them as fit beside the shortcuts hint. The best
                // time is the last to go.
                Row {
                    id: facts

                    readonly property string size: qsTr("%1 × %2").arg(game.columns).arg(game.rows)
                        + "  ·  " + qsTr("%n mines", "", game.mineCount)
                    readonly property string best: (translator.language, game.newBest
                        ? qsTr("New best %1").arg(game.formatTime(game.best, true))
                        : qsTr("Best %1").arg(game.formatTime(game.best, true)))
                    readonly property string won: qsTr("Won %1 of %2").arg(game.wins).arg(game.played)
                    readonly property string dot: "  ·  "

                    readonly property bool hasBest: game.best > 0
                    readonly property real room: footer.width - hint.width - 20
                    readonly property real sizeAndBest: sizeMetrics.width + (hasBest ? dotMetrics.width + bestMetrics.width : 0)
                    readonly property bool showSize: !hasBest || sizeAndBest <= room
                    readonly property bool showWon: game.played > 0 && showSize
                        && sizeAndBest + dotMetrics.width + wonMetrics.width <= room

                    visible: game.issue === ""
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter

                    TextMetrics { id: sizeMetrics; font: sizeText.font; text: facts.size }
                    TextMetrics { id: bestMetrics; font: bestText.font; text: facts.best }
                    TextMetrics { id: wonMetrics; font: sizeText.font; text: facts.won }
                    TextMetrics { id: dotMetrics; font: sizeText.font; text: facts.dot }

                    Text {
                        id: sizeText
                        visible: facts.showSize
                        width: Math.min(implicitWidth, facts.room)
                        text: facts.size
                        color: theme.dim
                        elide: Text.ElideRight
                        font.family: theme.fontFamily
                        font.pixelSize: 12
                    }
                    Text {
                        visible: facts.showSize && facts.hasBest
                        text: facts.dot
                        color: theme.dim
                        font: sizeText.font
                    }
                    Text {
                        id: bestText
                        visible: facts.hasBest
                        width: Math.min(implicitWidth, facts.room)
                        text: facts.best
                        color: game.newBest ? theme.accent : theme.dim
                        elide: Text.ElideRight
                        font.family: theme.fontFamily
                        font.pixelSize: 12
                        font.bold: game.newBest
                    }
                    Text {
                        visible: facts.showWon
                        text: facts.dot + facts.won
                        color: theme.dim
                        font: sizeText.font
                    }
                }
                MouseArea {
                    anchors.fill: facts
                    enabled: facts.visible
                    cursorShape: Qt.PointingHandCursor
                    onClicked: win.show("best")
                }

                Row {
                    id: hint
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    KeyCap { anchors.verticalCenter: parent.verticalCenter; label: "?" }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Shortcuts")
                        color: theme.dim
                        font.family: theme.fontFamily
                        font.pixelSize: 12
                    }
                }
                MouseArea {
                    anchors.fill: hint
                    cursorShape: Qt.PointingHandCursor
                    onClicked: win.show("help")
                }
            }
        }

        Sheet {
            id: helpSheet
            open: win.sheet === "help"
            title: qsTr("Shortcuts")
            cardWidth: 540
            onDismissed: win.sheet = ""

            Repeater {
                model: [
                    { keys: ["←", "↓", "↑", "→"], what: qsTr("Move around the board (or H J K L)") },
                    { keys: ["Space", "Enter"], what: qsTr("Open a cell, or chord on a number") },
                    { keys: ["F"], what: qsTr("Flag a cell, or take the flag off") },
                    { keys: ["C"], what: qsTr("Chord: open everything around a number whose flags are all down") },
                    { keys: ["N"], what: qsTr("New game") },
                    { keys: ["1", "2", "3", "4"], what: qsTr("Beginner, Intermediate, Expert, Custom") },
                    { keys: ["P"], what: qsTr("Pause") },
                    { keys: ["B"], what: qsTr("Best times") },
                    { keys: ["?"], what: qsTr("Show or hide these shortcuts") },
                    { keys: ["Q"], what: qsTr("Quit") }
                ]

                Row {
                    id: line

                    required property var modelData

                    width: parent.width
                    spacing: 16

                    Row {
                        width: 150
                        spacing: 4
                        Repeater {
                            model: line.modelData.keys
                            KeyCap {
                                required property string modelData
                                label: modelData
                            }
                        }
                    }
                    Text {
                        width: parent.width - 166
                        anchors.verticalCenter: parent.verticalCenter
                        text: line.modelData.what
                        color: theme.foreground
                        wrapMode: Text.WordWrap
                        font.family: theme.fontFamily
                        font.pixelSize: 13
                    }
                }
            }

            Text {
                width: parent.width
                topPadding: 6
                text: qsTr("With the mouse: left click opens a cell or chords on a number, right click flags, and middle click chords. Keys act on the cell under the pointer too.")
                color: theme.dim
                wrapMode: Text.WordWrap
                lineHeight: 1.25
                font.family: theme.fontFamily
                font.pixelSize: 12
            }
        }

        Sheet {
            id: bestSheet

            // Bumped when the scores change, so the card re-reads them.
            property int revision: 0

            open: win.sheet === "best"
            title: qsTr("Best times")
            cardWidth: 760
            onDismissed: win.sheet = ""

            Connections {
                target: game
                function onScoresChanged() { bestSheet.revision++ }
                function onCustomChanged() { bestSheet.revision++ }
            }

            Grid {
                id: grid
                width: parent.width
                columns: width >= 620 ? 4 : 2
                columnSpacing: 20
                rowSpacing: 20

                Repeater {
                    model: win.levels

                    Column {
                        id: entry

                        required property var modelData
                        readonly property var record: (bestSheet.revision, translator.language, game.record(modelData.level))

                        width: (grid.width - grid.columnSpacing * (grid.columns - 1)) / grid.columns
                        spacing: 6

                        Text {
                            width: parent.width
                            text: entry.modelData.name
                            color: entry.modelData.level === game.level ? theme.accent : theme.foreground
                            elide: Text.ElideRight
                            font.family: theme.fontFamily
                            font.pixelSize: 14
                            font.bold: true
                        }
                        Text {
                            width: parent.width
                            text: entry.record.played > 0
                                ? qsTr("Won %1 of %2").arg(entry.record.won).arg(entry.record.played)
                                : qsTr("Not played yet")
                            color: theme.dim
                            elide: Text.ElideRight
                            font.family: theme.fontFamily
                            font.pixelSize: 12
                        }
                        Rectangle { width: parent.width; height: 1; color: theme.faint; opacity: .5 }
                        Text {
                            visible: entry.record.times.length === 0
                            width: parent.width
                            text: qsTr("No times yet")
                            color: theme.dim
                            elide: Text.ElideRight
                            font.family: theme.fontFamily
                            font.pixelSize: 12
                        }
                        Repeater {
                            model: entry.record.times.slice(0, 5)

                            Item {
                                id: row

                                required property var modelData
                                required property int index

                                width: entry.width
                                height: 18

                                Text {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: row.modelData.time
                                    color: row.index === 0 ? theme.foreground : theme.dim
                                    font.family: theme.fontFamily
                                    font.pixelSize: 13
                                    font.bold: row.index === 0
                                }
                                Text {
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: row.modelData.date
                                    color: theme.dim
                                    opacity: .8
                                    font.family: theme.fontFamily
                                    font.pixelSize: 11
                                }
                            }
                        }
                    }
                }
            }

            Text {
                width: parent.width
                text: qsTr("Custom shows the board as it is set now: %1 × %2 with %3 mines. Every custom board keeps its own times.")
                    .arg(game.customColumns).arg(game.customRows).arg(game.customMines)
                color: theme.dim
                wrapMode: Text.WordWrap
                lineHeight: 1.25
                font.family: theme.fontFamily
                font.pixelSize: 12
            }
        }

        Sheet {
            id: customSheet

            readonly property int most: game.maxMines(columnsField.entered, rowsField.entered)

            function load() {
                columnsField.text = game.customColumns
                rowsField.text = game.customRows
                minesField.text = game.customMines
                columnsField.grab()
            }

            function apply() {
                win.sheet = ""
                game.setCustom(columnsField.entered, rowsField.entered, minesField.entered)
            }

            open: win.sheet === "custom"
            title: qsTr("Custom board")
            cardWidth: 440
            onDismissed: win.sheet = ""

            Row {
                width: parent.width
                spacing: 12

                NumberField {
                    id: columnsField
                    width: (parent.width - 24) / 3
                    label: qsTr("Width")
                    from: game.minSide
                    to: game.maxColumns
                    onAccepted: customSheet.apply()
                }
                NumberField {
                    id: rowsField
                    width: (parent.width - 24) / 3
                    label: qsTr("Height")
                    from: game.minSide
                    to: game.maxRows
                    onAccepted: customSheet.apply()
                }
                NumberField {
                    id: minesField
                    width: (parent.width - 24) / 3
                    label: qsTr("Mines")
                    from: 1
                    to: customSheet.most
                    onAccepted: customSheet.apply()
                }
            }

            Text {
                width: parent.width
                text: qsTr("Width %1 to %2, height %3 to %4. A board this size takes up to %5 mines.")
                    .arg(game.minSide).arg(game.maxColumns).arg(game.minSide).arg(game.maxRows).arg(customSheet.most)
                color: theme.dim
                wrapMode: Text.WordWrap
                lineHeight: 1.25
                font.family: theme.fontFamily
                font.pixelSize: 12
            }

            Row {
                anchors.right: parent.right
                spacing: 10

                ActionButton { text: qsTr("Cancel"); key: "Esc"; onClicked: win.sheet = "" }
                ActionButton { text: qsTr("Start"); key: "Enter"; primary: true; onClicked: customSheet.apply() }
            }
        }

        Sheet {
            open: win.sheet === "quit"
            title: qsTr("Quit in the middle of a game?")
            cardWidth: 440
            onDismissed: win.sheet = ""

            Text {
                width: parent.width
                text: qsTr("The game in play will be left unfinished.")
                color: theme.dim
                wrapMode: Text.WordWrap
                font.family: theme.fontFamily
                font.pixelSize: 13
            }

            Row {
                anchors.right: parent.right
                spacing: 10

                ActionButton { text: qsTr("Keep playing"); key: "Esc"; onClicked: win.sheet = "" }
                ActionButton { text: qsTr("Quit"); key: "Enter"; primary: true; onClicked: Qt.quit() }
            }
        }
    }
}
