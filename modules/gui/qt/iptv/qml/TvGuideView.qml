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

import VLC.MainInterface
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// The programme guide: the channels of the TV page (its group and search)
// down, the time across, the line of now; the programmes as cells of the
// controls' grey, the one on now brighter, the archived ones marked. A
// channel's programmes are asked of the add-on six hours at a time, for
// the hours in sight only: a guide of 20 000 channels scrolls as one of 20.
// The time goes from left to right in every language, as the seek bar.
FocusScope {
    id: root

    signal programmeClicked(var channel, var programme)

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(dark, light) { return VLCStyle.redesign.color(dark, light) }

    LayoutMirroring.enabled: false
    LayoutMirroring.childrenInherit: true

    readonly property real channelWidth: px(220)
    readonly property real rowHeight: px(56)
    readonly property real gridWidth: Math.max(px(200), width - channelWidth - px(20))
    // Settings > TV: the hours across the guide
    readonly property real pxPerSec: gridWidth / (Iptv.guideHours * 3600)
    // two days back (the archives), a week ahead
    readonly property real origin: {
        const d = new Date()
        d.setHours(0, 0, 0, 0)
        return Math.floor(d.getTime() / 1000) - 2 * 86400
    }
    readonly property real span: 9 * 86400
    readonly property real blockSecs: 6 * 3600
    // the time shown at the left of the grid
    readonly property real timeX: timeline.contentX
    readonly property real leftTime: origin + timeX / pxPerSec
    readonly property int firstBlock: Math.max(0, Math.floor(timeX / (blockSecs * pxPerSec)) - 1)
    readonly property int lastBlock: Math.floor((timeX + gridWidth) / (blockSecs * pxPerSec)) + 1

    function scrollTo(time, animate) {
        const x = Math.max(0, Math.min(timeline.contentWidth - timeline.width, (time - origin) * pxPerSec))
        if (animate) {
            scrollAnimation.to = x
            scrollAnimation.restart()
        } else {
            timeline.contentX = x
        }
    }
    function jumpToNow() { scrollTo(Iptv.now - Iptv.guideHours * 3600 / 4, true) }
    function showChannel(key) {
        const i = Iptv.channelModel.indexOfKey(key)
        if (i >= 0) {
            rows.currentIndex = i
            rows.positionViewAtIndex(i, ListView.Center)
        }
    }

    Component.onCompleted: Qt.callLater(() => root.scrollTo(Iptv.now - Iptv.guideHours * 3600 / 4, false))
    onPxPerSecChanged: Qt.callLater(() => root.scrollTo(root.leftTime, false))

    NumberAnimation {
        id: scrollAnimation
        target: timeline
        property: "contentX"
        duration: VLCStyle.duration_short
        easing.type: Easing.OutCubic
    }

    // keys: Up and Down the channels, Left and Right half an hour
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Left || event.key === Qt.Key_Right) {
            root.scrollTo(root.leftTime + (event.key === Qt.Key_Right ? 1800 : -1800), true)
            event.accepted = true
        } else if (event.key === Qt.Key_Home) {
            root.jumpToNow()
            event.accepted = true
        }
    }

    // the day and the way through the days, Now
    Row {
        id: toolbar
        x: root.px(20)
        height: root.px(34)
        spacing: root.px(10)

        Widgets.RedesignButton {
            text: qsTr("Now")
            accent: true
            onClicked: root.jumpToNow()
        }
        Widgets.RedesignButton {
            text: qsTr("Earlier")
            implicitWidth: root.px(90)
            onClicked: root.scrollTo(root.leftTime - Iptv.guideHours * 3600, true)
        }
        Widgets.RedesignButton {
            text: qsTr("Later")
            implicitWidth: root.px(90)
            onClicked: root.scrollTo(root.leftTime + Iptv.guideHours * 3600, true)
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            leftPadding: root.px(8)
            text: Iptv.dayText(root.leftTime + Iptv.guideHours * 1800)
            color: root.c("#969696", "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(16)
            font.weight: Font.DemiBold
        }
    }

    // the hours, every half hour; dragging them moves the time
    Flickable {
        id: timeline
        anchors.left: parent.left
        anchors.leftMargin: root.channelWidth
        anchors.right: parent.right
        anchors.rightMargin: root.px(20)
        anchors.top: toolbar.bottom
        anchors.topMargin: root.px(12)
        height: root.px(28)
        contentWidth: root.span * root.pxPerSec
        contentHeight: height
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Repeater {
            // the half hours in sight
            model: Math.ceil(root.gridWidth / (1800 * root.pxPerSec)) + 2
            Item {
                required property int index
                readonly property real time: Math.floor(root.leftTime / 1800) * 1800 + index * 1800
                x: (time - root.origin) * root.pxPerSec
                width: 1800 * root.pxPerSec
                height: timeline.height
                Rectangle {
                    width: 1
                    height: root.px(8)
                    anchors.bottom: parent.bottom
                    color: root.c("#373737", "#8e8e8d")
                }
                Text {
                    x: root.px(6)
                    anchors.verticalCenter: parent.verticalCenter
                    text: Iptv.timeText(parent.time)
                    color: root.c("#6B6B6B", "#5a5a59")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(12)
                }
            }
        }
    }

    // the line under the hours (Dark/Divider/Dim)
    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: root.px(20)
        anchors.right: parent.right
        anchors.rightMargin: root.px(20)
        anchors.top: timeline.bottom
        height: 1
        color: root.c("#222222", "#9c9c9b")
    }

    ListView {
        id: rows
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: timeline.bottom
        anchors.topMargin: root.px(6)
        anchors.bottom: hScroll.top
        model: Iptv.channelModel
        clip: true
        focus: true
        spacing: root.px(2)
        cacheBuffer: root.rowHeight * 4
        boundsBehavior: Flickable.StopAtBounds
        keyNavigationEnabled: true
        highlightMoveDuration: 0
        T.ScrollBar.vertical: Widgets.ScrollBarExt { }

        Keys.onReturnPressed: if (currentIndex >= 0) Iptv.play(Iptv.channelModel.keyAt(currentIndex))

        // a wheel across (or Shift and the wheel) moves the time
        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedModifiers: Qt.ShiftModifier
            onWheel: (event) => {
                timeline.contentX = Math.max(0, Math.min(timeline.contentWidth - timeline.width,
                                                         timeline.contentX - event.angleDelta.y))
            }
        }
        WheelHandler {
            acceptedDevices: PointerDevice.TouchPad
            orientation: Qt.Horizontal
            onWheel: (event) => {
                timeline.contentX = Math.max(0, Math.min(timeline.contentWidth - timeline.width,
                                                         timeline.contentX - event.angleDelta.x))
            }
        }

        delegate: FocusScope {
            id: row
            required property var model
            required property int index
            readonly property string key: model.key
            width: ListView.view.width - root.px(20)
            x: root.px(20)
            height: root.rowHeight

            // the programmes of the blocks asked, by start
            property var programmes: []
            property var _blocks: ({})
            property var _shown: []

            function load() {
                const wanted = []
                for (let b = root.firstBlock; b <= root.lastBlock; ++b)
                    if (!_blocks[b])
                        wanted.push(b)
                if (wanted.length === 0) {
                    refreshShown()
                    return
                }
                const from = root.origin + wanted[0] * root.blockSecs
                const to = root.origin + (wanted[wanted.length - 1] + 1) * root.blockSecs
                for (const b of wanted)
                    _blocks[b] = true
                const key = row.key
                Iptv.programmes([key], from, to, (result) => {
                    if (row.key !== key)
                        return
                    const list = result[key] || []
                    const known = {}
                    for (const p of row.programmes)
                        known[p.start] = true
                    const merged = row.programmes.concat(list.filter(p => !known[p.start]))
                    merged.sort((a, b) => a.start - b.start)
                    row.programmes = merged
                    row.refreshShown()
                })
            }
            function refreshShown() {
                const from = root.origin + root.firstBlock * root.blockSecs
                const to = root.origin + (root.lastBlock + 1) * root.blockSecs
                _shown = programmes.filter(p => p.stop > from && p.start < to)
            }
            onKeyChanged: {
                programmes = []
                _blocks = {}
                _shown = []
                if (model.hasGuide)
                    Qt.callLater(load)
            }
            Component.onCompleted: if (model.hasGuide) load()
            Connections {
                target: root
                function onFirstBlockChanged() { if (row.model.hasGuide) row.load() }
                function onLastBlockChanged() { if (row.model.hasGuide) row.load() }
            }
            Connections {
                target: Iptv
                function onGuideRevisionChanged() {
                    row.programmes = []
                    row._blocks = {}
                    if (row.model.hasGuide)
                        row.load()
                }
            }

            // the channel: its number, its logo, its name (a click plays it)
            T.ItemDelegate {
                id: channelCell
                width: root.channelWidth - root.px(28)
                height: parent.height
                padding: 0
                focusPolicy: Qt.NoFocus
                Accessible.name: row.model.name
                onClicked: Iptv.play(row.model.key)
                background: Rectangle {
                    radius: root.px(6)
                    color: row.model.current ? VLCStyle.redesign.color(VLCStyle.redesign.accentBackground, "")
                           : (channelCell.hovered || (rows.currentIndex === row.index && rows.activeFocus))
                             ? root.c("#151515", "#b5b5b4") : "transparent"
                    border.width: rows.currentIndex === row.index && rows.activeFocus ? 1 : 0
                    border.color: VLCStyle.redesign.focusColor
                }
                contentItem: Item {
                    Text {
                        id: number
                        x: root.px(4)
                        width: root.px(32)
                        anchors.verticalCenter: parent.verticalCenter
                        horizontalAlignment: Text.AlignRight
                        text: row.model.number > 0 ? row.model.number : ""
                        color: root.c("#6B6B6B", "#5a5a59")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(13)
                    }
                    TvLogo {
                        id: logo
                        anchors.left: number.right
                        anchors.leftMargin: root.px(10)
                        anchors.verticalCenter: parent.verticalCenter
                        width: root.px(36)
                        height: width
                        source: row.model.logo
                        name: row.model.name
                    }
                    Text {
                        anchors.left: logo.right
                        anchors.leftMargin: root.px(10)
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: row.model.name
                        elide: Text.ElideRight
                        color: channelCell.hovered || row.model.current ? VLCStyle.redesign.accent
                                                                        : root.c(VLCStyle.redesign.textSecondary, "#4d4d4d")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(13)
                    }
                }
            }

            // the programmes, moved with the time
            Item {
                x: root.channelWidth - root.px(20)
                width: root.gridWidth
                height: parent.height
                clip: true

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    x: root.px(12)
                    visible: !row.model.hasGuide
                    text: qsTr("No guide for this channel")
                    color: root.c("#404040", "#7c7c7b")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(12)
                }

                Item {
                    x: -root.timeX
                    height: parent.height

                    Repeater {
                        model: row._shown
                        delegate: T.AbstractButton {
                            id: cell
                            required property var modelData
                            readonly property bool onNow: modelData.start <= Iptv.now && Iptv.now < modelData.stop
                            readonly property bool past: modelData.stop <= Iptv.now
                            x: (modelData.start - root.origin) * root.pxPerSec + 1
                            width: Math.max(root.px(4), (modelData.stop - modelData.start) * root.pxPerSec - 2)
                            height: row.height
                            focusPolicy: Qt.NoFocus
                            Accessible.name: modelData.title + ", " + Iptv.timeText(modelData.start)
                            onClicked: root.programmeClicked(Iptv.channelModel.get(row.index), modelData)
                            onDoubleClicked: {
                                if (onNow)
                                    Iptv.play(row.model.key)
                                else if (past && modelData.archived)
                                    Iptv.playCatchup(row.model.key, modelData.start, modelData.stop, modelData.title)
                            }

                            background: Rectangle {
                                radius: root.px(6)
                                color: cell.hovered ? root.c("#222222", "#9c9c9b")
                                       : cell.onNow ? root.c("#1F1F1F", "#a3a3a2")
                                       : root.c(cell.past ? "#121212" : "#191919", cell.past ? "#bdbdbc" : "#ababaa")
                            }
                            // the progress of the one on now (its start to now)
                            Rectangle {
                                visible: cell.onNow && Iptv.showProgress
                                anchors.left: parent.left
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: root.px(6)
                                height: root.px(2)
                                width: Math.max(0, (Iptv.now - cell.modelData.start) * root.pxPerSec - root.px(6))
                                color: VLCStyle.redesign.accent
                            }
                            contentItem: Item {
                                clip: true
                                // the text stays in sight while its start is
                                // left of the grid
                                readonly property real inset: Math.max(0, root.timeX - cell.x)
                                Column {
                                    x: parent.inset + root.px(10)
                                    width: Math.max(0, parent.width - x - root.px(8))
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: root.px(3)
                                    Text {
                                        width: parent.width
                                        text: cell.modelData.title
                                        elide: Text.ElideRight
                                        horizontalAlignment: Text.AlignLeft
                                        color: cell.onNow ? root.c(VLCStyle.redesign.textPrimary, "#191919")
                                               : root.c(cell.past ? "#6B6B6B" : VLCStyle.redesign.textSecondary,
                                                        cell.past ? "#5a5a59" : "#4d4d4d")
                                        font.family: VLCStyle.redesign.fontFamily
                                        font.pixelSize: root.px(13)
                                        font.weight: cell.onNow ? Font.Medium : Font.Normal
                                    }
                                    Row {
                                        spacing: root.px(6)
                                        Text {
                                            text: Iptv.timeText(cell.modelData.start)
                                                  + (cell.modelData.episode ? "  ·  " + cell.modelData.episode : "")
                                            color: root.c("#6B6B6B", "#5a5a59")
                                            font.family: VLCStyle.redesign.fontFamily
                                            font.pixelSize: root.px(11)
                                        }
                                        Image {
                                            visible: cell.past && cell.modelData.archived
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: root.px(13)
                                            height: width
                                            sourceSize: Qt.size(width, height)
                                            source: visible ? SVGColorImage.colorize("qrc:///redesign/duo/recent.svg")
                                                                         .color1(root.c("#8F8F8F", "#5A5A59"))
                                                                         .color2(VLCStyle.redesign.accent).uri()
                                                            : ""
                                        }
                                        Image {
                                            visible: Iptv.hasReminder(row.model.key, cell.modelData.start)
                                                     && Iptv.reminders.length >= 0
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: root.px(13)
                                            height: width
                                            sourceSize: Qt.size(width, height)
                                            source: visible ? SVGColorImage.colorize("qrc:///redesign/duo/info-small.svg")
                                                                         .color1(VLCStyle.redesign.accent).uri()
                                                            : ""
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // now: a line of the accent across the rows
    Rectangle {
        readonly property real xNow: root.channelWidth + (Iptv.now - root.origin) * root.pxPerSec - root.timeX
        visible: xNow >= root.channelWidth && xNow <= root.channelWidth + root.gridWidth
        x: xNow - width / 2
        y: timeline.y + timeline.height - root.px(6)
        width: root.px(2)
        height: rows.y + rows.height - y
        color: VLCStyle.redesign.accent
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.px(8)
            height: width
            radius: width / 2
            color: VLCStyle.redesign.accent
        }
    }

    // the time across, under the grid
    Widgets.ScrollBarExt {
        id: hScroll
        anchors.left: timeline.left
        anchors.right: timeline.right
        anchors.bottom: parent.bottom
        orientation: Qt.Horizontal
        size: timeline.width / timeline.contentWidth
        position: timeline.contentX / timeline.contentWidth
        onPositionChanged: if (pressed) timeline.contentX = position * timeline.contentWidth
        policy: T.ScrollBar.AlwaysOn
    }

    Text {
        anchors.centerIn: rows
        visible: Iptv.channelModel.count === 0
        text: qsTr("No channel matches the search")
        color: root.c("#6B6B6B", "#4d4d4d")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.redesign.fontMedium
    }
}
