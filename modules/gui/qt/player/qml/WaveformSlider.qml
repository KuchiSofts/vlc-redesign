/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import VLC.Player
import VLC.Style

SliderBar {
    id: root
    objectName: "waveformSlider"
    redesign: true
    barHeight: VLCStyle.redesign.seekHeight

    Waveform {
        id: analysis
        // the helper decodes the whole file: not for a hidden slider (the
        // mini player's under the player, the player's under the library)
        source: (root.visible && Player.videoTracks.count === 0) ? Player.url : ""
        durationSeconds: Player.length.toSeconds()
    }

    // Built from Rectangles: VLC's Qt Quick is compiled without Canvas.
    // Over the bar of SliderBar: the track of the design with the waveform
    // in it (no fill: the played bars show the progress) and the handle; the
    // pointer handlers below still get the events
    Rectangle {
        id: graph
        anchors.fill: parent
        z: 1
        visible: analysis.ready && peaks.length > 0
        radius: VLCStyle.redesign.controlRadius
        color: VLCStyle.redesign.color(VLCStyle.redesign.seekBackground, "#dedede")
        readonly property real played: Math.max(0, Math.min(1, Number(Player.position)))
        readonly property real inset: VLCStyle.dp(6, VLCStyle.scale)
        readonly property real pitch: VLCStyle.dp(5, VLCStyle.scale)
        readonly property real barWidth: VLCStyle.dp(2, VLCStyle.scale)
        readonly property real maxHeight: height - VLCStyle.dp(2, VLCStyle.scale)
        // Relative peak (0..1) of each bar, from the 512 analysis bins
        readonly property var peaks: {
            const bins = analysis.bins
            if (!analysis.ready || width <= 0 || bins.length !== 512)
                return []
            const columns = Math.max(1, Math.min(512, Math.floor((width - 2 * inset + pitch - barWidth) / pitch)))
            let largest = 0
            for (let i = 0; i < bins.length; ++i)
                largest = Math.max(largest, Number(bins[i]))
            const scale = Math.max(largest, 0.05)
            const result = []
            for (let column = 0; column < columns; ++column) {
                const first = Math.floor(column * bins.length / columns)
                const last = Math.max(first + 1, Math.floor((column + 1) * bins.length / columns))
                let peak = 0
                for (let bin = first; bin < last; ++bin)
                    peak = Math.max(peak, Number(bins[bin]))
                result.push(peak / scale)
            }
            return result
        }
        // A bar is played once its middle is. The bars are drawn twice, each
        // set clipped on its side of the gap after the last played bar: the
        // position moves the two clips, the bars stay as they are
        readonly property int playedCount: Math.max(0, Math.min(peaks.length,
            Math.floor((played * width - inset - barWidth / 2) / pitch) + 1))
        readonly property real playedEdge: playedCount > 0
                                           ? inset + playedCount * pitch - (pitch - barWidth) / 2
                                           : 0
        Item {
            width: graph.playedEdge
            height: graph.height
            clip: true
            Repeater {
                model: graph.peaks.length
                delegate: Rectangle {
                    required property int index
                    readonly property real peak: graph.peaks[index]
                    x: graph.inset + index * graph.pitch
                    width: graph.barWidth
                    height: Math.max(graph.barWidth, Math.round(peak * graph.maxHeight))
                    y: (graph.height - height) / 2
                    radius: width / 2
                    color: VLCStyle.redesign.accent
                    opacity: 0.8
                }
            }
        }
        Item {
            id: unplayed
            x: graph.playedEdge
            width: graph.width - x
            height: graph.height
            clip: true
            Item {
                x: -unplayed.x
                Repeater {
                    model: graph.peaks.length
                    delegate: Rectangle {
                        required property int index
                        readonly property real peak: graph.peaks[index]
                        x: graph.inset + index * graph.pitch
                        width: graph.barWidth
                        height: Math.max(graph.barWidth, Math.round(peak * graph.maxHeight))
                        y: (graph.height - height) / 2
                        radius: width / 2
                        color: VLCStyle.redesign.color("#404040", "#9c9c9b")
                    }
                }
            }
        }
        // the handle of the seek bar
        Rectangle {
            x: Math.max(0, Math.min(graph.width - width, graph.played * graph.width - width / 2))
            y: VLCStyle.redesign.seekInset
            width: VLCStyle.redesign.seekHandleWidth
            height: graph.height - 2 * VLCStyle.redesign.seekInset
            radius: VLCStyle.redesign.controlRadius
            color: VLCStyle.redesign.accent
        }
    }
}
