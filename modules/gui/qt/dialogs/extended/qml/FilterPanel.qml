/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import VLC.Dialogs
import VLC.Style

// The settings of an audio filter (Figma 536:1513 Compressor, 536:1703
// Spatializer, 612:1223 Stereo Widener): a slider each, 76 px apart, 10 px
// under the header and 20 px over the bottom
ExpandPanel {
    id: root

    required property var effects
    required property string filterId
    property var items: []
    property int revision: 0

    // The names, units and scales of the design, the values kept as VLC
    // stores them: the spatializer shows 0 to 10, the widener its gains in
    // percent; ticks at each tenth of those (Figma 536:1703, 612:1223)
    readonly property var presentation: ({
        "compressor-rms-peak": { name: qsTr("RMS / Peak") },
        "compressor-attack": { name: qsTr("Attack"), unit: qsTr("ms") },
        "compressor-release": { name: qsTr("Release"), unit: qsTr("ms") },
        "compressor-threshold": { name: qsTr("Threshold"), unit: qsTr("dB") },
        "compressor-ratio": { name: qsTr("Ratio"), unit: qsTr(": 1") },
        "compressor-knee": { name: qsTr("Knee Radius"), unit: qsTr("dB") },
        "compressor-makeup-gain": { name: qsTr("Makeup Gain"), unit: qsTr("dB") },
        "spatializer-roomsize": { name: qsTr("Size"), scale: 10, ticks: true },
        "spatializer-width": { name: qsTr("Width"), scale: 10, ticks: true },
        "spatializer-wet": { name: qsTr("Wet"), scale: 10, ticks: true },
        "spatializer-dry": { name: qsTr("Dry"), scale: 10, ticks: true },
        "spatializer-damp": { name: qsTr("Damp"), scale: 10, ticks: true },
        "stereowiden-delay": { name: qsTr("Delay Time"), unit: qsTr("ms") },
        "stereowiden-feedback": { name: qsTr("Feedback Gain"), unit: "%", scale: 100, ticks: true },
        "stereowiden-crossfeed": { name: qsTr("Crossfeed"), unit: "%", scale: 100, ticks: true },
        "stereowiden-dry-mix": { name: qsTr("Dry Mix"), unit: "%", scale: 100, ticks: true }
    })

    // A tick at each tenth between the ends
    function tenths(from, to) {
        const result = []
        const count = Math.round((to - from) * 10)
        for (let i = 1; i < count; ++i)
            result.push(from + i / 10)
        return result
    }

    // The options are read once: a new model would destroy the slider being
    // dragged. Their values follow the revision.
    function refresh() {
        if (items.length === 0)
            items = effects.parameters(filterId).filter(p => p.type === "float")
    }
    Component.onCompleted: refresh()

    spacing: px(20)

    Connections {
        target: root.effects
        function onStateChanged() {
            root.revision++
            root.refresh()
        }
    }

    Repeater {
        model: root.items
        ValueSlider {
            required property var modelData
            readonly property var design: root.presentation[modelData.id] || {}
            Layout.fillWidth: true
            label: design.name || modelData.name
            unit: design.unit || ""
            displayScale: design.scale || 1
            ticks: design.ticks ? root.tenths(modelData.minimum, modelData.maximum) : []
            from: modelData.minimum
            to: modelData.maximum
            modelValue: { root.revision; return Number(root.effects.parameterValue(modelData.id)) }
            onEdited: (value) => root.effects.setParameter(modelData.id, value)
        }
    }

    // (not in the design: the button of the settings, under the sliders)
    SettingsButton {
        Layout.alignment: Qt.AlignRight
        text: qsTr("Reset")
        Accessible.name: qsTr("Reset to defaults")
        onClicked: root.effects.resetDefaults(root.filterId)
    }
}
