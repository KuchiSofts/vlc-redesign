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

    // The whole field takes the pixels
    TapHandler { onTapped: pixelsInput.forceActiveFocus(Qt.MouseFocusReason) }

    Text {
        x: root.px(12)
        width: pixelsInput.x - x - root.px(4)
        anchors.baseline: parent.verticalCenter
        anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize, parent.height)
        text: root.label
        color: root.c("#6B6B6B")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(14)
        elide: Text.ElideRight
    }
    TextInput {
        id: pixelsInput
        x: root.px(72)
        width: parent.width - x - root.px(12)
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
    // The unit right after the pixels
    Text {
        x: pixelsInput.x + Math.min(pixelsInput.contentWidth, pixelsInput.width) + spaceMetrics.advanceWidth(" ")
        anchors.baseline: pixelsInput.baseline
        visible: x + width <= parent.width - root.px(4)
        text: qsTr("px")
        color: root.c("#404040")
        font: pixelsInput.font
        FontMetrics { id: spaceMetrics; font: pixelsInput.font }
    }
}
