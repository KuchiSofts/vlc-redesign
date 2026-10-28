/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
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
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl

import VLC.Style

// Figma "option": a row of 48 (radius 6, padding 2 14, gap 10), Inter 14
// #8F8F8F, #1F1F1F when hovered or chosen, the check of the icon set in the
// accent on the right of the chosen one
T.MenuItem {
    id: root

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: VLCStyle.dp(48, VLCStyle.scale)

    leftPadding: VLCStyle.dp(14, VLCStyle.scale)
    rightPadding: VLCStyle.dp(14, VLCStyle.scale)
    topPadding: VLCStyle.dp(2, VLCStyle.scale)
    bottomPadding: VLCStyle.dp(2, VLCStyle.scale)
    spacing: VLCStyle.dp(10, VLCStyle.scale)

    checkable: false

    // An icon of the set on the left ("View Options"), optional
    property url iconSource

    contentItem: RowLayout {
        spacing: root.spacing

        Image {
            Layout.preferredWidth: VLCStyle.redesign.iconSize
            Layout.preferredHeight: VLCStyle.redesign.iconSize
            visible: root.iconSource.toString() !== ""
            source: root.iconSource
            sourceSize: Qt.size(width, height)
            fillMode: Image.Pad
            smooth: false
        }

        Text {
            id: label
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: root.text
            elide: Text.ElideRight
            // from the start of the row (the right in a right to left menu)
            horizontalAlignment: Text.AlignLeft
            color: VLCStyle.redesign.color(root.enabled ? VLCStyle.redesign.textMuted : VLCStyle.redesign.textDisabled)
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            // the text box of the design (17 px) centred, on a whole pixel
            verticalAlignment: Text.AlignTop
            topPadding: Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2)
                        + (Math.round(1.2109375 * font.pixelSize) - 1.2109375 * font.pixelSize) / 2
                        + 0.96875 * font.pixelSize - labelMetrics.ascent
            FontMetrics {
                id: labelMetrics
                font: label.font
            }
        }

        Impl.ColorImage {
            Layout.preferredWidth: VLCStyle.redesign.iconSize
            Layout.preferredHeight: VLCStyle.redesign.iconSize
            visible: root.checked
            source: "qrc:///redesign/icons/check/active.png"
            color: VLCStyle.redesign.accentTint
            sourceSize: Qt.size(width, height)
            fillMode: Image.Pad
            smooth: false
        }
    }

    background: Rectangle {
        implicitWidth: VLCStyle.dp(257, VLCStyle.scale)
        radius: VLCStyle.redesign.controlRadius
        color: (root.highlighted || root.checked)
               ? VLCStyle.redesign.color("#1F1F1F", "#9c9c9b")
               : "transparent"
    }
}
