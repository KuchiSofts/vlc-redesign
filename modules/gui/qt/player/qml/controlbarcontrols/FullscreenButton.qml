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


import VLC.Style
import VLC.Player
import VLC.Util

// the icon of the redesign set, in the accent while in fullscreen
RedesignIconButton {
    id: fullScreenBtn

    enabled: Player.hasVideoOutput
    iconSource: "qrc:///redesign/icons/fullscreen/default.svg"
    checked: Player.fullscreen
    onClicked: Player.fullscreen = !Player.fullscreen
    description: Player.fullscreen ? qsTr("Exit Fullscreen") : qsTr("Fullscreen")

    AccessibleCompat.id: "fullscreenButton"
}
