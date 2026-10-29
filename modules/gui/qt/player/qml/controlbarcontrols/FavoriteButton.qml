/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import VLC.Player
import VLC.MainInterface
import VLC.PlayerControls

RedesignIconButton {
    id: root
    iconSource: state.favorite ? "qrc:///redesign/icons/favorite/active.svg"
                               : "qrc:///redesign/icons/favorite/default.svg"
    checked: state.favorite
    enabled: !paintOnly && state.available && !state.busy
    description: state.error.length ? state.error
                 : !state.available ? qsTr("Available for items in your library")
                 : state.favorite ? qsTr("Remove from favorites") : qsTr("Add to favorites")
    Accessible.name: description
    onClicked: state.toggle()
    FavoriteState { id: state; ctx: MainCtx; url: Player.url }
}
