/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import VLC.Style

T.Control {
    id: root
    property bool paintOnly: false
    property url iconSource: ""
    property string value: ""
    property string placeholder: ""
    readonly property ColorContext colorContext: ColorContext {
        palette: VLCStyle.palette
        colorSet: ColorContext.ToolButton
    }
    // Figma 2160:8732: as the folder of the top bar, 28 high, the icon (24)
    // at 2, the text at 32, Inter 14 #8F8F8F, 4 after it
    readonly property bool _hasIcon: iconSource.toString().length > 0
    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    implicitHeight: VLCStyle.dp(28, VLCStyle.scale)
    leftPadding: _hasIcon ? VLCStyle.dp(2, VLCStyle.scale) : 0
    rightPadding: _hasIcon ? VLCStyle.dp(4, VLCStyle.scale) : 0
    Accessible.role: Accessible.Indicator
    Accessible.name: value.length ? value : placeholder
    contentItem: Row {
        spacing: VLCStyle.dp(6, VLCStyle.scale)
        Image {
            visible: root._hasIcon
            source: root.iconSource
            width: visible ? VLCStyle.redesign.iconSize : 0
            height: width
            sourceSize: Qt.size(width, height)
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            id: valueText
            text: root.value.length ? root.value : root.placeholder
            width: Math.min(implicitWidth, VLCStyle.dp(240, VLCStyle.scale))
            height: root.availableHeight
            elide: Text.ElideRight
            color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            // the text box of the design centred, on a whole pixel
            verticalAlignment: Text.AlignTop
            topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                       font.pixelSize) - valueMetrics.ascent
            FontMetrics {
                id: valueMetrics
                font: valueText.font
            }
        }
    }
}
