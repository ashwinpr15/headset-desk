import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// Grouped settings tile with an optional caption above it, in the style of
// system settings panes: one rounded surface per topic, never nested cards.
ColumnLayout {
    id: section
    required property var colors
    property string caption: ""
    property int padding: 16
    property color edge: colors.line
    default property alias content: layout.data
    spacing: 6
    Label {
        visible: section.caption.length > 0
        text: section.caption
        color: section.colors.muted
        font.pixelSize: 13; font.weight: Font.DemiBold
        Layout.leftMargin: 4
        Accessible.role: Accessible.Heading
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: layout.implicitHeight + section.padding * 2
        color: section.colors.card
        radius: 12
        border.color: section.edge
        Behavior on border.color { ColorAnimation { duration: 200 } }
        ColumnLayout {
            id: layout
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: section.padding }
            spacing: 12
        }
    }
}
