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
import QtQuick.Templates as T

import VLC.Player
import VLC.Style
import VLC.Widgets as Widgets

// Figma "Customize Player": "Thumbnail", the artwork of the media
T.Control {
    id: root
    property bool paintOnly: false

    implicitWidth: VLCStyle.dp(30, VLCStyle.scale)
    implicitHeight: VLCStyle.dp(32, VLCStyle.scale)
    Accessible.role: Accessible.Graphic
    Accessible.name: qsTr("Thumbnail")

    contentItem: Item {
        Image {
            anchors.centerIn: parent
            width: VLCStyle.redesign.iconSize
            height: width
            visible: artwork.status !== Image.Ready
            source: "qrc:///redesign/icons/thumbnail/default.svg"
            sourceSize: Qt.size(width, height)
            fillMode: Image.Pad
            smooth: false
        }
        Widgets.ImageExt {
            id: artwork
            anchors.centerIn: parent
            width: VLCStyle.redesign.iconSize
            height: width
            radius: VLCStyle.dp(4, VLCStyle.scale)
            visible: status === Image.Ready
            source: root.paintOnly ? "" : (Player.artwork || "")
            sourceSize: Qt.size(width * 2, height * 2)
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }
    }
}
