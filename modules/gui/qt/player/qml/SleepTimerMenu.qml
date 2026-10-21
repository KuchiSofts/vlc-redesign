/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic as Controls

import VLC.MainInterface
import VLC.Style

Controls.Popup {
    id: root
    focus: true
    property bool previousGlobalShortcuts: true
    onOpened: {
        previousGlobalShortcuts = MainCtx.useGlobalShortcuts
        MainCtx.useGlobalShortcuts = false
    }
    onClosed: MainCtx.useGlobalShortcuts = previousGlobalShortcuts
    objectName: "sleepTimerMenu"
    required property var timerController
    property int selectedAction: 0

    width: VLCStyle.dp(300, VLCStyle.scale)
    height: timerController.active ? VLCStyle.dp(190, VLCStyle.scale) : VLCStyle.dp(293, VLCStyle.scale)
    padding: VLCStyle.dp(16, VLCStyle.scale)
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside

    function formatRemaining(seconds) {
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.floor((seconds % 3600) / 60)
        const rest = seconds % 60
        const two = value => value < 10 ? "0" + value : String(value)
        return hours > 0 ? hours + ":" + two(minutes) + ":" + two(rest)
                         : two(minutes) + ":" + two(rest)
    }

    // the colours of the redesign, in the light theme too (Figma "Player - Light")
    background: Rectangle {
        color: VLCStyle.redesign.color("#1b1b1b", "#ababaa")
        radius: VLCStyle.dp(11, VLCStyle.scale)
        border.color: VLCStyle.redesign.color("#303030", "#929291")
    }

    contentItem: Column {
        spacing: VLCStyle.dp(12, VLCStyle.scale)

        Text {
            text: qsTr("Sleep timer")
            color: VLCStyle.redesign.color("#eaeaea", "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(18, VLCStyle.scale)
            font.weight: Font.DemiBold
        }

        Text {
            visible: root.timerController.active
            text: qsTr("Remaining: %1").arg(root.formatRemaining(root.timerController.remainingSeconds))
            color: VLCStyle.redesign.color("#c6c6c6", "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
        }

        Text {
            visible: !root.timerController.active
            text: qsTr("After")
            color: VLCStyle.redesign.color("#969696", "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }

        Row {
            visible: !root.timerController.active
            spacing: VLCStyle.dp(8, VLCStyle.scale)
            Controls.SpinBox {
                id: durationInput
                palette.text: VLCStyle.redesign.color("#eaeaea", "#2e2e2e")
                palette.base: VLCStyle.redesign.color("#101010", "#c2c2c1")
                palette.button: VLCStyle.redesign.color("#202020", "#9c9c9b")
                palette.buttonText: VLCStyle.redesign.color("#969696", "#4d4d4d")
                objectName: "sleepTimerMinutes"
                from: 1
                to: 720
                value: 30
                editable: true
                width: VLCStyle.dp(110, VLCStyle.scale)
                Accessible.name: qsTr("Minutes until sleep timer")
                background: Rectangle {
                    color: VLCStyle.redesign.color("#101010", "#c2c2c1")
                    border.color: VLCStyle.redesign.color("#383838", "#8e8e8d")
                    radius: VLCStyle.dp(6, VLCStyle.scale)
                }
                contentItem: TextInput {
                    z: 2
                    text: durationInput.textFromValue(durationInput.value, durationInput.locale)
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(15, VLCStyle.scale)
                    color: VLCStyle.redesign.color("#eaeaea", "#2e2e2e")
                    selectionColor: VLCStyle.redesign.accent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    readOnly: !durationInput.editable
                    validator: durationInput.validator
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                }
            }
            Text {
                height: durationInput.height
                text: qsTr("minutes")
                color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                verticalAlignment: Text.AlignVCenter
            }
        }

        Row {
            visible: !root.timerController.active
            width: parent.width
            spacing: VLCStyle.dp(8, VLCStyle.scale)
            Repeater {
                model: [qsTr("Stop playback"), qsTr("Pause playback")]
                delegate: Controls.Button {
                    required property int index
                    required property string modelData
                    objectName: index === 0 ? "sleepTimerStopAction" : "sleepTimerPauseAction"
                    width: (root.width - root.leftPadding - root.rightPadding - VLCStyle.dp(8, VLCStyle.scale)) / 2
                    height: VLCStyle.dp(34, VLCStyle.scale)
                    text: modelData
                    Accessible.name: text
                    onClicked: root.selectedAction = index
                    background: Rectangle {
                        color: root.selectedAction === index ? VLCStyle.redesign.color("#343434", "#9c9c9b")
                                                             : VLCStyle.redesign.color("#121212", "#c2c2c1")
                        border.color: root.selectedAction === index ? VLCStyle.redesign.accent
                                                                    : VLCStyle.redesign.color("#383838", "#8e8e8d")
                        radius: VLCStyle.dp(6, VLCStyle.scale)
                    }
                    contentItem: Text {
                        text: parent.text
                        color: VLCStyle.redesign.color("#c6c6c6", "#2e2e2e")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        Text {
            width: parent.width
            visible: root.timerController.error !== ""
            text: root.timerController.error
            color: VLCStyle.redesign.color("#e57373", "#c62828")
            wrapMode: Text.WordWrap
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
        }

        Controls.Button {
            objectName: root.timerController.active ? "cancelSleepTimer" : "startSleepTimer"
            width: parent.width
            height: VLCStyle.dp(34, VLCStyle.scale)
            text: root.timerController.active ? qsTr("Cancel timer") : qsTr("Start timer")
            Accessible.name: text
            onClicked: {
                if (root.timerController.active)
                    root.timerController.cancel()
                else
                    root.timerController.start(durationInput.value * 60, root.selectedAction)
            }
            background: Rectangle {
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: VLCStyle.redesign.accent
            }
            contentItem: Text {
                text: parent.text
                color: "#161616"
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(15, VLCStyle.scale)
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
