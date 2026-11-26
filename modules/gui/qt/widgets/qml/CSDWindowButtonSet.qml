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
import VLC.Player
import VLC.Style

Row {
    id: root

    // Figma "Player - customize": the fullscreen button moved into the bar
    property bool fullscreenHidden: false
    // the control of the bar it becomes when dragged there (-1: not)
    property int fullscreenControlId: -1
    spacing: 0
    padding: 0
    width: implicitWidth

    // In fullscreen (issue: no way out at the top, no minimize, no close): the
    // set shows there too, Maximize gives way to Exit Fullscreen, lit
    readonly property bool _fullscreen: MainCtx.intfMainWindow.visibility === Window.FullScreen

    property color color: theme.fg.primary
    property color hoverColor: theme.bg.primary.alpha(0.5)

    readonly property bool buttonHovered: {
        for (let i = 0; i < repeater.count; ++i) {
            const group = repeater.itemAt(i)
            if (group.buttonHovered) return true
        }
        return false
    }

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.Window
    }

    Repeater {
        id: repeater
        model: MainCtx.csdButtonModel.windowCSDButtons

        Row {
            required property var modelData
            height: root.height
            readonly property bool buttonHovered: windowButton.hovered || windowButton.showHovered || fullscreenButton.hovered
            // moved by the buttons before it (the fullscreen one shown or not)
            onXChanged: Qt.callLater(windowButton.updateRect)

            CSDWindowButton {
                id: windowButton
                csdButton: modelData
                height: root.height
                visible: !(root._fullscreen && modelData.type === CSDButton.MaximizeRestore)
                showHovered: modelData.showHovered
                externalPressed: modelData.externalPressed
                color: root.color
                hoverColor: modelData.type === CSDButton.Close ? "#c42b1c" : root.hoverColor
                isThemeDark: theme.palette.isDark
                readonly property bool restore: modelData.type === CSDButton.MaximizeRestore
                                                && MainCtx.intfMainWindow.visibility === Window.Maximized
                iconSource: {
                    switch (modelData.type) {
                    case CSDButton.Minimize: return "qrc:///redesign/player/minimize.svg"
                    case CSDButton.MaximizeRestore: return restore ? "" : "qrc:///redesign/player/maximize.svg"
                    case CSDButton.Close: return "qrc:///redesign/player/close.svg"
                    }
                    return ""
                }
                description: {
                    switch (modelData.type) {
                    case CSDButton.Minimize: return qsTr("Minimize")
                    case CSDButton.MaximizeRestore:
                        return MainCtx.intfMainWindow.visibility === Window.Maximized ? qsTr("Restore") : qsTr("Maximize")
                    case CSDButton.Close: return qsTr("Close")
                    }
                    return ""
                }
                onClicked: modelData.click()

                // Restore, while maximized: the set has no such icon, two
                // windows drawn with the strokes of the maximize one (24 px
                // box, 2 px #8F8F8F)
                Item {
                    anchors.centerIn: parent
                    width: VLCStyle.redesign.iconSize
                    height: width
                    visible: windowButton.restore

                    readonly property real unit: width / 24
                    readonly property color stroke: VLCStyle.redesign.color("#8F8F8F", "#2E2E2E")

                    // the window behind: its top and right sides
                    Rectangle {
                        x: 8 * parent.unit; y: 5 * parent.unit
                        width: 11 * parent.unit; height: 2 * parent.unit
                        color: parent.stroke
                    }
                    Rectangle {
                        x: 17 * parent.unit; y: 5 * parent.unit
                        width: 2 * parent.unit; height: 11 * parent.unit
                        color: parent.stroke
                    }
                    // the window in front
                    Rectangle {
                        x: 5 * parent.unit; y: 8 * parent.unit
                        width: 11 * parent.unit; height: 11 * parent.unit
                        color: "transparent"
                        border.color: parent.stroke
                        border.width: 2 * parent.unit
                    }
                }

                // Figma "Player - customize": outlined (36 x 24)
                DashedRect {
                    anchors.centerIn: parent
                    width: parent.width
                    height: VLCStyle.dp(24, VLCStyle.scale)
                    visible: VLCStyle.customizing
                    color: VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
                }

                // Preserve the native Windows hit-test/snap rectangles: only
                // while the button shows (a bar faded out in fullscreen, a set
                // gone: no Close left there, invisible, at the top right)
                onVisibleChanged: Qt.callLater(updateRect)
                Component.onDestruction: releaseRect()
                property rect _registered: Qt.rect(0, 0, 0, 0)
                function releaseRect() {
                    const r = modelData ? modelData.rect : null
                    if (r && r.x === _registered.x && r.y === _registered.y
                            && r.width === _registered.width && r.height === _registered.height)
                        modelData.rect = Qt.rect(0, 0, 0, 0)
                }
                onWidthChanged: Qt.callLater(updateRect)
                onHeightChanged: Qt.callLater(updateRect)
                onXChanged: Qt.callLater(updateRect)
                Component.onCompleted: Qt.callLater(updateRect)
                Connections {
                    target: VLCStyle
                    function onAppWidthChanged() { Qt.callLater(windowButton.updateRect) }
                    function onAppHeightChanged() { Qt.callLater(windowButton.updateRect) }
                }
                // the set placed again or made wider or narrower
                Connections {
                    target: root
                    function onXChanged() { Qt.callLater(windowButton.updateRect) }
                    function onYChanged() { Qt.callLater(windowButton.updateRect) }
                    function onWidthChanged() { Qt.callLater(windowButton.updateRect) }
                    function onFullscreenHiddenChanged() { Qt.callLater(windowButton.updateRect) }
                }
                function updateRect() {
                    if (!visible || width <= 0) {
                        releaseRect()
                        return
                    }
                    const point = mapToItem(null, 0, 0)
                    _registered = Qt.rect(point.x, point.y, width, height)
                    modelData.rect = _registered
                }
            }

            CSDWindowButton {
                id: fullscreenButton
                height: root.height
                visible: modelData.type === CSDButton.MaximizeRestore && (!root.fullscreenHidden || root._fullscreen)
                color: root.color
                hoverColor: root.hoverColor
                isThemeDark: theme.palette.isDark
                iconSource: "qrc:///redesign/player/fullscreen.svg"
                // lit as the bar's fullscreen button (RedesignIconButton)
                iconColor: !root._fullscreen ? "transparent"
                           : VLCStyle.palette.isDark ? VLCStyle.redesign.accent : VLCStyle.redesign.accentDim
                description: root._fullscreen ? qsTr("Exit Fullscreen") : qsTr("Fullscreen")
                onClicked: {
                    if (root._fullscreen)
                        MainCtx.exitFullscreen()
                    else if (Player.hasVideoOutput)
                        Player.fullscreen = !Player.fullscreen
                    else
                        MainCtx.toggleInterfaceFullScreen()
                }

                DashedRect {
                    anchors.centerIn: parent
                    width: parent.width
                    height: VLCStyle.dp(24, VLCStyle.scale)
                    visible: VLCStyle.customizing
                    color: VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
                }
                CustomizeDragArea {
                    controlId: root.fullscreenControlId
                    iconSource: "qrc:///redesign/player/fullscreen.svg"
                }
            }
        }
    }
}
