/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts

import VLC.Style

// Figma "Frame 324" of the dialogs: a title (Inter 16, line height 175 %,
// "Dark/FG/2"), optionally followed by a muted "(optional)", and 5 under
// it a line of explanation (Inter 14, line height 175 %, "Dark/FG/A3")
ColumnLayout {
    id: root

    property string text
    property string suffix
    property string description
    // a title longer than the line (a message, a question) goes on more
    // lines, else it is cut
    property bool headingWraps: false
    property bool darkTheme: VLCStyle.palette.isDark

    Layout.fillWidth: true
    spacing: VLCStyle.dp(5, VLCStyle.scale)

    // the line of a text box as Figma lays it: the glyphs (1.2109 em)
    // centred in the line height, from the top of the box
    component FigmaText: Text {
        id: figmaText

        property real lineHeightPx

        Layout.fillWidth: true
        Layout.preferredHeight: Math.ceil(lineHeightPx)
        // from the start of the line (the right in a right to left layout)
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignTop
        elide: Text.ElideRight
        font.family: VLCStyle.redesign.fontFamily
        // the baseline on the whole pixel where Figma's glyphs land
        topPadding: Math.round((lineHeightPx - 1.2109375 * font.pixelSize) / 2 + 0.96875 * font.pixelSize) - metrics.ascent

        FontMetrics {
            id: metrics
            font: figmaText.font
        }
    }

    FigmaText {
        lineHeightPx: VLCStyle.dp(28, VLCStyle.scale)
        wrapMode: root.headingWraps ? Text.Wrap : Text.NoWrap
        elide: root.headingWraps ? Text.ElideNone : Text.ElideRight
        // the lines 28 apart, as Figma sets them (one line: as before)
        lineHeightMode: root.headingWraps ? Text.FixedHeight : Text.ProportionalHeight
        lineHeight: root.headingWraps ? lineHeightPx : 1.0
        Layout.preferredHeight: root.headingWraps && lineCount > 1 ? implicitHeight : Math.ceil(lineHeightPx)
        textFormat: Text.StyledText
        text: root.suffix === "" ? root.text
                                 : "%1 <font color=\"%2\">%3</font>".arg(root.text)
                                       .arg(VLCStyle.redesign.color("#6B6B6B", "#4d4d4d", root.darkTheme))
                                       .arg(root.suffix)
        color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e", root.darkTheme)
        font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
    }

    FigmaText {
        visible: root.description !== ""
        lineHeightPx: VLCStyle.dp(24.5, VLCStyle.scale)
        // longer than the line (a translation): on more lines, not cut
        wrapMode: Text.Wrap
        elide: Text.ElideNone
        Layout.preferredHeight: lineCount > 1 ? implicitHeight : Math.ceil(lineHeightPx)
        text: root.description
        color: VLCStyle.redesign.color("#6B6B6B", "#4d4d4d", root.darkTheme)
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
    }
}
