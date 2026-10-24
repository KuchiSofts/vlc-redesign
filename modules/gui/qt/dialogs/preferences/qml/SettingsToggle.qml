/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import VLC.Style

// ON / OFF toggle of the design (Figma component "Toggle"): a 68 x 32 well
// holding a 33 x 26 label and a 26 x 26 square, swapped by the state
T.AbstractButton {
    id: root

    // The new state asked by a click (the owner sets checked from its value)
    signal switched(bool on)

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, value) }

    checkable: false
    onClicked: switched(!checked)
    focusPolicy: Qt.StrongFocus
    // The label of the design (33) holds ON and OFF; the longer words of
    // other languages widen it, and the toggle with it
    readonly property real labelWidth: Math.max(px(33), Math.ceil(Math.max(onMetrics.advanceWidth,
                                                                            offMetrics.advanceWidth)) + px(8))
    implicitWidth: px(68) + labelWidth - px(33)
    implicitHeight: px(32)
    Accessible.role: Accessible.CheckBox
    Accessible.checked: checked

    // Right to left, the label and the square swap sides
    function place(x, width) { return root.mirrored ? root.width - width - x : x }

    TextMetrics {
        id: onMetrics
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(12)
        text: qsTr("ON")
    }
    TextMetrics {
        id: offMetrics
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(12)
        text: qsTr("OFF")
    }

    background: Rectangle {
        radius: root.px(6)
        color: root.c("#090909")
        border.width: root.visualFocus ? 1 : 0
        border.color: VLCStyle.redesign.accent
    }

    contentItem: Item {
        Rectangle {
            x: root.place(root.checked ? root.px(3) : root.px(32), width)
            y: root.px(3)
            width: root.labelWidth
            height: root.px(26)
            radius: root.px(3)
            color: root.checked ? root.c(VLCStyle.redesign.accentBackground) : root.c("#191919")

            // Where the design puts it: ON at (10, 9) of the toggle, OFF at
            // (36, 9), the baseline at the ascender of Inter (0.96875 em); a
            // word of another language that does not fit there is centred
            Text {
                readonly property real designX: root.checked ? root.px(7) : root.px(4)
                readonly property real shownX: designX + width <= parent.width - root.px(3)
                                               ? designX : Math.round((parent.width - width) / 2)
                x: root.mirrored ? parent.width - width - shownX : shownX
                anchors.baseline: parent.top
                anchors.baselineOffset: VLCStyle.redesign.textBaseline(root.px(6), font.pixelSize)
                text: root.checked ? qsTr("ON") : qsTr("OFF")
                color: root.checked ? VLCStyle.redesign.accent : root.c("#6B6B6B")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(12)
            }
        }

        Rectangle {
            x: root.place(root.checked ? root.px(39) + root.labelWidth - root.px(33) : root.px(3), width)
            y: root.px(3)
            width: root.px(26)
            height: root.px(26)
            radius: root.px(3)
            color: root.checked ? VLCStyle.redesign.accentDim : root.c("#404040")
        }
    }
}
