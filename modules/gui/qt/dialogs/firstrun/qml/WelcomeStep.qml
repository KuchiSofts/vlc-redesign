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
import QtQuick.Templates as T

import VLC.Style
import VLC.Util

Item {
    id: root
    property var folderUrl
    // the folder as the design writes it ("~/Videos")
    property string folderText: ""
    // without the media library no folder is added: the choice is hidden
    property bool folderAvailable: true
    readonly property bool _hasFolder: !!folderUrl && folderUrl.toString() !== ""
    property bool metadataNetworkAccess: false
    signal browseRequested()
    // the folder left out (the library starts empty)
    signal clearRequested()
    signal consentRequested(bool allowed)

    // The left edge of the folder row (595 wide, centred); right to left the
    // row is mirrored around the middle
    readonly property real _rowX: Math.round((width - 595) / 2)
    // A longer description (a translation) moves the folder row down, never
    // up from where the design puts it
    readonly property real _rowShift: Math.max(0, description.y + description.height + 14 - 500)

    // Figma 1899:3055 on whole pixels (centring 1090 gives halves)
    Image {
        width: 141
        height: 141
        x: Math.round((parent.width - width) / 2)
        y: 116
        source: "qrc:///redesign/welcome.svg"
        sourceSize: Qt.size(width, height)
        fillMode: Image.PreserveAspectFit
        Accessible.name: qsTr("VLC")
    }
    // Figma 1899:3055: the text box (the advance rounded up) centred on the
    // pixel at its left (417)
    Text {
        x: Math.floor((parent.width - width) / 2)
        width: Math.ceil(implicitWidth - 0.05)
        y: 286
        text: qsTr("Welcome to VLC!")
        color: VLCStyle.redesign.color("#eaeaea", "#2e2e2e")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 32
        font.weight: Font.Light
    }
    Text {
        id: description
        x: Math.round((parent.width - width) / 2)
        y: 362
        width: 595
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignLeft
        // Figma: lines of 28 (175 %), the glyphs centred on each, the
        // baselines on whole pixels
        lineHeightMode: Text.FixedHeight
        lineHeight: 28
        topPadding: Math.round((28 - 1.2109375 * font.pixelSize) / 2 + 0.96875 * font.pixelSize) - descriptionMetrics.ascent
        text: qsTr("VLC is a free and open source cross-platform multimedia player and framework that plays most multimedia files as well as DVDs, Audio CDs, VCDs, and various streaming protocols.")
        color: VLCStyle.redesign.color("#969696", "#4d4d4d")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 16
        font.weight: Font.Light

        FontMetrics {
            id: descriptionMetrics
            font: description.font
        }
    }
    // Figma: a box of 17 (14, line 100 %), the baseline at 514
    Text {
        id: folderLabel
        x: LayoutMirroring.enabled ? parent.width - width - root._rowX : root._rowX
        y: 500 + root._rowShift
        width: Math.min(implicitWidth, 595)
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
        visible: root.folderAvailable
        topPadding: 14 - folderLabelMetrics.ascent
        text: qsTr("Choose a Media Folder to get started with")
        color: VLCStyle.redesign.color("#8f8f8f", "#2e2e2e")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 14

        FontMetrics {
            id: folderLabelMetrics
            font: folderLabel.font
        }
    }
    // (the field gives Browse the room a longer word needs)
    Rectangle {
        x: LayoutMirroring.enabled ? parent.width - width - root._rowX : root._rowX
        y: 533 + root._rowShift
        width: 595 - 10 - browseButton.width
        height: 36
        visible: root.folderAvailable
        color: VLCStyle.redesign.color("#090909", "#dedede")
        border.width: 2
        border.color: VLCStyle.redesign.color("#191919", "#ababaa")
        radius: 6
        Text {
            id: folderText
            anchors.fill: parent
            // 12 inside the border of 2 (the clear button: 24 and 8 more)
            anchors.leftMargin: 14
            anchors.rightMargin: clearFolder.visible ? 14 + 24 + 8 : 14
            // the baseline where the design centres a line of Inter
            verticalAlignment: Text.AlignTop
            topPadding: Math.round(height / 2 + VLCStyle.redesign.centeredBaseline(font.pixelSize)) - folderMetrics.ascent
            elide: Text.ElideMiddle
            horizontalAlignment: Text.AlignLeft
            text: root._hasFolder
                  ? (root.folderText || Helpers.localPathFromUrl(root.folderUrl)) : qsTr("No folder selected")
            color: VLCStyle.redesign.color("#c6c6c6", "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: 16

            FontMetrics {
                id: folderMetrics
                font: folderText.font
            }
        }
        // Not in the design: the suggested folder can be left out
        T.AbstractButton {
            id: clearFolder
            objectName: "onboardingClearFolder"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 24
            height: 24
            visible: root._hasFolder
            text: qsTr("Remove the folder")
            Accessible.name: text
            contentItem: Image {
                source: "qrc:///redesign/icons/close/default.svg"
                sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                fillMode: Image.Pad
                opacity: clearFolder.hovered || clearFolder.down ? 1 : 0.6
            }
            onClicked: root.clearRequested()
        }
    }
    OnboardingButton {
        id: browseButton
        // at the end of the row
        x: LayoutMirroring.enabled ? parent.width - root._rowX - 595 : root._rowX + 595 - width
        // 35 high (padding 8 around 19), centred in the 36 of the row
        y: 533.5 + root._rowShift
        width: Math.max(114, implicitWidth)
        height: 35
        visible: root.folderAvailable
        objectName: "onboardingBrowseFolder"
        text: qsTr("Browse")
        font.weight: Font.Normal
        onClicked: root.browseRequested()
    }
}
