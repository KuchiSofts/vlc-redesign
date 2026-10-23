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
    height: 48
    background: Rectangle {
        radius: 6
        color: root.hovered ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
    }
    contentItem: Text {
        text: MainCtx.minimalView ? qsTr("Show player UI") : qsTr("Hide player UI")
        color: VLCStyle.redesign.color("#969696", "#4d4d4d")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 16
        verticalAlignment: Text.AlignVCenter
        leftPadding: 12
    }
    Accessible.role: Accessible.CheckBox
    Accessible.checkable: true
    Accessible.checked: MainCtx.minimalView
    onClicked: MainCtx.minimalView = !MainCtx.minimalView
}
