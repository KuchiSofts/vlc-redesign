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

// The rows of the library's sidebar, in the order the user gave them (Edit
// Sidebar), with the ones hidden: kept in the settings of the interface as
// lists of keys. TV is a row of its own while the Live TV add-on is on; off,
// its place is kept for when it comes back. Folders and playlists can be
// pinned to it (their menus, as the designer proposed): rows of their own,
// moved and hidden in Edit Sidebar like the others, taken off with Unpin.
QtObject {
    id: root

    readonly property string orderKey: "MainWindow/sidebar-order"
    readonly property string hiddenKey: "MainWindow/sidebar-hidden"
    // the pinned folders and playlists: a JSON list of { kind: "folder", url,
    // title } and { kind: "playlist", id, title } (the id as the library
    // writes it, "VLC_ML_PARENT_PLAYLIST - 5": see playlistProps)
    readonly property string pinsKey: "MainWindow/sidebar-pins"

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
            if (key === root.orderKey || key === root.hiddenKey || key === root.pinsKey)
                ++root._revision
        }
    }

    function _keys(key) {
        void _revision
        // a text of keys separated by commas (a list of one would be read
        // back as a text by the settings)
        return String(MainCtx.settingValue(key, "")).split(",").filter(k => k.length > 0)
    }

    // the pins as rows (their key: the folder's URL encoded, so without the
    // commas that separate the keys; the playlist's id)
    function _pinKey(pin) {
        return pin.kind === "folder" ? "pin-f-" + encodeURIComponent(pin.url) : "pin-p-" + pin.id
    }
    readonly property var pins: {
        void _revision
        let list = []
        try {
            list = JSON.parse(String(MainCtx.settingValue(pinsKey, "[]")))
        } catch (e) {
            list = []
        }
        if (!Array.isArray(list))
            list = []
        return list.filter(p => p && (p.kind === "folder" ? !!p.url : (p.kind === "playlist" && p.id !== undefined)))
                   .map(p => ({
                       key: _pinKey(p), title: String(p.title || ""), pin: p,
                       uri: p.kind === "folder" ? ["folder"] : ["video", "playlists", "list"],
                       props: p.kind === "folder" ? { folderUrl: p.url } : null,
                       icon: p.kind === "folder" ? VLCIcons.folder : VLCIcons.playlist,
                       asset: p.kind === "folder" ? "folder" : "playlists",
                       library: p.kind === "playlist"
                   }))
    }
    readonly property var _all: defaults.concat(pins)

    // Every key in the user's order; a row the saved order does not know (a
    // new one) after the row it follows by default (a new pin: at the end)
    readonly property var order: {
        const saved = _keys(orderKey).filter(k => _all.some(d => d.key === k))
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
        for (const pin of pins)
            if (out.indexOf(pin.key) < 0)
                out.push(pin.key)
        return out
    }
    readonly property var hidden: _keys(hiddenKey)

    function entry(key) {
        return _all.find(d => d.key === key)
    }

    // the page of a pinned playlist (the library's id made again from its text)
    function playlistProps(pin) {
        return { parentId: MainCtx.mediaLibrary.deserializeMlItemIdFromString(String(pin.id)), title: pin.title }
    }

    function isPinnedFolder(url) {
        return pins.some(p => p.pin.kind === "folder" && p.pin.url === String(url))
    }
    function isPinnedPlaylist(id) {
        return pins.some(p => p.pin.kind === "playlist" && String(p.pin.id) === String(id))
    }
    function _savePins(list) {
        MainCtx.setSettingValue(pinsKey, JSON.stringify(list))
    }
    // a folder's title: its last name (the drive for a drive's root)
    function folderTitle(url) {
        const path = Helpers.localPathFromUrl(String(url)).replace(/[\\/]+$/, "")
        const name = path.substring(Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\")) + 1)
        return name.length > 0 ? name : path
    }
    function pinFolder(url) {
        if (isPinnedFolder(url))
            return
        _savePins(pins.map(p => p.pin).concat([{ kind: "folder", url: String(url), title: folderTitle(url) }]))
    }
    function pinPlaylist(id, title) {
        if (isPinnedPlaylist(id))
            return
        _savePins(pins.map(p => p.pin).concat([{ kind: "playlist", id: String(id), title: String(title) }]))
    }
    // taken off the list, its place in the order and the hidden list too
    function unpin(key) {
        _savePins(pins.filter(p => p.key !== key).map(p => p.pin))
        save(order.filter(k => k !== key), hidden.filter(k => k !== key))
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
