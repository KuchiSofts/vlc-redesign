/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import VLC.Style

// Startup volume of the design (Figma "Values", vertical): 320 x 78,
// #090909, radius 6, 2 px around and between two 316 x 36 options (radius 4,
// 12 px inset): "Remember last volume", then "Always starts at:" with its
// 182 x 32 field (radius 2, #090909). The chosen one is Accent Bright.
Rectangle {
    id: root

    // Remember the last volume, or start at volume (1: 100 %)
    property bool remember: true
    property real volume: 1

    signal chosen(bool remember)
    signal volumeEdited(real volume)

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, value) }

    implicitWidth: px(320)
    implicitHeight: px(78)
    radius: px(6)
    color: c("#090909")

    component Option: Rectangle {
        property bool selected
        property alias text: label.text
        signal picked()
        width: root.width - root.px(4)
        height: root.px(36)
        radius: root.px(4)
        color: selected ? root.c(VLCStyle.redesign.accentBackground)
                        : hover.hovered ? root.c("#222222") : root.c("#191919")
        // reached with Tab (a click gives it no focus), chosen with Space
        // or Enter
        activeFocusOnTab: true
        border.width: activeFocus ? 1 : 0
        border.color: VLCStyle.redesign.accent
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Space || event.key === Qt.Key_Return
             || event.key === Qt.Key_Enter) {
                picked()
                event.accepted = true
            }
        }
        Text {
            id: label
            x: root.px(12)
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
            color: parent.selected ? VLCStyle.redesign.accent : root.c("#6B6B6B")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
        }
        HoverHandler { id: hover }
        TapHandler { onTapped: parent.picked() }
    }

    Column {
        x: root.px(2)
        y: root.px(2)
        spacing: root.px(2)

        Option {
            objectName: "settingsRememberVolume"
            selected: root.remember
            text: qsTr("Remember last volume")
            onPicked: root.chosen(true)
            Accessible.role: Accessible.RadioButton
            Accessible.name: text
            Accessible.checked: selected
        }

        Option {
            id: startsAt
            objectName: "settingsStartsAtVolume"
            selected: !root.remember
            text: qsTr("Always starts at:")
            onPicked: { root.chosen(false); volumeInput.forceActiveFocus() }
            Accessible.role: Accessible.RadioButton
            Accessible.name: text
            Accessible.checked: selected

            // The field: 10 px after the label, 2 px from the other sides
            Rectangle {
                x: root.px(12 + 110 + 10)
                y: root.px(2)
                width: parent.width - x - root.px(2)
                height: parent.height - root.px(4)
                radius: root.px(2)
                color: root.c("#090909")
                border.width: volumeInput.activeFocus ? 1 : 0
                border.color: VLCStyle.redesign.accentDim

                TextInput {
                    id: volumeInput
                    objectName: "settingsStartupVolumeValue"
                    x: root.px(12)
                    width: parent.width - root.px(12 + 18)
                    anchors.baseline: parent.verticalCenter
                    anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
                    text: Math.round(root.volume * 100) + "%"
                    color: startsAt.selected ? root.c("#8F8F8F") : root.c("#404040")
                    selectionColor: VLCStyle.redesign.accentBackground
                    selectedTextColor: VLCStyle.redesign.accent
                    selectByMouse: true
                    activeFocusOnTab: true
                    clip: true
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(14)
                    validator: RegularExpressionValidator { regularExpression: /\d{0,3}\s*%?/ }
                    // clicked or typed in, not only passed through with Tab
                    TapHandler { onTapped: root.chosen(false) }
                    onTextEdited: root.chosen(false)
                    onEditingFinished: {
                        const percent = parseInt(text)
                        if (!isNaN(percent))
                            root.volumeEdited(Math.max(0, Math.min(percent, 125)) / 100)
                        text = Qt.binding(() => Math.round(root.volume * 100) + "%")
                    }
                    Accessible.name: qsTr("Startup volume")
                }
            }
        }
    }
}
