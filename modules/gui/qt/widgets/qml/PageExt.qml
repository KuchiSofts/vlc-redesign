/*****************************************************************************
 * Copyright (C) 2025 VLC authors and VideoLAN
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
import QtQuick as Quick
import QtQuick.Layouts
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Style
import VLC.Menus
import VLC.Util

T.Page {
    id: root

    //path of the current page loader
    property var pagePrefix: []

    //note that T.Page introduce an intermediate Item, so to access our delegate,
    //we need to use contentChildren[0] rather than contentItem
    readonly property Item _firstChild: contentChildren[0] ?? null

    //indicates whether the subview support grid/list mode
    property bool hasGridListMode: _firstChild?.hasGridListMode ?? false

    property bool isSearchable: _firstChild?.isSearchable ?? false

    property var sortModel: _firstChild?.sortModel ?? null

    //property is *not* readOnly, a PageLoader may define a localMenuDelegate common for its subviews (music, video)
    property SortMenu sortMenu: null

    property Component headerAction: null

    property bool libraryHeader: false
    property string libraryTitle: title
    property int libraryItemCount: -1
    // Address line of the library header (see LibraryHeader.crumbs)
    property var libraryCrumbs: null

    signal libraryCrumbClicked(int index)

    property int displayMarginBeginning: 0
    property int displayMarginEnd: 0

    property bool enableBeginningFade: true
    property bool enableEndFade: true

    readonly property SearchCtx search: _searchCtx
    readonly property SortCtx sort: _sortCtx

    // The order of the page until one is chosen: A-Z (SortCtx starts in
    // descending order, which listed artists, albums, tracks and the items of
    // a playlist backwards), the newest first for what is new or recent
    property int defaultSortOrder: Qt.AscendingOrder


    SearchCtx {
        id: _searchCtx
        available: root.isSearchable
    }

    SortCtx {
        id: _sortCtx

        readonly property string _sortCriteriaKey: ["sortCriteria", ...root.pagePrefix].join("/")
        readonly property string _sortOrderKey:  ["sortOrder", ...root.pagePrefix].join("/")

        available: Helpers.isArray(root.sortModel) && (root.sortModel).length > 0
        model: root.sortModel
        order: root.defaultSortOrder

        //save & restore setting of the page
        Component.onCompleted: {
            const criteria = MainCtx.settingValue(_sortCriteriaKey, undefined)
            if (criteria !== undefined)
                _sortCtx.criteria = criteria

            // An order is chosen with its criteria: one kept without (every
            // page kept its order when left, the descending one too) was not
            const order = MainCtx.settingValue(_sortOrderKey, undefined)
            if (order !== undefined && _sortCtx.criteria !== "")
                _sortCtx.order = parseInt(order)
        }

        Component.onDestruction: {
            MainCtx.setSettingValue(_sortCriteriaKey, _sortCtx.criteria)
            MainCtx.setSettingValue(_sortOrderKey, _sortCtx.order)
        }
    }

    Binding {
        target: MainCtx
        property: "hasGridListMode";
        value: root.hasGridListMode
    }

    on_FirstChildChanged: {
        if (!_firstChild)
            return
        _firstChild.Navigation.parentItem = root
        _firstChild.Navigation.upItem = root.header
    }

    function positionContentAtBeginning() {
        _firstChild?.positionContentAtBeginning?.()
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    header: Quick.Loader {
        id: pageHeader
        focus: true
        sourceComponent: root.libraryHeader ? libraryHeaderComponent : defaultHeaderComponent
        Navigation.navigable: true
        Navigation.parentItem: root
        Navigation.downAction: function() {
            if (typeof root.setCurrentItemFocus === "function")
                root.setCurrentItemFocus(Qt.TabFocusReason)
            else if (typeof root._firstChild?.setCurrentItemFocus === "function")
             root._firstChild.setCurrentItemFocus(Qt.TabFocusReason)
            else if (root._firstChild)
                root._firstChild.forceActiveFocus(Qt.TabFocusReason)
        }
    }

    Component {
        id: defaultHeaderComponent
        DefaultPageHeader {
            text: root.title
            headerAction: root.headerAction
            leftPadding: VLCStyle.dynamicAppMargins(width) + VLCStyle.layout_left_margin + root.leftPadding
            rightPadding: VLCStyle.dynamicAppMargins(width) + VLCStyle.layout_right_margin + root.rightPadding
            sortMenu: root.sortMenu
            search: _searchCtx
            sort: _sortCtx
            Navigation.parentItem: pageHeader
        }
    }

    Component {
        id: libraryHeaderComponent
        LibraryHeader {
            title: root.libraryTitle
            itemCount: root.libraryItemCount
            crumbs: root.libraryCrumbs
            headerAction: root.headerAction
            onCrumbClicked: (index) => root.libraryCrumbClicked(index)
            sortMenu: root.sortMenu
            search: _searchCtx
            sort: _sortCtx
            Navigation.parentItem: pageHeader
        }
    }

    component DefaultPageHeader: T.ToolBar {
        id: defaultPageHeader

        // Properties
        property alias text: label.text
        property Component headerAction: null

        property alias sortMenu: gridSortFilter.sortMenu

        property alias search: gridSortFilter.search
        property alias sort: gridSortFilter.sort

        position: T.ToolBar.Header

        topPadding: VLCStyle.dp(12, VLCStyle.scale)
        bottomPadding: VLCStyle.dp(12, VLCStyle.scale)

        // FIXME: `GridItem`'s background extends beyond its
        //        bounding rect, violating the hypothetical
        //        clip test (see 7e6b23db).
        bottomInset: (MainCtx.hasGridListMode && MainCtx.gridView) ? VLCStyle.gridItemSelectedBorder : undefined

        // Settings

        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                implicitContentWidth + leftPadding + rightPadding)
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 implicitContentHeight + topPadding + bottomPadding)

        Navigation.navigable: gridSortFilter.enabled || actionLoader.active

        readonly property bool stackActions: actionLoader.active &&
            availableWidth < label.implicitWidth + actionLoader.implicitWidth +
                             gridSortFilter.implicitWidth + VLCStyle.margin_normal * 2

        // Children

        readonly property ColorContext colorContext: ColorContext {
            id: theme
            colorSet: ColorContext.View
        }

        background: Rectangle {
            color: VLCStyle.redesign.color(VLCStyle.redesign.windowBackground, "#c2c2c1", theme.palette?.isDark ?? true)
        }

        contentItem: GridLayout {
            columns: defaultPageHeader.stackActions ? 2 : 3
            columnSpacing: VLCStyle.margin_xsmall
            rowSpacing: VLCStyle.margin_xsmall

            SubtitleLabel {
                id: label

                Layout.row: 0
                Layout.column: 0
                Layout.fillWidth: true

                color: VLCStyle.redesign.color(VLCStyle.redesign.textSecondary, "#4d4d4d", theme.palette?.isDark ?? true)
                font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
            }

            Loader {
                id: actionLoader
                sourceComponent: defaultPageHeader.headerAction
                active: sourceComponent !== null
                visible: active
                Layout.row: defaultPageHeader.stackActions ? 1 : 0
                Layout.column: defaultPageHeader.stackActions ? 0 : 1
                Layout.columnSpan: defaultPageHeader.stackActions ? 2 : 1
                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: defaultPageHeader.stackActions
            }

            GridSortFilterControls {
                id: gridSortFilter
                Layout.row: 0
                Layout.column: defaultPageHeader.stackActions ? 1 : 2
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                focus: true
                sort: _sortCtx
                search: _searchCtx
                Navigation.parentItem: defaultPageHeader
            }
        }
    }
}
