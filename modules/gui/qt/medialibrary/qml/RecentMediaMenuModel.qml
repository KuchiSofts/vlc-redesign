/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import VLC.MediaLibrary

// The media of "Open Recent" in the main menu (MainMenu.qml): compiled with
// the interface. Made from a string when the menu first opened, it held the
// interface a quarter of a second.
MLRecentMediaModel {
    ml: MediaLib
    limit: 10
}
