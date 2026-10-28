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

import VLC.MainInterface
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets
import VLC.Dialogs

// Live TV in the library (the TV entry of the sidebar): the channels of the
// user's sources by group, with what is on and what is next, or the guide
// (channels by time). The header of the library pages: "TV", the count,
// List/Grid, the search; Channels | Guide and the sources at its end.
// Only opened while the add-on is there and on (Iptv.enabled).
Widgets.PageExt {
    id: root

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    // (the light theme from the map of the design: VLCStyle.redesign.color)
    function c(dark, light) { return VLCStyle.redesign.color(dark) }

    // "channels" or "guide", kept between the visits
    property string view: MainCtx.settingValue("iptv/view", "channels")
    onViewChanged: MainCtx.setSettingValue("iptv/view", view)

    readonly property bool _hasSources: Iptv.sources.length > 0
    // an Xtream Codes account with films, with series
    readonly property bool _hasFilms: Iptv.sources.some(s => s.enabled && s.vod)
    readonly property bool _hasSeries: Iptv.sources.some(s => s.enabled && s.series)
    readonly property bool _library: view === "movies" || view === "series"
    readonly property bool _listMode: !MainCtx.gridView

    title: qsTr("TV")
    libraryHeader: true
    libraryItemCount: _hasSources ? Iptv.channelModel.count : -1
    isSearchable: _hasSources
    hasGridListMode: _hasSources && view === "channels"

    // the add-on switched off while the page is shown: back home
    Connections {
        target: Iptv
        function onEnabledChanged() {
            if (!Iptv.enabled)
                History.push(["home"])
        }
        // a channel chosen here: to the player at once, as a TV box
        function onChannelSwitched() {
            if (root.visible)
                MainCtx.playerView = true
        }
    }
    Component.onCompleted: {
        if (!Iptv.enabled)
            Qt.callLater(() => History.push(["home"]))
    }

    // the search of the header filters the channels (name or number)
    Binding {
        target: Iptv.channelModel
        property: "search"
        value: root.search.pattern
    }

    headerAction: Component {
        RowLayout {
            spacing: VLCStyle.dp(12, VLCStyle.scale)
            visible: root._hasSources

            SettingsSegmented {
                Layout.preferredWidth: Math.max(VLCStyle.dp(200, VLCStyle.scale), neededWidth)
                Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
                options: [{ text: qsTr("Channels"), value: "channels" },
                          { text: qsTr("Guide"), value: "guide" }]
                         .concat(root._hasFilms ? [{ text: qsTr("Films"), value: "movies" }] : [])
                         .concat(root._hasSeries ? [{ text: qsTr("Series"), value: "series" }] : [])
                value: root.view
                onChosen: (value) => root.view = value
            }

            Widgets.RedesignButton {
                Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
                text: qsTr("Sources")
                onClicked: root.openSources()
            }
        }
    }

    // The dialog of the sources, made when first opened
    property var _sourceDialog: null
    function openSources(page) {
        if (!_sourceDialog) {
            const component = Qt.createComponent("qrc:///qt/qml/VLC/Network/TvSourceDialog.qml")
            if (component.status !== Component.Ready) {
                console.warn(component.errorString())
                return
            }
            _sourceDialog = component.createObject(root)
        }
        _sourceDialog.openAt(page ?? (_hasSources ? "sources" : "link"))
    }

    property var _programmeDialog: null
    function openProgramme(channel, programme) {
        if (!_programmeDialog) {
            const component = Qt.createComponent("qrc:///qt/qml/VLC/Network/TvProgrammeDialog.qml")
            if (component.status !== Component.Ready) {
                console.warn(component.errorString())
                return
            }
            _programmeDialog = component.createObject(root)
        }
        _programmeDialog.show(channel, programme)
    }

    // Figma "file context menu" rows for a channel
    Widgets.RedesignContextMenu {
        id: channelMenu
        parentItem: root

        function channel(dataList) { return dataList[0] }

        actions: [
            { text: qsTr("Watch"), icon: "play",
              action: (dataList) => Iptv.play(channelMenu.channel(dataList).key) },
            { text: qsTr("Watch from the start"), icon: "recent",
              visible: (dataList) => channelMenu.channel(dataList).catchupDays > 0
                                     && channelMenu.channel(dataList).nowStart > 0,
              action: (dataList) => {
                  const ch = channelMenu.channel(dataList)
                  Iptv.playCatchup(ch.key, ch.nowStart, ch.nowStop, ch.nowTitle)
              } },
            { text: (dataList) => channelMenu.channel(dataList).favorite ? qsTr("Remove from Favorites")
                                                                         : qsTr("Favorite"),
              icon: "favorite",
              action: (dataList) => Iptv.toggleFavorite(channelMenu.channel(dataList).key) },
            { text: qsTr("Show in the Guide"), icon: "lists",
              action: (dataList) => {
                  root.view = "guide"
                  Qt.callLater(() => { if (guideLoader.item) guideLoader.item.showChannel(channelMenu.channel(dataList).key) })
              } },
            { text: qsTr("What's On"), icon: "info",
              enabled: (dataList) => channelMenu.channel(dataList).nowStart > 0,
              action: (dataList) => {
                  const ch = channelMenu.channel(dataList)
                  root.openProgramme(ch, { start: ch.nowStart, stop: ch.nowStop, title: ch.nowTitle })
              } }
        ]

        onRequestData: (requestID, indexes) => {
            setData(requestID, indexes.map(i => Iptv.channelModel.get(i)))
        }
    }

    Item {
        id: content
        anchors.fill: parent
        focus: true

        function setCurrentItemFocus(reason) {
            if (root.view === "guide" && guideLoader.item)
                guideLoader.item.forceActiveFocus(reason)
            else if (channelList.visible)
                channelList.forceActiveFocus(reason)
            else
                channelGrid.forceActiveFocus(reason)
        }

        // No source yet: what Live TV is, and the way to add one
        Column {
            anchors.centerIn: parent
            width: Math.min(parent.width - root.px(40), root.px(520))
            visible: !root._hasSources
            spacing: root.px(16)

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                width: root.px(48)
                height: width
                sourceSize: Qt.size(width, height)
                source: SVGColorImage.colorize("qrc:///redesign/duo/tv.svg")
                                     .color1(root.c("#8F8F8F", "#5A5A59"))
                                     .color2(VLCStyle.redesign.accent).uri()
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Watch TV in VLC")
                color: root.c("#C6C6C6", "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(18)
                font.weight: Font.Medium
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("Add the playlist link of your TV provider (M3U) or its Xtream Codes login, "
                           + "or a free channel list of the catalog. Its programme guide comes with it.")
                color: root.c("#6B6B6B", "#4d4d4d")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
                lineHeight: 1.3
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: root.px(12)
                Widgets.RedesignButton {
                    text: qsTr("Add a Link")
                    accent: true
                    onClicked: root.openSources("link")
                }
                Widgets.RedesignButton {
                    text: qsTr("Browse the Catalog")
                    onClicked: root.openSources("catalog")
                }
            }
        }

        // The groups: All, the favourites, the ones watched last, then the
        // provider's (chips of the segmented controls of the design)
        ListView {
            id: chips
            visible: root._hasSources && !root._library
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: root.px(20)
            anchors.rightMargin: root.px(20)
            anchors.topMargin: root.px(16)
            height: visible ? root.px(30) : 0
            orientation: ListView.Horizontal
            spacing: root.px(8)
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: [{ name: "", label: qsTr("All"), count: Iptv.channelCount },
                    { name: "favorites", label: qsTr("Favorites"), count: Iptv.favorites.length },
                    { name: "recent", label: qsTr("Recently Watched"), count: -1 }]
                   .concat(Iptv.groups.map(g => ({ name: g.name, label: g.name, count: g.count })))

            delegate: T.Button {
                id: chip
                required property var modelData
                readonly property bool selected: Iptv.channelModel.group === modelData.name
                height: chips.height
                width: chipText.implicitWidth + root.px(24)
                focusPolicy: Qt.StrongFocus
                Accessible.name: modelData.label
                Accessible.checked: selected
                onClicked: Iptv.channelModel.group = modelData.name

                background: Rectangle {
                    radius: root.px(6)
                    color: chip.selected ? VLCStyle.redesign.color(VLCStyle.redesign.accentBackground)
                                         : chip.hovered ? root.c("#222222", "#9c9c9b") : root.c("#191919", "#ababaa")
                    border.width: chip.visualFocus ? 1 : 0
                    border.color: VLCStyle.redesign.accent
                }
                contentItem: Text {
                    id: chipText
                    text: chip.modelData.count > 0 && chip.modelData.name !== "favorites"
                          ? chip.modelData.label + "  " + chip.modelData.count : chip.modelData.label
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: chip.selected ? VLCStyle.redesign.accent : root.c("#8F8F8F", "#2e2e2e")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(13)
                }
            }
        }

        // the reminders of the guide, and what could not be done
        TvToast {
            z: 2
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: root.px(12)
        }

        // what the add-on is doing (a guide of 30 MB takes a while)
        Text {
            id: statusText
            readonly property var _busySource: Iptv.sources.find(s => s.busy) ?? null
            readonly property var _failed: Iptv.sources.find(s => s.error && s.error.length > 0 && s.channels === 0) ?? null
            visible: root._hasSources && text !== ""
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: chips.bottom
            anchors.leftMargin: root.px(20)
            anchors.rightMargin: root.px(20)
            anchors.topMargin: root.px(10)
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            text: {
                if (_busySource) {
                    const mb = (_busySource.received / 1048576).toFixed(1)
                    return _busySource.stage === "guide"
                           ? qsTr("Updating the guide of %1… %2 MB").arg(_busySource.name).arg(mb)
                           : qsTr("Updating the channels of %1…").arg(_busySource.name)
                }
                if (_failed)
                    return qsTr("%1 could not be loaded: %2").arg(_failed.name).arg(_failed.error)
                return ""
            }
            color: _busySource ? root.c("#6B6B6B", "#4d4d4d") : root.c("#e15033", "#b03a22")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(12)
        }

        Item {
            id: body
            visible: root._hasSources
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: statusText.visible ? statusText.bottom : chips.bottom
            anchors.bottom: parent.bottom
            anchors.topMargin: root.px(12)

            // the channels, a row each
            ListView {
                id: channelList
                anchors.fill: parent
                anchors.leftMargin: root.px(12)
                anchors.rightMargin: root.px(12)
                visible: root.view === "channels" && root._listMode
                model: visible ? Iptv.channelModel : null
                clip: true
                focus: visible
                spacing: root.px(2)
                bottomMargin: root.px(20)
                reuseItems: true
                cacheBuffer: root.px(56) * 6
                boundsBehavior: Flickable.StopAtBounds
                keyNavigationEnabled: true
                highlightMoveDuration: 0
                T.ScrollBar.vertical: Widgets.ScrollBarExt { }

                delegate: TvChannelRow {
                    width: ListView.view.width
                    onPlay: Iptv.play(model.key)
                    onMenu: (point) => channelMenu.popup([index], point)
                }

                Keys.onReturnPressed: if (currentIndex >= 0) Iptv.play(Iptv.channelModel.keyAt(currentIndex))
            }

            // the channels as cards: the logo on the well, the name and what is on
            GridView {
                id: channelGrid
                anchors.fill: parent
                anchors.leftMargin: root.px(20)
                visible: root.view === "channels" && !root._listMode
                model: visible ? Iptv.channelModel : null
                clip: true
                focus: visible
                readonly property real cardWidth: root.px(196)
                readonly property real gap: root.px(20)
                cellWidth: cardWidth + gap
                cellHeight: cardWidth * 9 / 16 + root.px(14 + 19 + 4 + 15) + gap
                bottomMargin: root.px(20)
                reuseItems: true
                cacheBuffer: cellHeight * 2
                boundsBehavior: Flickable.StopAtBounds
                keyNavigationEnabled: true
                T.ScrollBar.vertical: Widgets.ScrollBarExt { }

                delegate: T.ItemDelegate {
                    id: card
                    required property var model
                    required property int index
                    width: channelGrid.cellWidth
                    height: channelGrid.cellHeight
                    padding: 0
                    background: null
                    focusPolicy: Qt.StrongFocus
                    Accessible.name: model.name
                    onClicked: Iptv.play(model.key)
                    Keys.onReturnPressed: Iptv.play(model.key)
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: (eventPoint) => channelMenu.popup([card.index], card.mapToGlobal(eventPoint.position))
                    }
                    readonly property real _progress: {
                        const start = model.nowStart ?? 0, stop = model.nowStop ?? 0
                        return stop > start ? Math.max(0, Math.min(1, (Iptv.now - start) / (stop - start))) : 0
                    }

                    contentItem: Item {
                        Rectangle {
                            id: picture
                            anchors.left: parent.left
                            width: channelGrid.cardWidth
                            height: width * 9 / 16
                            radius: root.px(12)
                            color: card.model.current ? VLCStyle.redesign.color(VLCStyle.redesign.accentBackground)
                                                      : root.c("#171717", "#dedede")
                            border.width: card.visualFocus ? 1 : 0
                            border.color: VLCStyle.redesign.focusColor

                            TvLogo {
                                anchors.centerIn: parent
                                width: root.px(64)
                                height: width
                                color: "transparent"
                                source: card.model.logo
                                name: card.model.name
                            }
                            Text {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.margins: root.px(10)
                                visible: card.model.number > 0
                                text: card.model.number
                                color: root.c("#6B6B6B", "#5a5a59")
                                font.family: VLCStyle.redesign.fontFamily
                                font.pixelSize: root.px(12)
                            }
                            TvBadge {
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: root.px(8)
                                visible: card.model.catchupDays > 0
                                icon: "recent"
                                text: qsTr("%1 d", "%1 d", card.model.catchupDays).arg(card.model.catchupDays)
                            }
                            // under the pointer: darkened, the play of the set
                            Rectangle {
                                anchors.fill: parent
                                radius: parent.radius
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
                            // what is on, its progress at the bottom (a time axis)
                            Item {
                                LayoutMirroring.enabled: false
                                LayoutMirroring.childrenInherit: true
                                anchors.left: parent.left
                                anchors.bottom: parent.bottom
                                visible: Iptv.showProgress && card._progress > 0
                                width: Math.round(parent.width * card._progress)
                                height: root.px(4)
                                clip: true
                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.bottom: parent.bottom
                                    width: picture.width
                                    height: 2 * picture.radius
                                    radius: picture.radius
                                    color: VLCStyle.redesign.accent
                                }
                            }
                        }
                        Text {
                            id: cardName
                            anchors.left: parent.left
                            y: picture.height + root.px(14)
                            width: channelGrid.cardWidth
                            text: card.model.name
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignLeft
                            color: card.hovered || card.model.current ? VLCStyle.redesign.accent
                                                                      : root.c(VLCStyle.redesign.textSecondary, "#4d4d4d")
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.px(14)
                        }
                        Text {
                            anchors.left: parent.left
                            anchors.top: cardName.bottom
                            anchors.topMargin: root.px(4)
                            width: channelGrid.cardWidth
                            text: card.model.nowTitle
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignLeft
                            color: root.c("#6B6B6B", "#5a5a59")
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.px(12)
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                width: Math.min(implicitWidth, parent.width - root.px(40))
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: root.view === "channels" && Iptv.channelsLoaded && Iptv.channelModel.count === 0
                text: Iptv.channelModel.search !== "" ? qsTr("No channel matches the search")
                      : Iptv.channelModel.group === "favorites" ? qsTr("No favorite channel yet: right-click a channel to add it")
                      : Iptv.channelModel.group === "recent" ? qsTr("No channel watched yet")
                      : Iptv.busy ? qsTr("Loading the channels…")
                      : qsTr("No channels")
                color: root.c("#6B6B6B", "#4d4d4d")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.redesign.fontMedium
            }

            Loader {
                anchors.fill: parent
                active: root._library
                visible: active
                source: "qrc:///qt/qml/VLC/Network/TvLibraryView.qml"
                onLoaded: {
                    item.kind = Qt.binding(() => root.view === "series" ? "series" : "movies")
                    item.search = Qt.binding(() => root.search.pattern)
                }
            }

            Loader {
                id: guideLoader
                anchors.fill: parent
                active: root.view === "guide"
                visible: active
                focus: active
                source: "qrc:///qt/qml/VLC/Network/TvGuideView.qml"
                onLoaded: item.programmeClicked.connect((channel, programme) => root.openProgramme(channel, programme))
            }
        }
    }
}
