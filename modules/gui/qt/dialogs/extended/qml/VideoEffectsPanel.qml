/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import VLC.Style

ColumnLayout {
    id: root
    required property var effects
    property int section: 0
    function c(value) { return VLCStyle.redesign.color(value, value) }
    readonly property var essential: [
        { id: "adjust", title: qsTr("Image adjustment") },
        { id: "sharpen", title: qsTr("Sharpen") },
        { id: "gradfun", title: qsTr("Debanding") },
        { id: "grain", title: qsTr("Film grain") }
    ]
    readonly property var colors: [
        { id: "extract", title: qsTr("Color extraction") },
        { id: "colorthres", title: qsTr("Color threshold") },
        { id: "invert", title: qsTr("Negate colors") },
        { id: "posterize", title: qsTr("Posterize") },
        { id: "gradient", title: qsTr("Gradient") },
        { id: "sepia", title: qsTr("Sepia") }
    ]
    readonly property var geometry: [
        { id: "transform", title: qsTr("Transform") },
        { id: "rotate", title: qsTr("Rotate") },
        { id: "magnify", title: qsTr("Interactive zoom") },
        { id: "puzzle", title: qsTr("Puzzle game") }
    ]
    readonly property var overlay: [
        { id: "logo", title: qsTr("Add logo") },
        { id: "marq", title: qsTr("Add text") },
        { id: "erase", title: qsTr("Logo erase") }
    ]
    readonly property var advanced: [
        { id: "anaglyph", title: qsTr("Anaglyph 3D") },
        { id: "mirror", title: qsTr("Mirror") },
        { id: "psychedelic", title: qsTr("Psychedelic") },
        { id: "wave", title: qsTr("Waves") },
        { id: "ripple", title: qsTr("Water effect") },
        { id: "motiondetect", title: qsTr("Motion detect") },
        { id: "gaussianblur", title: qsTr("Spatial blur") },
        { id: "motionblur", title: qsTr("Motion blur") },
        { id: "antiflicker", title: qsTr("Anti-flickering") },
        { id: "hqdn3d", title: qsTr("Denoiser") }
    ]
    // The filters of each section, the crop a panel of its own
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
                }
            }
        }
    }

    CropPanel {
        Layout.fillWidth: true
        visible: root.section === 1
        effects: root.effects
    }

    Repeater {
        model: root.sections[root.section]
        VideoFilterSection {
            required property var modelData
            Layout.fillWidth: true
            effects: root.effects
            filterId: modelData.id
            title: modelData.title
        }
    }
}
