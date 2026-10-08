import QtQuick
// Windows 11 (Fluent 2) theme tokens. Values follow the WinUI theme resources:
//   page      = SolidBackgroundFillColorBase (the Mica fallback colour)
//   card      = CardBackgroundFillColorDefault over the page
//   stroke    = CardStrokeColorDefault, divider = DividerStrokeColorDefault
//   text / muted / faint = TextFillColorPrimary / Secondary / Tertiary
// Colour only marks state: accent = Noise Cancelling and primary actions,
// success = connected and Ambient, caution = experimental and low battery.
QtObject {
    required property bool dark
    readonly property color page: dark ? "#202020" : "#f3f3f3"
    readonly property color layer: dark ? "#262626" : "#f7f7f7"      // content layer over the page (LayerFillColorDefault)
    readonly property color pane: dark ? "#1c1c1c" : "#eeeeee"       // navigation pane, a step below the page
    readonly property color card: dark ? "#2b2b2b" : "#fbfbfb"
    readonly property color cardHover: dark ? "#323232" : "#f6f6f6"
    readonly property color line: dark ? "#1affffff" : "#0f000000"   // card and control stroke
    readonly property color divider: dark ? "#15ffffff" : "#0f000000"
    readonly property color fill: dark ? "#0fffffff" : "#80ffffff"    // control fill (buttons, combo)
    readonly property color inset: dark ? "#0fffffff" : "#0a000000"        // quiet fill for tiles sitting on a card
    readonly property color inset2: dark ? "#1affffff" : "#14000000"
    readonly property color fillHover: dark ? "#15ffffff" : "#b3f9f9f9"
    readonly property color fillPressed: dark ? "#08ffffff" : "#4df9f9f9"
    readonly property color track: dark ? "#9affffff" : "#72000000"   // switch / slider rail
    readonly property color thumb: dark ? "#ffffff" : "#ffffff"
    readonly property color text: dark ? "#ffffff" : "#e4000000"
    readonly property color muted: dark ? "#c5ffffff" : "#9e000000"
    readonly property color faint: dark ? "#87ffffff" : "#72000000"
    readonly property color accent: dark ? "#60cdff" : "#0067c0"
    readonly property color accentText: dark ? "#60cdff" : "#003e92"
    readonly property color onAccent: dark ? "#000000" : "#ffffff"
    readonly property color green: dark ? "#6ccb5f" : "#0f7b0f"       // success
    readonly property color warn: dark ? "#fce100" : "#9d5d00"        // caution
    readonly property color critical: dark ? "#ff99a4" : "#c42b1c"
    readonly property color shadow: dark ? "#66000000" : "#1f000000"
}
