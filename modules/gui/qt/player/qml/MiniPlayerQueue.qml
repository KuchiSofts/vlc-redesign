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

import VLC.MainInterface
import VLC.Playlist
import VLC.Style
import VLC.Widgets as Widgets

Item {
    id: root
    objectName: "miniPlayerQueue"
    implicitHeight: VLCStyle.dp(228, VLCStyle.scale)
    readonly property bool lightTheme: !VLCStyle.palette.isDark
    // the row picked by a click ("−" removes it); a double click plays a row
    property int selectedIndex: -1

    PlaylistListModel {
        id: queueModel
        playlist: MainPlaylistController.playlist
    }
    // the rows move or go away: the selection is dropped
    Connections {
        target: queueModel
        function onModelReset() { root.selectedIndex = -1 }
        function onRowsInserted() { root.selectedIndex = -1 }
        function onRowsRemoved() { root.selectedIndex = -1 }
        function onRowsMoved() { root.selectedIndex = -1 }
    }
    Widgets.FilePicker { id: filePicker; ctx: MainCtx }

    Rectangle {
        anchors.fill: parent
        color: root.lightTheme ? "#c2c2c1" : "#111111"
        radius: VLCStyle.dp(8, VLCStyle.scale)
    }

    Text {
        x: VLCStyle.dp(12, VLCStyle.scale)
        y: VLCStyle.dp(10, VLCStyle.scale)
        width: parent.width - VLCStyle.dp(24, VLCStyle.scale)
        text: qsTr("Queue")
        color: root.lightTheme ? "#4d4d4d" : VLCStyle.redesign.textSecondary
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
        horizontalAlignment: Text.AlignHCenter
    }
    Rectangle {
        x: 0
        y: VLCStyle.dp(37, VLCStyle.scale)
        width: parent.width
        height: VLCStyle.dp(1, VLCStyle.scale)
        color: root.lightTheme ? "#858585" : VLCStyle.redesign.separator
    }
    Text {
        x: VLCStyle.dp(12, VLCStyle.scale)
        y: VLCStyle.dp(48, VLCStyle.scale)
        width: parent.width - VLCStyle.dp(110, VLCStyle.scale)
        text: queueModel.count > 0
              ? qsTr("%1 items · %2").arg(queueModel.count).arg(queueModel.duration.formatLong())
              : qsTr("Queue is empty")
        color: root.lightTheme ? "#7c7c7b" : VLCStyle.redesign.textMuted
        elide: Text.ElideRight
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(11, VLCStyle.scale)
    }
    Row {
        x: parent.width - width - VLCStyle.dp(8, VLCStyle.scale)
        y: VLCStyle.dp(41, VLCStyle.scale)
        spacing: 0
        Widgets.SortControl {
            width: VLCStyle.dp(26, VLCStyle.scale)
            height: VLCStyle.dp(28, VLCStyle.scale)
            enabled: queueModel.count > 1
            model: MainPlaylistController.sortKeyTitleList
            sortKey: MainPlaylistController.sortKey
            sortOrder: MainPlaylistController.sortOrder === PlaylistController.SORT_ORDER_ASC
                       ? Qt.AscendingOrder : Qt.DescendingOrder
            onSortSelected: key => {
                MainPlaylistController.sortKey = key
                MainPlaylistController.sort()
            }
            onSortOrderSelected: order => {
                MainPlaylistController.sortOrder = order === Qt.AscendingOrder
                                                 ? PlaylistController.SORT_ORDER_ASC
                                                 : PlaylistController.SORT_ORDER_DESC
                MainPlaylistController.sort()
            }
        }
        Controls.Button {
            width: VLCStyle.dp(26, VLCStyle.scale)
            height: VLCStyle.dp(28, VLCStyle.scale)
            text: "−"
            enabled: root.selectedIndex >= 0 && root.selectedIndex < queueModel.count
            Accessible.name: qsTr("Remove selected item from queue")
            background: Item {}
            contentItem: Text {
                text: parent.text
                color: parent.enabled
                       ? (root.lightTheme ? "#4d4d4d" : VLCStyle.redesign.textSecondary)
                       : VLCStyle.redesign.textDisabled
                font.pixelSize: VLCStyle.dp(20, VLCStyle.scale)
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: queueModel.removeItems([root.selectedIndex])
        }
        Controls.Button {
            width: VLCStyle.dp(26, VLCStyle.scale)
            height: VLCStyle.dp(28, VLCStyle.scale)
            text: "+"
            Accessible.name: qsTr("Add files to queue")
            background: Item {}
            contentItem: Text {
                text: parent.text
                color: root.lightTheme ? "#4d4d4d" : VLCStyle.redesign.textSecondary
                font.pixelSize: VLCStyle.dp(20, VLCStyle.scale)
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                const selected = filePicker.chooseFiles(qsTr("Add files to queue"))
                if (selected.length > 0)
                    MainPlaylistController.append(selected, false)
            }
        }
    }

    ListView {
        id: queueList
        objectName: "miniPlayerQueueList"
        x: VLCStyle.dp(10, VLCStyle.scale)
        y: VLCStyle.dp(78, VLCStyle.scale)
        width: parent.width - VLCStyle.dp(20, VLCStyle.scale)
        height: parent.height - VLCStyle.dp(86, VLCStyle.scale)
        clip: true
        model: queueModel
        boundsBehavior: Flickable.StopAtBounds
        delegate: Controls.ItemDelegate {
            required property int index
            required property string title
            required property var duration
            required property bool isCurrent
            width: queueList.width
            height: VLCStyle.dp(31, VLCStyle.scale)
            leftPadding: VLCStyle.dp(5, VLCStyle.scale)
            rightPadding: VLCStyle.dp(5, VLCStyle.scale)
            highlighted: index === root.selectedIndex
            background: Rectangle {
                color: parent.hovered || parent.highlighted
                       ? (root.lightTheme ? "#b0b0b0" : "#262626") : "transparent"
                radius: VLCStyle.dp(4, VLCStyle.scale)
            }
            contentItem: Row {
                spacing: VLCStyle.dp(8, VLCStyle.scale)
                Text {
                    width: VLCStyle.dp(14, VLCStyle.scale)
                    height: parent.height
                    text: isCurrent ? "▶" : "="
                    color: isCurrent ? VLCStyle.redesign.accent
                                     : (root.lightTheme ? "#7c7c7b" : VLCStyle.redesign.textMuted)
                    font.pixelSize: VLCStyle.dp(11, VLCStyle.scale)
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    width: parent.width - VLCStyle.dp(72, VLCStyle.scale)
                    height: parent.height
                    text: title
                    color: root.lightTheme ? "#4d4d4d" : VLCStyle.redesign.textSecondary
                    elide: Text.ElideRight
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(11, VLCStyle.scale)
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    width: VLCStyle.dp(43, VLCStyle.scale)
                    height: parent.height
                    text: duration.formatHMS()
                    color: root.lightTheme ? "#7c7c7b" : VLCStyle.redesign.textMuted
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(10, VLCStyle.scale)
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Accessible.name: title
            onClicked: root.selectedIndex = index
            onDoubleClicked: MainPlaylistController.goTo(index, true)
        }
    }
}
