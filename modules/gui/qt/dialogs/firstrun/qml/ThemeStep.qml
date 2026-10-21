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
import QtQuick.Controls.impl as Impl

import VLC.Style

Item {
    id: root
    property int themeIndex: 0
    property color accent: "#d37918"
    signal themeRequested(int index)
    signal accentRequested(color colorValue)

    readonly property var accentChoices: VLCStyle.redesign.accentChoices
    readonly property bool _light: root.themeIndex === 1

    // Figma: the player behind the choices, scaled down: the title strip
    // (the cone, the window buttons) and the video
    Rectangle {
        x: 17
        y: 19
        width: parent.width - 34
        height: 22
        color: root._light ? "#ababaa" : "#191919"

        // the cone and the window buttons as the design draws them (masks
        // of its raster, coloured)
        Impl.ColorImage {
            width: 1056
            height: 22
            source: "qrc:///redesign/theme-strip-cone.png"
            smooth: false
            color: root.accent
        }
        Impl.ColorImage {
            width: 1056
            height: 22
            source: "qrc:///redesign/theme-strip-buttons.png"
            smooth: false
            color: root._light ? "#4d4d4d" : "#8f8f8f"
        }
    }
    Rectangle {
        x: 17
        y: 41
        width: parent.width - 34
        height: 542
        color: root._light ? "#dedede" : "#090909"
    }

    // Figma 1902:3670: a box of 42 at 173 (Light 24, line 175 %), the
    // advance rounded up and centred, the baseline on the whole pixel
    Text {
        id: themeTitle
        // on the pixel at its left, as Figma places it (329)
        x: Math.floor((parent.width - width) / 2)
        y: 173
        width: Math.ceil(implicitWidth - 0.05)
        height: 42
        verticalAlignment: Text.AlignTop
        topPadding: Math.round((42 - 1.2109375 * font.pixelSize) / 2 + 0.96875 * font.pixelSize) - themeTitleMetrics.ascent
        text: qsTr("Select your preferred theme and color.")
        color: "#969696"
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 24
        font.weight: Font.Light

        FontMetrics {
            id: themeTitleMetrics
            font: themeTitle.font
        }
    }

    Row {
        y: 282
        spacing: 32
        anchors.horizontalCenter: parent.horizontalCenter
        Repeater {
            model: [
                { title: qsTr("Light"), index: 1 },
                { title: qsTr("Dark"), index: 2 },
                { title: qsTr("System"), index: 0 }
            ]
            delegate: Item {
                required property var modelData
                width: 124
                height: 172
                Rectangle {
                    x: 0
                    y: 0
                    width: 124
                    height: 124
                    // Figma: the ring of the chosen theme, radius 23
                    radius: 23
                    color: "transparent"
                    // Figma: a ring of 4
                    border.width: root.themeIndex === modelData.index ? 4 : 0
                    border.color: root.accent
                    // Figma: #AFAFAF light, #212121 dark, radius 12
                    Rectangle {
                        visible: modelData.index !== 0
                        x: 12
                        y: 12
                        width: 100
                        height: 100
                        radius: 12
                        color: modelData.index === 1 ? "#afafaf" : "#212121"
                    }
                    // System: dark left of the line (16,0)-(77,100) with a
                    // hard edge, the corners rounded (the tile of the design)
                    Image {
                        visible: modelData.index === 0
                        x: 12
                        y: 12
                        width: 100
                        height: 100
                        source: "qrc:///redesign/theme-system.png"
                        smooth: false
                    }
                }
                // Figma: Inter Light 16, #8F8F8F, the baseline on the pixel
                // (439)
                Text {
                    id: themeLabel
                    y: 142
                    topPadding: 15 - themeLabelMetrics.ascent
                    width: parent.width
                    text: modelData.title
                    color: "#8f8f8f"
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: 16
                    font.weight: Font.Light
                    horizontalAlignment: Text.AlignHCenter

                    FontMetrics {
                        id: themeLabelMetrics
                        font: themeLabel.font
                    }
                }
                Controls.Button {
                    id: themeButton
                    anchors.fill: parent
                    // reached with the keyboard: the tile outlined
                    background: Rectangle {
                        width: 124
                        height: 124
                        radius: 23
                        color: "transparent"
                        border.width: themeButton.visualFocus ? 2 : 0
                        border.color: "#eaeaea"
                    }
                    contentItem: Item {}
                    Accessible.name: modelData.title
                    onClicked: root.themeRequested(modelData.index)
                }
            }
        }
    }

    // A sample of the control bar in the chosen theme and accent (the
    // design shows the real one, scaled down)
    Rectangle {
        id: sample

        readonly property bool light: root.themeIndex === 1
        readonly property color iconColor: light ? "#4d4d4d" : "#8f8f8f"

        x: 17
        y: 583
        width: parent.width - 34
        height: 30
        color: light ? "#ababaa" : "#181818"

        // Figma 1902:4020: the player's bar scaled down (0.55), each part
        // where the design draws it (x from the bar, 30 high); a lighter
        // line on top
        Rectangle {
            width: parent.width
            height: 1
            visible: !sample.light
            color: "#1f1f1f"
        }

        // the icons and the times of the bar as the design draws them (a
        // mask of its raster, coloured)
        Impl.ColorImage {
            width: 1056
            height: 30
            source: "qrc:///redesign/theme-bar-mask.png"
            smooth: false
            color: sample.iconColor
        }
        // the play pill and its triangle (masks of the design's raster)
        Impl.ColorImage {
            x: 27
            y: 3
            width: 40
            height: 24
            source: "qrc:///redesign/theme-bar-pill.png"
            smooth: false
            color: root.accent
        }
        Impl.ColorImage {
            x: 41
            y: 9
            width: 14
            height: 14
            source: "qrc:///redesign/theme-bar-play.png"
            smooth: false
            color: "#161616"
        }

        // Seek bar: the elapsed part in a dim accent, the knob in the accent
        Rectangle {
            id: sampleSeek
            x: 139
            y: 7.5
            width: 682
            height: 15
            radius: 2.5
            color: sample.light ? "#dedede" : "#090909"
            Rectangle {
                width: parent.width * 0.075
                height: parent.height
                radius: parent.radius
                color: Qt.tint(sample.light ? "#dedede" : "#090909", Qt.alpha(root.accent, sample.light ? 0.35 : 0.24))
            }
        }
        // the knob (a mask of the design's raster)
        Impl.ColorImage {
            x: 186
            y: 5
            width: 12
            height: 20
            source: "qrc:///redesign/theme-bar-knob.png"
            smooth: false
            color: root.accent
        }

        Image {
            x: 895
            y: 8
            width: 49
            height: 15
            source: "qrc:///redesign/volume-background.svg"
            sourceSize: Qt.size(98, 30)
            // the level: the gradient of the whole track shown over its
            // first 17 px (Figma)
            Item {
                x: 1
                anchors.verticalCenter: parent.verticalCenter
                width: 17
                height: parent.height - 2
                clip: true
                Image {
                    width: parent.parent.width - 2
                    height: parent.height
                    source: "qrc:///redesign/volume-fill.svg"
                    sourceSize: Qt.size(width * 2, height * 2)
                }
            }
        }
    }

    // Figma 1902:4020: discs of 40 every 80 from x 53 at y 667, the chosen
    // one in a ring of 64 (4 thick, the accent at 30 %)
    Row {
        x: 41
        y: 655
        spacing: 16
        Repeater {
            model: root.accentChoices
            delegate: Item {
                id: choice

                required property int index
                required property var modelData
                readonly property bool chosen: root.accent.toString().toLowerCase() === modelData

                width: 64
                height: 64
                // The chosen color has a thin ring, apart from the disc
                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: "transparent"
                    border.width: choice.chosen ? 4 : 0
                    border.color: Qt.alpha(choice.modelData, 0.3)
                    Rectangle {
                        anchors.centerIn: parent
                        width: 40
                        height: 40
                        radius: 20
                        color: choice.modelData
                        // The others are dimmed
                        opacity: choice.chosen ? 1.0 : 0.5
                    }
                }
                Controls.Button {
                    id: accentButton
                    anchors.fill: parent
                    // reached with the keyboard: the disc outlined
                    background: Rectangle {
                        radius: width / 2
                        color: "transparent"
                        border.width: accentButton.visualFocus ? 2 : 0
                        border.color: "#eaeaea"
                    }
                    contentItem: Item {}
                    Accessible.name: qsTr("Accent color %1").arg(index + 1)
                    onClicked: root.accentRequested(modelData)
                }
            }
        }
    }
}
