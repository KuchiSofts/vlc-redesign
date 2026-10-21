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
    implicitWidth: 100
    implicitHeight: 32
    font.family: VLCStyle.redesign.fontFamily
    font.pixelSize: 16
    // Figma 1902:6770: Inter Semi Bold 16 (Back #8F8F8F); Browse is Regular
    font.weight: Font.DemiBold
    background: Rectangle {
        radius: 6
        color: !control.enabled ? "#292929"
             : control.primary ? VLCStyle.redesign.accent
             : control.down || control.hovered ? "#303030" : "#1b1b1b"
        // reached with the keyboard: outlined as the toggles are
        border.width: control.visualFocus ? 2 : 0
        border.color: "#eaeaea"
    }
    // Figma: the text box (its advance rounded up) centred in the button
    contentItem: Item {
        Text {
            id: label
            x: control.floorText ? Math.floor((parent.width - width) / 2) : (parent.width - width) / 2
            width: Math.ceil(exactMetrics.advanceWidth / 16 - text.length * font.pixelSize / 2816 - 0.001)
            height: parent.height
            text: control.text
            color: !control.enabled ? "#666666" : control.primary ? "#161616" : "#8f8f8f"
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
