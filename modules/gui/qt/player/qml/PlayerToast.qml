/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 *****************************************************************************/
import QtQuick

import VLC.Style

// A short message over the video (the subtitles' size Ctrl+wheel made): a
// card of the menus (#191919 at 95 %, radius 10) as Live TV's, a moment
Rectangle {
    id: root

    property string text

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }

    function say(message) {
        text = message
        hideTimer.restart()
    }

    visible: opacity > 0
    opacity: hideTimer.running ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: VLCStyle.duration_short } }
    width: Math.min(label.implicitWidth + px(32), parent ? parent.width - px(40) : label.implicitWidth)
    height: px(40)
    radius: px(10)
    // over a video: dark in both themes
    color: Qt.rgba(25 / 255, 25 / 255, 25 / 255, 0.95)

    Accessible.role: Accessible.AlertMessage
    Accessible.name: text

    Timer {
        id: hideTimer
        interval: 1500
    }

    Text {
        id: label
        anchors.centerIn: parent
        width: Math.min(implicitWidth, root.parent ? root.parent.width - root.px(72) : implicitWidth)
        text: root.text
        elide: Text.ElideRight
        color: "#C6C6C6"
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(14)
    }
}
