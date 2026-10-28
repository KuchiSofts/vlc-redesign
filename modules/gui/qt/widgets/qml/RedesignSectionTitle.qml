/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts

import VLC.Style

// The title of a group of rows in the tool windows, as the section titles
// of Settings: Inter 16, "Dark/FG/2", a line of 33
Text {
    id: root

    property bool darkTheme: VLCStyle.palette.isDark

    // (in a popup: not under the mirrored interface)
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft

    Layout.fillWidth: true
    Layout.preferredHeight: VLCStyle.dp(33, VLCStyle.scale)
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft
    elide: Text.ElideRight
    color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e", root.darkTheme)
    font.family: VLCStyle.redesign.fontFamily
    font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)

    Accessible.role: Accessible.Heading
}
