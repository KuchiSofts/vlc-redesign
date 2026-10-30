/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl

import VLC.MainInterface
import VLC.Player
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// Cast to a TV: the Chromecasts, TVs and speakers of the network (found by
// mDNS and UPnP, Player.rendererManager) and this computer. The media goes
// on playing there where it was, with the subtitle shown burnt into the
// picture; the bar goes on controlling it.
RedesignIconButton {
    id: root

    signal requestLockUnlockAutoHide(bool lock)

    readonly property var manager: Player.rendererManager

    iconSource: "qrc:///redesign/icons/cast/default.svg"
    description: root.manager.useRenderer ? qsTr("Casting to %1").arg(root.manager.rendererName)
                                          : qsTr("Cast to TV")
    checked: root.manager.useRenderer
    AccessibleCompat.id: "castButton"

    onClicked: popup.visible ? popup.close() : popup.open()
    // open while its list of devices shows
    active: popup.visible

    function forceUnlock() {
        popup.close()
    }

    // the devices found while a video plays, before the list opens (when it
    // was opened before, see prepare())
    Timer {
        interval: 3000
        running: Player.playingState === Player.PLAYING_STATE_PLAYING && Player.videoTracks.count > 0
        onTriggered: root.manager.prepare()
    }

    T.Popup {
        id: popup
        objectName: "castPopup"
        parent: root
        width: VLCStyle.dp(340, VLCStyle.scale)
        // (a template popup has no size of its own)
        implicitHeight: implicitContentHeight + topPadding + bottomPadding
        x: Math.round(root.width - width)
        y: -height - VLCStyle.dp(10, VLCStyle.scale)
        // kept in the window wherever the button is in the bar
        margins: VLCStyle.dp(8, VLCStyle.scale)
        padding: VLCStyle.dp(14, VLCStyle.scale)
        focus: true
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside
        // a press outside only closes it, as a menu: the video under it
        // does not pause, the button pressed (its own included) does
        // nothing more
        modal: true
        dim: false

        onOpened: {
            // opened from the keyboard: the keyboard goes on in the list (its
            // ring on the first row, so that the arrows reach the rows and
            // not VLC's hotkeys); opened with the mouse, Tab first, as
            // elsewhere in the interface
            if (root.visualFocus)
                thisComputer.forceActiveFocus(Qt.TabFocusReason)
            root.requestLockUnlockAutoHide(true)
            root.manager.StartScan()
            popup.searchedLong = false
            searchTimer.restart()
        }
        onClosed: {
            root.requestLockUnlockAutoHide(false)
            searchTimer.stop()
        }

        // it looks for devices as long as the list is open (a TV turned on
        // then shows up, one turned off goes), not for one scan only; after
        // a while with none, the text says so (the dot still shows it looks)
        property bool searchedLong: false
        Timer {
            id: searchTimer
            interval: 1000
            repeat: true
            property int ticks: 0
            onRunningChanged: if (running) ticks = 0
            onTriggered: {
                ticks++
                if (ticks >= 8)
                    popup.searchedLong = true
                // (StartScan while one runs: 20 s more from now)
                if (ticks % 15 === 0)
                    root.manager.StartScan()
            }
        }

        background: Rectangle {
            color: VLCStyle.redesign.color(VLCStyle.redesign.windowBackground, "#c2c2c1")
            radius: VLCStyle.dp(10, VLCStyle.scale)
            // the frame of the main window
            border.width: 1
            border.color: VLCStyle.redesign.color(VLCStyle.redesign.windowBorder)
        }

        contentItem: ColumnLayout {
            // right to left (Hebrew, Arabic...): a popup is not under the
            // mirrored interface (the bar it opens from keeps its order)
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true

            spacing: VLCStyle.dp(4, VLCStyle.scale)

            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: VLCStyle.dp(6, VLCStyle.scale)
                Text {
                    Layout.fillWidth: true
                    // (the texts from the right in a right to left layout)
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    text: qsTr("Cast to")
                    color: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#2e2e2e")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                    font.weight: Font.Medium
                }
                // looking for devices
                Rectangle {
                    visible: root.manager.status === RendererManager.RUNNING
                    implicitWidth: VLCStyle.dp(8, VLCStyle.scale)
                    implicitHeight: implicitWidth
                    radius: width / 2
                    color: VLCStyle.redesign.accent
                    SequentialAnimation on opacity {
                        running: popup.visible && root.manager.status === RendererManager.RUNNING
                        loops: Animation.Infinite
                        NumberAnimation { from: 1; to: 0.25; duration: 600; easing.type: Easing.InOutQuad }
                        NumberAnimation { from: 0.25; to: 1; duration: 600; easing.type: Easing.InOutQuad }
                    }
                }
            }

            DeviceRow {
                id: thisComputer
                objectName: "castThisComputer"
                Layout.fillWidth: true
                name: qsTr("This computer")
                iconName: "ui"
                selected: !root.manager.useRenderer
                onClicked: {
                    root.manager.disableRenderer()
                    popup.close()
                }
            }

            // five devices at most in view, then it scrolls
            ListView {
                id: devices
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(contentHeight, VLCStyle.dp(44 * 5 + 4 * 4, VLCStyle.scale))
                visible: count > 0
                spacing: VLCStyle.dp(4, VLCStyle.scale)
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: root.manager
                // (the roles through model: the row has its own name and selected)
                delegate: DeviceRow {
                    id: deviceRow
                    width: ListView.view.width
                    name: model.name
                    selected: model.selected
                    // VLC_RENDERER_CAN_VIDEO: a screen, else a speaker
                    iconName: (model.flags & 2) ? "tv" : "music"
                    // how the one it plays on does
                    alert: model.selected && root.manager.castStatus === RendererManager.CastFailed
                    detail: alert ? qsTr("Not playing: choose it to try again")
                          : model.selected && root.manager.castStatus === RendererManager.CastConnecting
                            ? qsTr("Connecting…")
                          : (model.flags & 2) ? "" : qsTr("Sound only")
                    // (right click, or the menu key): hidden from the list
                    hideable: !model.selected
                    onHideRequested: (x, y) => {
                        deviceMenu.row = index
                        deviceMenu.popup(deviceRow, x, y)
                    }
                    onClicked: {
                        // the one it plays on: again only when it failed
                        if (!model.selected)
                            root.manager.select(index)
                        else if (alert)
                            root.manager.retry()
                        popup.close()
                    }
                }
            }

            // the devices hidden from the list (kept in the settings)
            DeviceRow {
                objectName: "castShowHidden"
                Layout.fillWidth: true
                visible: root.manager.hiddenCount > 0
                name: qsTr("Show %1 hidden device", "Show %1 hidden devices",
                           root.manager.hiddenCount).arg(root.manager.hiddenCount)
                iconName: "refresh"
                onClicked: root.manager.showHiddenDevices()
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: VLCStyle.dp(6, VLCStyle.scale)
                visible: devices.count === 0
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                text: root.manager.status === RendererManager.RUNNING && !popup.searchedLong
                      ? qsTr("Looking for TVs and Chromecasts on your network…")
                      : qsTr("No device found. Turn your TV or Chromecast on, on the same network as this computer.")
                color: VLCStyle.redesign.color("#8F8F8F", "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
            }

            // the picture here too while it plays there (the sound plays there)
            T.Switch {
                id: displayHere
                objectName: "castDisplayHere"
                Layout.fillWidth: true
                Layout.topMargin: VLCStyle.dp(6, VLCStyle.scale)
                implicitHeight: VLCStyle.dp(32, VLCStyle.scale)
                focusPolicy: Qt.StrongFocus
                // the arrows go on from it as from the rows (it kept them);
                // the last one: down goes round to the first row
                Keys.onDownPressed: (event) => thisComputer.forceActiveFocus(Qt.TabFocusReason)
                Keys.onUpPressed: (event) => {
                    const previous = displayHere.nextItemInFocusChain(false)
                    if (previous)
                        previous.forceActiveFocus(Qt.BacktabFocusReason)
                }
                text: qsTr("Show the video here too")
                checked: root.manager.displayHere
                // (casting to a Chromecast: it has no picture here, the
                // note under it says why)
                enabled: root.manager.displayHereAvailable
                opacity: enabled ? 1 : 0.45
                onToggled: root.manager.displayHere = checked
                Accessible.name: text

                // (on the right in a right to left layout, on on its left)
                indicator: Rectangle {
                    x: displayHere.mirrored ? parent.width - width - VLCStyle.dp(8, VLCStyle.scale)
                                            : VLCStyle.dp(8, VLCStyle.scale)
                    anchors.verticalCenter: parent.verticalCenter
                    width: VLCStyle.dp(40, VLCStyle.scale)
                    height: VLCStyle.dp(22, VLCStyle.scale)
                    radius: height / 2
                    color: displayHere.checked ? VLCStyle.redesign.accent
                                               : VLCStyle.redesign.color("#262626", "#9c9c9b")
                    border.width: displayHere.visualFocus ? 2 : 0
                    border.color: VLCStyle.redesign.focusColor
                    Rectangle {
                        x: (displayHere.checked !== displayHere.mirrored) ? parent.width - width - 3 : 3
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.height - 6
                        height: width
                        radius: width / 2
                        color: "#eaeaea"
                        Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
                    }
                }
                contentItem: Text {
                    leftPadding: displayHere.mirrored ? 0 : VLCStyle.dp(58, VLCStyle.scale)
                    rightPadding: displayHere.mirrored ? VLCStyle.dp(58, VLCStyle.scale) : 0
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    text: displayHere.text
                    color: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#2e2e2e")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    elide: Text.ElideRight
                }
            }

            // (a Chromecast plays it only there)
            Text {
                Layout.fillWidth: true
                visible: !root.manager.displayHereAvailable
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                text: qsTr("With a Chromecast, the video plays only there.")
                color: VLCStyle.redesign.color("#8F8F8F", "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: VLCStyle.dp(8, VLCStyle.scale)
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                text: qsTr("The subtitles shown go to the TV with the picture.")
                color: VLCStyle.redesign.color("#5F5F5F", "#6b6b6b")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
            }
        }
    }

    // the menu of a device of the list
    Widgets.RedesignMenu {
        id: deviceMenu
        property int row: -1
        menuWidth: VLCStyle.dp(220, VLCStyle.scale)
        Widgets.RedesignMenuItem {
            // (a menu does not inherit the mirroring of the window)
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            text: qsTr("Hide this device")
            iconName: "remove"
            onTriggered: root.manager.hideDevice(deviceMenu.row)
        }
    }

    component DeviceRow: T.ItemDelegate {
        id: row
        property string name
        property string detail
        property string iconName
        property bool selected: false
        // the detail says it does not work
        property bool alert: false
        // it has a menu to hide it (hideRequested, where it opens)
        property bool hideable: false
        signal hideRequested(real x, real y)

        TapHandler {
            enabled: row.hideable
            acceptedButtons: Qt.RightButton
            onTapped: (eventPoint) => row.hideRequested(eventPoint.position.x, eventPoint.position.y)
        }
        // the arrows go from row to row as Tab does (and Space or Enter
        // chooses one)
        Keys.onDownPressed: (event) => {
            const next = row.nextItemInFocusChain(true)
            if (next)
                next.forceActiveFocus(Qt.TabFocusReason)
        }
        Keys.onUpPressed: (event) => {
            const previous = row.nextItemInFocusChain(false)
            if (previous)
                previous.forceActiveFocus(Qt.BacktabFocusReason)
        }
        Keys.onMenuPressed: (event) => {
            if (!row.hideable)
                return
            event.accepted = true
            row.hideRequested(0, row.height)
        }

        implicitHeight: VLCStyle.dp(44, VLCStyle.scale)
        focusPolicy: Qt.StrongFocus
        Accessible.name: name + (selected ? ", " + qsTr("playing here") : "")

        background: Rectangle {
            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: row.selected ? VLCStyle.redesign.color("#1b1b1b", "#ababaa")
                 : row.hovered || row.activeFocus ? VLCStyle.redesign.color("#151515", "#b5b5b4") : "transparent"
            border.width: row.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.focusColor
        }
        contentItem: RowLayout {
            spacing: VLCStyle.dp(10, VLCStyle.scale)
            Image {
                id: deviceIcon
                // "ui" has a stroke in the accent dim: with another accent,
                // its grey part and the stroke tinted above it
                readonly property bool twoTone: row.iconName === "ui" && !VLCStyle.redesign.defaultAccent
                Layout.leftMargin: VLCStyle.dp(8, VLCStyle.scale)
                source: "qrc:///redesign/icons/" + row.iconName + (twoTone ? "/default-grey.svg" : "/default.svg")
                sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                Impl.ColorImage {
                    anchors.fill: parent
                    visible: deviceIcon.twoTone
                    source: visible ? "qrc:///redesign/icons/" + row.iconName + "/default-accent.svg" : ""
                    sourceSize: parent.sourceSize
                    color: VLCStyle.redesign.accentDim
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    Layout.fillWidth: true
                    // (a device's name in Latin letters on the side where
                    // reading starts in Hebrew too, under its Hebrew rows)
                    horizontalAlignment: Text.AlignLeft
                    text: row.name
                    color: row.selected ? VLCStyle.redesign.accent
                                        : VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#2e2e2e")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    visible: row.detail !== ""
                    text: row.detail
                    color: row.alert ? VLCStyle.redesign.accent : VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(11, VLCStyle.scale)
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                }
            }
            Impl.ColorImage {
                // drawn at the size shown (an SVG: sharp at any scale and zoom)
                sourceSize: Qt.size(Layout.preferredWidth, Layout.preferredHeight)
                Layout.rightMargin: VLCStyle.dp(8, VLCStyle.scale)
                visible: row.selected
                source: "qrc:///redesign/track-check-active.svg"
                color: VLCStyle.redesign.accentTint
                Layout.preferredWidth: VLCStyle.dp(20, VLCStyle.scale)
                Layout.preferredHeight: VLCStyle.dp(20, VLCStyle.scale)
            }
        }
    }
}
