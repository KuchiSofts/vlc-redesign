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

// The mouse wheel over a list of rows (the open list of a dropdown): a notch
// moves it by one row, and it always stops on a row, never half of one. The
// notches of a fast turn add up (a turn of three moves three rows in one
// glide), the small steps of a high-resolution wheel or a touchpad move a row
// once they make a notch. Put it over the list (anchors.fill: view), not in it
// (a ListView moves its children with its content).
MouseArea {
    id: root

    required property ListView view
    property int rowsPerNotch: 1

    // the rows under it keep their hover and clicks
    acceptedButtons: Qt.NoButton
    hoverEnabled: false

    // what a high-resolution wheel sent short of a notch (120)
    property int _angle: 0
    // the row the list is gliding to the top of, -1 when it stands
    property int _target: -1

    // the y of the list with this row at its top, as the list puts it there
    // (at its end it shows its last rows: the row cannot be at the top)
    function _yOf(index) {
        const from = view.contentY
        view.positionViewAtIndex(index, ListView.Beginning)
        const y = view.contentY
        view.contentY = from
        return y
    }

    // the row at the top of the list: the next one when it is more than half
    // out of sight
    function _rowAt(y) {
        const x = view.width / 2
        let index = view.indexAt(x, y + 1)
        if (index < 0)
            index = view.indexAt(x, y + view.spacing + 1)
        if (index < 0)
            return 0
        const item = view.itemAtIndex(index)
        if (item && item.y + item.height / 2 < y && index < view.count - 1)
            index += 1
        return index
    }

    function _go(index) {
        glide.stop()
        // the last row that can be at the top: after it the list is at its end
        const lastTop = _rowAt(_yOf(view.count - 1))
        index = Math.max(0, Math.min(index, lastTop))
        _target = index
        const y = _yOf(index)
        if (Math.abs(y - view.contentY) < 0.5) {
            _target = -1
            return
        }
        glide.from = view.contentY
        glide.to = y
        glide.start()
    }

    onWheel: (wheel) => {
        // nothing to scroll: the wheel goes on to what is under the list
        if (view.count === 0 || view.contentHeight <= view.height + 0.5) {
            wheel.accepted = false
            return
        }
        wheel.accepted = true
        _angle += wheel.angleDelta.y
        const notches = (_angle > 0) ? Math.floor(_angle / 120) : Math.ceil(_angle / 120)
        if (notches === 0)
            return
        _angle -= notches * 120
        const from = (_target >= 0) ? _target : _rowAt(view.contentY)
        _go(from - notches * rowsPerNotch)
    }

    NumberAnimation {
        id: glide

        target: root.view
        property: "contentY"
        duration: 120
        easing.type: Easing.OutCubic

        onStopped: {
            if (root._target >= 0 && !running)
                root._target = -1
        }
    }

    // a list opened again starts from its own position
    Connections {
        target: root.view
        function onCountChanged() { root._angle = 0; root._target = -1 }
    }
}
