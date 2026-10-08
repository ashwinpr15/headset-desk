import QtQuick
QtObject {
    required property bool dark
    readonly property color page: dark ? "#202020" : "#f5f5f5"
    readonly property color card: dark ? "#292929" : "#ffffff"
    readonly property color line: dark ? "#3d3d3d" : "#e3e3e3"
    readonly property color text: dark ? "#f3f3f3" : "#202020"
    readonly property color muted: dark ? "#b6b6b6" : "#626262"
    readonly property color accent: dark ? "#75baff" : "#0067c0"
}
