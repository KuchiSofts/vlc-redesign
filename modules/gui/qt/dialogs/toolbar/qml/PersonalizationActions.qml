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

import VLC.Dialogs
import VLC.MainInterface
import VLC.Style

Column {
    id: root
    objectName: "personalizationActions"
    property var profileModel: MainCtx.controlbarProfileModel
    signal customizeRequested()
    width: VLCStyle.dp(320, VLCStyle.scale)
    // as the rows of the player's menu around them (PlayerMenu.qml)
    spacing: VLCStyle.dp(4, VLCStyle.scale)
    Controls.ItemDelegate {
        id: profileButton
        objectName: "openProfilesAction"
        width: parent.width
        height: VLCStyle.dp(48, VLCStyle.scale)
        background: Rectangle {
            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: profileButton.hovered || profilesMenu.opened ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
        }
        // (right to left: the row mirrored, the chevron pointing left)
        contentItem: Row {
            spacing: VLCStyle.dp(12, VLCStyle.scale)
            Text {
                id: profileLabel
                width: profileButton.width - VLCStyle.dp(54, VLCStyle.scale)
                height: parent.height
                text: qsTr("Profile (%1)").arg(root.profileModel.currentModel
                                            ? root.profileModel.currentModel.name : qsTr("None"))
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                verticalAlignment: Text.AlignVCenter
                // 12 in, as the other rows of the menu
                leftPadding: LayoutMirroring.enabled ? 0 : VLCStyle.dp(12, VLCStyle.scale)
                rightPadding: LayoutMirroring.enabled ? VLCStyle.dp(12, VLCStyle.scale) : 0
                HoverHandler { id: profileLabelHover }
                Controls.ToolTip.visible: profileLabelHover.hovered && profileLabel.truncated
                Controls.ToolTip.text: profileLabel.text
                Controls.ToolTip.delay: 650
            }
            Image {
                // drawn at the size shown (an SVG: sharp at any scale and zoom)
                sourceSize: Qt.size(width, height)
                width: VLCStyle.dp(24, VLCStyle.scale)
                height: VLCStyle.dp(24, VLCStyle.scale)
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:///redesign/icons/chevron-right/default.svg"
                mirror: LayoutMirroring.enabled
            }
        }
        onClicked: profilesMenu.open()

        ProfilesMenu {
            id: profilesMenu
            parent: profileButton
            profileModel: root.profileModel
            // under the row, from its start (right to left: its right edge)
            x: profileButton.LayoutMirroring.enabled ? profileButton.width - width : 0
            y: profileButton.height
        }
    }

    Controls.ItemDelegate {
        id: customizeButton
        objectName: "customizePlayerAction"
        width: parent.width
        height: VLCStyle.dp(48, VLCStyle.scale)
        background: Rectangle {
            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: customizeButton.hovered ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
        }
        contentItem: Text {
            text: qsTr("Customize player")
            color: VLCStyle.redesign.color("#969696", "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
            leftPadding: LayoutMirroring.enabled ? 0 : VLCStyle.dp(12, VLCStyle.scale)
            rightPadding: LayoutMirroring.enabled ? VLCStyle.dp(12, VLCStyle.scale) : 0
        }
        onClicked: root.customizeRequested()
    }
}
