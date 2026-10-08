import QtQuick
import QtQuick.Controls
// Slim slider used for EQ bands, Clear Bass and Ambient level.
// `centered` fills from the middle (0 dB) instead of from the start, and
// `known` hides the handle and fill so an unknown value never looks like 0.
Slider {
    id: control
    required property var colors
    property bool centered: false
    property bool known: true
    property color tint: colors.accent
    readonly property real handleSize: 18
    readonly property real span: (vertical ? availableHeight : availableWidth) - handleSize
    // Pixel position of the handle centre and of the fill origin along the track.
    readonly property real handlePos: handleSize / 2 + visualPosition * span
    readonly property real originPos: handleSize / 2 + (centered ? 0.5 : (vertical ? 1 : 0)) * span
    stepSize: 1
    snapMode: Slider.SnapAlways
    opacity: enabled ? 1 : 0.7
    implicitWidth: vertical ? 28 : 160
    implicitHeight: vertical ? 150 : 28
    Behavior on opacity { NumberAnimation { duration: 150 } }

    background: Item {
        x: control.leftPadding; y: control.topPadding
        width: control.availableWidth; height: control.availableHeight
        Rectangle {
            id: track
            color: control.colors.fill
            radius: 2
            width: control.vertical ? 4 : parent.width - control.handleSize
            height: control.vertical ? parent.height - control.handleSize : 4
            x: control.vertical ? (parent.width - width) / 2 : control.handleSize / 2
            y: control.vertical ? control.handleSize / 2 : (parent.height - height) / 2
        }
        Rectangle {
            visible: control.centered
            color: control.colors.muted; opacity: 0.6
            width: control.vertical ? 12 : 1
            height: control.vertical ? 1 : 12
            x: control.vertical ? (parent.width - width) / 2 : control.originPos
            y: control.vertical ? control.originPos : (parent.height - height) / 2
        }
        Rectangle {
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
        visible: control.known
        width: control.handleSize; height: control.handleSize; radius: width / 2
        x: control.leftPadding + (control.vertical ? (control.availableWidth - width) / 2 : control.handlePos - width / 2)
        y: control.topPadding + (control.vertical ? control.handlePos - height / 2 : (control.availableHeight - height) / 2)
        color: control.colors.dark ? "#f5f5f7" : "#ffffff"
        border.color: control.pressed ? control.tint : control.colors.line
        border.width: control.pressed ? 2 : 1
        Rectangle {
            z: -1; anchors.fill: parent; anchors.topMargin: 1; anchors.bottomMargin: -1
            radius: width / 2; color: control.colors.shadow
        }
    }
}
