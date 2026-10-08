import QtQuick
import QtQuick.Shapes
// Small line icons drawn as vector paths (24 x 24 grid), crisp at any scale and theme.
// No icon font and no image assets, so they look the same on every Windows setup.
Item {
    id: glyph
    property string kind: "headphones"
    property color tint: "#000000"
    property real size: 20
    property real weight: 1.6
    implicitWidth: size; implicitHeight: size
    readonly property var paths: ({
        headphones: "M4 14v-2a8 8 0 0 1 16 0v2 M4 13h3v7H5.5A1.5 1.5 0 0 1 4 18.5Z M20 13h-3v7h1.5a1.5 1.5 0 0 0 1.5-1.5Z",
        sound: "M4 7h6 M14 7h6 M10 7a2 2 0 1 0 4 0a2 2 0 1 0-4 0 M4 17h2 M10 17h10 M6 17a2 2 0 1 0 4 0a2 2 0 1 0-4 0",
        features: "M7 7h10a5 5 0 0 1 0 10H7A5 5 0 0 1 7 7Z M15 12a2 2 0 1 0 4 0a2 2 0 1 0-4 0",
        device: "M3 12a9 9 0 1 0 18 0a9 9 0 1 0-18 0 M12 11v5.5 M12 7.8v.01",
        menu: "M4 7h16 M4 12h16 M4 17h16",
        refresh: "M20 12a8 8 0 1 1-2.6-5.9 M20 4v4.5h-4.5",
        chat: "M5 5h14a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2h-7l-4 3.5V17H5a2 2 0 0 1-2-2V7a2 2 0 0 1 2-2Z",
        wave: "M3 12h1 M7 8v8 M11 4.5v15 M15 8.5v7 M19 10.5v3 M21 12h0.01",
        mic: "M12 4a3 3 0 0 1 3 3v5a3 3 0 0 1-6 0V7a3 3 0 0 1 3-3Z M6 11a6 6 0 0 0 12 0 M12 17v3.5",
        lock: "M7.5 11V8a4.5 4.5 0 0 1 9 0v3 M6 11h12v9.5H6Z",
        nc: "M3 12a9 9 0 1 0 18 0a9 9 0 1 0-18 0 M8 12a4 4 0 1 0 8 0a4 4 0 1 0-8 0 M12 12v.01",
        ambient: "M12 12v.01 M8.6 8.6a4.9 4.9 0 0 0 0 6.8 M15.4 8.6a4.9 4.9 0 0 1 0 6.8 M5.6 5.6a9 9 0 0 0 0 12.8 M18.4 5.6a9 9 0 0 1 0 12.8",
        off: "M3 12a9 9 0 1 0 18 0a9 9 0 1 0-18 0 M5.6 18.4L18.4 5.6",
        close: "M6 6l12 12 M18 6L6 18",
        check: "M5 12.5l4.5 4.5L19 7.5"
    })
    Shape {
        width: 24; height: 24
        scale: glyph.size / 24
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: glyph.tint; strokeWidth: glyph.weight
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
            PathSvg { path: glyph.paths[glyph.kind] || "" }
        }
    }
}
