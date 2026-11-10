/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick

import VLC.Style

// The sign that a view goes on under one of its edges: the colour of the
// surface it lies on fades in over the edge it continues under (a dialog's
// body in a low window: its last line was cut by the card's edge and nothing
// said that it scrolls). It stays over the view, not in what the view moves.
Item {
    id: root

    required property Flickable view
    // the colour under the view
    required property color surface
    property real fadeSize: VLCStyle.dp(28, VLCStyle.scale)

    readonly property bool more: view.contentHeight - view.height > 1
    readonly property bool above: more && view.contentY - view.originY > 1
    readonly property bool below: more && view.contentY - view.originY + view.height < view.contentHeight - 1
    readonly property color _clear: Qt.rgba(surface.r, surface.g, surface.b, 0)

    parent: view
    anchors.fill: parent
    z: 99

    Rectangle {
        width: parent.width
        height: root.fadeSize
        visible: opacity > 0
        opacity: root.above ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: VLCStyle.duration_short } }
        gradient: Gradient {
            GradientStop { position: 0; color: root.surface }
            GradientStop { position: 1; color: root._clear }
        }
    }

    Rectangle {
        width: parent.width
        height: root.fadeSize
        anchors.bottom: parent.bottom
        visible: opacity > 0
        opacity: root.below ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: VLCStyle.duration_short } }
        gradient: Gradient {
            GradientStop { position: 0; color: root._clear }
            GradientStop { position: 1; color: root.surface }
        }
    }
}
