/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import VLC.Dialogs as Dialogs
import VLC.MainInterface
import VLC.Style
import VLC.Widgets as Widgets

// What's new after an update (WhatsNewController, SetupDialogs.qml): the
// headlines of the version's section of the changelog, grouped as it groups
// them (New, Changed, Fixed), and its full notes on GitHub. Shown once per
// version, on the library once the window is up. The notes are the English
// ones of CHANGELOG.md: they are not translated.
Widgets.RedesignDialog {
    id: root

    objectName: "whatsNewDialog"

    property Dialogs.WhatsNewController controller: null
    // read when the dialog opens
    property var groups: []

    readonly property color textPrimary: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#1d1d1d", root.darkTheme)
    readonly property color textSecondary: VLCStyle.redesign.color(VLCStyle.redesign.textSecondary, "#3d3d3d", root.darkTheme)
    readonly property color textMuted: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#4d4d4d", root.darkTheme)

    title: qsTr("What's new")
    preferredWidth: VLCStyle.dp(560, VLCStyle.scale)
    cancelVisible: false
    acceptText: qsTr("Close")
    extraText: qsTr("Full changelog")

    function groupTitle(kind) {
        switch (kind) {
        case "added": return qsTr("New")
        case "changed": return qsTr("Changed")
        case "fixed": return qsTr("Fixed")
        default: return ""
        }
    }

    onAboutToShow: groups = controller ? controller.groups() : []
    // seen once it is on the screen: not again for this version
    onOpened: if (controller) controller.markSeen()
    onExtraClicked: {
        if (controller)
            controller.openChangelog()
        root.close()
    }

    Widgets.RedesignDialogHeading {
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        text: qsTr("VLC Redesign")
        suffix: root.controller ? root.controller.version : ""
        description: root.groups.length > 0
                     ? qsTr("Here is what changed in this version.")
                     : qsTr("This version's notes are on GitHub.")
    }

    // The headlines: they scroll in a low window
    Flickable {
        id: notesView

        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        visible: root.groups.length > 0
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.maximumHeight: VLCStyle.dp(420, VLCStyle.scale)
        implicitHeight: notes.implicitHeight
        contentWidth: width
        contentHeight: notes.implicitHeight
        clip: contentHeight > height
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds

        T.ScrollBar.vertical: Widgets.ScrollBarExt { }

        ColumnLayout {
            id: notes

            width: notesView.width
            spacing: VLCStyle.dp(16, VLCStyle.scale)

            Repeater {
                model: root.groups

                delegate: ColumnLayout {
                    id: group

                    required property var modelData

                    Layout.fillWidth: true
                    spacing: VLCStyle.dp(6, VLCStyle.scale)

                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        text: root.groupTitle(group.modelData.kind)
                        color: root.textPrimary
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                        font.weight: Font.DemiBold
                    }

                    Repeater {
                        model: group.modelData.items

                        delegate: RowLayout {
                            id: noteRow

                            required property string modelData

                            Layout.fillWidth: true
                            spacing: VLCStyle.dp(8, VLCStyle.scale)

                            // the same font as the text: on its first line
                            Text {
                                Layout.alignment: Qt.AlignTop
                                text: "•"
                                color: VLCStyle.redesign.accent
                                font.family: VLCStyle.redesign.fontFamily
                                font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
                                lineHeight: 1.3
                                Accessible.ignored: true
                            }
                            Text {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                                wrapMode: Text.WordWrap
                                horizontalAlignment: Text.AlignLeft
                                textFormat: Text.PlainText
                                text: noteRow.modelData
                                color: root.textSecondary
                                font.family: VLCStyle.redesign.fontFamily
                                font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
                                lineHeight: 1.3
                            }
                        }
                    }

                    Text {
                        visible: group.modelData.more > 0
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        wrapMode: Text.WordWrap
                        text: qsTr("And %1 more", "And %1 more", group.modelData.more).arg(group.modelData.more)
                        color: root.textMuted
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
                    }
                }
            }
        }
    }

    // the changelog is written in English
    Text {
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        visible: root.groups.length > 0 && root.controller !== null && root.controller.notesInOtherLanguage()
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        text: qsTr("The release notes are in English.")
        color: root.textMuted
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
    }
}
