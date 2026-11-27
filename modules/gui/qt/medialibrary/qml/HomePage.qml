/******************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 *
 * Author: Ash <ashutoshv191@gmail.com>
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
 ******************************************************************************/
import QtQuick
import QtQuick.Window
import QtQuick.Templates as T

import VLC.MediaLibrary

import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Util
import VLC.Style

Widgets.PageExt {
    id: root

    property var pagePrefix // behave like a Page (no default, see PageExt)

    // (its sections list what is recent: the newest first)
    defaultSortOrder: Qt.DescendingOrder

    // (the property of PageExt: one declared again here hid it from the
    // sort of the page, and the header showed no Sort)
    sortModel: [
        { text: qsTr("Alphabetic"), criteria: "title" },
        { text: qsTr("Duration"), criteria: "duration" }
    ]

    // Figma 6261:15078: the media of the library, counted in the header
    // (no count while the library has none: "0 items" stood over rows full
    // of the media played from outside its folders, and while it was read)
    libraryItemCount: _libraryMedia.count > 0 ? _libraryMedia.count : -1
    readonly property MLMediaModel _libraryMedia: MLMediaModel { ml: MediaLib }

    title: qsTr("Home")

    libraryHeader: true

    hasGridListMode: true

    isSearchable: true

    property real listCoverHeight: VLCStyle.listAlbumCover_height
    property real listCoverWidth: VLCStyle.listAlbumCover_width
    property real listCoverRadius: VLCStyle.listAlbumCover_radius

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View
    }

    signal seeAllButtonClicked(string name, int reason)

    // Figma: a section has a padding of 20 top and bottom, its cards one of
    // 20 around them, 17 between the sections: the rows end 57 under the
    // cards (in the grid; the list keeps its height)
    readonly property real _sectionEnd: VLCStyle.dp(20 + 20 + 17, VLCStyle.scale)
    readonly property real _cardsTop: VLCStyle.dp(20, VLCStyle.scale)
    // (the cards of a row grow to end it at the page's margin: the picture
    // of a card `cardWidth` wide in the design, `ratio` high for its width,
    // grows with it, and the row with the picture)
    function _rowHeight(view, cardHeight, cardWidth, ratio) {
        if (!MainCtx.gridView)
            return view.currentItem?.contentHeight ?? view.implicitHeight
        const grown = Math.max(0, (view.currentItem?.cellWidth ?? cardWidth) - cardWidth)
        return _cardsTop + cardHeight + Math.round(grown * ratio) + _sectionEnd
    }

    function _sectionMayBeVisible(header: Item): bool {
        if (!header || !mediaRows.contentItem || flickable.height <= 0)
            return false

        // The Column can still have a pending layout when a model's count
        // changes. Use its visible children's heights to find the header.
        let top = mediaRows.y + mediaRows.contentItem.y
        const children = mediaRows.contentChildren
        for (let i = 0; i < children.length; ++i) {
            const child = children[i]
            if (child === header)
                return top < flickable.contentY + flickable.height
            if (child.visible && child.height > 0)
                top += child.height + mediaRows.spacing
        }
        return true
    }

    function positionContentAtBeginning() {
        contentYBehavior.enabled = true
        flickable.contentY = -flickable.originY
        contentYBehavior.enabled = false
    }

    function setCurrentItemFocus(reason) {
        if (coneNButtons.visible) {
            coneNButtons.forceActiveFocus(reason)
            return
        }
        if (!flickable._hasMedias)
            return

        // `focus` is checked, because that indicates the item that has the focus within the focus scope
        if (continueWatchingRow.visible && continueWatchingRow.focus)
            continueWatchingRow.setCurrentItemFocus(reason)
        else if (listenAgainRow.visible && listenAgainRow.focus)
            listenAgainRow.setCurrentItemFocus(reason)
        else if (favoritesRow.visible && favoritesRow.focus)
            favoritesRow.setCurrentItemFocus(reason)
        else if (newVideoRow.visible && newVideoRow.focus)
            newVideoRow.setCurrentItemFocus(reason)
        else if (newMusicRow.visible && newMusicRow.focus)
            newMusicRow.setCurrentItemFocus(reason)
        else if (continueWatchingRow.visible)
            continueWatchingRow.forceActiveFocus(reason)
        else if (flickable._modelsLoaded)
            mediaRows.forceActiveFocus(reason)
    }

    contentItem: Flickable {
        id: flickable

        implicitWidth: Math.max(coneNButtons.implicitWidth, mediaRows.implicitWidth)
        implicitHeight: _hasMedias ? mediaRows.implicitHeight
                                   : (noResults.visible ? noResults.implicitHeight : coneNButtons.implicitHeight)

        contentWidth: width
        contentHeight: _hasMedias ? mediaRows.implicitHeight
                                  : height - topMargin - bottomMargin

        flickableDirection: Flickable.AutoFlickIfNeeded

        boundsBehavior: Flickable.StopAtBounds

        // FIXME: make the media rows positioners consider the flickable margins when adjusting flickable `contentY`.
        bottomMargin: VLCStyle.margin_large

        pixelAligned: (Screen.pixelDensity >= VLCStyle.highPixelDensityThreshold) // no need for sub-pixel alignment with high pixel density

        // true until known (the Binding sets the inverse: a true set here
        // with the Binding on it was a change from elsewhere for it)
        readonly property bool _hasMedias: !_noMedias
        property bool _noMedias
        // A search change must reach every model before its count describes the new query.
        readonly property bool _modelsLoaded: [continueWatchingRow.model, listenAgainRow.model,
                                              favoritesRow.model, newVideoRow.model, newMusicRow.model]
            .every(model => model.searchPattern === root.search.pattern && !model.loading)

        Binding on _noMedias {
            when: flickable._modelsLoaded
            value: !(continueWatchingRow.visible || listenAgainRow.visible || foldersRow.visible
                     || favoritesRow.visible || newVideoRow.visible || newMusicRow.visible)
            restoreMode: Binding.RestoreNone
        }

        T.ScrollBar.vertical: Widgets.ScrollBarExt {
            id: scrollBar
        }

        // This behavior allows to have similar "smooth" animation
        // that Qt views have with `highlightFollowsCurrentItem`.
        Behavior on contentY {
            id: contentYBehavior

            enabled: false

            // NOTE: Usage of `SmoothedAnimation` is intentional here.
            SmoothedAnimation {
                duration: VLCStyle.duration_veryLong
                easing.type: Easing.InOutSine
            }
        }

        property Component implicitFlickableScrollHandler: DefaultFlickableScrollHandler { }

        // NOTE: This property can be set to null to prevent using a scroll handler:
        property FlickableScrollHandler scrollHandler: {
            if (interactive) {
                // JS ownership:
                return implicitFlickableScrollHandler.createObject(null, { target: flickable })
            } else {
                return null
            }
        }

        Component.onCompleted: {
            if (!usingTouch) {
                // Flickable filters child mouse events for flicking (even when
                // the delegate is grabbed). However, this is not a useful
                // feature for non-touch cases, so disable it here and enable
                // it if touch is detected through the hover handler:
                MainCtx.setFiltersChildMouseEvents(flickable, false)
            }
        }

        readonly property bool usingTouch: MainCtx.usingTouch

        onUsingTouchChanged: {
            if (usingTouch)
                MainCtx.setFiltersChildMouseEvents(flickable, true)
            // We do not disable filtering child mouse events
            // because Qt currently has a bug that the flickable
            // jumps when it is enabled again later on.
        }

        // The design's home has no cone: it only welcomes an empty library
        NoMedialibHome.ConeNButtons {
            id: coneNButtons

            property bool _preferFocus: false

            visible: flickable._noMedias && flickable._modelsLoaded && root.search.pattern === ""
            focus: _preferFocus

            onVisibleChanged: {
                // Leave focus in the header while the user edits or clears a search.
                if (!root.header?.activeFocus)
                    _preferFocus = visible
            }

            orientation: flickable._hasMedias ? Qt.Horizontal : Qt.Vertical

            anchors.centerIn: flickable._hasMedias ? undefined : parent
            anchors.top: flickable._hasMedias ? parent.top : undefined
            anchors.left: flickable._hasMedias ? parent.left : undefined
            anchors.leftMargin: flickable._hasMedias ? newVideoRow.contentLeftMargin : 0

            Navigation.parentItem: root
            Navigation.upItem: root.header
            Navigation.downItem: mediaRows

            onActiveFocusChanged: {
                contentYBehavior.enabled = true
                Helpers.positionFlickableToContainItem(flickable, this)
                contentYBehavior.enabled = false
            }
        }

        Widgets.EmptyLabel {
            id: noResults

            anchors.centerIn: parent
            visible: flickable._noMedias && flickable._modelsLoaded && root.search.pattern !== ""
            text: qsTr("No results")
            cover: VLCStyle.noArtVideoCover
        }

        Widgets.NavigableCol {
            id: mediaRows

            anchors.top: coneNButtons.visible ? coneNButtons.bottom : parent.top
            // Figma "Frame 20": 17 between the line and the first section
            anchors.topMargin: coneNButtons.visible ? 0 : VLCStyle.dp(17, VLCStyle.scale)
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0 // NOTE: We depend on the top padding of view header currently.

            focus: !coneNButtons._preferFocus

            Navigation.parentItem: root
            Navigation.upItem: coneNButtons.visible ? coneNButtons : root.header

            Widgets.ViewHeader {
                topPadding: VLCStyle.dp(20, VLCStyle.scale)
                // (the list's column header: room under the title, as the grid's
                // cards have; it stood against it, #111)
                bottomPadding: MainCtx.gridView ? 0 : VLCStyle.dp(12, VLCStyle.scale)
                visible: continueWatchingRow.visible
                text: qsTr("Continue Watching")
                view: continueWatchingRow
                seeAllButton.visible: true // The design shows the chevron of every section

                onSeeAllButtonClicked: function (reason) {
                    root.seeAllButtonClicked("continueWatching", reason)
                }
            }

            VideoAll {
                id: continueWatchingRow

                AccessibleCompat.id: "continueWatching"

                anchors.left: parent.left
                anchors.right: parent.right

                height: root._rowHeight(this, VLCStyle.dp(217, VLCStyle.scale), VLCStyle.dp(316, VLCStyle.scale), 9 / 16)

                visible: model.count !== 0

                headerPositioning: ListView.InlineHeader
                enableBeginningFade: false
                enableEndFade: false
                sectionProperty: ""

                interactive: false

                emptyLabel: null

                // FIXME: `ExpandGridView` causes extreme performance degradation when `reuseItems`
                //        is true and items provided by the model change (#29084).
                reuseItems: !MainCtx.gridView

                listCoverWidth: root.listCoverWidth
                listCoverHeight: root.listCoverHeight
                listCoverRadius: root.listCoverRadius

                Navigation.parentItem: mediaRows

                model: MLRecentVideoModel {
                    ml: MediaLib

                    sortCriteria: root.sort.criteria
                    sortOrder: root.sort.order
                    searchPattern: root.search.pattern

                    chunkSize: VLCStyle.gridView_maxNbItemPerRow_wide

                    // FIXME: Make limit 0 load no items, instead of loading all items.
                    limit: MainCtx.gridView ? Math.max(continueWatchingRow.currentItem?.nbItemPerRow ?? null, 1) : 5
                }

                contextMenu: MLContextMenu {
                    model: continueWatchingRow.model

                    showPlayAsAudioAction: true
                }

                onActiveFocusChanged: {
                    if (activeFocus) {
                        const item = currentItem?.currentItem ?? currentItem?._getItem(currentIndex) // FIXME: `ExpandGridView` does not have `currentItem`.
                        contentYBehavior.enabled = true
                        Helpers.positionFlickableToContainItem(flickable, item ?? this)
                        contentYBehavior.enabled = false
                    }
                }

                onCurrentIndexChanged: {
                    if (activeFocus) {
                        const item = currentItem?.currentItem ?? currentItem?._getItem(currentIndex) // FIXME: `ExpandGridView` does not have `currentItem`.
                        if (item) {
                            contentYBehavior.enabled = true
                            Helpers.positionFlickableToContainItem(flickable, item)
                            contentYBehavior.enabled = false
                        }
                    }
                }
            }

            Widgets.ViewHeader {
                topPadding: VLCStyle.dp(20, VLCStyle.scale)
                // (the list's column header: room under the title, as the grid's
                // cards have; it stood against it, #111)
                bottomPadding: MainCtx.gridView ? 0 : VLCStyle.dp(12, VLCStyle.scale)
                visible: listenAgainRow.visible
                text: qsTr("Listen Again")
                view: listenAgainRow
                seeAllButton.visible: true // The design shows the chevron of every section

                onSeeAllButtonClicked: function (reason) {
                    root.seeAllButtonClicked("listenAgain", reason)
                }
            }

            MediaView {
                id: listenAgainRow

                compactMargins: true
                // (the design's own page: its card sizes win)
                designWidthWins: true
                gridCoverWidth: VLCStyle.dp(200, VLCStyle.scale)
                gridCoverHeight: VLCStyle.dp(200, VLCStyle.scale)

                AccessibleCompat.id: "listenAgain"

                anchors.left: parent.left
                anchors.right: parent.right

                height: root._rowHeight(this, VLCStyle.dp(268, VLCStyle.scale), VLCStyle.dp(200, VLCStyle.scale), 1)

                visible: model.count !== 0

                listHeaderPositioning: ListView.InlineHeader
                enableBeginningFade: false
                enableEndFade: false
                listSectionProperty: ""

                interactive: false

                emptyLabel: null

                // FIXME: `ExpandGridView` causes extreme performance degradation when `reuseItems`
                //        is true and items provided by the model change (#29084).
                reuseItems: !MainCtx.gridView

                listCoverWidth: root.listCoverWidth
                listCoverHeight: root.listCoverHeight
                listCoverRadius: root.listCoverRadius

                Navigation.parentItem: mediaRows

                model: MLRecentAudioModel {
                    ml: MediaLib

                    // the order of Continue Watching: the last played first
                    // (the model's own order was the first played first)
                    sortCriteria: root.sort.criteria
                    sortOrder: root.sort.order
                    searchPattern: root.search.pattern

                    // FIXME: Make limit 0 load no items, instead of loading all items.
                    limit: MainCtx.gridView ? Math.max(listenAgainRow.currentItem?.nbItemPerRow ?? null, 1) : 5
                }

                onActiveFocusChanged: {
                    if (activeFocus) {
                        const item = currentItem?.currentItem ?? currentItem?._getItem(currentIndex) // FIXME: `ExpandGridView` does not have `currentItem`.
                        contentYBehavior.enabled = true
                        Helpers.positionFlickableToContainItem(flickable, item ?? this)
                        contentYBehavior.enabled = false
                    }
                }
            }

            Widgets.ViewHeader {
                topPadding: VLCStyle.dp(20, VLCStyle.scale)
                bottomPadding: 0
                visible: foldersRow.visible
                text: qsTr("Frequently Visited Folders")
                view: foldersRow
                // The design shows the chevron of every section: the folders
                // open the folders view of the library (the videos grouped by
                // folder, as "Group by folder" of the Videos page), for this
                // view only: the saved grouping of the Videos page stays
                seeAllButton.visible: true

                onSeeAllButtonClicked: function (reason) {
                    History.push(["video", "all"], { groupingOverride: MainCtx.GROUPING_FOLDER }, reason)
                }
            }

            // Folders of the played media, the most played first
            ListView {
                id: foldersRow

                readonly property real contentLeftMargin: continueWatchingRow.contentLeftMargin
                readonly property real contentRightMargin: continueWatchingRow.contentRightMargin
                // (as large as the cards of the music rows, which grow to
                // end their row at the page's margin: 200 in the design)
                readonly property real baseCardSize: VLCStyle.dp(200, VLCStyle.scale)
                // (the design's size at the design's own page width, as the rows)
                // (fewer folders than the row holds keep the design's size:
                // four grown cards beside an empty third of the row filled nothing)
                readonly property real cardSize: count < fit ? baseCardSize
                    : VLCStyle.redesign.cardWidth(baseCardSize,
                        Math.floor((width - leftMargin - rightMargin + spacing) / fit) - spacing,
                        width - leftMargin - rightMargin)

                anchors.left: parent.left
                anchors.right: parent.right
                // Figma "Frame 57": 20, the cards (200, 20, the name 19),
                // 20 and the end of the section
                readonly property real cardHeight: cardSize + VLCStyle.dp(20 + 19, VLCStyle.scale)
                height: root._cardsTop + cardHeight + root._sectionEnd

                visible: count !== 0 && root.search.pattern === ""

                orientation: ListView.Horizontal
                leftMargin: contentLeftMargin
                rightMargin: contentRightMargin
                spacing: VLCStyle.dp(20, VLCStyle.scale)
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                // As the other rows: the cards that fit the width, none cut
                // at the edge (a narrow window showed half a card, and the
                // wheel scrolls the page, not the row); the chevron of the
                // header opens every folder
                interactive: false
                readonly property int fit: Math.max(1, Math.floor((width - leftMargin - rightMargin + spacing)
                                                                  / (baseCardSize + spacing)))

                model: MLFrequentFoldersModel {
                    ml: MediaLib
                    limit: foldersRow.fit
                }

                // Up and Down go to the rows above and below, as from the
                // other rows (a horizontal list only takes Left and Right)
                Navigation.parentItem: mediaRows
                Keys.priority: Keys.AfterItem
                Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

                onActiveFocusChanged: {
                    if (activeFocus) {
                        contentYBehavior.enabled = true
                        Helpers.positionFlickableToContainItem(flickable, this)
                        contentYBehavior.enabled = false
                    }
                }

                delegate: T.ItemDelegate {
                    id: folderCard

                    required property var model

                    width: foldersRow.cardSize
                    height: foldersRow.height
                    padding: 0

                    Accessible.name: model.name

                    background: null
                    // Figma: the card (#1E1E1E, radius 12, the folder at 48),
                    // 20, the name (Inter 16 #969696, box 19)
                    contentItem: Column {
                        topPadding: root._cardsTop
                        spacing: VLCStyle.dp(20, VLCStyle.scale)

                        Rectangle {
                            width: foldersRow.cardSize
                            height: foldersRow.cardSize
                            radius: VLCStyle.dp(12, VLCStyle.scale)
                            // (light: lighter than the page as the covers of
                            // the rows above it, not a darker grey beside them)
                            color: folderCard.hovered
                                   // (seen under the pointer in both themes: the
                                   // controls' hover was 4 levels from the card
                                   // in the dark theme and a dark slab in the light)
                                   ? VLCStyle.redesign.color("#2A2A2A", "#d0d0cf")
                                   : VLCStyle.redesign.color("#1E1E1E", "#dedede")
                            // the card the keys are on
                            border.width: folderCard.activeFocus ? VLCStyle.dp(1, VLCStyle.scale) : 0
                            border.color: VLCStyle.redesign.focusColor

                            Image {
                                anchors.centerIn: parent
                                width: VLCStyle.dp(48, VLCStyle.scale)
                                height: width
                                source: "qrc:///redesign/icons/folder/large.svg"
                                sourceSize: Qt.size(width, height)
                                fillMode: Image.Pad
                                smooth: false
                            }
                        }

                        Text {
                            id: folderName
                            width: foldersRow.cardSize
                            height: VLCStyle.dp(19, VLCStyle.scale)
                            textFormat: Text.PlainText
                            text: folderCard.model.name
                            // from the start of the card (the right in a right
                            // to left layout), whatever the script of the name
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideMiddle
                            color: VLCStyle.redesign.color(VLCStyle.redesign.textSecondary, "#4d4d4d")
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                            verticalAlignment: Text.AlignTop
                            topPadding: VLCStyle.redesign.textBaseline(0, font.pixelSize) - folderMetrics.ascent
                            FontMetrics {
                                id: folderMetrics
                                font: folderName.font
                            }
                        }
                    }

                    onClicked: History.push(["folder"], { folderUrl: folderCard.model.url }, Qt.MouseFocusReason)
                    // (an item delegate only takes Space)
                    Keys.onReturnPressed: clicked()
                    Keys.onEnterPressed: clicked()
                }
            }

            Widgets.ViewHeader {
                id: favoritesHeader

                topPadding: VLCStyle.dp(20, VLCStyle.scale)
                // (the list's column header: room under the title, as the grid's
                // cards have; it stood against it, #111)
                bottomPadding: MainCtx.gridView ? 0 : VLCStyle.dp(12, VLCStyle.scale)
                visible: favoritesRow.visible
                text: qsTr("Favorites")
                view: favoritesRow
                seeAllButton.visible: true // The design shows the chevron of every section

                onSeeAllButtonClicked: function (reason) {
                    root.seeAllButtonClicked("favorites", reason)
                }
            }

            MediaView {
                id: favoritesRow

                // Keep the model and the navigation item in the column, so a
                // lower row can become visible or take focus before the swap.
                // Latch once wanted so upper rows gaining height before the
                // swap cannot tear down and rebuild the view.
                // (latched a moment later, not by a Binding: loading the row changes its count, which the
                // condition reads, and a Binding that set it at once was a binding loop)
                property bool _wanted: false
                readonly property bool _wantNow: !_wanted
                    && (MainCtx.firstFrameSwapped || favoritesRow.focus || flickable.moving || scrollBar.interacting
                        || (favoritesRow.count > 0 && root._sectionMayBeVisible(favoritesHeader)))
                on_WantNowChanged: if (_wantNow) Qt.callLater(favoritesRow._want)
                function _want() { favoritesRow._wanted = true }
                Component.onCompleted: if (_wantNow) _wanted = true
                active: _wanted

                compactMargins: true
                // (the design's own page: its card sizes win)
                designWidthWins: true
                gridCoverWidth: VLCStyle.dp(200, VLCStyle.scale)
                gridCoverHeight: VLCStyle.dp(200, VLCStyle.scale)

                AccessibleCompat.id: "favourites"

                anchors.left: parent.left
                anchors.right: parent.right

                height: root._rowHeight(this, VLCStyle.dp(268, VLCStyle.scale), VLCStyle.dp(200, VLCStyle.scale), 1)

                visible: model.count !== 0

                listHeaderPositioning: ListView.InlineHeader
                enableBeginningFade: false
                enableEndFade: false
                listSectionProperty: ""

                interactive: false

                emptyLabel: null

                // FIXME: `ExpandGridView` causes extreme performance degradation when `reuseItems`
                //        is true and items provided by the model change (#29084).
                reuseItems: !MainCtx.gridView

                listCoverWidth: root.listCoverWidth
                listCoverHeight: root.listCoverHeight
                listCoverRadius: root.listCoverRadius

                Navigation.parentItem: mediaRows

                model: MLMediaModel {
                    favoriteOnly: true

                    ml: MediaLib

                    sortCriteria: root.sort.criteria || "insertion"
                    sortOrder: root.sort.order
                    searchPattern: root.search.pattern

                    chunkSize: VLCStyle.gridView_maxNbItemPerRow_square

                    // FIXME: Make limit 0 load no items, instead of loading all items.
                    limit: MainCtx.gridView ? Math.max(favoritesRow.currentItem?.nbItemPerRow ?? null, 1) : 5
                }

                onActiveFocusChanged: {
                    if (activeFocus) {
                        const item = currentItem?.currentItem ?? currentItem?._getItem(currentIndex) // FIXME: `ExpandGridView` does not have `currentItem`.
                        contentYBehavior.enabled = true
                        Helpers.positionFlickableToContainItem(flickable, item ?? this)
                        contentYBehavior.enabled = false
                    }
                }

                onCurrentIndexChanged: {
                    if (activeFocus) {
                        const item = currentItem?.currentItem ?? currentItem?._getItem(currentIndex) // FIXME: `ExpandGridView` does not have `currentItem`.
                        if (item) {
                            contentYBehavior.enabled = true
                            Helpers.positionFlickableToContainItem(flickable, item)
                            contentYBehavior.enabled = true
                        }
                    }
                }
            }

            Widgets.ViewHeader {
                id: newVideoHeader

                topPadding: VLCStyle.dp(20, VLCStyle.scale)
                // (the list's column header: room under the title, as the grid's
                // cards have; it stood against it, #111)
                bottomPadding: MainCtx.gridView ? 0 : VLCStyle.dp(12, VLCStyle.scale)
                text: qsTr("New Videos")
                visible: newVideoRow.visible
                view: newVideoRow
                seeAllButton.visible: true // The design shows the chevron of every section

                onSeeAllButtonClicked: function (reason) {
                    root.seeAllButtonClicked("newVideo", reason)
                }
            }

            VideoAll {
                id: newVideoRow

                // (latched a moment later, not by a Binding: loading the row changes its count, which the
                // condition reads, and a Binding that set it at once was a binding loop)
                property bool _wanted: false
                readonly property bool _wantNow: !_wanted
                    && (MainCtx.firstFrameSwapped || newVideoRow.focus || flickable.moving || scrollBar.interacting
                        || (newVideoRow.count > 0 && root._sectionMayBeVisible(newVideoHeader)))
                on_WantNowChanged: if (_wantNow) Qt.callLater(newVideoRow._want)
                function _want() { newVideoRow._wanted = true }
                Component.onCompleted: if (_wantNow) _wanted = true
                active: _wanted

                AccessibleCompat.id: "newVideo"

                anchors.left: parent.left
                anchors.right: parent.right

                height: root._rowHeight(this, VLCStyle.dp(217, VLCStyle.scale), VLCStyle.dp(316, VLCStyle.scale), 9 / 16)

                visible: model.count !== 0

                headerPositioning: ListView.InlineHeader
                enableBeginningFade: false
                enableEndFade: false
                sectionProperty: ""

                interactive: false

                emptyLabel: null

                // FIXME: `ExpandGridView` causes extreme performance degradation when `reuseItems`
                //        is true and items provided by the model change (#29084).
                reuseItems: !MainCtx.gridView

                listCoverWidth: root.listCoverWidth
                listCoverHeight: root.listCoverHeight
                listCoverRadius: root.listCoverRadius

                Navigation.parentItem: mediaRows

                model: MLVideoModel {
                    ml: MediaLib

                    sortCriteria: root.sort.criteria || "insertion"
                    sortOrder: root.sort.order
                    searchPattern: root.search.pattern

                    chunkSize: VLCStyle.gridView_maxNbItemPerRow_wide

                    // FIXME: Make limit 0 load no items, instead of loading all items.
                    limit: MainCtx.gridView ? Math.max(newVideoRow.currentItem?.nbItemPerRow ?? null, 1) : 5
                }

                contextMenu: MLContextMenu {
                    model: newVideoRow.model

                    showPlayAsAudioAction: true
                }

                onActiveFocusChanged: {
                    if (activeFocus) {
                        const item = currentItem?.currentItem ?? currentItem?._getItem(currentIndex) // FIXME: `ExpandGridView` does not have `currentItem`.
                        contentYBehavior.enabled = true
                        Helpers.positionFlickableToContainItem(flickable, item ?? this)
                        contentYBehavior.enabled = false
                    }
                }

                onCurrentIndexChanged: {
                    if (activeFocus) {
                        const item = currentItem?.currentItem ?? currentItem?._getItem(currentIndex) // FIXME: `ExpandGridView` does not have `currentItem`.
                        if (item) {
                            contentYBehavior.enabled = true
                            Helpers.positionFlickableToContainItem(flickable, item)
                            contentYBehavior.enabled = false
                        }
                    }
                }
            }

            Widgets.ViewHeader {
                id: newMusicHeader

                topPadding: VLCStyle.dp(20, VLCStyle.scale)
                // (the list's column header: room under the title, as the grid's
                // cards have; it stood against it, #111)
                bottomPadding: MainCtx.gridView ? 0 : VLCStyle.dp(12, VLCStyle.scale)
                text: qsTr("New Music")
                visible: newMusicRow.visible
                view: newMusicRow
                seeAllButton.visible: true // The design shows the chevron of every section

                onSeeAllButtonClicked: function (reason) {
                    root.seeAllButtonClicked("newMusic", reason)
                }
            }

            MediaView {
                id: newMusicRow

                // (latched a moment later, not by a Binding: loading the row changes its count, which the
                // condition reads, and a Binding that set it at once was a binding loop)
                property bool _wanted: false
                readonly property bool _wantNow: !_wanted
                    && (MainCtx.firstFrameSwapped || newMusicRow.focus || flickable.moving || scrollBar.interacting
                        || (newMusicRow.count > 0 && root._sectionMayBeVisible(newMusicHeader)))
                on_WantNowChanged: if (_wantNow) Qt.callLater(newMusicRow._want)
                function _want() { newMusicRow._wanted = true }
                Component.onCompleted: if (_wantNow) _wanted = true
                active: _wanted

                compactMargins: true
                // (the design's own page: its card sizes win)
                designWidthWins: true
                gridCoverWidth: VLCStyle.dp(200, VLCStyle.scale)
                gridCoverHeight: VLCStyle.dp(200, VLCStyle.scale)

                AccessibleCompat.id: "newMusic"

                anchors.left: parent.left
                anchors.right: parent.right

                height: root._rowHeight(this, VLCStyle.dp(268, VLCStyle.scale), VLCStyle.dp(200, VLCStyle.scale), 1)

                visible: model.count !== 0

                listHeaderPositioning: ListView.InlineHeader
                enableBeginningFade: false
                enableEndFade: false
                listSectionProperty: ""

                interactive: false

                emptyLabel: null

                // FIXME: `ExpandGridView` causes extreme performance degradation when `reuseItems`
                //        is true and items provided by the model change (#29084).
                reuseItems: !MainCtx.gridView

                listCoverWidth: root.listCoverWidth
                listCoverHeight: root.listCoverHeight
                listCoverRadius: root.listCoverRadius

                Navigation.parentItem: mediaRows

                model: MLAudioModel {
                    ml: MediaLib

                    sortCriteria: root.sort.criteria || "insertion"
                    sortOrder: root.sort.order
                    searchPattern: root.search.pattern

                    chunkSize: VLCStyle.gridView_maxNbItemPerRow_square

                    // FIXME: Make limit 0 load no items, instead of loading all items.
                    limit: MainCtx.gridView ? Math.max(newMusicRow.currentItem?.nbItemPerRow ?? null, 1) : 5
                }

                onActiveFocusChanged: {
                    if (activeFocus) {
                        const item = currentItem?.currentItem ?? currentItem?._getItem(currentIndex) // FIXME: `ExpandGridView` does not have `currentItem`.
                        contentYBehavior.enabled = true
                        Helpers.positionFlickableToContainItem(flickable, item ?? this)
                        contentYBehavior.enabled = false
                    }
                }

                onCurrentIndexChanged: {
                    if (activeFocus) {
                        const item = currentItem?.currentItem ?? currentItem?._getItem(currentIndex) // FIXME: `ExpandGridView` does not have `currentItem`.
                        if (item) {
                            contentYBehavior.enabled = true
                            Helpers.positionFlickableToContainItem(flickable, item)
                            contentYBehavior.enabled = false
                        }
                    }
                }
            }
        }
    }
}
