import QtQuick
import QtQuick.Shapes

// The mine: a six-sided body with a spike on each corner and a live core.
// Drawn on a 24-unit square and scaled to whatever size it is given.
Item {
    id: mark

    property color body: theme.foreground
    property color core: theme.danger

    Shape {
        width: 24
        height: 24
        scale: mark.width / 24
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: mark.body
            strokeWidth: 2
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathSvg {
                path: "M 12 7 L 12 2 M 16.33 9.5 L 20.66 7 M 16.33 14.5 L 20.66 17 " +
                      "M 12 17 L 12 22 M 7.67 14.5 L 3.34 17 M 7.67 9.5 L 3.34 7"
            }
        }
        ShapePath {
            strokeColor: "transparent"
            fillColor: mark.body
            PathSvg { path: "M 12 5.6 L 17.54 8.8 L 17.54 15.2 L 12 18.4 L 6.46 15.2 L 6.46 8.8 Z" }
        }
        ShapePath {
            strokeColor: "transparent"
            fillColor: mark.core
            PathSvg { path: "M 9.9 12 a 2.1 2.1 0 1 0 4.2 0 a 2.1 2.1 0 1 0 -4.2 0 Z" }
        }
    }
}
