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
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Style

// Figma "Frame 86", the text input of the dialogs: "Dark/BG/B2" (#090909),
// radius 6, a border of 2 inside ("Dark/BG/A4", #191919) counted in the
// layout, padding 8 / 18 / 8 / 12 from it, the text Inter 14 (lines of 17),
// the placeholder #404040. One line (37 high, in a row of 41) unless
// multiLine (194 high); Return then accepts.
T.TextArea {
    id: root

    property bool multiLine: false
    property bool darkTheme: VLCStyle.palette.isDark
    readonly property real border: VLCStyle.dp(2, VLCStyle.scale)

    signal accepted()

    Layout.fillWidth: true
    // the row of 41 around the single line field
    Layout.topMargin: multiLine ? 0 : VLCStyle.dp(2, VLCStyle.scale)
    Layout.bottomMargin: multiLine ? 0 : VLCStyle.dp(2, VLCStyle.scale)
    implicitHeight: multiLine ? VLCStyle.dp(194, VLCStyle.scale)
                              : VLCStyle.dp(8 + 21 + 8, VLCStyle.scale)
    // the first line as Figma places it
    topPadding: VLCStyle.redesign.textBaseline(border + VLCStyle.dp(8, VLCStyle.scale), font.pixelSize) - metrics.ascent
    bottomPadding: border + VLCStyle.dp(8, VLCStyle.scale)
    leftPadding: border + VLCStyle.dp(12, VLCStyle.scale)
    rightPadding: border + VLCStyle.dp(18, VLCStyle.scale)
    verticalAlignment: TextEdit.AlignTop
    wrapMode: multiLine ? TextEdit.WrapAnywhere : TextEdit.NoWrap

    color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e", root.darkTheme)
    placeholderTextColor: VLCStyle.redesign.color(VLCStyle.redesign.textDisabled, "#7c7c7b", root.darkTheme)
    selectionColor: VLCStyle.redesign.accentBackground
    selectedTextColor: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#191919", root.darkTheme)
    font.family: VLCStyle.redesign.fontFamily
    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
    selectByMouse: true

    FontMetrics {
        id: metrics
        font: root.font
    }

    // Return accepts; in multiple lines it adds a line, Ctrl+Return accepts.
    // On one line Tab goes to the next field (the text area would type it).
    Keys.onPressed: (event) => {
        if (!root.multiLine && (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab)) {
            event.accepted = true
            const forward = event.key === Qt.Key_Tab && !(event.modifiers & Qt.ShiftModifier)
            const next = root.nextItemInFocusChain(forward)
            if (next && next !== root)
                next.forceActiveFocus(forward ? Qt.TabFocusReason : Qt.BacktabFocusReason)
            return
        }
        if (event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter)
            return
        if (root.multiLine && !(event.modifiers & Qt.ControlModifier))
            return
        event.accepted = true
        root.accepted()
    }

    background: Rectangle {
        radius: VLCStyle.dp(6, VLCStyle.scale)
        color: VLCStyle.redesign.color("#090909", "#dedede", root.darkTheme)
        border.width: root.border
        border.color: VLCStyle.redesign.color("#191919", "#c4c4c3", root.darkTheme)
    }

    // The placeholder of T.TextArea is drawn by the style; draw it here
    Text {
        x: root.leftPadding
        y: root.topPadding
        width: root.width - root.leftPadding - root.rightPadding
        visible: root.length === 0 && root.preeditText === ""
        text: root.placeholderText
        color: root.placeholderTextColor
        font: root.font
        elide: Text.ElideRight
    }
}
