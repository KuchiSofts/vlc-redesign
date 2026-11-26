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
import VLC.MainInterface
import VLC.Style
import VLC.Util

// targetWidth: concerned widget's current width
// HorizontalResizeHandle actually doesn't resizes target
// you have to assign target's width manually using requestedWidth property
// e.g target.width: Helpers.clamp(requestedWidth, minimumWidth, maximumWidth)
MouseArea {
    id: root

    required property int currentWidth
    required property string accessibleName
    property int requestedWidth: 0
    property int minimumWidth: 0
    property int maximumWidth: Number.MAX_SAFE_INTEGER

    property bool atRight: true

    property int _previousX
    property bool _syncingValue: false

    // A mouse drag leaves focus with the pane contents.
    readonly property bool visualFocus: activeFocus
    // The artists pane can run out of space with both side panels open.
    activeFocusOnTab: maximumWidth > minimumWidth

    Accessible.role: Accessible.Slider
    Accessible.name: accessibleName
    Accessible.ignored: !root.visible || !root.activeFocusOnTab

    // Qt's accessible slider reads these property names and writes value.
    property int value
    readonly property int minimumValue: minimumWidth
    readonly property int maximumValue: Math.max(minimumWidth, maximumWidth)
    readonly property int stepSize: Math.max(1, VLCStyle.dp(16, VLCStyle.scale))

    readonly property int _displayedWidth: Helpers.clamp(currentWidth, minimumValue, maximumValue)
    on_DisplayedWidthChanged: _syncValue(_displayedWidth)

    function _syncValue(width) {
        _syncingValue = true
        value = width
        _syncingValue = false
    }

    onValueChanged: {
        if (_syncingValue || !activeFocusOnTab)
            return

        const width = Helpers.clamp(value, minimumValue, maximumValue)
        _syncValue(width)
        requestedWidth = width
    }

    cursorShape: Qt.SplitHCursor
    width: VLCStyle.resizeHandleWidth
    // (containsMouse: the pane shows where it can be dragged)
    hoverEnabled: true
    acceptedButtons: Qt.LeftButton
    preventStealing: true

    Keys.onPressed: (event) => {
        let width
        switch (event.key) {
        case Qt.Key_Left:
        case Qt.Key_Right: {
            const step = (event.modifiers & Qt.ShiftModifier) ? 4 * stepSize : stepSize
            const wider = (event.key === Qt.Key_Right) === atRight
            width = currentWidth + (wider ? step : -step)
            break
        }
        case Qt.Key_Home:
            width = minimumValue
            break
        case Qt.Key_End:
            width = maximumValue
            break
        default:
            root.Navigation.defaultKeyAction(event)
            return
        }
        if (activeFocusOnTab)
            requestedWidth = Helpers.clamp(width, minimumValue, maximumValue)
        event.accepted = true
    }

    onPressed: (mouse) => {
        MainCtx.setCursor(cursorShape)
        _previousX = mouseX
    }

    onReleased:(mouse) => {
        MainCtx.restoreCursor()
    }

    onCanceled: {
        MainCtx.restoreCursor()
    }

    onPositionChanged: {
        // with hoverEnabled the pointer passing over moves it too: only a
        // drag (pressed, from the point it was pressed at) resizes
        if (!pressed)
            return
        const f = atRight ? 1 : -1
        const delta = mouseX - _previousX
        root.requestedWidth = Helpers.clamp(
            root.currentWidth + (delta * f),
            minimumWidth,
            maximumWidth)
    }

}
