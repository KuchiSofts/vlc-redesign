/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl
import VLC.Style
import VLC.Widgets as Widgets

// Dropdown of the design (Figma component "Dropdown"): a 320 x 36 well
// (#090909, 2 px padding) around a 316 x 32 option (#191919), 14 px text
// and the 24 px chevron at 14 px from the edges
T.ComboBox {
    id: root

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, value) }

    implicitWidth: px(320)
    implicitHeight: px(36)
    padding: px(2)
    leftPadding: px(16)
    rightPadding: px(16 + 24 + 10)
    textRole: "label"
    focusPolicy: Qt.StrongFocus
    font.family: VLCStyle.redesign.fontFamily
    font.pixelSize: px(14)

    background: Rectangle {
        radius: root.px(6)
        color: root.c("#090909")
        border.width: root.visualFocus ? 1 : 0
        border.color: VLCStyle.redesign.accent

        Rectangle {
            anchors.fill: parent
            anchors.margins: root.px(2)
            radius: root.px(4)
            // (open, as closed: Figma 6456:13892)
            color: root.hovered && !root.popup.visible ? root.c("#222222") : root.c("#191919")
        }
    }

    contentItem: Text {
        id: valueText
        text: root.displayText
        color: root.c("#8F8F8F")
        font: root.font
        verticalAlignment: Text.AlignTop
        // the baseline where the design centres a line of Inter
        topPadding: height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize) - valueMetrics.ascent
        FontMetrics { id: valueMetrics; font: valueText.font }
        elide: Text.ElideRight
    }

    indicator: Image {
        x: root.width - root.px(2 + 14) - width
        y: (root.height - height) / 2
        width: root.px(24)
        height: width
        sourceSize: Qt.size(width, height)
        source: "qrc:///redesign/icons/chevron-down/default.png"
    }

    // Figma 6456:13892 / 2372:8300: rows of 44, 2 apart, the text 14 in,
    // #1F1F1F under the one chosen or the pointer; the one chosen keeps its
    // grey and gets the check of the accent, 13 from the end
    delegate: T.ItemDelegate {
        id: option
        required property var modelData
        required property int index

        readonly property bool chosen: index === root.currentIndex

        width: ListView.view ? ListView.view.width : root.width
        implicitHeight: root.px(44)
        rightPadding: root.px(14 + 24 + 13)
        highlighted: root.highlightedIndex === index

        // a choice with an icon (the curves of the equalizer presets): the
        // icon 14 in, the text 10 after it
        readonly property string iconDir: option.modelData.icon ?? ""
        leftPadding: iconDir ? root.px(14 + 24 + 10) : root.px(14)

        contentItem: Text {
            id: optionText
            text: option.modelData[root.textRole] !== undefined ? option.modelData[root.textRole] : option.modelData
            color: root.c("#8F8F8F")
            font: root.font
            verticalAlignment: Text.AlignTop
            // the baseline where the design centres a line of Inter
            topPadding: height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize) - optionMetrics.ascent
            FontMetrics { id: optionMetrics; font: optionText.font }
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: root.px(4)
            color: option.chosen || option.highlighted || option.hovered ? root.c("#1F1F1F") : "transparent"

            // drawn in grey and the accent: with another accent, the grey
            // part and the rest tinted above it
            Image {
                id: choiceIcon
                visible: option.iconDir.length > 0
                readonly property bool twoTone: !VLCStyle.redesign.defaultAccent
                x: root.px(14)
                anchors.verticalCenter: parent.verticalCenter
                width: root.px(24)
                height: width
                source: visible ? option.iconDir + (twoTone ? "/default-grey.png" : "/default.png") : ""
                Impl.ColorImage {
                    anchors.fill: parent
                    visible: choiceIcon.twoTone
                    source: visible ? option.iconDir + "/default-accent.png" : ""
                    color: VLCStyle.redesign.accent
                }
            }
            Impl.ColorImage {
                visible: option.chosen
                x: parent.width - root.px(13) - width
                anchors.verticalCenter: parent.verticalCenter
                width: root.px(24)
                height: width
                source: "qrc:///redesign/track-check-active.png"
                color: VLCStyle.redesign.accentTint
            }
        }
    }

    popup: T.Popup {
        // a press outside only closes it (on the box too: it doesn't open
        // again), and goes no further, as in a native list
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside
        modal: true
        dim: false
        // Figma: 20 under the field, as wide, padding 2, no border, the
        // drop shadow of the menus
        y: root.height + root.px(20)
        width: root.width
        implicitHeight: Math.min(contentItem.implicitHeight + topPadding + bottomPadding, root.px(46 * 8 + 2))
        padding: root.px(2)
        margins: root.px(8)

        contentItem: ListView {
            clip: true
            spacing: root.px(2)
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
        }
        background: Rectangle {
            radius: root.px(6)
            color: root.c("#191919")

            // Figma: drop shadow 0 8 blur 15 #000000 46 % (as RedesignMenu)
            Widgets.RoundedRectangleShadow {
                yOffset: root.px(8)
                blurRadius: 7.5 * VLCStyle.scale
                color: Qt.rgba(0, 0, 0, 0.46)
            }
        }
    }
}
