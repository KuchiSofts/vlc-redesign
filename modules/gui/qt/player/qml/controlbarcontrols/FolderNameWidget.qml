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
import VLC.Player

// Figma "Customize Player": "Folder Name", the folder of the media
MetadataText {
    iconSource: "qrc:///redesign/icons/folder/default.svg"
    value: {
        const mrl = Player.url.toString().split(/[?#]/)[0]
        const parts = mrl.split("/").filter(part => part.length > 0)
        if (parts.length < 2 || !mrl.startsWith("file:"))
            return ""
        try { return decodeURIComponent(parts[parts.length - 2]) }
        catch (error) { return parts[parts.length - 2] }
    }
    placeholder: qsTr("Folder name unavailable")
}
