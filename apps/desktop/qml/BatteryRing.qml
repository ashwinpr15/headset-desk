import QtQuick
import QtQuick.Shapes
// Battery level as an open ring around the headphones; the gap at the bottom holds the number.
// -1 draws only the rail (unknown never looks like 0 or 100).
Item {
    id: ring
    property int level: -1
    property color trackColor: "#33808080"
    property color levelColor: "#6ccb5f"
    property bool reduceMotion: false
    property real lineWidth: 6
    property real shown: level < 0 ? 0 : level
    Behavior on shown { NumberAnimation { duration: ring.reduceMotion ? 0 : 600; easing.type: Easing.OutCubic } }
    readonly property real radius: (Math.min(width, height) - lineWidth) / 2
    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: ring.trackColor; strokeWidth: ring.lineWidth; fillColor: "transparent"; capStyle: ShapePath.RoundCap
            PathAngleArc { centerX: ring.width / 2; centerY: ring.height / 2; radiusX: ring.radius; radiusY: ring.radius; startAngle: 120; sweepAngle: 300 }
        }
        ShapePath {
            strokeColor: ring.levelColor; strokeWidth: ring.lineWidth; fillColor: "transparent"; capStyle: ShapePath.RoundCap
            PathAngleArc { centerX: ring.width / 2; centerY: ring.height / 2; radiusX: ring.radius; radiusY: ring.radius
                startAngle: 120; sweepAngle: Math.max(0.01, 300 * ring.shown / 100) }
        }
    }
}
