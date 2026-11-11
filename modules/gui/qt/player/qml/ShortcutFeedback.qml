/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 *****************************************************************************/
import QtQuick

import VLC.MainInterface
import VLC.Style
import VLC.Util
import VLC.Player

// What a shortcut did in full screen while the controls stay hidden ("When
// you use a shortcut in full screen" of the settings, issue #72; VLC 3 drew
// a message on the video instead of its controller): a message at the top
// centre, where the player's messages show, never over the subtitles, and
// as large as a TV's (it grows with the screen: fontSize below):
// what the key did and the time ("+30 s  00:02:50 / 00:25:40"); with "Show
// only the progress" a thin line of the progress on the bottom edge too.
// Gone 1.5 s after the last key; the jumps of keys in a row add up (MainCtx).
// The volume keys show the window's volume meter (VolumeMeter.qml), the one
// display of the volume. Never takes the mouse or the focus.
Item {
    id: root

    // the controls show (the mouse moved, Tab): the line is under them
    property bool controlsVisible: false
    // the bottom of what is drawn at the top (the top bar while it shows):
    // the message goes under it
    property real messageTop: 0

    // Read from a sofa, as the TV app's: the message grows with the screen,
    // its text 2.2 % of the height of the video (24 px at 1080p), never under
    // 18 px; the card keeps the proportions of the player's messages
    // (PlayerToast.qml: 14 px text in a card of 40, radius 10, 16 on each
    // side, 24 under the top)
    readonly property real fontSize: Math.max(18, Math.round(height * 0.022))
    readonly property real _unit: fontSize / 14
    // the line: 4 px, a little thicker on a large screen
    readonly property real lineHeight: Math.max(4, Math.round(fontSize / 6))

    // MainCtx.fsShortcutFeedback 1 (the progress) or 2 (the message only),
    // in full screen with a video: PlayerView sets it
    readonly property bool active: MainCtx.shortcutFeedbackActive
    readonly property bool showsProgress: MainCtx.fsShortcutFeedback === 1

    // A key the player saw went somewhere else than the hotkeys after all
    // (an open panel, a focused control, Live TV's list): the controls show,
    // as before
    signal untouched()

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }

    // A key pressed in the window: true when its feedback stands for the
    // controls (the key goes on to the hotkeys as before)
    function takesKey(event) {
        if (!active)
            return false
        switch (event.key) {
        // a modifier alone starts a shortcut (the Ctrl of Ctrl+Up)
        case Qt.Key_Shift:
        case Qt.Key_Control:
        case Qt.Key_Alt:
        case Qt.Key_AltGr:
        case Qt.Key_Meta:
        case Qt.Key_Super_L:
        case Qt.Key_Super_R:
        case Qt.Key_CapsLock:
        case Qt.Key_NumLock:
            return true
        }
        // keys of no action of VLC (Tab), and those of the controls (the I
        // key, the menu key, Enter, Esc): the controls, as before
        const kind = MainCtx.shortcutKind(event.key, event.modifiers)
        if (kind === MainCtx.SHORTCUT_NONE || kind === MainCtx.SHORTCUT_CONTROLS)
            return false
        // where the key goes is known once it is delivered: the hotkeys
        // count it (MainCtx.hotkeySerial), something that took it before
        // them does not
        const serial = MainCtx.hotkeySerial()
        Qt.callLater(() => {
            if (MainCtx.hotkeySerial() === serial)
                root.untouched()
        })
        return true
    }

    // A message of the player for a key while active (the subtitles' size):
    // here, the one card at the top
    function say(text) {
        _present(MainCtx.SHORTCUT_MESSAGE, "", text, false, false)
    }

    property int _kind: MainCtx.SHORTCUT_NONE
    property string _action: ""
    property string _label: ""
    // the name of the action, if the core says nothing for it
    property string _fallback: ""
    property bool _withTime: false
    // the words come a moment later (the core's message, the new speed)
    property bool _waiting: false
    property bool _shown: false

    function _present(kind, action, label, withTime, wait) {
        // the same key again keeps its words until the new ones come
        const same = _shown && _kind === kind && _action === action
        _kind = kind
        _action = action
        _withTime = withTime
        if (!wait) {
            _label = label
            _waiting = false
        } else if (!same) {
            _label = ""
            _waiting = true
            graceTimer.restart()
        }
        _shown = true
        hideTimer.restart()
    }

    readonly property string _text: _kind === MainCtx.SHORTCUT_RATE
                                    ? (_waiting ? "" : MainCtx.shortcutRateText(Player.rate))
                                    : _label

    // the time as the seek bar writes it (the hours always: 00:02:50), from
    // left to right in every language: a left-to-right isolate
    readonly property string _time: {
        if (!_shown || !_withTime || !Player.isStarted)
            return ""
        const length = Player.length
        const flags = VLCTick.ForceHours | (length.isSubSecond() ? VLCTick.SubSecondFormattedAsMS : 0)
        const now = Player.time.formatHMS(flags)
        const text = length.valid() && length.toSeconds() > 0 ? now + " / " + length.formatHMS(flags) : now
        return "⁦" + text + "⁩"
    }

    onActiveChanged: {
        if (!active) {
            _shown = false
            hideTimer.stop()
        }
    }

    Connections {
        target: MainCtx

        function onShortcutFeedback(kind, action, text) {
            switch (kind) {
            case MainCtx.SHORTCUT_SEEK:
                // the jumps in a row ("+30 s"; nothing when it can't seek)
                root._present(kind, action, text, true, false)
                break
            case MainCtx.SHORTCUT_POSITION:
                root._present(kind, action, "", true, false)
                break
            case MainCtx.SHORTCUT_PLAYPAUSE: {
                // what it does (the player's state changes a moment later)
                const paused = action === "pause"
                               || (action === "play-pause" && Player.playingState !== Player.PLAYING_STATE_PAUSED)
                root._present(kind, action, paused ? qsTr("Paused") : qsTr("Playing"), true, false)
                break
            }
            case MainCtx.SHORTCUT_FRAME:
                root._present(kind, action, text, true, false)
                break
            case MainCtx.SHORTCUT_RATE:
                root._present(kind, action, "", false, true)
                break
            case MainCtx.SHORTCUT_MESSAGE:
                root._fallback = text
                root._present(kind, action, "", false, true)
                break
            }
        }

        // what the core said instead of drawing it on the video ("Subtitle
        // track: English"): the words of a message (the kinds with their own
        // words keep them), or a message of its own
        function onShortcutMessage(text) {
            if (!root.active)
                return
            if (root._shown && root._kind !== MainCtx.SHORTCUT_MESSAGE)
                return
            root._present(MainCtx.SHORTCUT_MESSAGE, root._action, text, false, false)
        }
    }

    Connections {
        target: Player
        enabled: root._kind === MainCtx.SHORTCUT_RATE && root._waiting
        function onRateChanged() { root._waiting = false }
    }

    Timer {
        id: graceTimer
        // the core's message comes in a few ms (from its thread)
        interval: 200
        onTriggered: {
            if (!root._waiting)
                return
            if (root._kind === MainCtx.SHORTCUT_MESSAGE)
                root._label = root._fallback
            root._waiting = false
        }
    }

    Timer {
        id: hideTimer
        // shortcutfeedback::feedbackMs
        interval: 1500
        onTriggered: root._shown = false
    }

    // The progress on the bottom edge: the played part in the accent colour
    // over a soft shade, from left to right in every language as the seek
    // bar (SliderBar.qml)
    Item {
        id: progressLine

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Math.round(root.fontSize * 1.5)

        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true

        visible: opacity > 0
        opacity: root._shown && root.showsProgress && !root.controlsVisible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: VLCStyle.duration_short } }

        // the line stays readable over a bright picture
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0; color: "transparent" }
                GradientStop { position: 1; color: Qt.alpha(VLCStyle.redesign.windowBackground, 0.55) }
            }
        }

        Rectangle {
            id: track

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: root.lineHeight
            color: Qt.alpha(VLCStyle.redesign.textPrimary, 0.25)

            Rectangle {
                height: parent.height
                width: progressLine.visible ? parent.width * Math.max(0, Math.min(1, Player.position)) : 0
                color: VLCStyle.redesign.accent
            }
        }

        Accessible.ignored: true
    }

    // The message: the card of the player's messages (PlayerToast.qml),
    // over the video: dark in both themes
    Rectangle {
        id: pill

        readonly property bool wanted: root._shown && (root._text.length > 0 || root._time.length > 0)

        anchors.horizontalCenter: parent.horizontalCenter
        y: Math.round(root.messageTop + 24 * root._unit)
        width: Math.min(row.implicitWidth + Math.round(32 * root._unit), root.width - root.px(40))
        height: Math.round(40 * root._unit)
        radius: Math.round(10 * root._unit)
        color: Qt.alpha(VLCStyle.redesign.controlBackground, 0.95)

        visible: opacity > 0
        opacity: wanted ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: VLCStyle.duration_short } }

        // read out as the player's other messages
        Accessible.role: Accessible.AlertMessage
        Accessible.name: [root._text, root._time].filter(part => part.length > 0).join(", ")

        // the words follow the language (on the right in a right-to-left
        // one), the time after them
        Row {
            id: row

            anchors.centerIn: parent
            spacing: Math.round(12 * root._unit)

            Text {
                id: label

                visible: text.length > 0
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, root.width - root.px(40) - Math.round(32 * root._unit)
                                - (timeText.visible ? timeText.implicitWidth + row.spacing : 0))
                text: root._text
                textFormat: Text.PlainText
                elide: Text.ElideRight
                color: VLCStyle.redesign.textPrimary
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.fontSize
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
            }

            Text {
                id: timeText

                visible: text.length > 0
                anchors.verticalCenter: parent.verticalCenter
                text: root._time
                textFormat: Text.PlainText
                color: VLCStyle.redesign.textSecondary
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.fontSize
                font.features: { "tnum": 1 }
            }
        }
    }
}
