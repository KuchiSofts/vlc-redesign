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


import VLC.MainInterface
import VLC.Widgets as Widgets
import VLC.Style
import VLC.Player
import VLC.Util

RedesignIconButton {
    iconSource: "qrc:///redesign/player/subtitles.svg"
    // Subtitles are for films and videos: for a live TV channel or music the
    // button only shows when there is a track to choose (subtitles carried
    // by the stream, several audio tracks), and takes no room otherwise
    // A video cast to a TV has no picture here, and its subtitles are chosen
    // here all the same (the TV follows)
    readonly property bool _offered: ((Player.hasVideoOutput || (Player.rendererManager.useRenderer
                                                                 && Player.rendererManager.rendererVideo
                                                                 && Player.videoTracks.count > 0))
                                      && !Iptv.playing)
                                     || Player.subtitleTracks.count > 0 || Player.audioTracks.count > 1
    implicitWidth: _offered ? VLCStyle.dp(30, VLCStyle.scale) : 0
    enabled: _offered
    opacity: _offered ? 1 : 0
    id: root

    // Proprerties

    readonly property var _parentItem: {
        if ((typeof rootPlayer !== 'undefined') && (rootPlayer !== null))
            return rootPlayer
        else
            return g_mainDisplay
    }

    // Signals

    signal requestLockUnlockAutoHide(bool lock)
    signal menuOpened(var menu)

    // When the menu closed: a press on this button closes it (a press
    // outside the menu) before the button gets the press, and the click
    // that follows must not open it again
    property double _menuClosedAt: 0

    // Settings

    text: VLCIcons.audiosub

    description: qsTr("Languages and tracks")

    // open while its panel shows
    active: !!menuLoader.item && menuLoader.item.opened

    AccessibleCompat.id: "langButton"

    // Events

    onClicked: {
        // the menu is built the first time it opens
        menuLoader.active = true
        const menu = menuLoader.item
        if (!menu)
            return
        if (menu.opened) {
            menu.close()
            return
        }
        if (root.pressedAt - root._menuClosedAt < 300)
            return
        const position = root.mapToItem(root._parentItem, 0, 0)
        menu.x = Math.max(0, Math.min(root._parentItem.width - menu.width,
                                      position.x + root.width - menu.width))
        menu.y = Math.max(0, root._parentItem.positionSliderY - menu.height)
        menu.open()
    }

    // Children

    Loader {
        id: menuLoader

        // not before the first click: every bar has this button, and so
        // have the previews of the toolbar editor
        active: false

        sourceComponent: TracksMenu {
            id: menu

            parent: root._parentItem

            z: 1

            focus: true

            onOpened: {
                root.requestLockUnlockAutoHide(true)
                root.menuOpened(menu)
            }

            onClosed: {
                root._menuClosedAt = Date.now()
                root.requestLockUnlockAutoHide(false)
                root.forceActiveFocus()
                root.menuOpened(null)
            }
        }
    }

    function forceUnlock() {
        if((menuLoader)&&(menuLoader.item))
            menuLoader.item.close()
    }
}
