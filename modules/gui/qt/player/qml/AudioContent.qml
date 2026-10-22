/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Dialogs
import VLC.Style
import VLC.Widgets as Widgets
import VLC.Player
import VLC.PlayerControls
import VLC.Util

MouseArea {
    id: centerContent

    // the player's menu on a right click, as on a video
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    onClicked: (mouse) => {
        if (mouse.button === Qt.RightButton)
            playerRoot.showAudioContextMenu(centerContent.mapToGlobal(mouse.x, mouse.y))
    }

    required property Item musicView
    required property Item playerRoot
    required property Item topBarItem
    required property Item toggleControlBarItem
    required property Item controlBarItem
    property url coverSource
    property alias coverImage: cover
    property alias lyricsPanel: lyricsLoader
    readonly property alias audioControls: audioControls

    readonly property ColorContext colorContext: ColorContext {
        id: centerTheme
        colorSet: ColorContext.View
    }

    onWheel: (wheel) => {
        wheel.accepted = true
        wheelToVlc.qmlWheelEvent(wheel)
    }

    TouchWheelHandler {
        wheelToVLCConverter: wheelToVlc
    }

    WheelToVLCConverter {
        id: wheelToVlc

        onVlcWheelKey: (key) => MainCtx.sendVLCHotkey(key)
    }

    // ── Two-panel lyrics layout ────────────────────────
    Item {
        id: leftSideParent

        visible: (opacity > 0.0) || (width > 0.0)

        opacity: (lyricsLoader.item ? 1.0 : 0.0)

        Behavior on opacity {
            id: opacityBehavior

            enabled: false

            NumberAnimation {
                duration: VLCStyle.duration_long

                easing.type: Easing.InOutSine
            }

            Component.onCompleted: {
                // The animation should not be used at initialization:
                Qt.callLater(() => { opacityBehavior.enabled = true })
            }
        }

        anchors.top: parent.top
        anchors.topMargin: VLCStyle.margin_large
        anchors.bottom: parent.bottom
        anchors.left: parent.left

        width: lyricsLoader.item ? (parent.width * 0.6) : 0.0

        Behavior on width {
            id: widthBehavior

            enabled: false

            NumberAnimation {
                duration: VLCStyle.duration_long

                easing.type: Easing.InOutSine
            }

            Component.onCompleted: {
                // The animation should not be used at initialization:
                Qt.callLater(() => { widthBehavior.enabled = true })
            }
        }

        Loader {
            id: lyricsLoader

            active: musicView.lyricsLayoutActive
            sourceComponent: T.Pane {
                // Swallow wheel events so scrolling the lyrics
                // does not bubble up to the player and change
                // the volume.
                wheelEnabled: true

                // Expose the inner Flickable's sync flag so the
                // Loader's `item.lyricsSyncToPlayback` reaches
                // through to LyricsFlickable in both directions.
                property alias lyricsSyncToPlayback: lyricsFlickable.lyricsSyncToPlayback

                implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                        implicitContentWidth + leftPadding + rightPadding)
                implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                         implicitContentHeight + topPadding + bottomPadding)

                contentItem: LyricsFlickable {
                    id: lyricsFlickable

                    onPlayerPositionChangeRequested: (time) => {
                        Player.setTime(time)
                        lyricsFlickable.lyricsSyncToPlayback = true
                    }
                }
            }
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            height: Math.min(implicitHeight, parent.height)
        }
    }

    Item {
        id: rightSideParent

        visible: leftSideParent.visible

        anchors.top: parent.top
        anchors.topMargin: VLCStyle.margin_large
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.left: leftSideParent.right
    }

    ColumnLayout {
        parent: leftSideParent.visible ? rightSideParent : centerContent

        // centred on a whole pixel (Figma: the cover at 591 of the 1920)
        anchors.verticalCenter: parent.verticalCenter
        x: Math.round((parent.width - width) / 2)
        spacing: 0

        Item {
            id: coverItem
            readonly property real side: Math.max(0, Math.min(
                VLCStyle.dp(738, VLCStyle.scale),
                centerContent.height - VLCStyle.dp(170, VLCStyle.scale),
                (musicView.lyricsLayoutActive ? rightSideParent.width : centerContent.width)
                    - VLCStyle.dp(48, VLCStyle.scale)))
            // Figma: 738 x 737
            Layout.preferredWidth: side
            Layout.preferredHeight: Math.round(side * 737 / 738)
            Layout.alignment: Qt.AlignHCenter

            Widgets.ImageExt {
                id: cover
                anchors.fill: parent
                radius: VLCStyle.dp(20, VLCStyle.scale)
                fillMode: Image.PreserveAspectCrop
                cache: false
                asynchronous: true
                sourceSize: Qt.size(decodedSide, decodedSide)
                readonly property real eDPR: MainCtx.effectiveDevicePixelRatio(Window.window) || 1
                readonly property int exactSide: Math.ceil(coverItem.side * eDPR)
                // decoded again (and the backdrop blurred again) once a resize
                // pauses, not at each of its steps; at its size then (1:1)
                property int decodedSide: exactSide
                onExactSideChanged: decodeTimer.restart()
                Timer {
                    id: decodeTimer
                    interval: 200
                    onTriggered: cover.decodedSide = cover.exactSide
                }
                readonly property real sar: paintedHeight > 0 ? paintedWidth / paintedHeight : 1
                readonly property url targetSource: VLCAccessImage.uri(centerContent.coverSource)
                source: targetSource
                onTargetSourceChanged: source = targetSource

                onStatusChanged: {
                    if (status === Image.Error)
                        source = VLCStyle.noArtAlbumCover
                }

                Accessible.role: Accessible.Graphic
                Accessible.name: qsTr("Cover")

                // Figma: drop shadow #FFFFFF 3 %, blur 90.5 (sigma 45.25),
                // spread 64 (the box and its radius grown by 64)
                Widgets.RoundedRectangleShadow {
                    readonly property real spread: VLCStyle.dp(64, VLCStyle.scale)
                    color: Qt.rgba(1, 1, 1, .03)
                    blurRadius: 45.25 * VLCStyle.scale // (dp() rounds to whole pixels)
                    width: parent.width + 2 * spread + 2 * blurRadius * compensationFactor
                    height: parent.height + 2 * spread + 2 * blurRadius * compensationFactor
                    radius: VLCStyle.dp(20, VLCStyle.scale) + spread
                    // drawn once in a texture: the shader would run on every
                    // frame of the player over a megapixel
                    layer.enabled: true
                }
            }
        }

        Widgets.SubtitleLabel {
            id: albumLabel

            // across the column, Figma's box (the advance of Inter 3.19
            // rounded up, measured 16 times larger less the unit per glyph the
            // bundled font adds) centred in it, not rounded (the layout would
            // put a narrower item on a whole pixel); the column as wide as the
            // box, not wider (the padding stays out of the layout)
            Layout.fillWidth: true
            Layout.preferredWidth: figmaBox
            // Figma: space-between, the gaps above the cover, between the
            // cover and the text and under the text are equal
            Layout.topMargin: Math.max(VLCStyle.margin_large,
                                       (centerContent.height - coverItem.Layout.preferredHeight - Layout.preferredHeight) / 3)
            // the text box of the design (29 px for 24 px), the line centred
            Layout.preferredHeight: Math.round(1.2109375 * font.pixelSize)
            verticalAlignment: Text.AlignTop
            topPadding: VLCStyle.redesign.textBaseline(0, font.pixelSize) - albumMetrics.ascent
            bottomPadding: 0
            FontMetrics {
                id: albumMetrics
                font: albumLabel.font
            }

            Binding on visible {
                delayed: true
                when: albumLabel.componentCompleted
                value: albumLabel.text.length > 0 && centerContent.height > (albumLabel.y + albumLabel.height)
            }

            // Figma "Player - Music": "untangle - juxtopposed", regular
            text: Player.title && Player.artist ? Player.title + " - " + Player.artist : (Player.title || "")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(24, VLCStyle.scale)
            font.weight: Font.Medium
            readonly property real figmaBox: Math.ceil(albumExact.advanceWidth / 16
                                                       - text.length * font.pixelSize / 2816 - 0.001)
            horizontalAlignment: figmaBox < width ? Text.AlignLeft : Text.AlignHCenter
            leftPadding: figmaBox < width ? (width - figmaBox) / 2 : 0
            TextMetrics {
                id: albumExact
                font.family: albumLabel.font.family
                font.weight: albumLabel.font.weight
                font.pixelSize: albumLabel.font.pixelSize * 16
                text: albumLabel.text
            }
            color: VLCStyle.redesign.color(VLCStyle.redesign.textSecondary, "#4d4d4d")
            Accessible.description: qsTr("Title and artist")

            property bool componentCompleted: false

            Component.onCompleted: {
                componentCompleted = true
            }
        }

        Widgets.MenuLabel {
            id: artistLabel

            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: VLCStyle.margin_small

            Binding on visible {
                delayed: true
                when: artistLabel.componentCompleted
                // The design shows the title and artist only
                value: false
            }

            text: Player.album
            font.weight: Font.Normal
            horizontalAlignment: Text.AlignHCenter
            color: centerTheme.fg.primary
            Accessible.description: qsTr("Album")

            property bool componentCompleted: false

            Component.onCompleted: {
                componentCompleted = true
            }
        }

        Widgets.SubtitleLabel {
            id: lyricsLabel

            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: VLCStyle.margin_large
            // Pin width and height so the surrounding centered
            // column does not shift when the current lyric
            // changes between empty / 1 line / 2 lines. Text
            // exceeding two lines wraps then elides on line 2.
            Layout.preferredWidth: parent.parent.width - VLCStyle.margin_xlarge * 2
            Layout.preferredHeight: (lyricsLabelTextMetrics.height * 2) + VLCStyle.margin_xxxsmall

            visible: false

            Binding on visible {
                delayed: true
                when: lyricsLabel.componentCompleted

                // Stay visible (reserving the 2-line slot) for the
                // entire track when it has any lyrics, so the
                // layout does not collapse between lyric lines.
                value: Player.hasLyrics && !musicView.lyricsLayoutActive &&
                       (centerContent.height > (lyricsLabel.y + lyricsLabel.height))
            }

            property bool componentCompleted: false

            Component.onCompleted: {
                componentCompleted = true
            }

            elide: Text.ElideRight

            text: Player.currentLyric.text
            font.pixelSize: VLCStyle.fontSize_xlarge
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            color: centerTheme.fg.primary

            // TODO: Do not use `Binding` when minimum Qt is 6.12:
            Binding {
                target: lyricsLabel
                property: "mutabilityGroup"
                when: (lyricsLabel.mutabilityGroup !== undefined)
                value: (Player.playingState === Player.PLAYING_STATE_PLAYING) ? Item.ModerateMutabilityGroup
                                                                              : Item.StaticMutabilityGroup
            }

            TextMetrics {
                id: lyricsLabelTextMetrics
                font: lyricsLabel.font
                text: "TEXT"
            }

            Accessible.role: Accessible.StaticText
            Accessible.name: qsTr("Lyrics")
            Accessible.description: text
        }

        Widgets.NavigableRow {
            id: audioControls

            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: VLCStyle.margin_large

            Binding on visible {
                delayed: true
                when: audioControls.componentCompleted
                // The jumps are in the control bar of the design; the
                // visualizer (Figma "Visualizer") and, for tracks with
                // lyrics, the lyrics button stay
                value: Player.videoTracks.count === 0
                       && centerContent.height > (audioControls.y + audioControls.height)
            }

            focus: true
            spacing: VLCStyle.margin_xxsmall
            Navigation.parentItem: playerRoot
            Navigation.upItem: topBarItem
            Navigation.downItem: syncToPlaybackCheckBox

            property bool componentCompleted: false

            Component.onCompleted: {
                componentCompleted = true
            }

            Widgets.IconToolButton {
                visible: false
                text: VLCIcons.skip_back
                font.pixelSize: VLCStyle.icon_audioPlayerButton
                onClicked: Player.jumpBwd()
                description: qsTr("Step back")

                AccessibleCompat.id: "audioStepBackButton"
            }

            Widgets.IconToolButton {
                text: VLCIcons.visualization
                font.pixelSize: VLCStyle.icon_audioPlayerButton
                onClicked: DialogsProvider.audioVisualizationDialog()
                description: qsTr("Visualizer")

                AccessibleCompat.id: "audioVisualizationButton"
            }

            Widgets.IconToolButton {
                visible: Player.hasLyrics
                text: VLCIcons.topbar_music
                font.pixelSize: VLCStyle.icon_audioPlayerButton
                checked: MainCtx.lyricsMode
                onClicked: MainCtx.lyricsMode = !MainCtx.lyricsMode
                description: qsTr("Lyrics")

                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Toggle lyrics view")
                AccessibleCompat.id: "audioLyricsButton"
            }

            Widgets.IconToolButton{
                visible: false
                text: VLCIcons.skip_for
                font.pixelSize: VLCStyle.icon_audioPlayerButton
                onClicked: Player.jumpFwd()
                description: qsTr("Step forward")

                AccessibleCompat.id: "audioStepForwardButton"
            }
        }

        // "Sync to Playback" checkbox pinned to bottom of right panel
        Widgets.CheckBoxExt {
            id: syncToPlaybackCheckBox

            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: VLCStyle.margin_small

            visible: false

            Binding on visible {
                delayed: true
                when: syncToPlaybackCheckBox.componentCompleted
                value: musicView.lyricsLayoutActive &&
                       centerContent.height > (syncToPlaybackCheckBox.y + syncToPlaybackCheckBox.height)
            }

            property bool componentCompleted: false

            Component.onCompleted: {
                componentCompleted = true
            }

            text: qsTr("Sync lyrics to playback")
            // Use a Binding element rather than an inline binding
            // so manual clicks (which write `checked` directly)
            // don't permanently break the source-of-truth link.
            Binding on checked {
                when: lyricsLoader.item
                value: lyricsLoader.item ? lyricsLoader.item.lyricsSyncToPlayback : true
                restoreMode: Binding.RestoreBindingOrValue
            }
            onClicked: {
                if (lyricsLoader.item)
                    lyricsLoader.item.lyricsSyncToPlayback = checked
            }

            Accessible.role: Accessible.CheckBox
            Accessible.name: qsTr("Sync lyrics to playback position")
            AccessibleCompat.id: "syncToPlaybackCheckBox"

            Navigation.parentItem: playerRoot
            Navigation.upItem: audioControls
            Navigation.downItem: Player.isInteractive ? toggleControlBarItem : controlBarItem
        }
    }

    Widgets.SubtitleLabel {
        id: labelVolume

        anchors.right: parent.right
        anchors.top: parent.top

        anchors.rightMargin: VLCStyle.margin_normal
        anchors.topMargin: VLCStyle.margin_xxsmall

        visible: false

        text: qsTr("Volume %1%").arg(Math.round(Player.volume * 100))

        color: centerTheme.fg.primary

        font.weight: Font.Normal

        Connections {
            target: Player

            function onVolumeChanged() {
                animationVolume.restart()
            }
        }

        SequentialAnimation {
            id: animationVolume

            PropertyAction { target: labelVolume; property: "visible"; value: true }

            PauseAnimation { duration: VLCStyle.duration_humanMoment }

            PropertyAction { target: labelVolume; property: "visible"; value: false }
        }
    }

}
