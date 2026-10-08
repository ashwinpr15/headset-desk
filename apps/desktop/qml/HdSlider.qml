import QtQuick
import QtQuick.Controls
// Windows 11 slider: 4 px rail, accent fill, 20 px thumb with an accent core that grows on
// hover and shrinks while pressed. Used for EQ bands, Clear Bass and Ambient level.
// `centered` fills from the middle (0 dB), and `known: false` hides the thumb and fill so an
// unknown value never looks like 0. While dragged the thumb follows the pointer exactly;
// otherwise (keyboard, confirmed readback, Reset) it settles to the new value with a short ease.
Slider {
    id: control
    required property var colors
    property bool centered: false
    property bool known: true
    property bool reduceMotion: false
    property color tint: colors.accent
    readonly property real handleSize: 20
    readonly property real span: (vertical ? availableHeight : availableWidth) - handleSize
    property real shownPosition: visualPosition
    Behavior on shownPosition {
        enabled: !control.pressed && !control.reduceMotion
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }
    // Pixel position of the thumb centre and of the fill origin along the rail.
    readonly property real handlePos: handleSize / 2 + shownPosition * span
    readonly property real originPos: handleSize / 2 + (centered ? 0.5 : (vertical ? 1 : 0)) * span
    // Thumb centre in the slider's own coordinates (the EQ curve passes through these points).
    readonly property real thumbX: leftPadding + (vertical ? availableWidth / 2 : handlePos)
    readonly property real thumbY: topPadding + (vertical ? handlePos : availableHeight / 2)
    stepSize: 1
    snapMode: Slider.SnapAlways
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1 : 0.55
    implicitWidth: vertical ? 32 : 160
    implicitHeight: vertical ? 160 : 32
    Behavior on opacity { NumberAnimation { duration: control.reduceMotion ? 0 : 150 } }

    background: Item {
        x: control.leftPadding; y: control.topPadding
        width: control.availableWidth; height: control.availableHeight
        Rectangle {
            color: control.colors.track; opacity: 0.45
            radius: 2
            width: control.vertical ? 4 : parent.width - control.handleSize
            height: control.vertical ? parent.height - control.handleSize : 4
            x: control.vertical ? (parent.width - width) / 2 : control.handleSize / 2
            y: control.vertical ? control.handleSize / 2 : (parent.height - height) / 2
        }
        Rectangle { // fill from origin to thumb
            visible: control.known
            color: control.tint
            radius: 2
            readonly property real from: Math.min(control.originPos, control.handlePos)
            readonly property real length: Math.abs(control.handlePos - control.originPos)
            width: control.vertical ? 4 : length
            height: control.vertical ? length : 4
            x: control.vertical ? (parent.width - width) / 2 : from
            y: control.vertical ? from : (parent.height - height) / 2
        }
    }
    handle: Rectangle {
        id: thumb
        visible: control.known
        width: control.handleSize; height: control.handleSize; radius: width / 2
        x: control.leftPadding + (control.vertical ? (control.availableWidth - width) / 2 : control.handlePos - width / 2)
        y: control.topPadding + (control.vertical ? control.handlePos - height / 2 : (control.availableHeight - height) / 2)
        color: control.colors.dark ? "#454545" : "#ffffff"
        border.color: control.colors.dark ? "#1affffff" : "#1a000000"
        Rectangle { // accent core
            anchors.centerIn: parent
            width: control.pressed ? 10 : control.hovered && control.enabled ? 14 : 12
            height: width; radius: width / 2
            color: control.tint
            Behavior on width { NumberAnimation { duration: control.reduceMotion ? 0 : 120; easing.type: Easing.OutCubic } }
        }
        Rectangle { // keyboard focus ring
            visible: control.visualFocus
            anchors.fill: parent; anchors.margins: -4; radius: width / 2
            color: "transparent"; border.width: 2; border.color: control.colors.text
        }
    }
}
