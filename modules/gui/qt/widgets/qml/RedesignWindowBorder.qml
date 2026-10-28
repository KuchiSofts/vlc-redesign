/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick

import VLC.Style

// The 1 px frame of a window opened over the interface (a dialog, a menu,
// a panel, a card): the colour of the main window's border, which the
// system draws (VLCStyle.redesign.windowBorder), so that it reads as a
// window of its own over the dimmed interface. Put in the card's
// background, it fills it and follows its corners: drawn inside the
// outline, under the content (a focus ring stays on top), the sizes as
// they are.
Rectangle {
    // the theme of the card (a card always dark keeps a dark frame)
    property bool darkTheme: VLCStyle.palette.isDark

    anchors.fill: parent
    radius: (parent && parent.radius !== undefined) ? parent.radius : 0
    color: "transparent"
    border.width: 1
    border.color: VLCStyle.redesign.color(VLCStyle.redesign.windowBorder, undefined, darkTheme)
    antialiasing: radius > 0
}
