import QtQuick
// Original, generic over-ear headphone line drawing built from primitives.
// Not modelled on any manufacturer artwork; no image assets are shipped.
Item {
    id: art
    required property var colors
    property bool active: false
    implicitWidth: 64; implicitHeight: 64
    readonly property real s: Math.min(width, height) / 64
    readonly property color ink: active ? colors.text : colors.muted

    Rectangle { // soft disc behind the drawing
        anchors.fill: parent; radius: width / 2
        color: art.colors.fill; opacity: art.active ? 1 : 0.6
    }
    Item { // headband: top half of a ring
        x: 15 * art.s; y: 13 * art.s; width: 34 * art.s; height: 17 * art.s
        clip: true
        Rectangle {
            width: parent.width; height: parent.width; radius: width / 2
            color: "transparent"; border.color: art.ink; border.width: 3 * art.s
        }
    }
    Repeater {
        model: [0, 1]
        Rectangle { // ear cups
            required property int modelData
            x: (modelData === 0 ? 11 : 41) * art.s; y: 27 * art.s
            width: 12 * art.s; height: 22 * art.s; radius: 5 * art.s
            color: art.ink
            Rectangle { // cushion edge facing inward
                width: 3 * art.s; height: parent.height - 6 * art.s; radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                x: modelData === 0 ? parent.width - width - 1.5 * art.s : 1.5 * art.s
                color: art.colors.card; opacity: 0.55
            }
        }
    }
}
