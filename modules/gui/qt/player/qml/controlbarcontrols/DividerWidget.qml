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

Image {
    // drawn at the size shown (an SVG: sharp at any scale and zoom)
    sourceSize: Qt.size(width, height)
    property bool paintOnly: false
    width: VLCStyle.dp(24, VLCStyle.scale)
    height: VLCStyle.dp(32, VLCStyle.scale)
    fillMode: Image.PreserveAspectFit
    source: "qrc:///redesign/icons/divider/default.png"
    Accessible.role: Accessible.Indicator
    Accessible.name: qsTr("Divider")
}
