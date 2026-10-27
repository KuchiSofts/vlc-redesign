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
    width: VLCStyle.dp(344, VLCStyle.scale)
    padding: VLCStyle.dp(12, VLCStyle.scale)
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
    // the colours of the redesign, in the light theme too (Figma "Player - Light")
    background: Rectangle {
        color: VLCStyle.redesign.color("#1b1b1b", "#ababaa")
        radius: VLCStyle.dp(11, VLCStyle.scale)
        border.color: VLCStyle.redesign.color("#252525", "#9c9c9b")
    }
    contentItem: Column {
        spacing: VLCStyle.dp(4, VLCStyle.scale)
        // a popup does not inherit the mirroring of the window (right to left)
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
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
                height: VLCStyle.dp(48, VLCStyle.scale)
                background: Rectangle {
                    color: parent.hovered ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
                    radius: VLCStyle.dp(6, VLCStyle.scale)
                }
                contentItem: Text {
                    text: modelData
                    color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    leftPadding: LayoutMirroring.enabled ? 0 : VLCStyle.dp(12, VLCStyle.scale)
                    rightPadding: LayoutMirroring.enabled ? VLCStyle.dp(12, VLCStyle.scale) : 0
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
