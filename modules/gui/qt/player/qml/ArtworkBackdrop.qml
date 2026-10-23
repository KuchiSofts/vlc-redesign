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

import VLC.MainInterface
import VLC.Player
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

Widgets.DualKawaseBlur {
    id: blurredBackground

    property Item coverItem
    readonly property real dar: parent && parent.height > 0 ? parent.width / parent.height : 1
    readonly property real sar: coverItem && coverItem.sar > 0 ? coverItem.sar : 1

    // Figma "Player - Music": the artwork 1952 x 1952 for 1920 x 1080 (the
    // width and 1.67 %), centred, layer blur 20, no tint
    mode: Widgets.DualKawaseBlur.Mode.FourPass
    radius: 2
    live: true
    anchors.centerIn: parent
    width: ((sar < dar) ? parent.width : parent.height * sar) * 1952 / 1920
    height: ((sar < dar) ? parent.width / sar : parent.height) * 1952 / 1920
    source: coverItem ? coverItem.sourceTextureProviderItem : null
    postprocess: true
    tint: bgtheme.palette.isDark ? "black" : "white"
    // the artwork as it is (Figma); the light placeholder of a media without
    // one stays dimmed
    readonly property bool _artwork: MainCtx.bgCone && !!Player.artwork && Player.artwork.toString() !== ""
                                     && !(coverItem && coverItem.status === Image.Error)
    tintStrength: bgtheme.palette.isDark ? (_artwork ? 0.0 : 0.82) : 0.6
    backgroundColor: bgtheme.bg.primary
    viewportRect: Qt.rect((width - parent.width) / 2,
                          (height - parent.height) / 2, parent.width, parent.height)

    // The software renderer cannot execute DualKawaseBlur's shader. Reuse the
    // loaded cover URL and blur a bounded thumbnail in the image provider.
    Image {
        anchors.fill: parent
        visible: blurredBackground.GraphicsInfo.api === GraphicsInfo.Software
        asynchronous: true
        cache: true
        fillMode: Image.PreserveAspectCrop
        sourceSize: Qt.size(320, 320)
        source: visible && blurredBackground.coverItem
                ? Effects.url(Effects.ArtworkBlur, {
                    "source": blurredBackground.coverItem.source,
                    "tint": bgtheme.palette.isDark ? "black" : "white",
                    "tintStrength": blurredBackground.tintStrength
                }) : ""
    }

    readonly property ColorContext colorContext: ColorContext {
        id: bgtheme
        colorSet: ColorContext.View
    }

    sourceTextureProviderObserver.onTextureChanged: liveTimer.transientTurnOnLive()

    Connections {
        target: blurredBackground.coverItem
        function onSourceChanged() { liveTimer.transientTurnOnLive() }
    }

    Component.onCompleted: {
        widthChanged.connect(liveTimer, liveTimer.transientTurnOnLive)
        heightChanged.connect(liveTimer, liveTimer.transientTurnOnLive)
        liveTimer.transientTurnOnLive()
    }

    Timer {
        id: liveTimer
        repeat: false
        interval: VLCStyle.duration_humanMoment

        function transientTurnOnLive() {
            if (!blurredBackground.sourceTextureIsValid)
                return
            blurredBackground.live = true
            liveTimer.restart()
        }
        onTriggered: blurredBackground.live = false
    }
}
