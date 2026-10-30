/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Templates as T

import VLC.Style

// Figma "Player - customize" (2160:7437): an item of the top bar dragged
// into the control bar while the player is customized. It drags like an
// item of the palette (objectName "buttonsList", mIndex: the control it
// becomes); the top bar hides the items whose control is in the bar.
MouseArea {
    id: root

    // the control of the bar this item becomes
    property int controlId: -1
    property url iconSource

    anchors.fill: parent
    visible: VLCStyle.customizing && controlId >= 0
    cursorShape: drag.active ? Qt.DragMoveCursor : Qt.OpenHandCursor
    drag.target: proxy
    drag.smoothed: false

    onPressed: (mouse) => {
        proxy.x = mouse.x - proxy.width / 2
        proxy.y = mouse.y - proxy.height / 2
    }
    drag.onActiveChanged: {
        if (drag.active) {
            proxy.Drag.start()
        } else {
            proxy.Drag.drop()
            proxy.x = 0
            proxy.y = 0
        }
    }

    // what is dragged: the icon in a dashed box, above everything
    Item {
        id: proxy

        objectName: "buttonsList"
        readonly property int mIndex: root.controlId

        parent: Drag.active ? T.Overlay.overlay : root
        visible: Drag.active
        width: VLCStyle.dp(30, VLCStyle.scale)
        height: VLCStyle.dp(32, VLCStyle.scale)
        Drag.hotSpot.x: width / 2
        Drag.hotSpot.y: height / 2
        Drag.source: proxy

        Rectangle {
            anchors.fill: parent
            radius: VLCStyle.dp(4, VLCStyle.scale)
            color: VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
        }
        DashedRect {
            anchors.fill: parent
            color: VLCStyle.redesign.accent
        }
        Image {
            anchors.centerIn: parent
            width: VLCStyle.redesign.iconSize
            height: width
            sourceSize: Qt.size(width, height)
            smooth: !VLCStyle.redesign.svgExact(width, height, Screen.devicePixelRatio)
            source: root.iconSource
        }
    }
}
