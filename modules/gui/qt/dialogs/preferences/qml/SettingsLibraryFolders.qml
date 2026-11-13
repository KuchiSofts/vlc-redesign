/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.Style
import VLC.Network as Network
import VLC.MainInterface
import VLC.Dialogs
import VLC.Widgets as Widgets

// The folders the media library reads (Settings › Library): each with its
// Remove button, and Add folder (issue #81: a folder added at the first start,
// Music next to Videos, could not be taken out of the library anywhere). Its
// media leave the library with it; the files themselves are not touched.
Column {
    id: root
    objectName: "settingsLibraryFolders"
    readonly property bool available: MainCtx.mediaLibraryAvailable

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(dark, light) { return VLCStyle.redesign.color(dark, light) }

    spacing: px(4)
    topPadding: px(6)
    bottomPadding: px(6)

    Network.MLFolderModel {
        id: folders
        ctx: MainCtx
        onOperationFailed: (op, url) => {
            DialogsProvider.messageDialog(qsTr("Could not add this folder to the library: %1").arg(url),
                                          qsTr("Media library"))
        }
    }

    Widgets.FilePicker { id: filePicker; ctx: MainCtx }

    // the heading row: "Folders" and Add folder
    Item {
        width: root.width
        height: root.px(42)
        Text {
            anchors.left: parent.left
            anchors.right: addButton.left
            anchors.rightMargin: root.px(12)
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Folders")
            elide: Text.ElideRight
            color: root.c("#8F8F8F", "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
        }
        SettingsButton {
            id: addButton
            objectName: "settingsLibraryAddFolder"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Add folder")
            enabled: root.available
            onClicked: {
                const folder = filePicker.chooseFolder(qsTr("Add folder to media library"))
                if (folder.toString() !== "")
                    folders.add(folder)
            }
        }
    }

    Repeater {
        id: folderRows
        model: folders
        delegate: Item {
            id: folderRow
            required property int index
            required property string display_url
            required property var url
            width: root.width
            height: root.px(38)
            Rectangle {
                anchors.fill: parent
                radius: root.px(6)
                color: root.c("#141414", "#e3e3e2")
            }
            Text {
                anchors.left: parent.left
                anchors.leftMargin: root.px(12)
                anchors.right: removeButton.left
                anchors.rightMargin: root.px(12)
                anchors.verticalCenter: parent.verticalCenter
                textFormat: Text.PlainText
                text: folderRow.display_url
                // (the folder's own name at the end stays in sight)
                elide: Text.ElideMiddle
                color: root.c("#BDBDBD", "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(13)
                HoverHandler { id: pathHover }
                ToolTip.visible: pathHover.hovered && truncated
                ToolTip.text: text
                ToolTip.delay: 650
            }
            SettingsButton {
                id: removeButton
                objectName: "settingsLibraryRemoveFolder"
                anchors.right: parent.right
                anchors.rightMargin: root.px(3)
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Remove")
                enabled: root.available
                Accessible.name: qsTr("Remove the folder") + ": " + folderRow.display_url
                onClicked: folders.removeAt(folderRow.index)
            }
        }
    }

    Text {
        visible: folderRows.count === 0
        width: root.width
        text: root.available ? qsTr("No folder selected") : qsTr("Media library unavailable")
        color: root.c("#5F5F5F", "#6b6b6b")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(13)
    }
}
