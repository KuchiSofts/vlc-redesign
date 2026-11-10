/*****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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
import QtQuick.Templates as T

import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

T.Control {
    id: root

    // Properties
    padding: VLCStyle.margin_normal

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    // Aliases

    default property alias contents: column.data

    property url cover
    // the icon of the empty state, when it is not the one of the cover
    property string iconName: ""
    readonly property bool _placeholder: root.cover.toString().startsWith("qrc:///placeholder/")

    property alias coverWidth: cover.width
    property alias coverHeight: cover.height

    property alias text: label.text

    property alias column: column

    spacing: VLCStyle.margin_normal

    enabled: visible

    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Empty view")

    // Children

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View
    }

    contentItem: Column {
        id: column

        spacing: root.spacing

        // The redesign's empty state: the page's icon, large and bare (the
        // grey box with a small icon in it was the old placeholder).
        Image {
            readonly property string _name: {
                if (root.iconName !== "")
                    return root.iconName
                const c = root.cover.toString()
                return c.includes("artistCover") ? "artist" : c.includes("albumCover") ? "album"
                     : c.includes("videoCover") ? "video" : ""
            }
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root._placeholder
            width: VLCStyle.dp(64, VLCStyle.scale)
            height: width
            sourceSize: Qt.size(width, height)
            source: visible ? SVGColorImage.colorize("qrc:///redesign/icons/" + (_name || "file") + "/default.svg")
                                           .any({ "#8F8F8F": VLCStyle.redesign.color("#6B6B6B", "#8a8a8a") }).uri()
                            : ""
        }

        ScaledImage {
            id: cover

            visible: !root._placeholder
            anchors.horizontalCenter: parent.horizontalCenter

            width: VLCStyle.colWidth(1)
            height: VLCStyle.colWidth(1)

            fillMode: Image.PreserveAspectFit

            // The light placeholders take the redesign colors in the dark theme
            source: (VLCStyle.palette.isDark && root.cover.toString().startsWith("qrc:///placeholder/"))
                    ? SVGColorImage.colorize(root.cover).any({ "#ECECEC": VLCStyle.redesign.controlBackground,
                                                               "#979797": VLCStyle.redesign.border }).uri()
                    : root.cover
        }

        T.Label {
            id: label

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            anchors.horizontalCenter: parent.horizontalCenter

            // a longer text (a translation) on more lines, within the view
            width: parent.width > 0 ? Math.min(implicitWidth, parent.width) : implicitWidth
            wrapMode: Text.Wrap

            focus: false

            color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")

            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(20, VLCStyle.scale)
            font.weight: Font.Normal
        }
    }
}
