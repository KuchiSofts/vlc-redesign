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
    // the shortcut that did it, under the message, dimmer (the designer's
    // OSD: what happened and how to do it again)
    property string shortcut

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }

    function say(message, keys) {
        text = message
        shortcut = keys ?? ""
        hideTimer.restart()
    }

    visible: opacity > 0
    opacity: hideTimer.running ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: VLCStyle.duration_short } }
    width: Math.min(Math.max(label.implicitWidth, keysLabel.visible ? keysLabel.implicitWidth : 0) + px(32),
                    parent ? parent.width - px(40) : label.implicitWidth)
    height: keysLabel.visible ? px(58) : px(40)
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
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: keysLabel.visible ? -root.px(9) : 0
        width: Math.min(implicitWidth, root.parent ? root.parent.width - root.px(72) : implicitWidth)
        text: root.text
        elide: Text.ElideRight
        color: "#C6C6C6"
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(14)
    }

    Text {
        id: keysLabel
        visible: root.shortcut.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: label.bottom
        anchors.topMargin: root.px(2)
        text: root.shortcut
        color: "#8F8F8F"
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(12)
    }
}
