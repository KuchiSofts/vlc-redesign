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

import VLC.MainInterface
import VLC.Player
import VLC.Util

// Figma "Customize Player": "Chapters", the chapters and titles of the media
RedesignIconButton {
    id: root

    signal requestLockUnlockAutoHide(bool lock)

    iconSource: "qrc:///redesign/icons/chapters/default.png"
    enabled: !paintOnly && (Player.hasChapters || Player.hasTitles)
    onClicked: menu.popup(this.mapToGlobal(0, 0), true)
    // open while its menu shows (a menu of the system: told by its signals)
    property bool _menuShown: false
    active: _menuShown
    description: qsTr("Chapters")
    AccessibleCompat.id: "chaptersButton"

    QmlBookmarkMenu {
        id: menu
        ctx: MainCtx
        player: Player
        onAboutToShow: { root._menuShown = true; root.requestLockUnlockAutoHide(true) }
        onAboutToHide: { root._menuShown = false; root.requestLockUnlockAutoHide(false) }
    }

    function forceUnlock() {
        if (menu)
            menu.close()
    }
}
