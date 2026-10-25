/*****************************************************************************
 * Copyright (C) 2021 VLC authors and VideoLAN
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


import VLC.Widgets as Widgets
import VLC.Style
import VLC.Dialogs
import VLC.Util

RedesignIconButton {
    iconSource: "qrc:///redesign/player/equalizer.png"
    id: extdSettingsBtn

    text: VLCIcons.effect_filter

    // (audio and video: the whole effects panel)
    description: qsTr("Effects")

    AccessibleCompat.id: "extendedSettingsButton"

    signal requestLockUnlockAutoHide(bool lock)
    signal menuOpened(var menu)

    // above the button, as the languages and tracks (the menu and the
    // shortcut still open it in the middle of the window)
    onClicked: panel.toggle()

    function forceUnlock() {
        panel.close()
    }

    BarPanel {
        id: panel
        button: extdSettingsBtn
        maximumHeight: VLCStyle.dp(780, VLCStyle.scale)
        sourceComponent: EffectsDialog { }
        onPanelOpened: (menu) => {
            extdSettingsBtn.requestLockUnlockAutoHide(true)
            extdSettingsBtn.menuOpened(menu)
        }
        onPanelClosed: {
            extdSettingsBtn.requestLockUnlockAutoHide(false)
            extdSettingsBtn.forceActiveFocus()
            extdSettingsBtn.menuOpened(null)
        }
    }
}
