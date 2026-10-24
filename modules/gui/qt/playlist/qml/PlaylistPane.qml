/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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
import QtQuick.Window
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl
import QtQuick.Layouts
import QtQml.Models


import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Util
import VLC.Playlist
import VLC.Style

T.Pane {
    id: root

    property var model: PlaylistListModel {
        playlist: MainPlaylistController.playlist
    }
    readonly property ListSelectionModel selectionModel: listView?.selectionModel ?? null

    property bool useAcrylic: true

    // Queue page of the card of the player: compact header and rows, the
    // card draws the background
    property bool card: false

    // Card: a "Queue" title above the count, where no tab names the page
    property bool cardTitle: false

    // The card of the player draws the background
    property bool drawBackground: !card || cardTitle

    // Card: the add button asks for files to queue
    signal addRequested()

    readonly property real minimumWidth: contentItem.Layout.minimumWidth +
                                         leftPadding +
                                         rightPadding +
                                         2 * (VLCStyle.margin_xsmall)

    readonly property ListView listView: contentItem.listView

    readonly property var searchBox: headerColumn.searchBox

    property alias contextMenu: contextMenu

    property alias dragItem: dragItem

    property alias isDropAcceptableFunc: listView.isDropAcceptableFunc
    property alias acceptDropFunc: listView.acceptDropFunc

    property alias view: listView

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    verticalPadding: VLCStyle.margin_normal

    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Play Queue")
    AccessibleCompat.id: "playqueuePane"

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        palette: VLCStyle.palette
        colorSet: ColorContext.View

        focused: root.activeFocus
        hovered: root.hovered
        enabled: root.enabled
    }

    Widgets.DragItem {
        id: dragItem

        view: root.listView

        onRequestData: (indexes, resolve, reject) => {
            resolve(indexes.map((index) => {
                const item = root.model.itemAt(index)
                return {
                    "title": item.title,
                    "cover": (!!item.artwork && item.artwork.toString() !== "") ? item.artwork : VLCStyle.noArtAlbumCover,
                    "url": item.url
                }
            }))
        }

        onRequestInputItems: (indexes, data, resolve, reject) => {
            resolve(root.model.getItemsForIndexes((indexes?.length > 0) ? indexes : root.selectionModel.selectedIndexesFlat))
        }
    }

    PlaylistContextMenu {
        id: contextMenu
        model: root.model
        proxyModel: (root.contentItem?.listView?.model instanceof AbstractProxyModel) ? root.contentItem.listView.model
                                                                                        : null
        selectionModel: root.selectionModel
        controler: MainPlaylistController
        ctx: MainCtx

        onJumpToCurrentPlaying: listView.positionViewAtIndex( MainPlaylistController.currentIndex, ItemView.Center)
    }

    background: Rectangle {
        color: root.drawBackground ? VLCStyle.redesign.color(VLCStyle.redesign.sidebarBackground, "#c2c2c1", VLCStyle.palette.isDark)
                                   : "transparent"
    }

    contentItem: ColumnLayout {
        spacing: VLCStyle.margin_xxsmall

        Layout.minimumWidth: Math.min(Math.max(noContentInfoColumn.implicitWidth,
                                               headerColumn.titleWidth ),
                                      /* Minimum width should not be too large, we rely on elision otherwise: */
                                      Window.width / 3) + (headerColumn.Layout.leftMargin + headerColumn.Layout.rightMargin)

        readonly property ListView listView: listView

        QueueHeader {
            id: headerColumn
            visible: !root.card
            listView: root.listView
            contextMenu: root.contextMenu
            navigationParent: root
        }

        Item {
            // Spacer
            visible: !root.card

            implicitHeight: VLCStyle.margin_xsmall
        }

        QueueColumns {
            visible: !root.card
            listView: root.listView
        }

        Text {
            visible: root.card && root.cardTitle
            Layout.fillWidth: true
            Layout.leftMargin: VLCStyle.dp(18, VLCStyle.scale)
            Layout.bottomMargin: VLCStyle.dp(4, VLCStyle.scale)
            text: qsTr("Queue")
            elide: Text.ElideRight
            // (the right in a right to left layout)
            horizontalAlignment: Text.AlignLeft
            color: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, theme.fg.primary)
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(18, VLCStyle.scale)
        }

        // Card: count and length, then the queue actions
        RowLayout {
            id: cardHeader

            visible: root.card
            Layout.fillWidth: true
            Layout.leftMargin: VLCStyle.dp(18, VLCStyle.scale)
            Layout.rightMargin: VLCStyle.dp(12, VLCStyle.scale)
            Layout.bottomMargin: VLCStyle.dp(6, VLCStyle.scale)
            spacing: VLCStyle.dp(4, VLCStyle.scale)

            Text {
                Layout.fillWidth: true
                text: {
                    const count = root.model.count
                    if (count === 0)
                        return qsTr("No items")
                    return qsTr("%1 item", "%1 items", count).arg(count) + " - " + root.model.duration.formatLong()
                }
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, theme.fg.secondary)
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            }

            Widgets.SortControl {
                id: cardSort

                description: qsTr("Sort")
                enabled: MainPlaylistController.count > 1
                checked: MainPlaylistController.sortKey !== PlaylistController.SORT_KEY_NONE
                popupAbove: false
                model: MainPlaylistController.sortKeyTitleList
                sortKey: MainPlaylistController.sortKey
                sortOrder: MainPlaylistController.sortOrder === PlaylistController.SORT_ORDER_DESC
                           ? Qt.DescendingOrder : Qt.AscendingOrder

                contentItem: Image {
                    id: sortIcon
                    // sorted: a dot in the accent, with another accent tinted
                    // above the grey arrows
                    readonly property bool twoTone: cardSort.checked && !VLCStyle.redesign.defaultAccent
                    source: "qrc:///redesign/icons/sort/" + (twoTone ? "active-grey"
                                                             : cardSort.checked ? "active" : "default") + ".png"
                    sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                    fillMode: Image.Pad
                    opacity: cardSort.enabled ? 1 : 0.4
                    Impl.ColorImage {
                        anchors.fill: parent
                        visible: sortIcon.twoTone
                        source: visible ? "qrc:///redesign/icons/sort/active-accent.png" : ""
                        sourceSize: parent.sourceSize
                        fillMode: parent.fillMode
                        color: VLCStyle.redesign.accent
                    }
                }
                background: CardButtonBackground { button: cardSort }

                onSortSelected: key => {
                    MainPlaylistController.sortKey = key
                    MainPlaylistController.sort()
                }
                onSortOrderSelected: type => {
                    MainPlaylistController.sortOrder = (type === Qt.DescendingOrder)
                        ? PlaylistController.SORT_ORDER_DESC : PlaylistController.SORT_ORDER_ASC
                    MainPlaylistController.sort()
                }
            }

            CardGlyphButton {
                text: MainPlaylistController.repeatMode === PlaylistController.PLAYBACK_REPEAT_CURRENT
                      ? VLCIcons.repeat_one : VLCIcons.repeat_all
                description: qsTr("Repeat")
                checked: MainPlaylistController.repeatMode !== PlaylistController.PLAYBACK_REPEAT_NONE
                onClicked: MainPlaylistController.toggleRepeatMode()
            }

            CardGlyphButton {
                text: VLCIcons.shuffle
                description: qsTr("Shuffle")
                checked: MainPlaylistController.random
                onClicked: MainPlaylistController.toggleRandom()
            }

            CardImageButton {
                iconName: "remove"
                description: qsTr("Remove selected items")
                enabled: !!root.selectionModel && root.selectionModel.hasSelection
                onClicked: {
                    const items = listView.mappedSelectedIndexes()
                    if (items.length > 0)
                        root.model.removeItems(items)
                }
            }

            CardImageButton {
                iconName: "add"
                description: qsTr("Add files to the queue")
                onClicked: root.addRequested()
            }
        }

        Widgets.ListViewExt {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true

            selectionModel: ListSelectionModel {
                model: listView.model
            }

            focus: true

            clip: !fadingEdge.implicitClipping && (height < contentHeight)

            model: proxyModel ?? root.model

            property QtSortFilterProxyModel proxyModel

            function adjustFiltering() {
                // Once text changes, we switch to the proxy model.
                // We do not switch back to the source model after that.
                if (!proxyModel) {
                    proxyModel = proxyModelComponent.createObject(listView)
                }

                if (searchBox.regexButtonToggled)
                    proxyModel.setFilterRegularExpression(searchBox.searchPattern)
                else
                    proxyModel.setFilterFixedString(searchBox.searchPattern)
            }

            Component {
                id: proxyModelComponent

                QtSortFilterProxyModel {
                    filterRole: PlaylistListModel.FilterRole

                    sourceModel: root.model

                    filterCaseSensitivity: searchBox.caseSensitiveButtonToggled ? Qt.CaseSensitive
                                                                                : Qt.CaseInsensitive
                }
            }

            fadingEdge.backgroundColor: (root.background && (root.background.color.a >= 1.0)) ? root.background.color
                                                                                              : Qt.alpha(root.background.color, 0.0)

            function mappedSelectedIndexes() : var {
                let mappedSelectedIndexes

                if (!listView.selectionModel)
                    return [] // unlikely

                const selectedIndexes = listView.selectionModel.selectedIndexesFlat
                if (listView.proxyModel && listView.model === listView.proxyModel) {
                    mappedSelectedIndexes = []
                    for (let i = 0; i < selectedIndexes.length; ++i) {
                        const mappedIndex = listView.model.mapToSource(listView.model.index(selectedIndexes[i], 0))
                        mappedSelectedIndexes.push(mappedIndex.row)
                    }
                } else {
                    mappedSelectedIndexes = selectedIndexes
                }

                return mappedSelectedIndexes
            }

            isDropAcceptableFunc: function(drop, index) {
                if (drop.source === dragItem)
                    return Helpers.itemsMovable(listView.mappedSelectedIndexes(), index)
                else if (Helpers.isValidInstanceOf(drop.source, Widgets.DragItem))
                    return true
                else if (drop.hasUrls)
                    return true
                else
                    return false
            }

            acceptDropFunc: function(index, drop) {
                const item = drop.source;

                // NOTE: Move implementation.
                if (dragItem === item) {
                    root.model.moveItemsPre(listView.mappedSelectedIndexes(), index);
                    listView.forceActiveFocus();
                // NOTE: Dropping medialibrary content into the queue.
                } else if (Helpers.isValidInstanceOf(item, Widgets.DragItem)) {
                    return item.getSelectedInputItem().then((inputItems) => {
                            if (!Helpers.isArray(inputItems) || inputItems.length === 0) {
                                console.warn("can't convert items to input items");
                                return
                            }
                            MainPlaylistController.insert(index, inputItems, false)
                        }).then(() => { listView.forceActiveFocus(); })
                // NOTE: Dropping an external item (i.e. filesystem) into the queue.
                } else if (drop.hasUrls) {
                    const urlList = [];

                    for (let url in drop.urls)
                        urlList.push(drop.urls[url]);

                    MainPlaylistController.insert(index, urlList, false);

                    // NOTE This is required otherwise backend may handle the drop as well yielding double addition.
                    drop.accept(Qt.IgnoreAction);
                    listView.forceActiveFocus();
                }

                return Promise.resolve()
            }

            property int shiftIndex: -1

            onShowContextMenu: (globalPos) => {
                contextMenu.popup(-1, globalPos)
            }

            Behavior on contentY {
                id: contentYBehavior

                enabled: false

                // NOTE: Usage of `SmoothedAnimation` is intentional here.
                SmoothedAnimation {
                    duration: VLCStyle.duration_veryLong
                    easing.type: Easing.InOutSine
                }
            }

            Connections {
                target: MainPlaylistController
                // Track the current item; disconnect when this queue view is destroyed.
                function onCurrentItemChanged() {
                    // FIXME: Qt does not provide the `contentY` with `positionViewAtIndex()` for us
                    //        to animate. For that reason, we capture the new `contentY`, adjust
                    //        `contentY` to it is old value then enable the animation and set `contentY`
                    //        to its new value.
                    const oldContentY = listView.contentY
                    listView.positionViewAtIndex(MainPlaylistController.currentIndex, ListView.Contain)
                    const newContentY = listView.contentY
                    if (Math.abs(oldContentY - newContentY) >= Number.EPSILON) {
                        contentYBehavior.enabled = false
                        listView.contentY = oldContentY
                        contentYBehavior.enabled = true
                        listView.contentY = newContentY
                        contentYBehavior.enabled = false
                    }
                }
            }

            Component.onCompleted: {
                searchBox.searchPatternChanged.connect(listView, listView.adjustFiltering)
                searchBox.regexButtonToggledChanged.connect(listView, listView.adjustFiltering)
            }

            Connections {
                target: listView.model

                function onRowsInserted() {
                    if (listView.currentIndex === -1)
                        listView.currentIndex = 0
                }

                function onModelReset() {
                    if (listView.currentIndex === -1 && listView.count > 0)
                        listView.currentIndex = 0
                }
            }

            // "Show the playlist as a tree" (Figma 3046:10057): the media
            // under the folder (or the host) they come from; the queue of
            // VLC 4 is flat, the folders are headings and the rows move,
            // select and drop as they did
            section.property: MainCtx.playlistTree ? "folder" : ""
            section.delegate: Item {
                id: folderHeading

                required property string section

                width: listView.contentWidth
                height: section.length > 0 ? VLCStyle.dp(32, VLCStyle.scale) : 0
                visible: height > 0

                Impl.ColorImage {
                    id: folderIcon
                    // (from the right in a right to left layout)
                    anchors.left: parent.left
                    anchors.leftMargin: root.card ? VLCStyle.dp(14, VLCStyle.scale) : VLCStyle.margin_normal
                    anchors.verticalCenter: parent.verticalCenter
                    width: VLCStyle.dp(20, VLCStyle.scale)
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: "qrc:///redesign/icons/folder/default.png"
                    color: VLCStyle.palette.isDark ? "transparent" : "#2e2e2e"
                }
                Text {
                    anchors.left: folderIcon.right
                    anchors.leftMargin: VLCStyle.dp(8, VLCStyle.scale)
                    anchors.right: parent.right
                    anchors.rightMargin: VLCStyle.margin_normal
                    anchors.verticalCenter: parent.verticalCenter
                    // the folder's name
                    text: {
                        const parts = folderHeading.section.split(/[\\/]/).filter(part => part.length > 0)
                        return parts.length > 0 ? parts[parts.length - 1] : folderHeading.section
                    }
                    color: VLCStyle.redesign.color("#6B6B6B", "#4d4d4d")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
                    elide: Text.ElideMiddle
                    horizontalAlignment: Text.AlignLeft
                }
            }

            delegate: PlaylistDelegate {
                id: delegate

                width: listView.contentWidth
                compact: root.card
                // the room of the scroll bar at the end: on the left in a
                // right to left layout, where the bar is
                readonly property real _startPadding: root.card ? VLCStyle.dp(14, VLCStyle.scale) : VLCStyle.margin_normal
                readonly property real _endPadding: Math.max(listView.T.ScrollBar.vertical.width, VLCStyle.margin_normal)
                leftPadding: mirrored ? _endPadding : _startPadding
                rightPadding: mirrored ? _startPadding : _endPadding

                contextMenu: root.contextMenu

                dragItem: root.dragItem

                isDropAcceptable: listView.isDropAcceptableFunc
                acceptDrop: listView.acceptDropFunc

                onContainsDragChanged: listView.updateItemContainsDrag(this, containsDrag)

                onIsCurrentChanged: {
                    if (isCurrent)
                        listView.fadingEdge.excludeItem = delegate
                    else if (listView.fadingEdge.excludeItem === delegate)
                        listView.fadingEdge.excludeItem = null
                }

                Component.onCompleted: {
                    listView.delegateInstantiated(delegate)
                }
            }

            Keys.onDeletePressed: {
                let items

                const selectedIndexes = selectionModel.selectedIndexesFlat
                if (listView.proxyModel && listView.model === listView.proxyModel) {
                    items = []
                    for (let i = 0; i < selectedIndexes.length; ++i) {
                        const mappedIndex = listView.proxyModel.mapToSource(listView.proxyModel.index(selectedIndexes[i], 0))
                        items.push(mappedIndex.row)
                    }
                } else {
                    items = selectedIndexes
                }

                if (items.length > 0)
                    root.model.removeItems(items)
            }

            Navigation.parentItem: root
            Navigation.upItem: headerColumn.sortButton

            onActionAtIndex: (index) => {
                if (index < 0)
                    return

                const sourceIndex = listView.proxyModel
                    ? listView.proxyModel.mapToSource(listView.proxyModel.index(index, 0)).row
                    : index
                if (sourceIndex >= 0) MainPlaylistController.goTo(sourceIndex, true)
            }

            Column {
                id: noContentInfoColumn

                anchors.centerIn: parent

                visible: false
                enabled: visible

                opacity: (listView.activeFocus) ? 1.0 : 0.4

                Binding on visible {
                    delayed: true
                    value: (listView.count === 0 && !listView.footerItem.firstItemIndicatorVisible)
                }

                Widgets.IconLabel {
                    id: label

                    anchors.horizontalCenter: parent.horizontalCenter

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    text: VLCIcons.playlist

                    color: theme.fg.primary

                    font.pixelSize: VLCStyle.dp(48, VLCStyle.scale)
                }

                T.Label {
                    anchors.topMargin: VLCStyle.margin_xlarge

                    anchors.horizontalCenter: parent.horizontalCenter

                    // a longer text (a translation) on more lines
                    width: listView.width > 0 ? Math.min(implicitWidth, listView.width - 2 * VLCStyle.margin_normal) : implicitWidth
                    wrapMode: Text.Wrap

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    text: (listView.proxyModel &&
                           listView.model === listView.proxyModel &&
                           listView.count !== root.model.count) ? qsTr("No results")
                                                                : qsTr("No content yet")

                    color: label.color

                    font.pixelSize: VLCStyle.fontSize_xxlarge
                }

                T.Label {
                    anchors.topMargin: VLCStyle.margin_normal

                    width: listView.width > 0 ? Math.min(implicitWidth, listView.width - 2 * VLCStyle.margin_normal) : implicitWidth
                    wrapMode: Text.Wrap

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    text: qsTr("Drag & Drop some content here!")
                    visible: (listView.proxyModel && listView.model === listView.proxyModel) ? (listView.count === root.model.count)
                                                                                             : true

                    color: label.color

                    font.pixelSize: VLCStyle.fontSize_large
                }
            }
        }
    }

    Keys.priority: Keys.AfterItem
    Keys.forwardTo: listView
    Keys.onPressed: (event) => root.Navigation.defaultKeyAction(event)

    component CardButtonBackground: Rectangle {
        required property T.AbstractButton button

        radius: VLCStyle.redesign.controlRadius
        color: (button.hovered || button.down) ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground,
                                                                         "#9c9c9b")
                                               : "transparent"
        border.width: button.visualFocus ? 1 : 0
        border.color: VLCStyle.redesign.focusColor
    }

    // Icons of the design for the card header
    component CardImageButton: Widgets.IconToolButton {
        id: imageButton

        property string iconName

        text: description
        implicitWidth: VLCStyle.dp(32, VLCStyle.scale)
        implicitHeight: VLCStyle.dp(32, VLCStyle.scale)

        contentItem: Image {
            source: "qrc:///redesign/icons/" + imageButton.iconName + "/default.png"
            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
            fillMode: Image.Pad
            opacity: imageButton.enabled ? 1 : 0.4
        }
        background: CardButtonBackground { button: imageButton }
    }

    // Repeat and shuffle: glyphs of the icon font, accent while on
    component CardGlyphButton: Widgets.IconToolButton {
        id: glyphButton

        implicitWidth: VLCStyle.dp(32, VLCStyle.scale)
        implicitHeight: VLCStyle.dp(32, VLCStyle.scale)
        font.pixelSize: VLCStyle.dp(20, VLCStyle.scale)
        color: checked ? VLCStyle.redesign.accent
                       : VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#5a5a59")
        background: CardButtonBackground { button: glyphButton }
    }

    component PlaylistToggleButton : Widgets.IconToolButton {
        id: playlistBtn

        focus: true

        font.pixelSize: VLCStyle.icon_banner
        text: VLCIcons.playlist
        description: qsTr("Play queue")

        AccessibleCompat.id: "playqueueToggle"

        width: VLCStyle.bannerButton_width
        height: VLCStyle.bannerButton_height
        //highlighted: MainCtx.playlistVisible

        property Item plListView

        DropArea {
            anchors.fill: parent

            onContainsDragChanged: {
                if (containsDrag) {
                    _timer.restart()

                    if (playlistBtn.plListView)
                        MainCtx.setCursor(Qt.DragCopyCursor)
                } else {
                    _timer.stop()

                    if (playlistBtn.plListView)
                        MainCtx.restoreCursor()
                }
            }

            onEntered: (drag) => {
                const plListView = playlistBtn.plListView
                if (plListView) {
                    console.assert(plListView.isDropAcceptableFunc)
                    console.assert(plListView.model)
                    if (plListView.isDropAcceptableFunc(drag, plListView.model.count)) {
                        drag.accept()
                    } else {
                        drag.accepted = false
                    }
                } else {
                    drag.accepted = false
                }
            }

            onDropped: (drop) => {
                const plListView = playlistBtn.plListView
                if (plListView) {
                    console.assert(plListView.acceptDropFunc)
                    plListView.acceptDropFunc(plListView.model.count, drop)
                }
            }

            Timer {
                id: _timer
                interval: VLCStyle.duration_humanMoment

                onTriggered: {
                    MainCtx.playqueuePanel.visible = true
                }
            }
        }
    }
}
