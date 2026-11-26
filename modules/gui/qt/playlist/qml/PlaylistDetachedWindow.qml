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
import VLC.Util
import VLC.Widgets as Widgets


// The Play Queue in a window of its own (View > Docked Play Queue, off), as
// the design draws the queue: the card of the player (Figma "Controls":
// queue, bookmarks, chapters, 400 px wide) under a title bar in the main
// window's style (40 px, the title centred, Close on the far side), in a
// frame of the main window's border and the card's 6 px corners. No system
// title bar: the bar moves the window, the frame's edges resize it.
Window {
    id: root

    visible: _componentCompleted && MainCtx.playqueuePanel.visible

    transientParent: MainCtx.intfMainWindow

    // The play queue page of the card (drag and drop, selection, navigation)
    readonly property Item playlistView: cardLoader.item ? cardLoader.item.queue : null

    readonly property real _frame: 1
    readonly property real _radius: VLCStyle.dp(6, VLCStyle.scale)
    readonly property real _grip: VLCStyle.dp(5, VLCStyle.scale)

    // the card's width in the design, and no narrower than its queue needs
    width: VLCStyle.dp(400, VLCStyle.scale) + 2 * _frame
    minimumWidth: Math.max(VLCStyle.dp(320, VLCStyle.scale),
                           playlistView ? playlistView.minimumWidth : 0) + 2 * _frame
    minimumHeight: VLCStyle.redesign.topbarHeight + VLCStyle.dp(200, VLCStyle.scale)

    title: qsTr("Play Queue")
    flags: Qt.Window | Qt.FramelessWindowHint
    // (the corners outside the frame's radius show what is behind the window)
    color: "transparent"

    // Right to left (Hebrew, Arabic...): a window of its own is not under
    // the mirrored interface, it mirrors itself
    readonly property bool _rtl: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.enabled: _rtl
    LayoutMirroring.childrenInherit: true

    property bool _componentCompleted: false

    onVisibleChanged: {
        if (visible) {
            console.assert(transientParent)
            // beside the main window: on its left in a right to left layout,
            // kept inside its screen when there is no room on that side
            MainCtx.placeWindowBesideParent(this, _rtl)
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

    // The frame: the card's colour in full (nothing is behind it here), the
    // main window's border, the card's corners
    Rectangle {
        anchors.fill: parent
        radius: root._radius
        color: VLCStyle.redesign.color("#191919")
        border.width: root._frame
        border.color: VLCStyle.redesign.color(VLCStyle.redesign.windowBorder)
    }

    // The title bar, as the main window's
    Rectangle {
        id: titleBar

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: root._frame
        }
        height: VLCStyle.redesign.topbarHeight
        topLeftRadius: root._radius - root._frame
        topRightRadius: topLeftRadius
        color: VLCStyle.redesign.color(VLCStyle.redesign.playerTopbarBackground)

        FontMetrics {
            id: titleMetrics
            font: titleText.font
        }

        Text {
            id: titleText

            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.max(0, parent.width - 2 * (closeButton.width + VLCStyle.margin_normal))
            height: parent.height
            textFormat: Text.PlainText
            text: root.title
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
        }

        // dragged, the bar moves the window
        DragHandler {
            target: null
            grabPermissions: TapHandler.CanTakeOverFromAnything
            onActiveChanged: {
                if (active)
                    root.startSystemMove()
            }
        }

        Widgets.CSDWindowButton {
            id: closeButton

            anchors.right: parent.right
            height: parent.height
            color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
            hoverColor: "#c42b1c"
            isThemeDark: VLCStyle.palette.isDark
            iconSource: "qrc:///redesign/player/close.svg"
            description: qsTr("Close")
            onClicked: root.close()
        }
    }

    Loader {
        id: cardLoader

        anchors {
            left: parent.left
            right: parent.right
            top: titleBar.bottom
            bottom: parent.bottom
            leftMargin: root._frame
            rightMargin: root._frame
            bottomMargin: root._frame
        }
        focus: true
        // (by its address: the player's module imports this one)
        source: "qrc:///qt/qml/VLC/Player/QueueCard.qml"
        onLoaded: {
            item.windowed = true
            item.focus = true
            item.Navigation.cancelAction = () => { root.close() }
        }
    }

    // The frame's edges and corners resize the window
    Repeater {
        model: [
            { edge: Qt.LeftEdge, shape: Qt.SizeHorCursor },
            { edge: Qt.RightEdge, shape: Qt.SizeHorCursor },
            { edge: Qt.TopEdge, shape: Qt.SizeVerCursor },
            { edge: Qt.BottomEdge, shape: Qt.SizeVerCursor },
            { edge: Qt.LeftEdge | Qt.TopEdge, shape: Qt.SizeFDiagCursor },
            { edge: Qt.RightEdge | Qt.BottomEdge, shape: Qt.SizeFDiagCursor },
            { edge: Qt.RightEdge | Qt.TopEdge, shape: Qt.SizeBDiagCursor },
            { edge: Qt.LeftEdge | Qt.BottomEdge, shape: Qt.SizeBDiagCursor }
        ]

        MouseArea {
            required property var modelData

            readonly property int edge: modelData.edge
            readonly property bool sideways: (edge & (Qt.LeftEdge | Qt.RightEdge)) !== 0
            readonly property bool upright: (edge & (Qt.TopEdge | Qt.BottomEdge)) !== 0
            readonly property bool corner: sideways && upright

            x: (edge & Qt.RightEdge) ? root.width - width : 0
            y: (edge & Qt.BottomEdge) ? root.height - height : 0
            width: sideways ? (corner ? 2 : 1) * root._grip : root.width
            height: upright ? (corner ? 2 : 1) * root._grip : root.height
            z: corner ? 3 : 2
            cursorShape: modelData.shape
            acceptedButtons: Qt.LeftButton
            onPressed: root.startSystemResize(edge)
        }
    }
}
