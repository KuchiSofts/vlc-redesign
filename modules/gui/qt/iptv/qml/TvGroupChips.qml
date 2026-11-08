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

// The groups of Live TV as chips (the segmented controls of the design): All,
// the favourites, the ones watched last, then the provider's, with their
// counts. A click shows the group in the list of `channels` (the TV page's,
// the library next to the video's).
ListView {
    id: root

    // the channels the chips filter: Iptv.channelModel, Iptv.playerModel
    property var channels: null

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    // (the light theme from the map of the design: VLCStyle.redesign.color)
    function c(dark, light) { return VLCStyle.redesign.color(dark) }

    readonly property var groups: [{ name: "", label: qsTr("All"), count: Iptv.channelCount },
                                   { name: "favorites", label: qsTr("Favorites"), count: Iptv.favorites.length },
                                   { name: "recent", label: qsTr("Recently Watched"), count: -1 }]
                                  .concat(Iptv.groups.map(g => ({ name: g.name, label: g.name, count: g.count })))

    // the chip of the group shown, in view (the provider's groups run past
    // the width of the list)
    function showCurrent() {
        const i = groups.findIndex(g => g.name === (channels ? channels.group : ""))
        if (i >= 0)
            positionViewAtIndex(i, ListView.Contain)
    }

    orientation: ListView.Horizontal
    spacing: px(8)
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    model: groups

    Connections {
        target: root.channels
        function onFilterChanged() { Qt.callLater(root.showCurrent) }
    }
    // (a count changed, a favourite added: the chips made again from the
    // start)
    onGroupsChanged: Qt.callLater(root.showCurrent)

    // The wheel of a mouse goes along the chips (a list across takes the
    // horizontal wheel only), forward to the provider's groups
    WheelHandler {
        enabled: root.contentWidth > root.width
        onWheel: (event) => {
            const delta = event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x
            const step = root.effectiveLayoutDirection === Qt.RightToLeft ? delta : -delta
            root.contentX = Math.max(root.originX,
                                     Math.min(root.originX + root.contentWidth - root.width, root.contentX + step))
        }
    }

    delegate: T.Button {
        id: chip
        required property var modelData
        readonly property bool selected: !!root.channels && root.channels.group === modelData.name
        height: root.height
        width: chipText.implicitWidth + root.px(24)
        focusPolicy: Qt.StrongFocus
        Accessible.role: Accessible.RadioButton
        Accessible.name: modelData.label
        Accessible.checkable: true
        Accessible.checked: selected
        onClicked: if (root.channels) root.channels.group = modelData.name

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
