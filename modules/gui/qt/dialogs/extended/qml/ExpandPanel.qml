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

// Settings of an effect as the design draws them (Figma 536:1513,
// 6114:13610, 536:1703, 612:1223, 536:1855, 543:990, 614:1430): closed, the
// 48 px "Expand Settings" bar (#090909, radius 6); open, ONE #1B1B1B panel
// whose first 48 px row is "Collapse Settings", the controls under it 16 px
// in. The text (14 px, #6B6B6B) 16 px in, the 24 px chevron (#6B6B6B) 10 px
// from the end, both centred in the 48 px.
Item {
    id: root

    property bool expanded: false
    // From the header to the first control, from the last control to the
    // bottom (the sliders: 10 and 20; the crop: 6 and 16)
    property real topSpacing: px(10)
    property real bottomSpacing: px(20)
    property alias spacing: body.spacing
    default property alias content: body.data

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, value) }

    implicitHeight: expanded ? header.height + topSpacing + body.implicitHeight + bottomSpacing
                             : header.height

    Rectangle {
        anchors.fill: parent
        radius: root.px(6)
        color: root.expanded ? root.c("#1B1B1B") : root.c("#090909")
    }

    T.Button {
        id: header
        width: parent.width
        height: root.px(48)
        focusPolicy: Qt.StrongFocus
        Accessible.name: root.expanded ? qsTr("Collapse settings") : qsTr("Expand settings")
        onClicked: root.expanded = !root.expanded

        background: Rectangle {
            radius: root.px(6)
            color: "transparent"
            border.width: header.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.accent
        }
        contentItem: Item {
            Text {
                x: root.px(16)
                anchors.baseline: parent.verticalCenter
                anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
                text: root.expanded ? qsTr("Collapse Settings") : qsTr("Expand Settings")
                color: root.c("#6B6B6B")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
            }
            Impl.ColorImage {
                x: parent.width - root.px(10) - width
                anchors.verticalCenter: parent.verticalCenter
                width: root.px(24)
                height: width
                sourceSize: Qt.size(width, height)
                source: root.expanded ? "qrc:///redesign/icons/chevron-down/default.png"
                                      : "qrc:///redesign/icons/chevron-right/default.png"
                color: root.c("#6B6B6B")
            }
        }
    }

    ColumnLayout {
        id: body
        visible: root.expanded
        x: root.px(16)
        y: header.height + root.topSpacing
        width: parent.width - root.px(32)
        spacing: 0
    }
}
