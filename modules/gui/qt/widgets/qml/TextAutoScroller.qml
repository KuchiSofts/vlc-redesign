/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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

import VLC.Style

Item {
    id: root

    readonly property alias scrolling: scrollAnimation.running

    // `label`: label to scroll, don't add horizontal anchors on it
    property Text label: undefined
    property bool forceScroll: false


    readonly property real requiredTextWidth: label.implicitWidth
    readonly property bool _needsToScroll: (label.width < requiredTextWidth)

    // right to left (Hebrew, Arabic...): the label at the right, its end
    // shown by scrolling it to the right
    readonly property bool _mirrored: LayoutMirroring.enabled
    readonly property real _restX: _mirrored ? root.width - label.width : 0

    //Accessible
    Accessible.role: Accessible.StaticText
    Accessible.name: label.text

    onLabelChanged: {
        label.width = Qt.binding(function () { return Math.min(label.implicitWidth, root.width) })

        label.x = Qt.binding(function () { return root._restX })

        label.elide = Qt.binding(function () {
            return root.scrolling ? Text.ElideNone : Text.ElideRight
        })

        if (label.mutabilityGroup !== undefined)
            label.mutabilityGroup = Qt.binding(function() { return (root.label === label && root.scrolling) ? Item.DynamicMutabilityGroup
                                                                                                            : Item.AutoMutabilityGroup })

        label.Accessible.ignored = true
    }

    SequentialAnimation {
        id: scrollAnimation

        running: root.forceScroll && root._needsToScroll
        loops: Animation.Infinite

        onStopped: {
            label.x = Qt.binding(function () { return root._restX })
        }

        PauseAnimation {
            duration: VLCStyle.duration_veryLong
        }

        SmoothedAnimation {
            target: label
            property: "x"
            from: root._restX
            to: root._mirrored ? root._restX + root.requiredTextWidth - label.width
                               : label.width - root.requiredTextWidth

            maximumEasingTime: 0
            velocity: 20
        }

        PauseAnimation {
            duration: VLCStyle.duration_veryLong
        }

        PropertyAction {
            target: label
            property: "x"
            value: root._restX
        }
    }
}

