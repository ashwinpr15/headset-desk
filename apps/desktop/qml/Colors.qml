import QtQuick
// Shared palette. Neutral greys carry the layout; colour only marks state
// (accent = active noise cancelling or primary action, green = connected / ambient,
// warn = experimental or low battery).
QtObject {
    required property bool dark
    readonly property color page: dark ? "#161618" : "#f2f2f7"
    readonly property color card: dark ? "#232325" : "#ffffff"
    readonly property color line: dark ? "#333336" : "#e3e3e8"
    readonly property color fill: dark ? "#343437" : "#e8e8ed"
    readonly property color thumb: dark ? "#58585d" : "#ffffff"
    readonly property color text: dark ? "#f5f5f7" : "#1d1d1f"
    readonly property color muted: dark ? "#98989f" : "#6e6e73"
    readonly property color accent: dark ? "#0a84ff" : "#007aff"
    readonly property color accentText: dark ? "#64b0ff" : "#0062cc"
    readonly property color green: dark ? "#30d158" : "#248a3d"
    readonly property color warn: dark ? "#ff9f0a" : "#b25000"
    readonly property color shadow: dark ? "#66000000" : "#1f000000"
}
