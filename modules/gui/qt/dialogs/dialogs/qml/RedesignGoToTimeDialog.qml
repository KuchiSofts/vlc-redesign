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
import VLC.Player
import VLC.Style
import VLC.Widgets as Widgets

// Go to Time (Ctrl+T, the menus): the time typed, as the time of the
// control bar takes it ("1:02:10", "62:10", "130")
Widgets.RedesignDialog {
    id: root

    objectName: "goToTimeDialog"

    title: qsTr("Go to Time")
    acceptText: qsTr("Go")
    // Start: back to the beginning
    extraText: qsTr("Start")
    preferredWidth: VLCStyle.dp(560, VLCStyle.scale)
    acceptEnabled: Player.isStarted && Player.seekable && seconds >= 0

    // the time typed (-1: not a time)
    readonly property real seconds: {
        const text = field.text.trim()
        if (text === "")
            return -1
        const parts = text.split(":").map((part) => part.trim())
        if (parts.length > 3 || parts.some((part) => !/^[0-9]+([.][0-9]+)?$/.test(part)))
            return -1
        const total = parts.map(Number).reduce((sum, n) => sum * 60 + n, 0)
        const length = Player.length.valid() ? Player.length.toSeconds() : -1
        return length > 0 ? Math.min(total, length) : total
    }

    // the time of the film, in the format of the field
    function _current() {
        const total = Player.time.valid() ? Player.time.toSeconds() : 0
        const hours = Math.floor(total / 3600)
        const minutes = Math.floor(total / 60) % 60
        const secs = total % 60
        const pad = (n) => (n < 10 ? "0" : "") + n
        return hours > 0 ? hours + ":" + pad(minutes) + ":" + pad(secs)
                         : minutes + ":" + pad(secs)
    }

    onAboutToShow: field.text = _current()
    onOpened: {
        field.forceActiveFocus()
        field.selectAll()
    }
    onAccepted: DialogsProvider.goToTime(root.seconds)
    onExtraClicked: {
        DialogsProvider.goToTime(0)
        close()
    }

    Widgets.RedesignDialogHeading {
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        headingWraps: true
        text: qsTr("Go to a time of the media")
        description: !Player.isStarted ? qsTr("Nothing is playing.")
                     : !Player.seekable ? qsTr("This media cannot be moved in.")
                     : Player.length.valid() && Player.length.toSeconds() > 0
                       ? qsTr("Hours, minutes and seconds, as 1:02:10, 62:10 or 130. The media lasts %1.")
                             .arg(Player.length.formatHMS())
                       : qsTr("Hours, minutes and seconds, as 1:02:10, 62:10 or 130.")
    }

    Widgets.RedesignTextArea {
        id: field

        objectName: "goToTimeField"
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        placeholderText: qsTr("e.g., 1:02:10")
        Accessible.name: qsTr("Time")
        onAccepted: root.accept()
    }

    // Track the visibility for the menus (a check mark)
    onVisibleChanged: DialogsProvider.setRedesignDialogVisible("gotoTimeDialog", visible)
}
