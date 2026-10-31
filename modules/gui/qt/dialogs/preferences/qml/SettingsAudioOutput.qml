/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import VLC.Dialogs
import VLC.MainInterface
import VLC.Player
import VLC.Style
import VLC.Widgets as Widgets

ColumnLayout {
    id: root
    property int selectedIndex: -1
    readonly property bool dirty: preview.dirty
    readonly property string error: preview.lastError
    spacing: VLCStyle.dp(5, VLCStyle.scale)

    function begin() {
        if (!preview.active && preview.begin()) selectedIndex = preview.currentIndex()
    }
    function canApply() { return preview.canCommit() }
    function apply() { return preview.commit() }
    function cancel() { return preview.cancel() }
    function reset() { if (preview.cancel()) begin() }

    AudioOutputPreview { id: preview; ctx: MainCtx }
    Repeater {
        id: deviceRows
        model: Player.audioDevices
        onCountChanged: {
            if (count > 0 && root.visible && !preview.active) root.begin()
            if (count === 0) root.selectedIndex = -1
        }
        Item {
            required property int index
            required property bool checked
            visible: false
            onCheckedChanged: { if (checked) root.selectedIndex = index }
            Component.onCompleted: { if (checked) root.selectedIndex = index }
        }
    }
    Connections {
        target: Player.audioDevices
        function onModelReset() { root.selectedIndex = -1 }
    }
    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTr("Current output device")
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            color: VLCStyle.redesign.color("#8F8F8F", "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
        ComboBox {
            id: deviceBox
            objectName: "settingsOutputDevice"
            // as the other dropdowns of Settings (SettingsDropdown)
            Layout.preferredWidth: VLCStyle.dp(320, VLCStyle.scale)
            Layout.preferredHeight: VLCStyle.dp(36, VLCStyle.scale)
            leftPadding: mirrored ? VLCStyle.dp(16 + 24 + 10, VLCStyle.scale) : VLCStyle.dp(4, VLCStyle.scale)
            rightPadding: mirrored ? VLCStyle.dp(4, VLCStyle.scale) : VLCStyle.dp(16 + 24 + 10, VLCStyle.scale)
            enabled: preview.active && deviceRows.count > 0
            model: Player.audioDevices
            textRole: "display"
            Binding { target: deviceBox; property: "currentIndex"; value: root.selectedIndex; when: !deviceBox.popup.visible }
            onActivated: (index) => {
                if (preview.preview(index, textAt(index))) root.selectedIndex = index
            }
            contentItem: Label {
                // 12 px from the start (the right, right to left)
                leftPadding: deviceBox.mirrored ? 0 : VLCStyle.dp(12, VLCStyle.scale)
                rightPadding: deviceBox.mirrored ? VLCStyle.dp(12, VLCStyle.scale) : 0
                horizontalAlignment: Text.AlignLeft
                text: deviceRows.count === 0 ? qsTr("No selectable device available")
                      : root.selectedIndex < 0 ? qsTr("System default") : deviceBox.displayText
                color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            }
            background: Rectangle {
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: VLCStyle.redesign.color("#090909")
                border.width: deviceBox.visualFocus ? 1 : 0
                border.color: VLCStyle.redesign.accent

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: VLCStyle.dp(2, VLCStyle.scale)
                    radius: VLCStyle.dp(4, VLCStyle.scale)
                    color: deviceBox.hovered && !deviceBox.popup.visible ? VLCStyle.redesign.color("#222222")
                                                                         : VLCStyle.redesign.color("#191919")
                }
            }
            indicator: Image {
                x: deviceBox.mirrored ? VLCStyle.dp(2 + 14, VLCStyle.scale)
                                      : deviceBox.width - VLCStyle.dp(2 + 14, VLCStyle.scale) - width
                y: (deviceBox.height - height) / 2
                width: VLCStyle.dp(24, VLCStyle.scale)
                height: width
                sourceSize: Qt.size(width, height)
                source: "qrc:///redesign/icons/chevron-down/default.svg"
                opacity: deviceBox.enabled ? 1 : 0.4
            }
            delegate: ItemDelegate {
                required property var model
                width: deviceBox.width
                text: model.display
                palette.text: VLCStyle.redesign.color("#EAEAEA", "#2e2e2e")
                background: Rectangle {
                    color: highlighted ? VLCStyle.redesign.color("#222222", "#9c9c9b")
                                       : VLCStyle.redesign.color("#191919", "#ababaa")
                }
            }
            popup: Popup {
                // a press outside only closes it (on the box too: it doesn't open
                // again), and goes no further, as in a native list
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                modal: true
                dim: false
                y: deviceBox.height
                width: deviceBox.width
                implicitHeight: Math.min(contentItem.implicitHeight, VLCStyle.dp(280, VLCStyle.scale))
                padding: 0
                contentItem: Item {
                    implicitWidth: popupList.implicitWidth
                    implicitHeight: popupList.implicitHeight

                    ListView {
                        id: popupList
                        anchors.fill: parent
                        // a popup: right to left as its box
                        LayoutMirroring.enabled: deviceBox.mirrored
                        LayoutMirroring.childrenInherit: true
                        clip: true
                        implicitHeight: contentHeight
                        model: deviceBox.popup.visible ? deviceBox.delegateModel : null
                        currentIndex: deviceBox.highlightedIndex
                    }

                    // the wheel moves it by whole rows
                    Widgets.RowSnapWheel {
                        anchors.fill: popupList
                        view: popupList
                    }
                }
                background: Rectangle {
                    color: VLCStyle.redesign.color("#191919", "#ababaa")
                    // the frame of the main window
                    border.color: VLCStyle.redesign.color(VLCStyle.redesign.windowBorder)
                }
            }
        }
    }
    Label {
        Layout.fillWidth: true
        text: qsTr("Device changes affect only the current audio output. Apply keeps this selection while it remains active.")
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignLeft
        color: VLCStyle.redesign.color("#8F8F8F", "#2e2e2e")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
    }
}
