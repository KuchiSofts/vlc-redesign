/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic as Controls

import VLC.MainInterface
import VLC.Style

// A row of the player's menu (PlayerMenu.qml), as its others: 48 high, the
// text 12 in, the hover of the redesign
Controls.ItemDelegate {
    id: root
    objectName: "hidePlayerUIAction"
    height: VLCStyle.dp(48, VLCStyle.scale)
    background: Rectangle {
        radius: VLCStyle.dp(6, VLCStyle.scale)
        color: root.hovered ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
    }
    contentItem: Text {
        text: MainCtx.minimalView ? qsTr("Show player UI") : qsTr("Hide player UI")
        color: VLCStyle.redesign.color("#969696", "#4d4d4d")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
        verticalAlignment: Text.AlignVCenter
        leftPadding: VLCStyle.dp(12, VLCStyle.scale)
    }
    Accessible.role: Accessible.CheckBox
    Accessible.checkable: true
    Accessible.checked: MainCtx.minimalView
    onClicked: MainCtx.minimalView = !MainCtx.minimalView
}
