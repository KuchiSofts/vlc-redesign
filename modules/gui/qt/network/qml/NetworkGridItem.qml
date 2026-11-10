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
import QtQml.Models

import VLC.Widgets as Widgets
import VLC.Network
import VLC.Style
import VLC.Util

Widgets.GridItem {
    id: root

    property var model: ({})
    property int index: -1

    pictureWidth: VLCStyle.gridCover_network_width
    pictureHeight: VLCStyle.gridCover_network_height
    // the redesign's card (the folder cards of Home: #1E1E1E, radius 12,
    // the name in the design's type), not a black card with a bold name
    redesign: true
    pictureRadius: VLCStyle.dp(12, VLCStyle.scale)
    mediaCover.color: VLCStyle.redesign.color("#1E1E1E", "#dedede")

    playCoverShowPlay: {
        switch (model.type) {
            case NetworkMediaModel.TYPE_NODE:
            case NetworkMediaModel.TYPE_DIRECTORY:
            case NetworkMediaModel.TYPE_PLAYLIST:
                return false
            default:
                return implicitPlayCoverShowPlay
        }
    }

    image: {
        if (model.artwork && model.artwork.toString() !== "") {
            return model.artwork
        }
        return ""
    }

    fillMode: Image.PreserveAspectCrop

    cacheImage: true // we may have network thumbnail

    fallbackImage: {
        return SVGColorImage.colorize(model.artworkFallback)
                            .color1(VLCStyle.redesign.color("#8F8F8F"))
                            .accent(root.colorContext.accent)
                            .uri()
    }

    title: model.name || qsTr("Unknown share")
    subtitle: {
        // make sure subtitle is never empty otherwise it causes alignment issues
        const defaultTxt = "--"

        const mrl = model.mrl?.toString() || ""
        const type = model.type

        if ((type === NetworkMediaModel.TYPE_NODE
            || type === NetworkMediaModel.TYPE_DIRECTORY
            || type === NetworkMediaModel.TYPE_PLAYLIST)
            && (mrl === "vlc://nop"))
            return defaultTxt

        // a folder of this computer as its path, not as a file:/// address
        if (mrl.startsWith("file:///")) {
            const path = mrl.substring(/^file:\/\/\/[A-Za-z]:/.test(mrl) ? 8 : 7)
            try { return decodeURIComponent(path) } catch (e) { return path }
        }
        return mrl || defaultTxt
    }

    pictureOverlay: Item {
        implicitWidth: root.pictureWidth
        implicitHeight: root.pictureHeight

        Widgets.VideoProgressBar {
            id: progressBar

            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }

            visible: (model.progress ?? - 1) > 0

            radius: root.pictureRadius
            value:  visible
                    ? Helpers.clamp(model.progress, 0, 1)
                    : 0
        }
    }
}
