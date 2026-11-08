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
import VLC.Widgets as Widgets

// The menu of a channel (Figma "file context menu" rows), a right-click on a
// channel of the TV page or of the library next to the video: Watch, Watch
// from the start (with catch-up), Favorite, Show in the Guide, What's On.
// popup([row], point) with a row of `channels`.
Widgets.RedesignContextMenu {
    id: root

    // the channels of the rows: Iptv.channelModel, Iptv.playerModel
    property var channels: null

    // the guide and the programme's card belong to the page showing them
    signal guideRequested(string key)
    signal programmeRequested(var channel, var programme)

    function channel(dataList) { return dataList[0] }

    actions: [
        { text: qsTr("Watch"), icon: "play",
          action: (dataList) => Iptv.play(root.channel(dataList).key) },
        { text: qsTr("Watch from the start"), icon: "recent",
          visible: (dataList) => root.channel(dataList).catchupDays > 0
                                 && root.channel(dataList).nowStart > 0,
          action: (dataList) => {
              const ch = root.channel(dataList)
              Iptv.playCatchup(ch.key, ch.nowStart, ch.nowStop, ch.nowTitle)
          } },
        { text: (dataList) => root.channel(dataList).favorite ? qsTr("Remove from Favorites")
                                                              : qsTr("Favorite"),
          icon: "favorite",
          action: (dataList) => Iptv.toggleFavorite(root.channel(dataList).key) },
        { text: qsTr("Show in the Guide"), icon: "lists",
          action: (dataList) => root.guideRequested(root.channel(dataList).key) },
        { text: qsTr("What's On"), icon: "info",
          enabled: (dataList) => root.channel(dataList).nowStart > 0,
          action: (dataList) => {
              const ch = root.channel(dataList)
              root.programmeRequested(ch, { start: ch.nowStart, stop: ch.nowStop, title: ch.nowTitle })
          } }
    ]

    onRequestData: (requestID, indexes) => {
        setData(requestID, indexes.map(i => root.channels.get(i)))
    }
}
