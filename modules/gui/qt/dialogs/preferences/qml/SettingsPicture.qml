/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.Style

// Picture section of the video page of the design: Hue, Brightness,
// Contrast, Saturation, Gamma sliders and Reset picture. Moving a slider
// turns the adjust filter on, Reset turns it off.
SettingsSectionTitle {
    id: root
    text: qsTr("Picture")

    required property var effects
    readonly property var adjustments: [
        { id: "hue", title: qsTr("Hue") },
        { id: "brightness", title: qsTr("Brightness") },
        { id: "contrast", title: qsTr("Contrast") },
        // ticks at the whole values, as the design draws them
        { id: "saturation", title: qsTr("Saturation"), ticks: [1, 2] },
        { id: "gamma", title: qsTr("Gamma"), ticks: [1, 2, 3, 4, 5, 6, 7, 8, 9] }
    ]
    // Re-read once the effects have their interface (the ranges of the
    // sliders: 0..1 and disabled without them)
    readonly property var parameters: { effects.revision; return effects.parameters("adjust") }
    visible: { effects.revision; return effects.filterAvailable("adjust") }

    function c(value) { return VLCStyle.redesign.color(value) }

    Repeater {
        model: root.adjustments
        Item {
            id: row
            required property var modelData
            readonly property var parameter: root.parameters.find(p => p.id === modelData.id)
            width: root.width
            height: root.px(42)

            Text {
                id: adjustmentLabel
                anchors.left: parent.left
                anchors.baseline: parent.verticalCenter
                anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
                // cut short before the slider (a long translation)
                width: Math.max(0, Math.min(implicitWidth, parent.width - adjustmentSlider.width - root.px(20)))
                elide: Text.ElideRight
                HoverHandler { id: adjustmentHover }
                ToolTip.visible: adjustmentHover.hovered && adjustmentLabel.truncated
                ToolTip.text: adjustmentLabel.text
                ToolTip.delay: 650
                text: row.modelData.title
                color: root.c("#8F8F8F")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
            }
            SettingsSlider {
                id: adjustmentSlider
                objectName: "settingsPicture-" + row.modelData.id
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(root.px(320), parent.width * 0.46)
                from: row.parameter && row.parameter.minimum !== undefined ? row.parameter.minimum : 0
                to: row.parameter && row.parameter.maximum !== undefined ? row.parameter.maximum : 1
                ticks: row.modelData.ticks || []
                enabled: !!row.parameter
                Binding on value {
                    when: !adjustmentSlider.pressed
                    value: { root.effects.revision; return Number(root.effects.parameterValue(row.modelData.id)) }
                }
                onMoved: {
                    if (!root.effects.filterEnabled("adjust"))
                        root.effects.setFilterEnabled("adjust", true)
                    root.effects.setParameter(row.modelData.id, value)
                }
                Accessible.name: row.modelData.title
            }
        }
    }

    SettingsActionRow {
        width: root.width
        label: qsTr("Reset picture")
        buttonText: qsTr("Reset")
        buttonWidth: root.px(100)
        onClicked: {
            root.effects.resetDefaults("adjust")
            if (root.effects.filterEnabled("adjust"))
                root.effects.setFilterEnabled("adjust", false)
        }
    }
}
