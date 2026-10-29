/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import VLC.Player

MetadataText {
    iconSource: "qrc:///redesign/icons/artist/default.svg"
    value: Player.artist || ""
    placeholder: qsTr("Artist unavailable")
}
