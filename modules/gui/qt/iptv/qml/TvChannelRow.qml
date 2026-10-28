/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl

import VLC.MainInterface
import VLC.Style
import VLC.Util

// A channel of Live TV (the TV page, the list over the video): its number,
// its logo, its name, what is on with the progress of it and what is next;
// the archive (catch-up) and the favourite as small marks. A row of the
// library's lists (the media rows of a folder: names Inter 14 #969696, the
// one under the pointer in the accent), 56 high for its two lines.
T.ItemDelegate {
    id: root

    required property var model
    required property int index
    // the list over the video: narrower, no "next" column
    property bool compact: false
    // the colours of the dark theme whatever the theme (over the video)
    property bool forceDark: false
    readonly property bool _dark: forceDark || VLCStyle.palette.isDark

    signal play()
    signal menu(point globalPos)

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(dark, light) { return VLCStyle.redesign.color(dark, light, root._dark) }

    readonly property bool _hasNow: (model.nowTitle ?? "") !== ""
    readonly property real _progress: {
        const start = model.nowStart ?? 0, stop = model.nowStop ?? 0
        if (!_hasNow || stop <= start)
            return 0
        return Math.max(0, Math.min(1, (Iptv.now - start) / (stop - start)))
    }

    implicitHeight: px(56)
    padding: 0
    focusPolicy: Qt.StrongFocus
    Accessible.name: (model.number > 0 ? model.number + " " : "") + model.name
                     + (_hasNow ? ", " + model.nowTitle : "")

    onClicked: root.play()
    Keys.onReturnPressed: root.play()
    Keys.onEnterPressed: root.play()

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (eventPoint) => root.menu(root.mapToGlobal(eventPoint.position))
    }

    // Figma's hovered rows: Dark/BG/A5 (#151515); the channel playing in the
    // accent's dark background
    background: Rectangle {
        radius: root.px(6)
        color: root.model.current ? (root._dark ? VLCStyle.redesign.accentBackground
                                                : VLCStyle.redesign.color(VLCStyle.redesign.accentBackground, "", false))
               : (root.hovered || root.visualFocus) ? root.c("#151515", "#b5b5b4")
               : "transparent"
        border.width: root.visualFocus ? 1 : 0
        border.color: VLCStyle.redesign.focusColor
    }

    contentItem: Item {
        // the number, right-aligned in its column (at the start of the row)
        Text {
            id: number
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: root.px(root.compact ? 34 : 44)
            horizontalAlignment: Text.AlignRight
            text: root.model.number > 0 ? root.model.number : ""
            color: root.c("#6B6B6B", "#5a5a59")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
            font.features: { "tnum": 1 }
        }

        TvLogo {
            id: logo
            anchors.left: number.right
            anchors.leftMargin: root.px(14)
            anchors.verticalCenter: parent.verticalCenter
            width: root.px(40)
            height: width
            source: root.model.logo ?? ""
            name: root.model.name ?? ""
            forceDark: root.forceDark
        }

        // the name and what is on, from the logo to the next programme
        Column {
            anchors.left: logo.right
            anchors.leftMargin: root.px(12)
            anchors.right: nextColumn.visible ? nextColumn.left : marks.left
            anchors.rightMargin: root.px(16)
            anchors.verticalCenter: parent.verticalCenter
            spacing: root.px(4)

            Text {
                width: parent.width
                text: root.model.name ?? ""
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                color: root.hovered ? VLCStyle.redesign.accent
                                    : root.model.current ? VLCStyle.redesign.accent
                                    : root.c(VLCStyle.redesign.textSecondary, "#4d4d4d")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
            }

            Row {
                width: parent.width
                spacing: root.px(8)
                visible: root._hasNow || !root.model.hasGuide

                Text {
                    width: Math.min(implicitWidth, parent.width - (bar.visible ? bar.width + parent.spacing : 0))
                    anchors.verticalCenter: parent.verticalCenter
                    text: root._hasNow ? root.model.nowTitle
                                       : (root.model.hasGuide ? "" : qsTr("No guide"))
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    color: root.c("#6B6B6B", "#5a5a59")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(12)
                }

                // the progress of what is on (a time axis: from the left in
                // every language, as the seek bar)
                Rectangle {
                    id: bar
                    LayoutMirroring.enabled: false
                    visible: Iptv.showProgress && root._hasNow
                    anchors.verticalCenter: parent.verticalCenter
                    width: root.px(root.compact ? 48 : 72)
                    height: root.px(3)
                    radius: height / 2
                    color: root.c("#222222", "#9c9c9b")
                    Rectangle {
                        width: Math.round(parent.width * root._progress)
                        height: parent.height
                        radius: parent.radius
                        color: VLCStyle.redesign.accent
                    }
                }
            }

            // the description of what is on (Settings > TV), one line
            Text {
                width: parent.width
                visible: !root.compact && Iptv.showDescriptions && text !== ""
                text: (root.model.nowDescription ?? "").replace(/\s+/g, " ")
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                color: root.c("#404040", "#7c7c7b")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(12)
            }
        }

        // what is next: its time and title
        Column {
            id: nextColumn
            visible: !root.compact && Iptv.showNext && (root.model.nextTitle ?? "") !== ""
            anchors.right: marks.left
            anchors.rightMargin: root.px(16)
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(root.px(260), root.width * 0.3)
            spacing: root.px(4)

            Text {
                width: parent.width
                text: qsTr("Next at %1").arg(Iptv.timeText(root.model.nextStart ?? 0))
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
                color: root.c("#404040", "#7c7c7b")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(12)
            }
            Text {
                width: parent.width
                text: root.model.nextTitle ?? ""
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
                color: root.c("#6B6B6B", "#5a5a59")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(12)
            }
        }

        // the archive and the favourite, at the end of the row
        Row {
            id: marks
            anchors.right: parent.right
            anchors.rightMargin: root.px(12)
            anchors.verticalCenter: parent.verticalCenter
            spacing: root.px(8)

            TvBadge {
                visible: (root.model.catchupDays ?? 0) > 0
                anchors.verticalCenter: parent.verticalCenter
                icon: "recent"
                text: qsTr("%1 d", "%1 d", root.model.catchupDays ?? 0).arg(root.model.catchupDays ?? 0)
                tip: qsTr("Catch-up: the past %1 day", "Catch-up: the past %1 days", root.model.catchupDays ?? 0)
                         .arg(root.model.catchupDays ?? 0)
                forceDark: root.forceDark
            }

            Image {
                visible: root.model.favorite ?? false
                anchors.verticalCenter: parent.verticalCenter
                width: root.px(18)
                height: width
                sourceSize: Qt.size(width, height)
                source: SVGColorImage.colorize("qrc:///redesign/duo/favorite.svg")
                                     .color2(VLCStyle.redesign.accent).uri()
            }
        }
    }
}
