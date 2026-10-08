import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
// One setting: optional icon, title, a line of explanation, and a control on the right.
// Windows 11 settings pattern: 4 px corners, hairline stroke, roughly 64 px tall.
Rectangle {
    id: card
    required property var colors
    property string title: ""
    property string description: ""
    property string icon: ""
    property string badge: ""
    property color iconTint: colors.text
    property bool dim: false
    property color edge: colors.line
    default property alias trailing: trailingRow.data
    Layout.fillWidth: true
    implicitHeight: Math.max(64, body.implicitHeight + 24)
    radius: 4
    color: colors.card
    border.color: edge
    Behavior on border.color { ColorAnimation { duration: 180 } }
    RowLayout {
        id: body
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 16; rightMargin: 16 }
        spacing: 16
        Glyph { visible: card.icon.length > 0; kind: card.icon; tint: card.dim ? card.colors.faint : card.iconTint; size: 20
            Layout.alignment: Qt.AlignVCenter }
        ColumnLayout {
            spacing: 1; Layout.fillWidth: true
            RowLayout {
                spacing: 8; Layout.fillWidth: true
                Label { text: card.title; color: card.dim ? card.colors.muted : card.colors.text
                    font.pixelSize: 14; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                Pill { visible: card.badge.length > 0; text: card.badge; tone: card.colors.warn; Layout.alignment: Qt.AlignVCenter }
            }
            Label { visible: card.description.length > 0; text: card.description; color: card.colors.muted
                font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        }
        RowLayout { id: trailingRow; spacing: 8; Layout.alignment: Qt.AlignVCenter }
    }
}
