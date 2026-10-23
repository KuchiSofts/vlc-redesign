/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later. */
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Controls
import QtQuick.Controls.impl as Impl
import VLC.MainInterface
import VLC.Player
import VLC.Style
Controls.Popup {
    id: root
    objectName: "audioVisualizationDialog"
    property string selectedText: qsTr("Disabled")
    property bool previousShortcuts: true
    width: Math.min(VLCStyle.dp(400, VLCStyle.scale), parent.width - 32)
    height: Math.min(VLCStyle.dp(400, VLCStyle.scale), parent.height - 48)
    padding: VLCStyle.dp(20, VLCStyle.scale)
    focus: true
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
    // a press outside only closes it: the video under it does not pause
    modal: true
    dim: false
    onOpened: { previousShortcuts = MainCtx.useGlobalShortcuts; MainCtx.useGlobalShortcuts = false }
    onClosed: MainCtx.useGlobalShortcuts = previousShortcuts
    background: Rectangle { color: VLCStyle.redesign.color("#191919", "#c2c2c1"); radius: 6 }
    contentItem: Column {
        spacing: VLCStyle.dp(8, VLCStyle.scale)
        Rectangle {
            width: parent.width
            height: VLCStyle.dp(46, VLCStyle.scale)
            color: VLCStyle.redesign.color("#222222", "#ababaa")
            radius: 4
            Text {
                anchors.left: parent.left; anchors.leftMargin: 16; anchors.verticalCenter: parent.verticalCenter
                text: root.selectedText
                color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                font.family: VLCStyle.redesign.fontFamily; font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            }
            Image {
                anchors.right: parent.right; anchors.rightMargin: 16; anchors.verticalCenter: parent.verticalCenter
                source: "qrc:///redesign/icons/chevron-down/default.png"; width: 24; height: 24
            }
        }
        ListView {
            id: options
            objectName: "audioVisualizationModes"
            width: parent.width
            height: root.availableHeight - VLCStyle.dp(54, VLCStyle.scale)
            clip: true
            model: Player.audioVisualization
            Controls.ScrollBar.vertical: Controls.ScrollBar {
                // Basic's handle is light grey: darker in the light theme
                palette.mid: VLCStyle.palette.isDark ? undefined : "#6b6b6b"
                palette.dark: VLCStyle.palette.isDark ? undefined : "#2e2e2e"
            }
            delegate: Controls.ItemDelegate {
                id: row
                required property int index
                required property var model
                checked: model.checked
                width: options.width
                height: VLCStyle.dp(46, VLCStyle.scale)
                text: index === 0 ? qsTr("Disabled") : model.display
                Component.onCompleted: if (checked) root.selectedText = text
                onCheckedChanged: if (checked) root.selectedText = text
                onClicked: Player.audioVisualization.toggleIndex(index)
                background: Rectangle {
                    color: row.hovered || row.checked ? VLCStyle.redesign.color("#222222", "#ababaa") : "transparent"
                    radius: 4
                    border.color: row.visualFocus ? VLCStyle.redesign.accent : "transparent"
                }
                contentItem: RowLayout {
                    spacing: 10
                    Image {
                        id: modeIcon
                        Layout.preferredWidth: row.index === 0 ? 0 : 24; Layout.preferredHeight: 24
                        // display can be undefined while the model is rebuilt
                        readonly property string mode: String(row.model.display ?? "").toLowerCase()
                        readonly property string iconDir: "qrc:///redesign/icons/" +
                            (mode.includes("spectrometer") ? "spectrometer" :
                             mode.includes("spectrum") ? "spectrum" :
                             mode.includes("scope") ? "scope" : "waveform")
                        // drawn in grey and the accent dim: with another
                        // accent, the grey part and the rest tinted above it
                        readonly property bool twoTone: row.index !== 0 && !VLCStyle.redesign.defaultAccent
                        source: row.index === 0 ? "" : iconDir + (twoTone ? "/default-grey.png" : "/default.png")
                        Impl.ColorImage {
                            anchors.fill: parent
                            visible: modeIcon.twoTone
                            source: visible ? modeIcon.iconDir + "/default-accent.png" : ""
                            color: VLCStyle.redesign.accentDim
                        }
                    }
                    Text {
                        Layout.fillWidth: true; verticalAlignment: Text.AlignVCenter
                        text: row.text; color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                        font.family: VLCStyle.redesign.fontFamily; font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    }
                    Impl.ColorImage { Layout.preferredWidth: 24; Layout.preferredHeight: 24
                        source: "qrc:///redesign/track-check-active.png"; visible: row.checked
                        color: VLCStyle.redesign.accentTint }
                }
            }
            Text {
                anchors.fill: parent
                visible: options.count === 0
                text: qsTr("Play audio to select a visualization.")
                color: VLCStyle.redesign.color(VLCStyle.redesign.textSecondary, "#4d4d4d")
                wrapMode: Text.WordWrap
                font.family: VLCStyle.redesign.fontFamily; font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            }
        }
    }
}
