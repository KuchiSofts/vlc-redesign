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
import QtQuick.Controls.impl as Impl

import VLC.Dialogs
import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Player
import VLC.Playlist
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// Card of the player over the video (Figma "Controls": queue, bookmarks,
// chapters): 400px wide, #191919 at 95%, tabs over the play queue, the
// bookmarks and the chapters of the media.
T.Pane {
    id: root

    property int currentTab: 0

    // The play queue page (drag and drop, selection, context menu)
    readonly property alias queue: queuePane

    readonly property bool darkTheme: VLCStyle.palette.isDark

    padding: 0

    implicitWidth: VLCStyle.dp(400, VLCStyle.scale)

    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Play queue, bookmarks and chapters")

    background: Rectangle {
        radius: VLCStyle.dp(6, VLCStyle.scale)
        color: root.darkTheme ? "#F2191919" : "#F2ABABAA"
        // the frame of the main window
        border.width: 1
        border.color: VLCStyle.redesign.color(VLCStyle.redesign.windowBorder, undefined, root.darkTheme)
    }

    contentItem: ColumnLayout {
        spacing: 0

        // Tabs, on a darker band
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: VLCStyle.dp(50, VLCStyle.scale)
            topLeftRadius: VLCStyle.dp(6, VLCStyle.scale)
            topRightRadius: topLeftRadius
            color: root.darkTheme ? "#66000000" : "#1A000000"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: VLCStyle.dp(6, VLCStyle.scale)
                anchors.rightMargin: VLCStyle.dp(6, VLCStyle.scale)
                spacing: 0

                CardTab { index: 0; iconName: "queue"; text: qsTr("Queue") }
                CardTab { index: 1; iconName: "bookmarks"; text: qsTr("Bookmarks") }
                CardTab { index: 2; iconName: "chapters"; text: qsTr("Chapters") }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: VLCStyle.dp(10, VLCStyle.scale)
            currentIndex: root.currentTab

            PlaylistPane {
                id: queuePane

                card: true
                useAcrylic: false
                focus: root.currentTab === 0
                topPadding: 0
                bottomPadding: VLCStyle.dp(8, VLCStyle.scale)
                leftPadding: VLCStyle.dp(6, VLCStyle.scale)
                rightPadding: VLCStyle.dp(6, VLCStyle.scale)

                onAddRequested: DialogsProvider.simpleOpenDialog(false)
            }

            // Bookmarks of the media, in the media library (empty and read
            // only without it)
            ColumnLayout {
                spacing: VLCStyle.dp(4, VLCStyle.scale)

                MLBookmarkModel {
                    id: bookmarks

                    player: Player
                    ml: MainCtx.mediaLibraryAvailable ? MediaLib : null
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: VLCStyle.dp(18, VLCStyle.scale)
                    Layout.rightMargin: VLCStyle.dp(12, VLCStyle.scale)
                    spacing: VLCStyle.dp(4, VLCStyle.scale)

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("%1 Bookmark", "%1 Bookmarks", bookmarkList.count).arg(bookmarkList.count)
                        elide: Text.ElideRight
                        // (from the right in a right to left layout)
                        horizontalAlignment: Text.AlignLeft
                        color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    }

                    CardButton {
                        iconName: "remove"
                        description: qsTr("Remove the bookmark")
                        enabled: MainCtx.mediaLibraryAvailable && bookmarkList.currentIndex >= 0
                                 && bookmarkList.count > 0
                        onClicked: bookmarks.removeAt(bookmarkList.currentIndex)
                    }

                    CardButton {
                        iconName: "add"
                        description: qsTr("Add a bookmark at the current time")
                        enabled: MainCtx.mediaLibraryAvailable
                                 && (Player.playingState === Player.PLAYING_STATE_PLAYING
                                     || Player.playingState === Player.PLAYING_STATE_PAUSED)
                        onClicked: bookmarks.add()
                    }
                }

                ListView {
                    id: bookmarkList

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: VLCStyle.dp(6, VLCStyle.scale)
                    Layout.rightMargin: VLCStyle.dp(6, VLCStyle.scale)
                    clip: true
                    model: bookmarks
                    currentIndex: -1
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: CardRow {
                        id: bookmarkRow

                        required property int index
                        required property string name
                        required property var milliseconds

                        width: ListView.view.width
                        highlighted: ListView.isCurrentItem
                        label: name
                        detail: {
                            // 00:00:42:023
                            const ms = Number(milliseconds)
                            const pad = (value, size) => String(Math.floor(value)).padStart(size, "0")
                            return pad(ms / 3600000, 2) + ":" + pad(ms / 60000 % 60, 2) + ":"
                                   + pad(ms / 1000 % 60, 2) + ":" + pad(ms % 1000, 3)
                        }
                        onClicked: {
                            bookmarkList.currentIndex = index
                            bookmarks.select(bookmarks.index(index, 0))
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: bookmarkList.count === 0
                        width: parent.width - VLCStyle.dp(40, VLCStyle.scale)
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: MainCtx.mediaLibraryAvailable ? qsTr("No bookmarks: add one at the current time with +")
                                                            : qsTr("Bookmarks need the media library")
                        color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    }
                }
            }

            // Chapters of the media: number, name, start
            ListView {
                id: chapterList

                clip: true
                leftMargin: VLCStyle.dp(6, VLCStyle.scale)
                rightMargin: VLCStyle.dp(6, VLCStyle.scale)
                model: Player.chapters
                boundsBehavior: Flickable.StopAtBounds

                delegate: CardRow {
                    required property int index
                    // "checked" is a role and a FINAL property of buttons
                    required property var model

                    width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
                    number: index + 1
                    current: !!model.checked
                    label: model.display ? model.display : qsTr("Chapter %1").arg(index + 1)
                    detail: model.time ? model.time.formatHMS(VLCTick.ForceHours) : ""
                    onClicked: Player.chapters.selectChapter(index)
                }

                Text {
                    anchors.centerIn: parent
                    visible: chapterList.count === 0
                    // a longer text (a translation) on more lines
                    width: parent.width - VLCStyle.dp(40, VLCStyle.scale)
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("No chapters in this media")
                    color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                }
            }
        }
    }

    // Icon, label and the accent line of the current tab
    component CardTab: T.AbstractButton {
        id: tab

        required property int index
        property string iconName
        readonly property bool current: root.currentTab === index

        Layout.fillWidth: true
        Layout.fillHeight: true

        Accessible.role: Accessible.PageTab
        Accessible.name: text

        onClicked: root.currentTab = index

        contentItem: Item {
            Row {
                anchors.centerIn: parent
                spacing: VLCStyle.dp(8, VLCStyle.scale)

                Impl.IconImage {
                    anchors.verticalCenter: parent.verticalCenter
                    width: VLCStyle.dp(18, VLCStyle.scale)
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: "qrc:///redesign/icons/" + tab.iconName + "/default.svg"
                    color: tab.current ? VLCStyle.redesign.accent
                                       : VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    // within the tab (a longer name, a translation, elided)
                    width: Math.min(implicitWidth, tab.width - VLCStyle.dp(18 + 8 + 8, VLCStyle.scale))
                    elide: Text.ElideRight
                    text: tab.text
                    color: tab.current || tab.hovered ? VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
                                                      : VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - VLCStyle.dp(16, VLCStyle.scale)
                height: VLCStyle.dp(2, VLCStyle.scale)
                color: VLCStyle.redesign.accent
                visible: tab.current
            }
        }
    }

    // Row of the bookmarks and chapters: [number] label ... detail
    component CardRow: T.ItemDelegate {
        id: row

        property int number: 0
        property bool current: false
        property string label
        property string detail

        implicitHeight: VLCStyle.dp(44, VLCStyle.scale)
        leftPadding: VLCStyle.dp(12, VLCStyle.scale)
        rightPadding: VLCStyle.dp(12, VLCStyle.scale)

        Accessible.name: label

        background: Rectangle {
            radius: VLCStyle.redesign.controlRadius
            color: (row.hovered || row.highlighted) ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                                                    : "transparent"
        }

        contentItem: RowLayout {
            spacing: VLCStyle.dp(14, VLCStyle.scale)

            Text {
                visible: row.number > 0
                Layout.preferredWidth: VLCStyle.dp(24, VLCStyle.scale)
                text: row.number
                color: VLCStyle.redesign.accent
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            }
            Text {
                Layout.fillWidth: true
                text: row.label
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                // The current chapter stands out, as in the design
                color: row.current || row.number === 0 ? VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
                                                       : VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(15, VLCStyle.scale)
            }
            Text {
                text: row.detail
                color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            }
        }
    }

    component CardButton: Widgets.IconToolButton {
        id: cardButton

        property string iconName

        text: description
        implicitWidth: VLCStyle.dp(32, VLCStyle.scale)
        implicitHeight: VLCStyle.dp(32, VLCStyle.scale)

        contentItem: Image {
            source: "qrc:///redesign/icons/" + cardButton.iconName + "/default.svg"
            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
            fillMode: Image.Pad
            opacity: cardButton.enabled ? 1 : 0.4
        }
        background: Rectangle {
            radius: VLCStyle.redesign.controlRadius
            color: (cardButton.hovered || cardButton.down) ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                                                           : "transparent"
        }
    }
}
