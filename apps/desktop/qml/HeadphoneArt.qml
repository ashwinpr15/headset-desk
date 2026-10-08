import QtQuick
// Original, generic over-ear headphone drawing built from primitives (no manufacturer artwork,
// no image assets). It sits on a disc; while connected, rings drift slowly across the stage:
// inward for Noise Cancelling (sealing the sound out), outward for Ambient (letting it in).
// The disc border and a one-shot pulse mark each mode change.
Item {
    id: art
    required property var colors
    property bool active: false
    property int mode: -1          // -1 unknown, 0 off, 1 noise cancelling, 2 ambient
    property bool reduceMotion: false
    implicitWidth: 160; implicitHeight: 160
    readonly property real s: Math.min(width, height) / 64
    readonly property color ink: active ? colors.text : colors.faint
    readonly property color tone: mode === 1 ? colors.accent : mode === 2 ? colors.green : "transparent"
    readonly property bool drifting: active && mode > 0 && !reduceMotion && visible

    Repeater { // slow rings that cross the stage
        model: [0, 1]
        Rectangle {
            id: wave
            required property int modelData
            anchors.centerIn: parent
            width: art.width * 0.9; height: width; radius: width / 2
            color: "transparent"; border.width: Math.max(1, 1.2 * art.s); border.color: art.tone
            opacity: 0
            SequentialAnimation {
                running: art.drifting; loops: Animation.Infinite
                onRunningChanged: if (!running) wave.opacity = 0
                PauseAnimation { duration: wave.modelData * 2200 }
                ParallelAnimation {
                    NumberAnimation { target: wave; property: "scale"; duration: 4400; easing.type: Easing.OutSine
                        from: art.mode === 1 ? 1.45 : 0.95; to: art.mode === 1 ? 0.95 : 1.45 }
                    SequentialAnimation {
                        NumberAnimation { target: wave; property: "opacity"; from: 0; to: 0.5; duration: 1100 }
                        NumberAnimation { target: wave; property: "opacity"; to: 0; duration: 3300 }
                    }
                }
            }
        }
    }
    Rectangle { // disc behind the drawing
        anchors.fill: parent; radius: width / 2
        gradient: Gradient {
            GradientStop { position: 0; color: art.colors.cardHover }
            GradientStop { position: 1; color: art.colors.fill }
        }
        opacity: art.active ? 1 : 0.6
        border.width: art.active && art.mode > 0 ? 2 * art.s * 0.5 : 1
        border.color: art.active && art.mode > 0 ? art.tone : art.colors.line
        Behavior on border.color { ColorAnimation { duration: art.reduceMotion ? 0 : 250 } }
    }
    Rectangle { // one-shot pulse on a mode change
        id: pulse
        anchors.centerIn: parent
        width: parent.width; height: width; radius: width / 2
        color: "transparent"; border.width: 2 * art.s * 0.6; border.color: art.tone
        opacity: 0
        ParallelAnimation {
            id: pulseAnim
            NumberAnimation { target: pulse; property: "scale"; from: 1.0; to: 1.18; duration: 480; easing.type: Easing.OutCubic }
            NumberAnimation { target: pulse; property: "opacity"; from: 0.8; to: 0; duration: 480; easing.type: Easing.OutCubic }
        }
    }
    onModeChanged: if (active && mode > 0 && !reduceMotion) pulseAnim.restart()

    Item { // headband: top half of a ring
        x: 15 * art.s; y: 13 * art.s; width: 34 * art.s; height: 17 * art.s
        clip: true
        Rectangle {
            width: parent.width; height: parent.width; radius: width / 2
            color: "transparent"; border.color: art.ink; border.width: 3 * art.s
            Behavior on border.color { ColorAnimation { duration: art.reduceMotion ? 0 : 250 } }
        }
    }
    Repeater {
        model: [0, 1]
        Rectangle { // ear cups
            required property int modelData
            x: (modelData === 0 ? 11 : 41) * art.s; y: 27 * art.s
            width: 12 * art.s; height: 22 * art.s; radius: 5 * art.s
            color: art.ink
            Behavior on color { ColorAnimation { duration: art.reduceMotion ? 0 : 250 } }
            Rectangle { // cushion edge facing inward
                width: 3 * art.s; height: parent.height - 6 * art.s; radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                x: modelData === 0 ? parent.width - width - 1.5 * art.s : 1.5 * art.s
                color: art.colors.card; opacity: 0.55
            }
        }
    }
}
