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
import VLC.Widgets as Widgets

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

        // a popup does not inherit the mirroring of the window (right to left)
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        ListView {
            id: profileList
            width: parent.width
            height: Math.min(root.profileModel.count, 6) * 48
            clip: true
            model: root.profileModel
            currentIndex: root.profileModel.selectedProfile
            boundsBehavior: Flickable.StopAtBounds

            delegate: Controls.ItemDelegate {
                id: profileRow
                required property int index
                required property string name
                readonly property bool activeProfile: root.profileModel.selectedProfile === index
                objectName: "profileOption_" + index
                width: profileList.width
                height: 48
                // (swapped right to left)
                leftPadding: LayoutMirroring.enabled ? 16 : 18
                rightPadding: LayoutMirroring.enabled ? 18 : 16
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
                        width: parent.width - 36 - 36
                        height: parent.height
                        text: name
                        color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: 16
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                // Rename, Clone and Delete of the profile: shown under the
                // pointer only (Figma 6648:20666 draws the names alone);
                // Customize Player asks for the name or the confirmation
                HoverHandler { id: rowHover }
                Controls.Button {
                    id: profileActions
                    objectName: "profileActions_" + profileRow.index
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    width: 32
                    height: 32
                    padding: 4
                    opacity: rowHover.hovered || actionsMenu.opened || visualFocus ? 1 : 0
                    Accessible.name: qsTr("Actions of %1").arg(profileRow.name)
                    background: Rectangle {
                        radius: 6
                        color: profileActions.hovered || actionsMenu.opened
                               ? VLCStyle.redesign.color("#2e2e2e", "#8e8e8d") : "transparent"
                    }
                    contentItem: Image {
                        source: "qrc:///redesign/icons/more/default.png"
                        sourceSize: Qt.size(24, 24)
                        fillMode: Image.Pad
                    }
                    // under the button, from its start (right to left: its
                    // right edge)
                    onClicked: actionsMenu.toggleFrom(profileActions,
                                                      LayoutMirroring.enabled ? profileActions.width - actionsMenu.width : 0,
                                                      profileActions.height)

                    Widgets.RedesignMenu {
                        id: actionsMenu
                        menuWidth: 200

                        // (a menu does not inherit the mirroring of the
                        // window: its rows mirror right to left by themselves)
                        Widgets.RedesignMenuItem {
                            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
                            LayoutMirroring.childrenInherit: true
                            text: qsTr("Rename")
                            iconName: "edit"
                            onTriggered: {
                                root.close()
                                root.profileModel.requestProfileAction(profileRow.index, "rename")
                            }
                        }
                        Widgets.RedesignMenuItem {
                            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
                            LayoutMirroring.childrenInherit: true
                            text: qsTr("Clone")
                            // (a new profile from this one: the set has no copy icon)
                            iconName: "add"
                            onTriggered: {
                                root.close()
                                root.profileModel.requestProfileAction(profileRow.index, "clone")
                            }
                        }
                        // the last profile stays
                        Widgets.RedesignMenuItem {
                            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
                            LayoutMirroring.childrenInherit: true
                            text: qsTr("Delete")
                            iconName: "remove"
                            enabled: root.profileModel.count > 1
                            onTriggered: {
                                root.close()
                                root.profileModel.requestProfileAction(profileRow.index, "delete")
                            }
                        }
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
            // (three lines in the 56 kept for it)
            maximumLineCount: 3
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            Accessible.role: Accessible.AlertMessage
        }

        Controls.ItemDelegate {
            objectName: "newProfileAction"
            width: parent.width
            height: 60
            // (the end as the style pads it; swapped right to left)
            leftPadding: LayoutMirroring.enabled ? horizontalPadding : 18
            rightPadding: LayoutMirroring.enabled ? 18 : horizontalPadding
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
                    width: Math.min(implicitWidth, parent.width - 32)
                    height: parent.height
                    elide: Text.ElideRight
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
