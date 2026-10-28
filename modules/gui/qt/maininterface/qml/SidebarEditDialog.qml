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
import QtQuick.Controls.impl as Impl

import VLC.MainInterface
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets
import VLC.Dialogs

// Edit Sidebar (the dialogs of the design, as New Playlist): the rows of the
// library's sidebar in their order, dragged by their handle as the controls
// of Customize Player, or moved up and down (the buttons, Ctrl+Up and
// Ctrl+Down), each shown or hidden; at least one stays. Reset to Default
// puts the design's order back. Kept with Save.
Widgets.RedesignDialog {
    id: root

    objectName: "sidebarEditDialog"

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }

    title: qsTr("Edit Sidebar")
    preferredWidth: VLCStyle.dp(560, VLCStyle.scale)
    extraText: qsTr("Reset to Default")
    onExtraClicked: _fill(true)
    onAccepted: _save()
    onAboutToShow: _fill(false)

    SidebarItems { id: items }

    // the rows of the list (the available ones: TV while its add-on is on)
    ListModel { id: rows }
    readonly property int _shownCount: {
        void rows.count
        void root._changes
        let n = 0
        for (let i = 0; i < rows.count; ++i)
            if (rows.get(i).shown)
                ++n
        return n
    }
    property int _changes: 0

    function _fill(defaults) {
        rows.clear()
        const order = defaults ? items.defaults.map(d => d.key) : items.order
        for (const key of order) {
            if (!items.available(key))
                continue
            const e = items.entry(key)
            rows.append({ key: key, title: e.title, asset: e.asset,
                          shown: defaults || items.hidden.indexOf(key) < 0 })
        }
        ++_changes
    }

    function _save() {
        const visible = []
        const hidden = []
        for (let i = 0; i < rows.count; ++i) {
            const row = rows.get(i)
            visible.push(row.key)
            if (!row.shown)
                hidden.push(row.key)
        }
        // the rows not listed (TV, its add-on off) keep their place: after
        // the row they followed
        const order = visible.slice()
        const previous = items.order
        for (let i = 0; i < previous.length; ++i) {
            const key = previous[i]
            if (order.indexOf(key) >= 0)
                continue
            let at = 0
            for (let j = i - 1; j >= 0; --j) {
                const before = order.indexOf(previous[j])
                if (before >= 0) {
                    at = before + 1
                    break
                }
            }
            order.splice(at, 0, key)
            if (items.hidden.indexOf(key) >= 0)
                hidden.push(key)
        }
        items.save(order, hidden)
    }

    function _move(from, to) {
        if (to < 0 || to >= rows.count || from === to)
            return
        rows.move(from, to, 1)
        list.currentIndex = to
    }

    Widgets.RedesignDialogHeading {
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        headingWraps: true
        text: qsTr("The pages of the sidebar")
        description: qsTr("Drag a page by its handle, or move it with the arrows. Hidden pages stay one click away here.")
    }

    ListView {
        id: list

        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        Layout.fillWidth: true
        // every row in sight: no scrolling, no jump when one moves
        Layout.preferredHeight: rows.count * root.px(48) + Math.max(0, rows.count - 1) * spacing
        interactive: false
        spacing: root.px(4)
        model: rows
        focus: true
        keyNavigationEnabled: true
        highlightMoveDuration: 0

        // Ctrl+Up and Ctrl+Down move the row with the keyboard focus
        Keys.onPressed: (event) => {
            if (!(event.modifiers & Qt.ControlModifier))
                return
            if (event.key === Qt.Key_Up) {
                root._move(currentIndex, currentIndex - 1)
                event.accepted = true
            } else if (event.key === Qt.Key_Down) {
                root._move(currentIndex, currentIndex + 1)
                event.accepted = true
            }
        }

        moveDisplaced: Transition {
            NumberAnimation { property: "y"; duration: VLCStyle.duration_veryShort; easing.type: Easing.OutSine }
        }

        delegate: Rectangle {
            id: row

            required property int index
            required property string key
            required property string title
            required property string asset
            required property bool shown

            readonly property bool current: ListView.isCurrentItem && list.activeFocus

            width: ListView.view.width
            height: root.px(48)
            radius: root.px(6)
            color: dragArea.drag.active || dragArea.containsMouse || current
                   ? VLCStyle.redesign.color("#222222") : VLCStyle.redesign.color("#151515")
            border.width: current ? 1 : 0
            border.color: VLCStyle.redesign.focusColor

            Accessible.role: Accessible.ListItem
            Accessible.name: title + (shown ? "" : ", " + qsTr("hidden"))

            // the handle: dragged up or down, the row takes the place it
            // is over
            Item {
                id: handle
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: root.px(40)

                Impl.IconImage {
                    anchors.centerIn: parent
                    width: VLCStyle.redesign.iconSize
                    height: width
                    source: "qrc:///redesign/icons/move/default.png"
                    color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted)
                }

                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    preventStealing: true
                    onPositionChanged: (mouse) => {
                        if (!pressed)
                            return
                        const y = mapToItem(list.contentItem, mouse.x, mouse.y).y
                        const target = Math.max(0, Math.min(rows.count - 1,
                                                            Math.floor(y / (root.px(48) + list.spacing))))
                        if (target !== row.index)
                            root._move(row.index, target)
                    }
                }
            }

            Impl.IconImage {
                id: pageIcon
                anchors.left: handle.right
                anchors.leftMargin: root.px(4)
                anchors.verticalCenter: parent.verticalCenter
                width: VLCStyle.redesign.iconSize
                height: width
                // the sidebar's own icon (drawn in #8F8F8F), in the muted colour
                source: SVGColorImage.colorize("qrc:///redesign/" + row.asset + ".svg")
                                     .any({ "#8F8F8F": VLCStyle.redesign.color(VLCStyle.redesign.textMuted) }).uri()
                opacity: row.shown ? 1 : 0.4
            }

            Text {
                anchors.left: pageIcon.right
                anchors.leftMargin: root.px(12)
                anchors.right: up.left
                anchors.rightMargin: root.px(12)
                anchors.verticalCenter: parent.verticalCenter
                text: row.title
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                color: row.shown ? VLCStyle.redesign.color("#C6C6C6") : VLCStyle.redesign.color("#6B6B6B")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
            }

            Widgets.RedesignButton {
                id: up
                anchors.right: down.left
                anchors.rightMargin: root.px(4)
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: root.px(36)
                text: "▲"
                enabled: row.index > 0
                focusPolicy: Qt.NoFocus
                Accessible.name: qsTr("Move up")
                onClicked: root._move(row.index, row.index - 1)
            }
            Widgets.RedesignButton {
                id: down
                anchors.right: toggle.left
                anchors.rightMargin: root.px(16)
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: root.px(36)
                text: "▼"
                enabled: row.index < rows.count - 1
                focusPolicy: Qt.NoFocus
                Accessible.name: qsTr("Move down")
                onClicked: root._move(row.index, row.index + 1)
            }
            // shown or hidden; the last one shown stays
            SettingsToggle {
                id: toggle
                anchors.right: parent.right
                anchors.rightMargin: root.px(12)
                anchors.verticalCenter: parent.verticalCenter
                checked: row.shown
                enabled: !row.shown || root._shownCount > 1
                Accessible.name: qsTr("Show %1").arg(row.title)
                onSwitched: (on) => {
                    rows.setProperty(row.index, "shown", on)
                    ++root._changes
                }
            }
        }
    }
}
