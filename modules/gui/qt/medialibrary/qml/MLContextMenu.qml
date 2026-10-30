
/*****************************************************************************
 * Copyright (C) 2022 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
import QtQml

import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Dialogs
import VLC.Widgets as Widgets

import VLC.Util

// @brief - a generic ML context menu, as the design's "file context menu"
Widgets.RedesignContextMenu {
    id: root

    // Properties

    required property MLBaseModel model

    property string idDataRole: "id"
    property string albumIdDataRole: "album_id"

    property bool showPlayAsAudioAction: false

    // Actions of the page, after the ones of VLC (see RedesignContextMenu)
    property var extraActions: []

    property var subtitleDialog: null
    property var audioTracksDialog: null
    property Component subtitleDialogComponent: Component { SubtitleAssociationsDialog {} }
    Component.onDestruction: {
        if (subtitleDialog) subtitleDialog.destroy()
        if (audioTracksDialog) audioTracksDialog.destroy()
    }

    // This is only applicable when the model is `MLAudioModel`:
    property bool showPlayAsAlbumAction: implicitShowPlayAsAlbumAction
    property bool implicitShowPlayAsAlbumAction: !showPlayAsAudioAction

    // Signals

    signal showMediaInformation(int index)


    // required by TableViewExt
    property var tableView_popup: function (index/* reserved for future */, selectedIndexes, globalPos) {
        return popup(selectedIndexes, globalPos)
    }

    // Settings

    // Figma "file context menu" (2404:13340): Play, Mark as Watched, Favorite,
    // Stream, Convert, Add to Queue, Add to Playlist, Show in File Explorer,
    // Go to Source, Add Subtitle Tracks, Add Audio Tracks, Properties &
    // Metadata. A row the kind of item has no use for (an album has no file)
    // is left out, one the selection can't take is greyed out. Then, after a
    // line, the actions of VLC the design does not list.
    actions: [{
            "text": qsTr("Play"),
            "icon": "play",
            "action": addAndPlay
        }, {
            // all watched (played to the end once, as the cards dim them;
            // one stopped partway is not): the row marks them unwatched
            "text": (dataList) => root._every(dataList, data => (data["playcount"] ?? 0) > 0)
                                  ? qsTr("Mark as Unwatched") : qsTr("Mark as Watched"),
            "icon": "mark-watched",
            "action": (dataList, options, indexes) => {
                root._setPlayed(indexes, !root._every(dataList, data => (data["playcount"] ?? 0) > 0))
            },
            "visible": (dataList) => typeof root.model.setItemPlayed === "function"
                                     && root._every(dataList, data => typeof data["isNew"] === "boolean")
        }, {
            "text": (dataList) => root._every(dataList, data => data["isFavorite"] === true)
                                  ? qsTr("Remove from Favorites") : qsTr("Favorite"),
            "icon": "favorite",
            "action": (dataList, options, indexes) => {
                root._setFavorite(indexes, !root._every(dataList, data => data["isFavorite"] === true))
            },
            "visible": (dataList) => typeof root.model.setMediaIsFavorite === "function"
                                     && root._every(dataList, data => typeof data["isFavorite"] === "boolean")
        }, {
            "text": qsTr("Stream"),
            "icon": "streaming",
            // the 2nd argument: transcode only (Convert), not the Stream output
            "action": function(dataList) { DialogsProvider.streamingDialog(_urlList(dataList), false) },
            "visible": _hasUrls
        }, {
            "text": qsTr("Convert"),
            "icon": "swap",
            "action": function(dataList) { DialogsProvider.streamingDialog(_urlList(dataList), true) },
            "visible": _hasUrls
        }, {
            "text": qsTr("Add to Queue"),
            "icon": "queue",
            "action": enqueue
        }, {
            "text": qsTr("Add to Playlist"),
            "icon": "playlist",
            "action": addToAPlaylist
        }, {
            // the folder of the file opens with the file selected
            "text": qsTr("Show in File Explorer"),
            "icon": "folder-open",
            "action": openContainingFolder,
            "visible": _hasUrls,
            "enabled": _openContainingFolder
        }, {
            // the folder of the file in the library
            "text": qsTr("Go to Source"),
            "icon": "go-to",
            "action": function(dataList, options, indexes) {
                History.push(["folder"], { folderUrl: _parentUrl(dataList, indexes) }, Qt.MouseFocusReason)
            },
            "visible": _hasUrls,
            "enabled": _openContainingFolder
        }, {
            "text": qsTr("Add Subtitle Tracks"),
            "icon": "subtitles",
            "action": function(dataList) { _tracksDialog(false).openFor(dataList[0][root.idDataRole]) },
            "visible": _showTracks,
            "enabled": _single
        }, {
            "text": qsTr("Add Audio Tracks"),
            "icon": "audio",
            "action": function(dataList) { _tracksDialog(true).openFor(dataList[0][root.idDataRole]) },
            "visible": _showTracks,
            "enabled": _single
        }, {
            "text": qsTr("Properties & Metadata"),
            "icon": "info",
            "action": function(dataList, options, indexes) {
                DialogsProvider.mediaInfoDialog(dataList[0][idDataRole])
            },
            // a media (not a group, an album...)
            "visible": function(dataList) {
                return _every(dataList, data => !!data[root.idDataRole] && !data[root.idDataRole].hasParent())
            },
            "enabled": _single
        }, {
            "separator": true
        }, {
            "text": qsTr("Play as Audio"),
            "icon": "audio",
            "action": playAsAudio,
            "visible": root.showPlayAsAudioAction
        }, {
            "text": qsTr("Play as Album"),
            "icon": "album",
            "action": playAsAlbum,
            "visible": root.showPlayAsAlbumAction &&
                       (root.model instanceof MLAudioModel)
        }, {
            "text": qsTr("Add Album to Queue"),
            "icon": "queue",
            "action": enqueueAsAlbum,
            "visible": root.showPlayAsAlbumAction &&
                       (root.model instanceof MLAudioModel)
        }, {
            "text": (root.showPlayAsAudioAction ? qsTr("Add to a Video-only Playlist") : qsTr("Add to an Audio-only Playlist")),
            "icon": "playlist",
            "action": (root.showPlayAsAudioAction ? root.addToVideoOnlyPlaylist : root.addToAudioOnlyPlaylist)
        }, {
            "text": qsTr("Information"),
            "icon": "info",
            "action": _signalShowInformation,
            "visible": showInformationAvailable
        }, {
            "text": qsTr("Delete"),
            "icon": "remove",
            "action": deleteStream,
            "visible": _deleteStream
        }, {
            "text": qsTr("Delete File From System"),
            "icon": "remove",
            "action": deleteFileFromSource,
            "visible": _deleteFileFromSource
        }
    ].concat(root.extraActions)

    // Events

    onRequestData: (requestID, indexes) => {
        model.getData(indexes, function (data) {
            setData(requestID, data)
        })
    }

    // Functions

    function addAndPlay(dataList, options, indexes) {
        model.ml.addAndPlay(_mlIDList(dataList), _playerOptions(options))
    }

    function playAsAudio(dataList, options, indexes) {
        model.ml.addAndPlay(_mlIDList(dataList), _playerOptions(options, ":no-video"))
    }

    function playAsAlbum(dataList, options, indexes) {
        model.ml.addAndPlay(_mlIDList(dataList, root.albumIdDataRole), _playerOptions(options))
    }

    function enqueue(dataList, options, indexes) {
        model.ml.addToPlaylist(_mlIDList(dataList), _playerOptions(options))
    }

    function enqueueAsAlbum(dataList, options, indexes) {
        model.ml.addToPlaylist(_mlIDList(dataList, root.albumIdDataRole), _playerOptions(options))
    }

    function addToAudioOnlyPlaylist(dataList, options, indexes) {
        addToAPlaylist(dataList, options, indexes, MLPlaylistListModel.PLAYLIST_TYPE_AUDIO_ONLY)
    }

    function addToVideoOnlyPlaylist(dataList, options, indexes) {
        addToAPlaylist(dataList, options, indexes, MLPlaylistListModel.PLAYLIST_TYPE_VIDEO_ONLY)
    }

    function addToAPlaylist(dataList, options, indexes, type) {
        if (type === undefined)
            type = MLPlaylistListModel.PLAYLIST_TYPE_ALL

        DialogsProvider.playlistsDialog(_mlIDList(dataList), type)
    }

    function addFavorite(dataList, options, indexes) {
        _setFavorite(indexes, true)
    }

    function removeFavorite(dataList, options, indexes) {
        _setFavorite(indexes, false)
    }

    function markSeen(dataList, options, indexes) {
        _setPlayed(indexes, true)
    }

    function markUnseen(dataList, options, indexes) {
        _setPlayed(indexes, false)
    }

    function openContainingFolder(dataList, options, indexes) {
        MainCtx.showInFileManager(String(dataList[0]["mrl"]))
    }

    function deleteStream(dataList, options, indexes) {
        model.deleteStream(dataList[0][idDataRole])
    }

    function deleteFileFromSource(dataList, options, indexes) {
        // asked even without the confirmations: the file is gone for good
        let confirm = DialogsProvider.questionDialog(qsTr("Are you sure you want to delete this file?"),
                                                     qsTr("Delete file"), true, qsTr("Delete"), qsTr("Cancel"));

        if (confirm) {
            model.deleteFileFromSource(indexes[0]);
            console.log("File Deleted !!");
        }
    }

    function showInformationAvailable(dataList, options, indexes) {
        return indexes.length === 1
                && Helpers.isInteger(options?.["information"] ?? null)
    }

    // Private

    // (false for no item)
    function _every(dataList, test) {
        return dataList.length > 0 && dataList.every(test)
    }

    function _single(dataList) {
        return dataList.length === 1
    }

    function _checkRole(dataList, role, expected) {
        if (dataList.length !== 1)
            return false

        if (!(role in dataList[0]))
            return false

        return (dataList[0][role] === expected)
    }

    // every selected item (as Add to Queue)
    function _setPlayed(indexes, played) {
        for (let i = 0; i < indexes.length; ++i)
            model.setItemPlayed(indexes[i], played)
    }

    function _setFavorite(indexes, favorite) {
        for (let i = 0; i < indexes.length; ++i)
            model.setMediaIsFavorite(indexes[i], favorite)
    }

    // A file of the computer: the "isLocal" of the model, else its URL
    function _isLocal(data) {
        if ("isLocal" in data)
            return data["isLocal"] === true
        return /^file:/i.test(String(data["mrl"] ?? ""))
    }

    function _openContainingFolder(dataList, options, indexes) {
        return dataList.length === 1 && _isLocal(dataList[0])
    }

    function _parentUrl(dataList, indexes) {
        if (typeof model.getParentURL === "function")
            return model.getParentURL(indexes[0])
        return MainCtx.folderMRL(String(dataList[0]["mrl"]))
    }

    // Add Subtitle / Audio Tracks: the videos (a media, a folder or a
    // playlist: MLSubtitleAssociations can not open groups, the "Group by" of
    // the videos)
    function _showTracks(dataList) {
        if (!root.showPlayAsAudioAction)
            return false
        return _every(dataList, data => {
            const id = data[root.idDataRole]
            if (!id || !id.hasParent())
                return !!id
            const serialized = id.toString()
            return serialized.startsWith("VLC_ML_PARENT_FOLDER")
                   || serialized.startsWith("VLC_ML_PARENT_PLAYLIST")
        })
    }

    // The dialogs of the tracks, made when first opened
    function _tracksDialog(audio) {
        if (audio) {
            if (!root.audioTracksDialog)
                root.audioTracksDialog = root.subtitleDialogComponent.createObject(g_mainDisplay, { audio: true })
            return root.audioTracksDialog
        }
        if (!root.subtitleDialog)
            root.subtitleDialog = root.subtitleDialogComponent.createObject(g_mainDisplay)
        return root.subtitleDialog
    }

    function _deleteStream(dataList,options, indexes) {
        return _checkRole(dataList, "isDeletable", true)
    }

    function _deleteFileFromSource(dataList, options, indexes) {
        return  _checkRole(dataList, "isDeletableFile", true)
    }

    function _signalShowInformation(dataList, options) {
        const index = options?.["information"] ?? null
        console.assert(Helpers.isInteger(index))
        showMediaInformation(index)
    }

    function _playerOptions(options, extraOptions) {
        const playerOpts = options?.["player-options"] ?? []
        return playerOpts.concat(extraOptions)
    }

    function _hasUrls(dataList) {
        return dataList.length > 0 && dataList.every(data => !!data["mrl"])
    }

    function _urlList(dataList) {
        return dataList.map(data => data["mrl"])
    }

    function _mlIDList(dataList, role = root.idDataRole) {
        const idList = []
        for (let i in dataList) {
            console.assert(dataList[i][role] !== undefined)
            idList.push(dataList[i][role])
        }

        return idList
    }
}
