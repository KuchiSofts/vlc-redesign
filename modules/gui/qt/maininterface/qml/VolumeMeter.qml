/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.impl as Impl
import QtQuick.Window

import VLC.MainInterface
import VLC.Style
import VLC.Util
import VLC.Player

// The volume as it changes (the wheel, the keys, mute, the slider of a bar
// that is hidden), instead of the message on the video (src/player/osd.c):
// a card at the side of the window with the speaker of the level, the
// design's volume (the bar's wedge and fill) and the percentage. Where it
// shows: Settings > Audio > Volume indicator (MainCtx.volumeMeterPosition):
// on a side, upright, in the middle of it; in a top corner, across. By
// default on the right, on the left in a right-to-left language. It never
// takes the focus or a click, and goes away by itself.
Item {
    id: root

    // the area it shows in (the window without the frame's margin)
    property Item area: parent
    // the top of the window that the top bar takes (the corners go under it)
    property real topInset: 0

    function dp(value) { return VLCStyle.dp(value, VLCStyle.scale) }

    // 1 right side, 2 left side, 3 top right corner, 4 top left corner;
    // 0: by the direction of the language
    readonly property int position: {
        const p = MainCtx.volumeMeterPosition
        if (p >= 1 && p <= 4)
            return p
        return Qt.application.layoutDirection === Qt.RightToLeft ? 2 : 1
    }
    readonly property bool upright: position <= 2
    readonly property bool onRight: position === 1 || position === 3

    readonly property real volume: Player.volume
    readonly property bool muted: Player.muted || volume <= 0
    // as the bar's speaker (VolumeWidget.qml)
    readonly property string level: muted ? "muted" : volume < .25 ? "low" : volume < .5 ? "medium" : "high"

    // the bar's drawn fill for a volume (VolumeWidget.qml: Figma "volume",
    // 0 to 200 %, stretched over the maximum volume)
    readonly property var _designFill: [[0, 0], [10, 5], [20, 12], [30, 18], [40, 24], [50, 31], [60, 39],
                                        [70, 46], [80, 51], [90, 56], [100, 61], [150, 73], [200, 85]]
    function _fill(v) {
        const d = v * 100 * 200 / MainCtx.maxVolume
        const t = _designFill
        if (d <= 0)
            return 0
        for (let i = 1; i < t.length; ++i) {
            if (d <= t[i][0]) {
                const a = t[i - 1], b = t[i]
                return (a[1] + (b[1] - a[1]) * (d - a[0]) / (b[0] - a[0])) / 86
            }
        }
        return 1
    }

    // shown a moment at each change; a slider in sight (the bar's, under
    // the pointer or dragged) shows the volume itself
    property bool _shown: false
    function _changed() {
        if (MainCtx.volumeSliderActive)
            return
        _shown = true
        hideTimer.restart()
    }
    Connections {
        target: Player
        function onVolumeChanged() { root._changed() }
        function onSoundMuteChanged() { root._changed() }
    }
    Timer {
        id: hideTimer
        interval: 1500
        onTriggered: root._shown = false
    }

    // never in the way: no focus, no click, nothing for the keyboard
    enabled: false
    activeFocusOnTab: false
    visible: opacity > 0
    opacity: _shown ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

    // in from the side (or from the top in a corner), 8 px
    property real _slide: _shown ? 0 : dp(8)
    Behavior on _slide { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

    width: card.width
    height: card.height
    // placed by its own coordinates: the sides and corners are the
    // window's, the same in every language (the choice says which)
    x: onRight ? area.x + area.width - width - dp(16) + (upright ? _slide : 0)
               : area.x + dp(16) - (upright ? _slide : 0)
    y: upright ? Math.round(area.y + (area.height - height) / 2)
               : area.y + topInset + dp(16) - _slide

    LayoutMirroring.enabled: false
    LayoutMirroring.childrenInherit: true

    readonly property bool _dark: VLCStyle.palette.isDark

    Rectangle {
        id: card

        // the menus' card (#191919, the light theme's #ababaa) a little
        // translucent over the picture, its 1 px border, 14 px corners
        width: root.upright ? root.dp(56) : content.width + root.dp(28)
        height: root.upright ? content.height + root.dp(28) : root.dp(52)
        radius: root.dp(14)
        color: Qt.alpha(VLCStyle.redesign.color("#191919", "#ababaa", root._dark), 0.95)
        border.width: VLCStyle.dp(1, VLCStyle.scale)
        border.color: VLCStyle.redesign.color(VLCStyle.redesign.border, "#8E8E8D", root._dark)

        Grid {
            id: content
            anchors.centerIn: parent
            columns: root.upright ? 1 : 3
            spacing: root.dp(10)
            horizontalItemAlignment: Grid.AlignHCenter
            verticalItemAlignment: Grid.AlignVCenter

            // the speaker of the level (the bar's)
            Impl.IconImage {
                width: VLCStyle.redesign.iconSize
                height: width
                sourceSize: Qt.size(width, height)
                color: root.muted ? VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e", root._dark)
                                  : VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#2E2E2E", root._dark)
                source: "qrc:///redesign/volume-" + root.level + ".svg"
            }

            // the design's volume: the bar's wedge and its fill, upright on
            // a side (the thin end down)
            Item {
                width: root.upright ? root.dp(27.3633) : root.dp(89)
                height: root.upright ? root.dp(89) : root.dp(27.3633)

                Item {
                    anchors.centerIn: parent
                    width: root.dp(89)
                    height: root.dp(27.3633)
                    rotation: root.upright ? -90 : 0

                    Image {
                        anchors.fill: parent
                        sourceSize: Qt.size(width, height)
                        source: root._dark ? "qrc:/redesign/volume-background.svg"
                                           : SVGColorImage.colorize("qrc:/redesign/volume-background.svg")
                                                          .any({ "#090909": "#888888" }).uri()
                    }
                    Item {
                        x: root.dp(1.5)
                        anchors.verticalCenter: parent.verticalCenter
                        width: root.dp(86) * (root.muted ? 0 : root._fill(root.volume))
                        height: root.dp(24.56)
                        clip: true
                        Behavior on width { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

                        Image {
                            width: root.dp(86)
                            height: root.dp(24.56)
                            sourceSize: Qt.size(width, height)
                            source: root._dark ? "qrc:/redesign/volume-fill.svg"
                                               : SVGColorImage.colorize("qrc:/redesign/volume-fill.svg")
                                                              .any({ "black": "#ffffff" }).uri()
                        }
                    }
                }
            }

            // the percentage (as the bar's: "100 %"), or that it is muted
            Text {
                width: root.upright ? root.dp(48) : implicitWidth
                horizontalAlignment: Text.AlignHCenter
                text: Player.muted ? qsTr("Mute") : qsTr("%1 %").arg(Math.max(0, Math.round(root.volume * 100)))
                elide: Text.ElideRight
                color: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#2E2E2E", root._dark)
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.dp(13)
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
            }
        }
    }

    Accessible.ignored: true
}
