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

import VLC.Style

// A dashed outline of the design (1 px, dashes of 4 and 4), inside the item
Item {
    id: root

    property color color: "#6B6B6B"
    property real dash: VLCStyle.dp(4, VLCStyle.scale)
    property real gap: VLCStyle.dp(4, VLCStyle.scale)
    property real lineWidth: VLCStyle.dp(1, VLCStyle.scale)
    // a dot at the corners (the outlines of the customized player)
    property bool corners: true
    // Figma's dashed strokes of a box (the drop box of "New Playlist"):
    // the first dash one gap after the corner, as many periods as fit,
    // stretched so that the side ends the same way
    property bool fromCorner: false
    // Figma's dashes on a long side: as many periods as fit, stretched to
    // the side, centred on its ends (half dashes there: the corners)
    property bool stretch: false

    // top, right, bottom, left: as Figma draws them, the dashes of each
    // side centered on it (as many as fit with a dash of room), a dot at the
    // corners. One rectangle per dash: none while the outline is hidden
    // (most are, until the player is customized)
    Repeater {
        model: root.visible ? 4 : 0
        delegate: Item {
            id: edge
            required property int index
            readonly property bool horizontal: index % 2 === 0
            readonly property real length: horizontal ? root.width : root.height
            readonly property int count: root.stretch
                                         ? Math.max(1, Math.round(length / (root.dash + root.gap))) + 1
                                         : root.fromCorner
                                         ? Math.max(0, Math.round((length - root.gap) / (root.dash + root.gap)))
                                         : Math.max(0, Math.floor((length - root.dash) / (root.dash + root.gap)))
            readonly property real period: root.stretch ? length / Math.max(1, count - 1)
                                           : root.fromCorner && count > 0 ? (length - root.gap) / count
                                                                          : root.dash + root.gap
            readonly property real dashLength: period * root.dash / (root.dash + root.gap)
            readonly property real margin: root.fromCorner ? root.gap
                                                           : (length - count * root.dash - Math.max(0, count - 1) * root.gap) / 2
            x: index === 1 ? root.width - root.lineWidth : 0
            y: index === 2 ? root.height - root.lineWidth : 0
            width: horizontal ? root.width : root.lineWidth
            height: horizontal ? root.lineWidth : root.height

            Repeater {
                model: edge.count
                delegate: Rectangle {
                    required property int index
                    // stretched: centred on the multiples of the period, cut at the ends
                    readonly property real start: root.stretch ? Math.max(0, index * edge.period - edge.dashLength / 2)
                                                               : edge.margin + index * edge.period
                    readonly property real end: root.stretch ? Math.min(edge.length, index * edge.period + edge.dashLength / 2)
                                                             : start + edge.dashLength
                    readonly property real offset: Math.round(start)
                    x: edge.horizontal ? offset : 0
                    y: edge.horizontal ? 0 : offset
                    width: edge.horizontal ? Math.max(1, Math.round(end) - offset) : root.lineWidth
                    height: edge.horizontal ? root.lineWidth : Math.max(1, Math.round(end) - offset)
                    color: root.color
                }
            }
            // the corner at the start of the side
            Rectangle {
                visible: root.corners
                width: root.lineWidth
                height: root.lineWidth
                color: root.color
            }
        }
    }
}
