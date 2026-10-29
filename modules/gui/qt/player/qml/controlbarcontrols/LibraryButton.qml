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
import VLC.Util

// Figma "Player - customize" (2160:7437): the library sidebar toggle of the
// top bar moved into the bar, in the accent while the library shows
RedesignIconButton {
    readonly property bool _hasPlayer: (typeof rootPlayer !== "undefined") && rootPlayer !== null

    iconSource: "qrc:///redesign/icons/sidebar/default.svg"
    // In the bar of the mini player the library is the page shown: the
    // button leaves it for the player
    checked: _hasPlayer ? rootPlayer.libraryVisible : !MainCtx.playerView
    description: qsTr("Library")
    AccessibleCompat.id: "barLibraryButton"

    onClicked: {
        if (_hasPlayer)
            rootPlayer.libraryVisible = !rootPlayer.libraryVisible
        else
            MainCtx.playerView = true
    }
}
