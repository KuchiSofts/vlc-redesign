/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.Style

Popup {
    id: capturePopup
    property bool captureGlobal: false
    property string captureAction: ""
    property string conflictAction: ""
    property string errorText: ""
    readonly property bool dark: VLCStyle.palette.isDark
    signal keyCaptured(int key, int modifiers)
    signal clearRequested()
    signal replaceRequested()
    // Esc while asking whether to take the key from another action
    signal conflictCancelled()

    // Esc cancels: the question about a key already used first (back to
    // waiting for a key), else the capture (nothing assigned). Esc can
    // therefore not be given to an action from this editor.
    function cancel() {
        if (conflictAction.length > 0) {
            conflictCancelled()
            keyInput.forceActiveFocus()
        } else {
            close()
        }
    }

    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    height: VLCStyle.dp(205, VLCStyle.scale)
    onOpened: keyInput.forceActiveFocus(Qt.PopupFocusReason)
    background: Rectangle {
        color: capturePopup.dark ? "#1b1b1b" : "#f5f5f5"
        border.color: capturePopup.dark ? "#555555" : "#cccccc"
        radius: 6
    }
    contentItem: FocusScope {
        id: captureFocus
        focus: true
        // Esc from a button of the popup (the keyboard left the capture)
        Keys.onEscapePressed: (event) => {
            event.accepted = true
            capturePopup.cancel()
        }
        TextInput {
            id: keyInput
            focus: true
            width: 1
            height: 1
            opacity: 0
            Keys.priority: Keys.BeforeItem
            // every key comes here, none triggers a shortcut of the window
            Keys.onShortcutOverride: (event) => event.accepted = true
            Keys.onPressed: (event) => {
                event.accepted = true
                if (event.key === Qt.Key_Escape)
                    capturePopup.cancel()
                else if (capturePopup.conflictAction.length === 0)
                    capturePopup.keyCaptured(event.key, event.modifiers)
                // asking whether to take the key: Enter replaces (not the
                // key held from the capture), Tab goes to the buttons
                else if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !event.isAutoRepeat)
                    capturePopup.replaceRequested()
                else if (event.key === Qt.Key_Tab)
                    cancelButton.forceActiveFocus(Qt.TabFocusReason)
                else if (event.key === Qt.Key_Backtab)
                    replaceButton.forceActiveFocus(Qt.BacktabFocusReason)
            }
        }
        Label {
            anchors.top: parent.top
            anchors.topMargin: 14
            anchors.left: parent.left
            anchors.leftMargin: 14
            text: capturePopup.captureGlobal ? qsTr("Global Hotkey") : qsTr("Hotkey")
            color: capturePopup.dark ? "#dddddd" : "#333333"
            font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
        }
        Label {
            anchors.top: parent.top
            anchors.topMargin: 52
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            text: capturePopup.conflictAction.length > 0
                  ? qsTr("This key is assigned to %1. Replace that assignment?").arg(capturePopup.conflictAction)
                  : qsTr("Press a key combination to add to %1 (Esc cancels)").arg(capturePopup.captureAction)
            color: capturePopup.conflictAction.length > 0 ? "#df8950"
                  : (capturePopup.dark ? "#aaaaaa" : "#555555")
            wrapMode: Text.WordWrap
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
        Label {
            visible: capturePopup.errorText.length > 0
                     && capturePopup.conflictAction.length === 0
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 61
            text: capturePopup.errorText
            color: "#df8950"
            elide: Text.ElideRight
            font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
        }
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 14
            spacing: 10
            ActionButton {
                text: qsTr("Unset")
                visible: capturePopup.conflictAction.length === 0
                onClicked: capturePopup.clearRequested()
            }
            ActionButton {
                id: cancelButton
                text: qsTr("Cancel")
                onClicked: capturePopup.close()
            }
            ActionButton {
                id: replaceButton
                text: qsTr("Replace")
                primary: true
                visible: capturePopup.conflictAction.length > 0
                onClicked: capturePopup.replaceRequested()
            }
        }
    }
}
