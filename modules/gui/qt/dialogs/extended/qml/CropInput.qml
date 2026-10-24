/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import VLC.Style

// A side of the crop (Figma 543:990): a 33 px field (#090909, radius 6), the
// name (14 px, #6B6B6B) 12 px in, the pixels (14 px, #8F8F8F) 72 px in,
// "px" (#404040) a space after them
Rectangle {
    id: root
    required property var effects
    required property string side
    property string label
    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, value) }
    signal edited(int pixels)
    implicitHeight: px(33)
    radius: px(6)
    color: c("#090909")
    border.width: pixelsInput.activeFocus ? 1 : 0
    border.color: VLCStyle.redesign.accentDim
    objectName: "crop-" + side

    // Where the pixels start: 72 in, further for a longer name (the
    // pixels keep 48 at least); mirrored right to left
    readonly property real valueStart: Math.max(px(72), Math.min(px(12) + nameText.implicitWidth + px(4),
                                                                 width - px(12) - px(48)))

    // The whole field takes the pixels
    TapHandler { onTapped: pixelsInput.forceActiveFocus(Qt.MouseFocusReason) }

    Text {
        id: nameText
        x: LayoutMirroring.enabled ? parent.width - width - root.px(12) : root.px(12)
        width: root.valueStart - root.px(12) - root.px(4)
        anchors.baseline: parent.verticalCenter
        anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize, parent.height)
        text: root.label
        color: root.c("#6B6B6B")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(14)
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
    }
    TextInput {
        id: pixelsInput
        x: LayoutMirroring.enabled ? root.px(12) : root.valueStart
        width: parent.width - root.valueStart - root.px(12)
        horizontalAlignment: TextInput.AlignLeft
        anchors.baseline: parent.verticalCenter
        anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize, parent.height)
        clip: true
        color: root.c("#8F8F8F")
        selectionColor: VLCStyle.redesign.accentBackground
        selectedTextColor: VLCStyle.redesign.accent
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(14)
        validator: IntValidator { bottom: 0; top: 2147483647 }
        selectByMouse: true
        text: root.effects.cropValue(root.side).toString()
        Accessible.name: qsTr("%1 crop in pixels").arg(root.label)
        onEditingFinished: {
            const parsed = Number(text)
            if (Number.isSafeInteger(parsed) && parsed >= 0) root.edited(parsed)
            text = root.effects.cropValue(root.side).toString()
        }
        Connections {
            target: root.effects
            function onStateChanged() {
                if (!pixelsInput.activeFocus)
                    pixelsInput.text = root.effects.cropValue(root.side).toString()
            }
        }
    }
    // The unit right after the pixels (right to left: before them, the
    // pixels at the start of their field)
    Text {
        readonly property real gap: spaceMetrics.advanceWidth(" ")
        readonly property real pixelsWidth: Math.min(pixelsInput.contentWidth, pixelsInput.width)
        x: LayoutMirroring.enabled ? pixelsInput.x + pixelsInput.width - pixelsWidth - gap - width
                                   : pixelsInput.x + pixelsWidth + gap
        anchors.baseline: pixelsInput.baseline
        visible: LayoutMirroring.enabled ? x >= root.px(4) : x + width <= parent.width - root.px(4)
        text: qsTr("px")
        color: root.c("#404040")
        font: pixelsInput.font
        FontMetrics { id: spaceMetrics; font: pixelsInput.font }
    }
}
