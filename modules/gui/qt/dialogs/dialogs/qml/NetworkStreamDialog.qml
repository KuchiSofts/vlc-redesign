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

import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Playlist
import VLC.Style
import VLC.Widgets as Widgets

// Figma "Open network stream": one or several links to play, optionally
// kept as a playlist of the library
Widgets.RedesignDialog {
    id: root

    objectName: "networkStreamDialog"

    // One link per line; spaces also separate links
    readonly property var _words: urlField.text.split(/\s+/).filter(url => url.length > 0)
    readonly property var urls: _words.map(url => root.withScheme(url))
    // What is neither a link nor a path ("my favourite channel" typed in the
    // links): not opened, nor added to the library as media
    readonly property var notLinks: _words.filter(word => !root.isLink(word))
    readonly property bool canSavePlaylist: MainCtx.mediaLibraryAvailable

    // A scheme ("http:", "rtsp:", "magnet:", a drive "C:"), a path, or a
    // host as withScheme() takes it
    function isLink(text) {
        return /^[A-Za-z][A-Za-z0-9+.-]*:/.test(text)
            || /^[\/\\~.]/.test(text)
            || /^(\[[0-9A-Fa-f:.]+\]|localhost|[A-Za-z0-9-]+(\.[A-Za-z0-9-]+)+)(:\d+)?([\/?#].*)?$/.test(text)
    }

    // A link typed without its scheme ("example.com/live.m3u8") names a
    // host, not a file of the computer: https, or http for an address,
    // localhost or a port (local servers, IPTV boxes). Links with a scheme
    // and local paths stay as they are.
    function withScheme(text) {
        // a path: "/", "\\", "~", ".", or a drive ("C:\", "C:/")
        if (/^[\/\\~.]/.test(text) || /^[A-Za-z]:([\/\\]|$)/.test(text))
            return text
        const host = /^(\[[0-9A-Fa-f:.]+\]|localhost|[A-Za-z0-9-]+(\.[A-Za-z0-9-]+)+)(:(\d+))?([\/?#].*)?$/.exec(text)
        if (!host) {
            // anything else as typed ("rtsp://...", "magnet:?...")
            return text
        }
        const address = host[1].startsWith("[") || host[1] === "localhost"
                        || /^\d{1,3}(\.\d{1,3}){3}$/.test(host[1])
        const port = host[4] !== undefined && host[4] !== "443"
        return ((address || port) ? "http://" : "https://") + text
    }

    title: qsTr("Open Network Stream")
    // Figma: "Frame 124" fixed at 509 (room under the fields)
    bodyHeight: VLCStyle.dp(509, VLCStyle.scale)
    // Opens the links, saved as a playlist when named (Figma: "Save")
    acceptText: qsTr("Save")
    acceptEnabled: urls.length > 0 && notLinks.length === 0
                   && (nameField.text.trim() === "" || (canSavePlaylist && !playlistModel.transactionPending))

    // Figma 6216:13614 opens with no field focused (no caret)
    initialField: urlField
    onClosed: {
        urlField.clear()
        nameField.clear()
    }

    onAccepted: {
        MainPlaylistController.append(root.urls, true)
        const name = nameField.text.trim()
        if (name !== "" && root.canSavePlaylist)
            playlistModel.create(name, root.urls)
    }

    MLPlaylistListModel {
        id: playlistModel
        ml: MainCtx.mediaLibraryAvailable ? MediaLib : null
    }

    Widgets.RedesignDialogHeading {
        text: qsTr("Network URL")
        description: root.notLinks.length > 0
                     ? qsTr("“%1” is not a link: links start with http://, rtsp://… or a site's address.").arg(root.notLinks[0])
                     : qsTr("You can enter multiple links to load.")
    }

    Widgets.RedesignTextArea {
        id: urlField

        objectName: "networkStreamUrlField"
        multiLine: true
        placeholderText: qsTr("e.g., https://www.example.com/stream.avi")
        Accessible.name: qsTr("Network URL")
        onAccepted: root.accept()
    }

    // "Vector": 1 px of "Dark/Divider/Dim" (#222222) centred between the
    // parts, which puts it half on two lines of pixels
    Item {
        Layout.fillWidth: true
        implicitHeight: 0
        Rectangle {
            y: -VLCStyle.dp(1, VLCStyle.scale)
            width: parent.width
            height: VLCStyle.dp(2, VLCStyle.scale)
            color: root.darkTheme ? Qt.rgba(34 / 255, 34 / 255, 34 / 255, 0.5) : Qt.rgba(0.61, 0.61, 0.61, 0.5)
        }
    }

    Widgets.RedesignDialogHeading {
        text: qsTr("Playlist Name")
        suffix: qsTr("(optional)")
        description: root.canSavePlaylist ? qsTr("The playlist will be saved and displayed with this title.")
                                          : qsTr("Playlists need the media library.")
    }

    Widgets.RedesignTextArea {
        id: nameField

        objectName: "networkStreamPlaylistField"
        enabled: root.canSavePlaylist
        placeholderText: qsTr("e.g., Favorite TV Channels")
        Accessible.name: qsTr("Playlist name")
        onAccepted: root.accept()
    }
}
