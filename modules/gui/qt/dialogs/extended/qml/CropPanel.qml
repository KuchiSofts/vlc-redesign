/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl
import VLC.Style

// The crop of the design (Figma 543:990): two rows of two 141 x 33 fields,
// 8 px apart, the 24 px link 11 px after them (3 px in); the rows 6 px
// under the header, 16 px apart and over the bottom
ExpandPanel {
    id: root

    required property var effects
    property bool verticalLinked: false
    property bool horizontalLinked: false

    function updateCrop(side, pixels) {
        if (!effects.setCrop(side, pixels)) return
        if (verticalLinked && side === "top") effects.setCrop("bottom", pixels)
        if (verticalLinked && side === "bottom") effects.setCrop("top", pixels)
        if (horizontalLinked && side === "left") effects.setCrop("right", pixels)
        if (horizontalLinked && side === "right") effects.setCrop("left", pixels)
    }

    topSpacing: px(6)
    bottomSpacing: px(16)
    spacing: px(16)

    RowLayout {
        Layout.fillWidth: true
        spacing: root.px(8)
        CropInput {
            Layout.fillWidth: true
            effects: root.effects
            side: "top"
            label: qsTr("Top")
            onEdited: (pixels) => root.updateCrop(side, pixels)
        }
        CropInput {
            Layout.fillWidth: true
            effects: root.effects
            side: "bottom"
            label: qsTr("Bottom")
            onEdited: (pixels) => root.updateCrop(side, pixels)
        }
        T.Button {
            id: verticalLink
            objectName: "linkVerticalCrop"
            Layout.leftMargin: root.px(3)
            Layout.rightMargin: root.px(3)
            implicitWidth: root.px(24)
            implicitHeight: implicitWidth
            focusPolicy: Qt.StrongFocus
            onClicked: root.verticalLinked = !root.verticalLinked
            Accessible.name: qsTr("Link top and bottom crop")
            background: Rectangle {
                radius: root.px(4)
                color: "transparent"
                border.width: verticalLink.visualFocus ? 1 : 0
                border.color: VLCStyle.redesign.accent
            }
            contentItem: Impl.ColorImage {
                source: root.verticalLinked ? "qrc:///redesign/icons/link/active.svg"
                                            : "qrc:///redesign/icons/link/default.svg"
                color: root.verticalLinked ? VLCStyle.redesign.accentTint : "transparent"
                sourceSize.width: root.px(24)
                sourceSize.height: root.px(24)
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: root.px(8)
        CropInput {
            Layout.fillWidth: true
            effects: root.effects
            side: "left"
            label: qsTr("Left")
            onEdited: (pixels) => root.updateCrop(side, pixels)
        }
        CropInput {
            Layout.fillWidth: true
            effects: root.effects
            side: "right"
            label: qsTr("Right")
            onEdited: (pixels) => root.updateCrop(side, pixels)
        }
        T.Button {
            id: horizontalLink
            objectName: "linkHorizontalCrop"
            Layout.leftMargin: root.px(3)
            Layout.rightMargin: root.px(3)
            implicitWidth: root.px(24)
            implicitHeight: implicitWidth
            focusPolicy: Qt.StrongFocus
            onClicked: root.horizontalLinked = !root.horizontalLinked
            Accessible.name: qsTr("Link left and right crop")
            background: Rectangle {
                radius: root.px(4)
                color: "transparent"
                border.width: horizontalLink.visualFocus ? 1 : 0
                border.color: VLCStyle.redesign.accent
            }
            contentItem: Impl.ColorImage {
                source: root.horizontalLinked ? "qrc:///redesign/icons/link/active.svg"
                                              : "qrc:///redesign/icons/link/default.svg"
                color: root.horizontalLinked ? VLCStyle.redesign.accentTint : "transparent"
                sourceSize.width: root.px(24)
                sourceSize.height: root.px(24)
            }
        }
    }
}
