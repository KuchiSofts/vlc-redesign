/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * Authors: Benjamin Arnaud <bunjee@omega.gg>
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


import VLC.MainInterface
import VLC.Style

// The ON / OFF toggle of the design (Figma "Toggle", as the settings): a
// 68 x 32 well (#090909, radius 6) holding a 33 x 26 label (ON in Accent
// Bright at 3 px, OFF in #191919 at 32 px, radius 3) and a 26 x 26 square
// (Accent Dim at 39 px, #404040 at 3 px, radius 3)
T.Switch {
    id: root

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value) }

    // Settings

    width : VLCStyle.checkButton_width
    height: VLCStyle.checkButton_height

    T.ToolTip.visible: (T.ToolTip.text && (hovered || visualFocus))

    T.ToolTip.text: text

    T.ToolTip.delay: VLCStyle.delayToolTipAppear

    // Keys

    Keys.priority: Keys.AfterItem

    Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

    // Accessible

    // As a click: toggles, then tells the users that act on clicked (the
    // effect switches)
    function userToggle() {
        root.toggle()
        root.toggled()
        root.clicked()
    }

    Accessible.onPressAction: root.userToggle()
    Accessible.onToggleAction: root.userToggle()

    // Children

    background: Rectangle {
        radius: root.px(6)
        color: root.c("#090909")
        border.width: root.visualFocus ? 1 : 0
        border.color: VLCStyle.redesign.accent
    }

    // the square on the side of the state: on the left when on in a right
    // to left layout (Hebrew, Arabic...), as Windows' toggles
    readonly property bool _labelFirst: root.checked !== root.mirrored

    indicator: Item {
        anchors.fill: parent

        Rectangle {
            x: root._labelFirst ? root.px(3) : root.px(32)
            y: root.px(3)
            width: root.px(33)
            height: root.px(26)
            radius: root.px(3)
            color: root.checked ? root.c(VLCStyle.redesign.accentBackground) : root.c("#191919")

            // ON at (10, 9) of the toggle, OFF at (36, 9), as the design;
            // a longer word (a translation) centred, smaller if it has to
            Text {
                id: stateText

                readonly property real _designSize: root.px(12)
                readonly property bool _fits: stateMetrics.advanceWidth
                                              <= root.px(root.checked ? 33 - 7 - 2 : 33 - 4 - 2)

                x: _fits ? (root.checked ? root.px(7) : root.px(4)) : Math.round((parent.width - width) / 2)
                anchors.baseline: parent.top
                anchors.baselineOffset: _fits ? VLCStyle.redesign.textBaseline(root.px(6), font.pixelSize)
                                              : VLCStyle.redesign.textBaseline(Math.round((parent.height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                                               font.pixelSize)
                text: root.checked ? qsTr("ON") : qsTr("OFF")
                color: root.checked ? VLCStyle.redesign.accent : root.c("#6B6B6B")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: _fits ? _designSize
                                      : Math.max(root.px(7), Math.min(_designSize,
                                                                      Math.floor(_designSize * root.px(33 - 2 * 3)
                                                                                 / Math.max(1, stateMetrics.advanceWidth))))

                TextMetrics {
                    id: stateMetrics
                    font.family: stateText.font.family
                    font.pixelSize: stateText._designSize
                    text: stateText.text
                }
            }
        }

        Rectangle {
            x: root._labelFirst ? root.px(39) : root.px(3)
            y: root.px(3)
            width: root.px(26)
            height: root.px(26)
            radius: root.px(3)
            color: root.checked ? VLCStyle.redesign.accentDim : root.c("#404040")
        }
    }
}
