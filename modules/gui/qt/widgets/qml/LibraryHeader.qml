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
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Style
import VLC.Menus
import VLC.Util
import VLC.Widgets as Widgets

T.ToolBar {
    id: root
    objectName: "libraryHeader"

    required property string title
    // Parts of an address line, each opens its place (crumbClicked), the last
    // one is the page itself; the title is shown when there are none
    property var crumbs: null
    property int itemCount: -1

    signal crumbClicked(int index)
    property Component headerAction: null
    property SortMenu sortMenu: null
    required property SearchCtx search
    required property SortCtx sort

    readonly property bool compact: width < VLCStyle.dp(760, VLCStyle.scale) ||
        (actionLoader.active && availableWidth < actionLoader.implicitWidth + VLCStyle.dp(680, VLCStyle.scale))
    // compact: the group at the right of the design (count, sort, view,
    // search, more) under the address; an action of buttons gets a row of
    // its own
    readonly property bool _actionRow: actionLoader.active
        && actionLoader.implicitWidth > VLCStyle.dp(120, VLCStyle.scale)
    // compact: the room of each crumb (the row less the arrows and 10, the
    // separators 8 with 6 on each side)
    readonly property real _crumbShare: (crumbs && crumbs.length > 0)
        ? Math.max(VLCStyle.dp(24, VLCStyle.scale),
                   (availableWidth - VLCStyle.dp(24 + 12 + 24 + 10, VLCStyle.scale)
                    - (crumbs.length - 1) * VLCStyle.dp(6 + 8 + 6, VLCStyle.scale)) / crumbs.length)
        : 0
    // MainDisplay (historyBack, historyForward), when the header is in it
    readonly property var _historyHost: typeof g_mainDisplay !== "undefined" ? g_mainDisplay : null
    readonly property string breadcrumb: {
        const detail = History.viewProp && History.viewProp.title ? History.viewProp.title : ""
        return detail && detail !== title ? title + "  ▸  " + detail : title
    }

    position: T.ToolBar.Header
    // Figma "Frame 20": padding 20; the row of 34, the line 17 under it
    // (1 px centred on the next pixel boundary: two rows)
    leftPadding: VLCStyle.dp(20, VLCStyle.scale)
    rightPadding: leftPadding
    topPadding: VLCStyle.dp(20, VLCStyle.scale)
    bottomPadding: VLCStyle.dp(17, VLCStyle.scale)
    implicitHeight: implicitContentHeight + topPadding + bottomPadding
    implicitWidth: implicitContentWidth + leftPadding + rightPadding

    // The baseline of a text box of the design (line height of Inter) centred
    // in a box, on a whole pixel
    function _topPadding(box, pixelSize, ascent, down) {
        const line = Math.round(1.2109375 * pixelSize)
        const top = down === false ? Math.floor((box - line) / 2) : Math.round((box - line) / 2)
        return top + (line - 1.2109375 * pixelSize) / 2 + 0.96875 * pixelSize - ascent
    }
    readonly property real _rowHeight: VLCStyle.dp(34, VLCStyle.scale)

    // The names of the design for the orders of the criteria (ascending,
    // descending)
    readonly property var _sortNames: ({
        "unwatched": [qsTr("Unwatched First"), qsTr("Watched First")],
        "insertion": [qsTr("Oldest First"), qsTr("Newest First")],
        "date": [qsTr("Oldest First"), qsTr("Newest First")],
        "title": [qsTr("Title A-Z"), qsTr("Title Z-A")],
        "name": [qsTr("Title A-Z"), qsTr("Title Z-A")],
        "main_artist": [qsTr("Artist A-Z"), qsTr("Artist Z-A")],
        "artist": [qsTr("Artist A-Z"), qsTr("Artist Z-A")],
        "album_title": [qsTr("Album A-Z"), qsTr("Album Z-A")],
        "track_number": [qsTr("Track Number Ascending"), qsTr("Track Number Descending")],
        "mrl": [qsTr("URI Ascending"), qsTr("URI Descending")],
        "url": [qsTr("URI Ascending"), qsTr("URI Descending")],
        "disc_number": [qsTr("Disc Number Ascending"), qsTr("Disc Number Descending")]
    })
    // "Newest First" before "Oldest First", as the design lists them
    readonly property var _sortOptions: {
        const options = []
        const model = root.sort.model ?? []
        for (let i = 0; i < model.length; ++i) {
            const entry = model[i]
            const names = root._sortNames[entry.criteria]
            const newestFirst = entry.criteria === "insertion" || entry.criteria === "date"
            const orders = newestFirst ? [Qt.DescendingOrder, Qt.AscendingOrder]
                                       : [Qt.AscendingOrder, Qt.DescendingOrder]
            for (const order of orders) {
                const text = names ? names[order === Qt.AscendingOrder ? 0 : 1]
                                   : entry.text + (order === Qt.AscendingOrder ? " \u2191" : " \u2193")
                options.push({ text: text, criteria: entry.criteria, order: order })
            }
        }
        return options
    }

    // Figma "Sorting Options": 265 wide, padding 4, radius 10, #191919 95 %,
    // shadow 0 8 blur 15 #000000 46 %
    Widgets.RedesignMenu {
        id: sortOptions
        menuWidth: VLCStyle.dp(265, VLCStyle.scale)
        delegate: Widgets.RedesignOptionItem { }

        Instantiator {
            model: root._sortOptions
            delegate: Widgets.RedesignOptionItem {
                required property var modelData
                text: modelData.text
                // no criteria: the page sorts by its default one (the first),
                // still in the order kept (a page kept in descending order
                // shows "Title Z-A")
                checked: (root.sort.criteria === "" ? root._sortOptions[0]?.criteria ?? ""
                                                     : root.sort.criteria) === modelData.criteria
                         && root.sort.order === modelData.order
                required property int index
                onTriggered: {
                    root.sort.criteria = modelData.criteria
                    root.sort.order = modelData.order
                }
            }
            onObjectAdded: (index, object) => sortOptions.insertItem(index, object)
            onObjectRemoved: (index, object) => sortOptions.removeItem(object)
        }
    }

    // The grouping of the videos, when the page gives a SortMenuVideo (the
    // sort menu of VLC lists it after the orders): a separator, then "Do not
    // group videos", "Group by name" and "Group by folder"
    readonly property bool _videoGrouping: root.sortMenu instanceof SortMenuVideo
    property var _groupingItems: []

    function _updateGroupingItems() {
        // (removeItem destroys the item)
        for (const item of _groupingItems)
            sortOptions.removeItem(item)
        _groupingItems = []
        if (!_videoGrouping)
            return
        const items = [groupingSeparator.createObject(sortOptions)]
        for (const entry of [[qsTr("Do not group videos"), MainCtx.GROUPING_NONE],
                             [qsTr("Group by name"), MainCtx.GROUPING_NAME],
                             [qsTr("Group by folder"), MainCtx.GROUPING_FOLDER]])
            items.push(groupingOption.createObject(sortOptions, { text: entry[0], grouping: entry[1] }))
        // after the orders, whenever these are added
        for (const item of items)
            sortOptions.addItem(item)
        _groupingItems = items
    }
    on_VideoGroupingChanged: _updateGroupingItems()
    Component.onCompleted: _updateGroupingItems()

    Component {
        id: groupingSeparator
        Widgets.RedesignMenuSeparator { }
    }

    Component {
        id: groupingOption
        Widgets.RedesignOptionItem {
            property int grouping
            checked: MainCtx.grouping === grouping
            // the page applies it (SortMenuVideo.onGrouping)
            onTriggered: root.sortMenu.grouping(grouping)
        }
    }

    Navigation.navigable: true

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View
    }
    readonly property bool darkTheme: theme.palette ? theme.palette.isDark : true

    background: Rectangle {
        color: VLCStyle.redesign.color(VLCStyle.redesign.windowBackground,
                                       theme.bg.primary, root.darkTheme)
        // Figma "Vector": #222222, 1 px centred on the top of the content:
        // half of it on each of the last two rows
        Rectangle {
            x: root.leftPadding
            y: parent.height - 1
            width: parent.width - root.leftPadding - root.rightPadding
            height: 1
            color: root.darkTheme ? "#181818" : VLCStyle.redesign.color(VLCStyle.redesign.separator, theme.border, false)
        }
        // over the first row of the content
        Rectangle {
            x: root.leftPadding
            y: parent.height
            z: 1
            width: parent.width - root.leftPadding - root.rightPadding
            height: 1
            visible: root.darkTheme
            color: "#171717"
        }
    }

    // Figma "Frame 26": gap 20
    contentItem: GridLayout {
        id: headerGrid
        columns: 4
        columnSpacing: VLCStyle.dp(20, VLCStyle.scale)
        rowSpacing: VLCStyle.dp(12, VLCStyle.scale)

        // Figma "Frame 25": gap 10
        RowLayout {
            Layout.row: 0
            Layout.column: 0
            Layout.columnSpan: root.compact ? 4 : 1
            Layout.fillWidth: true
            Layout.preferredHeight: root._rowHeight
            spacing: VLCStyle.dp(10, VLCStyle.scale)

            // the arrows: gap 12
            Row {
            spacing: VLCStyle.dp(12, VLCStyle.scale)

            Widgets.IconToolButton {
                text: VLCIcons.back
                description: qsTr("Previous")
                // A fixed size: sized from the image, the button would follow
                // the image reloaded at its size and never settle
                implicitWidth: VLCStyle.dp(24, VLCStyle.scale)
                implicitHeight: implicitWidth
                contentItem: Image {
                    source: VLCStyle.palette.isDark
                            ? "qrc:///redesign/icons/arrow-left/" + (parent.enabled ? "default" : "disabled") + ".png"
                            : "qrc:///redesign/arrow-left.svg"
                    sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                    fillMode: Image.Pad
                    smooth: false
                    opacity: (VLCStyle.palette.isDark || parent.enabled) ? 1 : 0.4
                }
                enabled: !History.previousEmpty
                onClicked: {
                    // the main display keeps the page left for "Next"
                    if (root._historyHost)
                        root._historyHost.historyBack()
                    else
                        History.previous()
                }
            }

            Widgets.IconToolButton {
                text: VLCIcons.back
                description: qsTr("Next")
                implicitWidth: VLCStyle.dp(24, VLCStyle.scale)
                implicitHeight: implicitWidth
                contentItem: Image {
                    source: VLCStyle.palette.isDark
                            ? "qrc:///redesign/icons/arrow-right/" + (parent.enabled ? "default" : "disabled") + ".png"
                            : "qrc:///redesign/arrow-right.svg"
                    sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                    fillMode: Image.Pad
                    smooth: false
                    opacity: (VLCStyle.palette.isDark || parent.enabled) ? 1 : 0.4
                }
                // History has no forward stack: the main display keeps the
                // pages left with "Previous"
                enabled: !!root._historyHost && !root._historyHost.historyForwardEmpty
                onClicked: root._historyHost.historyForward()
            }
            }

            RowLayout {
                id: crumbRow

                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: !!root.crumbs && root.crumbs.length > 0
                // Figma: the address items 6 apart
                spacing: VLCStyle.dp(6, VLCStyle.scale)
                clip: true

                Repeater {
                    model: crumbRow.visible ? root.crumbs : []

                    delegate: RowLayout {
                        id: crumbDelegate

                        required property int index
                        required property var modelData

                        spacing: crumbRow.spacing

                        Text {
                            id: crumbSeparator
                            visible: crumbDelegate.index > 0
                            // Figma: "\u25B6", Inter 8, #404040, in a box of 8 x 19
                            text: "\u25B6"
                            Layout.preferredWidth: VLCStyle.dp(8, VLCStyle.scale)
                            Layout.fillHeight: true
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: VLCStyle.dp(8, VLCStyle.scale)
                            color: VLCStyle.redesign.color(VLCStyle.redesign.textDisabled,
                                                           theme.fg.secondary, root.darkTheme)
                        }

                        T.AbstractButton {
                            id: crumb

                            readonly property bool current: crumbDelegate.index === root.crumbs.length - 1

                            Layout.maximumWidth: VLCStyle.dp(260, VLCStyle.scale)
                            Layout.fillHeight: true
                            // the advance of the text, as Figma (not rounded up);
                            // compact, at most an equal share of the row (elided)
                            Layout.preferredWidth: root.compact ? Math.min(crumbAdvance.advanceWidth, root._crumbShare)
                                                                : crumbAdvance.advanceWidth
                            implicitWidth: implicitContentWidth
                            // the text box of the design (the padding of the text
                            // follows the height: from it, a binding loop)
                            implicitHeight: Math.round(1.2109375 * VLCStyle.dp(16, VLCStyle.scale))
                            enabled: !current
                            text: String(crumbDelegate.modelData)

                            Accessible.name: text

                            // Figma: Inter 16 #8F8F8F, the current page
                            // SemiBold #969696; text boxes of 19
                            contentItem: Text {
                                id: crumbText
                                text: crumb.text
                                elide: Text.ElideMiddle
                                font.family: VLCStyle.redesign.fontFamily
                                font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                                font.weight: crumb.current ? Font.DemiBold : Font.Normal
                                font.underline: crumb.hovered
                                verticalAlignment: Text.AlignTop
                                topPadding: root._topPadding(crumb.height, font.pixelSize, crumbMetrics.ascent, false)
                                color: VLCStyle.redesign.color(crumb.current ? VLCStyle.redesign.textSecondary
                                                               : crumb.hovered ? VLCStyle.redesign.textPrimary
                                                                               : VLCStyle.redesign.textMuted,
                                                               theme.fg.primary, root.darkTheme)
                                FontMetrics {
                                    id: crumbMetrics
                                    font: crumbText.font
                                }
                                TextMetrics {
                                    id: crumbAdvance
                                    font: crumbText.font
                                    text: crumb.text
                                }
                            }

                            onClicked: root.crumbClicked(crumbDelegate.index)
                        }
                    }
                }

                Item { Layout.fillWidth: true }
            }

            // the page alone: the current page of the address
            Widgets.ListLabel {
                id: titleLabel
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: !crumbRow.visible
                text: root.breadcrumb
                elide: Text.ElideMiddle
                font.family: VLCStyle.redesign.fontFamily
                font.weight: Font.DemiBold
                font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                verticalAlignment: Text.AlignTop
                topPadding: root._topPadding(height, font.pixelSize, titleMetrics.ascent, false)
                bottomPadding: 0
                color: VLCStyle.redesign.color(VLCStyle.redesign.textSecondary,
                                               theme.fg.primary, root.darkTheme)
                FontMetrics {
                    id: titleMetrics
                    font: titleLabel.font
                }
            }
        }

        // compact: the right group aligned to the right
        Item {
            visible: root.compact
            Layout.row: 1
            Layout.column: 0
            Layout.fillWidth: true
        }

        Widgets.MenuCaption {
            id: countLabel
            Layout.row: root.compact ? 1 : 0
            Layout.column: 1
            Layout.preferredHeight: root._rowHeight
            visible: root.itemCount >= 0
            text: qsTr("%1 item", "%1 items", root.itemCount).arg(root.itemCount)
            color: VLCStyle.redesign.color("#6B6B6B", theme.fg.secondary,
                                           root.darkTheme)
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            verticalAlignment: Text.AlignTop
            topPadding: root._topPadding(root._rowHeight, font.pixelSize, countMetrics.ascent)
            bottomPadding: 0
            FontMetrics {
                id: countMetrics
                font: countLabel.font
            }
        }

        Loader {
            id: actionLoader
            Layout.row: root.compact ? (root._actionRow ? 2 : 1) : 0
            Layout.column: (root.compact && root._actionRow) ? 0 : 3
            Layout.columnSpan: (root.compact && root._actionRow) ? 4 : 1
            Layout.preferredWidth: root.compact ? Math.min(implicitWidth, root.availableWidth) : implicitWidth
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            sourceComponent: root.headerAction
            active: sourceComponent !== null
            visible: active
        }

        RowLayout {
            id: toolsRow
            Layout.row: root.compact ? 1 : 0
            Layout.column: 2
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            // Figma "Frame 102" and "Frame 103": gap 20
            spacing: VLCStyle.dp(20, VLCStyle.scale)

            // Figma "Sorting Options": the options of the page, one per
            // order of each criterion
            Widgets.IconToolButton {
                id: sortButton
                visible: root.sort.available
                // (its icon is the image below: the text names it)
                text: qsTr("Sort")
                description: qsTr("Sort")
                implicitWidth: VLCStyle.dp(24, VLCStyle.scale)
                implicitHeight: implicitWidth
                checked: sortOptions.opened
                contentItem: Image {
                    source: VLCStyle.palette.isDark ? "qrc:///redesign/icons/sort/dim.png"
                                                    : "qrc:///redesign/sort.svg"
                    sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                    fillMode: Image.Pad
                    smooth: false
                }
                onClicked: {
                    if (sortOptions.opened)
                        sortOptions.close()
                    else
                        sortOptions.popup(sortButton, 0, sortButton.height + VLCStyle.dp(8, VLCStyle.scale))
                }
            }

            Widgets.IconToolButton {
                visible: MainCtx.hasGridListMode
                text: MainCtx.gridView ? VLCIcons.list : VLCIcons.grid
                description: qsTr("List/Grid")
                implicitWidth: VLCStyle.dp(24, VLCStyle.scale)
                implicitHeight: implicitWidth
                contentItem: Image {
                    source: VLCStyle.palette.isDark
                            ? "qrc:///redesign/icons/" + (MainCtx.gridView ? "grid" : "list") + "/dim.png"
                            : (MainCtx.gridView ? "qrc:///redesign/grid.png" : "qrc:///redesign/list.png")
                    sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                    fillMode: Image.Pad
                    smooth: false
                }
                // Figma "View Options"
                checked: viewOptions.opened
                onClicked: {
                    if (viewOptions.opened)
                        viewOptions.close()
                    else
                        viewOptions.popup(this, 0, height + VLCStyle.dp(8, VLCStyle.scale))
                }

                Widgets.RedesignMenu {
                    id: viewOptions
                    menuWidth: VLCStyle.dp(265, VLCStyle.scale)

                    Widgets.RedesignOptionItem {
                        text: qsTr("Grid View")
                        iconSource: "qrc:///redesign/icons/grid/default.png"
                        checked: MainCtx.gridView
                        onTriggered: MainCtx.gridView = true
                    }
                    Widgets.RedesignOptionItem {
                        text: qsTr("List View")
                        iconSource: "qrc:///redesign/icons/list/default.png"
                        checked: !MainCtx.gridView
                        onTriggered: MainCtx.gridView = false
                    }
                }
            }

            Rectangle {
                id: searchBackground
                visible: root.search.available
                // narrower only when the header has no room
                Layout.fillWidth: true
                Layout.minimumWidth: VLCStyle.dp(120, VLCStyle.scale)
                Layout.maximumWidth: VLCStyle.dp(325, VLCStyle.scale)
                Layout.preferredWidth: VLCStyle.dp(325, VLCStyle.scale)
                Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: VLCStyle.redesign.color("#151515", theme.bg.secondary,
                                               root.darkTheme)
                border.width: searchField.activeFocus ? 1 : 0
                border.color: VLCStyle.redesign.focusColor

                // Figma "Frame 122": padding 5 8, gap 10
                Image {
                    id: searchIcon
                    anchors.left: parent.left
                    anchors.leftMargin: VLCStyle.dp(8, VLCStyle.scale)
                    anchors.top: parent.top
                    anchors.topMargin: VLCStyle.dp(5, VLCStyle.scale)
                    width: VLCStyle.redesign.iconSize
                    height: width
                    source: VLCStyle.palette.isDark ? "qrc:///redesign/icons/search/dim.png"
                                                    : "qrc:///redesign/search.svg"
                    sourceSize: Qt.size(width, height)
                    fillMode: Image.Pad
                    smooth: false
                }

                Widgets.TextFieldExt {
                    id: searchField
                    objectName: "librarySearchField"
                    anchors.left: searchIcon.right
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: VLCStyle.dp(10, VLCStyle.scale)
                    anchors.rightMargin: VLCStyle.dp(8, VLCStyle.scale)
                    background: Item {}
                    padding: 0
                    leftPadding: 0
                    rightPadding: 0
                    // sized by its anchors (the padding below comes from the height)
                    implicitHeight: VLCStyle.dp(24, VLCStyle.scale)
                    // the text box of the design (17 px) centred, on a
                    // whole pixel
                    verticalAlignment: TextInput.AlignTop
                    topPadding: root._topPadding(height, font.pixelSize, searchMetrics.ascent)
                    placeholderText: qsTr("Search")
                    placeholderTextColor: VLCStyle.redesign.color(VLCStyle.redesign.textDisabled,
                                                                  theme.fg.secondary, root.darkTheme)
                    color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted,
                                                   theme.fg.primary, root.darkTheme)
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    FontMetrics {
                        id: searchMetrics
                        font: searchField.font
                    }
                    Accessible.searchEdit: true

                    property bool previousGlobalShortcuts: true
                    onActiveFocusChanged: {
                        if (activeFocus) {
                            previousGlobalShortcuts = MainCtx.useGlobalShortcuts
                            MainCtx.useGlobalShortcuts = false
                        } else {
                            MainCtx.useGlobalShortcuts = previousGlobalShortcuts
                        }
                    }
                    Component.onDestruction: {
                        if (activeFocus) MainCtx.useGlobalShortcuts = previousGlobalShortcuts
                    }
                }

                Binding {
                    target: root.search
                    property: "pattern"
                    value: searchField.text
                    delayed: true
                }

                Connections {
                    target: root.search
                    function onAskShow() { searchField.forceActiveFocus(Qt.ShortcutFocusReason) }
                }
            }
        }
    }
}
