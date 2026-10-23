/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import VLC.Style

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
            color: root.hovered || root.popup.visible ? root.c("#222222") : root.c("#191919")
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
        rotation: root.popup.visible ? 180 : 0
    }

    delegate: T.ItemDelegate {
        required property var modelData
        required property int index

        width: ListView.view ? ListView.view.width : root.width
        implicitHeight: root.px(32)
        leftPadding: root.px(14)
        rightPadding: root.px(14)
        highlighted: root.highlightedIndex === index

        contentItem: Text {
            id: optionText
            text: modelData[root.textRole] !== undefined ? modelData[root.textRole] : modelData
            color: index === root.currentIndex ? VLCStyle.redesign.accent : root.c("#8F8F8F")
            font: root.font
            verticalAlignment: Text.AlignTop
            // the baseline where the design centres a line of Inter
            topPadding: height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize) - optionMetrics.ascent
            FontMetrics { id: optionMetrics; font: optionText.font }
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: root.px(4)
            color: parent.highlighted || parent.hovered ? root.c("#222222") : "transparent"
        }
    }

    popup: T.Popup {
        // a press outside only closes it (on the box too: it doesn't open
        // again), and goes no further, as in a native list
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside
        modal: true
        dim: false
        y: root.height + root.px(4)
        width: root.width
        implicitHeight: Math.min(contentItem.implicitHeight + topPadding + bottomPadding, root.px(32 * 8 + 4))
        padding: root.px(2)

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
        }
        background: Rectangle {
            radius: root.px(6)
            color: root.c("#191919")
            border.width: 1
            border.color: root.c("#222222")
        }
    }
}
