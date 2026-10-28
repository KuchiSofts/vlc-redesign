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

// The rows of the library's sidebar, in the order the user gave them (Edit
// Sidebar), with the ones hidden: kept in the settings of the interface as
// lists of keys. TV is a row of its own while the Live TV add-on is on; off,
// its place is kept for when it comes back.
QtObject {
    id: root

    readonly property string orderKey: "MainWindow/sidebar-order"
    readonly property string hiddenKey: "MainWindow/sidebar-hidden"

    // every row, in the order of the design
    readonly property var defaults: [
        { key: "home", title: qsTr("Home"), uri: ["home"], icon: VLCIcons.home, asset: "home" },
        { key: "video", title: qsTr("Videos"), uri: ["video", "all"], icon: VLCIcons.topbar_video, asset: "video", library: true },
        { key: "music", title: qsTr("Music"), uri: ["music"], icon: VLCIcons.topbar_music, asset: "music", library: true },
        { key: "network", title: qsTr("Network"), uri: ["network"], icon: VLCIcons.topbar_network, asset: "network" },
        { key: "tv", title: qsTr("TV"), uri: ["tv"], icon: "", asset: "tv", addon: true },
        // every playlist (the page lists songs ones too, see VideoPlaylistsDisplay)
        { key: "playlists", title: qsTr("Playlists"), uri: ["video", "playlists"], icon: VLCIcons.playlist, asset: "playlists", library: true },
        // the media played last, videos and songs
        { key: "recent", title: qsTr("Recent"), uri: ["home", "recent"], icon: VLCIcons.history, asset: "recent", library: true }
    ]

    // read again when Edit Sidebar keeps a change (or another window does)
    property int _revision: 0
    readonly property Connections _settings: Connections {
        target: MainCtx
        function onSettingValueChanged(key) {
            if (key === root.orderKey || key === root.hiddenKey)
                ++root._revision
        }
    }

    function _keys(key) {
        void _revision
        // a text of keys separated by commas (a list of one would be read
        // back as a text by the settings)
        return String(MainCtx.settingValue(key, "")).split(",").filter(k => k.length > 0)
    }

    // Every key in the user's order; a row the saved order does not know (a
    // new one) after the row it follows by default
    readonly property var order: {
        const saved = _keys(orderKey).filter(k => defaults.some(d => d.key === k))
        const out = saved.filter((k, i) => saved.indexOf(k) === i)
        for (let i = 0; i < defaults.length; ++i) {
            const key = defaults[i].key
            if (out.indexOf(key) >= 0)
                continue
            let at = 0
            for (let j = i - 1; j >= 0; --j) {
                const before = out.indexOf(defaults[j].key)
                if (before >= 0) {
                    at = before + 1
                    break
                }
            }
            out.splice(at, 0, key)
        }
        return out
    }
    readonly property var hidden: _keys(hiddenKey)

    function entry(key) {
        return defaults.find(d => d.key === key)
    }
    // the row can be in the sidebar (TV: its add-on is on)
    function available(key) {
        return key !== "tv" || Iptv.enabled
    }

    // the rows shown, in their order
    // (Home when none would be: TV alone kept, its add-on then off)
    readonly property var shown: {
        const rows = order.filter(k => available(k) && hidden.indexOf(k) < 0).map(k => entry(k))
        return rows.length > 0 ? rows : [entry("home")]
    }

    function save(newOrder, newHidden) {
        MainCtx.setSettingValue(orderKey, newOrder.join(","))
        MainCtx.setSettingValue(hiddenKey, newHidden.join(","))
    }
}
