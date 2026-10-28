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
import VLC.Style
import VLC.Widgets as Widgets

// A programme of the guide (the dialogs of the design, as Folder
// information): its title and episode, the channel and the time, its
// category and description; Watch it (on now, or from the archive),
// from its start, or a reminder for one to come
Widgets.RedesignDialog {
    id: root

    objectName: "tvProgrammeDialog"

    property var channel: ({})
    property var programme: ({})
    property string _description: ""

    readonly property bool _onNow: (programme.start ?? 0) <= Iptv.now && Iptv.now < (programme.stop ?? 0)
    readonly property bool _past: (programme.stop ?? 0) <= Iptv.now
    readonly property bool _future: (programme.start ?? 0) > Iptv.now
    readonly property bool _archived: (channel.catchupDays ?? 0) > 0
                                      && (programme.start ?? 0) < Iptv.now
                                      && Iptv.now - (programme.start ?? 0) <= (channel.catchupDays ?? 0) * 86400
    readonly property bool _reminded: Iptv.reminders.length >= 0 && Iptv.hasReminder(channel.key ?? "", programme.start ?? 0)

    function show(ch, p) {
        channel = ch
        programme = p
        _description = ""
        Iptv.description(ch.key, p.start, (text) => { if (root.programme === p) root._description = text })
        open()
    }

    title: qsTr("Programme")
    preferredWidth: VLCStyle.dp(713, VLCStyle.scale)
    cancelText: qsTr("Close")
    acceptText: _onNow ? qsTr("Watch")
               : _past ? qsTr("Watch")
               : _reminded ? qsTr("Remove Reminder") : qsTr("Remind Me")
    // a past programme without archive: nothing to do but close
    acceptVisible: !_past || _archived
    extraText: _onNow && _archived ? qsTr("Watch from the Start") : ""
    onExtraClicked: {
        Iptv.playCatchup(channel.key, programme.start, programme.stop, programme.title)
        close()
    }
    onAccepted: {
        if (_onNow)
            Iptv.play(channel.key)
        else if (_past)
            Iptv.playCatchup(channel.key, programme.start, programme.stop, programme.title)
        else if (_reminded)
            Iptv.removeReminder(channel.key, programme.start)
        else
            Iptv.addReminder(channel.key, programme.start, programme.title)
    }

    function _plain(text) {
        return (text || "").replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    }

    Widgets.RedesignDialogHeading {
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        headingWraps: true
        text: root._plain(root.programme.title)
        suffix: root.programme.episode ? root._plain(root.programme.episode) : ""
        description: root._past && !root._archived
                     ? qsTr("This programme is over and the channel keeps no archive.")
                     : (root.programme.subtitle ?? "")
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Widgets.RedesignInfoRow {
            label: qsTr("Channel")
            value: ((root.channel.number ?? 0) > 0 ? root.channel.number + "  " : "") + (root.channel.name ?? "")
        }
        Widgets.RedesignInfoRow {
            label: qsTr("Time")
            value: Iptv.dayText(root.programme.start ?? 0) + ", " + Iptv.timeText(root.programme.start ?? 0)
                   + " – " + Iptv.timeText(root.programme.stop ?? 0)
                   + "  (" + Iptv.durationText((root.programme.stop ?? 0) - (root.programme.start ?? 0)) + ")"
        }
        Widgets.RedesignInfoRow {
            visible: root._onNow
            label: qsTr("Left")
            value: Iptv.durationText((root.programme.stop ?? 0) - Iptv.now)
        }
        Widgets.RedesignInfoRow {
            visible: !!root.programme.category
            label: qsTr("Category")
            value: root.programme.category ?? ""
        }
        Widgets.RedesignInfoRow {
            label: qsTr("Catch-up")
            divider: root._description !== ""
            value: (root.channel.catchupDays ?? 0) > 0
                   ? qsTr("The past %1 day", "The past %1 days", root.channel.catchupDays).arg(root.channel.catchupDays)
                   : qsTr("None")
        }
    }

    Text {
        Layout.fillWidth: true
        visible: root._description !== ""
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        text: root._description
        wrapMode: Text.WordWrap
        maximumLineCount: 12
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
        lineHeight: 1.35
        color: VLCStyle.redesign.color("#8F8F8F", "#2e2e2e")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
    }
}
