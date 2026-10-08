import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// Grouped settings tile with an optional small caption above it, in the style
// of system settings panes: one rounded surface per topic, never nested cards.
ColumnLayout {
    id: section
    required property var colors
    property string caption: ""
    property int padding: 16
    default property alias content: layout.data
    spacing: 6
    Label {
        visible: section.caption.length > 0
        text: section.caption.toUpperCase()
        color: section.colors.muted
        font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.8
        Layout.leftMargin: 4
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: layout.implicitHeight + section.padding * 2
        color: section.colors.card
        radius: 12
        border.color: section.colors.line
        ColumnLayout {
            id: layout
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: section.padding }
            spacing: 12
        }
    }
}
