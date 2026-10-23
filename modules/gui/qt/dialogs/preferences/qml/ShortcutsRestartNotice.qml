/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.Style

Dialog {
    id: root
    objectName: "shortcutsRestartNotice"
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    title: qsTr("Shortcuts saved")
    background: Rectangle {
        color: VLCStyle.redesign.color("#1b1b1b", "#ababaa")
        radius: 6
        border.color: VLCStyle.redesign.color("#555555", "#8e8e8d")
    }
    header: Label {
        text: root.title
        color: VLCStyle.redesign.color("#eaeaea", "#2e2e2e")
        padding: 16
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
    }
    footer: ActionButton {
        text: qsTr("OK")
        primary: true
        onClicked: root.accept()
    }
    width: VLCStyle.dp(420, VLCStyle.scale)
    contentItem: Label {
        text: qsTr("Restart VLC to use the new keyboard shortcuts.")
        color: VLCStyle.redesign.color("#dddddd", "#2e2e2e")
        wrapMode: Text.WordWrap
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
    }
}
