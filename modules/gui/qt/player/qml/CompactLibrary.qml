/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import VLC.Dialogs
import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Playlist
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// Library next to the video (Figma "Player" with the sidebar open): a column
// of sections and a compact list, to pick media without leaving the player.
FocusScope {
    id: root

    // Current place: { section, folder (url), playlistId, title }
    property var location: ({ section: "home" })
    property var _back: []
    property var _forward: []
    // The model filtered by the search field (the place it was typed in)
    property var _searchedModel: null

    // The search belongs to the place it was typed in: another place starts
    // without it, and the place left shows all of its items again
    onLocationChanged: {
        if (_searchedModel && _searchedModel.searchPattern !== undefined)
            _searchedModel.searchPattern = ""
        _searchedModel = null
        searchField.visible = false
        searchField.text = ""
    }

    readonly property bool _mlAvailable: MainCtx.mediaLibraryAvailable
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

    // Folders of the path of the current folder ("C:", "Users", ...)
    readonly property var _folderParts: location.section === "folder"
        ? decodeURIComponent(String(folderModel.folder).replace(/^file:\/\/\/?/, ""))
              .split("/").filter(part => part.length > 0)
        : []

    // Folder of the first `depth` parts of the path
    function _folderAt(depth) {
        const path = _folderParts.slice(0, depth)
                                 .map((part, i) => i === 0 && /:$/.test(part) ? part : encodeURIComponent(part))
                                 .join("/")
        return "file:///" + path + (depth === 1 && /:$/.test(path) ? "/" : "")
    }

    readonly property string _title: {
        switch (location.section) {
        case "folder": return folderModel.folderName
        case "playlist": return location.title ?? ""
        default:
            const entry = _sections.find(s => s.section === location.section)
            return entry ? entry.title : ""
        }
    }

    // Model and row presentation of the current place
    readonly property var _model: {
        switch (location.section) {
        case "folder": return folderModel
        case "videos": return _mlAvailable ? videoModel : null
        case "music": return _mlAvailable ? audioModel : null
        case "playlists": return _mlAvailable ? playlistListModel : null
        case "playlist": return _mlAvailable ? playlistModel : null
        case "home":
        case "recent": return _mlAvailable ? recentModel : null
        default: return null
        }
    }

    LocalFolderModel {
        id: folderModel
        folder: root.location.section === "folder" ? root.location.folder : ""
        ml: root._mlAvailable ? MainCtx.mediaLibrary : null
        // Figma: unwatched first, as the folder pages
        sortCriteria: "unwatched"
        sortOrder: Qt.AscendingOrder
    }

    MLVideoModel { id: videoModel; ml: root._mlAvailable ? MediaLib : null }
    MLAudioModel { id: audioModel; ml: root._mlAvailable ? MediaLib : null }
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

    function _activate(entry, index) {
        switch (root.location.section) {
        case "folder":
            if (entry.isFolder)
                root.showFolder(entry.url)
            else
                MainPlaylistController.append([entry.url], true)
            break
        case "playlists":
            root.go({ section: "playlist", playlistId: entry.id, title: entry.name })
            break
        default:
            MediaLib.addAndPlay([entry.id])
        }
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
                    // Figma 2160:8732: a folder shown, the Videos icon filled (grey)
                    iconState: current && modelData.icon !== "recent" ? "active"
                             : (root.location.section === "folder" && modelData.section === "videos") ? "selected"
                             : "default"
                    description: modelData.title
                    onClicked: {
                        if (modelData.section === "network")
                            root.openLibraryRequested("network")
                        else if (!current)
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
                visible: root._folderParts.length < 2
                text: root._title
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

            // Address line of a folder: "... > parent > folder"
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: VLCStyle.dp(10, VLCStyle.scale)
                visible: root._folderParts.length >= 2
                spacing: VLCStyle.dp(6, VLCStyle.scale)
                clip: true

                CrumbButton {
                    visible: root._folderParts.length > 2
                    dimmed: true
                    text: "\u2026"
                    description: root._folderParts.length > 2 ? root._folderParts[root._folderParts.length - 3] : ""
                    onClicked: root.showFolder(root._folderAt(root._folderParts.length - 2))
                }
                CrumbSeparator { visible: root._folderParts.length > 2 }
                CrumbButton {
                    // Gives way to the current folder first
                    Layout.minimumWidth: Math.min(implicitWidth, VLCStyle.dp(40, VLCStyle.scale))
                    Layout.maximumWidth: implicitWidth
                    text: root._folderParts.length >= 2 ? root._folderParts[root._folderParts.length - 2] : ""
                    description: text
                    onClicked: root.showFolder(folderModel.parentFolder)
                }
                CrumbSeparator { }
                Text {
                    Layout.minimumWidth: Math.min(implicitWidth, VLCStyle.dp(130, VLCStyle.scale))
                    Layout.maximumWidth: implicitWidth
                    text: root._title
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
                // playlists are created in the media library only
                visible: root.location.section === "folder"
                         || (root.location.section === "playlists" && root._mlAvailable)
                onClicked: (root.location.section === "folder" ? folderMenu : playlistsMenu).toggleFrom(this)

                Widgets.RedesignMenu {
                    id: playlistsMenu
                    Widgets.RedesignMenuItem {
                        text: qsTr("Create playlist")
                        iconName: "add"
                        onTriggered: root._createPlaylist()
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
                        onTriggered: root.showFolder(folderModel.parentFolder)
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
                text: root._model ? qsTr("%1 item", "%1 items", root._model.count).arg(root._model.count) : ""
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
                    if (root._model && root._model.searchPattern !== undefined) {
                        root._model.searchPattern = text
                        root._searchedModel = text !== "" ? root._model : null
                    }
                }
                Keys.onEscapePressed: { text = ""; visible = false }
            }
            PanelButton {
                Layout.leftMargin: VLCStyle.dp(12, VLCStyle.scale)
                iconSource: "qrc:///redesign/icons/sort/dim.png"
                description: qsTr("Sort")
                enabled: !!root._model
                onClicked: sortMenu.toggleFrom(this)

                // Figma: an order applied, the accent dot at the corner (not
                // for the order of the place: folders list the unwatched
                // media first)
                Rectangle {
                    x: VLCStyle.dp(22, VLCStyle.scale)
                    y: -VLCStyle.dp(1, VLCStyle.scale)
                    width: VLCStyle.dp(4, VLCStyle.scale)
                    height: width
                    radius: width / 2
                    color: VLCStyle.redesign.accent
                    visible: {
                        if (!root._model || !root._model.sortCriteria)
                            return false
                        if (root._model === folderModel)
                            return folderModel.sortCriteria !== "unwatched"
                                   || folderModel.sortOrder !== Qt.AscendingOrder
                        return true
                    }
                }

                Widgets.RedesignMenu {
                    id: sortMenu
                    Widgets.RedesignMenuItem {
                        text: qsTr("Title A-Z")
                        onTriggered: root._sort(root.location.section === "folder" ? "name" : "title", Qt.AscendingOrder)
                    }
                    Widgets.RedesignMenuItem {
                        text: qsTr("Title Z-A")
                        onTriggered: root._sort(root.location.section === "folder" ? "name" : "title", Qt.DescendingOrder)
                    }
                    Widgets.RedesignMenuItem {
                        text: qsTr("Newest First")
                        onTriggered: root._sort(root.location.section === "folder" ? "date" : "insertion", Qt.DescendingOrder)
                    }
                    Widgets.RedesignMenuItem {
                        text: qsTr("Oldest First")
                        onTriggered: root._sort(root.location.section === "folder" ? "date" : "insertion", Qt.AscendingOrder)
                    }
                }
            }
            // Figma "View Options": the list here, the grid in the library
            PanelButton {
                Layout.leftMargin: VLCStyle.dp(12, VLCStyle.scale)
                iconSource: "qrc:///redesign/icons/list/dim.png"
                description: qsTr("View")
                enabled: !!root._model
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
                enabled: !!root._model
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
            model: root._model

            header: Loader {
                width: list.width - list.leftMargin - list.rightMargin
                // (playlists are created in the media library only)
                active: root.location.section === "playlists" && root._mlAvailable
                // a row of 32 and the gap of the list
                height: active ? VLCStyle.dp(42, VLCStyle.scale) : 0
                sourceComponent: Item {
                    Image {
                        width: VLCStyle.dp(32, VLCStyle.scale)
                        height: width
                        // the icon of the design at 32 px (Figma "Player" 187:328)
                        source: "qrc:///redesign/player/library-add.png"
                        sourceSize: Qt.size(width, height)
                    }
                    Text {
                        x: VLCStyle.dp(42, VLCStyle.scale)
                        height: VLCStyle.dp(32, VLCStyle.scale)
                        text: qsTr("Create playlist")
                        color: VLCStyle.redesign.color("#6B6B6B", "#7c7c7b")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.redesign.fontNormal
                        verticalAlignment: Text.AlignTop
                        topPadding: root._baseline(height, font.pixelSize, undefined, createMetrics.ascent)
                        id: createText
                        FontMetrics {
                            id: createMetrics
                            font: createText.font
                        }
                    }
                }
                TapHandler {
                    onTapped: root._createPlaylist()
                }
            }

            delegate: T.ItemDelegate {
                id: row

                required property var model
                required property int index

                readonly property bool isFolder: root.location.section === "folder" && model.isFolder
                // Songs read "title - artist" as in the design
                readonly property string label: {
                    const name = model.name ?? model.title ?? ""
                    return (root.location.section === "music" && model.main_artist)
                           ? name + " - " + model.main_artist : name
                }
                // the paths compared: the same file has URLs encoded differently
                readonly property bool playing: {
                    const mrl = String(model.mrl ?? model.url ?? "")
                    return mrl !== "" && !isFolder
                           && Helpers.localPathFromUrl(Player.url) === Helpers.localPathFromUrl(mrl)
                }
                // Media of the library (Figma "media"): an accent line under
                // the thumbnail, the title dimmed once watched to the end
                readonly property real progress: Math.max(0, Number(model.progress ?? 0))
                readonly property bool watched: Number(model.playcount ?? 0) > 0 || progress > 0
                // Videos of a folder show a frame, decoded on demand, songs
                // the artwork of the library
                readonly property string cover: {
                    // (the MRL of the library: the url as text decodes some
                    // characters)
                    if (root.location.section === "folder" && !model.isFolder) {
                        if (/\.(3gp|asf|avi|flv|m2ts|m4v|mkv|mov|mp4|mpe?g|mts|ogv|ts|vob|webm|wmv)$/i
                                .test(String(model.url)))
                            return VLCThumbnail.uri(String(model.mrl), 0.1)
                        return VLCThumbnail.artwork(String(model.mrl))
                    }
                    // playlists show the icon of the design (Figma "playlist")
                    if (root.location.section === "playlists")
                        return ""
                    return model.thumbnail ?? model.cover ?? model.small_cover ?? ""
                }

                // Figma "playlist" / "media": 337 x 32, an icon of 32, 10 px
                width: list.width - list.leftMargin - list.rightMargin
                height: VLCStyle.dp(32, VLCStyle.scale)
                leftPadding: 0
                rightPadding: 0
                spacing: VLCStyle.dp(10, VLCStyle.scale)

                background: Rectangle {
                    radius: VLCStyle.redesign.controlRadius
                    color: row.hovered ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                                       : "transparent"
                }

                contentItem: RowLayout {
                    spacing: row.spacing

                    Item {
                        Layout.preferredWidth: VLCStyle.dp(32, VLCStyle.scale)
                        Layout.preferredHeight: VLCStyle.dp(32, VLCStyle.scale)

                        Image {
                            anchors.fill: parent
                            // Until the cover is ready, or without one: the
                            // icons of the design drawn at 32 px
                            visible: row.cover === "" || coverImage.status !== Image.Ready
                            sourceSize: Qt.size(width, height)
                            source: {
                                if (row.isFolder)
                                    return "qrc:///redesign/folder.svg"
                                switch (root.location.section) {
                                case "playlists": return "qrc:///redesign/player/library-playlist.png"
                                case "music": return "qrc:///redesign/music.svg"
                                default: return "qrc:///redesign/video.svg"
                                }
                            }
                        }
                        Image {
                            id: coverImage

                            anchors.fill: parent
                            visible: status === Image.Ready
                            source: row.cover
                            fillMode: Image.PreserveAspectCrop
                            sourceSize: Qt.size(width * 2, height * 2)
                            asynchronous: true
                        }
                        Rectangle {
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            visible: row.progress > 0 || row.watched
                            width: parent.width * (row.progress > 0 ? Math.min(1, row.progress) : 1)
                            height: VLCStyle.dp(2, VLCStyle.scale)
                            color: VLCStyle.redesign.accent
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        // the text box (17 px) centred in the row
                        verticalAlignment: Text.AlignTop
                        topPadding: root._baseline(height, font.pixelSize, undefined, rowMetrics.ascent)
                        id: rowText
                        FontMetrics {
                            id: rowMetrics
                            font: rowText.font
                        }
                        text: row.label
                        elide: Text.ElideMiddle
                        // Figma "Player - Light": as dark, #969696 and watched
                        // #6B6B6B
                        color: row.playing ? VLCStyle.redesign.accent
                             : row.watched ? VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
                                           : (VLCStyle.palette.isDark ? VLCStyle.redesign.textSecondary : "#969696")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.redesign.fontNormal
                    }
                }

                // Folders and playlists open on a click, media play on a double click
                onClicked: if (row.isFolder || root.location.section === "playlists")
                               root._activate(row.model, row.index)
                onDoubleClicked: if (!row.isFolder && root.location.section !== "playlists")
                                     root._activate(row.model, row.index)
            }

            Text {
                anchors.centerIn: parent
                width: parent.width - VLCStyle.dp(40, VLCStyle.scale)
                visible: !root._model || root._model.count === 0
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: !root._model ? qsTr("The media library is not available.")
                                   : qsTr("Nothing here yet.")
                color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#7c7c7b")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.redesign.fontNormal
            }
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

    function _sort(criteria, order) {
        if (!_model)
            return
        _model.sortCriteria = criteria
        _model.sortOrder = order
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
        contentItem: Image {
            source: "qrc:///redesign/icons/" + sectionButton.iconName + "/" + sectionButton.iconState + ".png"
            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
            fillMode: Image.Pad
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
