/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *****************************************************************************/
import QtQuick

import VLC.MainInterface
import VLC.Menus as Menus
import VLC.Util

// Figma "Player - customize" (2160:7437): the ⋯ menu of the top bar moved
// into the bar
RedesignIconButton {
    id: root

    signal requestLockUnlockAutoHide(bool lock)

    iconSource: "qrc:///redesign/icons/more/default.png"
    description: qsTr("Menu")
    AccessibleCompat.id: "barMenuButton"

    onClicked: mainMenu.toggleFrom(root, 0, root.height)

    Menus.MainMenu {
        id: mainMenu
        onOpened: root.requestLockUnlockAutoHide(true)
        onClosed: root.requestLockUnlockAutoHide(false)
    }
}
