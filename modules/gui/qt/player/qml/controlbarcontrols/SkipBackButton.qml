/*****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
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


import VLC.Widgets as Widgets
import VLC.Style
import VLC.Player
import VLC.Util

Widgets.IconToolButton {
    id: stepBackBtn

    enabled: Player.seekable

    text: VLCIcons.skip_back
    onClicked: Player.jumpBy(-15)
    description: qsTr("Back 15 seconds")

    // Figma "Player - Music": jumps of 15 s, frames of 30 x 28 like Previous
    implicitWidth: VLCStyle.dp(30, VLCStyle.scale)
    implicitHeight: VLCStyle.dp(28, VLCStyle.scale)

    contentItem: Item {
        implicitWidth: VLCStyle.redesign.iconSize
        implicitHeight: VLCStyle.redesign.iconSize

        Image {
            anchors.centerIn: parent
            source: "qrc:///redesign/icons/backward-15-sec/default.svg"
            width: VLCStyle.redesign.iconSize
            height: VLCStyle.redesign.iconSize
            sourceSize: Qt.size(width, height)
            fillMode: Image.PreserveAspectFit
            opacity: stepBackBtn.enabled ? 1 : 0.4
        }
    }

    AccessibleCompat.id: "skipBackButton"
}
