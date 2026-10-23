/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
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

// The player with nothing to show: the picture of Figma's "Player" frames
// (their video.png, 1920 x 1080), the white pixel cat sitting on the floor
// of a dark room. Drawn in blocks, not from the picture: sharp at any size,
// no texture to keep, nothing to do once drawn. It covers the area as the
// picture covers the frame (the panels over it, the cat in the middle).
Item {
    id: root

    // the scene's pixels (Figma's frame) to the area's, covering it
    readonly property real _scale: Math.max(width / 1920, height / 1080)
    // the cat: 17 x 17 blocks of 24.0625 px from (755.9375, 375.5), on the
    // floor at 785; a whole number of pixels per block (sharp edges)
    readonly property int _block: Math.max(1, Math.round(24.0625 * _scale))
    readonly property real _sceneX: (width - 1920 * _scale) / 2
    readonly property real _sceneY: (height - 1080 * _scale) / 2
    readonly property real _floorY: Math.round(_sceneY + 785 * _scale)

    // [column, row, blocks, colour] of each run of a row (video.png)
    readonly property var _runs: [
        [1, 0, 1, "#ffffff"], [12, 0, 1, "#ffffff"], [1, 1, 1, "#e0e0e0"], [2, 1, 1, "#ffffff"],
        [11, 1, 1, "#ffffff"], [12, 1, 1, "#e0e0e0"], [1, 2, 3, "#e0e0e0"], [4, 2, 5, "#ffffff"],
        [9, 2, 1, "#e0e0e0"], [10, 2, 1, "#ffffff"], [11, 2, 2, "#e0e0e0"], [1, 3, 12, "#e0e0e0"],
        [0, 4, 2, "#000000"], [2, 4, 1, "#e0e0e0"], [3, 4, 1, "#000000"], [4, 4, 4, "#e0e0e0"],
        [8, 4, 1, "#000000"], [9, 4, 3, "#e0e0e0"], [12, 4, 2, "#000000"], [1, 5, 1, "#e0e0e0"],
        [2, 5, 2, "#ffd6e4"], [4, 5, 4, "#e0e0e0"], [8, 5, 2, "#ffd6e4"], [10, 5, 3, "#e0e0e0"],
        [0, 6, 2, "#000000"], [2, 6, 3, "#e0e0e0"], [5, 6, 1, "#000000"], [6, 6, 6, "#e0e0e0"],
        [12, 6, 2, "#000000"], [2, 7, 2, "#e0e0e0"], [4, 7, 1, "#000000"], [5, 7, 1, "#e0e0e0"],
        [6, 7, 1, "#000000"], [7, 7, 5, "#e0e0e0"], [3, 8, 8, "#e0e0e0"], [4, 9, 5, "#e0e0e0"],
        [9, 9, 1, "#ffffff"], [14, 9, 1, "#ffffff"], [4, 10, 5, "#e0e0e0"], [9, 10, 1, "#ffffff"],
        [15, 10, 1, "#ffffff"], [4, 11, 7, "#e0e0e0"], [16, 11, 1, "#ffffff"], [4, 12, 1, "#ffffff"],
        [5, 12, 2, "#e0e0e0"], [7, 12, 1, "#ffffff"], [8, 12, 4, "#e0e0e0"], [16, 12, 1, "#ffffff"],
        [4, 13, 1, "#ffffff"], [5, 13, 2, "#e0e0e0"], [7, 13, 1, "#ffffff"], [8, 13, 6, "#e0e0e0"],
        [14, 13, 2, "#ffffff"], [4, 14, 1, "#ffffff"], [5, 14, 2, "#e0e0e0"], [7, 14, 1, "#ffffff"],
        [8, 14, 2, "#e0e0e0"], [10, 14, 1, "#ffffff"], [11, 14, 3, "#e0e0e0"], [4, 15, 1, "#ffffff"],
        [5, 15, 2, "#e0e0e0"], [7, 15, 1, "#ffffff"], [8, 15, 1, "#e0e0e0"], [9, 15, 1, "#ffffff"],
        [10, 15, 4, "#e0e0e0"], [3, 16, 2, "#ffffff"], [5, 16, 1, "#e0e0e0"], [6, 16, 2, "#ffffff"],
        [8, 16, 1, "#e0e0e0"], [9, 16, 1, "#ffffff"], [10, 16, 3, "#e0e0e0"]
    ]

    clip: true

    // the wall
    Rectangle {
        anchors.fill: parent
        color: "#2d2b29"
    }

    // the floor: darker at its edge, as light as the wall at the frame's
    // bottom (and below it)
    Rectangle {
        x: 0
        y: root._floorY
        width: root.width
        height: Math.max(0, Math.round(root._sceneY + 1080 * root._scale) - root._floorY)
        gradient: Gradient {
            GradientStop { position: 0; color: "#272523" }
            GradientStop { position: 1; color: "#2d2b29" }
        }
    }

    // the cat, its feet on the floor, in the middle of the scene
    Item {
        id: cat

        width: 17 * root._block
        height: width
        x: Math.round(root._sceneX + (755.9375 + 17 * 24.0625 / 2) * root._scale - width / 2)
        y: root._floorY - height

        Repeater {
            model: root._runs

            Rectangle {
                required property var modelData

                x: modelData[0] * root._block
                y: modelData[1] * root._block
                width: modelData[2] * root._block
                height: root._block
                color: modelData[3]
                antialiasing: false
            }
        }
    }
}
