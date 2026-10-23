/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

import QtQuick
import QtQuick.Layouts
import VLC.Style
import VLC.MainInterface

import VLC.Dialogs
import VLC.Util
import VLC.Widgets as Widgets

Widgets.ButtonExt {
    id: root
    property bool iconOnly: false
    // The design has no add button in the header: its action goes in the menu
    property bool libraryFolder: false
    colorContext.palette: VLCStyle.palette
    padding: iconOnly ? 0 : VLCStyle.dp(8, VLCStyle.scale)
    contentItem: RowLayout {
        spacing: 0
        Image {
            visible: root.iconOnly
            Layout.preferredWidth: VLCStyle.redesign.iconSize
            Layout.preferredHeight: VLCStyle.redesign.iconSize
            // Figma: the more icon of the set, #6B6B6B
            source: VLCStyle.palette.isDark ? "qrc:///redesign/icons/more/dim.png" : "qrc:///redesign/menu.svg"
            // the size of the scale (the svg made for it, the png never larger)
            sourceSize: Qt.size(width, height)
            fillMode: Image.Pad
            smooth: false
        }
        Widgets.LabelExt {
            visible: !root.iconOnly
            text: root.text
            color: VLCStyle.redesign.textMuted
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
    }
    objectName: "openMediaButton"

    text: qsTr("Open media")
    AccessibleCompat.id: "openMediaButton"

    // Rows before the open actions: the design's "folder context menu" of
    // the folder shown (FolderDisplay), whose Add File and Add Folder stand
    // for File… and Folder…
    property var folderActions: []

    // Figma: the menu card under the button
    onClicked: menu.toggleFrom(root, 0, root.height)

    Widgets.RedesignContextMenu {
        id: menu

        readonly property bool _folder: root.folderActions.length > 0

        actions: root.folderActions.concat([
            { separator: true },
            { text: qsTr("Add folder to library…"), icon: "folder-add", action: () => librarySource.clicked(),
              visible: root.libraryFolder && MainCtx.mediaLibraryAvailable },
            { text: qsTr("File…"), icon: "open", action: () => DialogsProvider.simpleOpenDialog(),
              visible: !menu._folder },
            { text: qsTr("Folder…"), icon: "folder-open", action: () => DialogsProvider.PLOpenDir(),
              visible: !menu._folder },
            { text: qsTr("Network stream…"), icon: "streaming", action: () => DialogsProvider.openNetDialog() },
            { text: qsTr("Disc…"), icon: "disc", action: () => DialogsProvider.openDiscDialog() },
            { text: qsTr("Capture device…"), icon: "capture-device", action: () => DialogsProvider.openCaptureDialog() },
            { text: qsTr("Convert…"), icon: "swap", action: () => DialogsProvider.openAndTranscodingDialogs() },
            { text: qsTr("Stream…"), icon: "streaming", action: () => DialogsProvider.openAndStreamingDialogs() }
        ])

        onRequestData: (requestID, _) => setData(requestID, [])
    }

    // the folder picker and the library folders of the add button
    LibrarySourceButton {
        id: librarySource
        visible: false
    }
}
