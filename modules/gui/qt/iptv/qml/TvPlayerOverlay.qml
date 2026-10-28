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
        console.debug("Live TV: the overlay of the player is made")
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

    // the pointer at the bottom of the video, where the banner shows, brings
    // it back with its buttons (the clicks go through to the video)
    Item {
        id: bannerZone
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.bottomInset
        width: Math.min(root.px(680), parent.width)
        height: root.px(240)
        HoverHandler { id: zoneHover }
    }

    // ---- the banner ----
    Rectangle {
        id: banner
        readonly property bool wanted: bannerTimer.running || bannerHover.hovered || zoneHover.hovered
        visible: opacity > 0
        opacity: wanted && Iptv.bannerMode !== 3 ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: VLCStyle.duration_short } }
        anchors.left: parent.left
        anchors.leftMargin: root.px(24)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.bottomInset + root.px(16)
        width: Math.min(root.px(620), parent.width - root.px(48))
        height: bannerColumn.implicitHeight + root.px(32)
        radius: root.px(14)
        color: Qt.rgba(25 / 255, 25 / 255, 25 / 255, 0.95)

        HoverHandler { id: bannerHover }

        Text {
            id: bannerNumber
            x: root.px(18)
            y: root.px(18)
            width: root.px(44)
            horizontalAlignment: Text.AlignRight
            text: root.channel.number > 0 ? root.channel.number : ""
            color: root.c("#969696")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(24)
            font.weight: Font.DemiBold
        }
        TvLogo {
            id: bannerLogo
            anchors.left: bannerNumber.right
            anchors.leftMargin: root.px(14)
            y: root.px(16)
            width: root.px(56)
            height: width
            forceDark: true
            source: root.channel.logo ?? ""
            name: root.channel.name ?? ""
        }

        Column {
            id: bannerColumn
            anchors.left: bannerLogo.right
            anchors.leftMargin: root.px(16)
            anchors.right: parent.right
            anchors.rightMargin: root.px(18)
            y: root.px(16)
            spacing: root.px(6)

            Row {
                width: parent.width
                spacing: root.px(8)
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(implicitWidth, parent.width - root.px(120))
                    text: root.channel.name ?? ""
                    elide: Text.ElideRight
                    color: root.c(VLCStyle.redesign.textPrimary)
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(16)
                    font.weight: Font.DemiBold
                }
                TvBadge {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root._archive
                    forceDark: true
                    accent: true
                    icon: "recent"
                    text: qsTr("Archive")
                }
                TvBadge {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !root._archive && (root.channel.catchupDays ?? 0) > 0
                    forceDark: true
                    icon: "recent"
                    text: qsTr("%1 d", "%1 d", root.channel.catchupDays ?? 0).arg(root.channel.catchupDays ?? 0)
                    tip: qsTr("Catch-up: the past %1 day", "Catch-up: the past %1 days", root.channel.catchupDays ?? 0)
                             .arg(root.channel.catchupDays ?? 0)
                }
            }

            // what plays from the archive
            Text {
                visible: root._archive
                width: parent.width
                elide: Text.ElideRight
                text: qsTr("%1, %2 – %3").arg(root.catchup.title || qsTr("Programme"))
                          .arg(Iptv.dayText(root.catchup.start ?? 0) + " " + Iptv.timeText(root.catchup.start ?? 0))
                          .arg(Iptv.timeText(root.catchup.stop ?? 0))
                color: root.c("#C6C6C6")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
            }

            // what is on: its times, its title, its progress and what is left
            Column {
                visible: !root._archive && Iptv.bannerMode !== 2
                width: parent.width
                spacing: root.px(6)

                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    text: root._hasNow ? Iptv.timeText(root.now.start) + " – " + Iptv.timeText(root.now.stop)
                                         + "   " + root.now.title
                                       : (root.channel.hasGuide ? qsTr("Loading the guide…") : qsTr("No guide for this channel"))
                    color: root.c("#C6C6C6")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(14)
                }
                Row {
                    visible: root._hasNow && Iptv.showProgress
                    spacing: root.px(10)
                    // a time axis: from the left in every language
                    Rectangle {
                        LayoutMirroring.enabled: false
                        anchors.verticalCenter: parent.verticalCenter
                        width: root.px(220)
                        height: root.px(4)
                        radius: height / 2
                        color: root.c("#373737")
                        Rectangle {
                            height: parent.height
                            radius: parent.radius
                            color: VLCStyle.redesign.accent
                            width: root._hasNow && root.now.stop > root.now.start
                                   ? Math.round(parent.width * Math.max(0, Math.min(1, (Iptv.now - root.now.start)
                                                                                      / (root.now.stop - root.now.start))))
                                   : 0
                        }
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("%1 left").arg(Iptv.durationText((root.now.stop ?? 0) - Iptv.now))
                        color: root.c("#6B6B6B")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(12)
                    }
                }
                Text {
                    visible: Iptv.showDescriptions && (root.now.description ?? "") !== ""
                    width: parent.width
                    text: (root.now.description ?? "").replace(/\s+/g, " ")
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    color: root.c("#8F8F8F")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(12)
                }
                Text {
                    visible: Iptv.bannerMode === 0 && Iptv.showNext && (root.next.title ?? "") !== ""
                    width: parent.width
                    elide: Text.ElideRight
                    text: qsTr("Next at %1").arg(Iptv.timeText(root.next.start ?? 0)) + "   " + (root.next.title ?? "")
                    color: root.c("#6B6B6B")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(12)
                }
            }

            // the ways from here: the channels up and down, their list, the
            // start of what is on, back to live
            Row {
                spacing: root.px(8)
                Widgets.RedesignButton {
                    darkTheme: true
                    implicitWidth: root.px(40)
                    text: "▲"
                    Accessible.name: qsTr("Next channel")
                    onClicked: Iptv.channelUp()
                }
                Widgets.RedesignButton {
                    darkTheme: true
                    implicitWidth: root.px(40)
                    text: "▼"
                    Accessible.name: qsTr("Previous channel")
                    onClicked: Iptv.channelDown()
                }
                Widgets.RedesignButton {
                    darkTheme: true
                    text: qsTr("Channels")
                    onClicked: root.openList()
                }
                Widgets.RedesignButton {
                    darkTheme: true
                    visible: !root._archive && (root.channel.catchupDays ?? 0) > 0 && root._hasNow
                    text: qsTr("Watch from the Start")
                    onClicked: Iptv.playFromStart()
                }
                Widgets.RedesignButton {
                    darkTheme: true
                    visible: root._archive
                    accent: true
                    text: qsTr("Back to Live")
                    onClicked: Iptv.backToLive()
                }
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
