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
import QtQuick.Templates as T
import VLC.Player
import VLC.Widgets as Widgets
import VLC.Style

Flickable {
    id: lyricsFlickable

    property bool lyricsSyncToPlayback: true
    property bool animationsEnabled: true
    property bool fadingEdgeEnabled: true
    // the layer the fading edges are drawn with: off while the view's size
    // changes on every frame (a new texture each time), the edges plain then
    property bool fadingEdgeLayering: true

    readonly property ColorContext colorContext: ColorContext {
        colorSet: ColorContext.View
    }

    implicitWidth: contentWidth
    implicitHeight: contentHeight

    contentWidth: lyricsColumn.width
    contentHeight: lyricsColumn.height

    interactive: height < implicitHeight

    T.ScrollBar.vertical: Widgets.ScrollBarExt {}

    boundsBehavior: Flickable.StopAtBounds
    clip: !fadingEdge.implicitClipping && (height < implicitHeight)

    signal playerPositionChangeRequested(vlcTime time)

    // Lyrics without times (a file's plain text): a page with no current
    // line. "Sync lyrics to playback" moves it with the playback then, the
    // middle of the view at the part of the lyrics the part played gives:
    // its top at the start, its end as the last half page is sung (#101, #109)
    readonly property bool timed: Player.lyricsTimed
    // moved a line at a time, not at each change of the position (the view
    // would be drawn again all through the song)
    readonly property real _pageStep: VLCStyle.fontSize_xlarge * 2

    function snapToCurrentLyric() {
        if (!timed) {
            const range = Math.max(0, lyricsFlickable.contentHeight - lyricsFlickable.height)
            // the middle of the view where the song is (its part played, of
            // the lyrics): the lines sung are about there, the end is shown
            // as its last half page is sung (#109; the page's top there kept
            // the lines sung near the top, the end at the very last moment)
            const sung = lyricsFlickable.contentHeight * Math.max(0, Math.min(1, Player.position))
            let target = Math.round(Math.max(0, Math.min(range, sung - lyricsFlickable.height / 2)))
            // within a step of an end, the end itself: moved a step at a
            // time, the page stayed short of its last lines as the song
            // ended
            if (range - target < _pageStep)
                target = Math.round(range)
            else if (target < _pageStep)
                target = 0
            if (Math.abs(target - lyricsFlickable.contentY) >= Math.min(_pageStep, range)
                || target === 0 || target === Math.round(range))
                lyricsFlickable.contentY = target
            return
        }

        const idx = Player.currentLyricIndex
        if (idx < 0)
            return

        const item = lyricsRepeater.itemAt(idx)
        if (!item)
            return

        const targetY = item.y + item.height / 2 - lyricsFlickable.height / 2
        lyricsFlickable.contentY = Math.max(0, Math.min(
            targetY, lyricsFlickable.contentHeight - lyricsFlickable.height))
    }

    function scheduleSnapToCurrentLyric() {
        if (lyricsSyncToPlayback)
            Qt.callLater(lyricsFlickable.snapToCurrentLyric)
    }

    // Disable auto-scroll when user manually flicks
    onFlickStarted: lyricsSyncToPlayback = false
    onDragStarted: lyricsSyncToPlayback = false
    onLyricsSyncToPlaybackChanged: scheduleSnapToCurrentLyric()
    onHeightChanged: scheduleSnapToCurrentLyric()
    onContentHeightChanged: scheduleSnapToCurrentLyric()

    Component.onCompleted: scheduleSnapToCurrentLyric()

    Behavior on contentY {
        enabled: animationsEnabled && lyricsSyncToPlayback
        SmoothedAnimation {
            velocity: VLCStyle.dp(300, VLCStyle.scale)
            duration: VLCStyle.duration_veryLong
        }
    }

    Connections {
        target: Player
        enabled: lyricsSyncToPlayback
        function onCurrentLyricIndexChanged(idx) {
            lyricsFlickable.snapToCurrentLyric()
        }
    }

    Connections {
        target: Player
        enabled: lyricsSyncToPlayback && !lyricsFlickable.timed
        function onPositionChanged() {
            lyricsFlickable.snapToCurrentLyric()
        }
    }

    onTimedChanged: scheduleSnapToCurrentLyric()

    Column {
        id: lyricsColumn

        width: lyricsFlickable.width

        Repeater {
            id: lyricsRepeater

            model: Player.syltLyrics

            delegate: Text {
                id: lyricDelegate

                // (the lyrics come from the file: shown as written, never as HTML)
                textFormat: Text.PlainText

                required property int index
                required property timedText modelData

                readonly property bool isCurrent: lyricsFlickable.timed && index === Player.currentLyricIndex
                // a page without times: every line as readable as the next
                readonly property bool emphasized: isCurrent || !lyricsFlickable.timed

                anchors.left: parent.left
                anchors.right: parent.right
                // (the last line of timed lyrics can be centred in the view;
                // the empty line between two verses of a page: half a line)
                height: (lyricsFlickable.timed && index === (lyricsRepeater.count - 1))
                    ? Math.max(implicitHeight, lyricsFlickable.height / 2)
                    : (!lyricsFlickable.timed && text.length === 0) ? Math.round(implicitHeight / 2)
                                                                    : implicitHeight
                padding: VLCStyle.margin_small

                text: modelData.text

                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap

                // Use a single pixel size and rely on `scale` to differentiate
                // the current line; this is cheaper than re-rasterizing the
                // font on every animation frame.
                font.pixelSize: VLCStyle.fontSize_xlarge
                font.weight: isCurrent ? Font.Bold : Font.Normal

                color: lyricsFlickable.colorContext.fg.primary
                opacity: emphasized ? 1.0 : 0.45
                scale: isCurrent ? 1.0 : (VLCStyle.fontSize_large / VLCStyle.fontSize_xlarge)

                Behavior on scale {
                    // Native text rendering does not handle scaled glyphs
                    // gracefully, so skip the animation in that case.
                    enabled: lyricsFlickable.animationsEnabled
                             && lyricDelegate.renderType !== Text.NativeRendering
                    NumberAnimation { duration: VLCStyle.duration_long }
                }
                Behavior on opacity {
                    enabled: lyricsFlickable.animationsEnabled
                    // NOTE: Do not use `OpacityAnimator` here, interrupting the fade stalls the UI (QTBUG-151700).
                    NumberAnimation { duration: VLCStyle.duration_long }
                }

                TapHandler {
                    // (a line without a time has nowhere to seek to)
                    enabled: lyricsFlickable.timed
                    onTapped: {
                        lyricsFlickable.playerPositionChangeRequested(lyricDelegate.modelData.time)
                    }
                }
            }
        }
    }

    Widgets.FadingEdge {
        id: fadingEdge

        parent: lyricsFlickable

        anchors.fill: parent

        backgroundColor: "transparent"
        // (its own rule, a layer for a background that is not opaque: always
        // here, so only when the view's size is settled)
        useLayering: lyricsFlickable.fadingEdgeLayering && effectCompatible

        sourceItem: lyricsFlickable.contentItem

        sourceX: lyricsFlickable.contentX
        sourceY: lyricsFlickable.contentY

        orientation: Qt.Vertical

        // Scale the fade with the lyrics' default (non-current) line size,
        // similar to how list views derive it from delegate height.
        fadeSize: VLCStyle.fontSize_large / 2

        // `fadingEdgeEnabled` is the master toggle; the per-edge disabler
        // Bindings below override these when the view is scrolled to the
        // corresponding edge.
        enableBeginningFade: lyricsFlickable.fadingEdgeEnabled
        enableEndFade: lyricsFlickable.fadingEdgeEnabled

        Binding on enableBeginningFade {
            when: lyricsFlickable.atYBeginning
            value: false
        }

        Binding on enableEndFade {
            when: lyricsFlickable.atYEnd
            value: false
        }
    }
}