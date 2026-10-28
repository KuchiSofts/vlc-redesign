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

// Live TV over the video, as a set-top box: the banner of the channel at
// every switch (its number, its name, what is on with its progress and the
// time left, what is next), the channel number typed, the list of the
// channels (Enter) with its groups (Left, Right). The player loads it while
// a channel plays (Player.qml) and gives it its keys first. The cards of the
// design over the video: #191919 at 95 %, radius 14, the dark theme always.
FocusScope {
    id: root

    // room kept at the bottom (the control bar when shown)
    property real bottomInset: 0
    // the item the keyboard goes back to when the list closes
    property Item restoreFocus: null

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value, undefined, true) }

    readonly property var channel: Iptv.currentChannel
    readonly property var now: channel.now ?? ({})
    readonly property var next: channel.next ?? ({})
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

    // The keys of the TV, before the player's: true when taken
    function handleKey(event) {
        const plain = event.modifiers === Qt.NoModifier || event.modifiers === Qt.KeypadModifier
        if (listPanel.visible) {
            switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                closeList()
                return true
            case Qt.Key_Left:
                switchGroup(LayoutMirroring.enabled ? 1 : -1)
                return true
            case Qt.Key_Right:
                switchGroup(LayoutMirroring.enabled ? -1 : 1)
                return true
            case Qt.Key_Return:
            case Qt.Key_Enter:
                if (listView.currentIndex >= 0)
                    Iptv.play(Iptv.playerModel.keyAt(listView.currentIndex))
                closeList()
                return true
            }
        }
        if (!plain)
            return false
        if (event.key >= Qt.Key_0 && event.key <= Qt.Key_9) {
            if (numberEntry.text.length < 4)
                numberEntry.text += String(event.key - Qt.Key_0)
            numberTimer.restart()
            return true
        }
        switch (event.key) {
        case Qt.Key_PageUp:
        case Qt.Key_ChannelUp:
            Iptv.channelUp()
            return true
        case Qt.Key_PageDown:
        case Qt.Key_ChannelDown:
            Iptv.channelDown()
            return true
        case Qt.Key_Backspace:
            if (numberEntry.text.length > 0) {
                numberEntry.text = numberEntry.text.slice(0, -1)
                numberTimer.restart()
            } else {
                Iptv.previousChannel()
            }
            return true
        case Qt.Key_Return:
        case Qt.Key_Enter:
            if (numberEntry.text.length > 0) {
                numberTimer.stop()
                Iptv.playNumber(parseInt(numberEntry.text))
                numberEntry.text = ""
            } else {
                openList()
            }
            return true
        case Qt.Key_I:
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
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.bottomInset
        width: banner.width
        height: banner.height + root.px(48)
        HoverHandler { id: zoneHover }
    }

    // ---- the banner ----
    // A slim bar centred over the video, above its controls: the channel
    // (number, logo, name), what is on (its times, its title, its progress
    // live, the time left), what is next, the marks of the channel and the
    // buttons of the TV. The same item for every switch: its texts change,
    // it fades and slides in (180 ms) and hides by itself.
    Rectangle {
        id: banner

        readonly property bool wanted: Iptv.bannerMode !== 3
                                       && (bannerTimer.running || bannerHover.hovered || zoneHover.hovered
                                           || numberEntry.text.length > 0)
        readonly property bool _dark: VLCStyle.palette.isDark
        function c(value) { return VLCStyle.redesign.color(value, undefined, _dark) }

        // about two thirds of the video, from a 720p window to a 4K screen
        width: Math.max(Math.min(root.px(560), parent.width - root.px(32)),
                        Math.min(parent.width * 0.66, root.px(1100), parent.width - root.px(32)))
        // the slim bar, taller only for a description
        height: Math.max(root.px(76), bannerInfo.y + bannerInfo.implicitHeight + root.px(18))
        Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.bottomInset + root.px(24) + (wanted ? 0 : -root.px(8))
        radius: root.px(10)
        // the menus' card (#191919) a little translucent: readable over any
        // video, the picture still felt behind it
        color: Qt.alpha(c("#191919"), 0.92)
        border.width: 1
        border.color: c(VLCStyle.redesign.border)
        opacity: wanted ? 1 : 0
        visible: opacity > 0

        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        Behavior on anchors.bottomMargin { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

        HoverHandler { id: bannerHover }

        // the band of the channel and the buttons, the height of the slim bar
        Item {
            id: bannerHead
            width: parent.width
            height: root.px(76)
        }

        // the channel
        Text {
            id: bannerNumber
            anchors.left: parent.left
            anchors.leftMargin: root.px(16)
            anchors.verticalCenter: bannerHead.verticalCenter
            width: root.px(36)
            horizontalAlignment: Text.AlignRight
            text: root.channel.number > 0 ? root.channel.number : ""
            color: banner.c("#969696")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(18)
            font.weight: Font.DemiBold
            font.features: { "tnum": 1 }
        }
        TvLogo {
            id: bannerLogo
            anchors.left: bannerNumber.right
            anchors.leftMargin: root.px(12)
            anchors.verticalCenter: bannerHead.verticalCenter
            width: root.px(44)
            height: width
            source: root.channel.logo ?? ""
            name: root.channel.name ?? ""
        }
        Text {
            id: bannerName
            anchors.left: bannerLogo.right
            anchors.leftMargin: root.px(12)
            anchors.verticalCenter: bannerHead.verticalCenter
            width: Math.min(implicitWidth, banner.width * 0.24)
            text: root.channel.name ?? ""
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            color: banner.c(VLCStyle.redesign.textPrimary)
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(15)
            font.weight: Font.DemiBold
        }
        Rectangle {
            id: bannerDivider
            anchors.left: bannerName.right
            anchors.leftMargin: root.px(16)
            anchors.verticalCenter: bannerHead.verticalCenter
            width: 1
            height: root.px(36)
            color: banner.c(VLCStyle.redesign.separator)
        }

        // what is on, and next; with Settings > TV, its description
        // (the bar grows to it, smoothly, and stays slim without one)
        Column {
            id: bannerInfo
            anchors.left: bannerDivider.right
            anchors.leftMargin: root.px(16)
            anchors.right: bannerEnd.left
            anchors.rightMargin: root.px(16)
            anchors.top: parent.top
            anchors.topMargin: root.px(18)
            spacing: root.px(6)

            Row {
                width: parent.width
                spacing: root.px(10)
                Text {
                    id: nowTimes
                    visible: text !== ""
                    anchors.verticalCenter: parent.verticalCenter
                    text: root._archive ? Iptv.timeText(root.catchup.start ?? 0) + " – " + Iptv.timeText(root.catchup.stop ?? 0)
                          : root._hasNow && Iptv.bannerMode !== 2
                            ? Iptv.timeText(root.now.start) + " – " + Iptv.timeText(root.now.stop) : ""
                    color: banner.c("#8F8F8F")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(13)
                    font.features: { "tnum": 1 }
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - (nowTimes.visible ? nowTimes.width + parent.spacing : 0)
                    text: root._archive ? (root.catchup.title || qsTr("Programme"))
                          : Iptv.bannerMode === 2 ? ""
                          : root._hasNow ? root.now.title
                          : (root.channel.hasGuide ? qsTr("Loading the guide…") : qsTr("No guide for this channel"))
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    color: banner.c("#C6C6C6")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(16)
                    font.weight: Font.Medium
                }
            }

            Row {
                width: parent.width
                spacing: root.px(10)
                visible: Iptv.bannerMode !== 2 && (root._hasNow || root._archive)
                // the progress, live (a time axis: from the left in every
                // language, as the seek bar)
                Rectangle {
                    id: bannerProgress
                    LayoutMirroring.enabled: false
                    visible: Iptv.showProgress && root._hasNow && !root._archive
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(root.px(180), parent.width * 0.3)
                    height: root.px(3)
                    radius: height / 2
                    color: banner.c("#373737")
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
                    visible: bannerProgress.visible
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("%1 left").arg(Iptv.durationText((root.now.stop ?? 0) - Iptv.now))
                    color: banner.c("#6B6B6B")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(13)
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - (bannerProgress.visible ? bannerProgress.width + bannerLeft.width + 2 * parent.spacing : 0)
                    visible: root._archive || (Iptv.bannerMode === 0 && Iptv.showNext && (root.next.title ?? "") !== "")
                    text: root._archive ? qsTr("From the archive, %1").arg(Iptv.dayText(root.catchup.start ?? 0))
                                        : qsTr("Next: %1 · %2").arg(root.next.title ?? "").arg(Iptv.timeText(root.next.start ?? 0))
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    color: banner.c("#6B6B6B")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(13)
                }
            }

            // Settings > TV: the episode, its category and its description
            // (up to three lines; the whole text in the programme's details)
            Text {
                readonly property string meta: [root.now.subtitle, root.now.episode, root.now.category]
                                                .filter(t => !!t).join("  ·  ")
                width: parent.width
                visible: Iptv.showDescriptions && !root._archive && Iptv.bannerMode !== 2 && meta !== ""
                topPadding: root.px(4)
                text: meta
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                color: banner.c("#8F8F8F")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(13)
            }
            Text {
                width: parent.width
                visible: Iptv.showDescriptions && !root._archive && Iptv.bannerMode !== 2
                         && (root.now.description ?? "") !== ""
                text: (root.now.description ?? "").replace(/\s+/g, " ")
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
                lineHeight: 1.35
                horizontalAlignment: Text.AlignLeft
                color: banner.c("#8F8F8F")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(13)
            }
        }

        // the marks of the channel and the buttons of the TV
        Row {
            id: bannerEnd
            anchors.right: parent.right
            anchors.rightMargin: root.px(14)
            anchors.verticalCenter: bannerHead.verticalCenter
            spacing: root.px(6)

            TvBadge {
                anchors.verticalCenter: parent.verticalCenter
                visible: root._archive
                accent: true
                icon: "recent"
                text: qsTr("Archive")
            }
            TvBadge {
                anchors.verticalCenter: parent.verticalCenter
                visible: !root._archive && (root.channel.catchupDays ?? 0) > 0
                icon: "recent"
                text: qsTr("%1 d", "%1 d", root.channel.catchupDays ?? 0).arg(root.channel.catchupDays ?? 0)
                tip: qsTr("Catch-up: the past %1 day", "Catch-up: the past %1 days", root.channel.catchupDays ?? 0)
                         .arg(root.channel.catchupDays ?? 0)
            }
            Item { width: root.px(4); height: 1 }
            Widgets.RedesignButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: !root._archive && (root.channel.catchupDays ?? 0) > 0 && root._hasNow
                implicitHeight: root.px(28)
                text: qsTr("From the Start")
                onClicked: Iptv.playFromStart()
            }
            Widgets.RedesignButton {
                anchors.verticalCenter: parent.verticalCenter
                visible: root._archive
                accent: true
                implicitHeight: root.px(28)
                text: qsTr("Back to Live")
                onClicked: Iptv.backToLive()
            }
            Widgets.RedesignButton {
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: root.px(28)
                implicitHeight: root.px(28)
                text: "▲"
                Accessible.name: qsTr("Next channel")
                onClicked: Iptv.channelUp()
            }
            Widgets.RedesignButton {
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: root.px(28)
                implicitHeight: root.px(28)
                text: "▼"
                Accessible.name: qsTr("Previous channel")
                onClicked: Iptv.channelDown()
            }
            Widgets.RedesignButton {
                anchors.verticalCenter: parent.verticalCenter
                implicitHeight: root.px(28)
                text: qsTr("Channels")
                onClicked: root.openList()
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
        anchors.leftMargin: root.px(16)
        anchors.top: parent.top
        anchors.topMargin: root.px(16)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.bottomInset + root.px(16)
        width: Math.min(root.px(400), parent.width - root.px(32))
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
        anchors.top: parent.top
        anchors.topMargin: root.px(24)
        forceDark: true
    }
}
