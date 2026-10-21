/*****************************************************************************
 * Copyright (C) 2023 VLC authors and VideoLAN
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
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.PlayerControls
import VLC.Style
import VLC.Widgets as Widgets

Repeater {
    id: repeater

    property int alignment: Qt.AlignVCenter

    property real availableWidth: Number.MAX_VALUE
    property real availableHeight: Number.MAX_VALUE

    signal requestLockUnlockAutoHide(bool lock)
    signal menuOpened(var menu)

    // Figma "Controls": the buttons are grouped, 8 px apart among the
    // playback buttons, 12 px among the tool buttons (audio effects,
    // subtitles, settings), 20 px from one group to the other
    function designGroup(id) {
        switch (id) {
        case ControlListModel.PREVIOUS_BUTTON:
        case ControlListModel.PLAY_BUTTON:
        case ControlListModel.NEXT_BUTTON:
        case ControlListModel.SKIP_BACK_BUTTON:
        case ControlListModel.SKIP_FW_BUTTON:
        case ControlListModel.SLOWER_BUTTON:
        case ControlListModel.FASTER_BUTTON:
        case ControlListModel.STOP_BUTTON:
            return 1
        case ControlListModel.EXTENDED_BUTTON:
        case ControlListModel.LANG_BUTTON:
        case ControlListModel.SETTINGS_BUTTON:
            return 2
        // the items of the top bar (Figma 2160:8732)
        case ControlListModel.MENU_BUTTON:
        case ControlListModel.BACK_BUTTON:
            return 3
        default:
            return 0
        }
    }

    // the folder after the ⋯ menu or the library toggle: the divider of the
    // top bar between them (24, no gap)
    function _dividedFolder(previousId, id) {
        return id === ControlListModel.FOLDER_NAME_WIDGET && designGroup(previousId) === 3
    }

    function designGap(previousId, id) {
        if (_dividedFolder(previousId, id))
            return VLCStyle.dp(24, VLCStyle.scale)
        const group = designGroup(id)
        if (group !== 0 && group === designGroup(previousId))
            return VLCStyle.dp(group === 2 ? 12 : group === 3 ? 7 : 8, VLCStyle.scale)
        return VLCStyle.dp(20, VLCStyle.scale)
    }
    ///incomming signal to request all menu and popup to close
    signal forceUnlock()

    // NOTE: We apply the 'navigation chain' after adding the item.
    onItemAdded: (index, item) => {
        item.applyNavigation()
    }

    onItemRemoved: (index, item) => {
        // NOTE: We update the 'navigation chain' after removing the item.
        item.removeNavigation()

        item.recoverFocus(index)
    }

    delegate: Loader {
        id: loader

        // Settings

        source: PlayerControlbarControls.control(model.id).source

        readonly property int controlId: model.id

        // Figma 2160:8732: the divider icon of the top bar before the folder
        Image {
            readonly property var previous: index > 0 ? repeater.itemAt(index - 1) : null
            visible: !!previous && repeater._dividedFolder(previous.controlId, loader.controlId)
            x: -width
            anchors.verticalCenter: parent.verticalCenter
            width: VLCStyle.redesign.iconSize
            height: width
            sourceSize: Qt.size(width, height)
            source: "qrc:///redesign/icons/divider/default.png"
        }

        // Figma "Player - customize": the control outlined, dragged around
        // or out of the player; the items dropped before or after it
        readonly property var zoneModel: repeater.model ? (repeater.model.sourceModel ?? repeater.model) : null

        // the row of this control in its zone (not a binding: through the
        // filter, it went stale after the rows before it moved)
        function currentSourceRow() {
            let row = -1
            for (let i = 0; i < repeater.count; ++i) {
                if (repeater.itemAt(i) === loader) {
                    row = i
                    break
                }
            }
            const m = repeater.model
            if (row < 0 || !m)
                return -1
            return m.mapToSource ? m.mapToSource(m.index(row, 0)).row : row
        }

        // only while customizing: every control of every bar has one
        Loader {
            anchors.fill: parent
            z: 100
            active: VLCStyle.customizing

            sourceComponent: Item {
                id: editOverlay
                // not the spacers of no width (the empty center of 2160:6137)
                visible: VLCStyle.customizing && loader.status === Loader.Ready && loader.width > 0

                // the frame of the play component of the design (54 x 34)
                readonly property bool isPlay: loader.controlId === ControlListModel.PLAY_BUTTON
                // the volume's frame of the design: 5 wider on each side, 36 high
                readonly property bool isVolume: loader.controlId === ControlListModel.VOLUME
                readonly property real volumeInflate: VLCStyle.dp(5, VLCStyle.scale)

                Widgets.DashedRect {
                    x: editOverlay.isPlay ? Math.round((parent.width - width) / 2)
                       : editOverlay.isVolume ? -editOverlay.volumeInflate : 0
                    y: (editOverlay.isPlay || editOverlay.isVolume) ? Math.round((parent.height - height) / 2) : 0
                    width: editOverlay.isPlay ? VLCStyle.dp(54, VLCStyle.scale)
                           : editOverlay.isVolume ? parent.width + 2 * editOverlay.volumeInflate : parent.width
                    height: editOverlay.isPlay ? VLCStyle.dp(34, VLCStyle.scale)
                            : editOverlay.isVolume ? VLCStyle.dp(36, VLCStyle.scale) : parent.height
                    color: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
                    opacity: dragArea.drag.active ? 0.4 : 1
                }

                // where a dropped item goes
                Rectangle {
                    visible: dropArea.containsDrag
                    x: dropArea.after ? parent.width - width / 2 : -width / 2
                    width: VLCStyle.dp(2, VLCStyle.scale)
                    height: parent.height
                    color: VLCStyle.redesign.accent
                }

                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    cursorShape: drag.active ? Qt.DragMoveCursor : Qt.OpenHandCursor
                    drag.target: dragProxy
                    drag.smoothed: false
                    onPressed: (mouse) => {
                        dragProxy.x = mouse.x - dragProxy.width / 2
                        dragProxy.y = mouse.y - dragProxy.height / 2
                    }
                    drag.onActiveChanged: {
                        if (drag.active) {
                            dragProxy.sourceRow = loader.currentSourceRow()
                            dragProxy.Drag.start()
                        } else {
                            dragProxy.Drag.drop()
                            dragProxy.x = 0
                            dragProxy.y = 0
                        }
                    }
                }

                // what is dragged: the control's icon in a dashed box
                Item {
                    id: dragProxy
                    objectName: "playerControl"
                    // above the bar and the dialog while dragged
                    parent: Drag.active ? T.Overlay.overlay : editOverlay
                    readonly property int controlId: loader.controlId
                    property int sourceRow: -1
                    readonly property var zoneModel: loader.zoneModel
                    visible: Drag.active
                    z: 1
                    width: VLCStyle.dp(30, VLCStyle.scale)
                    height: VLCStyle.dp(32, VLCStyle.scale)
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2
                    Drag.source: dragProxy

                    Rectangle {
                        anchors.fill: parent
                        radius: VLCStyle.dp(4, VLCStyle.scale)
                        color: VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                    }
                    Widgets.DashedRect {
                        anchors.fill: parent
                        color: VLCStyle.redesign.accent
                    }
                    Image {
                        anchors.centerIn: parent
                        width: VLCStyle.redesign.iconSize
                        height: width
                        source: PlayerControlbarControls.control(loader.controlId).icon ?? ""
                        sourceSize: Qt.size(width, height)
                    }
                }

                DropArea {
                    id: dropArea
                    // over the gaps beside the control too: an item let go
                    // between two controls goes between them
                    readonly property real reach: VLCStyle.dp(10, VLCStyle.scale)
                    anchors.fill: parent
                    anchors.leftMargin: -reach
                    anchors.rightMargin: -reach
                    property bool after: false
                    onPositionChanged: (drag) => { after = drag.x > width / 2 }
                    onEntered: (drag) => {
                        after = drag.x > width / 2
                        if (drag.source === dragProxy)
                            drag.accepted = false
                    }
                    onDropped: (drop) => {
                        const zone = loader.zoneModel
                        if (!zone)
                            return
                        const source = drop.source
                        const row = loader.currentSourceRow()
                        if (row < 0)
                            return
                        let dest = row + (after ? 1 : 0)
                        // the models change after the event: a zone left empty
                        // deletes its controls, the dragged one among them
                        if (source.objectName === "buttonsList") {
                            const id = source.mIndex
                            Qt.callLater(() => zone.insert(Math.min(dest, zone.count), { "id": id }))
                            drop.accept(Qt.CopyAction)
                        } else if (source.objectName === "playerControl" && source.zoneModel === zone) {
                            const from = source.sourceRow
                            if (from < dest)
                                --dest
                            if (from >= 0 && from !== dest)
                                Qt.callLater(() => {
                                    if (from < zone.count && dest < zone.count)
                                        zone.move(from, dest)
                                })
                            drop.accept(Qt.MoveAction)
                        } else if (source.objectName === "playerControl" && source.zoneModel) {
                            const id = source.controlId
                            const fromZone = source.zoneModel
                            const from = source.sourceRow
                            Qt.callLater(() => {
                                if (from < 0 || from >= fromZone.count)
                                    return
                                fromZone.remove(from)
                                zone.insert(Math.min(dest, zone.count), { "id": id })
                            })
                            drop.accept(Qt.MoveAction)
                        }
                    }
                }
            }
        }

        focus: (index === 0)

        // the gaps of the design, the layout already spaces the controls
        Layout.leftMargin: {
            const previous = (index > 0) ? repeater.itemAt(index - 1) : null
            if (!previous || !parent || parent.spacing === undefined)
                return 0
            return Math.max(0, repeater.designGap(previous.controlId, controlId) - parent.spacing)
        }

        Layout.fillWidth: (item && item.Layout.minimumWidth > 0)
        Layout.minimumWidth: Layout.fillWidth ? item.Layout.minimumWidth : (item ? item.implicitWidth : -1)
        Layout.maximumWidth: Layout.fillWidth ? (item ? item.implicitWidth : -1) : -1

        Layout.minimumHeight: 0
        Layout.maximumHeight: Math.min((item && item.implicitHeight <= 0) ? Number.MAX_VALUE : item.implicitHeight, repeater.availableHeight)
        Layout.fillHeight: true

        Layout.alignment: repeater.alignment

        Binding on visible {
            delayed: true // this is important
            when: condition
            value: {
                if (condition)
                    return (loader.x + loader.Layout.minimumWidth <= repeater.availableWidth)
                else
                    return true;
            }

            property bool condition: false

            Binding on condition {
                delayed: true
                value: (loader.status === Loader.Ready) &&
                       (loader.Layout.minimumWidth > 0) &&
                       (repeater.availableWidth < Number.MAX_VALUE)
            }
        }

        // Events

        onActiveFocusChanged: {
            if (activeFocus && !item?.focus) {
                recoverFocus()
            }
        }

        onLoaded: {
            // labels and dividers only show something: no focus, no Tab stop
            const indicator = (item.Accessible.role === Accessible.Indicator)

            // control should not request focus if they are not enabled:
            item.focus = Qt.binding(function() { return !indicator && item.enabled && item.visible })

            // navigation parent of control is always controlLayout
            // so it can be set here unlike leftItem and rightItem:
            item.Navigation.parentItem = repeater
            // the arrow keys go past them
            if (indicator)
                item.Navigation.navigable = false

            if (typeof item.activeFocusOnTab === "boolean")
                item.activeFocusOnTab = !indicator

            item.visible = Qt.binding(function() { return loader.visible })

            if (item.requestLockUnlockAutoHide)
                item.requestLockUnlockAutoHide.connect(repeater.requestLockUnlockAutoHide)

            if (item.menuOpened)
                item.menuOpened.connect(repeater.menuOpened)

            if (item.forceUnlock)
                repeater.forceUnlock.connect(item.forceUnlock)

            //can't connect to enabledChanged in a Connections
            item.onEnabledChanged.connect(() => {
                if (loader.activeFocus && !item.enabled) // Loader has focus but item is not enabled
                    recoverFocus()
            })
        }

        // Connections

        Connections {
            target: item

            enabled: loader.status === Loader.Ready

            function onVisibleChanged() {
                if (loader.activeFocus && !item.visible)
                    recoverFocus()
            }
        }

        // Functions

        function applyNavigation() {
            if (item === null) return

            const itemLeft  = repeater.itemAt(index - 1)
            const itemRight = repeater.itemAt(index + 1)

            if (itemLeft) {
                const componentLeft = itemLeft.item

                if (componentLeft)
                {
                    item.Navigation.leftItem = componentLeft

                    componentLeft.Navigation.rightItem = item
                }
            }

            if (itemRight) {
                const componentRight = itemRight.item

                if (componentRight)
                {
                    item.Navigation.rightItem = componentRight

                    componentRight.Navigation.leftItem = item
                }
            }
        }

        function removeNavigation() {
            if (item === null) return

            const itemLeft = repeater.itemAt(index - 1)

            // NOTE: The current item was removed from the repeater so we test against the
            //       same index.
            const itemRight = repeater.itemAt(index)

            if (itemLeft) {
                if (itemRight) {
                    itemLeft.item.Navigation.rightItem = itemRight.item
                    itemRight.item.Navigation.leftItem = itemLeft.item
                }
                else
                    itemLeft.item.Navigation.rightItem = null
            }
            else if (itemRight) {
                itemRight.item.Navigation.leftItem = null
            }
        }

        function recoverFocus(_index) {
            if (item === null) return

            // the bar edited in the customize dialog keeps no focus frame
            if (VLCStyle.customizing) return

            const controlLayout = repeater.Navigation.parentItem

            if (!controlLayout || !controlLayout.visible)
                return

            if (_index === undefined)
                _index = index

            for (let i = 1; i <= Math.max(_index, repeater.count - (_index + 1)); ++i) {
                 if (i <= _index) {
                     const leftItem = repeater.itemAt(_index - i)

                     if (_focusIfFocusable(leftItem))
                         return
                 }

                 if (_index + i <= repeater.count - 1) {
                     const rightItem = repeater.itemAt(_index + i)

                     if (_focusIfFocusable(rightItem))
                         return
                 }
            }

            // focus to other alignment if focusable control
            // in the same alignment is not found:
            if (!!controlLayout.Navigation.rightItem) {
                controlLayout.Navigation.defaultNavigationRight()
            } else if (!!controlLayout.Navigation.leftItem) {
                controlLayout.Navigation.defaultNavigationLeft()
            } else if (controlLayout.altFocusAction) {
                controlLayout.altFocusAction()
            }
        }

        // Private

        function _focusIfFocusable(_loader) {
            if (_loader?.item?.focus) {
                if (item.focusReason !== undefined)
                    _loader.item.forceActiveFocus(item.focusReason)
                else {
                    console.warn("focusReason is not available in %1!".arg(item))
                    _loader.item.forceActiveFocus()
                }
                return true
            } else {
                return false
            }
        }
    }
}
