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

Controls.Popup {
    id: root
    objectName: "profilesMenu"
    property var profileModel: MainCtx.controlbarProfileModel
    property string switchError: ""

    width: 264
    height: Math.min(profileModel.count, 6) * 48 + 61
            + ((switchError || profileModel.lastError) ? 56 : 0)
    padding: 0
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside

    background: Rectangle {
        color: VLCStyle.redesign.color("#1b1b1b", "#ababaa")
        radius: 11
        border.color: VLCStyle.redesign.color("#252525", "#9c9c9b")
    }

    contentItem: Column {
        spacing: 0

        ListView {
            id: profileList
            width: parent.width
            height: Math.min(root.profileModel.count, 6) * 48
            clip: true
            model: root.profileModel
            currentIndex: root.profileModel.selectedProfile
            boundsBehavior: Flickable.StopAtBounds

            delegate: Controls.ItemDelegate {
                required property int index
                required property string name
                readonly property bool activeProfile: root.profileModel.selectedProfile === index
                objectName: "profileOption_" + index
                width: profileList.width
                height: 48
                leftPadding: 18
                rightPadding: 16
                background: Rectangle {
                    radius: 6
                    color: parent.hovered || parent.activeProfile ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
                }
                contentItem: Row {
                    spacing: 12
                    Image {
                        width: 24
                        height: 24
                        anchors.verticalCenter: parent.verticalCenter
                        source: "qrc:///redesign/icons/check/selected.png"
                        opacity: activeProfile ? 1 : 0
                    }
                    Text {
                        width: parent.width - 36
                        height: parent.height
                        text: name
                        color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                        elide: Text.ElideRight
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: 16
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                Accessible.name: name
                Accessible.description: activeProfile ? qsTr("Selected profile") : ""
                onClicked: {
                    root.switchError = ""
                    if (root.profileModel.selectedProfile !== index) {
                        const previous = root.profileModel.selectedProfile
                        if (!root.profileModel.setSelectedProfile(index))
                            return
                        if (!root.profileModel.save()) {
                            root.switchError = root.profileModel.lastError
                            root.profileModel.setSelectedProfile(previous)
                            return
                        }
                    }
                    root.close()
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: VLCStyle.redesign.color("#292929", "#9c9c9b")
        }

        Text {
            visible: !!(root.switchError || root.profileModel.lastError)
            width: parent.width - 32
            height: visible ? 56 : 0
            x: 16
            text: root.switchError || root.profileModel.lastError
            color: VLCStyle.redesign.color("#e4a580", "#c62828")
            wrapMode: Text.WordWrap
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            Accessible.role: Accessible.AlertMessage
        }

        Controls.ItemDelegate {
            objectName: "newProfileAction"
            width: parent.width
            height: 60
            leftPadding: 18
            background: Rectangle {
                radius: 6
                color: parent.hovered ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
            }
            contentItem: Row {
                spacing: 12
                Text {
                    width: 20
                    height: parent.height
                    text: "+"
                    color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                    font.pixelSize: 25
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    height: parent.height
                    text: qsTr("New profile…")
                    color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: 16
                    verticalAlignment: Text.AlignVCenter
                }
            }
            onClicked: {
                root.close()
                // Customize Player asks for the name, then the new profile is
                // arranged there (Save keeps it, Cancel drops it)
                root.profileModel.requestNewProfile()
            }
        }
    }
}
