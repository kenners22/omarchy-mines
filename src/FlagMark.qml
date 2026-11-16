import QtQuick
import QtQuick.Shapes

// The flag: a swallowtail pennant on a pole. Drawn on a 24-unit square and
// scaled to whatever size it is given, in the theme's colors.
Item {
    id: mark

    property color cloth: theme.accent
    property color pole: theme.foreground

    Shape {
        width: 24
        height: 24
        scale: mark.width / 24
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: "transparent"
            fillColor: mark.cloth
            PathSvg { path: "M 9.5 4 L 20 4 L 16.4 8.25 L 20 12.5 L 9.5 12.5 Z" }
        }
        ShapePath {
            strokeColor: mark.pole
            strokeWidth: 2
            fillColor: "transparent"
            capStyle: ShapePath.SquareCap
            PathSvg { path: "M 8.5 4 L 8.5 19.5 M 5.5 19.5 L 12.5 19.5" }
        }
    }
}
