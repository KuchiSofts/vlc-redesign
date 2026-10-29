/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 *
 * Authors: Leon Vitanos <leon.vitanos@gmail.com>
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


import VLC.MainInterface
import VLC.Style
import VLC.Util

T.Pane {
    id: root

    // Properties
    required property Item view

    leftPadding: view?.contentLeftMargin ?? 0
    rightPadding: view?.contentRightMargin ?? 0

    bottomPadding: VLCStyle.layoutTitle_bottom_padding
    topPadding: VLCStyle.layoutTitle_top_padding

    // Aliases

    property alias text: label.text

    property alias seeAllButton: button

    // Signals

    signal seeAllButtonClicked(int reason)

    // Settings

    width: view.width
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    Navigation.parentItem: view
    Navigation.navigable: button.visible

    Component.onCompleted: {
        // Qt Quick Pane sets a cursor for itself, unset it so that if the view has
        // busy cursor, it is visible over the header:
        MainCtx.unsetCursor(this)
    }

    // Children

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View
    }

    // Events
    onContentItemChanged: {
      console.assert(contentItem === root.row)
    }

    // Figma home sections: "Continue Watching ›", regular weight; the
    // title and the chevron open the full list
    // Figma "Frame 500": gap 5, Inter 20 #969696 in a box of 24
    RowLayout {
        id: row

        anchors.fill: parent
        spacing: VLCStyle.dp(5, VLCStyle.scale)

        Text {
            id: label

            Layout.maximumWidth: row.width - (button.visible ? button.width + row.spacing : 0)
            Layout.preferredHeight: Math.round(1.2109375 * font.pixelSize)

            elide: Text.ElideRight
            color: VLCStyle.redesign.color(VLCStyle.redesign.textSecondary, "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(20, VLCStyle.scale)
            verticalAlignment: Text.AlignTop
            topPadding: VLCStyle.redesign.textBaseline(0, font.pixelSize) - labelMetrics.ascent

            FontMetrics {
                id: labelMetrics
                font: label.font
            }

            TapHandler {
                enabled: button.visible
                onTapped: root.seeAllButtonClicked(Qt.MouseFocusReason)
            }
            HoverHandler {
                enabled: button.visible
                cursorShape: Qt.PointingHandCursor
            }
        }

        T.Button {
            id: button

            visible: false

            // the chevron of the icon set, 24
            implicitWidth: VLCStyle.dp(24, VLCStyle.scale)
            implicitHeight: implicitWidth

            focus: true

            Binding on focusReason {
                // NOTE: This is an explicit binding so that it does not get overridden:
                value: root.focusReason
            }

            text: qsTr("See All")
            Accessible.name: text

            background: Rectangle {
                radius: VLCStyle.redesign.controlRadius
                color: button.hovered ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                                      : "transparent"
                border.width: button.visualFocus ? 1 : 0
                border.color: VLCStyle.redesign.focusColor
            }
            contentItem: Image {
                source: "qrc:///redesign/icons/chevron-right/default.svg"
                sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                fillMode: Image.Pad
                smooth: false
                // after the title: it points left in a right to left layout
                mirror: button.mirrored
            }

            Navigation.parentItem: root

            onClicked: root.seeAllButtonClicked(focusReason)
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
