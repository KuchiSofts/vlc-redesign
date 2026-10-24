/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.Style
import VLC.Util

// Row of a settings page whose control is a button of the design
// ("Defaults: Edit Keyboard Shortcuts", "Clear history now: Clear")
Item {
    id: root

    property string label
    property string help
    property bool showInfo: help.length > 0
    property string buttonText
    property bool destructive: false
    // 320 px buttons fill the column of the controls; Clear, Browse: 100 px
    property real buttonWidth: px(320)

    signal clicked()

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, value) }

    width: parent ? parent.width : implicitWidth
    implicitHeight: px(42)

    Row {
        id: labelRow
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: root.px(10)

        Text {
            id: rowLabel
            // cut short before the button (a long translation)
            width: Math.max(0, Math.min(implicitWidth, root.width - rowButton.width - root.px(20)
                                        - (infoIcon.visible ? infoIcon.width + labelRow.spacing : 0)))
            elide: Text.ElideRight
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
            text: root.label
            color: root.c("#8F8F8F")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
            HoverHandler { id: labelHover }
            ToolTip.visible: labelHover.hovered && rowLabel.truncated
            ToolTip.text: rowLabel.text
            ToolTip.delay: 650
        }
        Image {
            id: infoIcon
            visible: root.showInfo
            anchors.verticalCenter: parent.verticalCenter
            width: root.px(18)
            height: width
            sourceSize: Qt.size(width, height)
            source: SVGColorImage.colorize("qrc:///redesign/duo/info-small.svg").color1(root.c("#404040")).uri()
        }
    }

    SettingsButton {
        id: rowButton
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        // wider for a longer translation, up to the column of the controls
        width: Math.min(Math.max(root.buttonWidth, implicitWidth), root.width * 0.46)
        text: root.buttonText
        destructive: root.destructive
        onClicked: root.clicked()
    }
}
