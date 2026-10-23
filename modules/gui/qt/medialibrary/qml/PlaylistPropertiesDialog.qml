/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts

import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// Properties & Metadata of a playlist (Figma "playlist context menu",
// 2459:9389): what the library knows of it, in the dialog of the redesign.
// The media information dialog of VLC takes a media; a playlist made from a
// file shows that file here too.
Widgets.RedesignDialog {
    id: root

    objectName: "playlistPropertiesDialog"

    // the data of the playlist's row (MLPlaylistListModel): name, count,
    // nb_video, nb_audio, duration, mrl
    property var playlist: ({})

    function openFor(data) {
        playlist = data
        open()
    }

    title: qsTr("Properties & Metadata")
    acceptText: qsTr("Close")
    cancelVisible: false
    preferredWidth: VLCStyle.dp(560, VLCStyle.scale)

    // "15 videos", "3 songs", "2 videos, 1 song" (the plural forms of qsTr
    // gave no text without a translation)
    function mediaText(p) {
        const videos = Number(p.nb_video ?? 0)
        const songs = Number(p.nb_audio ?? 0)
        const parts = []
        if (videos > 0)
            parts.push(videos === 1 ? qsTr("1 video") : qsTr("%1 videos").arg(videos))
        if (songs > 0)
            parts.push(songs === 1 ? qsTr("1 song") : qsTr("%1 songs").arg(songs))
        if (parts.length)
            return parts.join(", ")
        const count = Number(p.count ?? 0)
        return count === 1 ? qsTr("1 item") : qsTr("%1 items").arg(count)
    }

    component InfoRow: RowLayout {
        id: row
        property string label
        property string value
        Layout.fillWidth: true
        visible: value.length > 0
        spacing: VLCStyle.dp(20, VLCStyle.scale)
        Text {
            Layout.preferredWidth: VLCStyle.dp(120, VLCStyle.scale)
            text: row.label
            color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
        Text {
            Layout.fillWidth: true
            text: row.value
            color: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#1d1d1d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            wrapMode: Text.WrapAnywhere
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: VLCStyle.dp(14, VLCStyle.scale)

        InfoRow { label: qsTr("Name"); value: String(root.playlist.name ?? "") }
        InfoRow { label: qsTr("Media"); value: root.mediaText(root.playlist) }
        InfoRow {
            label: qsTr("Length")
            value: root.playlist.duration?.formatHMS?.() ?? ""
        }
        InfoRow {
            label: qsTr("File")
            value: root.playlist.mrl ? Helpers.localPathFromUrl(root.playlist.mrl) : ""
        }
    }
}
