/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import VLC.Dialogs
import VLC.Style

ColumnLayout {
    id: root
    required property var effects
    property int section: 0
    function c(value) { return VLCStyle.redesign.color(value, value) }
    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }

    // The filters of each section, ONE panel each as the design groups them
    // (Figma 536:1855 Essential, 614:1430 Colors), with its names and ticks
    // of the design; a filter of one slider names it ("Sharpen Sigma")
    readonly property var essential: [
        { id: "adjust", title: qsTr("Image Adjustment"),
          labels: { "hue": qsTr("Hue"), "brightness": qsTr("Brightness"), "contrast": qsTr("Contrast"),
                    "saturation": qsTr("Saturation"), "gamma": qsTr("Gamma") },
          ticks: { "saturation": [1, 2], "gamma": [1, 2, 3, 4, 5, 6, 7, 8, 9] } },
        { id: "sharpen", title: qsTr("Sharpen Sigma"), ticks: { "sharpen-sigma": [0.5, 1, 1.5] } },
        { id: "gradfun", title: qsTr("Banding Removal Radius"),
          ticks: { "gradfun-radius": [8, 12, 16, 20, 24, 28] } },
        { id: "grain", title: qsTr("Film Grain Variance"),
          ticks: { "grain-variance": [1, 2, 3, 4, 5, 6, 7, 8, 9] } }
    ]
    readonly property var colors: [
        { id: "extract", title: qsTr("Color Extraction") },
        { id: "colorthres", title: qsTr("Color Threshold"),
          labels: { "colorthres-saturationthres": qsTr("Saturation"),
                    "colorthres-similaritythres": qsTr("Similarity") } },
        { id: "invert", title: qsTr("Negate Colors") },
        { id: "posterize", title: qsTr("Posterize") },
        // (the dropdown shows the mode alone)
        { id: "gradient", title: qsTr("Gradient"),
          labels: { "gradient-mode": "", "gradient-color": qsTr("Color"),
                    "gradient-cartoon": qsTr("Cartoon") } },
        { id: "sepia", title: qsTr("Sepia Intensity"), ticks: { "sepia-intensity": [85, 170] } }
    ]
    readonly property var geometry: [
        { id: "transform", title: qsTr("Transform") },
        { id: "rotate", title: qsTr("Rotate") },
        { id: "magnify", title: qsTr("Interactive Zoom") },
        { id: "puzzle", title: qsTr("Puzzle Game") }
    ]
    readonly property var overlay: [
        { id: "logo", title: qsTr("Add Logo") },
        { id: "marq", title: qsTr("Add Text") },
        { id: "erase", title: qsTr("Logo Erase") }
    ]
    readonly property var advanced: [
        { id: "anaglyph", title: qsTr("Anaglyph 3D") },
        { id: "mirror", title: qsTr("Mirror") },
        { id: "psychedelic", title: qsTr("Psychedelic") },
        { id: "wave", title: qsTr("Waves") },
        { id: "ripple", title: qsTr("Water Effect") },
        { id: "motiondetect", title: qsTr("Motion Detect") },
        { id: "gaussianblur", title: qsTr("Spatial Blur") },
        { id: "motionblur", title: qsTr("Motion Blur") },
        { id: "antiflicker", title: qsTr("Anti-Flickering") },
        { id: "hqdn3d", title: qsTr("Denoiser") }
    ]
    // The crop a panel of its own
    readonly property var sections: [root.essential, [], root.colors, root.geometry, root.overlay, root.advanced]
    spacing: VLCStyle.dp(16, VLCStyle.scale)

    // Two rows of three, each as wide as one of the former three: the six
    // labels in one row (Advanced 67 px at 14 px) overflow their 61 px
    GridLayout {
        Layout.fillWidth: true
        columns: 3
        rowSpacing: 0
        columnSpacing: 0
        Repeater {
            model: [qsTr("Essential"), qsTr("Crop"), qsTr("Colors"),
                    qsTr("Geometry"), qsTr("Overlay"), qsTr("Advanced")]
            delegate: T.Button {
                id: sectionButton
                required property int index
                required property string modelData
                objectName: "videoEffectsSection" + index
                Layout.fillWidth: true
                implicitHeight: VLCStyle.dp(42, VLCStyle.scale)
                focusPolicy: Qt.StrongFocus
                onClicked: root.section = index
                background: Rectangle {
                    color: sectionButton.hovered ? root.c(VLCStyle.redesign.controlHoverBackground) : "transparent"
                    radius: VLCStyle.dp(5, VLCStyle.scale)
                }
                contentItem: Text {
                    text: sectionButton.modelData
                    color: root.section === sectionButton.index ? VLCStyle.redesign.accent : root.c(VLCStyle.redesign.textMuted)
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    // a longer name on two lines in the 42 of the button
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }
        }
    }

    // (open at first: the one panel of the section)
    CropPanel {
        Layout.fillWidth: true
        visible: root.section === 1
        expanded: true
        effects: root.effects
    }

    // Figma 614:1430: the first filter 6 px under the header, 20 px under
    // the last one
    ExpandPanel {
        id: filtersPanel
        Layout.fillWidth: true
        visible: root.section !== 1
        expanded: true
        topSpacing: root.px(6)

        Repeater {
            model: root.sections[root.section]
            VideoFilterSection {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                first: index === 0
                effects: root.effects
                filterId: modelData.id
                title: modelData.title
                labels: modelData.labels || ({})
                ticks: modelData.ticks || ({})
            }
        }

        // (not in the design: the button of the settings, the values of
        // every filter of the section back to their defaults)
        SettingsButton {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Reset")
            Accessible.name: qsTr("Reset to defaults")
            onClicked: {
                for (const filter of root.sections[root.section]) {
                    if (root.effects.filterAvailable(filter.id))
                        root.effects.resetDefaults(filter.id)
                }
            }
        }
    }
}
