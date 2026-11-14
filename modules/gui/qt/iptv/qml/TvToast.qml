/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick

import VLC.MainInterface
import VLC.Style
import VLC.Widgets as Widgets

// What Live TV has to say for a moment (no archive, no such channel), the
// reminders of the guide, with Watch, and what the automatic guide found,
// with Undo: a card of the menus (#191919 at 95 %, radius 10) at the top of
// the view
Rectangle {
    id: root

    property bool forceDark: false
    readonly property bool _dark: forceDark || VLCStyle.palette.isDark
    property string text
    property var reminder: null
    // the source whose automatic guide Undo removes ("": none)
    property string undoSource: ""

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }

    function say(message) {
        reminder = null
        undoSource = ""
        text = message
        hideTimer.interval = 4000
        hideTimer.restart()
    }

    visible: opacity > 0
    opacity: hideTimer.running ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: VLCStyle.duration_short } }
    width: Math.min(row.implicitWidth + px(32), parent ? parent.width - px(40) : row.implicitWidth)
    height: px(48)
    radius: px(10)
    color: _dark ? Qt.rgba(25 / 255, 25 / 255, 25 / 255, 0.95) : VLCStyle.redesign.color("#191919", undefined, false)

    Connections {
        target: Iptv
        function onMessage(text) { root.say(text) }
        function onGuideFound(result) {
            root.reminder = null
            root.undoSource = result.found > result.before ? result.id : ""
            root.text = result.found > result.before
                        ? qsTr("Guide found for %1 of %2 channels").arg(result.found).arg(result.total)
                        : result.error ? qsTr("The list of channels to match could not be downloaded")
                        : result.before > 0 ? qsTr("Guide for %1 of %2 channels").arg(result.before).arg(result.total)
                        : qsTr("No free guide was found for the channels of %1").arg(result.name)
            hideTimer.interval = result.found > result.before ? 15000 : 6000
            hideTimer.restart()
        }
        function onReminderDue(reminder) {
            root.undoSource = ""
            root.reminder = reminder
            root.text = qsTr("%1 starts now on %2").arg(reminder.title).arg(reminder.channel)
            hideTimer.interval = 30000
            hideTimer.restart()
        }
    }

    Timer { id: hideTimer }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: root.px(16)

        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, root.parent ? root.parent.width - root.px(200) : implicitWidth)
            textFormat: Text.PlainText
            text: root.text
            elide: Text.ElideRight
            color: VLCStyle.redesign.color("#C6C6C6", undefined, root._dark)
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
        }
        Widgets.RedesignButton {
            visible: root.reminder !== null
            anchors.verticalCenter: parent.verticalCenter
            darkTheme: root._dark
            accent: true
            text: qsTr("Watch")
            onClicked: {
                Iptv.play(root.reminder.key)
                hideTimer.stop()
            }
        }
        Widgets.RedesignButton {
            objectName: "tvGuideUndo"
            visible: root.undoSource !== ""
            anchors.verticalCenter: parent.verticalCenter
            darkTheme: root._dark
            text: qsTr("Undo")
            onClicked: {
                Iptv.removeAutoGuide(root.undoSource)
                root.undoSource = ""
                hideTimer.stop()
            }
        }
    }
}
