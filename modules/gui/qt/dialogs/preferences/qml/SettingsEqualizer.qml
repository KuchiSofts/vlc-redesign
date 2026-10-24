/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import VLC.Style
import VLC.Widgets as Widgets

ColumnLayout {
    id: root
    required property var effects
    property bool expanded: true
    spacing: VLCStyle.dp(12, VLCStyle.scale)
    visible: effects.filterAvailable("equalizer")

    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTr("Equalizer")
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            color: VLCStyle.redesign.color("#8F8F8F", "#7c7c7b")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
        SettingsToggle {
            id: enabledSwitch
            objectName: "settingsEqualizerEnabled"
            Accessible.name: qsTr("Equalizer")
            checked: { root.effects.bands; return root.effects.filterEnabled("equalizer") }
            onSwitched: (on) => root.effects.setFilterEnabled("equalizer", on)
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTr("Equalizer preset")
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            color: VLCStyle.redesign.color("#8F8F8F", "#7c7c7b")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
        ComboBox {
            id: presetBox
            objectName: "settingsEqualizerPreset"
            Layout.preferredWidth: VLCStyle.dp(319, VLCStyle.scale)
            model: root.effects.presets
            textRole: "name"
            valueRole: "id"
            currentIndex: {
                const index = root.effects.presets.findIndex(p => p.id === root.effects.preset)
                return index
            }
            onActivated: (index) => root.effects.setPreset(root.effects.presets[index].id)
            contentItem: Label {
                // 12 px from the start (the right, right to left)
                leftPadding: presetBox.mirrored ? 0 : 12
                rightPadding: presetBox.mirrored ? 12 : 0
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
                text: presetBox.displayText || qsTr("Custom")
                color: VLCStyle.redesign.color("#EAEAEA", "#2e2e2e")
                verticalAlignment: Text.AlignVCenter
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            }
            background: Rectangle { color: VLCStyle.redesign.color("#191919", "#ababaa"); radius: 4 }
            delegate: ItemDelegate {
                width: presetBox.width
                text: modelData.name
                palette.text: VLCStyle.redesign.color("#EAEAEA", "#2e2e2e")
                background: Rectangle { color: highlighted ? VLCStyle.redesign.color("#222222", "#9c9c9b")
                                                      : VLCStyle.redesign.color("#191919", "#ababaa") }
            }
            popup: Popup {
                // a press outside only closes it (on the box too: it doesn't open
                // again), and goes no further, as in a native list
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                modal: true
                dim: false
                y: presetBox.height
                width: presetBox.width
                implicitHeight: Math.min(contentItem.implicitHeight, VLCStyle.dp(280, VLCStyle.scale))
                padding: 0
                contentItem: Item {
                    implicitWidth: popupList.implicitWidth
                    implicitHeight: popupList.implicitHeight

                    ListView {
                        id: popupList
                        anchors.fill: parent
                        // a popup: right to left as its box
                        LayoutMirroring.enabled: presetBox.mirrored
                        LayoutMirroring.childrenInherit: true
                        clip: true; implicitHeight: contentHeight; model: presetBox.popup.visible ? presetBox.delegateModel : null; currentIndex: presetBox.highlightedIndex
                    }

                    // the wheel moves it by whole rows
                    Widgets.RowSnapWheel {
                        anchors.fill: popupList
                        view: popupList
                    }
                }
                background: Rectangle { color: VLCStyle.redesign.color("#191919", "#ababaa"); border.color: VLCStyle.redesign.color("#373737", "#8e8e8d") }
            }
        }
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: root.expanded ? VLCStyle.dp(480, VLCStyle.scale) : VLCStyle.dp(42, VLCStyle.scale)
        color: VLCStyle.redesign.color("#1b1b1b", VLCStyle.palette.lightGrey200)
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: VLCStyle.dp(16, VLCStyle.scale)
            spacing: VLCStyle.dp(12, VLCStyle.scale)
            Button {
                Layout.alignment: Qt.AlignHCenter
                text: root.expanded ? qsTr("Close Equalizer") : qsTr("Open Equalizer")
                onClicked: root.expanded = !root.expanded
                background: Item {}
                contentItem: Label { text: parent.text; color: VLCStyle.redesign.color("#8F8F8F", "#7c7c7b"); font.pixelSize: VLCStyle.dp(14, VLCStyle.scale) }
            }
            RowLayout {
                id: bandsRow
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.expanded
                spacing: VLCStyle.dp(5, VLCStyle.scale)
                // the room of a band: its names are cut short to it
                readonly property real bandWidth: (width - 10 * spacing) / 11
                Repeater {
                    model: 11
                    ColumnLayout {
                        id: bandColumn
                        required property int index
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: VLCStyle.dp(6, VLCStyle.scale)
                        readonly property real currentBand: index === 0 ? root.effects.preamp
                                                             : Number(root.effects.bands[index - 1] || 0)
                        Slider {
                            id: bandSlider
                            objectName: index === 0 ? "settingsEqualizerPreamp" : "settingsEqualizerBand" + (index - 1)
                            Layout.alignment: Qt.AlignHCenter
                            Layout.fillHeight: true
                            orientation: Qt.Vertical
                            from: -20
                            to: 20
                            Binding on value { when: !bandSlider.pressed; value: bandColumn.currentBand }
                            onMoved: {
                                if (index === 0) root.effects.setPreamp(value)
                                else root.effects.setBand(index - 1, value)
                            }
                            background: Rectangle {
                                x: (bandSlider.width - width) / 2
                                width: VLCStyle.dp(14, VLCStyle.scale)
                                height: bandSlider.height
                                radius: width / 2
                                color: "#0e0e0e"
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: parent.width
                                    height: bandSlider.visualPosition * parent.height
                                    color: "#54300c"
                                    radius: parent.radius
                                }
                            }
                            handle: Rectangle {
                                x: (bandSlider.width - width) / 2
                                y: bandSlider.topPadding + bandSlider.visualPosition * (bandSlider.availableHeight - height)
                                width: VLCStyle.dp(22, VLCStyle.scale)
                                height: VLCStyle.dp(7, VLCStyle.scale)
                                radius: 4
                                color: VLCStyle.redesign.accent
                            }
                        }
                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.maximumWidth: bandsRow.bandWidth
                            elide: Text.ElideRight
                            text: index === 0 ? qsTr("Preamp")
                                  : Math.round(Number(root.effects.frequencies[index - 1] || 0)) + qsTr(" Hz")
                            color: VLCStyle.redesign.color("#8F8F8F", "#7c7c7b")
                            font.pixelSize: VLCStyle.dp(10, VLCStyle.scale)
                        }
                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: Number(currentBand).toFixed(1) + qsTr(" dB")
                            color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                            font.pixelSize: VLCStyle.dp(10, VLCStyle.scale)
                        }
                    }
                }
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Label { Layout.fillWidth: true; elide: Text.ElideRight; horizontalAlignment: Text.AlignLeft; text: qsTr("Two-pass"); color: VLCStyle.redesign.color("#8F8F8F", "#7c7c7b"); font.pixelSize: VLCStyle.dp(14, VLCStyle.scale) }
        SettingsToggle {
            id: twoPassSwitch
            Accessible.name: qsTr("Two-pass")
            checked: root.effects.twoPass
            onSwitched: (on) => root.effects.setTwoPass(on)
        }
    }
}
