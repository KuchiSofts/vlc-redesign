/*****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.impl as Impl
import VLC.Widgets as Widgets
import VLC.Style

Widgets.IconToolButton {
    id: root
    required property url iconSource
    // the icon is the image below: the text only names the button
    text: description

    font.pixelSize: VLCStyle.redesign.iconSize
    implicitWidth: VLCStyle.dp(30, VLCStyle.scale)
    implicitHeight: VLCStyle.dp(32, VLCStyle.scale)
    color: VLCStyle.redesign.color(checked ? VLCStyle.redesign.accent : VLCStyle.redesign.textMuted,
                                   colorContext.fg.primary)
    // Figma 177:98: the hover box is white at 6 % over the bar, 30 x 32 even
    // where the bar's row is lower (the box reaches over it)
    backgroundColor: !(hovered || down) ? "transparent"
                     : VLCStyle.palette.isDark ? Qt.rgba(1, 1, 1, 0.06)
                     : VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, colorContext.bg.primary)
    topInset: Math.min(0, (height - VLCStyle.dp(32, VLCStyle.scale)) / 2)
    bottomInset: topInset
    radius: VLCStyle.dp(4, VLCStyle.scale)
    contentItem: Item {
        implicitWidth: VLCStyle.redesign.iconSize
        implicitHeight: VLCStyle.redesign.iconSize
        Impl.IconImage {
            anchors.centerIn: parent
            width: VLCStyle.redesign.iconSize
            height: width
            source: root.iconSource
            color: root.color
            opacity: root.enabled ? 1 : 0.4
        }
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            width: VLCStyle.dp(12, VLCStyle.scale)
            height: VLCStyle.dp(2, VLCStyle.scale)
            color: VLCStyle.redesign.accent
            visible: root.checked
        }
    }
}
