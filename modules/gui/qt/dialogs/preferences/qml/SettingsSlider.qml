/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import VLC.Style

// Slider of the settings of the design: a 320 x 16 track (#090909, 2 px
// inset), the 12 px fill (Accent Bright), 2 x 6 ticks (#1A1611 in Screen
// blend), the 8 x 24 handle (Accent Dim). With labels (12 px), 57 px high:
// 4 px, the handle (the track 4 px into it), 10 px, the labels, 4 px
T.Slider {
    id: root

    // Tick positions and labels ([{ value, text }]) in slider units
    property var ticks: []
    property var labels: []
    // Baseline of the labels from the top (45 px under the top of the
    // handle in the design; some rows of the design differ)
    property real labelsBaseline: px(4 + 45)
    // The first and last labels from the ends of the track
    property real labelsInset: 0

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value) }
    function fraction(value) { return to > from ? Math.max(0, Math.min(1, (value - from) / (to - from))) : 0 }
    // Right to left, the low end is on the right (as visualPosition)
    function visualFraction(value) { return mirrored ? 1 - fraction(value) : fraction(value) }
    // Screen blend of the ticks over what is below them
    function screen(below) {
        const tick = Qt.color("#1A1611")
        return Qt.rgba(1 - (1 - below.r) * (1 - tick.r), 1 - (1 - below.g) * (1 - tick.g),
                       1 - (1 - below.b) * (1 - tick.b), 1)
    }

    readonly property real trackWidth: availableWidth
    readonly property real handleCenter: px(2) + visualPosition * (trackWidth - px(4))

    implicitWidth: px(320)
    implicitHeight: labels.length > 0 ? px(57) : px(24)
    padding: 0
    focusPolicy: Qt.StrongFocus
    snapMode: T.Slider.SnapAlways

    background: Item {
        Rectangle {
            id: track
            y: root.labels.length > 0 ? root.px(8) : root.px(4)
            width: root.trackWidth
            height: root.px(16)
            radius: root.px(6)
            color: root.c("#090909")
            border.width: root.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.accent

            // the fill from the low end to the handle
            Rectangle {
                x: root.mirrored ? root.handleCenter : root.px(2)
                y: root.px(2)
                width: Math.max(0, root.mirrored ? root.trackWidth - root.px(2) - root.handleCenter
                                                 : root.handleCenter - root.px(2))
                height: root.px(12)
                radius: root.px(4)
                color: root.c(VLCStyle.redesign.accentBackground)
            }

            Repeater {
                model: root.ticks
                Rectangle {
                    required property var modelData
                    readonly property real center: root.px(2) + root.visualFraction(modelData) * (root.trackWidth - root.px(4))
                    // none at the ends of the track
                    visible: root.fraction(modelData) > 0 && root.fraction(modelData) < 1
                    x: center - width / 2
                    y: root.px(4)
                    width: root.px(2)
                    height: root.px(8)
                    radius: root.px(1)
                    // over the fill (on the other side of the handle, right to left)
                    readonly property bool filled: root.mirrored ? center > root.handleCenter : center < root.handleCenter
                    color: root.screen(Qt.color(filled ? root.c(VLCStyle.redesign.accentBackground)
                                                       : root.c("#090909")))
                }
            }
        }

        Repeater {
            model: root.labels
            Text {
                required property var modelData
                required property int index
                // centred as Figma places them (measured)
                // (whole pixels: as measured, px() rounds)
                readonly property real center: root.px(5) + root.visualFraction(modelData.value) * (root.trackWidth - root.px(10))
                // The first and last labels align with the ends of the track
                // (the first on the right, right to left)
                readonly property real startX: root.labelsInset
                readonly property real endX: root.trackWidth - root.labelsInset - width
                x: index === 0 ? (root.mirrored ? endX : startX)
                   : index === root.labels.length - 1 ? (root.mirrored ? startX : endX)
                   : center - width / 2
                // the labels 10 px under the handle, on the baseline of the design
                anchors.baseline: parent.top
                anchors.baselineOffset: root.labelsBaseline
                text: modelData.text
                color: root.c("#6B6B6B")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(12)
            }
        }
    }

    handle: Rectangle {
        // on whole pixels, as the design places it
        x: Math.round(Math.max(0, Math.min(root.trackWidth - width, root.handleCenter - width / 2)))
        y: root.labels.length > 0 ? root.px(4) : 0
        width: root.px(8)
        height: root.px(24)
        radius: root.px(4)
        color: VLCStyle.redesign.accentDim
    }
}
