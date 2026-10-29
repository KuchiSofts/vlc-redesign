/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.impl as Impl
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
    // open while its list shows: the accent on a tint of it, as the buttons
    // of the bar (RedesignIconButton); no border (the focus ring only)
    readonly property bool _open: popup.visible
    background: Rectangle {
        radius: VLCStyle.dp(4, VLCStyle.scale)
        color: {
            const hover = root.hovered || root.down
            if (root._open) {
                const tint = VLCStyle.palette.isDark ? VLCStyle.redesign.accentBackground
                                                     : Qt.alpha(VLCStyle.redesign.accent, 0.14)
                return hover ? Qt.tint(tint, VLCStyle.palette.isDark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.05))
                             : tint
            }
            return !hover ? "transparent"
                   : VLCStyle.palette.isDark ? Qt.rgba(1, 1, 1, 0.06)
                   : VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
        }
        border.width: root.visualFocus ? VLCStyle.focus_border : 0
        border.color: root.colorContext.visualFocus
        Behavior on color { ColorAnimation { duration: 120 } }
    }
    // the icon in an item: as the content itself it was stretched over the box
    contentItem: Item {
        Impl.IconImage {
            source: "qrc:///redesign/icons/zoom/default.svg"
            width: VLCStyle.dp(20, VLCStyle.scale)
            height: width
            sourceSize: Qt.size(width, height)
            smooth: !VLCStyle.redesign.svgExact(width, height, Screen.devicePixelRatio)
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            opacity: root.enabled ? 1 : 0.4
            // the image's own colours, the accent while open (transparent: no tint)
            color: !root._open ? "transparent"
                   : VLCStyle.palette.isDark ? VLCStyle.redesign.accent : VLCStyle.redesign.accentDim
        }
    }
    Accessible.name: qsTr("Zoom")
    Accessible.description: enabled ? qsTr("Choose a video zoom level")
                                    : qsTr("Zoom is unavailable without video")
}
