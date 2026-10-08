import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Rectangle {
    id: section
    required property var colors
    default property alias content: layout.data
    implicitHeight: layout.implicitHeight + 28
    color: colors.card
    radius: 8
    border.color: colors.line
    ColumnLayout {
        id: layout
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 }
        spacing: 12
    }
}
