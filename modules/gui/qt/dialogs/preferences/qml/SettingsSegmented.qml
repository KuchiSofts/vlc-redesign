/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import VLC.Style

// Choice of a few values of the design (Figma component "Values": Theme,
// Size values): a 320 x 36 well (#090909, 2 px padding, 2 px gap) of 32 px
// options, the chosen one in Accent Bright with the accent text
Rectangle {
    id: root

    // [{ text, value }]
    property var options: []
    property var value

    signal chosen(var value)

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value) }

    // the width where every option shows its whole text: a row gives a long
    // translation ("Da esquerda para a direita") more room than the design's
    readonly property real neededWidth: {
        let widest = 0
        for (const option of options)
            widest = Math.max(widest, Math.ceil(textMetrics.advanceWidth(option.text)))
        return options.length * (widest + px(24)) + px(2) * (options.length + 1)
    }

    implicitWidth: px(320)
    implicitHeight: px(36)
    radius: px(6)
    color: c("#090909")

    FontMetrics {
        id: textMetrics
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(14)
    }

    Row {
        anchors.fill: parent
        anchors.margins: root.px(2)
        spacing: root.px(2)

        Repeater {
            model: root.options
            T.Button {
                id: option
                required property var modelData
                readonly property bool selected: modelData.value === root.value

                width: (parent.width - root.px(2) * (root.options.length - 1)) / root.options.length
                height: parent.height
                focusPolicy: Qt.StrongFocus
                onClicked: root.chosen(modelData.value)
                Accessible.name: modelData.text
                Accessible.checked: selected
                // the whole name when it is cut short (a long translation)
                T.ToolTip.visible: hovered && optionText.truncated
                T.ToolTip.text: modelData.text
                T.ToolTip.delay: 650

                background: Rectangle {
                    radius: root.px(4)
                    color: option.selected ? root.c(VLCStyle.redesign.accentBackground)
                                           : option.hovered ? root.c("#222222") : root.c("#191919")
                    border.width: option.visualFocus ? 1 : 0
                    border.color: VLCStyle.redesign.accent
                }
                // Figma: the text box (the advance of Inter 3.19 rounded up,
                // measured 16 times larger less the unit per glyph the bundled
                // font adds) centred
                contentItem: Item {
                    Text {
                        id: optionText
                        x: Math.max(0, (parent.width - width) / 2)
                        width: Math.min(parent.width, Math.ceil(optionExact.advanceWidth / 16
                                                                - text.length * font.pixelSize / 2816 - 0.001))
                        height: parent.height
                        text: option.modelData.text
                        color: option.selected ? VLCStyle.redesign.accent : root.c("#6B6B6B")
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignTop
                        // the baseline where the design centres a line of Inter
                        topPadding: height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize) - optionMetrics.ascent
                        FontMetrics { id: optionMetrics; font: optionText.font }
                        TextMetrics {
                            id: optionExact
                            font.family: optionText.font.family
                            font.pixelSize: optionText.font.pixelSize * 16
                            text: optionText.text
                        }
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(14)
                    }
                }
            }
        }
    }
}
