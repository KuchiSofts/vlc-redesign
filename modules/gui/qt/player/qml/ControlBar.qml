/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import QtQml.Models


import VLC.MainInterface
import VLC.Style
import VLC.Widgets as Widgets
import VLC.Player
import VLC.PlayerControls
import VLC.Util

T.Pane {
    id: root

    enum TimeTextPosition {
        Hide,
        AboveSlider,
        LeftRightSlider
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    // the background under both rows
    topInset: 0

    bottomPadding: VLCStyle.margin_xsmall

    readonly property real sliderY: _mapSlider(0).y
    property int textPosition: ControlBar.TimeTextPosition.AboveSlider
    property int identifier: -1
    property real sliderHeight: VLCStyle.redesign.seekHeight
    property real bookmarksHeight: VLCStyle.controlBarBookmarksHeight
    property bool showRemainingTime: true
    readonly property bool inlineTimeline: playerControlLayout.inlineContentHost !== null
    // The design draws the times beside the track in both layouts
    readonly property int effectiveTextPosition: ControlBar.TimeTextPosition.LeftRightSlider
    // Figma: rows of 54, the controls (28) centered
    readonly property real _rowHeight: VLCStyle.redesign.controlHeight
    readonly property real _rowPadding: (VLCStyle.redesign.controlHeight - VLCStyle.redesign.playHeight) / 2


    // Figma "Player - customize": a line above each row while customizing
    Repeater {
        model: (VLCStyle.customizing && !root.inlineTimeline) ? 2 : 0
        delegate: Rectangle {
            required property int index
            parent: root.background
            y: index * root._rowHeight
            width: parent ? parent.width : 0
            height: VLCStyle.dp(1, VLCStyle.scale)
            color: VLCStyle.redesign.color(index === 0 ? VLCStyle.redesign.separator : "#212121", "#9c9c9b")
        }
    }

    // Figma "Controls": the row(s) of 54, 20 from the sides (one binding
    // per property: two exclusive ones restore the value of the other)
    Binding on topPadding {
        value: root.inlineTimeline ? (VLCStyle.redesign.controlHeight - VLCStyle.redesign.playHeight) / 2 : 0
    }
    Binding on bottomPadding {
        value: root.inlineTimeline
               ? Math.max(VLCStyle.applicationVerticalMargin + VLCStyle.margin_xsmall,
                          (VLCStyle.redesign.controlHeight - VLCStyle.redesign.playHeight) / 2)
               : Math.max(VLCStyle.applicationVerticalMargin, root._rowPadding)
    }
    Binding on leftPadding {
        value: Math.max(VLCStyle.applicationHorizontalMargin, VLCStyle.redesign.controlPadding)
    }
    Binding on rightPadding {
        value: Math.max(VLCStyle.applicationHorizontalMargin, VLCStyle.redesign.controlPadding)
    }
    Binding {
        target: root.background
        property: "color"
        when: root.inlineTimeline && root.background !== null
        // Figma "Controls" type=music: #191919 at 95 % over the backdrop
        value: (!MainCtx.hasEmbededVideo && Player.isStarted)
               ? VLCStyle.redesign.color(Qt.rgba(25 / 255, 25 / 255, 25 / 255, 0.95), "#ababaa", VLCStyle.palette.isDark)
               : VLCStyle.redesign.color(VLCStyle.redesign.playerControlBackground, "#ababaa", VLCStyle.palette.isDark)
        restoreMode: Binding.RestoreBinding
    }

    // Figma "Controls" type=music: a 1 px border on top, inside
    Rectangle {
        parent: root.background
        visible: root.inlineTimeline && !MainCtx.hasEmbededVideo && Player.isStarted
        width: parent ? parent.width : 0
        height: VLCStyle.dp(1, VLCStyle.scale)
        color: VLCStyle.redesign.color("#222222", "#9c9c9b", VLCStyle.palette.isDark)
    }

    Binding {
        target: root.background
        property: "visible"
        when: root.inlineTimeline && root.background !== null
        value: true
        restoreMode: Binding.RestoreBinding
    }
    Binding {
        target: root.background
        property: "opacity"
        when: root.inlineTimeline && root.background !== null
        value: 1
        restoreMode: Binding.RestoreBinding
    }

    property var menu: undefined

    signal requestLockUnlockAutoHide(bool lock)

    ///incomming signal to request all menu and popup to close
    signal forceUnlock()

    enabled: visible

    Keys.priority: Keys.AfterItem
    Keys.onPressed: (event) => root.Navigation.defaultKeyAction(event)
    Navigation.cancelAction: function() { History.previous(Qt.BacktabFocusReason) }

    Accessible.name: qsTr("Player controls")
    AccessibleCompat.id: "playerControlBar"

    function showChapterMarks() {
        if (contentItem.trackPositionSlider)
            contentItem.trackPositionSlider.showChapterMarks()
    }

    function applyMenu(menu) {
        if (root.menu === menu)
            return

        // NOTE: When applying a new menu we hide the previous one.
        if (menu)
            dismiss()

        root.menu = menu
    }

    // mapToItem() is not notified: the bindings that map the slider read
    // here what moves it (one row or two, the controls beside it)
    function _mapSlider(y) {
        const host = row2.parent
        void [trackPositionSlider.x, trackPositionSlider.y, row2.x, row2.y,
              host ? host.x : 0, host ? host.y : 0,
              playerControlLayout.x, playerControlLayout.y,
              root.contentItem ? root.contentItem.x : 0, root.contentItem ? root.contentItem.y : 0]
        return trackPositionSlider.mapToItem(root, 0, y)
    }

    function dismiss() {
        if ((typeof menu === undefined) || !menu)
            return
        if (menu.hasOwnProperty("dismiss"))
            menu.dismiss()
        else if (menu.hasOwnProperty("close"))
            menu.close()
    }

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        palette: VLCStyle.palette
        colorSet: ColorContext.Window
    }

    FontMetrics {
        id: timeFontMetrics
        font: timeTextMetrics.font
    }

    TextMetrics {
        id: timeTextMetrics
        font.pixelSize: VLCStyle.redesign.fontControl
        font.family: VLCStyle.redesign.fontFamily
        text: {
            // This text metrics is used for both the time and length, as we want them to have the same width.
            // Length might not be reported in some cases, for that reason we should check both and use the bigger one.

            // Some extra space to compensate non-monospaced fonts ("-" takes more space than ":", double "-" for guarantee)
            // The hours are always shown (VLCTick.ForceHours)
            return "00--00--00"
        }
    }

    background: Rectangle {
        color: theme.bg.primary
    }

    contentItem: ColumnLayout {
        spacing: 0

        property alias trackPositionSlider: trackPositionSlider

        property alias mediaRemainingTime: mediaRemainingTime

        property alias playerControlLayout: playerControlLayout

        readonly property list<Item> strayItems: [
            T.Label {
                id: mediaTime

                Layout.preferredWidth: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider) ? VLCStyle.redesign.timeWidth : -1

                text: {
                    const length = Player.length
                    // The design always shows the hours: 00:02:10
                    return Player.time.formatHMS(VLCTick.ForceHours
                                                 | (length.isSubSecond() ? VLCTick.SubSecondFormattedAsMS : 0))
                }
                color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, theme.fg.primary, theme.palette?.isDark ?? VLCStyle.darkPalette.isDark)
                font: timeTextMetrics.font

                // Figma "Current Time": the text box (17 px, a line of 100 %)
                // centred in the frame, the glyphs from its left edge
                Layout.fillHeight: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider)
                verticalAlignment: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider) ? Text.AlignTop : Text.AlignVCenter
                horizontalAlignment: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider) ? Text.AlignLeft : Text.AlignHCenter
                topPadding: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider)
                            ? (height - VLCStyle.dp(17, VLCStyle.scale)) / 2 + font.pixelSize * (1 - 1.2109375) / 2
                              + 0.96875 * font.pixelSize - timeFontMetrics.ascent
                            : 0
                // Figma's box: the advance of Inter 3.19 rounded up (measured 16
                // times larger, less the unit per glyph the bundled font adds)
                leftPadding: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider)
                             ? (width - Math.ceil(timeExact.advanceWidth / 16 - text.length * font.pixelSize / 2816 - 0.001)) / 2
                             : 0

                TextMetrics {
                    id: timeExact
                    font.family: mediaTime.font.family
                    font.pixelSize: mediaTime.font.pixelSize * 16
                    text: mediaTime.text
                }

                // TODO: Do not use `Binding` when minimum Qt is 6.12:
                Binding {
                    target: mediaTime
                    property: "mutabilityGroup"
                    when: (mediaTime.mutabilityGroup !== undefined)
                    value: (Player.playingState === Player.PLAYING_STATE_PLAYING) ? Item.ModerateMutabilityGroup
                                                                                  : Item.StaticMutabilityGroup
                }

                anchors.left: (parent === pseudoRow) ? parent.left : undefined
                anchors.verticalCenter: (parent === pseudoRow) ? parent.verticalCenter : undefined
            },
            T.Label {
                id: mediaRemainingTime

                Layout.preferredWidth: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider) ? VLCStyle.redesign.timeWidth : -1

                readonly property bool showRemainingTime: (MainCtx.showRemainingTime && Player.remainingTime.valid())

                text: showRemainingTime
                      ? "-" + Player.remainingTime.formatHMS(VLCTick.ForceHours | VLCTick.SubSecondFormattedAsMS)
                      : Player.length.formatHMS(VLCTick.ForceHours | VLCTick.SubSecondFormattedAsMS)
                color: mediaTime.color
                font: timeTextMetrics.font

                // Figma "Current Time": the text box (17 px, a line of 100 %)
                // centred in the frame, the glyphs from its left edge
                Layout.fillHeight: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider)
                verticalAlignment: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider) ? Text.AlignTop : Text.AlignVCenter
                horizontalAlignment: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider) ? Text.AlignLeft : Text.AlignHCenter
                topPadding: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider)
                            ? (height - VLCStyle.dp(17, VLCStyle.scale)) / 2 + font.pixelSize * (1 - 1.2109375) / 2
                              + 0.96875 * font.pixelSize - timeFontMetrics.ascent
                            : 0
                // Figma's box: the advance of Inter 3.19 rounded up (measured 16
                // times larger, less the unit per glyph the bundled font adds)
                leftPadding: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider)
                             ? (width - Math.ceil(remainingExact.advanceWidth / 16 - text.length * font.pixelSize / 2816 - 0.001)) / 2
                             : 0

                TextMetrics {
                    id: remainingExact
                    font.family: mediaRemainingTime.font.family
                    font.pixelSize: mediaRemainingTime.font.pixelSize * 16
                    text: mediaRemainingTime.text
                }

                visible: root.showRemainingTime

                // TODO: Do not use `Binding` when minimum Qt is 6.12:
                Binding {
                    target: mediaRemainingTime
                    property: "mutabilityGroup"
                    when: (mediaRemainingTime.mutabilityGroup !== undefined)
                    value: ((Player.playingState === Player.PLAYING_STATE_PLAYING) &&
                            mediaRemainingTime.showRemainingTime) ? Item.ModerateMutabilityGroup
                                                                  : Item.StaticMutabilityGroup
                }

                anchors.right: (parent === pseudoRow) ? parent.right : undefined
                anchors.verticalCenter: (parent === pseudoRow) ? parent.verticalCenter : undefined

                TapHandler {
                    onTapped: (eventPoint, button) => {
                        MainCtx.showRemainingTime = !MainCtx.showRemainingTime
                    }
                }

                HoverHandler {
                    id: mediaRemainingTimeHover
                }

                Widgets.ToolTipExt {
                    colorContext.palette: theme.palette

                    visible: mediaRemainingTimeHover.hovered && MainCtx.showRemainingTime && Player.rate !== 1
                    delay: VLCStyle.delayToolTipAppear
                    text: {
                        if (!visible)
                            return ""

                        const playbackSpeed = Math.abs(Player.rate)
                        const adjustedRemainingTime = Player.remainingTime.scale(1 / playbackSpeed)
                            ? Player.remainingTime.scale(1 / playbackSpeed)
                            : Player.remainingTime

                        return qsTr("%1 (%2x)")
                            .arg(adjustedRemainingTime.formatHMS())
                            .arg(+Player.rate.toFixed(2))
                    }

                    x: (parent.width - width) / 2
                }
            },
            WaveformSlider {
                id: trackPositionSlider

                barHeight: root.sliderHeight
                redesign: true

                Accessible.role: Accessible.Slider
                Accessible.name: qsTr("Progress")
                AccessibleCompat.id: "progressSlider"

                Layout.fillWidth: true
                enabled: Player.playingState === Player.PLAYING_STATE_PLAYING || Player.playingState === Player.PLAYING_STATE_PAUSED

                Navigation.parentItem: root
                Navigation.downItem: playerControlLayout
                Navigation.leftItem: inlineTimeline ? playerControlLayout.inlineLeftItem : null
                Navigation.rightItem: inlineTimeline ? playerControlLayout.inlineRightItem : null

                activeFocusOnTab: true

                focus: true

                Keys.onPressed: (event) => {
                    Navigation.defaultKeyAction(event)
                }
            },
            Loader {
                id: bookmarksLoader

                parent: root
                active: MainCtx.mediaLibraryAvailable
                source: "qrc:///qt/qml/VLC/MediaLibrary/Bookmarks.qml"

                x: root._mapSlider(0).x
                y: root._mapSlider(trackPositionSlider.height).y + VLCStyle.margin_xxsmall
                width: trackPositionSlider.width

                onLoaded: {
                   item.barHeight = Qt.binding(function() { return bookmarksHeight })
                   item.controlBarHovered = Qt.binding(function() { return root.hovered })
                   item.yShift = Qt.binding(function() { return row2.height + VLCStyle.margin_xxsmall })
                }
            }
        ]

        Item {
            // BUG: RowLayout can not be used here
            // because of a Qt bug. (Height is
            // incorrectly determined. Could be
            // about nested layouting).

            id: pseudoRow
            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.leftMargin: VLCStyle.margin_normal
            Layout.rightMargin: VLCStyle.margin_normal

            implicitHeight: visible ? Math.max(mediaTime.implicitHeight, mediaRemainingTime.implicitHeight) : 0

            visible: children.length > 0

            children: {
                switch (effectiveTextPosition) {
                case ControlBar.TimeTextPosition.AboveSlider:
                    return [mediaTime, mediaRemainingTime]
                case ControlBar.TimeTextPosition.Hide:
                case ControlBar.TimeTextPosition.LeftRightSlider:
                default:
                    return []
                }
            }
        }

        // Figma "Player - customize": the room of the row above the bar
        Rectangle {
            parent: root
            visible: VLCStyle.customizing && root.inlineTimeline
            y: -height
            width: root.width
            height: VLCStyle.dp(54, VLCStyle.scale)
            color: VLCStyle.redesign.color(VLCStyle.redesign.playerControlBackground, "#ababaa")
            Rectangle {
                width: parent.width
                height: VLCStyle.dp(1, VLCStyle.scale)
                color: VLCStyle.redesign.color(VLCStyle.redesign.separator, "#9c9c9b")
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: VLCStyle.dp(1, VLCStyle.scale)
                color: VLCStyle.redesign.color("#212121", "#9c9c9b")
            }

            // Figma 2160:6137: the timeline dropped here takes this row
            DropArea {
                anchors.fill: parent
                onEntered: (drag) => { drag.accepted = !!drag.source && drag.source.objectName === "timeline" }
                onDropped: (drop) => {
                    const center = playerControlLayout.model ? playerControlLayout.model.center : null
                    if (!center)
                        return
                    Qt.callLater(() => center.insert(0, { "id": ControlListModel.WIDGET_SPACER }))
                    drop.accept(Qt.MoveAction)
                }
            }
        }

        // Figma "Player - customize": the timeline outlined, dragged to
        // its own row (2160:6137) and back; a control dropped on it goes
        // in the center of the bar
        Widgets.DashedRect {
            parent: row2.parent
            visible: VLCStyle.customizing
            z: 100
            x: row2.x - (root.inlineTimeline ? VLCStyle.dp(4, VLCStyle.scale) : 0)
            y: row2.y + Math.round((row2.height - height) / 2)
            width: row2.width + (root.inlineTimeline ? VLCStyle.dp(8, VLCStyle.scale) : 0)
            height: VLCStyle.dp(36, VLCStyle.scale)
            stretch: true
            corners: false
            color: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")

            // the timeline dragged: on its row only from the one row bar,
            // back only from its row above an empty center
            MouseArea {
                id: timelineDrag
                anchors.fill: parent
                enabled: root.inlineTimeline || playerControlLayout.centerSpacerOnly
                cursorShape: drag.active ? Qt.DragMoveCursor : Qt.OpenHandCursor
                drag.target: timelineProxy
                drag.smoothed: false
                onPressed: (mouse) => {
                    timelineProxy.x = mouse.x - timelineProxy.width / 2
                    timelineProxy.y = mouse.y - timelineProxy.height / 2
                }
                drag.onActiveChanged: {
                    if (drag.active) {
                        timelineProxy.Drag.start()
                    } else {
                        timelineProxy.Drag.drop()
                        timelineProxy.x = 0
                        timelineProxy.y = 0
                    }
                }

                // what is dragged: a short timeline in a dashed box
                Item {
                    id: timelineProxy
                    objectName: "timeline"
                    parent: Drag.active ? T.Overlay.overlay : timelineDrag
                    visible: Drag.active
                    width: VLCStyle.dp(160, VLCStyle.scale)
                    height: VLCStyle.dp(36, VLCStyle.scale)
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2
                    Drag.source: timelineProxy

                    Rectangle {
                        anchors.fill: parent
                        radius: VLCStyle.dp(4, VLCStyle.scale)
                        color: VLCStyle.redesign.color(VLCStyle.redesign.playerControlBackground, "#ababaa")
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width - VLCStyle.dp(16, VLCStyle.scale)
                        height: VLCStyle.dp(12, VLCStyle.scale)
                        radius: VLCStyle.dp(4, VLCStyle.scale)
                        color: VLCStyle.redesign.color(VLCStyle.redesign.seekBackground, "#dedede")
                    }
                    Widgets.DashedRect {
                        anchors.fill: parent
                        color: VLCStyle.redesign.accent
                    }
                }
            }
        }
        DropArea {
            parent: row2.parent
            anchors.fill: parent
            enabled: VLCStyle.customizing && root.inlineTimeline
            z: 101
            onDropped: (drop) => {
                const center = playerControlLayout.model ? playerControlLayout.model.center : null
                if (!center)
                    return
                const source = drop.source
                // after the event (see ControlRepeater)
                if (source.objectName === "buttonsList") {
                    const id = source.mIndex
                    Qt.callLater(() => center.insert(center.count, { "id": id }))
                    drop.accept(Qt.CopyAction)
                } else if (source.objectName === "playerControl" && source.zoneModel) {
                    const id = source.controlId
                    const fromZone = source.zoneModel
                    const from = source.sourceRow
                    Qt.callLater(() => {
                        if (from < 0 || from >= fromZone.count)
                            return
                        fromZone.remove(from)
                        center.insert(center.count, { "id": id })
                    })
                    drop.accept(Qt.MoveAction)
                }
            }
        }

        // Figma: the upper row (54), the timeline centered in it
        Item {
            id: timelinePlaceholder
            visible: !inlineTimeline
            Layout.fillWidth: true
            Layout.preferredHeight: inlineTimeline ? 0 : root._rowHeight
            implicitHeight: inlineTimeline ? 0 : root._rowHeight

            RowLayout {
                id: row2

                parent: inlineTimeline ? playerControlLayout.inlineContentHost : timelinePlaceholder
                anchors.left: parent ? parent.left : undefined
                anchors.right: parent ? parent.right : undefined
                anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                height: inlineTimeline ? (parent ? parent.height : 0) : VLCStyle.redesign.seekHeight

                children: {
                    switch (effectiveTextPosition) {
                    case ControlBar.TimeTextPosition.Hide:
                    case ControlBar.TimeTextPosition.AboveSlider:
                        return [trackPositionSlider]
                    case ControlBar.TimeTextPosition.LeftRightSlider:
                        return [mediaTime, trackPositionSlider, mediaRemainingTime]
                    default:
                        return []
                    }
                }

                visible: children.length > 0

                spacing: VLCStyle.redesign.controlInnerSpacing

                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.leftMargin: (effectiveTextPosition === ControlBar.TimeTextPosition.LeftRightSlider) ? VLCStyle.margin_xsmall
                                                                                                  : 0

                Layout.rightMargin: Layout.leftMargin
            }
        }

        PlayerControlLayout {
            id: playerControlLayout

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            // the lower row: the controls centered in 54
            Layout.topMargin: root.inlineTimeline ? 0 : root._rowPadding
            spacing: VLCStyle.redesign.controlInnerSpacing
            layoutSpacing: VLCStyle.redesign.controlSpacing

            identifier: root.identifier
            inlineFocusItem: trackPositionSlider.enabled ? trackPositionSlider : null

            implicitHeight: VLCStyle.redesign.playHeight

            visible: !model?.empty ?? false

            Navigation.upItem: trackPositionSlider.enabled ? trackPositionSlider : root.Navigation.upItem

            onRequestLockUnlockAutoHide: (lock) => root.requestLockUnlockAutoHide(lock)

            onMenuOpened: (menu) => root.applyMenu(menu)

            Component.onCompleted: {
                root.forceUnlock.connect(playerControlLayout.forceUnlock)
            }
        }
    }
}
