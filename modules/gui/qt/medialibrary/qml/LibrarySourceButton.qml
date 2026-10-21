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
import VLC.Style
import VLC.Network as Network
import VLC.MainInterface
import VLC.Dialogs
import VLC.MediaLibrary
import VLC.Widgets as Widgets

Widgets.ButtonExt {
    id: root
    property bool iconOnly: false
    colorContext.palette: VLCStyle.palette
    padding: iconOnly ? 0 : VLCStyle.dp(8, VLCStyle.scale)
    contentItem: RowLayout {
        spacing: 0
        Image {
            visible: root.iconOnly
            Layout.preferredWidth: VLCStyle.redesign.iconSize
            Layout.preferredHeight: VLCStyle.redesign.iconSize
            source: "qrc:///redesign/add.svg"
            // made at the size of the scale, not stretched from 24
            sourceSize: Qt.size(width, height)
        }
        Widgets.LabelExt {
            visible: !root.iconOnly
            text: root.text
            color: VLCStyle.redesign.textMuted
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
    }
    objectName: "libraryAddFolderButton"
    text: qsTr("Add folder")
    enabled: MainCtx.mediaLibraryAvailable
    onClicked: {
        const folder = filePicker.chooseFolder(qsTr("Add folder to media library"))
        if (folder.toString() !== "") folders.add(folder)
    }

    Network.MLFolderModel {
        id: folders
        ctx: MainCtx
        onOperationFailed: (op, url) => {
            DialogsProvider.messageDialog(qsTr("Could not add this folder to the library: %1").arg(url),
                                           qsTr("Media library"))
        }
    }

    Widgets.FilePicker { id: filePicker; ctx: MainCtx }
}
