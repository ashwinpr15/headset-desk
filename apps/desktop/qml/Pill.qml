import QtQuick
import QtQuick.Controls
// Small status badge (for example "Experimental").
Rectangle {
    id: badge
    property alias text: badgeLabel.text
    property color tone: "#9d5d00"
    implicitWidth: badgeLabel.implicitWidth + 16; implicitHeight: 20; radius: 10
    color: Qt.alpha(tone, 0.14); border.color: Qt.alpha(tone, 0.5)
    Label { id: badgeLabel; anchors.centerIn: parent; color: badge.tone; font.pixelSize: 12; font.weight: Font.DemiBold }
}
