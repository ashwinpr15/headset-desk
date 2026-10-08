import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

ApplicationWindow {
    id: window
    objectName: "mainWindow"
    title: "Headset Desk"
    width: Math.min(1040, Screen.desktopAvailableWidth - 48)
    height: Math.min(720, Screen.desktopAvailableHeight - 48)
    minimumWidth: 360; minimumHeight: 420
    visible: true
    color: appColors.page
    font.family: Qt.platform.os === "windows" ? "Segoe UI Variable" : Qt.application.font.family
    font.pixelSize: 14
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
    function signed(value) { return (value > 0 ? "+" : value < 0 ? "−" : "") + Math.abs(value) }
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

    // ---- Presentation
    // Windows "Animation effects" off -> no decorative motion (set by main.cpp).
    readonly property bool calm: typeof reduceMotion === "boolean" && reduceMotion
    function ms(duration) { return calm ? 0 : duration }
    readonly property int batteryLevel: { const n = parseInt(telemetry.battery); return isNaN(n) ? -1 : n }
    readonly property var modes: [
        {mode: 1, name: "Noise Cancelling", sub: "Blocks sound", icon: "nc", hint: "Blocks outside sound."},
        {mode: 2, name: "Ambient", sub: "Lets sound in", icon: "ambient", hint: "Lets the room in. Drag to set how much."},
        {mode: 0, name: "Off", sub: "No processing", icon: "off", hint: "No noise processing."}
    ]
    readonly property int modeIndex: !telemetry.noiseKnown ? -1 : telemetry.noiseMode === 1 ? 0 : telemetry.noiseMode === 2 ? 1 : 2
    Colors { id: appColors; dark: darkTheme }
    onClosing: function(close) {
        close.accepted = false
        if (trayAvailable) window.hide()
        else device.quit()
    }

    // Messages can be closed; routine confirmations also clear themselves after a few seconds.
    property string dismissed: ""
    Timer {
        interval: 6000; running: device.message.indexOf("confirmed by headphones") >= 0 && device.message !== window.dismissed
        onTriggered: window.dismissed = device.message
    }

    // ---- Navigation. Windows breakpoints: expanded pane from 1008 px, icon rail above 640 px,
    // and a menu button that opens the pane as an overlay at or below 640 px.
    property int page: 0
    property bool paneOpen: false
    readonly property int navMode: width >= 1008 ? 2 : width > 640 ? 1 : 0
    readonly property bool paneExpanded: navMode === 2 || (navMode === 0 && paneOpen)
    readonly property int paneWidth: navMode === 2 ? 232 : navMode === 1 ? 48 : 0
    readonly property int pad: navMode === 0 ? 16 : 32
    readonly property var pageNames: ["Headphones", "Sound", "Features", "Device"]

    // Keyboard focus ring shared by app-drawn controls.
    component FocusRing: Rectangle {
        required property Item target
        anchors.fill: parent; anchors.margins: -2
        radius: 6; color: "transparent"
        border.width: 2; border.color: appColors.text
        visible: target.visualFocus
    }
    // Own-drawn buttons (4 px corners, 32 px tall) so light/dark never depends on native style assets.
    component HdButton: Button {
        id: pill
        property bool primary: false
        implicitHeight: 32
        leftPadding: 12; rightPadding: 12
        hoverEnabled: true
        font.pixelSize: 14
        background: Rectangle {
            radius: 4
            color: !pill.enabled ? (pill.primary ? appColors.fill : appColors.fill)
                 : pill.primary ? (pill.down ? Qt.darker(appColors.accent, 1.15) : pill.hovered ? Qt.darker(appColors.accent, 1.07) : appColors.accent)
                 : pill.down ? appColors.fillPressed : pill.hovered ? appColors.fillHover : appColors.fill
            border.color: pill.primary && pill.enabled ? "transparent" : appColors.line
            Behavior on color { ColorAnimation { duration: window.ms(100) } }
            FocusRing { target: pill }
        }
        contentItem: Label {
            text: pill.text; font: pill.font
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            color: !pill.enabled ? appColors.faint : pill.primary ? appColors.onAccent : appColors.text
        }
    }
    component Divider: Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: appColors.divider }
    // Expands and collapses its content by animating height, clipping while it moves.
    component Reveal: Item {
        id: reveal
        property bool open: false
        default property alias content: inner.data
        Layout.fillWidth: true
        Layout.preferredHeight: open ? inner.implicitHeight : 0
        visible: Layout.preferredHeight > 0.5
        clip: true
        opacity: open ? 1 : 0
        Behavior on Layout.preferredHeight { NumberAnimation { duration: window.ms(200); easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: window.ms(160) } }
        ColumnLayout { id: inner; width: parent.width; spacing: 8 }
    }
    // Page frame: 28 px title, then a message bar that appears only when there is something to say.
    component Page: ScrollView {
        id: pageView
        required property int index
        property string title: ""
        default property alias body: bodyColumn.data
        readonly property real columnWidth: Math.min(availableWidth - 2 * window.pad, 960)
        visible: window.page === index
        contentWidth: availableWidth
        contentHeight: bodyColumn.implicitHeight + 72
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        onVisibleChanged: if (visible && !window.calm) fadeIn.restart()
        NumberAnimation { id: fadeIn; target: pageView; property: "opacity"; from: 0; to: 1; duration: 180; easing.type: Easing.OutCubic }
        ColumnLayout {
            id: bodyColumn
            x: window.pad; y: 20
            width: pageView.columnWidth
            spacing: 16
            Label {
                text: pageView.title; color: appColors.text
                font.pixelSize: 28; font.weight: Font.DemiBold
                Accessible.role: Accessible.Heading
            }
            Reveal {
                open: device.message.length > 0 && device.message !== window.dismissed
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: msgRow.implicitHeight + 20; radius: 4
                    color: appColors.card; border.color: appColors.line
                    RowLayout {
                        id: msgRow
                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; leftMargin: 14; rightMargin: 14 }
                        spacing: 12
                        Glyph { kind: "device"; tint: appColors.accent; size: 18 }
                        Label { text: device.message; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: appColors.text; font.pixelSize: 14 }
                        Button {
                            id: closeMessage
                            implicitWidth: 28; implicitHeight: 28; hoverEnabled: true
                            Accessible.name: "Dismiss message"
                            onClicked: window.dismissed = device.message
                            background: Rectangle { radius: 4; color: closeMessage.down ? appColors.fillPressed : closeMessage.hovered ? appColors.fill : "transparent"; FocusRing { target: closeMessage } }
                            contentItem: Item { Glyph { anchors.centerIn: parent; kind: "close"; size: 12; tint: appColors.muted } }
                        }
                    }
                }
            }
        }
    }
    component NavItem: AbstractButton {
        id: nav
        property string glyph
        required property int pageIndex
        readonly property bool selected: window.page === pageIndex
        Layout.fillWidth: true
        implicitHeight: 40
        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        Accessible.role: Accessible.PageTab
        Accessible.name: text + (selected ? ", selected" : "")
        onClicked: { window.page = pageIndex; window.paneOpen = false }
        background: Rectangle {
            anchors { fill: parent; leftMargin: 4; rightMargin: 4; topMargin: 2; bottomMargin: 2 }
            radius: 4
            color: nav.selected ? appColors.fillHover : nav.down ? appColors.fillPressed : nav.hovered ? appColors.fill : "transparent"
            Behavior on color { ColorAnimation { duration: window.ms(100) } }
            Rectangle { // selection indicator on the left edge
                x: 0; anchors.verticalCenter: parent.verticalCenter
                width: 3; radius: 1.5; color: appColors.accent
                height: nav.selected ? 16 : 0
                Behavior on height { NumberAnimation { duration: window.ms(180); easing.type: Easing.OutCubic } }
            }
            FocusRing { target: nav }
        }
        contentItem: Item {
            Glyph { x: 14; anchors.verticalCenter: parent.verticalCenter; kind: nav.glyph; size: 20; tint: nav.selected ? appColors.accent : appColors.text }
            Label {
                x: 46; anchors.verticalCenter: parent.verticalCenter
                visible: window.paneExpanded; text: nav.text; color: appColors.text; font.pixelSize: 14
                font.weight: nav.selected ? Font.DemiBold : Font.Normal
            }
        }
        ToolTip.visible: hovered && !window.paneExpanded; ToolTip.text: text; ToolTip.delay: 500
    }
    // One of the three noise modes. Selected: tinted fill and tinted border; the others stay quiet.
    component ModeTile: AbstractButton {
        id: tile
        required property var modelData
        readonly property bool current: telemetry.noiseKnown && telemetry.noiseMode === modelData.mode
        readonly property color tone: modelData.mode === 1 ? appColors.accent : modelData.mode === 2 ? appColors.green : appColors.text
        objectName: "noiseMode" + modelData.mode
        Layout.fillWidth: true; Layout.preferredWidth: 100
        implicitHeight: 104
        checked: current; checkable: false
        hoverEnabled: true; focusPolicy: Qt.StrongFocus
        Accessible.role: Accessible.RadioButton
        Accessible.name: modelData.name + (current ? ", current mode" : "")
        scale: down && enabled ? 0.98 : 1
        Behavior on scale { NumberAnimation { duration: window.ms(90) } }
        background: Rectangle {
            radius: 8
            color: tile.current ? Qt.alpha(tile.tone, appColors.dark ? 0.16 : 0.10)
                 : tile.hovered && tile.enabled ? appColors.inset2 : appColors.inset
            border.width: tile.current ? 2 : 1
            border.color: tile.current ? tile.tone : appColors.line
            Behavior on color { ColorAnimation { duration: window.ms(150) } }
            Behavior on border.color { ColorAnimation { duration: window.ms(150) } }
            FocusRing { target: tile; radius: 10; anchors.margins: -3 }
        }
        contentItem: ColumnLayout {
            spacing: 6
            Item { Layout.fillHeight: true }
            Glyph { Layout.alignment: Qt.AlignHCenter; kind: tile.modelData.icon; size: 28; weight: 1.5
                tint: tile.current ? tile.tone : appColors.muted }
            Label {
                Layout.alignment: Qt.AlignHCenter; Layout.fillWidth: true
                text: tile.modelData.name; horizontalAlignment: Text.AlignHCenter
                fontSizeMode: Text.HorizontalFit; minimumPixelSize: 11; font.pixelSize: 14
                font.weight: tile.current ? Font.DemiBold : Font.Normal
                color: tile.current ? appColors.text : appColors.muted
            }
            Label {
                Layout.alignment: Qt.AlignHCenter; Layout.fillWidth: true
                text: tile.modelData.sub; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight
                color: appColors.faint; font.pixelSize: 12
            }
            Item { Layout.fillHeight: true }
        }
        opacity: enabled || current ? 1 : 0.7
    }

    // ================= Layout =================
    Item {
        id: root
        anchors.fill: parent
        clip: true

        // Content layer: a slightly lighter surface with a rounded leading corner, as in Windows 11 apps.
        Rectangle {
            id: layer
            anchors { left: parent.left; top: parent.top; right: parent.right; bottom: parent.bottom
                leftMargin: window.paneWidth; topMargin: window.navMode === 0 ? 48 : 0; rightMargin: -8; bottomMargin: -8 }
            Behavior on anchors.leftMargin { NumberAnimation { duration: window.ms(180); easing.type: Easing.OutCubic } }
            radius: 8
            color: appColors.layer
            border.color: appColors.line

            Item {
                anchors { fill: parent; rightMargin: 8; bottomMargin: 8 }
                Page {
                    anchors.fill: parent
                    index: 0; title: "Headphones"
                    Label {
                        visible: device.simulated
                        text: "Preview with simulated data"
                        color: appColors.warn; font.pixelSize: 12; font.weight: Font.DemiBold
                    }
                    // ---- Device card
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: hero.implicitHeight + 40
                        radius: 8; color: appColors.card; border.color: appColors.line
                        GridLayout {
                            id: hero
                            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 24 }
                            columns: width >= 520 ? 2 : 1
                            columnSpacing: 32; rowSpacing: 16
                            Item { // stage: battery ring, headphones and the level in the gap
                                Layout.preferredWidth: 216; Layout.preferredHeight: 216
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                BatteryRing {
                                    anchors.fill: parent
                                    level: window.batteryLevel; reduceMotion: window.calm
                                    trackColor: Qt.alpha(appColors.text, 0.12)
                                    levelColor: window.batteryLevel >= 0 && window.batteryLevel <= 20 ? appColors.warn : appColors.green
                                    opacity: telemetry.connected ? 1 : 0.5
                                }
                                HeadphoneArt {
                                    anchors.centerIn: parent; anchors.verticalCenterOffset: -4
                                    width: 136; height: 136
                                    colors: appColors; active: telemetry.connected; reduceMotion: window.calm
                                    mode: telemetry.connected && telemetry.noiseKnown ? telemetry.noiseMode : -1
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: -1
                                    text: telemetry.battery; color: appColors.text; font.pixelSize: 14; font.weight: Font.DemiBold
                                    Accessible.name: "Battery " + telemetry.battery
                                    ToolTip.visible: batteryHover.hovered && telemetry.batteryError.length > 0; ToolTip.text: telemetry.batteryError; ToolTip.delay: 600
                                    HoverHandler { id: batteryHover }
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                                spacing: 4
                                Label {
                                    text: !telemetry.connected && device.devices.length > 0 ? device.devices[device.selected] : telemetry.model
                                    font.pixelSize: 28; font.weight: Font.DemiBold; color: appColors.text
                                    elide: Text.ElideRight; Layout.fillWidth: true
                                }
                                RowLayout {
                                    spacing: 8
                                    Rectangle {
                                        id: statusDot
                                        width: 8; height: 8; radius: 4
                                        color: device.busy ? appColors.warn : telemetry.connected ? appColors.green : appColors.faint
                                        Behavior on color { ColorAnimation { duration: window.ms(200) } }
                                        SequentialAnimation on opacity { // gentle pulse only while working
                                            running: device.busy && !window.calm; loops: Animation.Infinite
                                            NumberAnimation { to: 0.3; duration: 500; easing.type: Easing.InOutSine }
                                            NumberAnimation { to: 1; duration: 500; easing.type: Easing.InOutSine }
                                            onRunningChanged: if (!running) statusDot.opacity = 1
                                        }
                                    }
                                    Label {
                                        text: device.busy ? "Working…" : telemetry.status + (telemetry.connected ? ", " + window.updateAge : "")
                                        color: appColors.muted; font.pixelSize: 14; elide: Text.ElideRight; Layout.fillWidth: true
                                    }
                                }
                                Label {
                                    visible: window.batteryLevel >= 0 || telemetry.charging !== "—"
                                    text: (window.batteryLevel >= 0 ? telemetry.battery + " battery" : "Battery unknown") + (telemetry.charging !== "—" ? " · " + telemetry.charging : "")
                                    color: appColors.muted; font.pixelSize: 12
                                    ToolTip.visible: chargingHover.hovered && telemetry.chargingError.length > 0; ToolTip.text: telemetry.chargingError; ToolTip.delay: 600
                                    HoverHandler { id: chargingHover }
                                }
                                RowLayout {
                                    Layout.topMargin: 16; Layout.fillWidth: true; spacing: 8
                                    ComboBox {
                                        id: picker
                                        objectName: "devicePicker"
                                        visible: device.devices.length > 1
                                        Layout.fillWidth: true; model: device.devices
                                        currentIndex: device.selected; enabled: !device.busy && count > 0
                                        displayText: count > 0 ? currentText : "No paired headphones"
                                        Accessible.name: "Choose headphones"
                                        onActivated: function(index) { device.select(index) }
                                        implicitHeight: 32
                                        leftPadding: 12; rightPadding: 30
                                        font.pixelSize: 14
                                        hoverEnabled: true
                                        background: Rectangle {
                                            radius: 4; color: picker.hovered && picker.enabled ? appColors.fillHover : appColors.fill
                                            border.color: appColors.line
                                            FocusRing { target: picker }
                                        }
                                        contentItem: Label {
                                            text: picker.displayText; font: picker.font; color: appColors.text
                                            verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
                                        }
                                        indicator: Label {
                                            text: "▾"; color: appColors.muted; font.pixelSize: 12
                                            x: picker.width - width - 12; y: (picker.height - height) / 2
                                            rotation: picker.popup.visible ? 180 : 0
                                            Behavior on rotation { NumberAnimation { duration: window.ms(150) } }
                                        }
                                    }
                                    Item { visible: device.devices.length <= 1; Layout.fillWidth: true }
                                    HdButton {
                                        objectName: "connectButton"
                                        text: telemetry.connected ? "Disconnect" : "Connect"
                                        primary: !telemetry.connected
                                        enabled: !device.busy && device.devices.length > 0
                                        onClicked: device.toggleConnection()
                                    }
                                    Button {
                                        id: refresh
                                        objectName: "refreshButton"
                                        implicitWidth: 32; implicitHeight: 32
                                        enabled: !device.busy; hoverEnabled: true
                                        Accessible.name: telemetry.connected ? "Refresh readings" : "Refresh paired headphone list"
                                        ToolTip.visible: hovered; ToolTip.text: telemetry.connected ? "Refresh readings" : "Refresh list"; ToolTip.delay: 600
                                        onClicked: telemetry.connected ? device.refresh() : device.rescan()
                                        background: Rectangle {
                                            radius: 4; color: refresh.down ? appColors.fillPressed : refresh.hovered && refresh.enabled ? appColors.fillHover : "transparent"
                                            FocusRing { target: refresh }
                                        }
                                        contentItem: Item {
                                            Glyph {
                                                id: refreshGlyph
                                                anchors.centerIn: parent; kind: "refresh"; size: 18
                                                tint: refresh.enabled ? appColors.text : appColors.faint
                                                RotationAnimation on rotation { // turns while a refresh is running
                                                    running: device.busy && !window.calm; loops: Animation.Infinite
                                                    from: 0; to: 360; duration: 900
                                                    onRunningChanged: if (!running) refreshGlyph.rotation = 0
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // ---- Noise control (and the switch that governs it and the EQ)
                    Section {
                        colors: appColors; caption: "Noise control"
                        Layout.fillWidth: true
                        SettingsCard {
                            colors: appColors
                            icon: "lock"; iconTint: telemetry.controlsEnabled ? appColors.warn : appColors.text
                            title: "Allow changes"; badge: "Experimental"
                            description: telemetry.controlsEnabled ? "On for this connection. Each change is checked by reading it back."
                                : telemetry.canEnableControls ? "Turn on to change noise control, EQ and features. Turns off when you disconnect."
                                : "Connect your headphones first."
                            edge: telemetry.controlsEnabled ? Qt.alpha(appColors.warn, 0.6) : appColors.line
                            Switch {
                                objectName: "controlsSwitch"
                                checked: telemetry.controlsEnabled
                                enabled: telemetry.canEnableControls && !device.busy
                                Accessible.name: "Allow experimental changes for this connection"
                                onToggled: device.enableControls(checked)
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: noiseBody.implicitHeight + 32
                            radius: 4; color: appColors.card; border.color: appColors.line
                            ColumnLayout {
                                id: noiseBody
                                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 16 }
                                spacing: 12
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 8
                                    enabled: window.controlsActive
                                    Repeater { model: window.modes; ModeTile {} }
                                }
                                Label {
                                    text: window.modeIndex >= 0 ? window.modes[window.modeIndex].hint : "Current mode unknown."
                                    color: appColors.muted; font.pixelSize: 12
                                }
                                Reveal {
                                    open: telemetry.noiseKnown && telemetry.noiseMode === 2
                                    Divider {}
                                    RowLayout {
                                        Layout.fillWidth: true; spacing: 12
                                        Glyph { kind: "ambient"; tint: appColors.muted; size: 20 }
                                        Label { text: "Ambient level"; color: appColors.text; Layout.preferredWidth: 110 }
                                        HdSlider {
                                            id: ambientSlider; objectName: "ambientLevel"
                                            colors: appColors; tint: appColors.green; reduceMotion: window.calm
                                            Layout.fillWidth: true; from: 1; to: 20
                                            value: telemetry.ambient; enabled: window.controlsActive; Accessible.name: "Ambient level"
                                            onMoved: { if (!pressed) ambientCommit.restart() }
                                            onPressedChanged: { if (!pressed && window.controlsActive) { ambientCommit.stop(); device.setNoise(2, Math.round(value)) } }
                                        }
                                        Label { text: Math.round(ambientSlider.value); color: appColors.text; font.weight: Font.DemiBold; Layout.preferredWidth: 24; horizontalAlignment: Text.AlignRight }
                                    }
                                    RowLayout {
                                        Layout.fillWidth: true; spacing: 12
                                        Glyph { kind: "mic"; tint: appColors.muted; size: 20 }
                                        ColumnLayout {
                                            spacing: 0; Layout.fillWidth: true
                                            Label { text: "Voice passthrough"; color: appColors.text }
                                            Label { text: "Lets voices through while Ambient is on."; color: appColors.muted; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                                        }
                                        Switch {
                                            objectName: "voiceSwitch"
                                            checked: telemetry.voice
                                            enabled: window.controlsActive
                                            Accessible.name: "Voice passthrough"
                                            onToggled: device.setVoicePassthrough(checked)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Page {
                    anchors.fill: parent
                    index: 1; title: "Sound"
                    Section {
                        colors: appColors; caption: "Equalizer"
                        Layout.fillWidth: true
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: eqBody.implicitHeight + 32
                            radius: 4; color: appColors.card; border.color: appColors.line
                            ColumnLayout {
                                id: eqBody
                                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 16 }
                                spacing: 12
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 8
                                    Label {
                                        text: !telemetry.eqKnown ? "Current EQ unknown."
                                            : window.eqDirty ? "Not applied yet."
                                            : telemetry.controlsEnabled ? "Drag a band, then Apply."
                                            : "Turn on Allow changes on the Headphones page to edit."
                                        color: window.eqDirty ? appColors.warn : appColors.muted
                                        font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight
                                        Behavior on color { ColorAnimation { duration: window.ms(150) } }
                                    }
                                    HdButton { objectName: "resetEq"; text: "Reset"; enabled: window.eqDirty && !device.busy; onClicked: window.resetEq(); Accessible.name: "Discard EQ edits" }
                                    HdButton { objectName: "applyEq"; text: "Apply"; primary: true; enabled: window.controlsActive && window.eqDirty; onClicked: device.applyEqualizer(window.bassDraft, window.eqDraft); Accessible.name: "Apply equalizer" }
                                }
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 8
                                    ColumnLayout { // dB axis
                                        Layout.preferredWidth: 28; Layout.alignment: Qt.AlignTop; spacing: 0
                                        Item { Layout.preferredHeight: 22 }
                                        Item {
                                            Layout.preferredHeight: eqStage.height; Layout.fillWidth: true
                                            Label { anchors.right: parent.right; y: 0; text: "+10"; color: appColors.faint; font.pixelSize: 11 }
                                            Label { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; text: "0"; color: appColors.faint; font.pixelSize: 11 }
                                            Label { anchors.right: parent.right; anchors.bottom: parent.bottom; text: "−10"; color: appColors.faint; font.pixelSize: 11 }
                                        }
                                    }
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 4
                                        RowLayout { // current values
                                            Layout.fillWidth: true; spacing: 0; uniformCellSizes: true
                                            Repeater {
                                                model: 5
                                                Label {
                                                    required property int index
                                                    Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter
                                                    text: telemetry.eqKnown ? window.signed(window.eqDraft[index]) : "—"
                                                    color: appColors.text; font.weight: Font.DemiBold
                                                    ToolTip.visible: eqHover.hovered && telemetry.eqError.length > 0 && index === 0; ToolTip.text: telemetry.eqError; ToolTip.delay: 600
                                                    HoverHandler { id: eqHover }
                                                }
                                            }
                                        }
                                        Item { // the curve runs through the thumbs
                                            id: eqStage
                                            Layout.fillWidth: true; Layout.preferredHeight: 200
                                            property var ys: [100, 100, 100, 100, 100]
                                            function setY(i, y) { if (Math.abs(ys[i] - y) < 0.05) return; const copy = ys.slice(); copy[i] = y; ys = copy }
                                            readonly property string curve: {
                                                const w = width, h = height, xs = []
                                                for (let i = 0; i < 5; ++i) xs.push((i + 0.5) * w / 5)
                                                const y = ys.map(v => Math.max(0, Math.min(h, v)))
                                                let d = "M0 " + y[0] + " L" + xs[0] + " " + y[0]
                                                for (let i = 0; i < 4; ++i) {
                                                    const a = Math.max(0, i - 1), b = i, c = i + 1, e = Math.min(4, i + 2)
                                                    d += " C" + (xs[b] + (xs[c] - xs[a]) / 6) + " " + (y[b] + (y[c] - y[a]) / 6) + " "
                                                       + (xs[c] - (xs[e] - xs[b]) / 6) + " " + (y[c] - (y[e] - y[b]) / 6) + " " + xs[c] + " " + y[c]
                                                }
                                                return d + " L" + w + " " + y[4]
                                            }
                                            Rectangle { // 0 dB line
                                                width: parent.width; height: 1; y: parent.height / 2
                                                color: appColors.divider
                                            }
                                            Shape {
                                                anchors.fill: parent
                                                visible: telemetry.eqKnown
                                                preferredRendererType: Shape.CurveRenderer
                                                ShapePath { // soft fill between the curve and 0 dB
                                                    strokeWidth: 0
                                                    fillColor: Qt.alpha(appColors.accent, 0.16)
                                                    PathSvg { path: eqStage.curve + " L" + eqStage.width + " " + (eqStage.height / 2) + " L0 " + (eqStage.height / 2) + " Z" }
                                                }
                                                ShapePath {
                                                    strokeColor: appColors.accent; strokeWidth: 2; fillColor: "transparent"
                                                    capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                                                    PathSvg { path: eqStage.curve }
                                                }
                                            }
                                            RowLayout {
                                                anchors.fill: parent; spacing: 0; uniformCellSizes: true
                                                Repeater {
                                                    model: 5
                                                    HdSlider {
                                                        id: band
                                                        required property int index
                                                        objectName: "eqBand" + index
                                                        Layout.fillWidth: true; Layout.fillHeight: true
                                                        colors: appColors; centered: true; known: telemetry.eqKnown; reduceMotion: window.calm
                                                        orientation: Qt.Vertical; from: -10; to: 10
                                                        value: window.eqDraft[index]
                                                        onMoved: window.editBand(index, value)
                                                        enabled: window.controlsActive
                                                        Accessible.name: ["400", "1k", "2.5k", "6.3k", "16k"][index] + " Hz"
                                                        onThumbYChanged: eqStage.setY(index, thumbY)
                                                        Component.onCompleted: eqStage.setY(index, thumbY)
                                                    }
                                                }
                                            }
                                        }
                                        RowLayout {
                                            Layout.fillWidth: true; spacing: 0; uniformCellSizes: true
                                            Repeater {
                                                model: ["400 Hz", "1 kHz", "2.5 kHz", "6.3 kHz", "16 kHz"]
                                                Label {
                                                    required property string modelData
                                                    Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter
                                                    text: modelData; color: appColors.muted; font.pixelSize: 12
                                                }
                                            }
                                        }
                                    }
                                }
                                Divider {}
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 12
                                    Label { text: "Clear Bass"; color: appColors.text; Layout.preferredWidth: 110 }
                                    HdSlider {
                                        objectName: "clearBass"
                                        colors: appColors; centered: true; known: telemetry.eqKnown; reduceMotion: window.calm
                                        Layout.fillWidth: true; from: -10; to: 10
                                        value: window.bassDraft; onMoved: window.bassDraft = Math.round(value)
                                        enabled: window.controlsActive; Accessible.name: "Clear Bass"
                                    }
                                    Label { text: telemetry.eqKnown ? window.signed(window.bassDraft) : "—"; color: appColors.text; font.weight: Font.DemiBold; Layout.preferredWidth: 28; horizontalAlignment: Text.AlignRight }
                                }
                            }
                        }
                    }
                }

                Page {
                    anchors.fill: parent
                    index: 2; title: "Features"
                    SettingsCard {
                        visible: !telemetry.controlsEnabled
                        colors: appColors; icon: "lock"
                        title: "Allow changes is off"
                        description: telemetry.canEnableControls ? "Turn it on from the Headphones page to read and change these features."
                            : "Connect your headphones to use these features."
                        HdButton { text: "Open Headphones"; onClicked: window.page = 0 }
                    }
                    Section {
                        colors: appColors; caption: "Calls and sound quality"
                        Layout.fillWidth: true
                        SettingsCard {
                            visible: telemetry.isXm5
                            colors: appColors; icon: "chat"; dim: !telemetry.speakKnown
                            title: "Speak-to-Chat"; badge: "Experimental"
                            description: !telemetry.controlsEnabled ? "Pauses music and lets voices in when you start talking."
                                : telemetry.speakKnown ? "Pauses music and lets voices in when you start talking."
                                : "The headphones didn't report this setting."
                            Switch {
                                objectName: "speakSwitch"
                                checked: telemetry.speak
                                enabled: window.controlsActive && telemetry.speakKnown
                                Accessible.name: "Speak-to-Chat"
                                onToggled: device.setSpeakToChat(checked)
                            }
                        }
                        SettingsCard {
                            colors: appColors; icon: "wave"; dim: !telemetry.dseeKnown
                            title: telemetry.isXm5 ? "DSEE Extreme" : "DSEE"; badge: "Experimental"
                            description: !telemetry.controlsEnabled || telemetry.dseeKnown
                                ? "Restores detail that compressed music loses."
                                : "The headphones didn't report this setting."
                            Switch {
                                objectName: "dseeSwitch"
                                checked: telemetry.dsee
                                enabled: window.controlsActive && telemetry.dseeKnown
                                Accessible.name: telemetry.isXm5 ? "DSEE Extreme" : "DSEE"
                                onToggled: device.setDsee(checked)
                            }
                        }
                    }
                    Label {
                        text: "Each change is sent once and confirmed by reading it back from the headphones. Nothing is retried."
                        color: appColors.muted; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap
                    }
                }

                Page {
                    anchors.fill: parent
                    index: 3; title: "Device"
                    Section {
                        colors: appColors; caption: "This headset"
                        Layout.fillWidth: true
                        SettingsCard { colors: appColors; title: "Model"; description: "From Windows Bluetooth pairing, not asked of the headphones."
                            Label { text: telemetry.model; color: appColors.muted } }
                        SettingsCard { colors: appColors; title: "Firmware"
                            Label { text: telemetry.firmware; color: appColors.muted
                                ToolTip.visible: fwHover.hovered && telemetry.firmwareError.length > 0 && text === "—"; ToolTip.text: telemetry.firmwareError; ToolTip.delay: 600
                                HoverHandler { id: fwHover } } }
                        SettingsCard { colors: appColors; title: "Reported codec"; description: "Reported by the headphones; not measured from Windows audio."
                            Label { text: telemetry.codec; color: appColors.muted } }
                        SettingsCard { colors: appColors; title: "Last full refresh"
                            Label { text: telemetry.updated; color: appColors.muted } }
                        SettingsCard { colors: appColors; title: "Last change"; description: "What the headphones reported after your most recent change."
                            Label { text: telemetry.lastChange && telemetry.lastChange.length > 0 ? telemetry.lastChange : "—"
                                color: appColors.muted; wrapMode: Text.WordWrap; Layout.maximumWidth: 260; horizontalAlignment: Text.AlignRight } }
                    }
                    Section {
                        colors: appColors; caption: "About"
                        Layout.fillWidth: true
                        SettingsCard { colors: appColors; icon: "headphones"; title: "Headset Desk"
                            description: "An independent project. Not affiliated with Sony."
                            Label { text: Qt.application.version; color: appColors.muted } }
                    }
                }
            }
        }

        // Navigation pane (icon rail, expanded, or overlay in the narrowest layout).
        Rectangle {
            id: pane
            visible: window.navMode > 0 || window.paneOpen
            anchors { left: parent.left; top: parent.top; bottom: parent.bottom; topMargin: window.navMode === 0 ? 48 : 0 }
            width: window.navMode === 0 ? 232 : window.paneWidth
            color: window.navMode === 0 ? appColors.card : "transparent"
            border.color: window.navMode === 0 ? appColors.line : "transparent"
            Behavior on width { NumberAnimation { duration: window.ms(180); easing.type: Easing.OutCubic } }
            z: 2
            ColumnLayout {
                anchors { fill: parent; topMargin: 8; bottomMargin: 8 }
                spacing: 0
                RowLayout {
                    visible: window.paneExpanded && window.navMode === 2
                    Layout.leftMargin: 16; Layout.bottomMargin: 12; Layout.topMargin: 4; spacing: 12
                    Label { text: "Headset Desk"; color: appColors.text; font.weight: Font.DemiBold; font.pixelSize: 14 }
                }
                Item { visible: !(window.paneExpanded && window.navMode === 2); Layout.preferredHeight: 4 }
                NavItem { text: "Headphones"; glyph: "headphones"; pageIndex: 0; objectName: "navHeadphones" }
                NavItem { text: "Sound"; glyph: "sound"; pageIndex: 1; objectName: "navSound" }
                NavItem { text: "Features"; glyph: "features"; pageIndex: 2; objectName: "navFeatures" }
                Item { Layout.fillHeight: true }
                NavItem { text: "Device"; glyph: "device"; pageIndex: 3; objectName: "navDevice" }
            }
        }
        // Title strip with the menu button when the pane is hidden.
        Rectangle {
            visible: window.navMode === 0
            anchors { left: parent.left; right: parent.right; top: parent.top }
            height: 48; color: "transparent"; z: 3
            RowLayout {
                anchors { fill: parent; leftMargin: 4; rightMargin: 16 }
                spacing: 8
                Button {
                    id: menuButton
                    objectName: "menuButton"
                    implicitWidth: 40; implicitHeight: 40
                    hoverEnabled: true
                    Accessible.name: window.paneOpen ? "Close navigation" : "Open navigation"
                    onClicked: window.paneOpen = !window.paneOpen
                    background: Rectangle { radius: 4; color: menuButton.down ? appColors.fillPressed : menuButton.hovered ? appColors.fill : "transparent"; FocusRing { target: menuButton } }
                    contentItem: Item { Glyph { anchors.centerIn: parent; kind: "menu"; size: 18; tint: appColors.text } }
                }
                Label { text: window.pageNames[window.page]; color: appColors.text; font.weight: Font.DemiBold }
                Item { Layout.fillWidth: true }
            }
        }
        MouseArea { // scrim behind the overlay pane
            anchors.fill: parent; z: 1
            visible: window.navMode === 0 && window.paneOpen
            onClicked: window.paneOpen = false
        }
    }
}
