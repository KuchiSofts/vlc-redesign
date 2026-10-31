/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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
import QtQuick.Templates as T
import QtQuick.Layouts
import QtQml.Models
import QtQuick.Window


import VLC.MainInterface
import VLC.Style
import VLC.Playlist
import VLC.Player
import VLC.Widgets as Widgets
import VLC.Menus as Menus
import VLC.Util

T.ToolBar {
    id: root

    Accessible.name: qsTr("Local top bar")
    AccessibleCompat.id: "localTopbar"

    property bool useAcrylic: true

    // For now, used for d&d functionality
    // Not strictly necessary to set
    property Item plListView

    property bool _showCSD: MainCtx.clientSideDecoration
        && !(MainCtx.intfMainWindow.visibility === Window.FullScreen)
        && !MainCtx.hasToolbarMenu

    // macOS: the room of the traffic lights in the bar (the unified title
    // bar), at the start and at the end of the layout (physical sides in
    // MainCtx: the bar may be mirrored, the system's buttons are not)
    readonly property int _insetStart: !_showCSD ? 0
        : (LayoutMirroring.enabled ? MainCtx.windowButtonsInsetRight : MainCtx.windowButtonsInsetLeft)
    readonly property int _insetEnd: !_showCSD ? 0
        : (LayoutMirroring.enabled ? MainCtx.windowButtonsInsetLeft : MainCtx.windowButtonsInsetRight)

    //whether the navigation pannel is actually visible or not
    property alias navigationVisible: navigationVisibilty.checked
    property bool playqueueVisible: false

    property alias navigationButton: navigationVisibilty

    signal toggleNavigationVisibility()
    signal togglePlayqueueVisibility()

    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                             implicitContentWidth + leftPadding + rightPadding + (_showCSD ? csdButtons.implicitWidth : 0)
                             + _insetStart + _insetEnd)

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.Window
    }

    // Figma "topbar": #1B1B1B (the same bar as the player)
    background: Rectangle {
        color: VLCStyle.redesign.color(VLCStyle.redesign.playerTopbarBackground, "#ababaa")
    }

    contentItem: Item {

        implicitHeight: VLCStyle.redesign.topbarHeight
        implicitWidth: navigationButtons.implicitWidth
                     + VLCStyle.margin_xsmall * 2

        Item {
            id: navigationButtons

            implicitWidth: navigationButtonsRow.implicitWidth

            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
                // the 6 px padding of the topbar of the design (after the
                // traffic lights on macOS)
                leftMargin: VLCStyle.dp(6, VLCStyle.scale) + root._insetStart
            }

            Navigation.parentItem: root

            Row {
                id: navigationButtonsRow

                anchors.verticalCenter: parent.verticalCenter
                spacing: VLCStyle.dp(7, VLCStyle.scale)

                Navigation.parentItem: root

                // Figma "Frame 8": the cone, an icon in a 38 x 28 frame
                Item {
                    anchors.verticalCenter: parent.verticalCenter
                    width: VLCStyle.dp(38, VLCStyle.scale)
                    height: VLCStyle.dp(28, VLCStyle.scale)

                    Widgets.BannerCone {
                        id: logo
                        source: "qrc:///redesign/logo.svg"

                        x: VLCStyle.dp(7, VLCStyle.scale)
                        anchors.verticalCenter: parent.verticalCenter
                        width: VLCStyle.redesign.iconSize
                        height: VLCStyle.redesign.iconSize
                        sourceSize: Qt.size(width, height)
                        smooth: !VLCStyle.redesign.svgExact(width, height, Screen.devicePixelRatio)
                        color: VLCStyle.redesign.logoColor
                    }
                }

                Widgets.IconToolButton {
                    id: menuButton
                    contentItem: Image {
                        source: "qrc:///redesign/icons/more/default.svg"
                        sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                        fillMode: Image.Pad
                    }
                    backgroundColor: VLCStyle.redesign.color(hovered || down || mainMenu.opened
                                                             ? VLCStyle.redesign.controlHoverBackground : "transparent")
                    radius: VLCStyle.redesign.controlRadius

                    focus: true

                    text: description
                    description: qsTr("Menu")

                    AccessibleCompat.id: "topBarMenuButton"

                    // an icon of 24 with 2 px around
                    anchors.verticalCenter: parent.verticalCenter
                    height: VLCStyle.dp(28, VLCStyle.scale)
                    width: VLCStyle.dp(28, VLCStyle.scale)

                    // under the button, from its left edge (its right edge in a
                    // right to left layout)
                    onClicked: mainMenu.popup(menuButton, LayoutMirroring.enabled ? menuButton.width - mainMenu.width : 0,
                                              menuButton.height)

                    Menus.MainMenu {
                        id: mainMenu
                    }

                    Navigation.parentItem: navigationButtons
                    // (the row is mirrored in a right to left layout)
                    Navigation.leftItem: LayoutMirroring.enabled ? navigationVisibilty : null
                    Navigation.rightItem: LayoutMirroring.enabled ? null : navigationVisibilty
                }

                // Not in the design: the navigation stays shown, the
                // toggle remains for small windows (the navigation hides)
                Widgets.IconToolButton {
                    id: navigationVisibilty
                    visible: VLCStyle.isScreenSmall
                    contentItem: Image {
                        source: "qrc:///redesign/icons/sidebar/active.svg"
                        sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                        fillMode: Image.Pad
                        opacity: navigationVisibilty.checked ? 1.0 : 0.6
                    }
                    backgroundColor: VLCStyle.redesign.color(hovered || down || checked
                                                             ? VLCStyle.redesign.controlHoverBackground : "transparent")
                    radius: VLCStyle.redesign.controlRadius

                    text: description
                    description: qsTr("Show navigation menu")

                    AccessibleCompat.id: "sidebarToggleButton"

                    height: VLCStyle.bannerButton_height
                    width: VLCStyle.bannerButton_width

                    onClicked: root.toggleNavigationVisibility()

                    Navigation.parentItem: navigationButtons
                    Navigation.leftItem: LayoutMirroring.enabled ? historyBack : menuButton
                    Navigation.rightItem: LayoutMirroring.enabled ? menuButton : historyBack
                }

                Widgets.IconToolButton {
                    id: historyBack
                    visible: false
                    color: VLCStyle.redesign.color(enabled ? VLCStyle.redesign.textMuted : VLCStyle.redesign.textDisabled)
                    backgroundColor: VLCStyle.redesign.color(hovered || down ? VLCStyle.redesign.controlHoverBackground : "transparent")
                    radius: VLCStyle.redesign.controlRadius

                    font.pixelSize: VLCStyle.redesign.iconSize
                    text: VLCIcons.back
                    description: qsTr("Previous")

                    // the arrow points to the right in a right to left layout
                    // (the background is symmetric: only the glyph changes)
                    transform: Scale {
                        origin.x: historyBack.width / 2
                        xScale: historyBack.LayoutMirroring.enabled ? -1 : 1
                    }

                    AccessibleCompat.id: "historyBackButton"

                    height: VLCStyle.bannerButton_height
                    width: VLCStyle.bannerButton_width

                    onClicked: History.previous()
                    enabled: !History.previousEmpty

                    onEnabledChanged: {
                        if (historyBack.activeFocus)
                            navigationVisibilty.focus = true
                    }

                    Navigation.parentItem: navigationButtons
                    Navigation.leftItem: LayoutMirroring.enabled ? null : navigationVisibilty
                    Navigation.rightItem: LayoutMirroring.enabled ? navigationVisibilty : null
                }


            }
        }

        Text {
            id: titleText
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.max(0, parent.width - 2 * Math.max(navigationButtons.width + root._insetStart,
                                                           (root._showCSD ? csdButtons.width : 0) + root._insetEnd)
                            - 2 * VLCStyle.margin_normal)
            height: parent.height
            // what plays (or is paused); stopped, the app's name again (the
            // player keeps the last media's title after a stop)
            text: (Player.isStarted && Player.title) || "VLC Redesign" // (a name: not translated)
            color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.redesign.fontNormal
            horizontalAlignment: Text.AlignHCenter
            // the text box of the design (17 px) centred on a whole pixel
            verticalAlignment: Text.AlignTop
            topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                       font.pixelSize) - titleMetrics.ascent
            elide: Text.ElideRight
            Accessible.ignored: true

            // the whole title when it is cut
            HoverHandler { id: titleHover }
            T.ToolTip.visible: titleHover.hovered && titleText.truncated
            T.ToolTip.text: titleText.text
            T.ToolTip.delay: VLCStyle.delayToolTipAppear

            FontMetrics {
                id: titleMetrics
                font: titleText.font
            }
        }

        Loader {
            z: -1

            anchors.fill: parent
            active: root._showCSD
            sourceComponent: Widgets.CSDTitlebarTapNDrapHandler {
            }
        }
    }

    Loader {
        id: csdButtons

        //csd button are
        parent:  root
        anchors {
            right: parent.right
            top: parent.top
            // the 6 px padding of the topbar of the design
            rightMargin: VLCStyle.dp(6, VLCStyle.scale)
        }

        height: VLCStyle.redesign.topbarHeight
        active: root._showCSD
        source: "qrc:///qt/qml/VLC/Widgets/CSDWindowButtonSet.qml"
        // no fullscreen button next to Close: it is in the bottom bar of the
        // player (F, F11 or a double-click elsewhere)
        onLoaded: item.fullscreenHidden = true
    }
}
