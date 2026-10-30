/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic as Controls

import VLC.Style
import VLC.Util

// A small mark of what a channel or a source can do (catch-up and how far
// back, the guide, films and series, free): an icon of the two-tone set and
// a word, on the controls' grey (radius 4); accent: on the accent's dark
// background
Rectangle {
    id: root

    property string text
    // a two-tone icon of pixmaps/redesign/duo, or none
    property string icon
    property string tip
    property bool accent: false
    property bool forceDark: false
    readonly property bool _dark: forceDark || VLCStyle.palette.isDark

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }

    implicitWidth: row.implicitWidth + px(12)
    implicitHeight: px(20)
    radius: px(4)
    color: accent ? VLCStyle.redesign.color(VLCStyle.redesign.accentBackground, undefined, _dark)
                  : VLCStyle.redesign.color("#191919", undefined, _dark)

    Row {
        id: row
        anchors.centerIn: parent
        spacing: root.px(4)

        Image {
            visible: root.icon !== ""
            anchors.verticalCenter: parent.verticalCenter
            width: root.px(14)
            height: width
            sourceSize: Qt.size(width, height)
            source: root.icon === "" ? ""
                    : SVGColorImage.colorize("qrc:///redesign/duo/" + root.icon + ".svg")
                                   .color1(root.accent ? VLCStyle.redesign.accent
                                                       : VLCStyle.redesign.color("#8F8F8F", undefined, root._dark))
                                   .color2(VLCStyle.redesign.accent).uri()
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            color: root.accent ? VLCStyle.redesign.accent : VLCStyle.redesign.color("#8F8F8F", undefined, root._dark)
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(11)
            font.weight: Font.Medium
        }
    }

    HoverHandler { id: hover }
    Controls.ToolTip.visible: hover.hovered && root.tip !== ""
    Controls.ToolTip.text: root.tip
    Controls.ToolTip.delay: 650
}
