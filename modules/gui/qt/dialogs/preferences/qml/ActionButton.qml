/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.Style

Button {
    id: control
    property bool primary: false

    implicitWidth: Math.max(VLCStyle.dp(88, VLCStyle.scale),
                            label.implicitWidth + VLCStyle.dp(28, VLCStyle.scale))
    implicitHeight: VLCStyle.dp(34, VLCStyle.scale)
    font.family: VLCStyle.redesign.fontFamily
    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)

    background: Rectangle {
        radius: 4
        // the accent one stays dark in the light theme, as in RedesignDialog
        color: !control.enabled ? VLCStyle.redesign.color("#1c1c1c", "#b8b8b7")
               : control.primary ? (control.down ? Qt.lighter(VLCStyle.redesign.accentBackground, 1.2) : VLCStyle.redesign.accentBackground)
               : control.down ? VLCStyle.redesign.color("#303030", "#929291")
               : control.hovered ? VLCStyle.redesign.color("#292929", "#9c9c9b")
               : VLCStyle.redesign.color("#202020", "#ababaa")
        border.color: control.visualFocus ? VLCStyle.redesign.accent
                      : control.primary ? "transparent"
                      : VLCStyle.redesign.color("#353535", "#8e8e8d")
    }
    contentItem: Label {
        id: label
        text: control.text
        color: !control.enabled ? VLCStyle.redesign.color("#666666", "#7c7c7b")
               : control.primary ? VLCStyle.redesign.accent
               : VLCStyle.redesign.color("#d0d0d0", "#2e2e2e")
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
