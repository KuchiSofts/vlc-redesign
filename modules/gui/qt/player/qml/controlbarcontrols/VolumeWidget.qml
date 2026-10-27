/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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
import QtQuick.Controls.impl as Impl
import QtQuick.Layouts
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Util
import VLC.Style
import VLC.Player

T.Pane {
    id: root

    // The volume grows from left to right in right to left languages too
    // (Hebrew, Arabic...), as the time of the seek bar (see ControlBar)
    LayoutMirroring.enabled: false
    LayoutMirroring.childrenInherit: true

    property bool paintOnly: false

    readonly property color sliderColor: {
        const theme = volControl.colorContext
        return (volControl.position > _fullvolpos) ? theme.fg.negative : theme.fg.primary
    }

    readonly property var _player: paintOnly ? ({ muted: false, volume: .5 }) : Player
    readonly property int _maxvol: MainCtx.maxVolume
    readonly property real _fullvolpos: designPosition(1)
    readonly property real _maxvolpos: _maxvol / 100

    // Figma "volume": the fill (px of 86) drawn for each state of the
    // design, 0 to 200 % (its maximum volume); a maximum other than 200 %
    // stretches the states over its range
    readonly property var _designFill: [[0, 0], [10, 5], [20, 12], [30, 18], [40, 24], [50, 31], [60, 39],
                                        [70, 46], [80, 51], [90, 56], [100, 61], [150, 73], [200, 85]]
    readonly property real _designWidth: 86

    // the drawn position (0 to 1) of a volume (1 = 100 %)
    function designPosition(volume) {
        const d = volume * 100 * 200 / _maxvol
        const t = _designFill
        if (d <= 0)
            return 0
        for (let i = 1; i < t.length; ++i) {
            if (d <= t[i][0]) {
                const a = t[i - 1], b = t[i]
                return (a[1] + (b[1] - a[1]) * (d - a[0]) / (b[0] - a[0])) / _designWidth
            }
        }
        return 1
    }

    // the volume (1 = 100 %) of a drawn position
    function designVolume(position) {
        const px = position * _designWidth
        const t = _designFill
        if (px <= 0)
            return 0
        let d = 200
        for (let i = 1; i < t.length; ++i) {
            if (px <= t[i][1]) {
                const a = t[i - 1], b = t[i]
                d = a[0] + (b[0] - a[0]) * (px - a[1]) / (b[1] - a[1])
                break
            }
        }
        return d / 100 * _maxvol / 200
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    contentWidth: volumeWidget.implicitWidth
    contentHeight: volumeWidget.implicitHeight

    Keys.priority: Keys.AfterItem
    Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

    RowLayout {
        id: volumeWidget

        anchors.fill: parent
        spacing: VLCStyle.redesign.controlInnerSpacing

        Widgets.IconToolButton{
            id: volumeBtn

            focus: true
            paintOnly: root.paintOnly
            // Figma "Player": 50 % shows the icon value=high; the glyph (the
            // editor's preview) and the image of the bar show the same level
            readonly property string level: {
                if (root._player.muted || root._player.volume === 0)
                    return "muted"
                if (root._player.volume < .25)
                    return "low"
                if (root._player.volume < .5)
                    return "medium"
                return "high"
            }
            text: level === "muted" ? VLCIcons.volume_muted
                  : level === "low" ? VLCIcons.volume_low
                  : level === "medium" ? VLCIcons.volume_medium
                  : VLCIcons.volume_high
            font.pixelSize: VLCStyle.redesign.iconSize
            contentItem: Item {
                implicitWidth: VLCStyle.redesign.iconSize
                implicitHeight: VLCStyle.redesign.iconSize
                Impl.IconImage {
                    anchors.centerIn: parent
                    width: VLCStyle.redesign.iconSize
                    height: width
                    color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, colorContext.fg.primary)
                    source: "qrc:///redesign/volume-" + volumeBtn.level + ".png"
                }
            }
            description: qsTr("Mute")

            AccessibleCompat.id: "muteButton"

            onClicked: Player.muted = !Player.muted

            Accessible.onIncreaseAction: {
                Player.muted = false
                Player.setVolumeUp()
            }

            Accessible.onDecreaseAction: {
                Player.muted = false
                Player.setVolumeDown()
            }

            Navigation.parentItem: root
            Navigation.rightItem: volControl
        }

        Widgets.SliderExt {
            id: volControl

            // FIXME: use VLCStyle
            implicitWidth: VLCStyle.redesign.volumeWidth

            // WARNING: Directly setting the insets break handle and background positioning.
            Binding on topInset {
                when: MainCtx.usingTouch
                value: VLCStyle.touchHandlerMargin
            }
            Binding on bottomInset {
                when: MainCtx.usingTouch
                value: VLCStyle.touchHandlerMargin
            }

            Layout.fillHeight: true
            Layout.margins: 0

            property bool _inhibitPlayerVolumeUpdate: false

            // FIXME: Currently we are not updating the ShiftModifier status while dragging. This
            //        could be fixed with a custom Slider based on a MouseArea.
            property bool _shiftPressed: false

            property real _clamp: 0.01

            property bool _keyPressed: false

            property bool filterEvents: false

            color: root.sliderColor

            handleWidth: 0
            background: Item {
                implicitWidth: VLCStyle.redesign.volumeWidth
                implicitHeight: VLCStyle.redesign.volumeHeight
                width: volControl.availableWidth
                height: volControl.availableHeight

                Image {
                    anchors.centerIn: parent
                    // Drawn for the dark theme (#090909); Figma "Player -
                    // Light": #888888
                    source: VLCStyle.palette.isDark
                            ? "qrc:/redesign/volume-background.svg"
                            : SVGColorImage.colorize("qrc:/redesign/volume-background.svg")
                                           .any({ "#090909": "#888888" }).uri()
                    width: VLCStyle.dp(89, VLCStyle.scale)
                    height: VLCStyle.dp(27.3633, VLCStyle.scale)
                }

                Item {
                    x: VLCStyle.dp(1.5, VLCStyle.scale)
                    anchors.verticalCenter: parent.verticalCenter
                    width: VLCStyle.dp(86, VLCStyle.scale) * volControl.position
                    height: VLCStyle.dp(24.56, VLCStyle.scale)
                    clip: true

                    Image {
                        // the gradient under black at 20 %; Figma "Player -
                        // Light": under white at 20 %
                        source: VLCStyle.palette.isDark
                                ? "qrc:/redesign/volume-fill.svg"
                                : SVGColorImage.colorize("qrc:/redesign/volume-fill.svg")
                                               .any({ "black": "#ffffff" }).uri()
                        width: VLCStyle.dp(86, VLCStyle.scale)
                        height: VLCStyle.dp(24.56, VLCStyle.scale)
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    radius: VLCStyle.redesign.controlRadius
                    border.width: volControl.visualFocus ? VLCStyle.dp(1, VLCStyle.scale) : 0
                    border.color: volControl.colorContext.visualFocus
                }
            }

            // the drawn position of the design (see designPosition)
            from: 0
            to: 1
            opacity: _player.muted ? 0.5 : 1

            enabled: !root.paintOnly // disables event handling depending on this

            toolTipTextProvider: function (value) {
                // the real value i.e 'Player.volume' bounds can be different
                // from bounds of this widget and we want to show the current
                // volume here, so directly use Player.volume instead of "value"

                return Math.round(Player.volume * 100) + "%"
            }

            Accessible.name: qsTr("Volume")
            AccessibleCompat.id: "volumeSlider"

            Keys.onPressed: (event) => {
                if (KeyHelper.matchOk(event)) {
                    event.accepted = true

                    _keyPressed = true
                } else {
                    Navigation.defaultKeyAction(event)
                }
            }

            Keys.onReleased: (event) => {
                if (_keyPressed === false)
                    return

                _keyPressed = false

                if (KeyHelper.matchOk(event)) {
                    Player.muted = !Player.muted
                }
            }

            function _syncVolumeWithPlayer() {
                volControl._inhibitPlayerVolumeUpdate = true
                volControl.value = root.designPosition(_player.volume)
                volControl._inhibitPlayerVolumeUpdate = false
            }

            function _adjustPlayerVolume() {
                Player.muted = false

                let value = volControl.value

                // NOTE: We are clamping the value to make it easier to restore the default volume.
                if (_shiftPressed === false) {
                    if (Math.abs(value - root._fullvolpos) < _clamp)
                        value = root._fullvolpos
                    else
                        _clamp = 0.01
                }

                Player.volume = root.designVolume(value)

                volControl.value = value
            }

            Component.onCompleted: {
                root.paintOnlyChanged.connect(_syncVolumeWithPlayer)
                volControl._syncVolumeWithPlayer()
            }

            Connections {
                target: Player
                enabled: !paintOnly

                function onVolumeChanged() { volControl._syncVolumeWithPlayer() }
            }

            // the value of the design (Figma "audio value", 2245:9805) in
            // place of the pointing tooltip
            Widgets.PointingToolTipAttached.visible: false

            // a change by the wheel or the keys shows the value a moment
            Timer {
                id: valueTimer
                interval: 1200
            }
            Connections {
                target: Player
                enabled: !root.paintOnly && (volControl.hovered || volControl.activeFocus || volControl.pressed)
                function onVolumeChanged() { valueTimer.restart() }
            }

            // Figma "audio value" (2245:9805): 121 x 37, #0F0F0F, "100 %"
            // (14 px, #969696) centred; 8 above the slider, centred on it
            T.Popup {
                id: valueBox
                parent: volControl
                x: Math.round((volControl.width - width) / 2)
                y: -height - VLCStyle.dp(8, VLCStyle.scale)
                width: VLCStyle.dp(121, VLCStyle.scale)
                height: VLCStyle.dp(37, VLCStyle.scale)
                padding: 0
                closePolicy: T.Popup.NoAutoClose
                visible: !root.paintOnly && root.visible
                         && (volControl.hovered || volControl.pressed || volControl.visualFocus || valueTimer.running)
                background: Rectangle {
                    color: VLCStyle.redesign.color("#0F0F0F", "#dedede")
                    radius: VLCStyle.dp(6, VLCStyle.scale)
                }
                contentItem: Text {
                    text: qsTr("%1 %").arg(Math.round(Player.volume * 100))
                    color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Navigation.leftItem: volumeBtn
            Navigation.parentItem: root

            Keys.onUpPressed: (event) => {
                Player.muted = false
                Player.setVolumeUp()
            }

            Keys.onDownPressed: (event) => {
                Player.muted = false
                Player.setVolumeDown()
            }

            Keys.priority: Keys.BeforeItem

            onValueChanged: {
                if (paintOnly)
                    return

                if (!volControl._inhibitPlayerVolumeUpdate) {
                    if (filterEvents) {
                        Qt.callLater(volControl._adjustPlayerVolume)
                    } else {
                        volControl._adjustPlayerVolume()
                        filterEvents = true
                    }
                }
            }

            Rectangle{
                id: tickmark

                parent: volControl.background

                x : parent.width * root._fullvolpos
                width: VLCStyle.dp(1, VLCStyle.scale)
                height: parent.height
                radius: VLCStyle.dp(2, VLCStyle.scale)

                // NOTE: This shouldn't be visible when the volume stops before a 100.
                visible: (root._maxvol > 100)

                // The volume of the design has no mark at 100 %: only the
                // help to snap to it stays
                color: "transparent"

                // NOTE: This is a helper to select the default volume when clicking on the
                //       tickmark. We apply a higher clamp value to achieve that behavior on
                //       the Slider.
                MouseArea {
                    anchors.fill: parent

                    anchors.margins: -(VLCStyle.dp(4, VLCStyle.scale))

                    onPressed: (mouse) => {
                        mouse.accepted = false

                        if (mouse.modifiers === Qt.ShiftModifier)
                            return

                        volControl._clamp = 0.1
                    }
                }
            }

            MouseArea {
                id: sliderMouseArea

                anchors.fill: parent

                acceptedButtons: (Qt.LeftButton | Qt.RightButton)

                onPressed: (mouse) => {
                    if (root.paintOnly) {
                        mouse.accepted = true
                        return
                    }

                    volControl._shiftPressed = (mouse.modifiers === Qt.ShiftModifier)

                    if (mouse.button === Qt.LeftButton) {
                        mouse.accepted = false

                        return
                    }

                    volControl.filterEvents = false
                    adjustVolume(mouse)
                }

                onPositionChanged: (mouse) => {
                    if (mouse.buttons & Qt.RightButton)
                        adjustVolume(mouse)
                }

                onWheel: (wheel) => {

                    if ((Math.abs(wheel.pixelDelta.x) % 120 > 0) || (Math.abs(wheel.pixelDelta.y) % 120 > 0)) {
                        let delta = 0
                        if (Math.abs(wheel.pixelDelta.x) > Math.abs(wheel.pixelDelta.y))
                            delta = wheel.pixelDelta.x
                        else
                            delta = wheel.pixelDelta.y

                        if (wheel.inverted)
                            delta = -delta
                        volControl.value += 0.001 * delta
                    }
                    else
                        wheelToVLC.qmlWheelEvent(wheel)

                    wheel.accepted = true
                }

                function adjustVolume(mouse) {
                    mouse.accepted = true

                    const volume = root.designVolume(mouse.x / width)

                    if (volume < 0.25)
                        volControl.value = 0
                    else if (volume < 0.75)
                        volControl.value = root.designPosition(0.5)
                    else if (volume < 1.125 || root._maxvol <= 100)
                        volControl.value = root._fullvolpos
                    else
                        volControl.value = 1
                }

                WheelToVLCConverter {
                    id: wheelToVLC

                    function handle(steps: int) {
                        if (steps > 0)
                            Player.setVolumeUp(steps)
                        else
                            Player.setVolumeDown(-steps)
                    }

                    Component.onCompleted: {
                        wheelUpDown.connect(wheelToVLC.handle)
                        wheelLeftRight.connect(wheelToVLC.handle)
                    }
                }
            }
        }
    }
}
