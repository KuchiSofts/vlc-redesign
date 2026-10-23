/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl
import VLC.Dialogs
import VLC.Style
import VLC.Widgets as Widgets

// The equalizer of the design (Figma 6114:13610): the preamp and the 10
// bands, a field and a slider each, 90 px apart. The presets (Figma
// 2372:8487 "EQ presets"), the two passes and Reset are not in that sheet:
// the dropdown, the toggle and the button of the design, above and under them
ExpandPanel {
    id: root

    required property var effects

    spacing: px(20)

    // Figma 2372:8487: the 48 px dropdown of the settings (#090909 2 px
    // around #191919, the text and the chevron 16 px in), its list 20 px
    // under it, rows of 44 with the curve of the preset 14 px in, the name
    // 10 px after it
    SettingsDropdown {
        id: presetBox
        objectName: "equalizerPreset"
        Layout.fillWidth: true
        implicitHeight: root.px(48)
        model: root.effects.presets
        textRole: "name"
        currentIndex: root.effects.presets.findIndex(p => p.id === root.effects.preset)
        displayText: currentIndex < 0 ? qsTr("Custom") : currentText
        Accessible.name: qsTr("Equalizer preset")
        onActivated: (index) => root.effects.setPreset(root.effects.presets[index].id)

        delegate: T.ItemDelegate {
            id: presetOption
            required property var modelData
            required property int index

            readonly property bool chosen: index === presetBox.currentIndex

            width: ListView.view ? ListView.view.width : presetBox.width
            implicitHeight: root.px(44)
            leftPadding: root.px(14 + 24 + 10)
            rightPadding: root.px(14 + 24 + 13)
            highlighted: presetBox.highlightedIndex === index
            Accessible.name: modelData.name

            contentItem: Text {
                id: presetName
                text: presetOption.modelData.name
                color: root.c("#8F8F8F")
                font: presetBox.font
                verticalAlignment: Text.AlignTop
                // the baseline where the design centres a line of Inter
                topPadding: height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize) - presetMetrics.ascent
                FontMetrics { id: presetMetrics; font: presetName.font }
                elide: Text.ElideRight
            }
            background: Rectangle {
                radius: root.px(4)
                color: presetOption.chosen || presetOption.highlighted || presetOption.hovered
                       ? root.c("#1F1F1F") : "transparent"

                // The curve of the preset, from the icons of the sheet
                // (Figma 42:356): drawn in grey and the accent; with another
                // accent, the grey part and the rest tinted above it
                Image {
                    id: curve
                    readonly property string iconDir: "qrc:///redesign/icons/eq-"
                                                      + String(presetOption.modelData.id).toLowerCase()
                    readonly property bool twoTone: !VLCStyle.redesign.defaultAccent
                    x: root.px(14)
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.px(24)
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: iconDir + (twoTone ? "/default-grey.png" : "/default.png")
                    Accessible.role: Accessible.Graphic
                    Accessible.name: qsTr("%1 equalizer curve").arg(presetOption.modelData.name)
                    Impl.ColorImage {
                        anchors.fill: parent
                        visible: curve.twoTone
                        source: visible ? curve.iconDir + "/default-accent.png" : ""
                        color: VLCStyle.redesign.accent
                    }
                }

                Impl.ColorImage {
                    visible: presetOption.chosen
                    x: parent.width - root.px(13) - width
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.px(24)
                    height: width
                    source: "qrc:///redesign/track-check-active.png"
                    color: VLCStyle.redesign.accentTint
                }
            }
        }
    }

    ValueSlider {
        Layout.fillWidth: true
        label: qsTr("Preamp")
        unit: qsTr("dB")
        from: -20
        to: 20
        modelValue: root.effects.preamp
        editable: true
        onEdited: (value) => root.effects.setPreamp(value)
    }

    Repeater {
        model: 10
        ValueSlider {
            required property int index
            readonly property real hz: root.effects.frequencies[index] || 0
            Layout.fillWidth: true
            label: hz >= 1000 ? (hz / 1000).toLocaleString(Qt.locale(), 'f', hz % 1000 ? 1 : 0) + " kHz"
                              : Math.round(hz) + " Hz"
            unit: qsTr("dB")
            from: -20
            to: 20
            modelValue: root.effects.bands[index] || 0
            editable: true
            onEdited: (value) => root.effects.setBand(index, value)
        }
    }

    // A row of the design with a toggle (Figma 614:1430 "Color"): the name
    // (14 px, #6B6B6B) and the 68 x 32 toggle at the end
    Item {
        Layout.fillWidth: true
        implicitHeight: root.px(32)
        Text {
            anchors.left: parent.left
            anchors.right: twoPass.left
            anchors.rightMargin: root.px(10)
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
            text: qsTr("Two-pass equalizer")
            color: root.c("#6B6B6B")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
            elide: Text.ElideRight
        }
        Widgets.CheckButton {
            id: twoPass
            objectName: "equalizerTwoPass"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            focusPolicy: Qt.StrongFocus
            Accessible.name: qsTr("Two-pass equalizer")
            Binding on checked {
                when: !twoPass.pressed
                value: root.effects.twoPass
            }
            onClicked: root.effects.setTwoPass(checked)
        }
    }

    SettingsButton {
        Layout.alignment: Qt.AlignRight
        text: qsTr("Reset")
        Accessible.name: qsTr("Reset to defaults")
        onClicked: root.effects.resetDefaults("equalizer")
    }
}
