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
import VLC.Widgets as Widgets
import VLC.Style

Widgets.ComboBoxExt {
    id: root
    property bool paintOnly: false
    // implicit: the bar lays its controls out by their implicit size
    implicitWidth: VLCStyle.dp(56, VLCStyle.scale)
    implicitHeight: VLCStyle.dp(32, VLCStyle.scale)
    // the names of the levels ("1:2 Half"...) were cut to the icon's width
    popupWidth: Math.max(width, VLCStyle.dp(160, VLCStyle.scale))
    model: Player.zoom
    textRole: "display"
    currentIndex: -1
    enabled: Player.zoom.hasCurrent && !paintOnly
    onActivated: (index) => Player.zoom.toggleIndex(index)
    // the icon in an item: as the content itself it was stretched over the box
    contentItem: Item {
        Image {
            source: "qrc:///redesign/icons/zoom/default.svg"
            width: VLCStyle.dp(20, VLCStyle.scale)
            height: width
            sourceSize: Qt.size(width, height)
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            opacity: root.enabled ? 1 : 0.4
        }
    }
    Accessible.name: qsTr("Zoom")
    Accessible.description: enabled ? qsTr("Choose a video zoom level")
                                    : qsTr("Zoom is unavailable without video")
}
