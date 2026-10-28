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
import VLC.Widgets as Widgets

// The films or the series of an Xtream Codes account (the TV page's Films
// and Series): posters in the cards of the library's grid, by the category
// chosen; a series opens on its episodes. They play as any media, not kept
// in the library.
FocusScope {
    id: root

    // "movies" or "series"
    property string kind: "movies"
    property string search: ""

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    // (the light theme from the map of the design: VLCStyle.redesign.color)
    function c(dark, light) { return VLCStyle.redesign.color(dark) }

    readonly property var accounts: Iptv.sources.filter(s => s.kind === "xtream" && s.enabled
                                                        && (kind === "series" ? s.series : s.vod))
    property string account: accounts.length > 0 ? accounts[0].id : ""
    property var items: []
    property bool loading: false
    property string category: ""
    // the series opened ({} for none) and its episodes
    property var series: ({})
    property var episodes: []

    readonly property var categories: items.map(i => i.category).filter((c, i, all) => !!c && all.indexOf(c) === i)
    readonly property var shown: {
        const text = search.trim().toLowerCase()
        return items.filter(i => (category === "" || i.category === category)
                                 && (!i.adult || Iptv.showAdult)
                                 && (text === "" || String(i.name).toLowerCase().indexOf(text) >= 0))
    }

    function load() {
        items = []
        series = {}
        episodes = []
        category = ""
        if (account === "")
            return
        loading = true
        const wanted = account + kind
        Iptv.library(account, kind, (list) => {
            if (root.account + root.kind !== wanted)
                return
            root.items = list
            root.loading = false
        })
    }
    onAccountChanged: load()
    onKindChanged: load()
    Component.onCompleted: load()

    function openSeries(item) {
        series = item
        episodes = []
        Iptv.seriesInfo(account, item.id, (info) => { if (root.series === item) root.episodes = info.episodes || [] })
    }

    // the categories of the account
    ListView {
        id: chips
        visible: root.series.id === undefined
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.px(20)
        anchors.rightMargin: root.px(20)
        height: root.px(30)
        orientation: ListView.Horizontal
        spacing: root.px(8)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: [""].concat(root.categories)
        delegate: T.Button {
            id: chip
            required property var modelData
            readonly property bool selected: root.category === modelData
            height: chips.height
            width: chipText.implicitWidth + root.px(24)
            onClicked: root.category = modelData
            background: Rectangle {
                radius: root.px(6)
                color: chip.selected ? VLCStyle.redesign.color(VLCStyle.redesign.accentBackground)
                                     : chip.hovered ? root.c("#222222", "#9c9c9b") : root.c("#191919", "#ababaa")
            }
            contentItem: Text {
                id: chipText
                text: chip.modelData === "" ? qsTr("All") : chip.modelData
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                color: chip.selected ? VLCStyle.redesign.accent : root.c("#8F8F8F", "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(13)
            }
        }
    }

    GridView {
        id: posters
        visible: root.series.id === undefined
        anchors.fill: parent
        anchors.topMargin: chips.height + root.px(16)
        anchors.leftMargin: root.px(20)
        clip: true
        focus: true
        model: root.shown
        readonly property real cardWidth: root.px(150)
        cellWidth: cardWidth + root.px(20)
        cellHeight: cardWidth * 1.5 + root.px(14 + 19 + 20)
        cacheBuffer: cellHeight * 2
        boundsBehavior: Flickable.StopAtBounds
        T.ScrollBar.vertical: Widgets.ScrollBarExt { }

        delegate: T.ItemDelegate {
            id: card
            required property var modelData
            width: posters.cellWidth
            height: posters.cellHeight
            padding: 0
            background: null
            Accessible.name: modelData.name
            onClicked: root.kind === "series" ? root.openSeries(modelData) : Iptv.playUrl(modelData.url, modelData.name)
            contentItem: Item {
                Rectangle {
                    id: poster
                    width: posters.cardWidth
                    height: width * 1.5
                    radius: root.px(12)
                    color: root.c("#171717", "#dedede")
                    clip: true
                    Image {
                        anchors.fill: parent
                        source: card.modelData.poster ? "image://iptvlogo/" + encodeURIComponent(card.modelData.poster) : ""
                        sourceSize: Qt.size(width * 2, height * 2)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }
                    Rectangle {
                        anchors.fill: parent
                        visible: card.hovered
                        color: "#99000000"
                        Image {
                            anchors.centerIn: parent
                            width: VLCStyle.redesign.iconSize
                            height: width
                            source: "qrc:///redesign/icons/play/outline-active.png"
                            sourceSize: Qt.size(width, height)
                        }
                    }
                }
                Text {
                    y: poster.height + root.px(14)
                    width: posters.cardWidth
                    text: card.modelData.name
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    color: card.hovered ? VLCStyle.redesign.accent : root.c(VLCStyle.redesign.textSecondary, "#4d4d4d")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(14)
                }
            }
        }

        Text {
            anchors.centerIn: parent
            visible: posters.count === 0
            text: root.account === "" ? qsTr("No account with films or series")
                  : root.loading ? qsTr("Loading…") : qsTr("Nothing here")
            color: root.c("#6B6B6B", "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.redesign.fontMedium
        }
    }

    // the episodes of the series opened
    Item {
        anchors.fill: parent
        visible: root.series.id !== undefined
        Row {
            id: seriesBar
            x: root.px(20)
            spacing: root.px(12)
            Widgets.RedesignButton {
                text: qsTr("Back")
                onClicked: root.series = ({})
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.series.name ?? ""
                color: root.c("#C6C6C6", "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(16)
                font.weight: Font.Medium
            }
        }
        ListView {
            anchors.fill: parent
            anchors.topMargin: seriesBar.height + root.px(16)
            anchors.leftMargin: root.px(12)
            anchors.rightMargin: root.px(12)
            clip: true
            model: root.episodes
            spacing: root.px(2)
            T.ScrollBar.vertical: Widgets.ScrollBarExt { }
            delegate: T.ItemDelegate {
                id: episode
                required property var modelData
                width: ListView.view.width
                height: root.px(48)
                onClicked: Iptv.playUrl(modelData.url, (root.series.name ?? "") + " – " + modelData.title)
                background: Rectangle {
                    radius: root.px(6)
                    color: episode.hovered ? root.c("#151515", "#b5b5b4") : "transparent"
                }
                contentItem: Text {
                    leftPadding: root.px(12)
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    text: qsTr("S%1 E%2").arg(episode.modelData.season).arg(episode.modelData.number)
                          + "   " + episode.modelData.title
                    color: episode.hovered ? VLCStyle.redesign.accent : root.c(VLCStyle.redesign.textSecondary, "#4d4d4d")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(14)
                }
            }
        }
    }
}
