import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// A titled group of cards. Windows settings spacing: 4 px between cards in a group.
ColumnLayout {
    id: section
    required property var colors
    property string caption: ""
    default property alias content: layout.data
    spacing: 8
    Label {
        visible: section.caption.length > 0
        text: section.caption
        color: section.colors.text
        font.pixelSize: 14; font.weight: Font.DemiBold
        Layout.topMargin: 8
        Accessible.role: Accessible.Heading
    }
    ColumnLayout { id: layout; Layout.fillWidth: true; spacing: 4 }
}
