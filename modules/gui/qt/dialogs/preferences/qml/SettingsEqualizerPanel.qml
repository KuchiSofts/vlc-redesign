/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import VLC.Style

// Equalizer of the design (Figma "EQ"): closed, the 36 px "Open Equalizer"
// bar (#090909, radius 2); open, a #191919 panel (radius 2, 6 px above, 20
// below, 24 between) with "Close Equalizer" and 11 columns of 60 px, 8 px
// apart: a vertical range (16 x 320, the fill from the bottom, the 24 x 8
// thumb), 33 px below it the name (14 px) and, 13 px lower, the value in a
// 60 x 32 box (#090909, radius 6, 12 px).
Item {
    id: root

    // The 10 bands (dB, "" for those of the preset) and the preamp
    property var bands: []
    property real preamp: 12
    property bool expanded: false
    // The band clicked last (-1: none): it takes the wheel while the
    // pointer stays on it
    property int wheelBand: -1

    signal bandsEdited(var bands)
    signal preampEdited(real preamp)

    readonly property var names: [qsTr("Preamp"), qsTr("60 Hz"), qsTr("170 Hz"), qsTr("310 Hz"),
                                  qsTr("600 Hz"), qsTr("1 kHz"), qsTr("3 kHz"), qsTr("6 kHz"),
                                  qsTr("12 kHz"), qsTr("14 kHz"), qsTr("16 kHz")]

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, value) }
    function valueAt(column) { return column === 0 ? preamp : Number(bands[column - 1] || 0) }

    implicitHeight: expanded ? px(489) : px(36)

    // "Open Equalizer" / "Close Equalizer" with the chevron, centred
    component Header: Item {
        property alias text: headerText.text
        Row {
            anchors.centerIn: parent
            spacing: root.px(10)
            Text {
                id: headerText
                anchors.baseline: parent.verticalCenter
                anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
                color: root.c("#8F8F8F")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
            }
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: root.px(24)
                height: width
                sourceSize: Qt.size(width, height)
                source: "qrc:///redesign/icons/chevron-down/default.png"
            }
        }
        TapHandler { onTapped: root.expanded = !root.expanded }
        HoverHandler { cursorShape: Qt.PointingHandCursor }
    }

    Rectangle {
        visible: !root.expanded
        width: parent.width
        height: root.px(36)
        radius: root.px(2)
        color: root.c("#090909")
        Header {
            objectName: "settingsOpenEqualizer"
            anchors.fill: parent
            text: qsTr("Open Equalizer")
            Accessible.role: Accessible.Button
            Accessible.name: text
        }
    }

    Rectangle {
        visible: root.expanded
        width: parent.width
        height: root.px(489)
        radius: root.px(2)
        color: root.c("#191919")

        Header {
            objectName: "settingsCloseEqualizer"
            y: root.px(6)
            width: parent.width
            height: root.px(24)
            text: qsTr("Close Equalizer")
            Accessible.role: Accessible.Button
            Accessible.name: text
        }

        Row {
            y: root.px(6 + 24 + 24)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root.px(8)

            Repeater {
                model: root.names.length
                Item {
                    id: column
                    required property int index
                    readonly property real value: root.valueAt(index)
                    width: root.px(60)
                    height: root.px(415)

                    // -20 dB at the bottom, +20 dB at the top
                    Rectangle {
                        id: track
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: root.px(16)
                        height: root.px(320)
                        radius: root.px(6)
                        color: root.c("#090909")
                        readonly property real usable: height - root.px(4)
                        readonly property real fraction: (Math.max(-20, Math.min(20, column.value)) + 20) / 40

                        Rectangle {
                            x: root.px(2)
                            width: root.px(12)
                            height: track.fraction * track.usable
                            y: track.height - root.px(2) - height
                            radius: root.px(4)
                            color: root.c(VLCStyle.redesign.accentBackground)
                        }
                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: root.px(24)
                            height: root.px(8)
                            radius: root.px(4)
                            y: Math.max(0, Math.min(track.height - height,
                                                    track.height - root.px(2) - track.fraction * track.usable - height / 2))
                            color: VLCStyle.redesign.accentDim
                        }

                        function set(y) {
                            const fraction = 1 - Math.max(0, Math.min(1, (y - root.px(2)) / usable))
                            const dB = Math.round((fraction * 40 - 20) * 10) / 10
                            if (column.index === 0) {
                                root.preampEdited(dB)
                            } else {
                                const bands = []
                                for (let i = 0; i < 10; ++i)
                                    bands.push(i === column.index - 1 ? dB : Number(root.bands[i] || 0))
                                root.bandsEdited(bands)
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            anchors.leftMargin: -root.px(22)
                            anchors.rightMargin: -root.px(22)
                            hoverEnabled: true
                            onPressed: (mouse) => {
                                root.wheelBand = column.index
                                track.set(mouse.y)
                            }
                            onPositionChanged: (mouse) => { if (pressed) track.set(mouse.y) }
                            onExited: if (root.wheelBand === column.index) root.wheelBand = -1
                            // the wheel scrolls the page over the bands: it
                            // moves the band clicked, or any band with Ctrl
                            onWheel: (wheel) => {
                                if (root.wheelBand !== column.index && !(wheel.modifiers & Qt.ControlModifier)) {
                                    wheel.accepted = false
                                    return
                                }
                                track.set(track.height - root.px(2) - track.fraction * track.usable
                                          - (wheel.angleDelta.y > 0 ? 1 : -1) * track.usable / 80)
                            }
                        }
                        Accessible.role: Accessible.Slider
                        Accessible.name: root.names[column.index]
                    }

                    Text {
                        id: nameText
                        width: parent.width
                        y: root.px(320 + 33)
                        horizontalAlignment: Text.AlignHCenter
                        text: root.names[column.index]
                        color: root.c("#6B6B6B")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(14)
                        topPadding: VLCStyle.redesign.textBaseline(0, font.pixelSize) - nameMetrics.ascent
                        FontMetrics { id: nameMetrics; font: nameText.font }
                    }

                    Rectangle {
                        y: root.px(320 + 33 + 17 + 13)
                        width: parent.width
                        height: root.px(32)
                        radius: root.px(6)
                        color: root.c("#090909")
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.baseline: parent.verticalCenter
                            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
                            text: column.value.toFixed(1) + " dB"
                            color: root.c("#8F8F8F")
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.px(12)
                        }
                    }
                }
            }
        }
    }
}
