/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later. */
import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Controls.impl as Impl
import VLC.Dialogs
import VLC.MainInterface
import VLC.Style
Controls.Popup {
    id: root
    objectName: "hdrOutputDialog"
    property bool previousShortcuts: true
    width: Math.min(VLCStyle.dp(400, VLCStyle.scale), parent.width - 32)
    height: Math.min(VLCStyle.dp(445, VLCStyle.scale), parent.height - 48)
    padding: VLCStyle.dp(20, VLCStyle.scale)
    focus: true
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
    // a press outside only closes it: the video under it does not pause
    modal: true
    dim: false
    onOpened: { previousShortcuts = MainCtx.useGlobalShortcuts; MainCtx.useGlobalShortcuts = false }
    onClosed: MainCtx.useGlobalShortcuts = previousShortcuts
    HdrOutput { id: output; ctx: MainCtx }
    background: Rectangle { color: VLCStyle.redesign.color("#191919", "#c2c2c1"); radius: 6 }
    contentItem: Column {
        spacing: 8
        Text {
            text: output.selectedIndex >= 0 ? output.choices[output.selectedIndex].label : qsTr("HDR unavailable")
            height: VLCStyle.dp(40, VLCStyle.scale)
            color: VLCStyle.redesign.color("#969696", "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily; font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
        Repeater {
            model: output.choices
            // the reason of a choice that cannot be chosen shows on hover: a
            // disabled control gets no hover itself, the row around it does
            delegate: Item {
                id: choice
                required property int index
                required property var modelData
                width: root.availableWidth
                height: VLCStyle.dp(46, VLCStyle.scale)
                HoverHandler { id: choiceHover }
                Controls.ToolTip.visible: choiceHover.hovered && modelData.reason.length > 0
                Controls.ToolTip.text: modelData.reason
                Controls.ToolTip.delay: 300
                Controls.ItemDelegate {
                    id: row
                    anchors.fill: parent
                    enabled: choice.modelData.enabled
                    onClicked: output.select(choice.index)
                    background: Rectangle {
                        radius: 4
                        color: row.hovered || output.selectedIndex === choice.index ? VLCStyle.redesign.color("#222222", "#ababaa") : "transparent"
                        border.color: row.visualFocus ? VLCStyle.redesign.accent : "transparent"
                    }
                    contentItem: Row {
                        Text {
                            width: row.width - 54; height: parent.height
                            text: choice.modelData.label; opacity: row.enabled ? 1 : 0.45
                            color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                            font.family: VLCStyle.redesign.fontFamily; font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                            verticalAlignment: Text.AlignVCenter
                        }
                        Impl.ColorImage { width: 24; height: 24; anchors.verticalCenter: parent.verticalCenter
                            source: "qrc:///redesign/track-check-active.png"; visible: output.selectedIndex === choice.index
                            color: VLCStyle.redesign.accentTint }
                    }
                }
            }
        }
        Text {
            width: parent.width
            text: output.message
            wrapMode: Text.WordWrap
            color: VLCStyle.redesign.color("#969696", "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily; font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
        }
    }
}
