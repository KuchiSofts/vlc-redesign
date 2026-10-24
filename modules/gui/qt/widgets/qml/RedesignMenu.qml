/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import QtQuick.Window

import VLC.Style

// Menu of the redesign (Figma "menu", "right-click menu" and the context
// menus): a dark rounded panel of 48px rows, see RedesignMenuItem.
T.Menu {
    id: root

    // Icon of the item opening this menu as a submenu, see RedesignMenuItem
    property string iconName
    // Figma's menus are 265 wide (context menus, options, the profile
    // submenu); the main menu 375, the right-click menu 292
    property real menuWidth: VLCStyle.dp(265, VLCStyle.scale)

    // A press outside closes it before the button that opens it gets that
    // press: the click that follows closes it, not opens it again
    property double _closedAt: 0

    Component.onCompleted: root.closed.connect(() => { root._closedAt = Date.now() })

    // From the click of the button that opens it: opens it (at x, y of
    // the button, else where the mouse is), or closes it
    function toggleFrom(button, x, y) {
        if (root.opened) {
            root.close()
            return
        }
        if (button.pressedAt - root._closedAt < 300)
            return
        if (x === undefined)
            root.popup()
        else
            root.popup(button, x, y)
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: VLCStyle.dp(4, VLCStyle.scale)
    margins: 0
    // Submenus open 11 px clear of their menu (Figma "menu" 6062:13354 and
    // "Profiles" 6648:20666), their rows in line with the one that opens them
    overlap: -VLCStyle.dp(11, VLCStyle.scale)
    cascade: true

    delegate: RedesignMenuItem { }

    contentItem: ListView {
        implicitHeight: contentHeight
        model: root.contentModel
        interactive: Window.window
                     ? contentHeight + root.topPadding + root.bottomPadding > Window.window.height
                     : false
        clip: true
        currentIndex: root.currentIndex
        boundsBehavior: Flickable.StopAtBounds

        // Taller than the window (the main menu in a window under about
        // 620 px), it scrolls, its last rows cut: show it, as Qt's menus do
        T.ScrollIndicator.vertical: T.ScrollIndicator {
            id: scrollIndicator
            // (a template has no size of its own)
            implicitWidth: implicitContentWidth + leftPadding + rightPadding
            implicitHeight: implicitContentHeight + topPadding + bottomPadding
            padding: VLCStyle.dp(2, VLCStyle.scale)
            visible: scrollIndicator.size < 1.0
            contentItem: Rectangle {
                implicitWidth: VLCStyle.dp(3, VLCStyle.scale)
                implicitHeight: VLCStyle.dp(3, VLCStyle.scale)
                radius: width / 2
                color: VLCStyle.redesign.color("#6B6B6B", "#6B6B6B")
            }
        }
    }

    background: Rectangle {
        implicitWidth: root.menuWidth
        radius: VLCStyle.dp(10, VLCStyle.scale)
        // "Dark/BG/A3 Transparent": #191919 at 95%
        color: VLCStyle.palette.isDark ? Qt.rgba(0x19 / 255, 0x19 / 255, 0x19 / 255, 0.95)
                                       : Qt.rgba(0xab / 255, 0xab / 255, 0xaa / 255, 0.95)

        // Figma: drop shadow 0 8 blur 15 #000000 46 % (as RedesignDialog)
        RoundedRectangleShadow {
            yOffset: VLCStyle.dp(8, VLCStyle.scale)
            blurRadius: 7.5 * VLCStyle.scale // (dp() rounds to whole pixels)
            color: Qt.rgba(0, 0, 0, 0.46)
        }
    }
}
