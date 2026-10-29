/*****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

import QtQuick
import QtQuick.Templates as T


import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Style
import VLC.Player
import VLC.Playlist
import VLC.Dialogs
import VLC.Util

T.Control {
    id: root

    // Properties

    property bool paintOnly: false

    readonly property ColorContext colorContext: ColorContext {
        id: theme

        colorSet: ColorContext.ToolButton

        enabled: root.enabled || root.paintOnly
        focused: root.activeFocus
        hovered: root.cursorInside
        pressed: mouseArea.containsPress
    }

    property bool _keyOkPressed: false

    // Playing: the pause variant of the design (Figma "play" type=pause)
    readonly property bool _redesignPlaying: !paintOnly
                                             && Player.playingState !== Player.PLAYING_STATE_PAUSED
                                             && Player.playingState !== Player.PLAYING_STATE_STOPPED

    // Aliases

    property alias cursorInside: mouseArea.cursorInside

    // Settings

    implicitWidth: implicitBackgroundWidth + leftInset + rightInset
    implicitHeight: implicitBackgroundHeight + topInset + bottomInset

    scale: (_keyOkPressed || (mouseArea.pressed && cursorInside)) ? 0.95
                                                                  : 1.00

    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Play/Pause")
    AccessibleCompat.id: "playButton"
    Accessible.checkable: true
    Accessible.checked: Player.playingState !== Player.PLAYING_STATE_PAUSED
                        && Player.playingState !== Player.PLAYING_STATE_STOPPED
    Accessible.onPressAction: root._activate()
    Accessible.onToggleAction: root._activate()

    // Tooltip

    T.ToolTip.visible: (hovered || visualFocus)
    T.ToolTip.delay: VLCStyle.delayToolTipAppear
    T.ToolTip.text: root.Accessible.name

    // Keys

    Keys.onPressed: (event) => {
        if (KeyHelper.matchOk(event) ) {
            if (!event.isAutoRepeat) {
                _keyOkPressed = true
                keyHoldTimer.restart()
                innerRectangle.state = "diminished"
            }
            event.accepted = true
        }
        Navigation.defaultKeyAction(event)
    }

    Keys.onReleased: (event) => {
        if (KeyHelper.matchOk(event)) {
            if (!event.isAutoRepeat) {
                _keyOkPressed = false
                keyHoldTimer.stop()
                innerRectangle.state = ""
                if (Player.playingState !== Player.PLAYING_STATE_STOPPED)
                    MainPlaylistController.togglePlayPause()
            }
            event.accepted = true
        }
    }

    // Functions

    // Nothing plays and the queue is empty (the bar of the library kept
    // shown): the files to play are asked, as the Open button does
    function _activate() {
        if (!Player.isStarted && MainPlaylistController.empty)
            DialogsProvider.simpleOpenDialog()
        else
            MainPlaylistController.togglePlayPause()
    }

    function _pressAndHoldAction() {
        innerRectangle.state = ""
        _keyOkPressed = false
        MainPlaylistController.stop()
    }

    // Children

    Timer {
        id: keyHoldTimer

        interval: mouseArea.pressAndHoldInterval
        repeat: false

        Component.onCompleted: {
            triggered.connect(_pressAndHoldAction)
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent

        hoverEnabled: true

        readonly property bool cursorInside: containsMouse

        onCursorInsideChanged: {
            if (pressed && !cursorInside) {
                // Press and hold action can no longer be done,
                // so reset the state in order to reset the
                // animation:
                innerRectangle.state = ""
            }
        }

        onPressed: (mouse) => {
            if (!cursorInside) {
                mouse.accepted = false
                return
            }

            root.forceActiveFocus(Qt.MouseFocusReason)
            innerRectangle.state = "diminished"
        }

        onReleased: {
            innerRectangle.state = ""
        }

        onClicked: (mouse) => {
            root._activate()
            mouse.accepted = true
        }

        onPressAndHold: (mouse) => {
            if (!cursorInside) {
                mouse.accepted = false
                return
            }

            _pressAndHoldAction()
            mouse.accepted = true
        }
    }

    contentItem: T.Label {
        text: {
            const state = Player.playingState

            // drawn by the images of the design
            return ""
        }

        color: VLCStyle.redesign.color(VLCStyle.redesign.windowBackground, "#c2c2c1", theme.palette?.isDark ?? VLCStyle.darkPalette.isDark)

        font.pixelSize: VLCStyle.redesign.iconSize

        font.family: VLCIcons.fontFamily

        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        Accessible.ignored: true

        // Figma "play": the triangle in the 22 px between the paddings of
        // 17 and 15 px, the pause bars in the middle
        Image {
            // drawn at the size shown (an SVG: sharp at any scale and zoom)
            sourceSize: Qt.size(width, height)
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: visible ? VLCStyle.dp(1, VLCStyle.scale) : 0
            source: "qrc:/redesign/play.svg"
            width: VLCStyle.redesign.iconSize
            height: VLCStyle.redesign.iconSize
            visible: !root._redesignPlaying
        }

        Image {
            anchors.centerIn: parent
            source: "qrc:///redesign/player/pause.svg"
            width: VLCStyle.redesign.iconSize
            height: VLCStyle.redesign.iconSize
            sourceSize: Qt.size(width, height)
            visible: root._redesignPlaying
        }

        DelayedBehavior on color {
            delayedEnabled: theme.initialized

            ColorAnimation {
                duration: VLCStyle.duration_veryShort
                easing.type: Easing.InOutSine
            }
        }
    }

    background: Item {
        implicitWidth: VLCStyle.redesign.playWidth
        implicitHeight: VLCStyle.redesign.playHeight

        Widgets.AnimatedBackground {
            anchors.fill: parent
            radius: VLCStyle.redesign.controlRadius
            border.pixelAligned: false
            enabled: theme.initialized
            border.color: root.visualFocus ? theme.visualFocus : Qt.alpha(theme.visualFocus, 0.0)
        }

        Rectangle {
            id: innerRectangle
            anchors.fill: parent
            // Figma "Player - customize": the pill 48 wide in its frame of 54
            anchors.leftMargin: VLCStyle.customizing ? VLCStyle.dp(3, VLCStyle.scale) : 0
            anchors.rightMargin: anchors.leftMargin
            radius: VLCStyle.redesign.controlRadius
            // Figma "play" type=pause: a grey pill while playing
            color: root._redesignPlaying
                   ? VLCStyle.redesign.color("#6B6B6B", "#6b6b6b", theme.palette?.isDark ?? VLCStyle.darkPalette.isDark)
                   : VLCStyle.redesign.color(VLCStyle.redesign.accent, theme.accent, theme.palette?.isDark ?? VLCStyle.darkPalette.isDark)
            opacity: state === "diminished" ? 0.75 : 1
        }
    }
}
