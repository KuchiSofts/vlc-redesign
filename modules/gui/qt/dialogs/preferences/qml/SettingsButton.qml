/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import VLC.Style

// Button of the settings of the design (Browse, Clear, Rescan, Edit Keyboard
// Shortcuts): 32 px high, #191919, radius 6, Inter Semi Bold 14 #8F8F8F
T.Button {
    id: root

    // Reset all settings: the red of the design
    property bool destructive: false

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value) }

    // wider than the design's 100 for a longer translation
    implicitWidth: Math.max(px(100), Math.ceil(implicitContentWidth) + leftPadding + rightPadding)
    implicitHeight: px(32)
    leftPadding: px(6)
    rightPadding: px(6)
    focusPolicy: Qt.StrongFocus
    font.family: VLCStyle.redesign.fontFamily
    font.pixelSize: px(14)
    font.weight: Font.DemiBold

    background: Rectangle {
        radius: root.px(6)
        color: root.down ? root.c("#090909") : root.hovered ? root.c("#222222") : root.c("#191919")
        border.width: root.visualFocus ? 1 : 0
        border.color: VLCStyle.redesign.accent
        opacity: root.enabled ? 1 : 0.5
    }
    // Figma: the text box (the advance of Inter 3.19 rounded up, measured 16
    // times larger less the unit per glyph the bundled font adds) centred
    contentItem: Item {
        implicitWidth: label.implicitWidth
        implicitHeight: label.implicitHeight
        Text {
            id: label
            x: Math.max(0, (parent.width - width) / 2)
            width: Math.min(parent.width, Math.ceil(labelExact.advanceWidth / 16
                                                    - text.length * font.pixelSize / 2816 - 0.001))
            height: parent.height
            text: root.text
            font: root.font
            color: root.destructive ? root.c("#E15032") : root.c("#8F8F8F")
            verticalAlignment: Text.AlignTop
            // the baseline where the design centres a line of Inter
            topPadding: height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize) - labelMetrics.ascent
            FontMetrics { id: labelMetrics; font: label.font }
            TextMetrics {
                id: labelExact
                font.family: label.font.family
                font.weight: label.font.weight
                font.pixelSize: label.font.pixelSize * 16
                text: label.text
            }
            elide: Text.ElideRight
            opacity: root.enabled ? 1 : 0.5
        }
    }
}
