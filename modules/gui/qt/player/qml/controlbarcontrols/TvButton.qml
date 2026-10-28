/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick

import VLC.MainInterface
import VLC.Style
import VLC.Util

// "TV" of Customize Player: while a channel plays, the list of the channels
// over the video; else the TV page of the library. Offered in the palette
// while the Live TV add-on is on; in a bar kept from then, it takes no room
// when the add-on is off or left out.
RedesignIconButton {
    id: root
    readonly property bool _available: Iptv.enabled
    iconSource: "qrc:///redesign/icons/tv/default.png"
    implicitWidth: _available ? VLCStyle.dp(30, VLCStyle.scale) : 0
    enabled: _available
    opacity: _available ? 1 : 0
    checked: _available && Iptv.playing
    onClicked: {
        if (Iptv.playing) {
            Iptv.showChannelList()
        } else {
            History.push(["tv"])
            MainCtx.playerView = false
        }
    }
    description: Iptv.playing ? qsTr("Channels") : qsTr("TV")
    AccessibleCompat.id: "tvButton"
}
