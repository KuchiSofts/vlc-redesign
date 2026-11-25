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
// Without a folder: the folders of the library, the start of the way through
// them (#112; the design's Videos page lists them, "Shows", "Films",
// "Anime"): the Folders row of the sidebar, History.push(["folders"]).
Widgets.PageExt {
    id: root

    property url folderUrl

    readonly property bool _libraryRoots: String(folderUrl) === ""

    // The folders of the path
    readonly property var _parts: Helpers.folderBreadcrumbs(folderUrl)

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
        // (a folder replaced by another leaves the count as it was)
        property int revision: 0
        onObjectAdded: ++revision
        onObjectRemoved: ++revision
    }
    // their URLs, the rows of the page without a folder
    readonly property var _libraryRootUrls: {
        void libraryFolderUrls.revision
        let urls = []
        for (let i = 0; i < libraryFolderUrls.count; ++i) {
            const entry = libraryFolderUrls.objectAt(i)
            if (entry)
                urls.push(String(entry.url))
        }
        return urls
    }
    // Number of the folders of the path above the library folder holding it,
    // or that it is (-1: outside the library). The address starts at the page
    // of the library's folders, as the design's starts at the page listing
    // them ("Videos ▸ Shows ▸ The Wand ▸ S01"): "Folders ▸ media ▸ shows",
    // the library folder's own name first (#112)
    readonly property int _rootDepth: {
        let best = -1
        for (let i = 0; i < libraryFolderUrls.count; ++i) {
            const entry = libraryFolderUrls.objectAt(i)
            if (!entry)
                continue
            const parts = Helpers.folderBreadcrumbs(entry.url)
            // (the deepest one, when one is inside another)
            if (parts.length === 0 || parts.length > _parts.length || parts.length - 1 <= best)
                continue
            if (parts.every((part, j) => part.isAuthority === _parts[j].isAuthority
                                         && part.title.toLowerCase() === _parts[j].title.toLowerCase()))
                best = parts.length - 1
        }
        return best
    }
    readonly property bool _inLibrary: _rootDepth >= 0
    readonly property int _shown: _inLibrary ? _parts.length - _rootDepth : Math.min(3, _parts.length)
    readonly property var _crumbs: _libraryRoots ? [qsTr("Folders")]
                                                 : (_inLibrary ? [qsTr("Folders")] : [])
                                                   .concat(_parts.slice(-_shown).map(part => part.title))
    readonly property string _path: _crumbs.join("  \u25B8  ")

    title: _path
    hasGridListMode: true

    readonly property bool _listMode: !MainCtx.gridView
    // (the library's folders: a page of the sidebar, its title as Videos' is)
    libraryCrumbs: _libraryRoots ? null : _crumbs
    onLibraryCrumbClicked: (index) => {
        // (the library's folders: the page shown)
        if (_libraryRoots)
            return
        if (_inLibrary && index === 0) {
            History.push(["folders"], Qt.MouseFocusReason)
            return
        }
        const part = _parts[_parts.length - _shown + index - (_inLibrary ? 1 : 0)]
        History.push(["folder"], { folderUrl: part.url }, Qt.MouseFocusReason)
    }
    libraryHeader: true
    libraryItemCount: folderModel.count
    // Figma: the more button at the end of the header, the "folder context
    // menu" of this folder then the other open actions
    headerAction: Component {
        // (the library's folders: Add folder to library… first, no folder of
        // the page for the folder's rows)
        OpenMediaButton { iconOnly: true; libraryFolder: true; folderActions: root._libraryRoots ? [] : root._folderActions }
    }
    isSearchable: true
    // Figma "Sorting Options": "Unwatched First" by default
    sortModel: [
        { text: qsTr("Unwatched First"), criteria: "unwatched" },
        { text: qsTr("Date"), criteria: "date" },
        { text: qsTr("Name"), criteria: "name" }
    ]

    // Figma "folder context menu" (2139:8935): of a folder of the page, or
    // of the page's folder (the more button of the header gives no data)
    function _folderOf(dataList) { return dataList.length > 0 ? dataList[0].url : root.folderUrl }
    readonly property var _folderActions: [
        { text: qsTr("Add File"), icon: "add",
          action: () => DialogsProvider.simpleOpenDialog() },
        { text: qsTr("Add Folder"), icon: "folder-add",
          action: () => DialogsProvider.PLOpenDir() },
        { text: qsTr("Edit Subtitle Tracks"), icon: "subtitles",
          action: (dataList) => root._tracksDialog(false).openForFolderUrl(root._folderOf(dataList)) },
        { text: qsTr("Edit Audio Tracks"), icon: "audio",
          action: (dataList) => root._tracksDialog(true).openForFolderUrl(root._folderOf(dataList)) },
        { text: qsTr("Resync"), icon: "sync",
          action: () => {
              if (!MainCtx.mediaLibraryAvailable)
                  return
              folderModel.reload()
              MainCtx.mediaLibrary.reload()
          },
          enabled: () => MainCtx.mediaLibraryAvailable },
        // a row of its own in the sidebar (the designer's pinned folders)
        { text: (dataList) => sidebarPins.isPinnedFolder(root._folderOf(dataList)) ? qsTr("Unpin from Sidebar")
                                                                                   : qsTr("Pin to Sidebar"),
          icon: "pin",
          action: (dataList) => {
              const url = String(root._folderOf(dataList))
              if (sidebarPins.isPinnedFolder(url))
                  sidebarPins.unpin(sidebarPins._pinKey({ kind: "folder", url: url }))
              else
                  sidebarPins.pinFolder(url)
          } },
        // what the folder holds (Folder information)
        { text: qsTr("Info"), icon: "info",
          action: (dataList) => DialogsProvider.folderInfoDialog(root._folderOf(dataList)) }
    ]

    SidebarItems { id: sidebarPins }

    Widgets.RedesignContextMenu {
        id: folderMenu

        parentItem: root
        actions: root._folderActions

        onRequestData: (requestID, indexes) => {
            setData(requestID, indexes.map(i => ({
                url: folderModel.data(folderModel.index(i, 0), LocalFolderModel.UrlRole)
            })))
        }
    }

    // Figma "file context menu" (2404:13340) for the media of the folder; the
    // rows that need the file in the library are greyed out until it is
    Widgets.RedesignContextMenu {
        id: fileMenu

        parentItem: root

        function row(indexes) { return indexes[0] }
        function file(dataList) { return dataList[0] }
        function indexed(dataList) { return dataList.length === 1 && dataList[0].isIndexed }

        actions: [
            { text: qsTr("Play"), icon: "play",
              action: (dataList) => MainPlaylistController.append([fileMenu.file(dataList).url], true) },
            { text: (dataList) => fileMenu.indexed(dataList) && dataList[0].played ? qsTr("Mark as Unwatched")
                                                                                     : qsTr("Mark as Watched"),
              icon: "mark-watched",
              action: (dataList, options, indexes) => folderModel.setPlayed(fileMenu.row(indexes), !dataList[0].played),
              enabled: fileMenu.indexed },
            { text: (dataList) => fileMenu.indexed(dataList) && dataList[0].isFavorite ? qsTr("Remove from Favorites")
                                                                                         : qsTr("Favorite"),
              icon: "favorite",
              action: (dataList, options, indexes) => folderModel.setFavorite(fileMenu.row(indexes), !dataList[0].isFavorite),
              enabled: fileMenu.indexed },
            // the 2nd argument: transcode only (Convert), not the Stream output
            { text: qsTr("Stream"), icon: "streaming",
              action: (dataList) => DialogsProvider.streamingDialog([fileMenu.file(dataList).url], false) },
            { text: qsTr("Convert"), icon: "swap",
              action: (dataList) => DialogsProvider.streamingDialog([fileMenu.file(dataList).url], true) },
            { text: qsTr("Add to Queue"), icon: "queue",
              action: (dataList) => MainPlaylistController.append([fileMenu.file(dataList).url], false) },
            { text: qsTr("Add to Playlist"), icon: "playlist",
              action: (dataList, options, indexes) => DialogsProvider.playlistsDialog([folderModel.mediaId(fileMenu.row(indexes))]),
              enabled: fileMenu.indexed },
            // the folder opens with the file selected
            { text: qsTr("Show in File Explorer"), icon: "folder-open",
              action: (dataList) => MainCtx.showInFileManager(String(fileMenu.file(dataList).url)) },
            // the folder of the file in the library; this page already
            // shows it: the file is made current and scrolled to
            { text: qsTr("Go to Source"), icon: "go-to",
              action: (dataList, options, indexes) => {
                  const url = String(fileMenu.file(dataList).url)
                  const parent = url.substring(0, url.lastIndexOf("/"))
                  // the paths compared: the URLs of the folder and of the
                  // file are encoded differently ("Rock & Roll")
                  const path = (link) => Helpers.localPathFromUrl(link).replace(/[\/\\]$/, "")
                  if (path(parent) !== path(root.folderUrl)) {
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
              enabled: fileMenu.indexed }
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
        roots: root._libraryRoots ? root._libraryRootUrls : []
        ml: MainCtx.mediaLibraryAvailable ? MainCtx.mediaLibrary : null
        searchPattern: root.search.pattern
        sortCriteria: root.sort.criteria !== "" ? root.sort.criteria : "unwatched"
        // the default of the design: "Unwatched First" (the page's order
        // only means something once a criterion is chosen)
        sortOrder: root.sort.criteria !== "" ? root.sort.order : Qt.AscendingOrder
    }

    Connections {
        target: folderModel.ml
        enabled: !root._libraryRoots

        function onIdleChanged() {
            // Discovery can finish while the library is still parsing new files.
            if (folderModel.ml.idle)
                folderModel.loadLibraryData()
        }
    }

    GridView {
        id: grid

        // Figma "Frame 53" / "Frame 45": 20 around, cards of 316 every 336,
        // rows of 217 (216.75) every 257 (256.75)
        readonly property real margin: VLCStyle.dp(20, VLCStyle.scale)
        // Figma 970:3792: rows of 406 x 32, 10 apart
        readonly property real rowWidth: VLCStyle.dp(406, VLCStyle.scale)
        readonly property real rowGap: VLCStyle.dp(10, VLCStyle.scale)
        // (the design's card; the cards of a row grow to end it at the
        // page's margin, by a third at most, as the other grids do)
        readonly property real baseCardWidth: VLCStyle.dp(316, VLCStyle.scale)
        readonly property real cardWidth: Math.max(baseCardWidth,
            Math.min(Math.floor((width - margin + gap) / columns) - gap, Math.round(baseCardWidth * 1.35)))
        // 316 x 9 / 16: 177.75 as Figma lays the rows out (dp() would round)
        readonly property real pictureHeight: cardWidth * 9 / 16
        readonly property real gap: VLCStyle.dp(20, VLCStyle.scale)
        readonly property int columns: root._listMode
                                       ? Math.max(1, Math.floor((width - margin + rowGap) / (rowWidth + rowGap)))
                                       : Math.max(1, Math.floor((width - margin + gap) / (baseCardWidth + gap)))

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

        // Keyboard: Return/Enter do what a click does, Menu / Shift+F10 open
        // the context menu of the current card, Up on the first row leaves
        // the grid for the header (the arrows move the current card)
        Keys.onReturnPressed: (event) => { currentItem?.activate(Qt.TabFocusReason); event.accepted = currentItem !== null }
        Keys.onEnterPressed: (event) => { currentItem?.activate(Qt.TabFocusReason); event.accepted = currentItem !== null }
        Keys.onMenuPressed: (event) => { currentItem?.showMenu(); event.accepted = currentItem !== null }
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_F10 && (event.modifiers & Qt.ShiftModifier)) {
                currentItem?.showMenu()
                event.accepted = currentItem !== null
            } else if (event.key === Qt.Key_Up) {
                // (the cells of the first row are at the top of the content)
                const firstRow = currentItem ? currentItem.y < cellHeight / 2 : currentIndex < columns
                if (firstRow)
                    Navigation.defaultKeyAction(event)
            }
        }

        delegate: T.ItemDelegate {
            id: card

            required property var model
            required property int index

            readonly property bool isVideo: !model.isFolder
                && /\.(3gp|asf|avi|flv|m2ts|m4v|mkv|mov|mp4|mpe?g|mts|ogv|ts|vob|webm|wmv)$/i.test(String(model.url))
            readonly property real progress: model.progress ?? 0
            readonly property bool played: (model.playcount ?? 0) > 0 || progress > 0
            // the card the keys are on (only with the focus in the grid)
            readonly property bool _current: grid.activeFocus && card.GridView.isCurrentItem
            // the look of the hover, given to the current card too
            readonly property bool _lit: card.hovered || card._current
            // (the MRL of the library: the url as text decodes some characters)
            readonly property url previewSource: card.model.isFolder ? ""
                                                 : card.isVideo ? VLCThumbnail.uri(card.model.mrl, 0.1)
                                                                : VLCThumbnail.artwork(card.model.mrl)

            width: grid.cellWidth
            height: grid.cellHeight
            padding: 0
            // (the card draws its own name: the one an assistive tool reads)
            Accessible.name: card.model?.name ?? ""

            // Figma 970:3792: the row under the pointer has no background
            // (its name in the accent, its picture darkened with the play)
            background: null

            contentItem: Item {
            // List mode (Figma "media"): the thumbnail, 10, the name (only the
            // tree of the mode shown is made)
            // (anchored: at the start of its cell, the right in a right to
            // left layout, where the grid lays the cells out from the right)
            Loader {
                anchors.left: parent.left
                active: root._listMode
                sourceComponent: Item {
                    width: grid.rowWidth
                    height: VLCStyle.dp(32, VLCStyle.scale)

                    Rectangle {
                        id: thumbnail
                        anchors.left: parent.left
                        width: VLCStyle.dp(32, VLCStyle.scale)
                        height: width
                        radius: VLCStyle.dp(2, VLCStyle.scale)
                        color: card.model.isFolder ? "transparent" : VLCStyle.redesign.color("#171717", "#dedede")
                        border.width: card._current ? VLCStyle.dp(1, VLCStyle.scale) : 0
                        border.color: VLCStyle.redesign.focusColor

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
                            visible: smallPreview.status !== Image.Ready && !card._lit
                            width: card.model.isFolder ? VLCStyle.dp(32, VLCStyle.scale) : VLCStyle.redesign.iconSize
                            height: width
                            source: card.model.isFolder ? "qrc:///redesign/folder.svg"
                                                        : "qrc:///redesign/icons/file/default.svg"
                            sourceSize: Qt.size(width, height)
                            smooth: !VLCStyle.redesign.svgExact(width, height, Screen.devicePixelRatio)
                        }
                        // the progress, at the bottom of the thumbnail (a time
                        // axis: from the left in every language, as the seek bar)
                        Item {
                            LayoutMirroring.enabled: false
                            LayoutMirroring.childrenInherit: true
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
                        // under the pointer (Figma 970:3792; the media playing is
                        // not marked, 970:3308): darkened, the outline play of the set
                        Rectangle {
                            anchors.fill: parent
                            radius: parent.radius
                            visible: card._lit && !card.model.isFolder
                            color: "#B3000000"
                            Impl.ColorImage {
                                anchors.centerIn: parent
                                width: VLCStyle.redesign.iconSize
                                height: width
                                source: "qrc:///redesign/icons/play/outline-active.svg"
                                color: VLCStyle.redesign.accentTint
                                sourceSize: Qt.size(width, height)
                                fillMode: Image.Pad
                                smooth: false
                            }
                        }
                    }

                    Text {
                        id: rowName
                        anchors.left: thumbnail.right
                        anchors.leftMargin: grid.rowGap
                        anchors.right: parent.right
                        height: parent.height
                        textFormat: Text.PlainText
                        text: card.model.name
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideMiddle
                        color: card._lit ? VLCStyle.redesign.accent
                                            : VLCStyle.redesign.color(card.played ? "#6B6B6B" : VLCStyle.redesign.textSecondary)
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
                    // (anchored: at the start of the cell, as the rows above)
                    Rectangle {
                        id: picture
                        anchors.left: parent.left
                        width: grid.cardWidth
                        height: grid.pictureHeight
                        radius: VLCStyle.dp(12, VLCStyle.scale)
                        color: card.model.isFolder ? VLCStyle.redesign.color("#1E1E1E", "#dedede")
                                                   : VLCStyle.redesign.color("#171717", "#dedede")
                        border.width: card._current ? VLCStyle.dp(1, VLCStyle.scale) : 0
                        border.color: VLCStyle.redesign.focusColor

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
                        // inside the rounded corners (from the left in every
                        // language: a time axis)
                        Item {
                            LayoutMirroring.enabled: false
                            LayoutMirroring.childrenInherit: true
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
                            source: card.model.isFolder ? "qrc:///redesign/icons/folder/large.svg"
                                                        : "qrc:///redesign/icons/file/default.svg"
                            sourceSize: Qt.size(width, height)
                            fillMode: Image.Pad
                            smooth: false
                        }
                    }

                    Text {
                        id: cardName
                        anchors.left: parent.left
                        y: picture.height + VLCStyle.dp(20, VLCStyle.scale)
                        width: grid.cardWidth
                        height: VLCStyle.dp(19, VLCStyle.scale)
                        textFormat: Text.PlainText
                        text: card.model.name
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        // #6B6B6B once played
                        color: card._lit ? VLCStyle.redesign.accent
                                            : VLCStyle.redesign.color(card.played ? "#6B6B6B" : VLCStyle.redesign.textSecondary)
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

            // Folders open, media play (a click, or Return on the current card)
            function activate(reason) {
                if (card.model.isFolder)
                    History.push(["folder"], { folderUrl: card.model.url }, reason ?? Qt.MouseFocusReason)
                else
                    MainPlaylistController.append([card.model.url], true)
            }

            // The context menu of the card at a point of the screen, by
            // default the middle of the card (from the keyboard)
            function showMenu(globalPoint) {
                const point = globalPoint ?? card.mapToGlobal(Qt.point(card.width / 2, card.height / 2))
                if (card.model.isFolder)
                    folderMenu.popup([card.index], point)
                else
                    fileMenu.popup([card.index], point)
            }

            onClicked: card.activate()

            // Figma "folder context menu"
            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: (eventPoint) => card.showMenu(card.mapToGlobal(eventPoint.position))
            }
        }

        Text {
            anchors.centerIn: parent
            // a longer translation on more lines in a narrow window
            width: Math.min(implicitWidth, grid.width - 2 * grid.margin)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: grid.count === 0
            text: folderModel.searchPattern !== "" ? qsTr("No media matches the search")
                                                   : root._libraryRoots ? qsTr("The library has no folders yet")
                                                                        : qsTr("No media in this folder")
            color: VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.redesign.fontMedium
        }
    }
}
