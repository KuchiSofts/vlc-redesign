/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.Style

Popup {
    id: root
    signal confirmed()

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    height: VLCStyle.dp(172, VLCStyle.scale)
    onOpened: cancelButton.forceActiveFocus(Qt.PopupFocusReason)

    background: Rectangle {
        color: VLCStyle.redesign.color("#1b1b1b", "#ababaa")
        border.color: VLCStyle.redesign.color("#555555", "#8e8e8d")
        radius: 6
    }
    contentItem: FocusScope {
        focus: true
        Label {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 16
            text: qsTr("Reset all shortcuts?")
            color: VLCStyle.redesign.color("#eaeaea", "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
        }
        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 52
            text: qsTr("Local and global shortcuts will return to VLC defaults when you save. Cancel keeps your current settings.")
            color: VLCStyle.redesign.color("#aaaaaa", "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            wrapMode: Text.WordWrap
        }
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16
            spacing: 10
            ActionButton {
                id: cancelButton
                text: qsTr("Cancel")
                onClicked: root.close()
            }
            ActionButton {
                text: qsTr("Reset all")
                primary: true
                onClicked: {
                    root.confirmed()
                    root.close()
                }
            }
        }
    }
}
