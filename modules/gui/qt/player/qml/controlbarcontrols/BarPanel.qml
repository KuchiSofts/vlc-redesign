/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
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

import VLC.Style

// A panel (a popup) that a button of the control bar shows, as the languages
// and tracks menu: above the bar, its right edge at the button's, inside the
// player; a press outside closes it, and so does the button. Built the first
// time it opens (every bar has its buttons, and so have the previews of the
// toolbar editor).
//   BarPanel { id: panel; button: root; sourceComponent: SomePopup { } }
//   onClicked: panel.toggle()
Loader {
    id: loader

    // the button that shows it (an IconToolButton: pressedAt)
    required property Item button

    // where it opens: the player (its seek bar), else the main display
    readonly property Item container: {
        if ((typeof rootPlayer !== 'undefined') && (rootPlayer !== null))
            return rootPlayer
        return g_mainDisplay
    }

    // its height: this one at most, less when the room above the bar is
    // smaller (a tall panel, such as the effects, in a small window); 0: the
    // panel sizes itself
    property real maximumHeight: 0

    readonly property bool opened: item ? item.visible : false

    signal panelOpened(var panel)
    signal panelClosed()

    // a press on the button closes the panel (a press outside it) before
    // the button gets the press, and the click that follows must not open
    // it again
    property double _closedAt: 0

    // the bottom of the room: the seek bar of the player, else the button
    readonly property real _bottom: {
        if (!container)
            return 0
        if (container.positionSliderY !== undefined)
            return container.positionSliderY
        return button.mapToItem(container, 0, 0).y
    }

    active: false

    function toggle() {
        active = true
        const panel = item
        if (!panel)
            return
        if (panel.visible) {
            panel.close()
            return
        }
        if (button.pressedAt - _closedAt < 300)
            return
        place()
        panel.open()
    }

    function close() {
        if (item)
            item.close()
    }

    function place() {
        const panel = item
        const margin = VLCStyle.dp(8, VLCStyle.scale)
        // clear of the window's top bar, as the languages and tracks
        const top = VLCStyle.dp(48, VLCStyle.scale)
        if (maximumHeight > 0)
            panel.height = Math.min(maximumHeight, Math.max(0, _bottom - margin - top))
        const position = button.mapToItem(container, 0, 0)
        panel.x = Math.max(margin, Math.min(container.width - panel.width - margin,
                                            position.x + button.width - panel.width))
        panel.y = Math.max(margin, _bottom - panel.height - margin)
    }

    onLoaded: {
        item.parent = container
        item.z = 1
        // placed above its button, not centred
        if (item.centered !== undefined)
            item.centered = false
    }

    // (not item.opened.connect(): "opened" of a popup is also a property)
    Connections {
        target: loader.item
        function onOpened() { loader.panelOpened(loader.item) }
        function onClosed() {
            loader._closedAt = Date.now()
            loader.panelClosed()
        }
    }

    // a window resized while it is open, or a panel measured once shown
    // (its content's height): it stays above the bar
    Connections {
        target: loader.item ? loader.container : null
        enabled: loader.opened
        function onWidthChanged() { loader.place() }
        function onHeightChanged() { loader.place() }
    }
    Connections {
        target: loader.item
        enabled: loader.opened
        function onWidthChanged() { loader.place() }
        function onHeightChanged() { loader.place() }
    }
}
