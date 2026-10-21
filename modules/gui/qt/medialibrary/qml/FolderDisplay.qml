/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl

import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Player
import VLC.Playlist
import VLC.Style
import VLC.Util
import VLC.Dialogs
import VLC.Widgets as Widgets
import VLC.Network as Network

// Media of a local folder in the library (Figma "Player - library grid":
// "Videos ▸ Shows ▸ The Wand ▸ S01", 15 items, frames in a grid). Opened
// with History.push(["folder"], { folderUrl: url }).
Widgets.PageExt {
    id: root

    property url folderUrl

    // The folders of the path
    readonly property var _parts: decodeURIComponent(String(folderUrl).replace(/^file:\/\/\/?/, ""))
                                      .split("/").filter(part => part.length > 0)

    // The folders of the library (its entry points)
    Network.MLFolderModel {
        id: libraryFolders
        ctx: MainCtx
    }
    Instantiator {
        id: libraryFolderUrls
        model: libraryFolders
        delegate: QtObject {
            required property var url
        }
    }
    // Number of the folders of the path that make the library folder holding
    // it (0: outside the library)
    readonly property int _rootDepth: {
        let best = 0
        for (let i = 0; i < libraryFolderUrls.count; ++i) {
            const entry = libraryFolderUrls.objectAt(i)
            if (!entry)
                continue
            const parts = decodeURIComponent(String(entry.url).replace(/^file:\/\/\/?/, ""))
                              .split("/").filter(part => part.length > 0)
            if (parts.length >= _parts.length || parts.length <= best)
                continue
            if (parts.every((part, j) => part.toLowerCase() === _parts[j].toLowerCase()))
                best = parts.length
        }
        return best
    }
    // The section of the design's address: "Videos" (or "Music" when the
    // folder holds songs and no video)
    readonly property bool _musicFolder: {
        let audio = false
        for (let i = 0; i < folderModel.count; ++i) {
            const url = String(folderModel.data(folderModel.index(i, 0), LocalFolderModel.UrlRole))
            if (/\.(3gp|asf|avi|flv|m2ts|m4v|mkv|mov|mp4|mpe?g|mts|ogv|ts|vob|webm|wmv)$/i.test(url))
                return false
            if (/\.(aac|aiff?|alac|ape|flac|m4a|mka|mp3|oga|ogg|opus|wav|wma)$/i.test(url))
                audio = true
        }
        return audio
    }
    readonly property bool _inLibrary: _rootDepth > 0
    readonly property int _shown: _inLibrary ? _parts.length - _rootDepth : Math.min(3, _parts.length)
    readonly property string _section: _musicFolder ? qsTr("Music") : qsTr("Videos")
    readonly property var _crumbs: (_inLibrary ? [_section] : []).concat(_parts.slice(-_shown))
    readonly property string _path: _crumbs.join("  \u25B8  ")

    title: _path
    hasGridListMode: true

    readonly property bool _listMode: !MainCtx.gridView
    libraryCrumbs: _crumbs
    onLibraryCrumbClicked: (index) => {
        if (_inLibrary && index === 0) {
            History.push([_musicFolder ? "music" : "video"], Qt.MouseFocusReason)
            return
        }
        // Folder of the crumb: the path up to it
        const depth = _parts.length - _shown + index + (_inLibrary ? 0 : 1)
        // Names encoded ("#", "%", any script), the drive kept as is ("C:")
        const path = _parts.slice(0, depth)
                           .map((part, i) => i === 0 && /:$/.test(part) ? part : encodeURIComponent(part))
                           .join("/")
        // Drive roots keep their slash ("C:/")
        const url = "file:///" + path + (depth === 1 && /:$/.test(path) ? "/" : "")
        History.push(["folder"], { folderUrl: url }, Qt.MouseFocusReason)
    }
    libraryHeader: true
    libraryItemCount: folderModel.count
    // Figma: the more button at the end of the header
    headerAction: Component {
        OpenMediaButton { iconOnly: true; libraryFolder: true }
    }
    isSearchable: true
    // Figma "Sorting Options": "Unwatched First" by default
    sortModel: [
        { text: qsTr("Unwatched First"), criteria: "unwatched" },
        { text: qsTr("Date"), criteria: "date" },
        { text: qsTr("Name"), criteria: "name" }
    ]

    // Figma "folder context menu" (2139:8935)
    NativeMenu {
        id: folderMenu

        function folderUrl(dataList) { return dataList[0].url }

        actions: [
            { text: qsTr("Add File"), icon: "add",
              action: () => DialogsProvider.simpleOpenDialog() },
            { text: qsTr("Add Folder"), icon: "folder-add",
              action: () => DialogsProvider.PLOpenDir() },
            { text: qsTr("Edit Subtitle Tracks"), icon: "subtitles",
              action: (dataList) => root._tracksDialog(false).openForFolderUrl(folderMenu.folderUrl(dataList)) },
            { text: qsTr("Edit Audio Tracks"), icon: "audio",
              action: (dataList) => root._tracksDialog(true).openForFolderUrl(folderMenu.folderUrl(dataList)) },
            // (the design's "Resync": the whole library is scanned again)
            { text: qsTr("Reload Library"), icon: "sync",
              action: () => { if (MainCtx.mediaLibraryAvailable) MainCtx.mediaLibrary.reload() },
              visible: () => MainCtx.mediaLibraryAvailable },
            // (the design's "Info": the folder opens in the file manager)
            { text: qsTr("Show in File Explorer"), icon: "folder-open",
              action: (dataList) => Qt.openUrlExternally(folderMenu.folderUrl(dataList)) }
        ]

        onRequestData: (requestID, indexes) => {
            setData(requestID, indexes.map(i => ({
                url: folderModel.data(folderModel.index(i, 0), LocalFolderModel.UrlRole)
            })))
        }
    }

    // Figma "file context menu" (6065:13616) for the media of the folder
    NativeMenu {
        id: fileMenu

        function row(indexes) { return indexes[0] }
        function file(dataList) { return dataList[0] }

        actions: [
            { text: qsTr("Play"), icon: "play",
              action: (dataList) => MainPlaylistController.append([fileMenu.file(dataList).url], true) },
            { text: qsTr("Mark as Watched"), icon: "mark-watched",
              action: (dataList, options, indexes) => folderModel.setPlayed(fileMenu.row(indexes), true),
              visible: (dataList) => dataList.length === 1 && dataList[0].isIndexed && !dataList[0].played },
            { text: qsTr("Mark as Unwatched"), icon: "mark-watched",
              action: (dataList, options, indexes) => folderModel.setPlayed(fileMenu.row(indexes), false),
              visible: (dataList) => dataList.length === 1 && dataList[0].isIndexed && dataList[0].played },
            { text: qsTr("Favorite"), icon: "favorite",
              action: (dataList, options, indexes) => folderModel.setFavorite(fileMenu.row(indexes), true),
              visible: (dataList) => dataList.length === 1 && dataList[0].isIndexed && !dataList[0].isFavorite },
            { text: qsTr("Remove from Favorites"), icon: "favorite",
              action: (dataList, options, indexes) => folderModel.setFavorite(fileMenu.row(indexes), false),
              visible: (dataList) => dataList.length === 1 && dataList[0].isIndexed && dataList[0].isFavorite },
            // the 2nd argument: transcode only (Convert), not the Stream output
            { text: qsTr("Stream"), icon: "streaming",
              action: (dataList) => DialogsProvider.streamingDialog([fileMenu.file(dataList).url], false) },
            { text: qsTr("Convert"), icon: "swap",
              action: (dataList) => DialogsProvider.streamingDialog([fileMenu.file(dataList).url], true) },
            { text: qsTr("Add to Queue"), icon: "queue",
              action: (dataList) => MainPlaylistController.append([fileMenu.file(dataList).url], false) },
            { text: qsTr("Add to Playlist"), icon: "playlist",
              action: (dataList, options, indexes) => DialogsProvider.playlistsDialog([folderModel.mediaId(fileMenu.row(indexes))]),
              visible: (dataList) => dataList.length === 1 && dataList[0].isIndexed },
            { text: qsTr("Show in File Explorer"), icon: "folder-open",
              action: () => Qt.openUrlExternally(root.folderUrl) },
            // the folder of the file in the library; this page already
            // shows it: the file is made current and scrolled to
            { text: qsTr("Go to Source"), icon: "go-to",
              action: (dataList, options, indexes) => {
                  const url = String(fileMenu.file(dataList).url)
                  const parent = url.substring(0, url.lastIndexOf("/"))
                  if (parent !== String(root.folderUrl).replace(/\/$/, "")) {
                      History.push(["folder"], { folderUrl: parent }, Qt.MouseFocusReason)
                  } else {
                      grid.currentIndex = fileMenu.row(indexes)
                      grid.positionViewAtIndex(grid.currentIndex, GridView.Contain)
                  }
              } },
            { text: qsTr("Add Subtitle Tracks"), icon: "subtitles",
              action: (dataList) => root._tracksDialog(false).openForUrl(fileMenu.file(dataList).url) },
            { text: qsTr("Add Audio Tracks"), icon: "audio",
              action: (dataList) => root._tracksDialog(true).openForUrl(fileMenu.file(dataList).url) },
            { text: qsTr("Properties & Metadata"), icon: "info",
              action: (dataList, options, indexes) => DialogsProvider.mediaInfoDialog(folderModel.mediaId(fileMenu.row(indexes))),
              visible: (dataList) => dataList.length === 1 && dataList[0].isIndexed }
        ]

        onRequestData: (requestID, indexes) => {
            setData(requestID, indexes.map(i => {
                const index = folderModel.index(i, 0)
                return {
                    url: folderModel.data(index, LocalFolderModel.UrlRole),
                    isIndexed: folderModel.data(index, LocalFolderModel.IsIndexedRole),
                    isFavorite: folderModel.data(index, LocalFolderModel.IsFavoriteRole),
                    played: folderModel.data(index, LocalFolderModel.PlayCountRole) > 0
                }
            }))
        }
    }

    // The dialogs of the tracks, made when first opened: each one holds
    // models of the library
    property var _subtitleDialog: null
    property var _audioDialog: null
    Component {
        id: tracksDialogComponent
        SubtitleAssociationsDialog { }
    }
    function _tracksDialog(audio) {
        if (audio) {
            if (!_audioDialog)
                _audioDialog = tracksDialogComponent.createObject(root, { audio: true })
            return _audioDialog
        }
        if (!_subtitleDialog)
            _subtitleDialog = tracksDialogComponent.createObject(root)
        return _subtitleDialog
    }

    LocalFolderModel {
        id: folderModel

        folder: root.folderUrl
        ml: MainCtx.mediaLibraryAvailable ? MainCtx.mediaLibrary : null
        searchPattern: root.search.pattern
        sortCriteria: root.sort.criteria !== "" ? root.sort.criteria : "unwatched"
        // the default of the design: "Unwatched First" (the page's order
        // only means something once a criterion is chosen)
        sortOrder: root.sort.criteria !== "" ? root.sort.order : Qt.AscendingOrder
    }

    GridView {
        id: grid

        // Figma "Frame 53" / "Frame 45": 20 around, cards of 316 every 336,
        // rows of 217 (216.75) every 257 (256.75)
        readonly property real margin: VLCStyle.dp(20, VLCStyle.scale)
        // Figma 970:3792: rows of 406 x 32, 10 apart
        readonly property real rowWidth: VLCStyle.dp(406, VLCStyle.scale)
        readonly property real rowGap: VLCStyle.dp(10, VLCStyle.scale)
        readonly property real cardWidth: VLCStyle.dp(316, VLCStyle.scale)
        // 316 x 9 / 16: 177.75 as Figma lays the rows out (dp() would round)
        readonly property real pictureHeight: cardWidth * 9 / 16
        readonly property real gap: VLCStyle.dp(20, VLCStyle.scale)
        readonly property int columns: root._listMode
                                       ? Math.max(1, Math.floor((width - margin + rowGap) / (rowWidth + rowGap)))
                                       : Math.max(1, Math.floor((width - margin + gap) / (cardWidth + gap)))

        anchors.fill: parent
        anchors.leftMargin: margin
        anchors.rightMargin: 0
        // the list: 10 + 20 above the rows
        topMargin: root._listMode ? margin + rowGap : margin
        bottomMargin: margin
        clip: true
        focus: true

        cellWidth: root._listMode ? rowWidth + rowGap : cardWidth + gap
        cellHeight: root._listMode ? VLCStyle.dp(32, VLCStyle.scale) + rowGap
                                   : pictureHeight + VLCStyle.dp(20 + 19, VLCStyle.scale) + VLCStyle.dp(40, VLCStyle.scale)

        model: folderModel

        T.ScrollBar.vertical: Widgets.ScrollBarExt { }

        delegate: T.ItemDelegate {
            id: card

            required property var model
            required property int index

            readonly property bool isVideo: !model.isFolder
                && /\.(3gp|asf|avi|flv|m2ts|m4v|mkv|mov|mp4|mpe?g|mts|ogv|ts|vob|webm|wmv)$/i.test(String(model.url))
            // the media being played (not the one left after a stop); the
            // paths compared, the same file has URLs encoded differently
            readonly property bool playing: !model.isFolder && Player.isStarted
                                            && Helpers.localPathFromUrl(Player.url) === Helpers.localPathFromUrl(model.url)
            readonly property real progress: model.progress ?? 0
            readonly property bool played: (model.playcount ?? 0) > 0 || progress > 0
            // (the MRL of the library: the url as text decodes some characters)
            readonly property url previewSource: card.model.isFolder ? ""
                                                 : card.isVideo ? VLCThumbnail.uri(card.model.mrl, 0.1)
                                                                : VLCThumbnail.artwork(card.model.mrl)

            width: grid.cellWidth
            height: grid.cellHeight
            padding: 0

            background: Rectangle {
                visible: root._listMode
                width: grid.rowWidth
                height: VLCStyle.dp(32, VLCStyle.scale)
                radius: VLCStyle.redesign.controlRadius
                color: card.hovered ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                                    : "transparent"
            }

            contentItem: Item {
            // List mode (Figma "media"): the thumbnail, 10, the name (only the
            // tree of the mode shown is made)
            Loader {
                active: root._listMode
                sourceComponent: Item {
                    width: grid.rowWidth
                    height: VLCStyle.dp(32, VLCStyle.scale)

                    Rectangle {
                        id: thumbnail
                        width: VLCStyle.dp(32, VLCStyle.scale)
                        height: width
                        radius: VLCStyle.dp(2, VLCStyle.scale)
                        color: card.model.isFolder ? "transparent" : VLCStyle.redesign.color("#171717", "#dedede")

                        Widgets.ImageExt {
                            id: smallPreview
                            anchors.fill: parent
                            visible: status === Image.Ready
                            radius: parent.radius
                            source: root._listMode ? card.previewSource : ""
                            sourceSize: Qt.size(width * 2, height * 2)
                            fillMode: card.isVideo ? Image.PreserveAspectCrop : Image.PreserveAspectFit
                            asynchronous: true
                        }
                        Image {
                            anchors.centerIn: parent
                            visible: smallPreview.status !== Image.Ready && !card.playing
                            width: card.model.isFolder ? VLCStyle.dp(32, VLCStyle.scale) : VLCStyle.redesign.iconSize
                            height: width
                            source: card.model.isFolder ? "qrc:///redesign/folder.svg"
                                                        : "qrc:///redesign/icons/file/default.png"
                            sourceSize: Qt.size(width, height)
                        }
                        // the progress, at the bottom of the thumbnail
                        Item {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            visible: card.progress > 0
                            width: Math.round(parent.width * Math.min(1, card.progress))
                            height: VLCStyle.dp(3, VLCStyle.scale)
                            clip: true

                            Rectangle {
                                anchors.left: parent.left
                                anchors.bottom: parent.bottom
                                width: thumbnail.width
                                height: 2 * thumbnail.radius + VLCStyle.dp(3, VLCStyle.scale)
                                radius: thumbnail.radius
                                color: VLCStyle.redesign.accent
                            }
                        }
                        // the media playing: darkened, the outline play of the set
                        Rectangle {
                            anchors.fill: parent
                            radius: parent.radius
                            visible: card.playing
                            color: "#B3000000"
                            Image {
                                anchors.centerIn: parent
                                width: VLCStyle.redesign.iconSize
                                height: width
                                source: "qrc:///redesign/icons/play/outline-active.png"
                                sourceSize: Qt.size(width, height)
                                fillMode: Image.Pad
                                smooth: false
                            }
                        }
                    }

                    Text {
                        id: rowName
                        x: thumbnail.width + grid.rowGap
                        width: parent.width - x
                        height: parent.height
                        text: card.model.name
                        elide: Text.ElideMiddle
                        color: card.playing ? VLCStyle.redesign.accent
                                            : VLCStyle.redesign.color(card.played ? "#6B6B6B" : VLCStyle.redesign.textSecondary,
                                                                      card.played ? "#5a5a59" : "#4d4d4d")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                        // the text box of the design (17 px) centred, on a whole pixel;
                        // 0.2 px higher, where Figma 970:3792 draws the names (measured)
                        verticalAlignment: Text.AlignTop
                        topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                                   font.pixelSize) - rowMetrics.ascent - 0.2 * VLCStyle.scale
                        FontMetrics {
                            id: rowMetrics
                            font: rowName.font
                        }
                    }
                }
            }

            // Figma "Frame 43": the picture, 20, the name
            Loader {
                anchors.fill: parent
                active: !root._listMode
                sourceComponent: Item {
                    Rectangle {
                        id: picture
                        width: grid.cardWidth
                        height: grid.pictureHeight
                        radius: VLCStyle.dp(12, VLCStyle.scale)
                        color: card.model.isFolder ? VLCStyle.redesign.color("#1E1E1E", "#ababaa")
                                                   : VLCStyle.redesign.color("#171717", "#dedede")

                        // A frame of videos, the artwork of the library for songs
                        Widgets.ImageExt {
                            id: preview

                            anchors.fill: parent
                            visible: status === Image.Ready
                            radius: parent.radius
                            source: root._listMode ? "" : card.previewSource
                            sourceSize: Qt.size(width * MainCtx.effectiveDevicePixelRatio(Window.window), 0)
                            fillMode: card.isVideo ? Image.PreserveAspectCrop : Image.PreserveAspectFit
                            asynchronous: true
                        }
                        // #000000 at 20 % over the frames
                        Rectangle {
                            anchors.fill: parent
                            radius: parent.radius
                            visible: preview.visible && VLCStyle.palette.isDark
                            color: "#33000000"
                        }
                        // the progress: a line of the accent at the bottom,
                        // inside the rounded corners
                        Item {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            visible: card.progress > 0
                            width: Math.round(parent.width * Math.min(1, card.progress))
                            height: VLCStyle.dp(4, VLCStyle.scale)
                            clip: true

                            Rectangle {
                                anchors.left: parent.left
                                anchors.bottom: parent.bottom
                                width: picture.width
                                height: 2 * picture.radius
                                radius: picture.radius
                                color: VLCStyle.redesign.accent
                            }
                        }
                        Image {
                            anchors.centerIn: parent
                            visible: preview.status !== Image.Ready
                            width: card.model.isFolder ? VLCStyle.dp(48, VLCStyle.scale) : VLCStyle.redesign.iconSize
                            height: width
                            source: card.model.isFolder ? "qrc:///redesign/icons/folder/large.png"
                                                        : "qrc:///redesign/icons/file/default.png"
                            sourceSize: Qt.size(width, height)
                            fillMode: Image.Pad
                            smooth: false
                        }
                    }

                    Text {
                        id: cardName
                        y: picture.height + VLCStyle.dp(20, VLCStyle.scale)
                        width: grid.cardWidth
                        height: VLCStyle.dp(19, VLCStyle.scale)
                        text: card.model.name
                        elide: Text.ElideRight
                        // #6B6B6B once played
                        color: card.playing ? VLCStyle.redesign.accent
                                            : VLCStyle.redesign.color(card.played ? "#6B6B6B" : VLCStyle.redesign.textSecondary,
                                                                      card.played ? "#5a5a59" : "#4d4d4d")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                        verticalAlignment: Text.AlignTop
                        // Figma: the baseline a whole number of pixels under the top
                        // of the box (15 for Inter 16; measured on 970:4028)
                        topPadding: Math.round(VLCStyle.redesign.textBaseline(0, font.pixelSize)) - cardMetrics.ascent
                        FontMetrics {
                            id: cardMetrics
                            font: cardName.font
                        }
                    }
                }
            }
            }

            // Folders open, media play
            onClicked: {
                if (card.model.isFolder)
                    History.push(["folder"], { folderUrl: card.model.url }, Qt.MouseFocusReason)
                else
                    MainPlaylistController.append([card.model.url], true)
            }

            // Figma "folder context menu"
            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: (eventPoint) => {
                    if (card.model.isFolder)
                        folderMenu.popup([card.index], card.mapToGlobal(eventPoint.position))
                    else
                        fileMenu.popup([card.index], card.mapToGlobal(eventPoint.position))
                }
            }
        }

        Text {
            anchors.centerIn: parent
            visible: grid.count === 0
            text: folderModel.searchPattern !== "" ? qsTr("No media matches the search")
                                                   : qsTr("No media in this folder")
            color: VLCStyle.redesign.color("#6B6B6B", "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.redesign.fontMedium
        }
    }
}
