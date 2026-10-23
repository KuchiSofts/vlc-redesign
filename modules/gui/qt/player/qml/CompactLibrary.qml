/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl

import VLC.Dialogs
import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Network as Network
import VLC.Playlist
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// Library next to the video (Figma "Player" with the sidebar open, the
// places of Figma 848:1996 "sidebar"): a column of sections and a compact
// list, to pick media without leaving the player.
FocusScope {
    id: root

    // Current place: { section, title, trail (the places above it, as
    // crumbs { title, place }), and its own: folder (url) and root (the
    // library folder it was opened from), playlistId, parentId (album,
    // artist, genre), kind (a section of Home), source (a network service),
    // tree (a network node) }
    property var location: ({ section: "home" })
    property var _back: []
    property var _forward: []
    // The models filtered by the search field (the place it was typed in)
    property var _searchedModels: []
    // Home sorted from the panel (its sections have no order of their own)
    property bool _homeSorted: false
    // The service and the node of the network places: changed when one is
    // entered only, its model kept as it was until it goes
    property string _netSource: ""
    property var _netTree: null

    // The search belongs to the place it was typed in: another place starts
    // without it, and the place left shows all of its items again
    onLocationChanged: {
        root._clearSearch()
        root._updateNetwork()
    }
    // Closed, the models of the panel are gone (and nothing is searched)
    onVisibleChanged: {
        root._clearSearch()
        root._updateNetwork()
    }

    function _clearSearch() {
        for (const model of _searchedModels) {
            if (model && model.searchPattern !== undefined)
                model.searchPattern = ""
        }
        _searchedModels = []
        searchField.visible = false
        searchField.text = ""
    }

    readonly property bool _mlAvailable: MainCtx.mediaLibraryAvailable
    // The folder shown holds the song playing (the paths compared: the same
    // file has URLs encoded differently)
    readonly property bool _songFolder: {
        if (location.section !== "folder" || !Player.isStarted
            || Player.videoTracks.count > 0 || Player.audioTracks.count === 0)
            return false
        const folder = Helpers.localPathFromUrl(location.folder ?? "").toLowerCase()
        const song = Helpers.localPathFromUrl(Player.url).toLowerCase()
        return folder !== "" && song.startsWith(folder.endsWith("\\") ? folder : folder + "\\")
    }
    readonly property var _sections: [
        { section: "home", icon: "home", title: qsTr("Home") },
        { section: "videos", icon: "videos", title: qsTr("Videos") },
        { section: "music", icon: "songs", title: qsTr("Music") },
        { section: "network", icon: "streaming", title: qsTr("Network") },
        { section: "playlists", icon: "playlist", title: qsTr("Playlists") },
        { section: "recent", icon: "recent", title: qsTr("Recent") }
    ]

    // Leave the player for the library window, on the same section
    signal openLibraryRequested(string section)

    // Figma "sidebar": the column of sections (50) and the panel (377)
    implicitWidth: VLCStyle.dp(427, VLCStyle.scale)

    // Text of the design in a box of `box` px: a line of `line` px centred
    // in it (Inter: ascender 0.96875, line 1.2109375 em), the baseline from
    // the top of the box
    function _baseline(box, pixelSize, line, ascent) {
        const l = line !== undefined ? line : Math.round(1.2109375 * pixelSize)
        // 0.3 px higher: where Figma 187:328 draws the title and the rows
        // (measured on their glyphs: 0.26..0.43 low otherwise)
        return Math.round((box - l) / 2) + (l - 1.2109375 * pixelSize) / 2 + 0.96875 * pixelSize - ascent - 0.3 * VLCStyle.scale
    }

    function go(place) {
        _back = _back.concat([location])
        _forward = []
        location = place
    }
    function back() {
        if (_back.length === 0)
            return
        _forward = [location].concat(_forward)
        location = _back[_back.length - 1]
        _back = _back.slice(0, -1)
    }
    function forward() {
        if (_forward.length === 0)
            return
        _back = _back.concat([location])
        location = _forward[0]
        _forward = _forward.slice(1)
    }
    // Folder holding a local media (empty for streams)
    function folderOf(media) {
        return folderModel.folderOf(media)
    }
    function showFolder(url) {
        if (!url || url.toString() === "")
            return
        go({ section: "folder", folder: url })
    }
    // A folder from the folder shown: still under the library folder it
    // was opened from, while it is in it
    function _openFolder(url) {
        if (!url || url.toString() === "")
            return
        const libraryRoot = location.section === "folder" ? location.root : undefined
        go({ section: "folder", folder: url,
             root: libraryRoot && _isPrefix(_partsOf(libraryRoot.folder), _partsOf(url)) ? libraryRoot : undefined })
    }

    // Folders of the path of a folder url ("C:", "Users", ...)
    function _partsOf(url) {
        let path = String(url ?? "").replace(/^file:\/\/\/?/, "")
        try {
            path = decodeURIComponent(path)
        } catch (e) {
            // (not encoded: kept as it is)
        }
        return path.split("/").filter(part => part.length > 0)
    }
    // The folders `parts` begin the path `path` (a folder holds the other)
    function _isPrefix(parts, path) {
        return parts.length > 0 && parts.length <= path.length
               && parts.every((part, i) => part.toLowerCase() === path[i].toLowerCase())
    }
    // Name of a folder url (its last folder)
    function _nameOf(url) {
        const parts = _partsOf(url)
        return parts.length > 0 ? parts[parts.length - 1] : String(url ?? "")
    }
    function _icon(name) {
        return "qrc:///redesign/icons/" + name + "/default.png"
    }

    // Folders of the path of the current folder
    readonly property var _folderParts: location.section === "folder" ? _partsOf(folderModel.folder) : []

    // Folder of the first `depth` parts of the path
    function _folderAt(depth) {
        const path = _folderParts.slice(0, depth)
                                 .map((part, i) => i === 0 && /:$/.test(part) ? part : encodeURIComponent(part))
                                 .join("/")
        return "file:///" + path + (depth === 1 && /:$/.test(path) ? "/" : "")
    }

    function _isVideoFile(url) {
        return /\.(3gp|asf|avi|flv|m2ts|m4v|mkv|mov|mp4|mpe?g|mts|ogv|ts|vob|webm|wmv)$/i.test(String(url))
    }
    function _isAudioFile(url) {
        return /\.(aac|aiff?|alac|ape|flac|m4a|mka|mp3|oga|ogg|opus|wav|wma)$/i.test(String(url))
    }
    // What the folder shown holds: videos, songs
    readonly property var _folderContent: {
        const content = { video: false, audio: false }
        if (location.section !== "folder")
            return content
        for (let i = 0; i < folderModel.count; ++i) {
            const url = folderModel.data(folderModel.index(i, 0), LocalFolderModel.UrlRole)
            if (_isVideoFile(url))
                content.video = true
            else if (_isAudioFile(url))
                content.audio = true
        }
        return content
    }

    // The crumbs of the sections, above their places
    readonly property var _musicCrumb: ({ title: qsTr("Music"), place: { section: "music" } })
    readonly property var _videosTrail: [{ title: qsTr("Videos"), place: { section: "videos" } }]
    readonly property var _songsTrail: [_musicCrumb, { title: qsTr("Songs"),
                                                      place: { section: "songs", title: qsTr("Songs"),
                                                               trail: [_musicCrumb] } }]

    // The library folder holding the folder shown, and the section it is
    // in: the one it was opened from, else the one holding it (Music >
    // Songs for songs, Videos otherwise)
    readonly property var _folderRoot: {
        if (location.section !== "folder")
            return null
        if (location.root)
            return location.root
        const urls = mlModels.item ? mlModels.item.libraryFolderUrls : []
        let best = ""
        let bestLength = 0
        for (const url of urls) {
            const parts = _partsOf(url)
            if (parts.length > bestLength && _isPrefix(parts, _folderParts)) {
                best = url
                bestLength = parts.length
            }
        }
        if (best === "")
            return null
        const songs = _songFolder || (_folderContent.audio && !_folderContent.video)
        return { section: songs ? "songs" : "videos", folder: best, trail: songs ? _songsTrail : _videosTrail }
    }
    // The section of the column the place is in
    readonly property string _columnSection: {
        switch (location.section) {
        case "homeall":
            return "home"
        case "songs":
        case "albums":
        case "artists":
        case "genres":
        case "album":
        case "artist":
        case "genre":
            return "music"
        case "service":
        case "netfolder":
            return "network"
        case "playlist":
            return "playlists"
        case "folder":
            // Figma 2160:8732: a folder shown, the Videos icon filled; the
            // folder of the song playing, the Music one (4778:25123)
            if (_folderRoot)
                return _folderRoot.section === "songs" ? "music" : "videos"
            return _songFolder ? "music" : "videos"
        default:
            return location.section
        }
    }

    // The address of the place (Figma 848:1996: "Videos > Shows", "... >
    // Shows > The Wand", "Music > Songs", "... > Songs > My Songs"): crumbs
    // { title, place }, the place shown last
    readonly property var _folderCrumbs: {
        const parts = _folderParts
        const libraryRoot = _folderRoot
        const rootParts = libraryRoot ? _partsOf(libraryRoot.folder) : []
        const inRoot = libraryRoot !== null && _isPrefix(rootParts, parts)
        // (under the library folder, from it; else the whole path)
        const crumbs = inRoot ? libraryRoot.trail.slice() : []
        for (let depth = inRoot ? rootParts.length : 1; depth <= parts.length; ++depth) {
            crumbs.push({ title: parts[depth - 1],
                          place: { section: "folder", folder: _folderAt(depth),
                                   root: inRoot ? libraryRoot : undefined } })
        }
        if (crumbs.length > 0 && folderModel.folderName !== "")
            crumbs[crumbs.length - 1].title = folderModel.folderName
        return crumbs
    }
    readonly property var _crumbs: {
        if (location.section === "folder")
            return _folderCrumbs
        const entry = _sections.find(s => s.section === location.section)
        const own = { title: location.title ?? (entry ? entry.title : ""), place: location }
        return location.trail ? location.trail.concat([own]) : [own]
    }

    // The models of the place: the media library's (loaded with it, while
    // the panel is shown) and the network's (VLC.Network, as the library's
    // network pages: they always exist)
    Loader {
        id: mlModels
        active: root.visible && root._mlAvailable
        source: "qrc:///qt/qml/VLC/Player/CompactLibraryModels.qml"
        onLoaded: root._homeSorted = false
    }
    Binding {
        target: mlModels.item
        property: "location"
        // (what they use of the place: not its crumbs)
        value: ({ section: root.location.section, kind: root.location.kind, parentId: root.location.parentId })
        when: mlModels.status === Loader.Ready
    }

    Loader {
        id: networkModels
    }
    // (set here, after the service or the node of the place)
    function _updateNetwork() {
        const section = location.section
        if (section === "service")
            _netSource = location.source ?? ""
        else if (section === "netfolder")
            _netTree = location.tree ?? null
        networkModels.sourceComponent = !root.visible ? null
                                      : section === "network" ? networkSourcesComponent
                                      : section === "service" ? networkServiceComponent
                                      : section === "netfolder" ? networkNodeComponent
                                      : null
    }
    // Figma 848:1996 "Network": the services of the Internet (Podcasts,
    // Radio), as the library's Discover > Services
    Component {
        id: networkSourcesComponent
        Item {
            readonly property Network.NetworkSourcesModel model: Network.NetworkSourcesModel { ctx: MainCtx }
        }
    }
    Component {
        id: networkServiceComponent
        Item {
            readonly property Network.NetworkDeviceModel model: Network.NetworkDeviceModel {
                ctx: MainCtx
                sd_source: Network.NetworkDeviceModel.CAT_INTERNET
                source_name: root._netSource
            }
        }
    }
    Component {
        id: networkNodeComponent
        Item {
            readonly property Network.NetworkMediaModel model: Network.NetworkMediaModel {
                ctx: MainCtx
                tree: root._netTree
            }
        }
    }

    // Model and row presentation of the current place (Home: its sections)
    readonly property var _model: {
        const ml = mlModels.item
        switch (location.section) {
        case "folder": return folderModel
        case "videos": return ml ? ml.videoFolders : null
        case "songs": return ml ? ml.songFolders : null
        case "music": return _mlAvailable ? musicCategories : null
        case "albums": return ml ? ml.albums : null
        case "artists": return ml ? ml.artists : null
        case "genres": return ml ? ml.genres : null
        case "album":
        case "artist":
        case "genre": return ml ? ml.tracks : null
        case "homeall": return ml ? (ml[location.kind] ?? null) : null
        case "network":
        case "service":
        case "netfolder": return networkModels.item ? networkModels.item.model : null
        case "playlists": return _mlAvailable ? playlistListModel : null
        case "playlist": return _mlAvailable ? playlistModel : null
        case "recent": return _mlAvailable ? recentModel : null
        default: return null
        }
    }
    // Home: its sections, as the library's Home (HomePage.qml)
    readonly property var _homeSections: {
        const ml = mlModels.item
        if (!ml)
            return []
        return [
            { kind: "continueWatching", title: qsTr("Continue Watching"), model: ml.continueWatching },
            { kind: "listenAgain", title: qsTr("Listen Again"), model: ml.listenAgain },
            { kind: "frequentFolders", title: qsTr("Frequently Visited Folders"), model: ml.frequentFolders },
            { kind: "favorites", title: qsTr("Favorites"), model: ml.favorites },
            { kind: "newVideos", title: qsTr("New Videos"), model: ml.newVideos },
            { kind: "newMusic", title: qsTr("New Music"), model: ml.newMusic }
        ]
    }
    // The models Home sorts and searches (its folders have neither)
    function _homeModels() {
        const ml = mlModels.item
        return ml ? [ml.continueWatching, ml.listenAgain, ml.favorites, ml.newVideos, ml.newMusic] : []
    }
    // The models the sort and the search of the tool row act on
    readonly property var _toolModels: location.section === "home" ? _homeModels()
                                                                  : (_model ? [_model] : [])

    // Items counted in the tool row (Figma: "3 items" under Add; Home, the
    // media of the library)
    readonly property int _count: {
        switch (location.section) {
        case "home":
            return mlModels.item ? mlModels.item.allMedia.count : 0
        case "network":
            // (the "Add a service" row of the sources, gone while searching)
            return Math.max(0, list.count - (searchField.text === "" ? 1 : 0))
        default:
            return list.count
        }
    }
    readonly property bool _counted: location.section === "home" ? !!mlModels.item : !!_model

    // The Add row at the top of the list (Figma 848:1996): a folder to the
    // library, or media to a folder (the Open media menu of the library's
    // folders), in Videos, Music and their folders; not in a folder of
    // videos (2425:13624); a playlist in Playlists
    readonly property bool _addShown: {
        switch (location.section) {
        case "playlists":
        case "videos":
        case "songs":
        case "music":
            return _mlAvailable
        case "folder":
            return folderModel.count > 0 && !_folderContent.video
        default:
            return false
        }
    }

    // The Open media menu of Add, under its row
    OpenMediaButton {
        id: addMediaButton
        // (the list is in the panel, right of the column)
        x: panel.x + list.x + list.leftMargin
        y: panel.y + list.y + list.topMargin
        width: VLCStyle.dp(32, VLCStyle.scale)
        height: width
        // (under the list: the row takes the clicks)
        z: -1
        opacity: 0
        enabled: root._addShown && root.location.section !== "playlists"
        iconOnly: true
        libraryFolder: true
    }

    LocalFolderModel {
        id: folderModel
        folder: root.location.section === "folder" ? root.location.folder : ""
        ml: root._mlAvailable ? MainCtx.mediaLibrary : null
        // Figma: unwatched first, as the folder pages
        sortCriteria: "unwatched"
        sortOrder: Qt.AscendingOrder
    }

    MLPlaylistListModel {
        id: playlistListModel
        ml: root._mlAvailable ? MediaLib : null
        // in the order of the Playlists page of the library (its saved sort)
        Component.onCompleted: {
            const criteria = MainCtx.settingValue("sortCriteria/video/playlists/all", undefined)
            if (criteria !== undefined && String(criteria).length > 0)
                sortCriteria = String(criteria)
            const order = MainCtx.settingValue("sortOrder/video/playlists/all", undefined)
            if (order !== undefined)
                sortOrder = parseInt(order)
        }
    }
    MLPlaylistModel { id: playlistModel; ml: root._mlAvailable ? MediaLib : null }
    MLRecentMediaModel { id: recentModel; ml: root._mlAvailable ? MediaLib : null }

    Binding {
        target: playlistModel
        property: "parentId"
        value: root.location.playlistId
        when: root.location.section === "playlist"
    }

    // Figma 848:1996 "Music": Songs (the library's folders, "My Songs"),
    // Albums, Artists, Genres
    ListModel {
        id: musicCategories
        ListElement { place: "songs"; name: qsTr("Songs"); icon: "music" }
        ListElement { place: "albums"; name: qsTr("Albums"); icon: "album" }
        ListElement { place: "artists"; name: qsTr("Artists"); icon: "artist" }
        ListElement { place: "genres"; name: qsTr("Genres"); icon: "genre" }
    }

    function _activate(entry, index) {
        const place = root.location
        switch (place.section) {
        case "folder":
            if (entry.isFolder)
                root._openFolder(entry.url)
            else
                MainPlaylistController.append([entry.url], true)
            break
        case "videos":
        case "songs":
            // a folder of the library, browsed under the section
            root.go({ section: "folder", folder: entry.url,
                      root: { section: place.section, folder: String(entry.url), trail: root._crumbs } })
            break
        case "music":
            root.go({ section: entry.place, title: entry.name, trail: root._crumbs })
            break
        case "albums":
        case "artists":
        case "genres":
            root.go({ section: place.section.slice(0, -1), parentId: entry.id,
                      title: entry.name ?? entry.title ?? "", trail: root._crumbs })
            break
        case "network":
            if (entry.type === Network.NetworkSourcesModel.TYPE_DUMMY)
                DialogsProvider.openNetDialog()
            else
                root.go({ section: "service", source: entry.name, title: entry.long_name, trail: root._crumbs })
            break
        case "service":
        case "netfolder":
            if (root._isNetworkContainer(entry.type))
                root.go({ section: "netfolder", tree: entry.tree, title: entry.name, trail: root._crumbs })
            else if (root._model)
                root._model.addAndPlay(index)
            break
        case "playlists":
            root.go({ section: "playlist", playlistId: entry.id, title: entry.name, trail: root._crumbs })
            break
        case "homeall":
            if (place.kind === "frequentFolders")
                root.showFolder(entry.url)
            else
                MediaLib.addAndPlay([entry.id])
            break
        default:
            MediaLib.addAndPlay([entry.id])
        }
    }
    // A node of a network source that opens (a directory, a feed, a list)
    function _isNetworkContainer(type) {
        return type === Network.NetworkMediaModel.TYPE_DIRECTORY
               || type === Network.NetworkMediaModel.TYPE_NODE
               || type === Network.NetworkMediaModel.TYPE_PLAYLIST
    }

    // Section column
    Rectangle {
        id: sectionColumn

        width: VLCStyle.dp(50, VLCStyle.scale)
        height: parent.height
        color: VLCStyle.redesign.color(VLCStyle.redesign.sidebarBackground, "#ababaa")

        Column {
            y: VLCStyle.dp(8, VLCStyle.scale)
            width: parent.width
            spacing: VLCStyle.dp(14, VLCStyle.scale)

            Repeater {
                model: root._sections
                delegate: SectionButton {
                    required property var modelData
                    readonly property bool current: root.location.section === modelData.section
                                                    || (modelData.section === "playlists"
                                                        && root.location.section === "playlist")
                    iconName: modelData.icon
                    // Figma 848:1996: a section's own place, its icon in the
                    // accent; the places in it (a folder, Songs, a
                    // service...), the icon filled (grey)
                    iconState: current && modelData.icon !== "recent" ? "active"
                             : (!current && root._columnSection === modelData.section
                                && modelData.section !== "recent" && modelData.section !== "playlists") ? "selected"
                             : "default"
                    description: modelData.title
                    onClicked: {
                        if (!current)
                            root.go({ section: modelData.section })
                    }
                }
            }

            SectionButton {
                iconName: "add"
                description: qsTr("Open media")
                onClicked: DialogsProvider.simpleOpenDialog()
            }
        }
    }

    // List panel
    Rectangle {
        id: panel

        anchors.left: sectionColumn.right
        anchors.right: parent.right
        height: parent.height
        // #0E0E0E at 95 % over the video
        color: VLCStyle.redesign.color("#0F0F0F", "#c2c2c1")

        // Header: history, title, menu and the full library
        RowLayout {
            id: header

            x: VLCStyle.dp(20, VLCStyle.scale)
            y: VLCStyle.dp(20, VLCStyle.scale)
            width: parent.width - 2 * x
            height: VLCStyle.dp(24, VLCStyle.scale)
            spacing: 0

            PanelButton {
                iconSource: "qrc:///redesign/arrow-left.svg"
                description: qsTr("Previous")
                enabled: root._back.length > 0
                onClicked: root.back()
            }
            PanelButton {
                Layout.leftMargin: VLCStyle.dp(12, VLCStyle.scale)
                iconSource: "qrc:///redesign/arrow-right.svg"
                description: qsTr("Next")
                enabled: root._forward.length > 0
                onClicked: root.forward()
            }
            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: VLCStyle.dp(10, VLCStyle.scale)
                visible: root._crumbs.length < 2
                text: root._crumbs.length > 0 ? root._crumbs[0].title : ""
                elide: Text.ElideRight
                color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#4d4d4d")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                // a line of 100 % centred in the 24 px of the row
                id: titleText
                verticalAlignment: Text.AlignTop
                topPadding: root._baseline(height, font.pixelSize, font.pixelSize, titleMetrics.ascent)
                FontMetrics {
                    id: titleMetrics
                    font: titleText.font
                }
            }

            // Address line of the places in a section: "... > parent > place"
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: VLCStyle.dp(10, VLCStyle.scale)
                visible: root._crumbs.length >= 2
                spacing: VLCStyle.dp(6, VLCStyle.scale)
                clip: true

                CrumbButton {
                    readonly property var crumbData: root._crumbs.length > 2 ? root._crumbs[root._crumbs.length - 3] : null
                    visible: !!crumbData
                    dimmed: true
                    text: "\u2026"
                    description: crumbData ? crumbData.title : ""
                    onClicked: root.go(crumbData.place)
                }
                CrumbSeparator { visible: root._crumbs.length > 2 }
                CrumbButton {
                    readonly property var crumbData: root._crumbs.length >= 2 ? root._crumbs[root._crumbs.length - 2] : null
                    // Gives way to the current place first
                    Layout.minimumWidth: Math.min(implicitWidth, VLCStyle.dp(40, VLCStyle.scale))
                    Layout.maximumWidth: implicitWidth
                    text: crumbData ? crumbData.title : ""
                    description: text
                    onClicked: root.go(crumbData.place)
                }
                CrumbSeparator { }
                Text {
                    Layout.minimumWidth: Math.min(implicitWidth, VLCStyle.dp(130, VLCStyle.scale))
                    Layout.maximumWidth: implicitWidth
                    text: root._crumbs.length > 0 ? root._crumbs[root._crumbs.length - 1].title : ""
                    elide: Text.ElideRight
                    // Dimmed: the parents are the ones to click
                    color: VLCStyle.palette.isDark ? VLCStyle.redesign.textMuted : "#8f8f8f"
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                }
                Item { Layout.fillWidth: true }
            }
            PanelButton {
                Layout.leftMargin: VLCStyle.dp(10, VLCStyle.scale)
                iconSource: "qrc:///redesign/icons/more/dim.png"
                description: qsTr("More")
                // playlists are created in the media library only; the
                // songs of an album, an artist, a genre or a playlist are
                // played together
                // (Figma 848:1996 draws it on every place: the sections add
                // to the library and rescan it)
                visible: ["folder", "home", "videos", "music", "songs", "network"].includes(root.location.section)
                         || (root._mlAvailable && ["playlists", "playlist", "album", "artist", "genre"]
                                                      .includes(root.location.section))
                onClicked: {
                    switch (root.location.section) {
                    case "folder": folderMenu.toggleFrom(this); break
                    case "playlists": playlistsMenu.toggleFrom(this); break
                    case "home": homeMenu.toggleFrom(this); break
                    case "videos":
                    case "music":
                    case "songs": foldersMenu.toggleFrom(this); break
                    case "network": networkMenu.toggleFrom(this); break
                    default: collectionMenu.toggleFrom(this)
                    }
                }

                Widgets.RedesignMenu {
                    id: homeMenu
                    Widgets.RedesignMenuItem {
                        text: qsTr("Rescan library")
                        iconName: "refresh"
                        enabled: root._mlAvailable
                        onTriggered: MediaLib.reload()
                    }
                }

                Widgets.RedesignMenu {
                    id: foldersMenu
                    Widgets.RedesignMenuItem {
                        text: qsTr("Add folder to library…")
                        iconName: "add"
                        onTriggered: addMediaButton.clicked()
                    }
                    Widgets.RedesignMenuItem {
                        text: qsTr("Rescan library")
                        iconName: "refresh"
                        enabled: root._mlAvailable
                        onTriggered: MediaLib.reload()
                    }
                }

                Widgets.RedesignMenu {
                    id: networkMenu
                    Widgets.RedesignMenuItem {
                        text: qsTr("Add network stream…")
                        iconName: "add"
                        onTriggered: DialogsProvider.openNetDialog()
                    }
                }

                Widgets.RedesignMenu {
                    id: playlistsMenu
                    Widgets.RedesignMenuItem {
                        text: qsTr("Create playlist")
                        iconName: "add"
                        onTriggered: root._createPlaylist()
                    }
                }

                Widgets.RedesignMenu {
                    id: collectionMenu
                    Widgets.RedesignMenuItem {
                        text: qsTr("Play all")
                        iconName: "play"
                        onTriggered: root._playCollection(false)
                    }
                    Widgets.RedesignMenuItem {
                        text: qsTr("Add all to queue")
                        iconName: "queue"
                        onTriggered: root._playCollection(true)
                    }
                }

                Widgets.RedesignMenu {
                    id: folderMenu
                    Widgets.RedesignMenuItem {
                        text: qsTr("Play all")
                        iconName: "play"
                        onTriggered: root._playFolder(false)
                    }
                    Widgets.RedesignMenuItem {
                        text: qsTr("Add all to queue")
                        iconName: "queue"
                        onTriggered: root._playFolder(true)
                    }
                    Widgets.RedesignMenuItem {
                        text: qsTr("Up one folder")
                        iconName: "folder"
                        enabled: folderModel.parentFolder.toString() !== ""
                        onTriggered: root._openFolder(folderModel.parentFolder)
                    }
                }
            }
            PanelButton {
                Layout.leftMargin: VLCStyle.dp(12, VLCStyle.scale)
                iconSource: "qrc:///redesign/icons/open/dim.png"
                description: qsTr("Open in the library")
                onClicked: root.openLibraryRequested(root.location.section)
            }
        }

        // Count, sorting, search
        RowLayout {
            id: toolRow

            anchors.top: header.bottom
            anchors.topMargin: VLCStyle.dp(20, VLCStyle.scale)
            x: VLCStyle.dp(20, VLCStyle.scale)
            width: parent.width - 2 * x
            height: VLCStyle.dp(24, VLCStyle.scale)
            spacing: 0

            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: !searchField.visible
                text: root._counted ? qsTr("%1 item", "%1 items", root._count).arg(root._count) : ""
                color: VLCStyle.redesign.color("#6B6B6B", "#7c7c7b")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.redesign.fontNormal
                // the text box (17 px) centred in the 24 px of the row
                verticalAlignment: Text.AlignTop
                topPadding: (height - VLCStyle.dp(17, VLCStyle.scale)) / 2
                            + (VLCStyle.dp(17, VLCStyle.scale) - 1.2109375 * font.pixelSize) / 2
                            + 0.96875 * font.pixelSize - countMetrics.ascent
                id: countText
                FontMetrics {
                    id: countMetrics
                    font: countText.font
                }
            }
            T.TextField {
                id: searchField

                Layout.fillWidth: true
                visible: false
                color: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.redesign.fontNormal
                placeholderText: qsTr("Search")
                placeholderTextColor: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#7c7c7b")
                onTextChanged: {
                    const models = root._toolModels.filter(model => model && model.searchPattern !== undefined)
                    for (const model of models)
                        model.searchPattern = text
                    root._searchedModels = text !== "" ? models : []
                }
                Keys.onEscapePressed: { text = ""; visible = false }
            }
            PanelButton {
                Layout.leftMargin: VLCStyle.dp(12, VLCStyle.scale)
                iconSource: "qrc:///redesign/icons/sort/dim.png"
                description: qsTr("Sort")
                enabled: root._toolModels.some(model => model && model.sortCriteria !== undefined)
                onClicked: sortMenu.toggleFrom(this)

                // Figma: an order applied, the accent dot at the corner (not
                // for the order of the place: folders list the unwatched
                // media first, the new media of Home the newest first)
                Rectangle {
                    x: VLCStyle.dp(22, VLCStyle.scale)
                    y: -VLCStyle.dp(1, VLCStyle.scale)
                    width: VLCStyle.dp(4, VLCStyle.scale)
                    height: width
                    radius: width / 2
                    color: VLCStyle.redesign.accent
                    visible: {
                        if (root.location.section === "home")
                            return root._homeSorted
                        if (!root._model || !root._model.sortCriteria)
                            return false
                        if (root._model === folderModel)
                            return folderModel.sortCriteria !== "unwatched"
                                   || folderModel.sortOrder !== Qt.AscendingOrder
                        if (root.location.section === "homeall"
                            && ["favorites", "newVideos", "newMusic"].includes(root.location.kind))
                            return root._model.sortCriteria !== "insertion"
                                   || root._model.sortOrder !== Qt.DescendingOrder
                        return true
                    }
                }

                Widgets.RedesignMenu {
                    id: sortMenu
                    Widgets.RedesignMenuItem {
                        text: qsTr("Title A-Z")
                        onTriggered: root._sort("title", Qt.AscendingOrder)
                    }
                    Widgets.RedesignMenuItem {
                        text: qsTr("Title Z-A")
                        onTriggered: root._sort("title", Qt.DescendingOrder)
                    }
                    Widgets.RedesignMenuItem {
                        text: qsTr("Newest First")
                        enabled: root._dateCriteria() !== ""
                        onTriggered: root._sort("date", Qt.DescendingOrder)
                    }
                    Widgets.RedesignMenuItem {
                        text: qsTr("Oldest First")
                        enabled: root._dateCriteria() !== ""
                        onTriggered: root._sort("date", Qt.AscendingOrder)
                    }
                }
            }
            // Figma "View Options": the list here, the grid in the library
            PanelButton {
                Layout.leftMargin: VLCStyle.dp(12, VLCStyle.scale)
                iconSource: "qrc:///redesign/icons/list/dim.png"
                description: qsTr("View")
                enabled: root._counted
                onClicked: viewMenu.toggleFrom(this)

                // (the options of the library header: the list, the only
                // view here, stays checked: not checkable, a click keeps it)
                Widgets.RedesignMenu {
                    id: viewMenu
                    Widgets.RedesignOptionItem {
                        text: qsTr("Grid View")
                        iconSource: "qrc:///redesign/icons/grid/default.png"
                        onTriggered: root.openLibraryRequested(root.location.section)
                    }
                    Widgets.RedesignOptionItem {
                        text: qsTr("List View")
                        iconSource: "qrc:///redesign/icons/list/default.png"
                        checked: true
                    }
                }
            }
            PanelButton {
                Layout.leftMargin: VLCStyle.dp(12, VLCStyle.scale)
                iconSource: "qrc:///redesign/icons/search/dim.png"
                description: qsTr("Search")
                enabled: root._toolModels.some(model => model && model.searchPattern !== undefined)
                onClicked: {
                    searchField.visible = !searchField.visible
                    if (searchField.visible)
                        searchField.forceActiveFocus()
                    else
                        searchField.text = ""
                }
            }
        }

        Rectangle {
            id: separator

            // Frame 54: 17 px under the header, a line of 1 px (#222222
            // drawn #1B1B1B on the panel)
            anchors.top: toolRow.bottom
            anchors.topMargin: VLCStyle.dp(16, VLCStyle.scale)
            x: VLCStyle.dp(20, VLCStyle.scale)
            width: parent.width - 2 * x
            // Figma "Player - Light": two rows, #7D7D7D and #636363
            height: VLCStyle.dp(VLCStyle.palette.isDark ? 1 : 2, VLCStyle.scale)
            color: VLCStyle.palette.isDark ? "#1B1B1B" : "#636363"
            Rectangle {
                visible: !VLCStyle.palette.isDark
                width: parent.width
                height: VLCStyle.dp(1, VLCStyle.scale)
                color: "#7d7d7d"
            }
        }

        // Figma 848:1996 "Home": the sections of the library's Home, a row
        // of 32 for the name and the chevron, 15, three media (rows of 32,
        // 10 apart), 30 to the next one
        ListView {
            id: homeList

            anchors {
                top: separator.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            topMargin: VLCStyle.dp(20, VLCStyle.scale)
            bottomMargin: VLCStyle.dp(20, VLCStyle.scale)
            leftMargin: VLCStyle.dp(20, VLCStyle.scale)
            rightMargin: VLCStyle.dp(20, VLCStyle.scale)
            spacing: 0
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            visible: root.location.section === "home"
            model: visible ? root._homeSections : []

            delegate: Column {
                id: homeSection

                required property var modelData
                readonly property var sectionModel: modelData.model
                // (a section without media is left out, as in the library's
                // Home; the folders while searching: they have no search)
                readonly property bool shown: !!sectionModel && sectionModel.count > 0
                                              && (modelData.kind !== "frequentFolders" || searchField.text === "")

                width: homeList.width - homeList.leftMargin - homeList.rightMargin
                height: shown ? implicitHeight : 0
                visible: shown
                spacing: 0

                Item {
                    width: parent.width
                    height: VLCStyle.dp(32, VLCStyle.scale)

                    // Inter 16 #969696
                    Text {
                        id: sectionTitle
                        width: parent.width - chevron.width - VLCStyle.dp(10, VLCStyle.scale)
                        height: parent.height
                        text: homeSection.modelData.title
                        elide: Text.ElideRight
                        color: VLCStyle.redesign.color(VLCStyle.redesign.textSecondary, "#4d4d4d")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                        verticalAlignment: Text.AlignTop
                        topPadding: root._baseline(height, font.pixelSize, undefined, sectionMetrics.ascent)
                        FontMetrics {
                            id: sectionMetrics
                            font: sectionTitle.font
                        }
                    }
                    // all of the section, in its own place
                    PanelButton {
                        id: chevron
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        width: VLCStyle.dp(24, VLCStyle.scale)
                        height: VLCStyle.dp(24, VLCStyle.scale)
                        iconSource: "qrc:///redesign/icons/chevron-right/default.png"
                        description: qsTr("See all")
                        onClicked: root.go({ section: "homeall", kind: homeSection.modelData.kind,
                                             title: homeSection.modelData.title, trail: root._crumbs })
                    }
                }
                Item {
                    width: 1
                    height: VLCStyle.dp(15, VLCStyle.scale)
                }
                Column {
                    width: parent.width
                    spacing: VLCStyle.dp(10, VLCStyle.scale)

                    Repeater {
                        // (three at most: the models' limit)
                        model: homeSection.sectionModel

                        delegate: LibraryRow {
                            id: homeRow

                            required property var model
                            required property int index

                            readonly property bool folder: homeSection.modelData.kind === "frequentFolders"
                            readonly property bool song: !!model.main_artist || model.isVideo === false
                            readonly property bool watched: !folder && !song
                                                            && (Number(model.playcount ?? 0) > 0 || progress > 0)

                            width: parent.width
                            // videos by their file, as the library's Home
                            // names them (Figma 848:1996)
                            label: folder ? (model.name ?? "")
                                          : (model.main_artist ? (model.title ?? "") + " - " + model.main_artist
                                                               : (model.fileName || model.title || model.name || ""))
                            cover: folder ? "" : (model.thumbnail ?? model.cover ?? model.small_cover ?? "")
                            iconSource: folder ? root._icon("folder")
                                               : (song ? "qrc:///redesign/music.svg" : "qrc:///redesign/video.svg")
                            smallIcon: folder
                            progress: folder ? 0 : Math.max(0, Number(model.progress ?? 0))
                            progressShown: !folder && !song && (progress > 0 || watched)
                            // Figma 848:1996: the folders, the media watched
                            // and the songs listened again dimmed
                            dimmed: folder || watched || homeSection.modelData.kind === "listenAgain"

                            onClicked: if (folder) root.showFolder(model.url)
                            onDoubleClicked: if (!folder) MediaLib.addAndPlay([model.id])
                        }
                    }
                }
                Item {
                    width: 1
                    height: VLCStyle.dp(30, VLCStyle.scale)
                }
            }
        }

        ListView {
            id: list

            // Frame 53: padding 20, rows of 32, 10 px apart
            anchors {
                top: separator.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            topMargin: VLCStyle.dp(20, VLCStyle.scale)
            bottomMargin: VLCStyle.dp(20, VLCStyle.scale)
            leftMargin: VLCStyle.dp(20, VLCStyle.scale)
            rightMargin: VLCStyle.dp(20, VLCStyle.scale)
            spacing: VLCStyle.dp(10, VLCStyle.scale)
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            visible: root.location.section !== "home"
            model: root._model

            header: Loader {
                width: list.width - list.leftMargin - list.rightMargin
                active: root._addShown
                // a row of 32 and the gap of the list
                height: active ? VLCStyle.dp(42, VLCStyle.scale) : 0
                sourceComponent: Item {
                    LibraryRow {
                        width: parent.width
                        // the icon of the design at 32 px (Figma "Player" 187:328)
                        iconSource: "qrc:///redesign/player/library-add.png"
                        label: root.location.section === "playlists" ? qsTr("Create playlist") : qsTr("Add")
                        dimmed: true
                        onClicked: {
                            if (root.location.section === "playlists")
                                root._createPlaylist()
                            else
                                addMediaButton.clicked()
                        }
                    }
                }
            }

            delegate: LibraryRow {
                id: row

                required property var model
                required property int index

                readonly property string place: root.location.section
                // Figma 848:1996 "Network": the sources begin with "Add a
                // service", the design's "Add Network Stream"
                readonly property bool addRow: place === "network"
                                               && model.type === Network.NetworkSourcesModel.TYPE_DUMMY
                // Rows that open a place (on a click; media play on a
                // double click)
                readonly property bool container: {
                    switch (place) {
                    case "folder":
                        return !!model.isFolder
                    case "service":
                    case "netfolder":
                        return root._isNetworkContainer(model.type)
                    case "homeall":
                        return root.location.kind === "frequentFolders"
                    case "videos":
                    case "songs":
                    case "music":
                    case "albums":
                    case "artists":
                    case "genres":
                    case "playlists":
                        return true
                    case "network":
                        return !addRow
                    default:
                        return false
                    }
                }
                readonly property bool media: !container && !addRow
                // (not a song: one is played again, Figma 4778:25123 shows
                // its songs of Listen Again as the others)
                readonly property bool song: !!model.main_artist || model.isVideo === false
                readonly property bool watched: media && !song
                                                && (Number(model.playcount ?? 0) > 0 || progress > 0)

                // Figma "playlist" / "media": 337 x 32, an icon of 32, 10 px
                width: list.width - list.leftMargin - list.rightMargin
                // Songs read "title - artist" as in the design
                label: {
                    if (addRow)
                        return qsTr("Add Network Stream")
                    switch (place) {
                    case "folder":
                        // a song of a folder the library knows, as well
                        // (Figma 4778:25123); the videos keep their file name
                        return model.title && model.main_artist ? model.title + " - " + model.main_artist
                                                                : (model.name ?? "")
                    case "videos":
                    case "songs":
                        return root._nameOf(model.url)
                    case "network":
                        return model.long_name ?? model.name ?? ""
                    default: {
                        // (videos by their file, as Home names them)
                        const name = model.name ?? (model.main_artist ? model.title
                                                                      : (model.fileName || model.title)) ?? ""
                        return model.main_artist ? name + " - " + model.main_artist : name
                    }
                    }
                }
                // Videos of a folder show a frame, decoded on demand, songs
                // the artwork of the library
                cover: {
                    switch (place) {
                    case "folder":
                        if (model.isFolder)
                            return ""
                        // (the MRL of the library: the url as text decodes
                        // some characters)
                        if (root._isVideoFile(model.url))
                            return VLCThumbnail.uri(String(model.mrl), 0.1)
                        return VLCThumbnail.artwork(String(model.mrl))
                    case "service":
                    case "netfolder": {
                        // the artwork of a feed or a station (not the icons
                        // of the sources)
                        const artwork = String(model.artwork ?? "")
                        return /^qrc:/.test(artwork) ? "" : artwork
                    }
                    // the icons of the design (Figma "playlist", "folder")
                    case "videos":
                    case "songs":
                    case "music":
                    case "network":
                    case "genres":
                    case "playlists":
                        return ""
                    default:
                        return model.thumbnail ?? model.cover ?? model.small_cover ?? ""
                    }
                }
                // Until the cover is ready, or without one: the icons of the
                // design (24, Figma 848:1996), the placeholders drawn at 32
                iconSource: {
                    if (addRow)
                        return "qrc:///redesign/player/library-add.png"
                    switch (place) {
                    case "videos":
                    case "songs":
                        return root._icon("folder")
                    case "music":
                        return root._icon(model.icon)
                    case "albums":
                        return root._icon("album")
                    case "artists":
                        return root._icon("artist")
                    case "genres":
                        return root._icon("genre")
                    case "network":
                        return root._icon("streaming")
                    case "service":
                    case "netfolder":
                        return root._icon(container ? "folder" : "streaming")
                    case "playlists":
                        return "qrc:///redesign/player/library-playlist.png"
                    default:
                        if (container)
                            return root._icon("folder")
                        return song ? "qrc:///redesign/music.svg" : "qrc:///redesign/video.svg"
                    }
                }
                smallIcon: String(iconSource).indexOf("/redesign/icons/") >= 0
                // Media of the library (Figma "media"): an accent line under
                // the thumbnail, the title dimmed once watched to the end
                progress: Math.max(0, Number(model.progress ?? 0))
                progressShown: media && !song && (progress > 0 || watched)
                // Figma 848:1996: the rows leading to a place (folders,
                // sections, sources) dimmed as the Add rows; the playlists
                // (Figma "Player": #969696) and the albums, artists and
                // genres as the media
                // (the media playing is not marked: Figma 4778:25123,
                // 970:3308)
                dimmed: addRow || watched
                        || (container && !["playlists", "albums", "artists", "genres"].includes(place))

                // Places open on a click, media play on a double click
                onClicked: if (container || addRow) root._activate(row.model, row.index)
                onDoubleClicked: if (media) root._activate(row.model, row.index)
            }

            Text {
                anchors.centerIn: parent
                width: parent.width - VLCStyle.dp(40, VLCStyle.scale)
                // (not while the model loads; the sources have their Add)
                visible: !root._model ? root.location.section !== "home"
                                      : (list.count === 0 && !(root._model.loading ?? false)
                                         && root.location.section !== "network")
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: !root._model ? qsTr("The media library is not available.")
                                   : qsTr("Nothing here yet.")
                color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#7c7c7b")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.redesign.fontNormal
            }
        }

        // Home: empty, or without the media library
        Text {
            anchors.centerIn: homeList
            width: homeList.width - VLCStyle.dp(40, VLCStyle.scale)
            visible: homeList.visible && (!mlModels.item || (mlModels.item.allMedia.count === 0
                                                             && !mlModels.item.allMedia.loading))
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: !root._mlAvailable ? qsTr("The media library is not available.")
                                     : qsTr("Nothing here yet.")
            color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#7c7c7b")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.redesign.fontNormal
        }
    }

    PlaylistCreateDialog {
        id: playlistCreate
        playlistModel: playlistListModel
    }

    // The model of the playlists needs the media library (it asserts it)
    function _createPlaylist() {
        if (root._mlAvailable)
            playlistCreate.open()
    }

    // The criteria of the place's model to sort by date ("" when it has
    // none: artists, genres, the network's sources)
    function _dateCriteria() {
        switch (location.section) {
        case "folder": return "date"
        case "albums": return "release_year"
        case "artists":
        case "genres":
        case "network":
        case "service": return ""
        case "netfolder": return "fileModified"
        default: return "insertion"
        }
    }
    function _sort(kind, order) {
        let criteria
        switch (location.section) {
        case "folder":
        case "artists":
        case "genres":
        case "network":
        case "service":
        case "netfolder":
            criteria = kind === "title" ? "name" : _dateCriteria()
            break
        default:
            criteria = kind === "title" ? "title" : _dateCriteria()
        }
        if (criteria === "")
            return
        for (const model of _toolModels) {
            if (!model || model.sortCriteria === undefined)
                continue
            model.sortCriteria = criteria
            model.sortOrder = order
        }
        if (location.section === "home")
            _homeSorted = true
    }

    function _playFolder(enqueue) {
        const urls = []
        for (let i = 0; i < folderModel.count; ++i) {
            const index = folderModel.index(i, 0)
            if (!folderModel.data(index, LocalFolderModel.IsFolderRole))
                urls.push(folderModel.data(index, LocalFolderModel.UrlRole))
        }
        if (urls.length > 0)
            MainPlaylistController.append(urls, !enqueue)
    }
    // The songs of the album, the artist, the genre or the playlist shown
    function _playCollection(enqueue) {
        const id = location.section === "playlist" ? location.playlistId : location.parentId
        if (!root._mlAvailable || id === undefined)
            return
        if (enqueue)
            MediaLib.addToPlaylist([id])
        else
            MediaLib.addAndPlay([id])
    }

    // Row of the lists (Figma "media", "playlist", "folder": 32 high, the
    // image or the icon in a box of 32, 10, the name in Inter 14)
    component LibraryRow: T.ItemDelegate {
        id: libraryRow

        // the name, the image of the media (a frame, a cover) and the icon
        // drawn without one
        property string label
        property string cover
        property string iconSource
        // an icon of the design at 24, centred in the box (folders,
        // sections, sources), not a placeholder drawn at 32
        property bool smallIcon: false
        // Media of the library (Figma "media"): an accent line under the
        // thumbnail, its progress (all of it once watched)
        property real progress: 0
        property bool progressShown: false
        // #6B6B6B (Figma 848:1996), #969696 otherwise
        property bool dimmed: false

        height: VLCStyle.dp(32, VLCStyle.scale)
        leftPadding: 0
        rightPadding: 0
        spacing: VLCStyle.dp(10, VLCStyle.scale)
        Accessible.name: label

        background: Rectangle {
            radius: VLCStyle.redesign.controlRadius
            color: libraryRow.hovered ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                                      : "transparent"
        }

        contentItem: RowLayout {
            spacing: libraryRow.spacing

            Item {
                Layout.preferredWidth: VLCStyle.dp(32, VLCStyle.scale)
                Layout.preferredHeight: VLCStyle.dp(32, VLCStyle.scale)

                Impl.ColorImage {
                    anchors.centerIn: parent
                    width: libraryRow.smallIcon ? VLCStyle.redesign.iconSize : parent.width
                    height: width
                    // Until the cover is ready, or without one
                    visible: libraryRow.cover === "" || coverImage.status !== Image.Ready
                    source: libraryRow.iconSource
                    sourceSize: Qt.size(width, height)
                    fillMode: Image.Pad
                    // Figma "Player - Light": the icons #2E2E2E
                    color: libraryRow.smallIcon && !VLCStyle.palette.isDark ? "#2e2e2e" : "transparent"
                }
                Image {
                    id: coverImage

                    anchors.fill: parent
                    visible: status === Image.Ready
                    source: libraryRow.cover
                    fillMode: Image.PreserveAspectCrop
                    sourceSize: Qt.size(width * 2, height * 2)
                    asynchronous: true
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    visible: libraryRow.progressShown
                    width: parent.width * (libraryRow.progress > 0 ? Math.min(1, libraryRow.progress) : 1)
                    height: VLCStyle.dp(2, VLCStyle.scale)
                    color: VLCStyle.redesign.accent
                }
            }

            Text {
                id: rowText

                Layout.fillWidth: true
                Layout.fillHeight: true
                // the text box (17 px) centred in the row, 0.3 px higher
                // (where Figma 187:328 draws the rows)
                verticalAlignment: Text.AlignTop
                topPadding: {
                    const line = Math.round(1.2109375 * font.pixelSize)
                    return Math.round((height - line) / 2) + (line - 1.2109375 * font.pixelSize) / 2
                           + 0.96875 * font.pixelSize - rowMetrics.ascent - 0.3 * VLCStyle.scale
                }
                FontMetrics {
                    id: rowMetrics
                    font: rowText.font
                }
                text: libraryRow.label
                elide: Text.ElideMiddle
                // Figma "Player - Light": as dark, #969696 and dimmed #6B6B6B
                color: libraryRow.dimmed ? VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
                                         : (VLCStyle.palette.isDark ? VLCStyle.redesign.textSecondary : "#969696")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.redesign.fontNormal
            }
        }
    }

    // Icon button of the section column
    component SectionButton: Widgets.IconToolButton {
        id: sectionButton

        property string iconName
        property string iconState: "default"

        // Drawn from an image, the text only names the button
        text: description
        anchors.horizontalCenter: parent?.horizontalCenter
        // Figma "Icon Wrapper": 30 x 32, radius 4
        width: VLCStyle.dp(30, VLCStyle.scale)
        height: VLCStyle.dp(32, VLCStyle.scale)
        radius: VLCStyle.dp(4, VLCStyle.scale)
        backgroundColor: VLCStyle.redesign.color(hovered || down ? VLCStyle.redesign.controlHoverBackground
                                                                 : "transparent", "#9c9c9b")
        contentItem: Impl.ColorImage {
            source: "qrc:///redesign/icons/" + sectionButton.iconName + "/" + sectionButton.iconState + ".png"
            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
            fillMode: Image.Pad
            // the filled icon of the section shown, drawn in the accent
            color: sectionButton.iconState === "active" ? VLCStyle.redesign.accentTint : "transparent"
        }
    }

    // Icon button of the panel header and tool row
    component CrumbSeparator: Text {
        // Figma (as the library header): "\u25B6", Inter 8, #404040, a box of 8
        text: "\u25B6"
        Layout.preferredWidth: VLCStyle.dp(8, VLCStyle.scale)
        Layout.fillHeight: true
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        // (Figma "Player - Light" as dark)
        color: "#404040"
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(8, VLCStyle.scale)
    }

    component CrumbButton: T.AbstractButton {
        id: crumb

        property string description
        // the "..." of the parents above (Figma "Player - Light": #8F8F8F)
        property bool dimmed: false

        implicitWidth: implicitContentWidth
        implicitHeight: implicitContentHeight
        Accessible.name: description

        contentItem: Text {
            text: crumb.text
            elide: Text.ElideMiddle
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
            font.underline: crumb.hovered
            // Figma: the parents as the folder, #8F8F8F
            color: (crumb.dimmed && !VLCStyle.palette.isDark) ? "#8f8f8f"
                                                              : VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
        }
    }

    component PanelButton: Widgets.IconToolButton {
        id: panelButton

        property string iconSource

        // Drawn from an image, the text only names the button
        text: description
        // the icons of the design, 24 x 24
        Layout.preferredWidth: VLCStyle.dp(24, VLCStyle.scale)
        Layout.preferredHeight: VLCStyle.dp(24, VLCStyle.scale)
        radius: VLCStyle.dp(4, VLCStyle.scale)
        backgroundColor: VLCStyle.redesign.color(hovered || down ? VLCStyle.redesign.controlHoverBackground
                                                                 : "transparent", "#9c9c9b")
        contentItem: Image {
            // the arrows (SVG, #8F8F8F): Figma "Player - Light" #2E2E2E
            source: (!VLCStyle.palette.isDark && panelButton.iconSource.endsWith(".svg"))
                    ? SVGColorImage.colorize(panelButton.iconSource)
                                   .any({ "#8F8F8F": VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e") }).uri()
                    : panelButton.iconSource
            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
            fillMode: Image.Pad
            opacity: panelButton.enabled ? 1 : 0.35
        }
    }
}
