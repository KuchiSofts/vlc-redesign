/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import VLC.Style

// Section of a settings page of the design: the title (Inter Medium 16
// #C6C6C6, 10 px above and below it), then its rows
Column {
    id: root

    property alias text: title.text
    default property alias rows: rowsColumn.data

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }

    width: parent ? parent.width : implicitWidth

    // a whole number of pixels (the line of Inter 16 is 19.4 px), for the
    // rows below to stay on the pixel grid; the line 10 px from the top
    Item {
        width: parent.width
        height: root.px(39)
        Text {
            id: title
            width: parent.width
            anchors.baseline: parent.top
            // on a whole pixel, as the titles of the option lists (Figma)
            anchors.baselineOffset: Math.round(VLCStyle.redesign.textBaseline(root.px(10), font.pixelSize))
            color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(16)
            font.weight: Font.Medium
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
        }
    }

    Column {
        id: rowsColumn
        width: parent.width
    }
}
