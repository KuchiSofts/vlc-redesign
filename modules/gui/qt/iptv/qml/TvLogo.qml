/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick

import VLC.Style

// The logo of a channel on the thumbnail well of the library (#171717,
// radius 6); until it comes (from the add-on's cache, or never), the
// initials of the channel
Rectangle {
    id: root

    property string source
    property string name
    property bool forceDark: false
    readonly property bool _dark: forceDark || VLCStyle.palette.isDark

    radius: VLCStyle.dp(6, VLCStyle.scale)
    color: VLCStyle.redesign.color("#171717", undefined, _dark)
    clip: true

    Image {
        id: image
        anchors.fill: parent
        anchors.margins: VLCStyle.dp(4, VLCStyle.scale)
        source: root.source
        // twice the size, for the screens of more pixels
        sourceSize: Qt.size(width * 2, height * 2)
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        cache: true
        smooth: true
        visible: status === Image.Ready
    }

    Text {
        anchors.centerIn: parent
        visible: image.status !== Image.Ready
        // "Channel 12 HD" -> "C1", "BBC One" -> "BO"
        text: root.name.replace(/^[A-Za-z]{2,3}\s*[:|]\s+/, "").split(/\s+/).filter(w => w.length > 0)
                  .slice(0, 2).map(w => w.charAt(0).toUpperCase()).join("")
        color: VLCStyle.redesign.color("#6B6B6B", undefined, root._dark)
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: Math.round(root.height * 0.34)
        font.weight: Font.DemiBold
    }
}
