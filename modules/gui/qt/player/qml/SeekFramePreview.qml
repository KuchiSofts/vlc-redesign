/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Window

import VLC.MainInterface
import VLC.Player
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// Frame of the playing media at a position of the seek bar, with its time
// (design setting "Show thumbnail while scrubbing"). Figma "thumbnail"
// (2151:9673): 272 x 189, the frame 252 wide 10 in, the time (14 px,
// #8F8F8F) 10 under it, 10 to the end.
Rectangle {
    id: root

    // Position of the frame, from 0 to 1
    property real position: 0
    property string timeText
    property bool darkTheme: true

    readonly property real padding: VLCStyle.dp(10, VLCStyle.scale)
    readonly property real frameWidth: VLCStyle.dp(252, VLCStyle.scale)
    // Height of the decoded frames, 16:9 until the first one arrives
    readonly property real frameHeight: frameSource.implicitWidth > 0
        ? Math.round(frameWidth * frameSource.implicitHeight / frameSource.implicitWidth)
        : Math.round(frameWidth * 9 / 16)
    readonly property real _eDPR: MainCtx.effectiveDevicePixelRatio(Window.window)
    // The rounded frame is drawn by a shader; Qt's software renderer has
    // none (no OpenGL), the decoded frame is then shown as it is
    readonly property bool _shaders: GraphicsInfo.shaderType === GraphicsInfo.RhiShader

    implicitWidth: frameWidth + 2 * padding
    // (the line of the time: 17 px, as in Figma 2151:9673, 189 high in all)
    implicitHeight: frameHeight + label.height + 3 * padding

    radius: VLCStyle.dp(12, VLCStyle.scale)
    // Menu surface of the design: #191919 at 95%
    color: root.darkTheme ? "#F2191919" : "#F2ABABAA"

    // Positions are rounded to a thousandth: close hovers share a request
    Image {
        id: frameSource

        visible: !root._shaders
        x: root.padding
        y: root.padding
        width: root.frameWidth
        height: root.frameHeight
        fillMode: Image.PreserveAspectCrop
        clip: true
        asynchronous: true
        // The previous frame stays while the next one decodes
        retainWhileLoading: true
        sourceSize: Qt.size(root.frameWidth * root._eDPR, 0)
        source: root.visible && Player.url.toString() !== ""
                ? VLCThumbnail.preview(Player.url.toString(), root.position) : ""
    }

    Widgets.ImageExt {
        visible: root._shaders
        x: root.padding
        y: root.padding
        width: root.frameWidth
        height: root.frameHeight
        radius: VLCStyle.dp(3, VLCStyle.scale)
        backgroundColor: "#0E0E0E"
        fillMode: Image.PreserveAspectCrop
        textureProviderItem: frameSource
    }

    Text {
        id: label

        anchors.horizontalCenter: parent.horizontalCenter
        y: root.padding * 2 + root.frameHeight
        height: VLCStyle.dp(17, VLCStyle.scale)
        verticalAlignment: Text.AlignVCenter
        text: root.timeText
        color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e", root.darkTheme)
        font.family: VLCStyle.redesign.fontFamily
        // (Figma 2151:9673: the bar's time style, 15 px)
        font.pixelSize: VLCStyle.dp(15, VLCStyle.scale)
    }
}
