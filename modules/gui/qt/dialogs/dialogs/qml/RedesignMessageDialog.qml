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

import VLC.MainInterface
import VLC.Dialogs
import VLC.Style
import VLC.Widgets as Widgets

// The messages, questions and prompts of the C++ side (DialogsProvider::
// showRedesignMessage(): the play queue to restore after a crash, the
// confirmations of the library, a text to type), one after the other, as
// the other dialogs of the redesign
Widgets.RedesignDialog {
    id: root

    objectName: "redesignMessageDialog"
    // over the dialog that asked (Settings, the effects...)
    z: 100

    // the message shown (-1: none) and the ones waiting
    property int messageId: -1
    property var _queue: []
    property string heading: ""
    property string description: ""
    // RedesignMessage::Input: 0 none, 1 a line, 2 several lines
    property int inputMode: 0
    property bool defaultAccept: true
    // RedesignMessage::Answer: 0 cancelled, 1 the action, 2 the other one
    property int _answer: 0
    // dropped by the C++ side: closed without an answer
    property bool _dropped: false

    preferredWidth: VLCStyle.dp(inputMode === 2 ? 713 : 560, VLCStyle.scale)

    // the one showing the messages (the last made, if the interface is
    // made again)
    Component.onCompleted: DialogsProvider.setRedesignMessageHost(root)
    Component.onDestruction: DialogsProvider.releaseRedesignMessageHost(root)

    function enqueue(id, message) {
        _queue.push({ id: id, message: message })
        if (messageId < 0 && !visible)
            showNext()
    }

    // the message dropped (VLC quitting, another window showing it)
    function drop(id) {
        if (id === messageId) {
            _dropped = true
            close()
            return
        }
        _queue = _queue.filter((entry) => entry.id !== id)
    }

    function showNext() {
        if (messageId >= 0 || _queue.length === 0)
            return
        const entry = _queue.shift()
        const message = entry.message
        messageId = entry.id
        _answer = 0
        _dropped = false
        title = message.title || ""
        heading = message.heading || ""
        description = message.text || ""
        acceptText = message.acceptText || qsTr("OK")
        cancelText = message.cancelText || qsTr("Cancel")
        cancelVisible = message.cancelVisible !== false
        extraText = message.extraText || ""
        defaultAccept = message.defaultAccept !== false
        inputMode = message.input || 0
        field.text = message.inputText || ""
        field.placeholderText = message.placeholder || ""
        open()
    }

    // a text of the program, not markup (a path, an error)
    function _plain(text) {
        return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
                   .replace(/\n/g, "<br>")
    }

    onOpened: {
        if (inputMode > 0) {
            field.forceActiveFocus()
            if (inputMode === 1)
                field.selectAll()
        }
    }
    onAccepted: _answer = 1
    onExtraClicked: {
        _answer = 2
        close()
    }
    onClosed: {
        const id = messageId
        messageId = -1
        if (id >= 0 && !_dropped)
            DialogsProvider.answerRedesignMessage(id, _answer, inputMode > 0 ? field.text : "")
        _dropped = false
        Qt.callLater(showNext)
    }

    Widgets.RedesignDialogHeading {
        // (a popup of its own: not under the mirrored interface)
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        visible: root.heading !== "" || root.description !== ""
        headingWraps: true
        text: root._plain(root.heading)
        description: root.description
    }

    Widgets.RedesignTextArea {
        id: field

        objectName: "redesignMessageField"
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        visible: root.inputMode > 0
        multiLine: root.inputMode === 2
        Accessible.name: root.heading
        onAccepted: root.accept()
    }

    // Return: the action, else (a question deleting something) Cancel
    Shortcut {
        sequences: ["Return", "Enter"]
        enabled: root.opened && root.inputMode === 0
        context: Qt.WindowShortcut
        onActivated: {
            if (root.defaultAccept)
                root.accept()
            else
                root.close()
        }
    }
}
