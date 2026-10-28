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

import VLC.MainInterface
import VLC.Dialogs
import VLC.Player
import VLC.Style
import VLC.Widgets as Widgets

// Media Statistics (the Tools menu): what the Statistics tab of Media
// Information shows, live, with the input bitrate of the last two minutes
Widgets.RedesignDialog {
    id: root

    objectName: "statisticsDialog"

    title: qsTr("Media Statistics")
    acceptText: qsTr("Close")
    cancelVisible: false
    preferredWidth: VLCStyle.dp(713, VLCStyle.scale)
    // without the statistics collected: the option turned on
    extraText: stats.collecting ? "" : qsTr("Collect statistics")
    onExtraClicked: stats.startCollecting()

    MediaStatistics {
        id: stats
        ctx: MainCtx
    }

    readonly property var v: stats.values
    readonly property bool hasValues: Object.keys(v).length > 0

    function _number(key) {
        const value = v[key]
        return value === undefined ? "—" : Number(value).toLocaleString(Qt.locale(), "f", 0)
    }
    function _kilobytes(key) {
        const value = v[key]
        return value === undefined ? "—" : qsTr("%1 KiB").arg(Number(value).toLocaleString(Qt.locale(), "f", 0))
    }
    function _bitrate(key) {
        const value = v[key]
        return value === undefined ? "—" : qsTr("%1 kb/s").arg(Number(value).toLocaleString(Qt.locale(), "f", 0))
    }

    Widgets.RedesignDialogHeading {
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        headingWraps: true
        text: Player.isStarted && Player.name !== "" ? Player.name : qsTr("Nothing is playing")
        description: !stats.collecting
                     ? qsTr("Statistics are not collected. Turn them on to see them for the next media played.")
                     : !root.hasValues && Player.isStarted ? qsTr("Waiting for the first values…")
                     : ""
    }

    // the input bitrate: a well of "Dark/BG/B2", the line in the accent
    Rectangle {
        id: graph

        Layout.fillWidth: true
        implicitHeight: VLCStyle.dp(96, VLCStyle.scale)
        radius: VLCStyle.dp(6, VLCStyle.scale)
        color: VLCStyle.redesign.color("#090909", "#dedede")
        clip: true
        visible: stats.collecting

        Accessible.role: Accessible.Chart
        Accessible.name: qsTr("Input bitrate")

        readonly property var points: stats.bitrates
        readonly property real top: Math.max(1, stats.bitrateMax * 1.15)
        readonly property real inset: VLCStyle.dp(8, VLCStyle.scale)

        // Bars of Rectangles: VLC's Qt Quick is built without Canvas (and
        // QtQuick.Shapes is not in every build), as the waveform of the music
        Item {
            id: plot

            anchors.fill: parent
            anchors.margins: graph.inset
            visible: graph.points.length > 1
            // two minutes across, the newest at the end (on the left, right
            // to left: the layout mirrors the bars)
            readonly property real step: width / 120

            Repeater {
                model: graph.points.length

                Rectangle {
                    required property int index
                    readonly property real value: graph.points[index] ?? 0

                    x: plot.width - (graph.points.length - index) * plot.step
                    width: Math.max(1, plot.step - VLCStyle.dp(1, VLCStyle.scale))
                    height: Math.max(VLCStyle.dp(1, VLCStyle.scale), plot.height * value / graph.top)
                    y: plot.height - height
                    radius: VLCStyle.dp(1, VLCStyle.scale)
                    color: VLCStyle.redesign.accent
                    opacity: 0.85
                }
            }
        }

        // the highest value of the period, top left
        Text {
            x: VLCStyle.dp(10, VLCStyle.scale)
            y: VLCStyle.dp(6, VLCStyle.scale)
            visible: stats.bitrateMax > 0
            text: qsTr("%1 kb/s").arg(Number(stats.bitrateMax).toLocaleString(Qt.locale(), "f", 0))
            color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
        }
    }

    // the three groups side by side (Input, Video, Audio)
    RowLayout {
        Layout.fillWidth: true
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        spacing: VLCStyle.dp(20, VLCStyle.scale)
        visible: stats.collecting

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            spacing: 0

            Widgets.RedesignSectionTitle { text: qsTr("Input") }
            Widgets.RedesignInfoRow { label: qsTr("Read"); value: root._kilobytes("read") }
            Widgets.RedesignInfoRow { label: qsTr("Input bitrate"); value: root._bitrate("inputBitrate") }
            Widgets.RedesignInfoRow { label: qsTr("Demuxed"); value: root._kilobytes("demuxed") }
            Widgets.RedesignInfoRow { label: qsTr("Stream bitrate"); value: root._bitrate("streamBitrate") }
            Widgets.RedesignInfoRow { label: qsTr("Corrupted"); value: root._number("corrupted") }
            Widgets.RedesignInfoRow { label: qsTr("Discontinuities"); value: root._number("discontinuities"); divider: false }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            spacing: 0

            Widgets.RedesignSectionTitle { text: qsTr("Video") }
            Widgets.RedesignInfoRow { label: qsTr("Decoded"); value: root._number("videoDecoded") }
            Widgets.RedesignInfoRow { label: qsTr("Displayed"); value: root._number("videoDisplayed") }
            Widgets.RedesignInfoRow { label: qsTr("Late"); value: root._number("videoLate") }
            Widgets.RedesignInfoRow { label: qsTr("Lost"); value: root._number("videoLost"); divider: false }

            Widgets.RedesignSectionTitle {
                Layout.topMargin: VLCStyle.dp(16, VLCStyle.scale)
                text: qsTr("Audio")
            }
            Widgets.RedesignInfoRow { label: qsTr("Decoded"); value: root._number("audioDecoded") }
            Widgets.RedesignInfoRow { label: qsTr("Played"); value: root._number("audioPlayed") }
            Widgets.RedesignInfoRow { label: qsTr("Lost"); value: root._number("audioLost"); divider: false }
        }
    }
}
