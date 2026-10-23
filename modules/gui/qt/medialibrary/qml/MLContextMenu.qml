
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

import VLC.Util

// @brief - a generic ML context menu
NativeMenu {
    id: root

    // Properties

    required property MLBaseModel model

    property string idDataRole: "id"
    property string albumIdDataRole: "album_id"

    property bool showPlayAsAudioAction: false

    property var subtitleDialog: null
    property Component subtitleDialogComponent: Component { SubtitleAssociationsDialog {} }
    Component.onDestruction: if (subtitleDialog) subtitleDialog.destroy()

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

    // Figma "file context menu": Play, Mark as Watched, Favorite, Stream,
    // Convert, Add to Queue, Add to Playlist, Show in File Explorer, Go to
    // Source, Add Subtitle Tracks, Properties & Metadata; then the actions
    // of VLC the design does not list
    actions: [{
            "text": qsTr("Play"),
            "icon": "play",
            "action": addAndPlay
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
            "text": qsTr("Mark as Watched"),
            "icon": "mark-watched",
            "action": markSeen,
            "visible": _showSeen
        }, {
            "text": qsTr("Mark as Unwatched"),
            "icon": "mark-watched",
            "action": markUnseen,
            "visible": _showUnseen
        }, {
            "text": qsTr("Favorite"),
            "icon": "favorite",
            "action": addFavorite,
            "visible": _showAddFavorite
        }, {
            "text": qsTr("Remove from Favorites"),
            "icon": "favorite",
            "action": removeFavorite,
            "visible": _showRemoveFavorite
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
            "text": qsTr("Add Album to Queue"),
            "icon": "queue",
            "action": enqueueAsAlbum,
            "visible": root.showPlayAsAlbumAction &&
                       (root.model instanceof MLAudioModel)
        }, {
            "text": qsTr("Add to Playlist"),
            "icon": "playlist",
            "action": addToAPlaylist
        }, {
            "text": (root.showPlayAsAudioAction ? qsTr("Add to a Video-only Playlist") : qsTr("Add to an Audio-only Playlist")),
            "icon": "playlist",
            "action": (root.showPlayAsAudioAction ? root.addToVideoOnlyPlaylist : root.addToAudioOnlyPlaylist)
        }, {
            "text": qsTr("Show in File Explorer"),
            "icon": "folder-open",
            "action": openContainingFolder,
            "visible": _openContainingFolder
        }, {
            "text": qsTr("Go to Source"),
            "icon": "go-to",
            "action": function(dataList, options, indexes) {
                History.push(["folder"], { folderUrl: model.getParentURL(indexes[0]) }, Qt.MouseFocusReason)
            },
            "visible": _openContainingFolder
        }, {
            "text": qsTr("Add Subtitle Tracks"),
            "icon": "subtitles",
            "action": function(dataList) {
                if (!root.subtitleDialog)
                    root.subtitleDialog = root.subtitleDialogComponent.createObject(g_mainDisplay)
                root.subtitleDialog.openFor(dataList[0][root.idDataRole])
            },
            // a media, a folder or a playlist: MLSubtitleAssociations can not
            // open groups (the "Group by" of the videos)
            "visible": function(dataList) {
                if (!root.showPlayAsAudioAction || dataList.length !== 1)
                    return false
                const id = dataList[0][root.idDataRole]
                if (!id || !id.hasParent())
                    return !!id
                const serialized = id.toString()
                return serialized.startsWith("VLC_ML_PARENT_FOLDER")
                       || serialized.startsWith("VLC_ML_PARENT_PLAYLIST")
            }
        }, {
            "text": qsTr("Properties & Metadata"),
            "icon": "info",
            "action": function(dataList, options, indexes) {
                DialogsProvider.mediaInfoDialog(dataList[0][idDataRole])
            },
            "visible": function(dataList, options, indexes) {
                return (dataList.length === 1)
                        && !(dataList[0][idDataRole].hasParent())
            }
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
    ]

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
        model.setMediaIsFavorite(indexes[0], true)
    }

    function removeFavorite(dataList, options, indexes) {
        model.setMediaIsFavorite(indexes[0], false)
    }

    function markSeen(dataList, options, indexes) {
        model.setItemPlayed(indexes[0], true)
    }

    function markUnseen(dataList, options, indexes) {
        model.setItemPlayed(indexes[0], false)
    }

    function openContainingFolder(dataList, options, indexes) {
        const parentDir = model.getParentURL(indexes[0]);

        Qt.openUrlExternally(parentDir)
    }

    function deleteStream(dataList, options, indexes) {
        model.deleteStream(dataList[0][idDataRole])
    }

    function deleteFileFromSource(dataList, options, indexes) {
        // asked even without the confirmations: the file is gone for good
        let confirm = DialogsProvider.questionDialog(qsTr("Are you sure you want to delete this file?"), "", true);
        
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

    function _checkRole(dataList, role, expected) {
        if (dataList.length !== 1)
            return false

        if (!(role in dataList[0]))
            return false

        return (dataList[0][role] === expected)
    }

    function _showAddFavorite(dataList, options, indexes) {
        return _checkRole(dataList, "isFavorite", false)
    }

    function _showRemoveFavorite(dataList, options, indexes) {
        return _checkRole(dataList, "isFavorite", true)
    }

    function _showSeen(dataList, options, indexes) {
        return _checkRole(dataList, "isNew", true)
    }

    function _showUnseen(dataList, options, indexes) {
        return _checkRole(dataList, "isNew", false)
    }

    function _openContainingFolder(dataList, options, indexes) {
        return _checkRole(dataList, "isLocal", true)
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
