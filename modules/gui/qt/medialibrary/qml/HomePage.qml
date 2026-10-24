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

    property var pagePrefix: [] // behave like a Page

    // (its sections list what is recent: the newest first)
    defaultSortOrder: Qt.DescendingOrder

    // (the property of PageExt: one declared again here hid it from the
    // sort of the page, and the header showed no Sort)
    sortModel: [
        { text: qsTr("Alphabetic"), criteria: "title" },
        { text: qsTr("Duration"), criteria: "duration" }
    ]

    // Figma 6261:15078: the media of the library, counted in the header
    libraryItemCount: _libraryMedia.count
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
    function _rowHeight(view, cardHeight) {
        if (!MainCtx.gridView)
            return view.currentItem?.contentHeight ?? view.implicitHeight
        return _cardsTop + cardHeight + _sectionEnd
    }

    function positionContentAtBeginning() {
        contentYBehavior.enabled = true
        flickable.contentY = -flickable.originY
        contentYBehavior.enabled = false
    }

    function setCurrentItemFocus(reason) {
        // `focus` is checked, because that indicates the item that has the focus within the focus scope
        if (continueWatchingRow.focus)
            continueWatchingRow.setCurrentItemFocus(reason)
        else if (listenAgainRow.focus)
            listenAgainRow.setCurrentItemFocus(reason)
        else if (favoritesRow.focus)
            favoritesRow.setCurrentItemFocus(reason)
        else if (newVideoRow.focus)
            newVideoRow.setCurrentItemFocus(reason)
        else if (newMusicRow.focus)
            newMusicRow.setCurrentItemFocus(reason)
        else if (coneNButtons.visible)
            coneNButtons.forceActiveFocus(reason)
        else
            continueWatchingRow.forceActiveFocus(reason)
    }

    contentItem: Flickable {
        id: flickable

        implicitWidth: Math.max(coneNButtons.implicitWidth, mediaRows.implicitWidth)
        implicitHeight: _hasMedias ? mediaRows.implicitHeight : coneNButtons.implicitHeight

        contentWidth: width
        contentHeight: _hasMedias ? mediaRows.implicitHeight
                                  : height - topMargin - bottomMargin

        flickableDirection: Flickable.AutoFlickIfNeeded

        boundsBehavior: Flickable.StopAtBounds

        // FIXME: make the media rows positioners consider the flickable margins when adjusting flickable `contentY`.
        bottomMargin: VLCStyle.margin_large

        pixelAligned: (MainCtx.qtVersion() >= MainCtx.qtVersionCheck(6, 2, 5)) // QTBUG-103996
                      && (Screen.pixelDensity >= VLCStyle.highPixelDensityThreshold) // no need for sub-pixel alignment with high pixel density

        property bool _hasMedias: true

        Binding on _hasMedias {
            when: !continueWatchingRow.model.loading && !listenAgainRow.model.loading
                  && !favoritesRow.model.loading
                  && !newVideoRow.model.loading && !newMusicRow.model.loading
            value: continueWatchingRow.visible || listenAgainRow.visible || foldersRow.visible
                   || favoritesRow.visible || newVideoRow.visible || newMusicRow.visible
            restoreMode: Binding.RestoreNone
        }

        T.ScrollBar.vertical: Widgets.ScrollBarExt {}

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

            visible: !flickable._hasMedias
            focus: visible

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

        Widgets.NavigableCol {
            id: mediaRows

            anchors.top: coneNButtons.visible ? coneNButtons.bottom : parent.top
            // Figma "Frame 20": 17 between the line and the first section
            anchors.topMargin: coneNButtons.visible ? 0 : VLCStyle.dp(17, VLCStyle.scale)
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0 // NOTE: We depend on the top padding of view header currently.

            focus: !coneNButtons.visible

            Navigation.parentItem: root
            Navigation.upItem: coneNButtons.visible ? coneNButtons : root.header

            Widgets.ViewHeader {
                topPadding: VLCStyle.dp(20, VLCStyle.scale)
                bottomPadding: 0
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

                height: root._rowHeight(this, VLCStyle.dp(217, VLCStyle.scale))

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
                bottomPadding: 0
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
                gridCoverWidth: VLCStyle.dp(200, VLCStyle.scale)
                gridCoverHeight: VLCStyle.dp(200, VLCStyle.scale)

                AccessibleCompat.id: "listenAgain"

                anchors.left: parent.left
                anchors.right: parent.right

                height: root._rowHeight(this, VLCStyle.dp(268, VLCStyle.scale))

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
                // folder, as "Group by folder" of the Videos page)
                seeAllButton.visible: true

                onSeeAllButtonClicked: function (reason) {
                    MainCtx.grouping = MainCtx.GROUPING_FOLDER
                    History.push(["video", "all"], reason)
                }
            }

            // Folders of the played media, the most played first
            ListView {
                id: foldersRow

                readonly property real contentLeftMargin: continueWatchingRow.contentLeftMargin
                readonly property real contentRightMargin: continueWatchingRow.contentRightMargin
                readonly property real cardSize: VLCStyle.dp(200, VLCStyle.scale)

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

                model: MLFrequentFoldersModel {
                    ml: MediaLib
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
                            color: folderCard.hovered
                                   ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                                   : VLCStyle.redesign.color("#1E1E1E", "#ababaa")

                            Image {
                                anchors.centerIn: parent
                                width: VLCStyle.dp(48, VLCStyle.scale)
                                height: width
                                source: "qrc:///redesign/icons/folder/large.png"
                                sourceSize: Qt.size(width, height)
                                fillMode: Image.Pad
                                smooth: false
                            }
                        }

                        Text {
                            id: folderName
                            width: foldersRow.cardSize
                            height: VLCStyle.dp(19, VLCStyle.scale)
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
                }
            }

            Widgets.ViewHeader {
                topPadding: VLCStyle.dp(20, VLCStyle.scale)
                bottomPadding: 0
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

                compactMargins: true
                gridCoverWidth: VLCStyle.dp(200, VLCStyle.scale)
                gridCoverHeight: VLCStyle.dp(200, VLCStyle.scale)

                AccessibleCompat.id: "favourites"

                anchors.left: parent.left
                anchors.right: parent.right

                height: root._rowHeight(this, VLCStyle.dp(268, VLCStyle.scale))

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
                topPadding: VLCStyle.dp(20, VLCStyle.scale)
                bottomPadding: 0
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

                AccessibleCompat.id: "newVideo"

                anchors.left: parent.left
                anchors.right: parent.right

                height: root._rowHeight(this, VLCStyle.dp(217, VLCStyle.scale))

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
                topPadding: VLCStyle.dp(20, VLCStyle.scale)
                bottomPadding: 0
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

                compactMargins: true
                gridCoverWidth: VLCStyle.dp(200, VLCStyle.scale)
                gridCoverHeight: VLCStyle.dp(200, VLCStyle.scale)

                AccessibleCompat.id: "newMusic"

                anchors.left: parent.left
                anchors.right: parent.right

                height: root._rowHeight(this, VLCStyle.dp(268, VLCStyle.scale))

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
