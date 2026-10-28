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
import VLC.Dialogs
import VLC.Style
import VLC.Widgets as Widgets

// Folder information (Info of the folder context menu): where the folder is,
// what it holds (counted with its subfolders), its size
Widgets.RedesignDialog {
    id: root

    objectName: "folderInfoDialog"

    property alias folderUrl: info.url

    title: qsTr("Folder information")
    acceptText: qsTr("Close")
    cancelVisible: false
    extraText: info.exists ? qsTr("Show in File Explorer") : ""
    onExtraClicked: MainCtx.showInFileManager(String(info.url))
    preferredWidth: VLCStyle.dp(713, VLCStyle.scale)

    FolderInformation {
        id: info
    }

    function _count(value) {
        if (info.busy)
            return qsTr("Counting…")
        const text = Number(value).toLocaleString(Qt.locale(), "f", 0)
        // stopped at its limit: at least so many
        return info.partial ? qsTr("%1 or more").arg(text) : text
    }

    Widgets.RedesignDialogHeading {
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        headingWraps: true
        text: root._plain(info.name)
        description: info.exists ? "" : qsTr("This folder cannot be found.")
    }

    function _plain(text) {
        return (text || "").replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0
        visible: info.exists

        Widgets.RedesignInfoRow { label: qsTr("Location"); value: info.path; wraps: true }
        Widgets.RedesignInfoRow { label: qsTr("Modified"); value: info.modified }
        Widgets.RedesignInfoRow { label: qsTr("Size"); value: info.busy ? qsTr("Counting…") : info.size }
        Widgets.RedesignInfoRow { label: qsTr("Files"); value: root._count(info.files) }
        Widgets.RedesignInfoRow { label: qsTr("Videos"); value: root._count(info.videos) }
        Widgets.RedesignInfoRow { label: qsTr("Music"); value: root._count(info.audios) }
        Widgets.RedesignInfoRow { label: qsTr("Subtitles"); value: root._count(info.subtitles) }
        Widgets.RedesignInfoRow { label: qsTr("Folders"); value: root._count(info.folders); divider: false }
    }
}
