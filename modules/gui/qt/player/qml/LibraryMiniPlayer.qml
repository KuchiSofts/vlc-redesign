/*
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or later.
 */
import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Playlist
import VLC.Player
import VLC.Style
import VLC.Widgets as Widgets

T.Control {
    id: root
    objectName: "libraryMiniPlayer"
    Accessible.name: qsTr("Mini player")

    property real dragXMin: 0
    property real dragXMax: 0
    property real dragYMin: 0
    property real dragYMax: 0
    property bool queueExpanded: false
    readonly property bool lightTheme: !VLCStyle.palette.isDark
    readonly property string folderName: {
        const parts = decodeURIComponent(Player.url.toString()).split("/")
        return parts.length > 1 ? parts[parts.length - 2] : ""
    }

    // Figma "Miniplayer": padding 8, "Frame 112" of 230, 20, "Frame 115"
    // (4, the title 19, 14, the row 32, 4)
    padding: VLCStyle.dp(8, VLCStyle.scale)
    implicitWidth: VLCStyle.dp(391, VLCStyle.scale)
    implicitHeight: topPadding + bottomPadding +
                    (queueExpanded ? queueArea.height + VLCStyle.dp(6, VLCStyle.scale) : 0) +
                    _frameHeight + VLCStyle.dp(20 + 4 + 19 + 14 + 32 + 4, VLCStyle.scale)
    // "Frame 112": the video and the seek bar, 230 for the video of 211;
    // narrower, the video is lower (16:9) and the frame with it
    readonly property real _frameHeight: video.height + VLCStyle.dp(19, VLCStyle.scale)
    background: Rectangle {
        color: root.lightTheme ? "#ababaa" : VLCStyle.redesign.playerControlBackground
        radius: VLCStyle.dp(14, VLCStyle.scale)
        border.width: root.lightTheme ? 1 : 0
        border.color: "#858585"

        // Figma: drop shadow 0 0, blur 11.8, #000000 38 %
        Widgets.RoundedRectangleShadow {
            visible: !root.lightTheme
            color: Qt.rgba(0, 0, 0, 0.38)
            blurRadius: 5.9 * VLCStyle.scale // (dp() rounds to whole pixels)
        }
    }
    contentItem: Item {
        MiniPlayerQueue {
            id: queueArea
            width: parent.width
            height: implicitHeight
            visible: root.queueExpanded
        }

        PIPPlayer {
            id: video
            objectName: "miniPlayerVideo"
            y: root.queueExpanded ? queueArea.height + VLCStyle.dp(6, VLCStyle.scale) : 0
            width: parent.width
            // Figma: 375 x 210.94
            height: Math.round(width * 9 / 16)
            dragTarget: root
            dragXMin: root.dragXMin
            dragXMax: root.dragXMax
            dragYMin: root.dragYMin
            dragYMax: root.dragYMax
            hideControls: false
        }

        // the rounded corners of the video (radius 6): the panel around them,
        // as far as its edge (8: the outer radius is the panel's 14)
        Rectangle {
            readonly property real cover: root.leftPadding
            x: video.x - cover
            y: video.y - cover
            width: video.width + 2 * cover
            height: video.height + 2 * cover
            radius: VLCStyle.dp(6, VLCStyle.scale) + cover
            color: "transparent"
            border.width: cover
            border.color: root.lightTheme ? "#ababaa" : VLCStyle.redesign.playerControlBackground
        }
        // Figma: a border of 2 #090909 inside
        Rectangle {
            x: video.x
            y: video.y
            width: video.width
            height: video.height
            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: "transparent"
            border.width: VLCStyle.dp(2, VLCStyle.scale)
            border.color: root.lightTheme ? "#dedede" : VLCStyle.redesign.seekBackground
        }

        Controls.Slider {
            id: seek
            objectName: "miniPlayerSeek"
            // Figma "Frame 21": 4 under the video, 375 x 16
            y: video.y + video.height + VLCStyle.dp(4, VLCStyle.scale)
            z: 1
            width: parent.width
            height: VLCStyle.dp(16, VLCStyle.scale)
            padding: 0
            from: 0
            to: 1
            value: Player.position
            enabled: Player.seekable
            Accessible.name: qsTr("Playback position")
            // Dragged: key frame seeks, at most one every 150 ms (a precise
            // seek decodes up to seconds of frames), the release precisely,
            // as SliderBar does
            onMoved: {
                if (!pressed) {
                    Player.position = value
                    return
                }
                fastSeekTimer.pending = value
                fastSeekTimer.moved = true
                if (!fastSeekTimer.running)
                    fastSeekTimer.start()
            }
            onPressedChanged: {
                if (pressed)
                    return
                fastSeekTimer.stop()
                fastSeekTimer.pending = -1
                // (a click moves it on release, still pressed)
                if (fastSeekTimer.moved)
                    Player.position = value
                fastSeekTimer.moved = false
            }
            Timer {
                id: fastSeekTimer
                property real pending: -1
                property bool moved: false
                interval: 150
                onTriggered: {
                    if (pending < 0 || !seek.pressed)
                        return
                    Player.setPositionFast(pending)
                    pending = -1
                    restart()
                }
            }
            Keys.onLeftPressed: Player.jumpBwd()
            Keys.onRightPressed: Player.jumpFwd()
            // Figma: the track radius 6 #090909, the fill 2 inside, radius 4,
            // #371F06
            background: Rectangle {
                color: root.lightTheme ? "#dedede" : VLCStyle.redesign.seekBackground
                radius: VLCStyle.dp(6, VLCStyle.scale)
                Rectangle {
                    readonly property real inset: VLCStyle.dp(2, VLCStyle.scale)
                    x: inset
                    y: inset
                    visible: width > 0
                    width: Math.max(0, (parent.width - 2 * inset) * seek.visualPosition)
                    height: parent.height - 2 * inset
                    radius: VLCStyle.dp(4, VLCStyle.scale)
                    color: root.lightTheme
                           ? Qt.rgba(VLCStyle.redesign.accent.r, VLCStyle.redesign.accent.g,
                                     VLCStyle.redesign.accent.b, 0.35)
                           : VLCStyle.redesign.accentBackground
                }
            }
            handle: Rectangle {
                width: VLCStyle.dp(7, VLCStyle.scale)
                height: VLCStyle.dp(7, VLCStyle.scale)
                radius: width / 2
                x: Math.max(0, Math.min(seek.width - width, seek.visualPosition * seek.width - width / 2))
                y: (seek.height - height) / 2
                color: VLCStyle.redesign.accent
                visible: seek.hovered || seek.pressed || seek.visualFocus
            }
        }

        // Figma "Frame 115": padding 4 8, the title (box 19), 14, the row
        Text {
            id: titleLabel
            x: VLCStyle.dp(8, VLCStyle.scale)
            y: video.y + root._frameHeight + VLCStyle.dp(20 + 4, VLCStyle.scale)
            width: parent.width - 2 * x
            height: VLCStyle.dp(19, VLCStyle.scale)
            text: Player.name
            color: root.lightTheme ? "#4d4d4d" : VLCStyle.redesign.textSecondary
            elide: Text.ElideRight
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
            verticalAlignment: Text.AlignTop
            topPadding: VLCStyle.redesign.textBaseline(0, font.pixelSize) - titleMetrics.ascent
            FontMetrics {
                id: titleMetrics
                font: titleLabel.font
            }
        }

        // the folder and the artist (icon 24, 8, the name), 20 apart
        Row {
            id: details
            x: VLCStyle.dp(8, VLCStyle.scale)
            y: titleLabel.y + titleLabel.height + VLCStyle.dp(14, VLCStyle.scale)
            height: VLCStyle.dp(32, VLCStyle.scale)
            spacing: VLCStyle.dp(20, VLCStyle.scale)

            Repeater {
                model: [
                    { iconName: "folder", label: root.folderName },
                    { iconName: "artist", label: Player.artist }
                ]
                delegate: Row {
                    id: detail
                    required property var modelData
                    visible: detail.modelData.label.length > 0
                    height: details.height
                    spacing: VLCStyle.dp(8, VLCStyle.scale)
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        width: VLCStyle.redesign.iconSize
                        height: width
                        source: "qrc:///redesign/icons/" + detail.modelData.iconName + "/default.png"
                        sourceSize: Qt.size(width, height)
                        fillMode: Image.Pad
                        smooth: false
                    }
                    Text {
                        id: detailText
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(implicitWidth, VLCStyle.dp(120, VLCStyle.scale))
                        height: VLCStyle.dp(19, VLCStyle.scale)
                        text: detail.modelData.label
                        color: root.lightTheme ? "#4d4d4d" : VLCStyle.redesign.textSecondary
                        elide: Text.ElideRight
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                        verticalAlignment: Text.AlignTop
                        topPadding: VLCStyle.redesign.textBaseline(0, font.pixelSize) - detailMetrics.ascent
                        FontMetrics {
                            id: detailMetrics
                            font: detailText.font
                        }
                    }
                }
            }
        }

        // Figma "Icon Wrapper": 30 x 32, radius 4, padding 4
        Controls.Button {
            id: queueButton
            objectName: "miniPlayerQueueToggle"
            x: parent.width - width - VLCStyle.dp(8, VLCStyle.scale)
            y: details.y
            width: VLCStyle.dp(30, VLCStyle.scale)
            height: VLCStyle.dp(32, VLCStyle.scale)
            padding: VLCStyle.dp(4, VLCStyle.scale)
            Accessible.name: root.queueExpanded ? qsTr("Hide queue") : qsTr("Show queue")
            onClicked: root.queueExpanded = !root.queueExpanded
            background: Rectangle {
                radius: VLCStyle.dp(4, VLCStyle.scale)
                color: (queueButton.hovered || root.queueExpanded)
                       ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                       : "transparent"
            }
            contentItem: Item {
                Image {
                    width: VLCStyle.redesign.iconSize
                    height: width
                    source: "qrc:///redesign/icons/lists/default.png"
                    sourceSize: Qt.size(width, height)
                    fillMode: Image.Pad
                    smooth: false
                }
            }
        }
    }
}
