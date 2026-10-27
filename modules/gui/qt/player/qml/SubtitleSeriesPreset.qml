/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
pragma Singleton

import QtQuick

import VLC.MainInterface
import VLC.Player

Item {
    id: root
    objectName: "subtitleSeriesPreset"
    width: 0
    height: 0
    visible: false

    // A series token such as S01E02 distinguishes shows sharing one directory.
    // Without that token, the explicit preset applies to this local folder.
    readonly property string seriesKey: {
        const url = Player.url.toString()
        if (!url.startsWith("file://")) return ""
        const slash = url.lastIndexOf("/")
        if (slash < 8) return ""
        const directory = url.substring(0, slash)
        const filename = decodeURIComponent(url.substring(slash + 1))
        const episode = filename.match(/^(.+?)[ ._-]+s\d{1,2}e\d{1,3}/i)
        const series = episode ? episode[1].replace(/[._-]+/g, " ").trim().toLowerCase() : ""
        return directory + "|" + series
    }
    readonly property string label: {
        if (!seriesKey) return ""
        const parts = seriesKey.split("|")
        if (parts[1].length > 0) return parts[1]
        const directory = parts[0]
        return decodeURIComponent(directory.substring(directory.lastIndexOf("/") + 1))
    }
    readonly property string storageKey: "redesign/subtitleSeries/" + encodeURIComponent(seriesKey)
    property var preset: null
    // what was done last, said in the language of the interface (again in
    // another one chosen meanwhile)
    property string _statusKind: ""
    readonly property string status: {
        switch (_statusKind) {
        case "unreadable": return qsTr("The saved series preset could not be read.")
        case "unavailable": return qsTr("A saved audio or subtitle track is unavailable in this episode.")
        case "applied": return qsTr("Series preset applied.")
        case "saved": return qsTr("Saved for episodes of %1.").arg(label)
        case "removed": return qsTr("Series preset removed.")
        default: return ""
        }
    }
    property bool delayApplied: false
    property bool audioApplied: false
    property bool subtitleApplied: false

    function selectedTrackName(model) {
        for (let i = 0; i < model.count; ++i) {
            const index = model.index(i, 0)
            if (model.data(index, Qt.CheckStateRole))
                return model.data(index, Qt.DisplayRole)
        }
        return ""
    }

    function selectTrack(model, title) {
        for (let i = 0; i < model.count; ++i) {
            const index = model.index(i, 0)
            if (model.data(index, Qt.DisplayRole) === title)
                return model.setData(index, true, Qt.CheckStateRole)
        }
        return false
    }

    function disableTracks(model) {
        if (model.count === 0) return false
        for (let i = 0; i < model.count; ++i) {
            const index = model.index(i, 0)
            if (model.data(index, Qt.CheckStateRole) &&
                    !model.setData(index, false, Qt.CheckStateRole))
                return false
        }
        return true
    }

    function refresh() {
        delayApplied = false
        audioApplied = false
        subtitleApplied = false
        preset = null
        _statusKind = ""
        if (!seriesKey) return
        const raw = MainCtx.settingValue(storageKey, "")
        if (!raw) return
        try {
            const loaded = JSON.parse(raw)
            if (loaded.version !== 1 && loaded.version !== 2) return
            preset = loaded
            Qt.callLater(tryApply)
        } catch (error) {
            _statusKind = "unreadable"
        }
    }

    function tryApply() {
        if (!preset || !seriesKey || !Player.isStarted) return
        if (!delayApplied) {
            Player.subtitleDelayMS = preset.subtitleDelayMS
            Player.secondarySubtitleDelayMS = preset.secondarySubtitleDelayMS
            Player.subtitleFPS = preset.subtitleFPS
            delayApplied = true
        }
        // Version 1 had no explicit Off state: an empty title meant no selection request.
        if (!audioApplied)
            audioApplied = preset.version === 2 && preset.audioOff
                         ? disableTracks(Player.audioTracks)
                         : !preset.audioTrack || selectTrack(Player.audioTracks, preset.audioTrack)
        if (!subtitleApplied)
            subtitleApplied = preset.version === 2 && preset.subtitleOff
                            ? disableTracks(Player.subtitleTracks)
                            : !preset.subtitleTrack || selectTrack(Player.subtitleTracks, preset.subtitleTrack)
        if (!audioApplied || !subtitleApplied)
            _statusKind = "unavailable"
        else if (delayApplied && audioApplied && subtitleApplied)
            _statusKind = "applied"
    }

    function saveCurrent() {
        if (!seriesKey || !Player.isStarted) return false
        const audioTrack = selectedTrackName(Player.audioTracks)
        const subtitleTrack = selectedTrackName(Player.subtitleTracks)
        const next = {
            version: 2,
            audioTrack: audioTrack,
            subtitleTrack: subtitleTrack,
            audioOff: Player.audioTracks.count > 0 && !audioTrack,
            subtitleOff: Player.subtitleTracks.count > 0 && !subtitleTrack,
            subtitleDelayMS: Player.subtitleDelayMS,
            secondarySubtitleDelayMS: Player.secondarySubtitleDelayMS,
            subtitleFPS: Player.subtitleFPS
        }
        MainCtx.setSettingValue(storageKey, JSON.stringify(next))
        preset = next
        delayApplied = true
        audioApplied = true
        subtitleApplied = true
        _statusKind = "saved"
        return true
    }

    function clear() {
        if (!seriesKey) return
        MainCtx.removeSettingValue(storageKey)
        preset = null
        _statusKind = "removed"
    }

    Component.onCompleted: refresh()
    Connections {
        target: Player
        function onInputChanged() { root.refresh() }
        function onPlayingStateChanged() { Qt.callLater(root.tryApply) }
    }
    Connections {
        target: Player.audioTracks
        function onCountChanged() { Qt.callLater(root.tryApply) }
        function onDataChanged() { if (!root.audioApplied) Qt.callLater(root.tryApply) }
    }
    Connections {
        target: Player.subtitleTracks
        function onCountChanged() { Qt.callLater(root.tryApply) }
        function onDataChanged() { if (!root.subtitleApplied) Qt.callLater(root.tryApply) }
    }
}
