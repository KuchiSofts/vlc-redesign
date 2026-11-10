/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 *
 * Authors: Leon Vitanos <leon.vitanos@gmail.com>
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

import VLC.Style

QtObject{
    // NOTE: Base picture sizing
    required property int basePictureWidth
    required property int basePictureHeight

    required property int availableWidth

    // Keep media artwork at its designed size instead of stretching it to fill a row.
    property bool fixedPictureWidth: false

    property int titleTopMargin: VLCStyle.gridItemTitle_topMargin
    property int titleHeight: VLCStyle.gridItemTitle_height

    property int subtitleTopMargin: VLCStyle.gridItemSubtitle_topMargin
    property int subtitleHeight: VLCStyle.gridItemSubtitle_height

    // NOTE: The total text height of title and subtitle
    //       1dp is added to each, to correctly represent the actual height
    readonly property int textHeight: (titleHeight ? titleHeight + titleTopMargin + VLCStyle.dp(1, VLCStyle.scale): 0) +
                                      (subtitleHeight ? subtitleHeight + subtitleTopMargin + VLCStyle.dp(1, VLCStyle.scale) : 0)

    property int horizontalSpacing: nbItemPerRow === 2 ? VLCStyle.margin_large + VLCStyle.margin_xxxsmall
                                                       : (nbItemPerRow === 3 ? VLCStyle.margin_large + VLCStyle.margin_xxsmall
                                                                             : _defaultHorizontalSpacing)

    readonly property int _defaultHorizontalSpacing: VLCStyle.column_spacing
    readonly property int _spacingForCount: fixedPictureWidth ? horizontalSpacing : _defaultHorizontalSpacing

    // The rows fill the page: as many cards as fit at the design's size,
    // which then grow (their picture keeps its shape) so that the row ends
    // at the page's margin, by this much at most. A page wider than that
    // gets more cards in a row, not larger ones. Where the cards fit
    // exactly (the design's own window), they are the design's.
    readonly property real maxGrowth: 1.35
    // The width of the page in the design's own window (1920 wide: 1660
    // between the margins, five video cards exactly). There the design's
    // sizes win, a row that has room left too (seven music cards of 200):
    // the growth is for the other widths.
    // (A grid always fills its rows, at the design's window too: seven
    // artists of 200 left 140 empty before the margin there. Only Home's
    // rows, which scroll sideways, keep the design's size at that width:
    // VLCStyle.redesign.cardWidth.)

    property var maxNbItemPerRow
    readonly property int nbItemPerRow: {
        const _nbItemPerRow = Math.max(
            Math.floor(
                (availableWidth + _spacingForCount) /
                (basePictureWidth + _spacingForCount)
            ), 1
        )
        const _capped = maxNbItemPerRow ? Math.min(_nbItemPerRow, maxNbItemPerRow) : _nbItemPerRow
        // (the most cards of a row holds until they would be larger than
        // maxGrowth: a 4K or ultra-wide page)
        const _needed = Math.ceil((availableWidth + _spacingForCount) /
                                  (basePictureWidth * maxGrowth + _spacingForCount))
        return Math.max(_capped, Math.min(_nbItemPerRow, _needed))
    }

    // NOTE: Responsive cell sizing based on available width
    readonly property int _filledWidth: (availableWidth + horizontalSpacing) / nbItemPerRow - horizontalSpacing
    // While the window is being resized the width changes at every pixel,
    // and every card with it (each change lays the whole grid out again:
    // the interface waited 50 ms and more at each step). The cards then
    // grow in steps of 8, and take their exact width once the resize rests.
    property bool _resizing: false
    readonly property Timer _resizeRest: Timer {
        interval: 120
        onTriggered: _resizing = false
    }
    onAvailableWidthChanged: {
        _resizing = true
        _resizeRest.restart()
    }
    function _stepped(width) {
        return _resizing ? basePictureWidth + Math.floor(Math.max(0, width - basePictureWidth) / 8) * 8 : width
    }
    readonly property int cellWidth: fixedPictureWidth
        ? _stepped(Math.max(basePictureWidth, Math.min(_filledWidth, Math.round(basePictureWidth * maxGrowth))))
        : _stepped(_filledWidth)
    readonly property int cellHeight: (basePictureHeight / basePictureWidth) * cellWidth + textHeight

    // NOTE: Find the maximum picture size for nbItemPerRow == 1, so that we downscale in most of the cases
    //       formula for maxPictureWidth depended on nbItemPerRow would be:
    //       (basePictureWidth + _defaultHorizontalSpacing) * (1 + 1 / nbItemPerRow) - _defaultHorizontalSpacing
    readonly property int maxPictureWidth: fixedPictureWidth ? Math.round(basePictureWidth * maxGrowth)
        : (basePictureWidth + _defaultHorizontalSpacing) * (1 + 1 / 3) - _defaultHorizontalSpacing
    readonly property int maxPictureHeight: (basePictureHeight / basePictureWidth) * maxPictureWidth
}
