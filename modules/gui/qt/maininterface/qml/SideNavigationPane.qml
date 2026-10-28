/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import VLC.Style
import VLC.Widgets as Widgets
import VLC.MainInterface
import VLC.Dialogs
import VLC.Util

T.Pane {
    id: root

    signal itemClicked(var uri, Item item)
    property int safeAreaLeftMargin: 0
    property int safeAreaRightMargin: 0
    property bool useAcrylic: true
    property bool delegateShowText: true
    property alias model: navigationModel
    // The width of the design, more for a longer label (a translation longer
    // than the English), up to half as much again; past it, the labels elide
    readonly property int minimumWidth: Math.max(VLCStyle.expandNavigationPaneWidth,
                                                 Math.min(Math.ceil(_labelsWidth), Math.round(VLCStyle.expandNavigationPaneWidth * 1.5)))

    // The width the longest label of the rows needs (all of them, the pages
    // of Music too: the width does not change with the page shown)
    readonly property real _labelsWidth: {
        void labelMetrics.font // (measured again once the font is loaded)
        let widest = 0
        for (const label of [ qsTr("Home"), qsTr("Videos"), qsTr("Music"), qsTr("Network"), qsTr("TV"),
                              qsTr("Playlists"), qsTr("Recent"), qsTr("Add item") ])
            widest = Math.max(widest, labelMetrics.advanceWidth(label))
        for (const label of [ qsTr("Artists"), qsTr("Albums"), qsTr("Tracks"), qsTr("Genres"), qsTr("Playlists") ])
            widest = Math.max(widest, labelMetrics.advanceWidth(label) + VLCStyle.dp(16, VLCStyle.scale))
        // the paddings of the pane and the border of a resize handle (not
        // read from the paddings: they depend on the width), the paddings of
        // the rows, the icon and the spacing (see the delegates)
        return widest + 1 + 2 * padding + VLCStyle.border + safeAreaLeftMargin + safeAreaRightMargin
               + VLCStyle.dp(16 + 16 + 32, VLCStyle.scale) + VLCStyle.margin_xsmall
    }

    FontMetrics {
        id: labelMetrics
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.redesign.fontNormal
    }

    padding: VLCStyle.margin_xsmall
    implicitWidth: minimumWidth
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding
    Accessible.name: qsTr("Sidebar")

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.Window
    }

    // Retain the controller model for the existing navigation context menu.
    NavigationModel {
        id: navigationModel
        hasMedialib: MainCtx.mediaLibraryAvailable
    }

    // The pages of Music listed under it (a bool: the list is made again
    // only when this changes, not on every navigation)
    readonly property bool _musicExpanded: History.viewPath[0] === "music" && delegateShowText

    readonly property var entries: {
        const rows = [
            {title: qsTr("Home"), uri: ["home"], icon: VLCIcons.home, asset: "home"},
            {title: qsTr("Videos"), uri: ["video", "all"], icon: VLCIcons.topbar_video, asset: "video", library: true},
            {title: qsTr("Music"), uri: ["music"], icon: VLCIcons.topbar_music, asset: "music", library: true}
        ]
        if (_musicExpanded) {
            for (const entry of [ ["artists", qsTr("Artists")], ["albums", qsTr("Albums")],
                                  ["tracks", qsTr("Tracks")], ["genres", qsTr("Genres")],
                                  ["playlists", qsTr("Playlists")] ])
                rows.push({title: entry[1], uri: ["music", entry[0]], icon: "", asset: "", library: true, depth: 1})
        }
        rows.push({title: qsTr("Network"), uri: ["network"], icon: VLCIcons.topbar_network, asset: "network"})
        // Live TV, while its add-on is there and on
        if (Iptv.enabled)
            rows.push({title: qsTr("TV"), uri: ["tv"], icon: "", asset: "tv"})
        // every playlist (the page lists songs ones too, see VideoPlaylistsDisplay)
        rows.push({title: qsTr("Playlists"), uri: ["video", "playlists"], icon: VLCIcons.playlist, asset: "playlists", library: true})
        // the media played last, videos and songs
        rows.push({title: qsTr("Recent"), uri: ["home", "recent"], icon: VLCIcons.history, asset: "recent", library: true})
        return rows
    }

    // The entry of the page shown (-1: none, e.g. a folder page)
    property int matchedIndex: -1
    function updateCurrentIndex() {
        let matched = -1
        for (let i = 0; i < entries.length; ++i) {
            if (History.match(History.viewPath, entries[i].uri))
                matched = i
        }
        matchedIndex = matched
        listView.currentIndex = matched
    }
    onEntriesChanged: Qt.callLater(updateCurrentIndex)
    Connections {
        target: History
        function onViewPathChanged() { root.updateCurrentIndex() }
    }

    background: Rectangle {
        color: VLCStyle.redesign.color(VLCStyle.redesign.windowBackground, "#c2c2c1")
        Rectangle {
            anchors.fill: parent
            anchors.margins: VLCStyle.margin_xsmall
            radius: VLCStyle.redesign.controlRadius
            color: VLCStyle.redesign.color(VLCStyle.redesign.sidebarBackground, "#ababaa")
        }
    }

    // Figma "Frame 110": padding 12 16, rows of 32, gap 14
    readonly property real _rowHeight: VLCStyle.dp(32, VLCStyle.scale)
    readonly property real _rowSpacing: VLCStyle.dp(14, VLCStyle.scale)

    contentItem: ColumnLayout {
        spacing: 0

        Widgets.ListViewExt {
            id: listView
            Layout.fillWidth: true
            Layout.topMargin: VLCStyle.dp(12, VLCStyle.scale)
            Layout.preferredHeight: Math.min(contentHeight, root.availableHeight - Layout.topMargin
                                             - addItem.implicitHeight - addItem.Layout.topMargin)
            spacing: root._rowSpacing
            clip: true
            focus: true
            model: root.entries
            colorContext.colorSet: ColorContext.Window
            Navigation.parentItem: root
            Navigation.downItem: addItem

            delegate: SideNavigationDelegate {
                required property var modelData
                required property int index
                readonly property var uri: modelData.uri
                readonly property bool expandable: false
                readonly property int depth: modelData.depth ?? 0
                navigationUri: uri
                navigationDepth: depth
                width: ListView.view.width
                height: root._rowHeight
                topPadding: 0
                bottomPadding: 0
                // the indent at the start of the row: on the right in a
                // right to left layout
                readonly property real _startPadding: root.safeAreaLeftMargin + VLCStyle.dp(root.delegateShowText ? 16 + depth * 16 : 8, VLCStyle.scale)
                readonly property real _endPadding: root.safeAreaRightMargin + VLCStyle.dp(16, VLCStyle.scale)
                leftPadding: LayoutMirroring.enabled ? _endPadding : _startPadding
                rightPadding: LayoutMirroring.enabled ? _startPadding : _endPadding
                leftInset: (LayoutMirroring.enabled ? root.safeAreaRightMargin : root.safeAreaLeftMargin) + VLCStyle.dp(8, VLCStyle.scale)
                rightInset: (LayoutMirroring.enabled ? root.safeAreaLeftMargin : root.safeAreaRightMargin) + VLCStyle.dp(8, VLCStyle.scale)
                // Figma "Icon Wrapper": 30 wide, 32 for Videos
                iconSlotWidth: VLCStyle.dp(modelData.asset === "video" ? 32 : 30, VLCStyle.scale)
                iconTxt: modelData.icon
                referenceIconName: modelData.asset
                text: modelData.title
                showText: root.delegateShowText
                enabled: !modelData.library || MainCtx.mediaLibraryAvailable
                // the page shown (not the keyboard position: a folder page has
                // none, the list would keep the last one)
                checked: index === root.matchedIndex
                // Figma 970:4028: a folder page shows the Videos icon filled
                contextual: modelData.asset === "video" && History.viewPath[0] === "folder"
                onClicked: {
                    // The navigation may rebuild the delegates: the view is
                    // updated first
                    listView.currentIndex = index
                    root.matchedIndex = index
                    listView.forceActiveFocus(Qt.MouseFocusReason)
                    root.itemClicked(uri, this)
                }
            }
        }

        SideNavigationDelegate {
            id: addItem
            Layout.fillWidth: true
            Layout.topMargin: root._rowSpacing
            Layout.preferredHeight: root._rowHeight
            topPadding: 0
            bottomPadding: 0
            // (mirrored like the rows above)
            readonly property real _startPadding: root.safeAreaLeftMargin + VLCStyle.dp(root.delegateShowText ? 16 : 8, VLCStyle.scale)
            readonly property real _endPadding: root.safeAreaRightMargin + VLCStyle.dp(16, VLCStyle.scale)
            leftPadding: LayoutMirroring.enabled ? _endPadding : _startPadding
            rightPadding: LayoutMirroring.enabled ? _startPadding : _endPadding
            leftInset: (LayoutMirroring.enabled ? root.safeAreaRightMargin : root.safeAreaLeftMargin) + VLCStyle.dp(8, VLCStyle.scale)
            rightInset: (LayoutMirroring.enabled ? root.safeAreaLeftMargin : root.safeAreaRightMargin) + VLCStyle.dp(8, VLCStyle.scale)
            iconSlotWidth: VLCStyle.dp(30, VLCStyle.scale)
            iconTxt: VLCIcons.add
            referenceIconName: "add"
            text: qsTr("Add item")
            showText: root.delegateShowText
            focusPolicy: Qt.StrongFocus
            onClicked: DialogsProvider.simpleOpenDialog()
            Navigation.parentItem: root
            Navigation.upItem: listView
        }

        Item { Layout.fillHeight: true }
        Loader {
            Layout.fillWidth: true
            active: MainCtx.mediaLibraryAvailable && !MainCtx.mediaLibrary.idle
            visible: active
            source: "qrc:///qt/qml/VLC/MediaLibrary/ScanProgressBar.qml"
        }
    }
}
