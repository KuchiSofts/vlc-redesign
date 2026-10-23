/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick

import VLC.MainInterface
import VLC.MediaLibrary

// The models of the library beside the video (CompactLibrary) that exist
// only with the media library: registered with it (mainui.cpp), a view
// naming them would not load without it. The panel loads them while it is
// shown; each reads the library once a list of the panel shows it.
Item {
    id: models

    // The place of the panel ({ section, kind, parentId, ... })
    property var location: ({})
    readonly property string _section: location.section ?? ""
    readonly property bool _home: _section === "home"
    // Figma 848:1996 "Home": three media a section; all of them in the
    // section's own place (its chevron)
    readonly property int _limit: _home ? 3 : 0

    // The media of the library, counted in the header of Home (as the
    // library's Home, Figma 6261:15078)
    readonly property MLMediaModel allMedia: MLMediaModel { ml: MediaLib }

    // The sections of the library's Home (HomePage.qml), in its order: the
    // last played first (its sort, by default; the models' own order is
    // the first played first)
    readonly property MLRecentVideoModel continueWatching: MLRecentVideoModel {
        ml: MediaLib
        sortOrder: Qt.DescendingOrder
        limit: models._limit
    }
    readonly property MLRecentAudioModel listenAgain: MLRecentAudioModel {
        ml: MediaLib
        sortOrder: Qt.DescendingOrder
        limit: models._limit
    }
    readonly property MLFrequentFoldersModel frequentFolders: MLFrequentFoldersModel {
        // (it reads the history as soon as it has the library: only where
        // it is shown)
        ml: models._home || (models._section === "homeall" && models.location.kind === "frequentFolders")
            ? MediaLib : null
        limit: models._limit
    }
    readonly property MLMediaModel favorites: MLMediaModel {
        ml: MediaLib
        favoriteOnly: true
        sortCriteria: "insertion"
        sortOrder: Qt.DescendingOrder
        limit: models._limit
    }
    readonly property MLVideoModel newVideos: MLVideoModel {
        ml: MediaLib
        sortCriteria: "insertion"
        sortOrder: Qt.DescendingOrder
        limit: models._limit
    }
    readonly property MLAudioModel newMusic: MLAudioModel {
        ml: MediaLib
        sortCriteria: "insertion"
        sortOrder: Qt.DescendingOrder
        limit: models._limit
    }

    // The folders of the library (its entry points): the folders of Videos
    // and of Music > Songs (Figma 848:1996: "Shows", "Films", "My Songs")
    readonly property MLFolderModel libraryFolders: MLFolderModel { ctx: MainCtx }
    // the ones holding videos, songs (or nothing yet): Videos, Music > Songs
    readonly property MLFolderModel videoFolders: MLFolderModel {
        ctx: models._section === "videos" ? MainCtx : null
        kind: MLFolderModel.VideoFolders
    }
    readonly property MLFolderModel songFolders: MLFolderModel {
        ctx: models._section === "songs" ? MainCtx : null
        kind: MLFolderModel.AudioFolders
    }
    Instantiator {
        id: libraryFolderEntries
        model: models.libraryFolders
        delegate: QtObject {
            required property var url
        }
    }
    // their urls ("file:///..."): the library folder holding a folder shown
    readonly property var libraryFolderUrls: {
        const urls = []
        for (let i = 0; i < libraryFolderEntries.count; ++i) {
            const entry = libraryFolderEntries.objectAt(i)
            if (entry)
                urls.push(String(entry.url))
        }
        return urls
    }

    // Music > Albums, Artists, Genres, and the songs of one of them
    readonly property MLAlbumModel albums: MLAlbumModel { ml: MediaLib }
    readonly property MLArtistModel artists: MLArtistModel { ml: MediaLib }
    readonly property MLGenreModel genres: MLGenreModel { ml: MediaLib }
    readonly property MLAudioModel tracks: MLAudioModel { ml: MediaLib }
    Binding {
        target: models.tracks
        property: "parentId"
        value: models.location.parentId
        when: models._section === "album" || models._section === "artist" || models._section === "genre"
    }
}
