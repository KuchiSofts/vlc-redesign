/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Templates as T

import VLC.Style

// Basic shortcuts of the onboarding (Figma 1902:6770): two columns (at 62
// and 605) of six rows 82 apart from 154, the action (220 wide) and its keys
// 20 after it. A key: 42 high, a border of 3 (#373737), radius 12, #1B1B1B,
// its text 20 from the sides (12 for a sign) and a drop of 6 below (#5C5C5C
// at 25 %); "/" 10 from the keys it separates, the keys of a combination 4
// apart. Inter Light 16 on lines of 28 (175 %).
Item {
    id: root

    // [{ action, sequences: ["Space"] or ["Left", "Right"] }]
    property var shortcuts: []

    readonly property int rows: Math.ceil(shortcuts.length / 2)

    // "Shift+s" reads SHIFT, S; "Left" ←; letters are capitals
    function keysOf(sequence) {
        const names = { "Left": "←", "Right": "→", "Up": "↑", "Down": "↓",
                        "Shift": "SHIFT", "Ctrl": "CTRL", "Alt": "ALT", "Meta": "META",
                        // VLC's hotkeys of macOS ("Command+...")
                        "Command": "⌘" }
        // "+" itself is a key ("Ctrl++")
        const parts = sequence.replace(/\+\+$/, "+plus").split("+").filter(part => part.length > 0)
        return parts.map(part => part === "plus" ? "+"
                                                 : (names[part] ?? (part.length === 1 ? part.toUpperCase() : part)))
    }

    // Inter as Figma lays it out: the line box (1.2109 em, ascender
    // 0.96875) centred in the line, the baseline on the whole pixel
    component FigmaText: Text {
        id: figmaText

        property real lineHeightPx: 28

        height: lineHeightPx
        verticalAlignment: Text.AlignTop
        color: VLCStyle.redesign.color("#969696", "#4d4d4d")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 16
        font.weight: Font.Light
        topPadding: Math.round((lineHeightPx - 1.2109375 * font.pixelSize) / 2 + 0.96875 * font.pixelSize) - figmaMetrics.ascent

        FontMetrics {
            id: figmaMetrics
            font: figmaText.font
        }
    }

    // A raised key
    component Keycap: Item {
        id: keycap

        property string text

        // a sign takes 12 on each side, a letter or a word 20
        readonly property bool sign: text.length === 1 && !/[A-Za-z0-9]/.test(text)
        readonly property bool arrow: text.length === 1 && text >= "←" && text <= "↓"
        // the box of the text: its advance to the pixel above (Figma's
        // hug), the arrows on 16
        readonly property int textWidth: Math.max(Math.ceil(label.implicitWidth - 0.05), arrow ? 16 : 0)

        width: 2 * (3 + (sign ? 12 : 20)) + textWidth
        height: 42

        // the drop shadow (0, 6, no blur), behind the key
        Rectangle {
            y: 6
            width: parent.width
            height: parent.height
            radius: 12
            color: Qt.rgba(0x5c / 255, 0x5c / 255, 0x5c / 255, 0.25)
        }
        Rectangle {
            anchors.fill: parent
            radius: 12
            color: VLCStyle.redesign.color("#1b1b1b", "#ababaa")
            border.width: 3
            border.color: VLCStyle.redesign.color("#373737", "#8e8e8d")
        }
        FigmaText {
            id: label
            x: 3 + (keycap.sign ? 12 : 20)
            y: 7
            width: keycap.textWidth
            // Figma: the text at the start of its box
            horizontalAlignment: Text.AlignLeft
            text: keycap.text
            color: VLCStyle.redesign.color("#eaeaea", "#2e2e2e")
        }
    }

    FigmaText {
        y: 60
        width: parent.width
        lineHeightPx: 42
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        text: qsTr("Learn the basic keyboard shortcuts.")
        font.pixelSize: 24
        font.weight: Font.Normal
    }

    FigmaText {
        anchors.centerIn: parent
        visible: root.shortcuts.length === 0
        text: qsTr("No playback shortcuts are currently assigned.")
    }

    Repeater {
        model: root.shortcuts

        delegate: Item {
            id: row

            required property int index
            required property var modelData

            readonly property int column: index < root.rows ? 0 : 1
            readonly property int columnX: column === 0 ? 62 : 605

            // right to left: the first column on the right, the action at
            // the right of its row, its keys before it
            x: LayoutMirroring.enabled ? parent.width - width - columnX : columnX
            y: 154 + (index % root.rows) * 82
            width: 393
            height: 42

            FigmaText {
                id: actionText
                x: LayoutMirroring.enabled ? parent.width - width : 0
                y: 7
                width: 220
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                text: row.modelData.action
                HoverHandler { id: actionHover }
                T.ToolTip.visible: actionHover.hovered && actionText.truncated
                T.ToolTip.text: actionText.text
                T.ToolTip.delay: 650
            }

            // The keys read left to right, as on the keyboard and in the
            // menus ("CTRL" "+" "S"), in any language
            Row {
                x: row.LayoutMirroring.enabled ? row.width - width - 240 : 240
                LayoutMirroring.enabled: false
                LayoutMirroring.childrenInherit: true
                spacing: 10

                Repeater {
                    model: row.modelData.sequences

                    delegate: Row {
                        required property int index
                        required property string modelData

                        spacing: 10

                        FigmaText {
                            visible: index > 0
                            y: 7
                            width: Math.ceil(implicitWidth - 0.05)
                            text: "/"
                        }

                        Row {
                            spacing: 4

                            Repeater {
                                model: root.keysOf(modelData)
                                delegate: Keycap {
                                    required property string modelData
                                    text: modelData
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // (beside Back and Finish: a longer one cut before them)
    FigmaText {
        x: LayoutMirroring.enabled ? parent.width - width - 62 : 62
        y: 671
        width: Math.min(implicitWidth, parent.width - 62 - 300)
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
        text: qsTr("Find the full list in the menu.")
        color: "#6b6b6b"
        font.weight: Font.Normal
    }
}
