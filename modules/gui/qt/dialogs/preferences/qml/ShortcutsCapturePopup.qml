/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.MainInterface
import VLC.Style

Popup {
    id: capturePopup
    property bool captureGlobal: false
    property string captureAction: ""
    property string conflictAction: ""
    property string errorText: ""
    signal keyCaptured(int key, int modifiers)
    // the wheel turned (up or down) with a modifier held: "Ctrl+Mouse Wheel Up"
    signal wheelCaptured(bool up, int modifiers)
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
    // taller when a longer translation of the message takes more lines
    height: Math.max(VLCStyle.dp(205, VLCStyle.scale),
                     topPadding + bottomPadding + 52 + message.implicitHeight + 8 + 76
                     + (wheelHint.visible ? wheelHint.implicitHeight + 8 : 0))
    onOpened: keyInput.forceActiveFocus(Qt.PopupFocusReason)
    // the window lets the wheel with Ctrl come here (not the zoom)
    onOpenedChanged: MainCtx.wheelCapture = opened
    background: Rectangle {
        color: VLCStyle.redesign.color("#1b1b1b", "#ababaa")
        border.color: VLCStyle.redesign.color("#555555", "#8e8e8d")
        radius: 6
    }
    contentItem: FocusScope {
        id: captureFocus
        // (a popup: not under the mirrored interface, right to left)
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        focus: true
        // Esc from a button of the popup (the keyboard left the capture)
        Keys.onEscapePressed: (event) => {
            event.accepted = true
            capturePopup.cancel()
        }
        // Ctrl (Alt, Shift) and a turn of the wheel: a key of the wheel (a
        // whole step, once: a touchpad sends many small ones)
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            property int turned: 0
            onWheel: (wheel) => {
                wheel.accepted = true
                const modifiers = wheel.modifiers & (Qt.ControlModifier | Qt.AltModifier
                                                     | Qt.ShiftModifier | Qt.MetaModifier)
                if (capturePopup.captureGlobal || capturePopup.conflictAction.length > 0
                        || modifiers === 0)
                    return
                const dy = wheel.inverted ? -wheel.angleDelta.y : wheel.angleDelta.y
                if (Math.abs(dy) < Math.abs(wheel.angleDelta.x))
                    return
                turned += dy
                if (Math.abs(turned) < 120)
                    return
                const up = turned > 0
                turned = 0
                capturePopup.wheelCaptured(up, modifiers)
            }
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
            anchors.right: parent.right
            anchors.rightMargin: 14
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            text: capturePopup.captureGlobal ? qsTr("Global Hotkey") : qsTr("Hotkey")
            color: VLCStyle.redesign.color("#dddddd", "#2e2e2e")
            font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
        }
        Label {
            id: message
            anchors.top: parent.top
            anchors.topMargin: 52
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            text: capturePopup.conflictAction.length > 0
                  ? qsTr("This key is assigned to %1. Replace that assignment?").arg(capturePopup.conflictAction)
                  : qsTr("Press a key combination to add to %1 (Esc cancels)").arg(capturePopup.captureAction)
            color: capturePopup.conflictAction.length > 0 ? VLCStyle.redesign.accent
                  : VLCStyle.redesign.color("#aaaaaa", "#4d4d4d")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
        Label {
            id: wheelHint
            // (not for a global hotkey: the system's keys, no wheel)
            visible: !capturePopup.captureGlobal && capturePopup.conflictAction.length === 0
                     && capturePopup.errorText.length === 0
            anchors.top: message.bottom
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            text: qsTr("Or turn the mouse wheel while holding Ctrl, Alt or Shift")
            color: VLCStyle.redesign.color("#6b6b6b", "#6b6b6b")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
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
            color: VLCStyle.redesign.accent
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
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
