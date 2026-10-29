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

    width: VLCStyle.dp(264, VLCStyle.scale)
    // (the rows, the line, "New profile…" and the message, scaled as they are)
    height: Math.min(profileModel.count, 6) * VLCStyle.dp(48, VLCStyle.scale)
            + VLCStyle.dp(1, VLCStyle.scale) + VLCStyle.dp(60, VLCStyle.scale)
            + ((switchError || profileModel.lastError) ? VLCStyle.dp(56, VLCStyle.scale) : 0)
    padding: 0
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside

    background: Rectangle {
        color: VLCStyle.redesign.color("#1b1b1b", "#ababaa")
        radius: VLCStyle.dp(11, VLCStyle.scale)
    }

    // the frame of the main window, over the content (which reaches the
    // card's edges): the card reads as a window over the interface
    readonly property Item _windowBorder: Widgets.RedesignWindowBorder {
        parent: root.background ? root.background.parent : null
        radius: VLCStyle.dp(11, VLCStyle.scale)
    }

    contentItem: Column {
        spacing: 0
        // a popup does not inherit the mirroring of the window (right to left)
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        ListView {
            id: profileList
            width: parent.width
            height: Math.min(root.profileModel.count, 6) * VLCStyle.dp(48, VLCStyle.scale)
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
                height: VLCStyle.dp(48, VLCStyle.scale)
                // (swapped right to left)
                leftPadding: LayoutMirroring.enabled ? VLCStyle.dp(16, VLCStyle.scale) : VLCStyle.dp(18, VLCStyle.scale)
                rightPadding: LayoutMirroring.enabled ? VLCStyle.dp(18, VLCStyle.scale) : VLCStyle.dp(16, VLCStyle.scale)
                background: Rectangle {
                    radius: VLCStyle.dp(6, VLCStyle.scale)
                    color: parent.hovered || parent.activeProfile ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
                }
                contentItem: Row {
                    spacing: VLCStyle.dp(12, VLCStyle.scale)
                    Image {
                        // drawn at the size shown (an SVG: sharp at any scale and zoom)
                        sourceSize: Qt.size(width, height)
                        smooth: !VLCStyle.redesign.svgExact(width, height, Screen.devicePixelRatio)
                        width: VLCStyle.dp(24, VLCStyle.scale)
                        height: VLCStyle.dp(24, VLCStyle.scale)
                        anchors.verticalCenter: parent.verticalCenter
                        source: "qrc:///redesign/icons/check/selected.svg"
                        opacity: activeProfile ? 1 : 0
                    }
                    Text {
                        width: parent.width - 2 * VLCStyle.dp(36, VLCStyle.scale)
                        height: parent.height
                        text: name
                        color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
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
                    anchors.rightMargin: VLCStyle.dp(12, VLCStyle.scale)
                    anchors.verticalCenter: parent.verticalCenter
                    width: VLCStyle.dp(32, VLCStyle.scale)
                    height: VLCStyle.dp(32, VLCStyle.scale)
                    padding: VLCStyle.dp(4, VLCStyle.scale)
                    opacity: rowHover.hovered || actionsMenu.opened || visualFocus ? 1 : 0
                    Accessible.name: qsTr("Actions of %1").arg(profileRow.name)
                    background: Rectangle {
                        radius: VLCStyle.dp(6, VLCStyle.scale)
                        color: profileActions.hovered || actionsMenu.opened
                               ? VLCStyle.redesign.color("#2e2e2e", "#8e8e8d") : "transparent"
                    }
                    contentItem: Image {
                        source: "qrc:///redesign/icons/more/default.svg"
                        sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                        fillMode: Image.Pad
                    }
                    // under the button, from its start (right to left: its
                    // right edge)
                    onClicked: actionsMenu.toggleFrom(profileActions,
                                                      LayoutMirroring.enabled ? profileActions.width - actionsMenu.width : 0,
                                                      profileActions.height)

                    Widgets.RedesignMenu {
                        id: actionsMenu
                        menuWidth: VLCStyle.dp(200, VLCStyle.scale)
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
            height: VLCStyle.dp(1, VLCStyle.scale)
            color: VLCStyle.redesign.color("#292929", "#9c9c9b")
        }

        Text {
            visible: !!(root.switchError || root.profileModel.lastError)
            width: parent.width - VLCStyle.dp(32, VLCStyle.scale)
            height: visible ? VLCStyle.dp(56, VLCStyle.scale) : 0
            x: VLCStyle.dp(16, VLCStyle.scale)
            text: root.switchError || root.profileModel.lastError
            color: VLCStyle.redesign.color("#e4a580", "#c62828")
            wrapMode: Text.WordWrap
            // (three lines in the 56 kept for it)
            maximumLineCount: 3
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
            verticalAlignment: Text.AlignVCenter
            Accessible.role: Accessible.AlertMessage
        }

        Controls.ItemDelegate {
            objectName: "newProfileAction"
            width: parent.width
            height: VLCStyle.dp(60, VLCStyle.scale)
            // (the end as the style pads it; swapped right to left)
            leftPadding: LayoutMirroring.enabled ? horizontalPadding : VLCStyle.dp(18, VLCStyle.scale)
            rightPadding: LayoutMirroring.enabled ? VLCStyle.dp(18, VLCStyle.scale) : horizontalPadding
            background: Rectangle {
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: parent.hovered ? VLCStyle.redesign.color("#252525", "#9c9c9b") : "transparent"
            }
            contentItem: Row {
                spacing: VLCStyle.dp(12, VLCStyle.scale)
                Text {
                    width: VLCStyle.dp(20, VLCStyle.scale)
                    height: parent.height
                    text: "+"
                    color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                    font.pixelSize: VLCStyle.dp(25, VLCStyle.scale)
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    width: Math.min(implicitWidth, parent.width - VLCStyle.dp(32, VLCStyle.scale))
                    height: parent.height
                    elide: Text.ElideRight
                    text: qsTr("New profile…")
                    color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
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
