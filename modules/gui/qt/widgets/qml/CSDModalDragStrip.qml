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
import QtQuick.Window
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Style

// A modal popup blocks the whole window, title bar included. Opened along a
// full window modal dialog, this strip keeps the title bar usable: drag to
// move, double click to maximize, right click for the window menu. The
// window buttons are hit tested natively and keep working anyway.
T.Popup {
    id: root

    required property T.Popup dialog

    parent: T.Overlay.overlay
    // The overlay lets the topmost popup under the pointer handle the press
    z: dialog.z + 1
    x: 0
    y: 0
    width: parent ? parent.width : 0
    height: VLCStyle.redesign.topbarHeight
    padding: 0
    modal: false
    dim: false
    focus: false
    closePolicy: T.Popup.NoAutoClose

    visible: dialog.visible && MainCtx.clientSideDecoration
             && MainCtx.intfMainWindow.visibility !== Window.FullScreen

    background: null
    // (right to left as the interface, though nothing in it has a side)
    contentItem: CSDTitlebarTapNDrapHandler {
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
    }
}
