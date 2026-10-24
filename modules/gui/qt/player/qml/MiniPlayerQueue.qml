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
import QtQuick.Controls.impl as Impl

import VLC.MainInterface
import VLC.Playlist
import VLC.Style
import VLC.Widgets as Widgets

// Figma 970:3091 (the queue open): the queue built into the top of the mini
// player's card, as wide as the card (laid out from the card's corner)
Item {
    id: root
    objectName: "miniPlayerQueue"
    // the card's top to 8 above the video (the video at 359 under the top)
    implicitHeight: VLCStyle.dp(351, VLCStyle.scale)
    readonly property bool lightTheme: !VLCStyle.palette.isDark
    // the border of the card (light theme), left visible around the header
    property real edge: 0
    // the radius of the card, the header's top corners
    property real radius: VLCStyle.dp(14, VLCStyle.scale)
    // the row picked by a click ("−" removes it); a double click plays a row
    property int selectedIndex: -1

    // Right to left (Hebrew, Arabic...): the count on the right, the
    // icons on the left, the rows from the right
    readonly property bool _mirrored: LayoutMirroring.enabled

    // Text of the design (Inter: a line of 1.2109 em) centred in a box of
    // `box` px: its top padding
    function _textTop(box, pixelSize, ascent) {
        return VLCStyle.redesign.textBaseline(Math.round((box - Math.round(1.2109375 * pixelSize)) / 2),
                                              pixelSize) - ascent
    }

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

    // Figma: the "Queue" header, the width of the card, 58 high, #151515,
    // the card's top corners (the queue below it on the card's #1B1B1B)
    Rectangle {
        id: header
        x: root.edge
        y: root.edge
        width: parent.width - 2 * root.edge
        height: VLCStyle.dp(58, VLCStyle.scale) - root.edge
        color: VLCStyle.redesign.color("#151515", "#b5b5b4")
        topLeftRadius: Math.max(0, root.radius - root.edge)
        topRightRadius: Math.max(0, root.radius - root.edge)
    }
    // Inter 16 #8F8F8F, centred in the header
    Text {
        id: headerText
        width: parent.width
        height: VLCStyle.dp(58, VLCStyle.scale)
        text: qsTr("Queue")
        color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignTop
        topPadding: root._textTop(height, font.pixelSize, headerMetrics.ascent)
        FontMetrics {
            id: headerMetrics
            font: headerText.font
        }
    }

    // Figma: "4 items - 2h 3m" (Inter 14 #6B6B6B, 37 from the card's edge)
    // and the icons (24, 20 apart, 28 from the other edge), on a row of 24
    // at 79
    Text {
        id: countText
        x: root._mirrored ? tools.x + tools.width + VLCStyle.dp(12, VLCStyle.scale)
                          : VLCStyle.dp(37, VLCStyle.scale)
        y: VLCStyle.dp(79, VLCStyle.scale)
        width: root._mirrored ? parent.width - VLCStyle.dp(37, VLCStyle.scale) - x
                              : tools.x - x - VLCStyle.dp(12, VLCStyle.scale)
        height: VLCStyle.dp(24, VLCStyle.scale)
        text: {
            const count = queueModel.count
            if (count === 0)
                return qsTr("Queue is empty")
            const minutes = Math.round(queueModel.duration.toSeconds() / 60)
            const length = minutes >= 60 ? qsTr("%1h %2m").arg(Math.floor(minutes / 60)).arg(minutes % 60)
                                         // xgettext:no-c-format
                                         : qsTr("%1m").arg(minutes)
            return qsTr("%1 item", "%1 items", count).arg(count) + " - " + length
        }
        color: VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.redesign.fontNormal
        verticalAlignment: Text.AlignTop
        topPadding: root._textTop(height, font.pixelSize, countMetrics.ascent)
        FontMetrics {
            id: countMetrics
            font: countText.font
        }
    }
    Row {
        id: tools
        x: root._mirrored ? VLCStyle.dp(28, VLCStyle.scale)
                          : parent.width - width - VLCStyle.dp(28, VLCStyle.scale)
        y: VLCStyle.dp(79, VLCStyle.scale)
        spacing: VLCStyle.dp(20, VLCStyle.scale)
        Widgets.SortControl {
            id: sortControl
            width: VLCStyle.dp(24, VLCStyle.scale)
            height: VLCStyle.dp(24, VLCStyle.scale)
            radius: VLCStyle.dp(4, VLCStyle.scale)
            backgroundColor: VLCStyle.redesign.color(hovered || down ? VLCStyle.redesign.controlHoverBackground
                                                                     : "transparent", "#9c9c9b")
            enabled: queueModel.count > 1
            model: MainPlaylistController.sortKeyTitleList
            sortKey: MainPlaylistController.sortKey
            sortOrder: MainPlaylistController.sortOrder === PlaylistController.SORT_ORDER_ASC
                       ? Qt.AscendingOrder : Qt.DescendingOrder
            // the sort icon of the design (#6B6B6B)
            contentItem: Image {
                source: "qrc:///redesign/icons/sort/dim.png"
                sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                fillMode: Image.Pad
                opacity: sortControl.enabled ? 1 : 0.35
            }
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
        // (Figma: "−" drawn as "+", no row picked: it stays so, unusable)
        QueueButton {
            iconSource: "qrc:///redesign/icons/remove/default.png"
            description: qsTr("Remove selected item from queue")
            enabled: root.selectedIndex >= 0 && root.selectedIndex < queueModel.count
            onClicked: queueModel.removeItems([root.selectedIndex])
        }
        QueueButton {
            iconSource: "qrc:///redesign/icons/add/default.png"
            description: qsTr("Add files to queue")
            onClicked: {
                const selected = filePicker.chooseFiles(qsTr("Add files to queue"))
                if (selected.length > 0)
                    MainPlaylistController.append(selected, false)
            }
        }
    }

    // Figma: rows of 44 from 118, the handle (24) 32 from the card's edge,
    // 14, the name (Inter 14 #969696), the time (Inter 14 #6B6B6B) ending 32
    // from the other edge; room for five rows, 21 above the video
    ListView {
        id: queueList
        objectName: "miniPlayerQueueList"
        // the rows' hover 12 wider than their content on each side
        x: VLCStyle.dp(20, VLCStyle.scale)
        y: VLCStyle.dp(118, VLCStyle.scale)
        width: parent.width - 2 * x
        height: parent.height - y - VLCStyle.dp(13, VLCStyle.scale)
        clip: true
        model: queueModel
        boundsBehavior: Flickable.StopAtBounds
        delegate: Controls.ItemDelegate {
            id: queueRow
            required property int index
            required property string title
            required property var duration
            // moved by its handle: where the pointer took it
            property real dragOffset: 0
            width: queueList.width
            height: VLCStyle.dp(44, VLCStyle.scale)
            leftPadding: VLCStyle.dp(12, VLCStyle.scale)
            rightPadding: VLCStyle.dp(12, VLCStyle.scale)
            topPadding: 0
            bottomPadding: 0
            z: handleArea.pressed ? 1 : 0
            transform: Translate { y: queueRow.dragOffset }
            highlighted: index === root.selectedIndex || handleArea.pressed
            background: Rectangle {
                color: queueRow.hovered || queueRow.highlighted
                       ? (root.lightTheme ? "#b0b0b0" : "#262626") : "transparent"
                radius: VLCStyle.dp(4, VLCStyle.scale)
            }
            contentItem: Item {
                // Figma: the "=" handle (the move icon, #8F8F8F), of every
                // row, the one playing as well: dragged, it moves the row
                Impl.ColorImage {
                    id: handle
                    // (the handle first: on the right in a right to left row)
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: VLCStyle.redesign.iconSize
                    height: width
                    source: "qrc:///redesign/icons/move/default.png"
                    sourceSize: Qt.size(width, height)
                    fillMode: Image.Pad
                    color: root.lightTheme ? "#2e2e2e" : "transparent"

                    MouseArea {
                        id: handleArea
                        property real startY: 0
                        anchors.fill: parent
                        // (the list would take the drag for its scrolling)
                        preventStealing: true
                        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                        onPressed: (mouse) => {
                            startY = mapToItem(queueList.contentItem, mouse.x, mouse.y).y
                        }
                        onPositionChanged: (mouse) => {
                            queueRow.dragOffset = mapToItem(queueList.contentItem, mouse.x, mouse.y).y - startY
                        }
                        onReleased: {
                            const target = Math.max(0, Math.min(queueModel.count - 1,
                                queueRow.index + Math.round(queueRow.dragOffset / queueRow.height)))
                            queueRow.dragOffset = 0
                            if (target !== queueRow.index)
                                queueModel.moveItemsPost([queueRow.index], target)
                        }
                        onCanceled: queueRow.dragOffset = 0
                    }
                }
                Text {
                    id: nameText
                    anchors.left: handle.right
                    anchors.leftMargin: VLCStyle.dp(14, VLCStyle.scale)
                    anchors.right: timeText.left
                    anchors.rightMargin: VLCStyle.dp(16, VLCStyle.scale)
                    height: parent.height
                    text: queueRow.title
                    color: root.lightTheme ? "#4d4d4d" : VLCStyle.redesign.textSecondary
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.redesign.fontNormal
                    verticalAlignment: Text.AlignTop
                    topPadding: root._textTop(height, font.pixelSize, rowMetrics.ascent)
                    FontMetrics {
                        id: rowMetrics
                        font: nameText.font
                    }
                }
                Text {
                    id: timeText
                    anchors.right: parent.right
                    width: implicitWidth
                    height: parent.height
                    text: queueRow.duration.formatHMS()
                    color: VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.redesign.fontNormal
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignTop
                    topPadding: root._textTop(height, font.pixelSize, rowMetrics.ascent)
                }
            }
            Accessible.name: title
            onClicked: root.selectedIndex = index
            onDoubleClicked: MainPlaylistController.goTo(index, true)
        }
    }

    // Icon button of the queue's row (24, the icons of the design in
    // #6B6B6B as the sort's)
    component QueueButton: Widgets.IconToolButton {
        id: queueButton

        property string iconSource

        // Drawn from an image, the text only names the button
        text: description
        width: VLCStyle.dp(24, VLCStyle.scale)
        height: VLCStyle.dp(24, VLCStyle.scale)
        radius: VLCStyle.dp(4, VLCStyle.scale)
        backgroundColor: VLCStyle.redesign.color(hovered || down ? VLCStyle.redesign.controlHoverBackground
                                                                 : "transparent", "#9c9c9b")
        contentItem: Impl.ColorImage {
            source: queueButton.iconSource
            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
            fillMode: Image.Pad
            color: VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
        }
    }
}
