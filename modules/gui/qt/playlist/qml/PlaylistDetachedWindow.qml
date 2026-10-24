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
import QtQuick.Window


import VLC.MainInterface
import VLC.Style


Window {
    visible: _componentCompleted && MainCtx.playqueuePanel.visible

    transientParent: MainCtx.intfMainWindow

    property alias playlistView: playlistView

    width: 350
    minimumWidth: playlistView.minimumWidth

    title: qsTr("Play Queue")
    color: theme.bg.primary

    // Right to left (Hebrew, Arabic...): a window of its own is not under
    // the mirrored interface, it mirrors itself
    readonly property bool _rtl: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.enabled: _rtl
    LayoutMirroring.childrenInherit: true

    property bool _componentCompleted: false

    onVisibleChanged: {
        if (visible) {
            console.assert(transientParent)
            height = transientParent.height
            minimumHeight = transientParent.minimumHeight
            // beside the main window: on its left in a right to left layout
            x = _rtl ? transientParent.x - width - 10
                     : transientParent.x + transientParent.width + 10
            y = transientParent.y
        }
    }

    onClosing: {
        MainCtx.playqueuePanel.visible = false
    }

    Component.onCompleted: {
        if (MainCtx.createWindowWithoutRedirectionSurface) { // Win32
            const ret = MainCtx.createWindowWithoutRedirectionSurface(this)
            if (!ret)
                console.debug("MainCtx::createWindowWithoutRedirectionSurface(): returned false for window", this)
        }

        _componentCompleted = true
    }

    PlaylistPane {
        id: playlistView

        useAcrylic: false
        focus: true
        anchors.fill: parent

        colorContext.palette: VLCStyle.palette
    }
}
