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

// A line of information in the tool windows (Statistics, Folder
// information): the name in "Dark/FG/A2" on the left, the value in
// "Dark/FG/2" on the right, Inter 14 on a row of 32, a line of "Dark/
// Divider/Dim" under it (but under the last one)
Item {
    id: root

    property string label
    property string value
    property bool divider: true
    // a value that may be long (a path): on more lines, selectable
    property bool wraps: false
    property bool darkTheme: VLCStyle.palette.isDark

    // (in a popup: not under the mirrored interface)
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Layout.fillWidth: true
    implicitHeight: Math.max(VLCStyle.dp(32, VLCStyle.scale),
                             valueText.implicitHeight + 2 * VLCStyle.dp(7, VLCStyle.scale))

    Accessible.role: Accessible.StaticText
    Accessible.name: label + ": " + value

    Text {
        id: labelText

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: VLCStyle.dp(7, VLCStyle.scale)
        width: Math.min(implicitWidth, parent.width * 0.45)
        text: root.label
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
        color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e", root.darkTheme)
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
    }

    TextEdit {
        id: valueText

        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: VLCStyle.dp(7, VLCStyle.scale)
        width: parent.width - labelText.width - VLCStyle.dp(20, VLCStyle.scale)
        text: root.value
        readOnly: true
        selectByMouse: true
        wrapMode: root.wraps ? TextEdit.WrapAnywhere : TextEdit.NoWrap
        clip: !root.wraps
        horizontalAlignment: TextEdit.AlignRight
        color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e", root.darkTheme)
        selectionColor: VLCStyle.redesign.color(VLCStyle.redesign.accentBackground, "#dedede", root.darkTheme)
        selectedTextColor: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#2e2e2e", root.darkTheme)
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        activeFocusOnPress: false
    }

    Rectangle {
        visible: root.divider
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: VLCStyle.redesign.color("#222222", "#9c9c9b", root.darkTheme)
    }
}
