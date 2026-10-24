/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import VLC.Style

// Text field of the design: 36 px high, #090909 with a 2 px #191919 border,
// radius 6, 12 px text inset, 14 px text; an optional unit after the value
// ("3 s", "5000 ms")
T.TextField {
    id: root

    property string suffix: ""

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, value) }

    implicitWidth: px(320)
    implicitHeight: px(36)
    // 12 px inside the 2 px border, as the design lays it out (from the
    // right, right to left)
    leftPadding: LayoutMirroring.enabled ? px(18) : px(14)
    rightPadding: LayoutMirroring.enabled ? px(14) : px(18)
    horizontalAlignment: TextInput.AlignLeft
    // The baseline where the design centres a line of Inter
    topPadding: height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize) - metrics.ascent
    bottomPadding: 0
    verticalAlignment: TextInput.AlignTop
    selectByMouse: true
    color: c("#8F8F8F")
    selectionColor: VLCStyle.redesign.accentBackground
    selectedTextColor: VLCStyle.redesign.accent
    placeholderTextColor: c("#404040")
    font.family: VLCStyle.redesign.fontFamily
    font.pixelSize: px(14)

    FontMetrics { id: metrics; font: root.font }

    // The Templates field draws no placeholder (#404040 in the design)
    Text {
        x: root.leftPadding
        y: root.topPadding
        width: root.width - root.leftPadding - root.rightPadding
        visible: root.text.length === 0 && root.preeditText.length === 0
        text: root.placeholderText
        color: root.placeholderTextColor
        font: root.font
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
    }

    background: Rectangle {
        radius: root.px(6)
        color: root.c("#090909")
        border.width: root.px(2)
        border.color: root.activeFocus ? VLCStyle.redesign.accentDim : root.c("#191919")
    }

    // The unit right after the text (before it on the left, right to left)
    Text {
        visible: root.suffix.length > 0 && root.text.length > 0
        x: LayoutMirroring.enabled ? root.width - root.rightPadding - root.contentWidth - metrics.advanceWidth(" ") - width
                                   : root.leftPadding + root.contentWidth + metrics.advanceWidth(" ")
        y: root.topPadding
        text: root.suffix
        color: root.color
        font: root.font
    }
}
