/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic as Controls

import VLC.Style

Controls.Button {
    id: control
    property bool primary: false
    // the text box on the pixel at its left (Figma's Browse)
    property bool floorText: false
    // the 100 of the design, wider for a longer word (16 on each side)
    implicitWidth: Math.max(100, label.width + 32)
    implicitHeight: 32
    font.family: VLCStyle.redesign.fontFamily
    font.pixelSize: 16
    // Figma 1902:6770: Inter Semi Bold 16 (Back #8F8F8F); Browse is Regular
    font.weight: Font.DemiBold
    background: Rectangle {
        radius: 6
        color: !control.enabled ? VLCStyle.redesign.color("#292929", "#b8b8b7")
             : control.primary ? VLCStyle.redesign.accent
             : control.down || control.hovered ? VLCStyle.redesign.color("#303030", "#9c9c9b")
             : VLCStyle.redesign.color("#1b1b1b", "#ababaa")
        // reached with the keyboard: outlined as the toggles are
        border.width: control.visualFocus ? 2 : 0
        border.color: VLCStyle.redesign.color("#eaeaea", "#2e2e2e")
    }
    // Figma: the text box (its advance rounded up) centred in the button
    contentItem: Item {
        Text {
            id: label
            x: control.floorText ? Math.floor((parent.width - width) / 2) : (parent.width - width) / 2
            width: Math.ceil(exactMetrics.advanceWidth / 16 - text.length * font.pixelSize / 2816 - 0.001)
            height: parent.height
            text: control.text
            color: !control.enabled ? VLCStyle.redesign.color("#666666", "#7c7c7b")
                 : control.primary ? "#161616" : VLCStyle.redesign.color("#8f8f8f", "#2e2e2e")
            font: control.font
            verticalAlignment: Text.AlignVCenter
            // the advance unrounded (Qt's are in 1/64 px): measured 16
            // times larger
            TextMetrics {
                id: exactMetrics
                font.family: label.font.family
                font.weight: label.font.weight
                font.pixelSize: label.font.pixelSize * 16
                text: label.text
            }
        }
    }
}
