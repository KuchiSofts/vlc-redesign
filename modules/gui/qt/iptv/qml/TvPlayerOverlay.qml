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

// Live TV over the video, as a set-top box: the information bar of the
// channel at every switch (its number, its name, what is on with its progress
// and the time left, what is next), the channel number typed, the list of the
// channels (Enter) with its groups (Left, Right). The player loads it while
// a channel plays (Player.qml) and gives it its keys first. The cards of the
// design over the video: #191919 at 95 %, radius 14, the dark theme always;
// the bar has the colour of the player's control bar.
FocusScope {
    id: root

    // room kept at the bottom (the control bar when shown)
    property real bottomInset: 0
    // room kept at the start (the library next to the video when shown: the
    // banner and the list over the video only, not over its channels)
    property real startInset: 0
    // the player shows its idle picture: nothing plays (Player.qml)
    property bool idle: false
    // the item the keyboard goes back to when the list closes
    property Item restoreFocus: null

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, undefined, true) }
    readonly property var channel: Iptv.currentChannel
    readonly property var liveNow: channel.now ?? ({})

    // Browsing the guide of the channel in the banner (Left, Right, its
    // arrows, a swipe): 0 is what is on, -1 the programme before, +1 the
    // next... The programmes around now are asked once per channel.
    property int browse: 0
    property var _programmes: []
    property int _nowIndex: -1
    property string _browseDescription: ""
    function _loadProgrammes() {
        const key = channel.key ?? ""
        browse = 0
        _programmes = []
        _nowIndex = -1
        if (key === "" || !channel.hasGuide)
            return
        Iptv.programmes([key], Iptv.now - 12 * 3600, Iptv.now + 24 * 3600, (result) => {
            if ((root.channel.key ?? "") !== key)
                return
            const list = result[key] || []
            root._programmes = list
            root._nowIndex = list.findIndex(p => p.start <= Iptv.now && Iptv.now < p.stop)
        })
    }
    onChannelChanged: if ((channel.key ?? "") !== _loadedKey) { _loadedKey = channel.key ?? ""; _loadProgrammes() }
    property string _loadedKey: ""
    readonly property bool canBrowse: _nowIndex >= 0 && !_archive
    function browseBy(step) {
        if (!canBrowse)
            return
        const target = Math.max(-_nowIndex, Math.min(_programmes.length - 1 - _nowIndex, browse + step))
        if (target === browse)
            return
        browse = target
        _browseDescription = ""
        browseTimer.restart()
        bannerTimer.restart()
        const p = _programmes[_nowIndex + browse]
        if (p && p.hasDescription && Iptv.showDescriptions)
            Iptv.description(channel.key, p.start, (text) => {
                if (root._programmes[root._nowIndex + root.browse] === p)
                    root._browseDescription = text
            })
    }
    // Enter on a programme browsed to: a past one from the archive, a
    // reminder for one to come
    function browseAction() {
        const p = now
        if (browse < 0 && p.archived) {
            Iptv.playCatchup(channel.key, p.start, p.stop, p.title)
            browse = 0
        } else if (browse > 0) {
            if (Iptv.hasReminder(channel.key, p.start))
                Iptv.removeReminder(channel.key, p.start)
            else
                Iptv.addReminder(channel.key, p.start, p.title)
            bannerTimer.restart()
        }
    }

    // the programme the banner shows: the one browsed to, or what is on
    readonly property var now: browse !== 0 && _programmes[_nowIndex + browse] ? _programmes[_nowIndex + browse] : liveNow
    readonly property var next: browse !== 0 ? (_programmes[_nowIndex + browse + 1] ?? ({})) : (channel.next ?? ({}))
    readonly property bool _hasNow: (now.title ?? "") !== ""
    readonly property var catchup: Iptv.catchupProgramme
    readonly property bool _archive: (catchup.start ?? 0) > 0
    readonly property bool listOpen: listPanel.visible

    function showBanner() {
        if (Iptv.bannerMode === 3)
            return
        bannerTimer.restart()
    }

    function openList() {
        // the group of the channel playing, the list on it
        Iptv.playerModel.group = Iptv.channelModel.group
        listPanel.visible = true
        Qt.callLater(() => {
            const i = Iptv.playerModel.currentIndex
            listView.currentIndex = Math.max(0, i)
            listView.positionViewAtIndex(listView.currentIndex, ListView.Center)
            listView.forceActiveFocus(Qt.OtherFocusReason)
        })
    }
    function closeList() {
        listPanel.visible = false
        if (root.restoreFocus)
            root.restoreFocus.forceActiveFocus(Qt.OtherFocusReason)
    }

    // the groups the list goes through: all, the favourites, the provider's
    readonly property var _groups: ["", "favorites"].concat(Iptv.groups.map(g => g.name))
    function switchGroup(step) {
        const i = Math.max(0, _groups.indexOf(Iptv.playerModel.group))
        Iptv.playerModel.group = _groups[(i + step + _groups.length) % _groups.length]
        listView.currentIndex = Math.max(0, Iptv.playerModel.currentIndex)
    }

    // The keys of the TV, before the player's and VLC's hotkeys: what the
    // key does here, "" nothing (it stays a hotkey). The channels up and
    // down, the last one, the list and the banner on the keys of the
    // shortcuts (Settings > Shortcuts, Live TV); the digits type a number.
    function _keyAction(key, modifiers) {
        const plain = modifiers === Qt.NoModifier || modifiers === Qt.KeypadModifier
        if (listPanel.visible) {
            switch (key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                return "close-list"
            case Qt.Key_Left:
                return LayoutMirroring.enabled ? "group-next" : "group-previous"
            case Qt.Key_Right:
                return LayoutMirroring.enabled ? "group-previous" : "group-next"
            case Qt.Key_Return:
            case Qt.Key_Enter:
                return "play-selected"
            }
        }
        // the banner shown: Left and Right go through the guide of the
        // channel (they seek elsewhere, and in a programme of the archive)
        if (plain && banner.wanted && root.canBrowse) {
            const back = LayoutMirroring.enabled ? Qt.Key_Right : Qt.Key_Left
            const forward = LayoutMirroring.enabled ? Qt.Key_Left : Qt.Key_Right
            if (key === back)
                return "guide-previous"
            if (key === forward)
                return "guide-next"
            if (root.browse !== 0 && (key === Qt.Key_Return || key === Qt.Key_Enter))
                return "guide-action"
            if (root.browse !== 0 && (key === Qt.Key_Escape || key === Qt.Key_Back))
                return "guide-now"
        }
        if (plain) {
            if (key >= Qt.Key_0 && key <= Qt.Key_9)
                return "digit"
            if (key === Qt.Key_ChannelUp)
                return "channel-up"
            if (key === Qt.Key_ChannelDown)
                return "channel-down"
            // a number typed: Backspace takes its last digit, Enter goes
            if (numberEntry.text.length > 0) {
                if (key === Qt.Key_Backspace)
                    return "erase-digit"
                if (key === Qt.Key_Return || key === Qt.Key_Enter)
                    return "play-number"
            }
        }
        return MainCtx.tvKeyAction(key, modifiers)
    }

    // MainCtx.keysBeforeHotkeys: the window gives the keys here first,
    // before the hotkeys and the item with the focus (takeKey: true when
    // done). Every other key without a modifier goes to the list while it
    // is open (wantsKey): it moves through the channels.
    function takeKey(key, modifiers) {
        return _run(_keyAction(key, modifiers), key)
    }
    function wantsKey(key, modifiers) {
        return listPanel.visible && (modifiers === Qt.NoModifier || modifiers === Qt.KeypadModifier)
    }

    // the player's Keys: true when taken
    function handleKey(event) {
        return _run(_keyAction(event.key, event.modifiers), event.key)
    }

    function _run(action, key) {
        switch (action) {
        case "close-list":
            closeList()
            return true
        case "group-previous":
            switchGroup(-1)
            return true
        case "group-next":
            switchGroup(1)
            return true
        case "play-selected":
            if (listView.currentIndex >= 0)
                Iptv.play(Iptv.playerModel.keyAt(listView.currentIndex))
            closeList()
            return true
        case "guide-previous":
            root.browseBy(-1)
            return true
        case "guide-next":
            root.browseBy(1)
            return true
        case "guide-action":
            root.browseAction()
            return true
        case "guide-now":
            root.browse = 0
            return true
        case "digit":
            if (numberEntry.text.length < 4)
                numberEntry.text += String(key - Qt.Key_0)
            numberTimer.restart()
            return true
        case "erase-digit":
            numberEntry.text = numberEntry.text.slice(0, -1)
            numberTimer.restart()
            return true
        case "play-number":
            numberTimer.stop()
            Iptv.playNumber(parseInt(numberEntry.text))
            numberEntry.text = ""
            return true
        case "channel-up":
            Iptv.channelUp()
            return true
        case "channel-down":
            Iptv.channelDown()
            return true
        case "previous":
            Iptv.previousChannel()
            return true
        case "list":
            openList()
            return true
        case "info":
            showBanner()
            return true
        }
        return false
    }

    Connections {
        target: Iptv
        function onChannelSwitched() { root.showBanner() }
        function onChannelListRequested() { root.listOpen ? root.closeList() : root.openList() }
    }
    Component.onCompleted: {
        showBanner()
    }

    Timer { id: bannerTimer; interval: 5000 }
    // a while without browsing: back to what is on
    Timer { id: browseTimer; interval: 15000; onTriggered: root.browse = 0 }
    Timer {
        id: numberTimer
        interval: 1500
        onTriggered: {
            if (numberEntry.text.length > 0)
                Iptv.playNumber(parseInt(numberEntry.text))
            numberEntry.text = ""
        }
    }

    // the pointer over the banner's place brings it back with its buttons
    // (the clicks go through to the video)
    Item {
        id: bannerZone
        anchors.left: parent.left
        anchors.leftMargin: root.startInset
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.bottomInset
        height: banner.height + root.px(48)
        HoverHandler { id: zoneHover }
    }

    // ---- the banner ----
    // The information bar of a cable box (as on the TV): the whole width of
    // the player, flush with the bottom of the window or sitting right on the
    // player's control bar (its colour, no gap), a thin line on top and a
    // soft shade above it over the picture. Three columns: the channel (its
    // logo big, number, name, group), the programme (where it is, its title,
    // times, progress, story, what is next), the side (the time now, the
    // archive, what Enter does, the keys; the buttons under the pointer).
    // The same item for every switch: its texts change, it fades and slides
    // in (180 ms) and hides by itself.
    Rectangle {
        id: banner

        // (none over the idle player: a channel stopped stays the current
        // item, and Customize Player shows the player with nothing playing)
        readonly property bool wanted: Iptv.bannerMode !== 3 && !root.idle
                                       && (bannerTimer.running || bannerHover.hovered || zoneHover.hovered || root.browse !== 0
                                           || numberEntry.text.length > 0)
        readonly property bool pointed: bannerHover.hovered || zoneHover.hovered
        readonly property bool _dark: VLCStyle.palette.isDark
        // a colour of the TV's bar, and its own in the light theme
        function c(value, light) { return VLCStyle.redesign.color(value, light, _dark) }

        // the time now, while the bar shows
        property string _clock: ""
        Timer {
            running: banner.visible
            repeat: true
            triggeredOnStart: true
            interval: 1000
            onTriggered: banner._clock = Iptv.timeText(Math.floor(Date.now() / 1000))
        }

        // compact (the TV's bar made 5 % lower): taller only for a
        // description
        height: Math.max(root.px(100), bannerHead.implicitHeight, bannerInfo.y + bannerInfo.implicitHeight + root.px(13),
                         bannerEnd.implicitHeight) + root.px(6)
        Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        anchors.left: parent.left
        anchors.leftMargin: root.startInset
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.bottomInset + (wanted ? 0 : -root.px(8))
        color: c(VLCStyle.redesign.playerControlBackground, "#ababaa")
        opacity: wanted ? 1 : 0
        visible: opacity > 0

        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        Behavior on anchors.bottomMargin { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

        HoverHandler { id: bannerHover }
        // a swipe across on a touchpad: the programmes before and after
        WheelHandler {
            acceptedDevices: PointerDevice.TouchPad
            orientation: Qt.Horizontal
            property real _sum: 0
            onWheel: (event) => {
                _sum += event.angleDelta.x
                if (Math.abs(_sum) >= 120) {
                    root.browseBy((_sum < 0) !== root.LayoutMirroring.enabled ? 1 : -1)
                    _sum = 0
                }
            }
        }

        // the soft shade over the picture, above the bar
        Rectangle {
            anchors.bottom: parent.top
            width: parent.width
            height: root.px(90)
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.rgba(0, 0, 0, 0) }
                GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0.55) }
            }
        }
        // the thin line on top
        Rectangle {
            z: 1
            width: parent.width
            height: 1
            color: banner.c("#333333", "#9c9c9b")
        }

        // ---- the channel ----
        Item {
            id: bannerHead
            anchors.left: parent.left
            anchors.leftMargin: root.px(24)
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.px(6)
            width: Math.min(root.px(300), parent.width * 0.3)
            implicitHeight: Math.max(bannerLogo.y + bannerLogo.height, bannerWho.y + bannerWho.implicitHeight) + root.px(13)

            TvLogo {
                id: bannerLogo
                anchors.left: parent.left
                anchors.leftMargin: root.px(18)
                anchors.top: parent.top
                anchors.topMargin: root.px(13)
                width: root.px(64)
                height: width
                source: root.channel.logo ?? ""
                name: root.channel.name ?? ""
            }
            Column {
                id: bannerWho
                anchors.left: bannerLogo.right
                anchors.leftMargin: root.px(12)
                anchors.right: parent.right
                anchors.rightMargin: root.px(14)
                anchors.top: parent.top
                anchors.topMargin: root.px(19)
                spacing: root.px(4)

                Row {
                    id: bannerNameLine
                    width: parent.width
                    spacing: root.px(8)
                    Text {
                        id: bannerNumber
                        visible: text !== ""
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.channel.number > 0 ? root.channel.number : ""
                        color: banner.c("#EAEAEA", "#191919")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(20)
                        font.weight: Font.Bold
                        font.features: { "tnum": 1 }
                    }
                    Text {
                        id: bannerName
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(implicitWidth, parent.width - (bannerNumber.visible ? bannerNumber.width + parent.spacing : 0)
                                        - (bannerStar.visible ? bannerStar.width + root.px(6) : 0))
                        text: root.channel.name ?? ""
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        color: banner.c("#EAEAEA", "#191919")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(16)
                        font.weight: Font.DemiBold
                    }
                    // a favourite: the star (6 px from the name)
                    Image {
                        id: bannerStar
                        visible: Iptv.favorites.indexOf(root.channel.key ?? "") >= 0
                        anchors.verticalCenter: parent.verticalCenter
                        width: root.px(16)
                        height: width
                        sourceSize: Qt.size(width, height)
                        source: SVGColorImage.colorize("qrc:///redesign/duo/favorite.svg")
                                             .color1(VLCStyle.redesign.accent).color2(VLCStyle.redesign.accent).uri()
                        Accessible.name: qsTr("Favorite")
                    }
                }
                Text {
                    width: parent.width
                    visible: text !== ""
                    text: root.channel.group ?? ""
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    color: banner.c("#8F8F8F")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(13)
                }
            }
            // the divider, the bar's height
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.bottom: parent.bottom
                width: 1
                color: banner.c("#262626", "#9c9c9b")
            }
        }

        // ---- the programme ----
        // what is on (or browsed to), and next; with Settings > TV, its
        // description (the bar grows to it, smoothly)
        Column {
            id: bannerInfo
            anchors.left: bannerHead.right
            anchors.leftMargin: root.px(20)
            anchors.right: bannerEnd.left
            anchors.rightMargin: root.px(20)
            anchors.top: parent.top
            anchors.topMargin: root.px(13)

            // where the programme is (Live, Earlier 2, Later, Archive) and
            // its title
            Row {
                width: parent.width
                height: root.px(26)
                spacing: root.px(12)
                Rectangle {
                    id: bannerPill
                    readonly property bool live: !root._archive && root.browse === 0
                    readonly property string label: root._archive ? qsTr("Archive")
                                                    : root.browse !== 0 ? (root.browse > 0 ? qsTr("Later") : qsTr("Earlier"))
                                                                          + (Math.abs(root.browse) > 1 ? " " + Math.abs(root.browse) : "")
                                                    : root._hasNow && Iptv.bannerMode !== 2 ? qsTr("Live") : ""
                    visible: label !== ""
                    anchors.verticalCenter: parent.verticalCenter
                    width: bannerPillText.implicitWidth + root.px(16)
                    height: root.px(22)
                    radius: root.px(5)
                    color: live ? VLCStyle.redesign.accent : banner.c("#373737")
                    Text {
                        id: bannerPillText
                        anchors.centerIn: parent
                        text: bannerPill.label
                        color: bannerPill.live ? "#0E0E0E" : banner.c("#EAEAEA", "#191919")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(12)
                        font.weight: Font.Bold
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: root.px(12) * 0.04
                    }
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - (bannerPill.visible ? bannerPill.width + parent.spacing : 0)
                    text: root._archive ? (root.catchup.title || qsTr("Programme"))
                          : Iptv.bannerMode === 2 ? ""
                          : root._hasNow ? root.now.title
                          : (root.channel.hasGuide ? qsTr("Loading the guide…") : qsTr("No guide for this channel"))
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    color: banner.c("#FFFFFF", "#191919")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(20)
                    font.weight: Font.DemiBold
                }
            }

            // its times, the progress live and the time left (or when it
            // was, is; the day of the archive)
            Row {
                width: parent.width
                topPadding: root.px(7)
                spacing: root.px(14)
                visible: Iptv.bannerMode !== 2 && (root._hasNow || root._archive)
                Text {
                    id: nowTimes
                    anchors.verticalCenter: bannerLeft.verticalCenter
                    // the start first in every language: left to right
                    // (U+202A ... U+202C), as on the TV and Android
                    text: "‪" + (root._archive ? Iptv.timeText(root.catchup.start ?? 0) + " – " + Iptv.timeText(root.catchup.stop ?? 0)
                                      : Iptv.timeText(root.now.start ?? 0) + " – " + Iptv.timeText(root.now.stop ?? 0)) + "‬"
                    color: banner.c("#EAEAEA", "#191919")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(15)
                    font.weight: Font.DemiBold
                    font.features: { "tnum": 1 }
                }
                // a time axis: from the left in every language, as the
                // seek bar
                Rectangle {
                    id: bannerProgress
                    LayoutMirroring.enabled: false
                    visible: Iptv.showProgress && root._hasNow && !root._archive && root.browse === 0
                    anchors.verticalCenter: bannerLeft.verticalCenter
                    width: Math.max(root.px(80), Math.min(root.px(360), parent.width - nowTimes.width - bannerLeft.width
                                                                         - 2 * parent.spacing))
                    height: root.px(5)
                    radius: root.px(3)
                    color: banner.c("#333333", "#9c9c9b")
                    Rectangle {
                        height: parent.height
                        radius: parent.radius
                        color: VLCStyle.redesign.accent
                        width: root.now.stop > root.now.start
                               ? Math.round(parent.width * Math.max(0, Math.min(1, (Iptv.now - root.now.start)
                                                                                  / (root.now.stop - root.now.start))))
                               : 0
                    }
                }
                Text {
                    id: bannerLeft
                    height: root.px(20)
                    verticalAlignment: Text.AlignVCenter
                    text: root._archive ? qsTr("From the archive, %1").arg(Iptv.dayText(root.catchup.start ?? 0))
                          : root.browse === 0 ? qsTr("%1 left").arg(Iptv.durationText((root.now.stop ?? 0) - Iptv.now))
                          : root.now.start > Iptv.now ? qsTr("in %1").arg(Iptv.durationText(root.now.start - Iptv.now))
                          : root.now.stop <= Iptv.now ? qsTr("ended %1 ago").arg(Iptv.durationText(Iptv.now - root.now.stop))
                          : qsTr("on now")
                    color: banner.c("#A6A6A6", "#404040")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(14)
                }
            }

            // Settings > TV: the episode, its category and its description
            // (up to two lines; the whole text in the programme's details)
            Text {
                readonly property string meta: [root.now.subtitle, root.now.episode, root.now.category]
                                                .filter(t => !!t).join("  ·  ")
                width: parent.width
                visible: Iptv.showDescriptions && !root._archive && Iptv.bannerMode !== 2 && meta !== ""
                topPadding: root.px(10)
                text: meta
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                lineHeight: root.px(18)
                lineHeightMode: Text.FixedHeight
                color: banner.c("#A6A6A6", "#404040")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
            }
            Text {
                width: parent.width
                visible: Iptv.showDescriptions && !root._archive && Iptv.bannerMode !== 2
                         && (root.browse === 0 ? (root.now.description ?? "") : root._browseDescription) !== ""
                topPadding: root.px(6)
                text: (root.browse === 0 ? (root.now.description ?? "") : root._browseDescription).replace(/\s+/g, " ")
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
                lineHeight: root.px(20)
                lineHeightMode: Text.FixedHeight
                horizontalAlignment: Text.AlignLeft
                color: banner.c("#8F8F8F")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
            }

            // what is next: its time and its title
            Row {
                width: parent.width
                topPadding: root.px(10)
                spacing: root.px(10)
                visible: !root._archive && Iptv.bannerMode === 0 && Iptv.showNext && (root.next.title ?? "") !== ""
                Rectangle {
                    id: bannerNextKey
                    anchors.verticalCenter: bannerNextText.verticalCenter
                    width: bannerNextLabel.implicitWidth + root.px(14)
                    height: root.px(20)
                    radius: root.px(4)
                    color: "transparent"
                    border.width: 1
                    border.color: banner.c("#404040")
                    Text {
                        id: bannerNextLabel
                        anchors.centerIn: parent
                        text: qsTr("Next")
                        color: banner.c("#C6C6C6")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(11)
                        font.weight: Font.Bold
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: root.px(11) * 0.04
                    }
                }
                Text {
                    id: bannerNextText
                    width: parent.width - bannerNextKey.width - parent.spacing
                    height: root.px(20)
                    verticalAlignment: Text.AlignVCenter
                    text: Iptv.timeText(root.next.start ?? 0) + "  " + (root.next.title ?? "")
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    color: banner.c("#A6A6A6", "#404040")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(14)
                }
            }
        }

        // ---- the side ----
        // the time now; the archive of the channel and what Enter does on
        // the programme shown; the keys (the buttons under the pointer)
        Item {
            id: bannerEnd
            anchors.right: parent.right
            anchors.rightMargin: root.px(24 + 18)
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.px(6)
            width: Math.max(root.px(150), bannerMarks.implicitWidth, bannerKeys.implicitWidth, bannerButtons.implicitWidth)
            implicitHeight: root.px(12) + bannerClock.height + (bannerMarks.implicitWidth > 0 ? root.px(8) + bannerMarks.height : 0)
                            + root.px(8) + Math.max(bannerKeys.implicitHeight, bannerButtons.implicitHeight) + root.px(12)

            Text {
                id: bannerClock
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: root.px(12)
                text: banner._clock
                lineHeight: root.px(30)
                lineHeightMode: Text.FixedHeight
                color: banner.c("#EAEAEA", "#191919")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(26)
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
            }

            Row {
                id: bannerMarks
                anchors.right: parent.right
                anchors.top: bannerClock.bottom
                anchors.topMargin: root.px(8)
                spacing: root.px(8)

                TvBadge {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !root._archive && (root.channel.catchupDays ?? 0) > 0
                    icon: "recent"
                    text: qsTr("%1 d", "%1 d", root.channel.catchupDays ?? 0).arg(root.channel.catchupDays ?? 0)
                    tip: qsTr("Catch-up: the past %1 day", "Catch-up: the past %1 days", root.channel.catchupDays ?? 0)
                             .arg(root.channel.catchupDays ?? 0)
                }
                // what is on: from its start (the archive)
                BarButton {
                    visible: !root._archive && (root.channel.catchupDays ?? 0) > 0 && root._hasNow && root.browse === 0
                    iconName: "recent"
                    tip: qsTr("Watch from the Start")
                    onClicked: Iptv.playFromStart()
                }
                // a programme browsed to: watch it from the archive, or a
                // reminder for it
                Widgets.RedesignButton {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: (root.browse < 0 && !!root.now.archived) || root.browse > 0
                    accent: root.browse < 0
                    implicitWidth: Math.min(root.px(160), Math.max(root.px(90), implicitContentWidth + root.px(24)))
                    implicitHeight: root.px(28)
                    text: root.browse < 0 ? qsTr("Watch")
                          : (Iptv.reminders.length >= 0 && Iptv.hasReminder(root.channel.key ?? "", root.now.start ?? 0))
                            ? qsTr("Reminder Set") : qsTr("Remind Me")
                    onClicked: root.browseAction()
                }
                // a programme of the archive plays
                Widgets.RedesignButton {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root._archive
                    accent: true
                    implicitWidth: Math.min(root.px(160), Math.max(root.px(90), implicitContentWidth + root.px(24)))
                    implicitHeight: root.px(28)
                    text: qsTr("Back to Live")
                    onClicked: Iptv.backToLive()
                }
            }

            // the keys of the bar (as the remote's on the TV); under the
            // pointer, the buttons that do the same
            Column {
                id: bannerKeys
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: root.px(12)
                visible: !banner.pointed
                readonly property string _up: MainCtx.tvKeyText("channel-up")
                readonly property string _down: MainCtx.tvKeyText("channel-down")
                Repeater {
                    model: [root.canBrowse ? "←  →   " + qsTr("Programs") : "",
                            bannerKeys._up !== "" || bannerKeys._down !== ""
                            ? [bannerKeys._up, bannerKeys._down].filter(k => k !== "").join(" / ") + "   " + qsTr("Channels") : ""]
                    Text {
                        required property string modelData
                        anchors.right: parent.right
                        visible: modelData !== ""
                        text: modelData
                        color: banner.c("#6B6B6B")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(12)
                        lineHeight: root.px(16)
                        lineHeightMode: Text.FixedHeight
                    }
                }
            }
            Row {
                id: bannerButtons
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: root.px(9)
                height: root.px(30)
                spacing: root.px(4)
                visible: banner.pointed

                BarButton {
                    visible: root.canBrowse
                    glyph: LayoutMirroring.enabled ? "›" : "‹"
                    tip: qsTr("Programme before")
                    onClicked: root.browseBy(-1)
                }
                BarButton {
                    visible: root.canBrowse
                    glyph: LayoutMirroring.enabled ? "‹" : "›"
                    tip: qsTr("Programme after")
                    onClicked: root.browseBy(1)
                }
                BarButton {
                    glyph: "▲"
                    tip: qsTr("Next channel")
                    onClicked: Iptv.channelUp()
                }
                BarButton {
                    glyph: "▼"
                    tip: qsTr("Previous channel")
                    onClicked: Iptv.channelDown()
                }
                BarButton {
                    iconName: "lists"
                    tip: qsTr("Channels")
                    active: root.listOpen
                    onClicked: root.openList()
                }
            }
        }
    }

    // a small square button of the bar: a two-tone icon or a glyph, its
    // name in a tool tip
    component BarButton: T.Button {
        id: barButton
        property string iconName
        property string glyph
        property string tip
        // open: what it opens shows (the accent on a tint of it, as the
        // buttons of the player's bar)
        property bool active: false
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
        implicitWidth: root.px(30)
        implicitHeight: root.px(30)
        focusPolicy: Qt.NoFocus
        Accessible.name: tip
        T.ToolTip.visible: hovered && tip !== ""
        T.ToolTip.text: tip
        T.ToolTip.delay: 650
        background: Rectangle {
            radius: root.px(6)
            color: barButton.active ? (barButton.down || barButton.hovered
                                       ? Qt.tint(VLCStyle.redesign.accentBackground, Qt.rgba(1, 1, 1, 0.06))
                                       : VLCStyle.redesign.accentBackground)
                   : barButton.down || barButton.hovered ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground)
                   : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }
        }
        contentItem: Item {
            Image {
                anchors.centerIn: parent
                visible: barButton.iconName !== ""
                width: root.px(20)
                height: width
                sourceSize: Qt.size(width, height)
                source: barButton.iconName === "" ? ""
                        : SVGColorImage.colorize("qrc:///redesign/duo/" + barButton.iconName + ".svg")
                                       .color1(barButton.active ? VLCStyle.redesign.accent
                                                                : VLCStyle.redesign.color(VLCStyle.redesign.textMuted))
                                       .color2(VLCStyle.redesign.accent).uri()
            }
            Text {
                anchors.centerIn: parent
                visible: barButton.glyph !== ""
                text: barButton.glyph
                color: barButton.hovered ? VLCStyle.redesign.color(VLCStyle.redesign.textPrimary)
                                         : VLCStyle.redesign.color(VLCStyle.redesign.textMuted)
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(12)
            }
        }
    }


    // ---- the number typed ----
    Rectangle {
        visible: numberEntry.text.length > 0
        anchors.right: parent.right
        anchors.rightMargin: root.px(24)
        anchors.top: parent.top
        anchors.topMargin: root.px(24)
        width: Math.max(root.px(96), numberEntry.implicitWidth + root.px(40))
        height: root.px(72)
        radius: root.px(14)
        color: Qt.rgba(25 / 255, 25 / 255, 25 / 255, 0.95)
        Text {
            id: numberEntry
            anchors.centerIn: parent
            text: ""
            color: VLCStyle.redesign.accent
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(40)
            font.weight: Font.DemiBold
        }
    }

    // ---- the list of the channels ----
    Rectangle {
        id: listPanel
        visible: false
        anchors.left: parent.left
        anchors.leftMargin: root.startInset + root.px(16)
        anchors.top: parent.top
        anchors.topMargin: root.px(16)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.bottomInset + root.px(16)
        width: Math.min(root.px(400), parent.width - root.startInset - root.px(32))
        radius: root.px(14)
        color: Qt.rgba(14 / 255, 14 / 255, 14 / 255, 0.95)

        // the group, and the arrows to the others
        Item {
            id: groupBar
            x: root.px(12)
            y: root.px(12)
            width: parent.width - root.px(24)
            height: root.px(36)
            Widgets.RedesignButton {
                id: previousGroup
                anchors.left: parent.left
                implicitWidth: root.px(36)
                darkTheme: true
                text: LayoutMirroring.enabled ? "›" : "‹"
                Accessible.name: qsTr("Previous group")
                onClicked: root.switchGroup(-1)
            }
            Text {
                anchors.left: previousGroup.right
                anchors.right: nextGroup.left
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: Iptv.playerModel.group === "" ? qsTr("All Channels")
                      : Iptv.playerModel.group === "favorites" ? qsTr("Favorites")
                      : Iptv.playerModel.group === "recent" ? qsTr("Recently Watched")
                      : Iptv.playerModel.group
                color: root.c("#C6C6C6")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(15)
                font.weight: Font.Medium
            }
            Widgets.RedesignButton {
                id: nextGroup
                anchors.right: parent.right
                implicitWidth: root.px(36)
                darkTheme: true
                text: LayoutMirroring.enabled ? "‹" : "›"
                Accessible.name: qsTr("Next group")
                onClicked: root.switchGroup(1)
            }
        }

        // the guide of the TV page, the channels down and the time across
        Widgets.RedesignButton {
            id: guideButton
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: root.px(12)
            darkTheme: true
            text: qsTr("Guide")
            onClicked: {
                root.closeList()
                MainCtx.setSettingValue("iptv/view", "guide")
                History.push(["tv"])
                MainCtx.playerView = false
            }
        }

        ListView {
            id: listView
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: groupBar.bottom
            anchors.bottom: guideButton.top
            anchors.margins: root.px(8)
            clip: true
            model: listPanel.visible ? Iptv.playerModel : null
            keyNavigationEnabled: true
            highlightMoveDuration: 0
            boundsBehavior: Flickable.StopAtBounds
            reuseItems: true
            T.ScrollBar.vertical: Widgets.ScrollBarExt { }
            delegate: TvChannelRow {
                width: ListView.view.width
                compact: true
                forceDark: true
                focus: ListView.isCurrentItem
                onPlay: {
                    Iptv.play(model.key)
                    root.closeList()
                }
            }
            Text {
                anchors.centerIn: parent
                visible: listView.count === 0
                text: qsTr("No channels")
                color: root.c("#6B6B6B")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
            }
        }
    }

    TvToast {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: root.startInset / 2
        anchors.top: parent.top
        anchors.topMargin: root.px(24)
        forceDark: true
    }
}
