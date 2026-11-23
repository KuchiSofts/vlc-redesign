/*****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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

import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Dialogs
import VLC.Menus

Item {
    ShortcutExt{ sequence:"Ctrl+B"; onActivated: DialogsProvider.bookmarksDialog() }

    MLRecentMediaModel {
        id: recentModel
        limit: 10
        ml: MediaLib
        // Ctrl+1 is the media played last, as in "Open Recent"
        sortOrder: Qt.DescendingOrder
    }

    //build all the shortcuts everytime, it seems that they can't be added/removed dynamically
    Repeater {
        model: 10

        Item {
            ShortcutExt {
                sequence: "Ctrl+" + ((index + 1) % 10)
                // not over a key of VLC's hotkeys: Ctrl+0 resets the
                // subtitles' size, and this shortcut took it first (the size
                // stayed, and nothing played with fewer than 10 recent media)
                enabled: MainCtx.useGlobalShortcuts
                         && MainCtx.isHotkeyFree(Qt.Key_0 + ((index + 1) % 10), Qt.ControlModifier)
                onActivated:  {
                    if (index >= recentModel.count)
                        return
                    // The media's library id, read as the context menu reads it.
                    // The model's roles are not enums QML can see: the old
                    // MLRecentMediaModel.MEDIA_ID was undefined, the URL text came
                    // back and played as an address ("c:/...": "can't be opened").
                    recentModel.getData([recentModel.index(index, 0)], (data) => {
                        const id = data?.[0]?.id
                        if (id)
                            MediaLib.addAndPlay(id)
                    })
                }
            }
        }
    }
}
