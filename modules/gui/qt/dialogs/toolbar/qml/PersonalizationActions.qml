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
    width: 320
    // as the rows of the player's menu around them (PlayerMenu.qml)
    spacing: 4

    Controls.ItemDelegate {
        id: profileButton
        objectName: "openProfilesAction"
        width: parent.width
        height: 48
        background: Rectangle {
            radius: 6
            color: profileButton.hovered || profilesMenu.opened ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
        }
        // (right to left: the row mirrored, the chevron pointing left)
        contentItem: Row {
            spacing: 12
            Text {
                id: profileLabel
                width: profileButton.width - 54
                height: parent.height
                text: qsTr("Profile (%1)").arg(root.profileModel.currentModel
                                            ? root.profileModel.currentModel.name : qsTr("None"))
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: 16
                verticalAlignment: Text.AlignVCenter
                // 12 in, as the other rows of the menu
                leftPadding: LayoutMirroring.enabled ? 0 : 12
                rightPadding: LayoutMirroring.enabled ? 12 : 0
                HoverHandler { id: profileLabelHover }
                Controls.ToolTip.visible: profileLabelHover.hovered && profileLabel.truncated
                Controls.ToolTip.text: profileLabel.text
                Controls.ToolTip.delay: 650
            }
            Image {
                width: 24
                height: 24
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:///redesign/icons/chevron-right/default.png"
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
        height: 48
        background: Rectangle {
            radius: 6
            color: customizeButton.hovered ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
        }
        contentItem: Text {
            text: qsTr("Customize player")
            color: VLCStyle.redesign.color("#969696", "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: 16
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
            leftPadding: LayoutMirroring.enabled ? 0 : 12
            rightPadding: LayoutMirroring.enabled ? 12 : 0
        }
        onClicked: root.customizeRequested()
    }
}
