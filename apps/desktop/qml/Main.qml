import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    objectName: "mainWindow"
    title: "Headset Desk"
    width: 420; height: 720
    minimumWidth: 360; minimumHeight: 420
    visible: true
    color: appColors.page
    font.family: Qt.platform.os === "windows" ? "Segoe UI Variable" : Qt.application.font.family
    font.pixelSize: 13
    palette.window: appColors.page
    palette.windowText: appColors.text
    palette.text: appColors.text
    palette.buttonText: appColors.text
    palette.highlight: appColors.accent
    palette.accent: appColors.accent

    // ---- State. Semantics unchanged from 0.3.0: drafts stay local until Apply,
    // and ordinary busy notifications never overwrite a user's in-progress drag.
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
    function signed(value) { return (value > 0 ? "+" : "") + value }
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
    readonly property string updateAge: ageMinutes === 0 ? "updated just now" : "updated " + ageMinutes + " min ago"
    Timer { interval: 30000; running: window.visible; repeat: true; onTriggered: window.currentTime = Date.now() }
    readonly property int batteryLevel: { const n = parseInt(telemetry.battery); return isNaN(n) ? -1 : n }
    readonly property var modes: [
        {mode: 1, name: "Noise Cancelling", hint: "Blocks outside sound."},
        {mode: 2, name: "Ambient", hint: "Lets the room in."},
        {mode: 0, name: "Off", hint: "No noise processing."}
    ]
    readonly property int modeIndex: !telemetry.noiseKnown ? -1 : telemetry.noiseMode === 1 ? 0 : telemetry.noiseMode === 2 ? 1 : 2
    Colors { id: appColors; dark: darkTheme }
    onClosing: function(close) {
        close.accepted = false
        if (trayAvailable) window.hide()
        else device.quit()
    }

    component Chip: Rectangle {
        property alias label: chipText.text
        property color tone: appColors.muted
        implicitWidth: chipText.implicitWidth + 14; implicitHeight: 20; radius: 10
        color: Qt.alpha(tone, 0.12)
        Label { id: chipText; anchors.centerIn: parent; color: parent.tone; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6 }
    }
    // Own-drawn buttons so light/dark rendering never depends on native style assets.
    component PillButton: Button {
        id: pill
        property bool primary: false
        implicitHeight: 32
        leftPadding: 14; rightPadding: 14
        hoverEnabled: true
        font.pixelSize: 13; font.weight: Font.DemiBold
        background: Rectangle {
            radius: 8
            color: pill.primary && pill.enabled ? appColors.accent : appColors.fill
            Rectangle { anchors.fill: parent; radius: parent.radius; color: appColors.text
                opacity: !pill.enabled ? 0 : pill.down ? 0.12 : pill.hovered ? 0.06 : 0 }
        }
        contentItem: Label {
            text: pill.text; font: pill.font
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            color: !pill.enabled ? appColors.muted : pill.primary ? "#ffffff" : appColors.text
        }
    }
    component Divider: Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: appColors.line }
    component DetailRow: RowLayout {
        property alias name: nameLabel.text
        property alias value: valueLabel.text
        property string tip: ""
        Layout.fillWidth: true
        Label { id: nameLabel; color: appColors.text; Layout.fillWidth: true }
        Label { id: valueLabel; color: appColors.muted; horizontalAlignment: Text.AlignRight
            ToolTip.visible: tip.length > 0 && valueHover.hovered; ToolTip.text: tip; ToolTip.delay: 600
            HoverHandler { id: valueHover }
        }
    }

    ScrollView {
        id: scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: scroll.availableWidth
            spacing: 18
            Item { Layout.preferredHeight: 0 }

            // ---- Status chips
            RowLayout {
                Layout.fillWidth: true; Layout.leftMargin: 20; Layout.rightMargin: 20; Layout.bottomMargin: -8
                spacing: 6
                Label { text: "Headset Desk"; color: appColors.muted; font.pixelSize: 12; font.weight: Font.DemiBold; Layout.fillWidth: true }
                Chip { visible: device.simulated; label: "SIMULATED" }
                Chip { label: telemetry.controlsEnabled ? "EXPERIMENTAL" : "BETA"; tone: telemetry.controlsEnabled ? appColors.warn : appColors.muted }
            }

            // ---- Device
            Section {
                colors: appColors; Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                RowLayout {
                    Layout.fillWidth: true; spacing: 14
                    HeadphoneArt { colors: appColors; active: telemetry.connected; Layout.preferredWidth: 60; Layout.preferredHeight: 60 }
                    ColumnLayout {
                        spacing: 4; Layout.fillWidth: true
                        Label {
                            text: !telemetry.connected && device.devices.length > 0 ? device.devices[device.selected] : telemetry.model
                            font.pixelSize: 19; font.weight: Font.DemiBold; color: appColors.text
                            elide: Text.ElideRight; Layout.fillWidth: true
                        }
                        RowLayout {
                            spacing: 6
                            Rectangle {
                                width: 7; height: 7; radius: 4
                                color: device.busy ? appColors.warn : telemetry.connected ? appColors.green : appColors.muted
                            }
                            Label {
                                text: device.busy ? "Refreshing…" : telemetry.status + (telemetry.connected ? " · " + window.updateAge : "")
                                color: appColors.muted; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true
                            }
                        }
                    }
                    ColumnLayout {
                        spacing: 5
                        Label {
                            text: telemetry.battery; font.pixelSize: 24; font.weight: Font.DemiBold; color: appColors.text
                            Layout.alignment: Qt.AlignRight
                            ToolTip.visible: batteryHover.hovered && telemetry.batteryError.length > 0; ToolTip.text: telemetry.batteryError; ToolTip.delay: 600
                            HoverHandler { id: batteryHover }
                        }
                        Rectangle { // battery capsule
                            visible: window.batteryLevel >= 0
                            Layout.alignment: Qt.AlignRight
                            implicitWidth: 40; implicitHeight: 14; radius: 4
                            color: "transparent"; border.color: appColors.muted; border.width: 1
                            Rectangle {
                                x: 2; y: 2; height: parent.height - 4; radius: 2
                                width: Math.max(2, (parent.width - 4) * window.batteryLevel / 100)
                                color: window.batteryLevel <= 20 ? appColors.warn : appColors.green
                            }
                            Rectangle { x: parent.width + 1; y: 4; width: 2; height: 6; radius: 1; color: appColors.muted }
                        }
                        Label {
                            visible: window.batteryLevel >= 0 || telemetry.charging !== "—"
                            text: telemetry.charging; color: appColors.muted; font.pixelSize: 11; Layout.alignment: Qt.AlignRight
                            ToolTip.visible: chargingHover.hovered && telemetry.chargingError.length > 0; ToolTip.text: telemetry.chargingError; ToolTip.delay: 600
                            HoverHandler { id: chargingHover }
                        }
                    }
                }
                Divider {}
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    ComboBox {
                        objectName: "devicePicker"
                        visible: device.devices.length > 1
                        Layout.fillWidth: true; model: device.devices
                        currentIndex: device.selected; enabled: !device.busy && count > 0
                        displayText: count > 0 ? currentText : "No paired headphones"
                        Accessible.name: "Choose headphones"
                        onActivated: function(index) { device.select(index) }
                        implicitHeight: 32
                        leftPadding: 12; rightPadding: 30
                        font.pixelSize: 13
                        background: Rectangle { radius: 8; color: appColors.fill }
                        contentItem: Label {
                            text: parent.displayText; font: parent.font; color: appColors.text
                            verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
                        }
                        indicator: Label {
                            text: "▾"; color: appColors.muted; font.pixelSize: 12
                            x: parent.width - width - 12; y: (parent.height - height) / 2
                        }
                    }
                    Item { visible: device.devices.length <= 1; Layout.fillWidth: true }
                    PillButton {
                        objectName: "connectButton"
                        text: telemetry.connected ? "Disconnect" : "Connect"
                        primary: !telemetry.connected
                        enabled: !device.busy && device.devices.length > 0
                        onClicked: device.toggleConnection()
                    }
                    ToolButton {
                        objectName: "refreshButton"; text: "↻"; font.pixelSize: 18
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

            // ---- Noise control
            Section {
                colors: appColors; caption: "Noise control"
                Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                Rectangle { // segmented capsule
                    id: segments
                    Layout.fillWidth: true; implicitHeight: 40; radius: 10
                    color: appColors.fill
                    enabled: window.controlsActive
                    opacity: enabled ? 1 : 0.6
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                    readonly property real slot: (width - 6) / 3
                    Rectangle { // sliding selection; stays visible while disabled so the current mode is readable
                        visible: window.modeIndex >= 0
                        x: 3 + window.modeIndex * segments.slot; y: 3
                        width: segments.slot; height: segments.height - 6; radius: 8
                        color: appColors.thumb
                        border.color: appColors.dark ? "transparent" : appColors.line
                        Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                    }
                    Row {
                        x: 3; y: 3
                        Repeater {
                            model: window.modes
                            AbstractButton {
                                id: segment
                                required property var modelData
                                required property int index
                                objectName: "noiseMode" + modelData.mode
                                width: segments.slot; height: segments.height - 6
                                checked: telemetry.noiseKnown && telemetry.noiseMode === modelData.mode
                                checkable: false
                                hoverEnabled: true
                                Accessible.role: Accessible.RadioButton
                                Accessible.name: modelData.name + (checked ? ", current mode" : "")
                                onClicked: { ambientCommit.stop(); device.setNoise(modelData.mode) }
                                contentItem: Label {
                                    text: segment.modelData.name
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                    fontSizeMode: Text.HorizontalFit; minimumPixelSize: 11
                                    font.pixelSize: 13; font.weight: segment.checked ? Font.DemiBold : Font.Normal
                                    color: !segment.checked ? appColors.muted
                                         : segment.modelData.mode === 1 ? appColors.accentText
                                         : segment.modelData.mode === 2 ? appColors.green : appColors.text
                                }
                                background: Rectangle {
                                    radius: 8; color: appColors.text
                                    opacity: segment.hovered && !segment.checked && segment.enabled ? 0.05 : 0
                                }
                            }
                        }
                    }
                }
                Label {
                    text: window.modeIndex >= 0 ? window.modes[window.modeIndex].hint : "Current mode unknown."
                    color: appColors.muted; font.pixelSize: 12
                }
                RowLayout {
                    visible: telemetry.noiseKnown && telemetry.noiseMode === 2
                    Layout.fillWidth: true; spacing: 10
                    Label { text: "Level"; color: appColors.text }
                    HdSlider {
                        id: ambientSlider; objectName: "ambientLevel"
                        colors: appColors; tint: appColors.green
                        Layout.fillWidth: true; from: 1; to: 20
                        value: telemetry.ambient; enabled: window.controlsActive; Accessible.name: "Ambient level"
                        onMoved: { if (!pressed) ambientCommit.restart() }
                        onPressedChanged: { if (!pressed && window.controlsActive) { ambientCommit.stop(); device.setNoise(2, Math.round(value)) } }
                    }
                    Label { text: Math.round(ambientSlider.value); color: appColors.text; font.weight: Font.DemiBold; Layout.preferredWidth: 22; horizontalAlignment: Text.AlignRight }
                }
                Divider {}
                RowLayout {
                    Layout.fillWidth: true; spacing: 12
                    ColumnLayout {
                        spacing: 2; Layout.fillWidth: true
                        Label { text: "Experimental controls"; color: appColors.text }
                        Label {
                            text: !telemetry.canEnableControls && !telemetry.controlsEnabled
                                  ? "Connect headphones to allow changes."
                                  : "Each change is checked by reading it back. Turns off when you disconnect."
                            color: appColors.muted; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true
                        }
                    }
                    Switch {
                        objectName: "controlsSwitch"
                        checked: telemetry.controlsEnabled
                        enabled: telemetry.canEnableControls && !device.busy
                        Accessible.name: "Enable experimental controls for this connection"
                        onToggled: device.enableControls(checked)
                    }
                }
            }

            // ---- Equalizer
            Section {
                colors: appColors; caption: "Equalizer"
                Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: !telemetry.eqKnown ? "Current EQ unknown." : window.eqDirty ? "Unsaved changes" : "Range ±10"
                        color: window.eqDirty ? appColors.warn : appColors.muted; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight
                    }
                    PillButton { objectName: "resetEq"; text: "Reset"; enabled: window.eqDirty && !device.busy; onClicked: window.resetEq(); Accessible.name: "Discard EQ edits" }
                    PillButton { objectName: "applyEq"; text: "Apply"; primary: true; enabled: window.controlsActive && window.eqDirty; onClicked: device.applyEqualizer(window.bassDraft, window.eqDraft); Accessible.name: "Apply equalizer" }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 0
                    Repeater {
                        model: ["400", "1k", "2.5k", "6.3k", "16k"]
                        ColumnLayout {
                            required property string modelData
                            required property int index
                            Layout.fillWidth: true; Layout.maximumWidth: Infinity; Layout.preferredWidth: 56; spacing: 4
                            Label {
                                text: telemetry.eqKnown ? window.signed(window.eqDraft[index]) : "—"
                                color: appColors.text; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter
                                ToolTip.visible: eqHover.hovered && telemetry.eqError.length > 0; ToolTip.text: telemetry.eqError; ToolTip.delay: 600
                                HoverHandler { id: eqHover }
                            }
                            HdSlider {
                                objectName: "eqBand" + index
                                colors: appColors; centered: true; known: telemetry.eqKnown
                                orientation: Qt.Vertical; from: -10; to: 10
                                value: window.eqDraft[index]
                                onMoved: window.editBand(index, value)
                                enabled: window.controlsActive
                                Layout.preferredHeight: 150; Layout.alignment: Qt.AlignHCenter
                                Accessible.name: modelData + " Hz"
                            }
                            Label { text: modelData; color: appColors.muted; font.pixelSize: 11; Layout.alignment: Qt.AlignHCenter }
                        }
                    }
                }
                Divider {}
                RowLayout {
                    Layout.fillWidth: true; spacing: 10
                    Label { text: "Clear Bass"; color: appColors.text }
                    HdSlider {
                        objectName: "clearBass"
                        colors: appColors; centered: true; known: telemetry.eqKnown
                        Layout.fillWidth: true; from: -10; to: 10
                        value: window.bassDraft; onMoved: window.bassDraft = Math.round(value)
                        enabled: window.controlsActive; Accessible.name: "Clear Bass"
                    }
                    Label { text: telemetry.eqKnown ? window.signed(window.bassDraft) : "—"; color: appColors.text; font.weight: Font.DemiBold; Layout.preferredWidth: 26; horizontalAlignment: Text.AlignRight }
                }
            }

            // ---- Details (progressive disclosure)
            Section {
                colors: appColors; padding: 4
                Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16
                AbstractButton {
                    id: details; objectName: "detailsButton"
                    checkable: true; hoverEnabled: true
                    Layout.fillWidth: true; implicitHeight: 40
                    Accessible.name: "Headphone details"
                    background: Rectangle { radius: 9; color: appColors.text; opacity: details.hovered ? 0.04 : 0 }
                    contentItem: RowLayout {
                        Label { text: "Details"; color: appColors.text; Layout.fillWidth: true; Layout.leftMargin: 12 }
                        Label {
                            text: "›"; color: appColors.muted; font.pixelSize: 18; Layout.rightMargin: 12
                            rotation: details.checked ? 90 : 0
                            Behavior on rotation { NumberAnimation { duration: 150 } }
                        }
                    }
                }
                ColumnLayout {
                    visible: details.checked; spacing: 10
                    Layout.fillWidth: true; Layout.leftMargin: 12; Layout.rightMargin: 12; Layout.bottomMargin: 10
                    Divider {}
                    DetailRow { name: "Firmware"; value: telemetry.firmware; tip: telemetry.firmwareError }
                    DetailRow { name: "Reported codec"; value: telemetry.codec
                        tip: telemetry.codec === "—" ? telemetry.codecError : "Headphone report; not a Windows codec measurement." }
                    DetailRow { name: "Last full refresh"; value: telemetry.updated }
                    Label { text: "Model name comes from Windows pairing."; color: appColors.muted; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
            }
            Item { Layout.preferredHeight: 4 }
        }
    }
}
