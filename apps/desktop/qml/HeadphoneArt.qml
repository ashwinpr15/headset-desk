import QtQuick
// Original, generic over-ear headphone line drawing built from primitives.
// Not modelled on any manufacturer artwork; no image assets are shipped.
// The disc reflects the current noise mode, and a ring animates once when it changes:
// it tightens inward when noise cancelling seals, and ripples outward for ambient sound.
Item {
    id: art
    required property var colors
    property bool active: false
    property int mode: -1          // -1 unknown, 0 off, 1 noise cancelling, 2 ambient
    property bool reduceMotion: false
    implicitWidth: 64; implicitHeight: 64
    readonly property real s: Math.min(width, height) / 64
    readonly property color ink: active ? colors.text : colors.muted
    readonly property color tone: mode === 1 ? colors.accent : mode === 2 ? colors.green : "transparent"

    onModeChanged: if (active && mode > 0 && !reduceMotion) { ripple.restart() }

    Rectangle { // disc behind the drawing
        anchors.fill: parent; radius: width / 2
        color: art.colors.fill; opacity: art.active ? 1 : 0.6
        border.width: art.active && art.mode > 0 ? 2 * art.s : 0
        border.color: art.tone
        Behavior on border.color { ColorAnimation { duration: art.reduceMotion ? 0 : 220 } }
    }
    Rectangle { // one-shot ring for mode changes
        id: ring
        anchors.centerIn: parent
        width: parent.width; height: width; radius: width / 2
        color: "transparent"; border.width: 2 * art.s; border.color: art.tone
        opacity: 0
        ParallelAnimation {
            id: ripple
            NumberAnimation { target: ring; property: "scale"; from: art.mode === 1 ? 1.35 : 1.0; to: art.mode === 1 ? 1.0 : 1.4; duration: 520; easing.type: Easing.OutCubic }
            NumberAnimation { target: ring; property: "opacity"; from: art.mode === 1 ? 0 : 0.9; to: art.mode === 1 ? 0.9 : 0; duration: 520; easing.type: Easing.OutCubic }
            onFinished: ring.opacity = 0
        }
    }
    Item { // headband: top half of a ring
        x: 15 * art.s; y: 13 * art.s; width: 34 * art.s; height: 17 * art.s
        clip: true
        Rectangle {
            width: parent.width; height: parent.width; radius: width / 2
            color: "transparent"; border.color: art.ink; border.width: 3 * art.s
            Behavior on border.color { ColorAnimation { duration: art.reduceMotion ? 0 : 220 } }
        }
    }
    Repeater {
        model: [0, 1]
        Rectangle { // ear cups
            required property int modelData
            x: (modelData === 0 ? 11 : 41) * art.s; y: 27 * art.s
            width: 12 * art.s; height: 22 * art.s; radius: 5 * art.s
            color: art.ink
            Behavior on color { ColorAnimation { duration: art.reduceMotion ? 0 : 220 } }
            Rectangle { // cushion edge facing inward
                width: 3 * art.s; height: parent.height - 6 * art.s; radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                x: modelData === 0 ? parent.width - width - 1.5 * art.s : 1.5 * art.s
                color: art.colors.card; opacity: 0.55
            }
        }
    }
}
