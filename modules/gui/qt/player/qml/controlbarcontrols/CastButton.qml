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

import VLC.MainInterface
import VLC.Player
import VLC.Style
import VLC.Util

// Cast to a TV: the Chromecasts, TVs and speakers of the network (found by
// mDNS and UPnP, Player.rendererManager) and this computer. The media goes
// on playing there where it was, with the subtitle shown burnt into the
// picture; the bar goes on controlling it.
RedesignIconButton {
    id: root

    signal requestLockUnlockAutoHide(bool lock)

    readonly property var manager: Player.rendererManager

    iconSource: "qrc:///redesign/icons/cast/default.png"
    description: root.manager.useRenderer ? qsTr("Casting to %1").arg(root.manager.rendererName)
                                          : qsTr("Cast to TV")
    checked: root.manager.useRenderer
    AccessibleCompat.id: "castButton"

    onClicked: popup.visible ? popup.close() : popup.open()

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
            root.requestLockUnlockAutoHide(true)
            root.manager.StartScan()
        }
        onClosed: root.requestLockUnlockAutoHide(false)

        background: Rectangle {
            color: VLCStyle.redesign.windowBackground
            radius: VLCStyle.dp(10, VLCStyle.scale)
            border.width: 1
            border.color: VLCStyle.redesign.color(VLCStyle.redesign.separator, "#9c9c9b")
        }

        contentItem: ColumnLayout {
            spacing: VLCStyle.dp(4, VLCStyle.scale)

            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: VLCStyle.dp(6, VLCStyle.scale)
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Cast to")
                    color: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#1e1e1e")
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

            Text {
                Layout.fillWidth: true
                Layout.topMargin: VLCStyle.dp(6, VLCStyle.scale)
                visible: devices.count === 0
                wrapMode: Text.WordWrap
                text: root.manager.status === RendererManager.RUNNING
                      ? qsTr("Looking for TVs and Chromecasts on your network…")
                      : qsTr("No device found. Turn your TV or Chromecast on, on the same network as this computer.")
                color: VLCStyle.redesign.color("#8F8F8F", "#4d4d4d")
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
                text: qsTr("Show the video here too")
                checked: root.manager.displayHere
                onToggled: root.manager.displayHere = checked
                Accessible.name: text

                indicator: Rectangle {
                    x: VLCStyle.dp(8, VLCStyle.scale)
                    anchors.verticalCenter: parent.verticalCenter
                    width: VLCStyle.dp(40, VLCStyle.scale)
                    height: VLCStyle.dp(22, VLCStyle.scale)
                    radius: height / 2
                    color: displayHere.checked ? VLCStyle.redesign.accent
                                               : VLCStyle.redesign.color("#262626", "#9c9c9b")
                    border.width: displayHere.visualFocus ? 2 : 0
                    border.color: VLCStyle.redesign.focusColor
                    Rectangle {
                        x: displayHere.checked ? parent.width - width - 3 : 3
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.height - 6
                        height: width
                        radius: width / 2
                        color: "#eaeaea"
                        Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
                    }
                }
                contentItem: Text {
                    leftPadding: VLCStyle.dp(58, VLCStyle.scale)
                    verticalAlignment: Text.AlignVCenter
                    text: displayHere.text
                    color: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#1e1e1e")
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
                text: qsTr("With a Chromecast, the video plays only there.")
                color: VLCStyle.redesign.color("#8F8F8F", "#4d4d4d")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: VLCStyle.dp(8, VLCStyle.scale)
                wrapMode: Text.WordWrap
                text: qsTr("The subtitles shown go to the TV with the picture.")
                color: VLCStyle.redesign.color("#5F5F5F", "#6b6b6b")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
            }
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

        implicitHeight: VLCStyle.dp(44, VLCStyle.scale)
        focusPolicy: Qt.StrongFocus
        Accessible.name: name + (selected ? ", " + qsTr("playing here") : "")

        background: Rectangle {
            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: row.selected ? VLCStyle.redesign.color("#1b1b1b", "#b4b4b3")
                 : row.hovered || row.activeFocus ? VLCStyle.redesign.color("#151515", "#9c9c9b") : "transparent"
            border.width: row.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.focusColor
        }
        contentItem: RowLayout {
            spacing: VLCStyle.dp(10, VLCStyle.scale)
            Image {
                Layout.leftMargin: VLCStyle.dp(8, VLCStyle.scale)
                source: "qrc:///redesign/icons/" + row.iconName + "/default.png"
                sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    Layout.fillWidth: true
                    text: row.name
                    color: row.selected ? VLCStyle.redesign.accent
                                        : VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#1e1e1e")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    visible: row.detail !== ""
                    text: row.detail
                    color: row.alert ? VLCStyle.redesign.accent : VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(11, VLCStyle.scale)
                    elide: Text.ElideRight
                }
            }
            Image {
                Layout.rightMargin: VLCStyle.dp(8, VLCStyle.scale)
                visible: row.selected
                source: "qrc:///redesign/track-check-active.png"
                Layout.preferredWidth: VLCStyle.dp(20, VLCStyle.scale)
                Layout.preferredHeight: VLCStyle.dp(20, VLCStyle.scale)
            }
        }
    }
}
