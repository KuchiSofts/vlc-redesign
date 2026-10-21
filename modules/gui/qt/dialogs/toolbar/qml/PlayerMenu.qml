/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later. */
import QtQuick
import QtQuick.Controls.Basic as Controls
import VLC.Dialogs
import VLC.MainInterface
import VLC.Style
import VLC.Player

Controls.Popup {
    id: root
    focus: true
    property bool previousGlobalShortcuts: true
    onOpened: {
        previousGlobalShortcuts = MainCtx.useGlobalShortcuts
        MainCtx.useGlobalShortcuts = false
    }
    onClosed: MainCtx.useGlobalShortcuts = previousGlobalShortcuts
    width: 344
    padding: 12
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
    // the colours of the redesign, in the light theme too (Figma "Player - Light")
    background: Rectangle {
        color: VLCStyle.redesign.color("#1b1b1b", "#ababaa")
        radius: 11
        border.color: VLCStyle.redesign.color("#252525", "#9c9c9b")
    }
    contentItem: Column {
        spacing: 4
        PersonalizationActions {
            width: parent.width
            onCustomizeRequested: {
                root.close()
                DialogsProvider.showToolbarEditorDialog()
            }
        }
        HideUIAction { width: parent.width; onClicked: root.close() }
        Repeater {
            model: [qsTr("Sleep timer"), qsTr("Preferences")]
            delegate: Controls.ItemDelegate {
                required property int index
                required property string modelData
                width: root.availableWidth
                height: 48
                background: Rectangle {
                    color: parent.hovered ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
                    radius: 6
                }
                contentItem: Text {
                    text: modelData
                    color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: 16
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 12
                }
                onClicked: {
                    root.close()
                    if (index === 0) DialogsProvider.sleepTimerDialog()
                    // the menu of the player: its settings
                    else DialogsProvider.prefsDialogSection("playback")
                }
            }
        }
    }
}
