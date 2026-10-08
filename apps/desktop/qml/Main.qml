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
    readonly property bool controlsActive: telemetry.controlsEnabled && !device.busy
    property var eqDraft: [0, 0, 0, 0, 0]
    property int bassDraft: 0
    property string confirmedEq: ""
    readonly property bool eqDirty: telemetry.eqKnown && (JSON.stringify(eqDraft) !== JSON.stringify(telemetry.bands) || bassDraft !== telemetry.bass)
    function resetEq() {
        eqDraft = telemetry.eqKnown ? telemetry.bands.slice() : [0, 0, 0, 0, 0]
        bassDraft = telemetry.eqKnown ? telemetry.bass : 0
    }
    function editBand(index, value) { const draft = eqDraft.slice(); draft[index] = Math.round(value); eqDraft = draft }
    Connections {
        target: device
        function onChanged() {
            if (!device.busy) {
                const signature = JSON.stringify(telemetry.bands) + ":" + telemetry.bass + ":" + telemetry.controlsEnabled + ":" + telemetry.connected
                if (signature !== confirmedEq) { confirmedEq = signature; resetEq() }
            }
            if (!window.controlsActive) ambientCommit.stop()
        }
    }
    Component.onCompleted: resetEq()
    Timer { id: ambientCommit; interval: 250; onTriggered: { if (window.controlsActive && telemetry.noiseMode === 2) device.setNoise(2, Math.round(ambientSlider.value)) } }
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
                Label { text: telemetry.controlsEnabled ? "EXPERIMENTAL" : "BETA · READ ONLY"; font.pixelSize: 10; font.letterSpacing: 1; color: appColors.muted }
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
                    Switch {
                        objectName: "controlsSwitch"
                        Layout.preferredHeight: 32
                        text: "Controls"; checked: telemetry.controlsEnabled
                        enabled: telemetry.canEnableControls && !device.busy
                        Accessible.name: "Enable experimental controls for this connection"
                        onToggled: device.enableControls(checked)
                    }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 4; enabled: window.controlsActive; opacity: enabled ? 1 : 0.65
                    Repeater {
                        model: [{name: "NC", mode: 1}, {name: "Ambient", mode: 2}, {name: "Off", mode: 0}]
                        Button {
                            required property var modelData
                            Layout.fillWidth: true; text: modelData.name
                            checked: telemetry.noiseKnown && telemetry.noiseMode === modelData.mode
                            objectName: "noiseMode" + modelData.mode
                            checkable: false
                            Accessible.name: modelData.name + (checked ? ", current mode" : "")
                            onClicked: { ambientCommit.stop(); device.setNoise(modelData.mode) }
                        }
                    }
                }
                RowLayout {
                    visible: telemetry.noiseKnown && telemetry.noiseMode === 2
                    Layout.fillWidth: true
                    Label { text: "Ambient"; color: appColors.muted }
                    Slider {
                        id: ambientSlider; objectName: "ambientLevel"
                        Layout.fillWidth: true; from: 1; to: 20; stepSize: 1; snapMode: Slider.SnapAlways
                        value: telemetry.ambient; enabled: window.controlsActive; Accessible.name: "Ambient level"
                        onMoved: { if (!pressed) ambientCommit.restart() }
                        onPressedChanged: { if (!pressed && window.controlsActive) { ambientCommit.stop(); device.setNoise(2, Math.round(value)) } }
                    }
                    Label { text: Math.round(ambientSlider.value); color: appColors.text }
                }
                Label {
                    text: "Experimental controls · writes not hardware-verified."
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; color: appColors.muted; font.pixelSize: 12
                }
            }
            Section {
                colors: appColors; Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Equalizer"; font.pixelSize: 15; font.weight: Font.DemiBold; color: appColors.text; Layout.fillWidth: true }
                    ToolButton { objectName: "resetEq"; text: "Reset"; Layout.preferredHeight: 32; enabled: window.eqDirty && !device.busy; onClicked: window.resetEq(); Accessible.name: "Discard EQ edits" }
                    Button { objectName: "applyEq"; text: "Apply"; Layout.preferredHeight: 32; enabled: window.controlsActive && window.eqDirty; onClicked: device.applyEqualizer(window.bassDraft, window.eqDraft); Accessible.name: "Apply equalizer" }
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
                                text: telemetry.eqKnown ? (window.eqDraft[index] > 0 ? "+" : "") + window.eqDraft[index] : "—"
                                color: appColors.text; Layout.alignment: Qt.AlignHCenter
                                ToolTip.visible: eqHover.hovered; ToolTip.text: telemetry.eqError; ToolTip.delay: 600
                                HoverHandler { id: eqHover }
                            }
                            Slider {
                                objectName: "eqBand" + index
                                orientation: Qt.Vertical; from: -10; to: 10
                                value: window.eqDraft[index]; stepSize: 1; snapMode: Slider.SnapAlways
                                onMoved: window.editBand(index, value)
                                enabled: window.controlsActive; Layout.preferredHeight: 136; Layout.alignment: Qt.AlignHCenter
                                Accessible.name: modelData + " Hz"
                            }
                            Label { text: modelData; color: appColors.muted; font.pixelSize: 11; Layout.alignment: Qt.AlignHCenter }
                        }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: appColors.line }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Clear Bass"; color: appColors.text; font.pixelSize: 12 }
                    Label { text: telemetry.eqKnown ? (window.bassDraft > 0 ? "+" : "") + window.bassDraft : "—"; color: appColors.text }
                    Slider { objectName: "clearBass"; Layout.fillWidth: true; from: -10; to: 10; stepSize: 1; snapMode: Slider.SnapAlways; value: window.bassDraft; onMoved: window.bassDraft = Math.round(value); enabled: window.controlsActive; Accessible.name: "Clear Bass" }
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
            Item { Layout.preferredHeight: 0 }
        }
    }
}
