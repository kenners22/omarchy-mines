import QtQuick

// A whole number typed into a labelled box. Up and Down nudge it, and
// `entered` is what was typed, pulled into range.
Column {
    id: field

    property string label
    property int from: 1
    property int to: 99
    property alias text: input.text
    readonly property int entered: Math.max(from, Math.min(to, parseInt(input.text) || from))

    signal accepted()

    function nudge(by) {
        input.text = String(Math.max(from, Math.min(to, (parseInt(input.text) || from) + by)))
    }

    function grab() {
        input.forceActiveFocus()
        input.selectAll()
    }

    spacing: 6

    Text {
        width: parent.width
        text: field.label
        color: input.activeFocus ? theme.accent : theme.dim
        elide: Text.ElideRight
        font.family: theme.fontFamily
        font.pixelSize: 12
    }

    Rectangle {
        width: parent.width
        height: 44
        radius: theme.radius
        color: theme.well
        border.width: input.activeFocus ? 2 : 1
        border.color: input.activeFocus ? theme.accent : theme.faint

        TextInput {
            id: input

            anchors.fill: parent
            anchors.margins: 8
            horizontalAlignment: TextInput.AlignHCenter
            verticalAlignment: TextInput.AlignVCenter
            color: theme.foreground
            selectionColor: theme.accent
            selectedTextColor: theme.accentForeground
            font.family: theme.fontFamily
            font.pixelSize: 16
            validator: IntValidator { bottom: 0; top: 9999 }
            inputMethodHints: Qt.ImhDigitsOnly
            maximumLength: 4
            selectByMouse: true
            activeFocusOnTab: true

            Keys.onUpPressed: field.nudge(1)
            Keys.onDownPressed: field.nudge(-1)
            Keys.onReturnPressed: field.accepted()
            Keys.onEnterPressed: field.accepted()
            onActiveFocusChanged: {
                if (activeFocus)
                    selectAll()
                else
                    text = String(field.entered)
            }
        }
    }
}
