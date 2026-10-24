/*****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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
import QtQuick.Layouts


import VLC.MainInterface
import VLC.Style
import VLC.Widgets as Widgets
import VLC.PlayerControls

FocusScope {
    id: playerControlLayout

    // Properties

    property real spacing: VLCStyle.margin_normal // spacing between controls

    property real layoutSpacing: VLCStyle.margin_xxlarge // spacing between layouts (left, center, and right)

    property int identifier: -1

    // A default profile with an empty center leaves room for the timeline.
    readonly property Item inlineContentHost: (width >= VLCStyle.dp(950, VLCStyle.scale))
                                             ? (loaderLeftRight.item?.inlineContentHost ?? null) : null
    property Item inlineFocusItem: null
    readonly property Item inlineLeftItem: loaderLeftRight.item?.lastLeftControl ?? null
    readonly property Item inlineRightItem: loaderLeftRight.item?.firstRightControl ?? null

    // Figma 2160:6137: the timeline on its row, nothing in the center (an
    // invisible spacer keeps the two rows)
    readonly property bool centerSpacerOnly: {
        const center = model ? model.center : null
        if (!center || center.count === 0)
            return false
        return center.countOf(ControlListModel.WIDGET_SPACER)
               + center.countOf(ControlListModel.WIDGET_SPACER_EXTEND) === center.count
    }

    readonly property PlayerControlbarModel model: {
        if (!!MainCtx.controlbarProfileModel.currentModel)
            return MainCtx.controlbarProfileModel.currentModel.getModel(identifier)
        else
            return null
    }

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.Window
    }

    // Signals

    signal requestLockUnlockAutoHide(bool lock)
    signal menuOpened(var menu)
    ///incomming signal to request all menu and popup to close
    signal forceUnlock()

    // Settings

    implicitWidth: loaderLeftRight.active ? loaderLeftRight.implicitWidth
                                          : (loaderLeft.implicitWidth + loaderCenter.implicitWidth + loaderRight.implicitWidth)

    implicitHeight: loaderLeftRight.active ? loaderLeftRight.implicitHeight
                                           : Math.max(loaderLeft.implicitHeight, loaderCenter.implicitHeight, loaderRight.implicitHeight)

    // Events

    Component.onCompleted: console.assert(identifier >= 0)

    // Children

    Loader {
        id: loaderLeftRight

        anchors.fill: parent

        active: !loaderCenter.active &&
                playerControlLayout.model &&
                ((playerControlLayout.model.left && (playerControlLayout.model.left.count > 0)) ||
                (playerControlLayout.model.right && (playerControlLayout.model.right.count > 0)))

        focus: active

        sourceComponent: RowLayout {
            property alias inlineContentHost: inlineContentHost
            readonly property Item lastLeftControl: leftRepeater.itemAt(leftRepeater.count - 1)
            readonly property Item firstRightControl: rightRepeater.itemAt(0)

            spacing: playerControlLayout.spacing

            focus: true

            // TODO: Qt >= 5.13 Use QConcatenateTablesProxyModel
            //       instead of multiple repeaters

            ControlRepeater {
                id: leftRepeater
                model: ControlListFilter {
                    sourceModel: playerControlLayout.model.left

                    player: Player
                    ctx: MainCtx
                }

                Navigation.parentItem: playerControlLayout
                Navigation.rightAction: function() {
                    if (playerControlLayout.inlineContentHost && playerControlLayout.inlineFocusItem) {
                        playerControlLayout.inlineFocusItem.forceActiveFocus(Qt.TabFocusReason)
                        return true
                    }
                    const item = rightRepeater.itemAt(0)
                    if (item)
                        item.forceActiveFocus(Qt.TabFocusReason)
                    else
                        return false
                }

                availableWidth: loaderLeftRight.width
                availableHeight: loaderLeftRight.height

                Component.onCompleted: {
                    requestLockUnlockAutoHide.connect(playerControlLayout.requestLockUnlockAutoHide)
                    menuOpened.connect(playerControlLayout.menuOpened)
                    playerControlLayout.forceUnlock.connect(leftRepeater.forceUnlock)
                }
            }

            Item {
                id: inlineContentHost

                function containsVisibleItem(repeater) {
                    for (let i = 0; i < repeater.count; ++i) {
                        const item = repeater.itemAt(i)

                        if (item && item.visible)
                            return true
                    }

                    return false
                }

                Layout.minimumWidth: (containsVisibleItem(leftRepeater) && containsVisibleItem(rightRepeater)) ? playerControlLayout.layoutSpacing
                                                                                                               : 0

                // Figma "Controls": the timeline 20 px from the buttons
                // around it (the layout already spaces them)
                Layout.leftMargin: playerControlLayout.inlineContentHost ? playerControlLayout.layoutSpacing - playerControlLayout.spacing : 0
                Layout.rightMargin: Layout.leftMargin

                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: true
            }

            ControlRepeater {
                id: rightRepeater
                model: ControlListFilter {
                    sourceModel: playerControlLayout.model.right

                    player: Player
                    ctx: MainCtx
                }

                Navigation.parentItem: playerControlLayout
                Navigation.leftAction: function() {
                    if (playerControlLayout.inlineContentHost && playerControlLayout.inlineFocusItem) {
                        playerControlLayout.inlineFocusItem.forceActiveFocus(Qt.BacktabFocusReason)
                        return true
                    }
                    const item = leftRepeater.itemAt(leftRepeater.count - 1)
                    if (item)
                        item.forceActiveFocus(Qt.BacktabFocusReason)
                    else
                        return false
                }

                availableWidth: loaderLeftRight.width
                availableHeight: loaderLeftRight.height

                // the left controls first, then the room between the two
                // (see ControlRepeater); the outer controls stay the longest
                fromEnd: true
                reservedWidth: leftRepeater.neededWidth + 2 * playerControlLayout.spacing
                               + playerControlLayout.layoutSpacing

                Component.onCompleted: {
                    requestLockUnlockAutoHide.connect(playerControlLayout.requestLockUnlockAutoHide)
                    menuOpened.connect(playerControlLayout.menuOpened)
                    playerControlLayout.forceUnlock.connect(rightRepeater.forceUnlock)
                }
            }

        }
    }


    Loader {
        id: loaderLeft

        anchors {
            right: loaderCenter.left
            left: parent.left
            top: parent.top
            bottom: parent.bottom

            // Spacing for the filler item acts as padding
            rightMargin: layoutSpacing - spacing
        }

        active: !!playerControlLayout.model?.left && (playerControlLayout.model.left.count > 0) &&
                !loaderLeftRight.active

        focus: active

        sourceComponent: ControlLayout {
            id: leftControlLayout

            model: ControlListFilter {
                sourceModel: playerControlLayout.model.left

                player: Player
                ctx: MainCtx
            }

            alignment: (Qt.AlignVCenter | Qt.AlignLeft)

            spacing: playerControlLayout.spacing

            focus: true

            altFocusAction: () => Navigation.defaultNavigationRight()

            Navigation.parentItem: playerControlLayout
            Navigation.rightItem: loaderCenter.item

            Component.onCompleted: {
                requestLockUnlockAutoHide.connect(playerControlLayout.requestLockUnlockAutoHide)
                menuOpened.connect(playerControlLayout.menuOpened)
                playerControlLayout.forceUnlock.connect(leftControlLayout.forceUnlock)
            }
        }
    }

    // Figma "Player - customize": the free room of the zones of the two rows
    // layout, dashed, where the controls drop (left of the center, right of
    // it): 9 from the center, 6 from the controls of the zone, 36 high
    Repeater {
        // one area across the empty center (Figma 2160:6137)
        model: (VLCStyle.customizing && loaderCenter.active) ? (playerControlLayout.centerSpacerOnly ? 1 : 2) : 0
        delegate: Item {
            id: emptyZone
            required property int index
            readonly property bool isLeft: index === 0
            readonly property var zoneModel: playerControlLayout.model ? (isLeft ? playerControlLayout.model.left
                                                                                : playerControlLayout.model.right) : null
            readonly property real centerGap: VLCStyle.dp(8, VLCStyle.scale)
            readonly property real controlsGap: VLCStyle.dp(5, VLCStyle.scale)
            // the volume's outline is 5 wider (see ControlRepeater): 6 from it
            readonly property real rightGap: {
                const right = playerControlLayout.model ? playerControlLayout.model.right : null
                if (!right || right.count === 0)
                    return controlsGap
                const first = right.data(right.index(0, 0), ControlListModel.ID_ROLE)
                return first === ControlListModel.VOLUME ? VLCStyle.dp(5 + 6, VLCStyle.scale) : controlsGap
            }
            readonly property bool zoneFilled: !!zoneModel && zoneModel.count > 0
            readonly property bool spanning: playerControlLayout.centerSpacerOnly
            readonly property bool leftFilled: !!playerControlLayout.model && playerControlLayout.model.left.count > 0
            readonly property bool rightFilled: !!playerControlLayout.model && playerControlLayout.model.right.count > 0
            readonly property real start: (isLeft || spanning)
                ? (leftFilled && loaderLeft.item ? loaderLeft.x + loaderLeft.item.lastControlRight + controlsGap : 0)
                : loaderCenter.x + loaderCenter.width + centerGap
            readonly property real end: (isLeft && !spanning)
                ? loaderCenter.x - centerGap
                : (rightFilled && loaderRight.item ? loaderRight.x + loaderRight.item.firstControlX - rightGap
                                                   : playerControlLayout.width)
            visible: !!zoneModel && width >= VLCStyle.dp(30, VLCStyle.scale)
            x: start
            width: Math.max(0, end - start)
            height: VLCStyle.dp(36, VLCStyle.scale)
            anchors.verticalCenter: parent.verticalCenter

            Widgets.DashedRect {
                anchors.fill: parent
                stretch: true
                corners: false
                // Figma 2160:6137: across the empty center, darker (#373737)
                color: emptyZone.spanning ? VLCStyle.redesign.color("#373737", "#8e8e8d")
                                          : VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
            }

            DropArea {
                anchors.fill: parent
                onDropped: (drop) => {
                    const source = drop.source
                    // the timeline back in the bar: one row again
                    if (source.objectName === "timeline") {
                        if (emptyZone.spanning) {
                            const center = playerControlLayout.model.center
                            Qt.callLater(() => center.clear())
                            drop.accept(Qt.MoveAction)
                        }
                        return
                    }
                    // across the empty center: the controls go there
                    const zone = emptyZone.spanning ? playerControlLayout.model.center : emptyZone.zoneModel
                    // after the event (see ControlRepeater)
                    if (source.objectName === "buttonsList") {
                        const id = source.mIndex
                        Qt.callLater(() => zone.insert(zone.count, { "id": id }))
                        drop.accept(Qt.CopyAction)
                    } else if (source.objectName === "playerControl" && source.zoneModel) {
                        const id = source.controlId
                        const fromZone = source.zoneModel
                        const from = source.sourceRow
                        Qt.callLater(() => {
                            if (from < 0 || from >= fromZone.count)
                                return
                            fromZone.remove(from)
                            zone.insert(zone.count, { "id": id })
                        })
                        drop.accept(Qt.MoveAction)
                    }
                }
            }
        }
    }

    Loader {
        id: loaderCenter

        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            bottom: parent.bottom
        }

        // TODO: "ControlListFilter"'s count......
        active: !!playerControlLayout.model && !!playerControlLayout.model.center && (playerControlLayout.model.center.count > 0)

        Binding on width {
            delayed: true
            when: loaderCenter._componentCompleted
            value: {
                const item = loaderCenter.item

                const minimumWidth = (item && item.Layout.minimumWidth > 0) ? item.Layout.minimumWidth : implicitWidth
                const maximumWidth = (item && item.Layout.maximumWidth > 0) ? item.Layout.maximumWidth : implicitWidth

                if ((loaderLeft.active && (loaderLeft.width > 0)) || (loaderRight.active && (loaderRight.width > 0))) {
                    return minimumWidth
                } else {
                    return Math.min(loaderCenter.parent.width, maximumWidth)
                }
            }
        }

        property bool _componentCompleted: false

        Component.onCompleted: {
            _componentCompleted = true
        }

        sourceComponent: ControlLayout {
            id: centerControlLayout

            model: ControlListFilter {
                sourceModel: playerControlLayout.model.center

                player: Player
                ctx: MainCtx
            }

            focus: true

            spacing: playerControlLayout.spacing

            altFocusAction: () => Navigation.defaultNavigationUp()

            Navigation.parentItem: playerControlLayout
            Navigation.leftItem: loaderLeft.item
            Navigation.rightItem: loaderRight.item

            Component.onCompleted: {
                requestLockUnlockAutoHide.connect(playerControlLayout.requestLockUnlockAutoHide)
                menuOpened.connect(playerControlLayout.menuOpened)
                playerControlLayout.forceUnlock.connect(centerControlLayout.forceUnlock)
            }
        }
    }

    Loader {
        id: loaderRight

        anchors {
            left: loaderCenter.right
            right: parent.right
            top: parent.top
            bottom: parent.bottom

            // Spacing for the filler item acts as padding
            leftMargin: layoutSpacing - spacing
        }

        active: !!playerControlLayout.model && !!playerControlLayout.model.right && (playerControlLayout.model.right.count > 0) &&
                !loaderLeftRight.active

        sourceComponent: ControlLayout {
            id: rightControlLayout

            model: ControlListFilter {
                sourceModel: playerControlLayout.model.right

                player: Player
                ctx: MainCtx
            }

            alignment: (Qt.AlignVCenter | Qt.AlignRight)

            spacing: playerControlLayout.spacing

            focus: true

            altFocusAction: () => Navigation.defaultNavigationLeft()

            Navigation.parentItem: playerControlLayout
            Navigation.leftItem: loaderCenter.item

            onRequestLockUnlockAutoHide: (lock) => playerControlLayout.requestLockUnlockAutoHide(lock)

            onMenuOpened: (menu) => playerControlLayout.menuOpened(menu)

            Component.onCompleted:{
                playerControlLayout.forceUnlock.connect(rightControlLayout.forceUnlock)
            }
        }
    }
}
