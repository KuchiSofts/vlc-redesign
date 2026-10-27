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

// A setting of an effect (Figma 536:1513, 536:1703, 612:1223, 536:1855):
// the name and the value (14 px, #6B6B6B) in a 17 px text box, 15 px over
// the slider of the settings (SettingsSlider: the 16 px #090909 track, the
// 12 px Accent Bright fill 2 px in, the 2 x 8 ticks, the 8 x 24 Accent Dim
// handle), the next setting 20 px under it: 76 apart. Editable (the
// equalizer, Figma 6114:13610): the name beside a 140 x 36 field (12 px
// text), 10 px over the slider: 90 apart.
ColumnLayout {
    id: root

    property string label
    property string unit
    property real from: 0
    property real to: 1
    property real modelValue: 0
    property int decimals: 1
    // The value shows times it (the spatializer: 0 to 10 for the 0 to 1
    // VLC stores)
    property real displayScale: 1
    property bool editable: false
    property real stepSize: editable ? 0.1 : 0
    // In the units of the slider
    property var ticks: []
    // Its filter off: the grey of the design (Figma 614:1430, the color
    // threshold off)
    property bool active: true
    // At the end of the row of the name (the toggle of a filter of one setting)
    property Component trailing: null

    signal edited(real value)

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, value) }
    function shown(value) { return Number(value * displayScale).toFixed(decimals) }
    function formatted(value) { return shown(value) + (unit.length ? " " + unit : "") }

    // (10 px under a field or a toggle)
    spacing: nameRow.implicitHeight > px(17) ? px(10) : px(15)

    Item {
        id: nameRow
        Layout.fillWidth: true
        implicitHeight: root.editable ? root.px(36) : root.trailing ? root.px(32) : root.px(17)

        Text {
            id: nameText
            anchors.left: parent.left
            anchors.right: root.editable ? valueField.left : valueText.left
            anchors.rightMargin: root.px(10)
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize, parent.height)
            text: root.label
            color: root.c("#6B6B6B")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            HoverHandler { id: nameHover }
            T.ToolTip.visible: nameHover.hovered && nameText.truncated
            T.ToolTip.text: nameText.text
            T.ToolTip.delay: 650
        }

        Text {
            id: valueText
            visible: !root.editable
            anchors.right: trailingLoader.active ? trailingLoader.left : parent.right
            anchors.rightMargin: trailingLoader.active ? root.px(10) : 0
            anchors.baseline: nameText.baseline
            text: root.formatted(root.modelValue)
            color: root.c("#6B6B6B")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
        }

        // The field of the design (SettingsTextField: #090909, the 2 px
        // #191919 border, the text 14 px in), 12 px text, the unit after it
        SettingsTextField {
            id: valueField
            visible: root.editable
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: root.px(140)
            height: root.px(36)
            font.pixelSize: root.px(12)
            suffix: root.unit
            text: root.shown(root.modelValue)
            Accessible.name: root.label
            onEditingFinished: {
                const number = Number(text.replace(root.unit, "").trim().replace(",", ".")) / root.displayScale
                if (Number.isFinite(number) && number >= root.from && number <= root.to)
                    root.edited(number)
                text = root.shown(root.modelValue)
            }
            Connections {
                target: root
                function onModelValueChanged() {
                    if (!valueField.activeFocus) valueField.text = root.shown(root.modelValue)
                }
            }
        }

        Loader {
            id: trailingLoader
            active: !!root.trailing
            sourceComponent: root.trailing
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    T.Slider {
        id: slider

        readonly property real handleCenter: root.px(2) + visualPosition * (availableWidth - root.px(4))
        readonly property color fill: root.active ? root.c(VLCStyle.redesign.accentBackground)
                                                  : VLCStyle.redesign.color("#232323", "#c2c2c1")

        function fraction(value) { return to > from ? Math.max(0, Math.min(1, (value - from) / (to - from))) : 0 }
        // Screen blend of the ticks (#1A1611) over what is below them
        function screen(below) {
            const tick = Qt.color("#1A1611")
            return Qt.rgba(1 - (1 - below.r) * (1 - tick.r), 1 - (1 - below.g) * (1 - tick.g),
                           1 - (1 - below.b) * (1 - tick.b), 1)
        }

        Layout.fillWidth: true
        implicitHeight: root.px(24)
        padding: 0
        from: root.from
        to: root.to
        stepSize: root.stepSize
        snapMode: T.Slider.SnapAlways
        focusPolicy: Qt.StrongFocus
        Accessible.name: root.label
        Binding on value {
            when: !slider.pressed
            value: root.modelValue
        }
        onMoved: root.edited(value)
        // A continuous slider (stepSize 0) steps by 0.1 in its units with the
        // arrow keys: 1800 presses for a hue from 0 to 180. Its keys step by
        // 1 % of the range instead; dragging stays continuous
        Keys.onPressed: (event) => {
            if (root.stepSize > 0 || to <= from)
                return
            const up = event.key === Qt.Key_Up || event.key === (mirrored ? Qt.Key_Left : Qt.Key_Right)
            const down = event.key === Qt.Key_Down || event.key === (mirrored ? Qt.Key_Right : Qt.Key_Left)
            if (!up && !down)
                return
            event.accepted = true
            const step = (to - from) / 100
            const next = Math.max(from, Math.min(to, value + (up ? step : -step)))
            if (next !== value) {
                value = next
                root.edited(next)
            }
        }

        background: Rectangle {
            y: root.px(4)
            width: slider.availableWidth
            height: root.px(16)
            radius: root.px(6)
            color: root.c("#090909")
            border.width: slider.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.accent

            // from the low end to the handle (right to left: the low end on
            // the right, visualPosition mirrored)
            Rectangle {
                x: slider.mirrored ? slider.handleCenter : root.px(2)
                y: root.px(2)
                width: Math.max(0, slider.mirrored ? slider.availableWidth - root.px(2) - slider.handleCenter
                                                   : slider.handleCenter - root.px(2))
                height: root.px(12)
                radius: root.px(4)
                color: slider.fill
            }

            Repeater {
                model: root.ticks
                Rectangle {
                    required property var modelData
                    readonly property real visualFraction: slider.mirrored ? 1 - slider.fraction(modelData)
                                                                           : slider.fraction(modelData)
                    readonly property real center: root.px(2) + visualFraction * (slider.availableWidth - root.px(4))
                    // none at the ends of the track
                    visible: slider.fraction(modelData) > 0 && slider.fraction(modelData) < 1
                    x: center - width / 2
                    y: root.px(4)
                    width: root.px(2)
                    height: root.px(8)
                    radius: root.px(1)
                    color: slider.screen(Qt.color((slider.mirrored ? center > slider.handleCenter : center < slider.handleCenter)
                                                  ? slider.fill : root.c("#090909")))
                }
            }
        }

        handle: Rectangle {
            // on whole pixels, as the design places it
            x: Math.round(Math.max(0, Math.min(slider.availableWidth - width, slider.handleCenter - width / 2)))
            width: root.px(8)
            height: root.px(24)
            radius: root.px(4)
            color: root.active ? VLCStyle.redesign.accentDim : VLCStyle.redesign.color("#707070", "#8e8e8d")
        }
    }
}
