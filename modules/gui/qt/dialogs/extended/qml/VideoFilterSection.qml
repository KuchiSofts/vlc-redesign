/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import VLC.Style
import VLC.Widgets as Widgets

// A video filter in the one panel of its section, as the design groups
// them (Figma 536:1855 Essential, 614:1430 Colors): a 32 px row of its name
// (14 px, #6B6B6B) and its ON / OFF toggle at the end (a color option of the
// filter between them: "Color Extraction"), then its options; a filter of
// one slider holds the toggle at the end of the row of its slider ("Sepia
// Intensity"). 2 px #1E1E1E across the panel between two filters, 19 px
// from what it parts (9 for a filter without options, 14 for one without
// sliders: Figma 614:1430).
ColumnLayout {
    id: root

    required property var effects
    required property string filterId
    required property string title
    // The names and the ticks of the design, by option
    property var labels: ({})
    property var ticks: ({})
    // No rule above the first filter
    property bool first: false
    property var items: []

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value) }
    function isSlider(option) {
        return (option.type === "float" || option.type === "integer")
            && option.minimum !== undefined && option.maximum !== undefined
    }
    function isColor(option) { return option.type === "color" || option.id === "extract-component" }

    readonly property bool filterOn: { effects.revision; return effects.filterEnabled(filterId) }
    // A color first goes in the row of the name
    readonly property var headerOption: items.length > 0 && isColor(items[0]) ? items[0] : null
    readonly property var rows: headerOption ? items.slice(1) : items
    readonly property bool sliderToggle: !headerOption && rows.length === 1 && isSlider(rows[0])
    readonly property real pad: !headerOption && rows.length === 0 ? px(9)
                              : headerOption || rows.some(option => root.isSlider(option)) ? px(19)
                              : px(14)

    spacing: 0
    visible: effects.filterAvailable(filterId)

    Component.onCompleted: items = effects.parameters(filterId)

    // The ON / OFF toggle of the design
    Component {
        id: filterToggle
        Widgets.CheckButton {
            id: toggle
            objectName: "videoFilter-" + root.filterId
            focusPolicy: Qt.StrongFocus
            Accessible.name: qsTr("Enable %1").arg(root.title)
            Binding on checked {
                when: !toggle.pressed
                value: root.filterOn
            }
            onClicked: root.effects.setFilterEnabled(root.filterId, checked)
        }
    }

    // The rule between two filters, across the panel (16 px out of its controls)
    Item {
        visible: !root.first
        Layout.fillWidth: true
        implicitHeight: root.px(2)
        Rectangle {
            x: -root.px(16)
            width: parent.width + root.px(32)
            height: parent.height
            // (the separators of the light theme: #222222)
            color: VLCStyle.redesign.color("#1E1E1E", "#9c9c9b")
        }
    }

    Item {
        visible: !root.sliderToggle
        Layout.fillWidth: true
        Layout.topMargin: root.first ? 0 : root.pad
        implicitHeight: root.px(32)

        Text {
            id: titleText
            anchors.left: parent.left
            anchors.right: headerColor.active ? headerColor.left : headerToggle.left
            anchors.rightMargin: root.px(10)
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
            text: root.title
            color: root.c("#6B6B6B")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            HoverHandler { id: titleHover }
            T.ToolTip.visible: titleHover.hovered && titleText.truncated
            T.ToolTip.text: titleText.text
            T.ToolTip.delay: 650
        }
        // Figma 614:1430: the field 8 px before the toggle
        Loader {
            id: headerColor
            active: !!root.headerOption
            anchors.right: headerToggle.left
            anchors.rightMargin: root.px(8)
            anchors.verticalCenter: parent.verticalCenter
            sourceComponent: VideoOption {
                effects: root.effects
                option: root.headerOption
                label: root.title
                compact: true
            }
        }
        Loader {
            id: headerToggle
            active: !root.sliderToggle
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            sourceComponent: filterToggle
        }
    }

    Repeater {
        model: root.rows
        VideoOption {
            required property var modelData
            required property int index
            readonly property bool afterSlider: index > 0 && root.isSlider(root.rows[index - 1])
            Layout.fillWidth: true
            // 15 px under the row of the name or a row, 20 under a slider
            // (Figma 614:1430); in place of that row, the slider of a
            // filter of one
            Layout.topMargin: root.sliderToggle ? (root.first ? 0 : root.pad)
                                                : afterSlider ? root.px(20) : root.px(15)
            effects: root.effects
            option: modelData
            label: root.sliderToggle ? root.title : (root.labels[modelData.id] ?? modelData.name)
            ticks: root.ticks[modelData.id] || []
            active: root.filterOn
            trailing: root.sliderToggle ? filterToggle : null
        }
    }

    Item {
        Layout.fillWidth: true
        implicitHeight: root.pad
    }
}
