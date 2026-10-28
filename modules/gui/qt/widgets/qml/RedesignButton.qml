/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Templates as T

import VLC.Style

// The buttons of the redesign's dialogs (Figma "Cancel" / "Save"): 32 high,
// radius 6, Inter SemiBold 14; neutral #1B1B1B and #8F8F8F, accent
// #371F06 and the accent colour. Outside RedesignDialog (the update banner,
// About).
T.Button {
    id: button

    property bool accent: false
    property bool darkTheme: VLCStyle.palette.isDark

    implicitWidth: Math.max(VLCStyle.dp(100, VLCStyle.scale),
                            buttonText.implicitWidth + 2 * VLCStyle.dp(16, VLCStyle.scale))
    implicitHeight: VLCStyle.dp(32, VLCStyle.scale)
    opacity: enabled ? 1 : 0.5

    Accessible.name: text

    contentItem: Text {
        id: buttonText
        text: button.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: (button.accent && button.enabled) ? VLCStyle.redesign.accent
                                                 : VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e", button.darkTheme)
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        font.weight: Font.DemiBold
    }

    background: Rectangle {
        radius: VLCStyle.dp(6, VLCStyle.scale)
        color: (button.accent && button.enabled)
               ? (button.down || button.hovered ? Qt.lighter(VLCStyle.redesign.accentBackground, 1.15)
                                                : VLCStyle.redesign.accentBackground)
               : (button.down || button.hovered) ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b", button.darkTheme)
               : VLCStyle.redesign.color(VLCStyle.redesign.playerControlBackground, "#ababaa", button.darkTheme)
        border.width: button.visualFocus ? 1 : 0
        border.color: VLCStyle.redesign.focusColor
    }
}
