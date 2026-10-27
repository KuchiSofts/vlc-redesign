/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQml

import VLC.Dialogs
import VLC.MediaLibrary
import VLC.Widgets as Widgets

// Figma "playlist context menu" (2459:9389) for the playlists of the library:
// Play, Favorite, Shuffle, Export Playlist, Add to Queue, Add Subtitle
// Tracks, Add Audio Tracks, Properties & Metadata; then, after a line, the
// actions of VLC the design does not list (Rename, Delete). Every row applies
// to all the playlists selected, unless greyed out for more than one.
Widgets.RedesignContextMenu {
    id: root

    required property MLBaseModel model

    // Delete: the page asks first (PlaylistMediaList.deleteSelectedPlaylists)
    signal requestDeleteSelectedPlaylists()
    // Add Subtitle Tracks / Add Audio Tracks: the media of the playlist
    signal requestTracksDialog(var playlistId, bool audio)

    // required by TableViewExt
    function tableView_popup(index, selectedIndexes, globalPos) {
        return popup(selectedIndexes, globalPos)
    }

    actions: [{
            "text": qsTr("Play"),
            "icon": "play",
            "action": (dataList) => root.model.ml.addAndPlay(root._ids(dataList))
        }, {
            "text": (dataList) => root._allFavorite(dataList) ? qsTr("Remove from Favorites") : qsTr("Favorite"),
            "icon": "favorite",
            "action": (dataList, options, indexes) => {
                const favorite = !root._allFavorite(dataList)
                for (let i = 0; i < indexes.length; ++i)
                    root.model.setFavorite(indexes[i], favorite)
            }
        }, {
            "text": qsTr("Shuffle"),
            "icon": "shuffle",
            "action": (dataList) => root.model.ml.addAndPlayShuffled(root._ids(dataList))
        }, {
            "text": qsTr("Export Playlist"),
            "icon": "export",
            "action": (dataList) => root.model.ml.exportToFile(root._ids(dataList))
        }, {
            "text": qsTr("Add to Queue"),
            "icon": "queue",
            "action": (dataList) => root.model.ml.addToPlaylist(root._ids(dataList))
        }, {
            "text": qsTr("Add Subtitle Tracks"),
            "icon": "subtitles",
            "action": (dataList) => root.requestTracksDialog(dataList[0].id, false),
            "enabled": (dataList) => dataList.length === 1
        }, {
            "text": qsTr("Add Audio Tracks"),
            "icon": "audio",
            "action": (dataList) => root.requestTracksDialog(dataList[0].id, true),
            "enabled": (dataList) => dataList.length === 1
        }, {
            // what the library knows of it (and its file, for a playlist
            // made from one): the media information dialog takes a media
            "text": qsTr("Properties & Metadata"),
            "icon": "info",
            "action": (dataList) => root._propertiesDialog().openFor(dataList[0]),
            "enabled": (dataList) => dataList.length === 1
        }, {
            "separator": true
        }, {
            "text": qsTr("Rename"),
            "icon": "edit",
            "action": (dataList, options, indexes) => root.model.showDialogRename(indexes[0]),
            "visible": (dataList) => dataList.length === 1
        }, {
            "text": qsTr("Delete"),
            "icon": "remove",
            "action": () => root.requestDeleteSelectedPlaylists()
        }
    ]

    onRequestData: (requestID, indexes) => {
        model.getData(indexes, function (data) {
            setData(requestID, data)
        })
    }

    property var _dialog: null
    function _propertiesDialog() {
        if (!_dialog)
            _dialog = propertiesComponent.createObject(g_mainDisplay)
        return _dialog
    }
    // made on the main display, not a child: it goes with the menu
    Component.onDestruction: if (_dialog) _dialog.destroy()
    property Component propertiesComponent: Component { PlaylistPropertiesDialog {} }

    function _ids(dataList) {
        return dataList.map(data => data.id)
    }

    function _allFavorite(dataList) {
        return dataList.length > 0 && dataList.every(data => data.isFavorite === true)
    }
}
