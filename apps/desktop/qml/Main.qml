import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    objectName: "mainWindow"
    title: "Headset Desk"
    width: 420; height: 700
    minimumWidth: 360; minimumHeight: 420
    visible: true
    color: appColors.page
    font.family: "Segoe UI Variable"
    font.pixelSize: 13
    palette.window: appColors.page
    palette.windowText: appColors.text
    palette.text: appColors.text
    palette.buttonText: appColors.text
    palette.highlight: appColors.accent
    readonly property var telemetry: device.state
    property double currentTime: Date.now()
    readonly property int ageMinutes: Math.max(0, Math.floor((currentTime - telemetry.updatedEpoch) / 60000))
    readonly property string updateAge: ageMinutes === 0 ? "Updated just now" : "Updated " + ageMinutes + " min ago"
    Timer { interval: 30000; running: window.visible; repeat: true; onTriggered: window.currentTime = Date.now() }
    Colors { id: appColors; dark: darkTheme }
    onClosing: function(close) {
        close.accepted = false
        if (trayAvailable) window.hide()
        else device.quit()
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: scroll.availableWidth
            spacing: 8
            Item { Layout.preferredHeight: 0 }
            RowLayout {
                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 20
                Label { text: "Headset Desk"; font.pixelSize: 20; font.weight: Font.DemiBold; color: appColors.text; Layout.fillWidth: true }
                Label { text: "READ ONLY"; font.pixelSize: 10; font.letterSpacing: 1; color: appColors.muted }
            }
            Label {
                visible: device.simulated
                text: "Preview · simulated data"
                color: appColors.muted; font.pixelSize: 12
                Layout.leftMargin: 20
            }
            Section {
                colors: appColors; Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        spacing: 4; Layout.fillWidth: true; Layout.maximumWidth: Infinity
                        Label { text: !telemetry.connected && device.devices.length > 0 ? device.devices[device.selected] : telemetry.model; font.pixelSize: 20; font.weight: Font.DemiBold; color: appColors.text }
                        Label { text: device.busy ? "Refreshing…" : telemetry.status; color: appColors.muted; font.pixelSize: 12 }
                    }
                    ColumnLayout {
                        spacing: 3
                        Label { text: telemetry.battery; font.pixelSize: 24; font.weight: Font.DemiBold; color: appColors.text; Layout.alignment: Qt.AlignRight
                            ToolTip.visible: batteryHover.hovered; ToolTip.text: telemetry.batteryError; ToolTip.delay: 600
                            HoverHandler { id: batteryHover }
                        }
                        Label { text: telemetry.charging; color: appColors.muted; font.pixelSize: 11; Layout.alignment: Qt.AlignRight
                            ToolTip.visible: chargingHover.hovered; ToolTip.text: telemetry.chargingError; ToolTip.delay: 600
                            HoverHandler { id: chargingHover }
                        }
                        Label { visible: telemetry.connected; text: window.updateAge; color: appColors.muted; font.pixelSize: 10; Layout.alignment: Qt.AlignRight }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    ComboBox {
                        objectName: "devicePicker"
                        visible: device.devices.length > 1
                        Layout.fillWidth: true; model: device.devices
                        currentIndex: device.selected; enabled: !device.busy && count > 0
                        displayText: count > 0 ? currentText : "No paired headphones"
                        Accessible.name: "Choose headphones"
                        onActivated: function(index) { device.select(index) }
                    }
                    Button { objectName: "connectButton"; text: telemetry.connected ? "Disconnect" : "Connect"; enabled: !device.busy && device.devices.length > 0; onClicked: device.toggleConnection() }
                    Item { visible: device.devices.length <= 1; Layout.fillWidth: true }
                    ToolButton {
                        objectName: "refreshButton"; text: "↻"; font.pixelSize: 20
                        enabled: !device.busy
                        Accessible.name: telemetry.connected ? "Refresh readings" : "Refresh paired headphone list"
                        ToolTip.visible: hovered; ToolTip.text: telemetry.connected ? "Refresh readings" : "Refresh list"; ToolTip.delay: 600
                        onClicked: telemetry.connected ? device.refresh() : device.rescan()
                    }
                }
                Label {
                    visible: device.message.length > 0; text: device.message
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; color: appColors.muted; font.pixelSize: 12
                }
            }
            Section {
                colors: appColors; Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Noise control"; font.pixelSize: 15; font.weight: Font.DemiBold; color: appColors.text; Layout.fillWidth: true }
                    Label { text: telemetry.noiseKnown ? ["Off", "NC", "Ambient"][telemetry.noiseMode] : "—"; color: appColors.muted; font.pixelSize: 12
                        ToolTip.visible: noiseHover.hovered; ToolTip.text: telemetry.noiseError
                        HoverHandler { id: noiseHover }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 4; enabled: false; opacity: 0.65
                    Repeater {
                        model: [{name: "NC", mode: 1}, {name: "Ambient", mode: 2}, {name: "Off", mode: 0}]
                        Button {
                            required property var modelData
                            Layout.fillWidth: true; text: modelData.name
                            checked: telemetry.noiseKnown && telemetry.noiseMode === modelData.mode
                            checkable: true; enabled: false
                            Accessible.name: modelData.name + (checked ? ", current mode, read only" : ", read only")
                        }
                    }
                }
                RowLayout {
                    visible: telemetry.noiseKnown && telemetry.noiseMode === 2
                    Layout.fillWidth: true
                    Label { text: "Ambient"; color: appColors.muted }
                    Slider { Layout.fillWidth: true; from: 1; to: 20; value: telemetry.ambient; enabled: false; Accessible.name: "Ambient level, read only" }
                    Label { text: telemetry.ambient; color: appColors.text }
                }
                Label {
                    text: "Changing settings isn’t available yet."
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; color: appColors.muted; font.pixelSize: 12
                }
            }
            Section {
                colors: appColors; Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Equalizer"; font.pixelSize: 15; font.weight: Font.DemiBold; color: appColors.text; Layout.fillWidth: true }
                    Label { text: "Read only"; color: appColors.muted; font.pixelSize: 11 }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Repeater {
                        model: ["400", "1k", "2.5k", "6.3k", "16k"]
                        ColumnLayout {
                            required property string modelData
                            required property int index
                            Layout.fillWidth: true; Layout.maximumWidth: Infinity; Layout.preferredWidth: 56; spacing: 3
                            Label {
                                text: telemetry.eqKnown ? (telemetry.bands[index] > 0 ? "+" : "") + telemetry.bands[index] : "—"
                                color: appColors.text; Layout.alignment: Qt.AlignHCenter
                                ToolTip.visible: eqHover.hovered; ToolTip.text: telemetry.eqError; ToolTip.delay: 600
                                HoverHandler { id: eqHover }
                            }
                            Slider {
                                objectName: "eqBand" + index
                                orientation: Qt.Vertical; from: -10; to: 10
                                value: telemetry.eqKnown ? telemetry.bands[index] : 0
                                enabled: false; Layout.preferredHeight: 160; Layout.alignment: Qt.AlignHCenter
                                Accessible.name: modelData + " Hz, read only"
                            }
                            Label { text: modelData; color: appColors.muted; font.pixelSize: 11; Layout.alignment: Qt.AlignHCenter }
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: appColors.line }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Clear Bass"; color: appColors.text; font.pixelSize: 12 }
                    Label { text: telemetry.eqKnown ? (telemetry.bass > 0 ? "+" : "") + telemetry.bass : "—"; color: appColors.text }
                    Slider { objectName: "clearBass"; Layout.fillWidth: true; from: -10; to: 10; value: telemetry.eqKnown ? telemetry.bass : 0; enabled: false; Accessible.name: "Clear Bass, read only" }
                }
            }
            RowLayout {
                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 16
                ToolButton { objectName: "detailsButton"; id: details; text: checked ? "Details  ▴" : "Details  ▾"; checkable: true; Accessible.name: "Headphone details" }
                Item { Layout.fillWidth: true }
            }
            ColumnLayout {
                visible: details.checked; spacing: 5; Layout.leftMargin: 24; Layout.rightMargin: 24; Layout.fillWidth: true
                Label { text: "Firmware   " + telemetry.firmware; color: appColors.muted; font.pixelSize: 12
                    ToolTip.visible: firmwareHover.hovered; ToolTip.text: telemetry.firmwareError
                    HoverHandler { id: firmwareHover }
                }
                Label { text: "Reported codec   " + telemetry.codec; color: appColors.muted; font.pixelSize: 12
                    ToolTip.visible: codecHover.hovered; ToolTip.text: telemetry.codec === "—" ? telemetry.codecError : "Headphone report; not a Windows codec measurement."
                    HoverHandler { id: codecHover }
                }
                Label { text: "Full refresh   " + telemetry.updated; color: appColors.muted; font.pixelSize: 12 }
                Label { text: "Model name comes from Windows pairing."; color: appColors.muted; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            }
            Item { Layout.preferredHeight: 8 }
        }
    }
}
