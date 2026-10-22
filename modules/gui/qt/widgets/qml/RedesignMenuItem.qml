/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.impl as Impl
import QtQuick.Templates as T

import VLC.Style
import VLC.Util

// Row of a RedesignMenu: icon, label, then the shortcut or a submenu arrow.
// A checkable row shows its check mark where the icon goes.
T.MenuItem {
    id: root

    // Name of an icon in pixmaps/redesign/icons
    property string iconName
    // The check mark of a choice kept by a model (the selected profile): the
    // row is not checkable, a click doesn't toggle it, the mark follows the
    // model only (a toggled check stayed on the rows chosen before)
    property bool marked: false
    // a row of such a choice: room for the mark, marked or not
    property bool markable: false

    Accessible.checkable: checkable || marked
    Accessible.checked: checkable ? checked : marked
    // Shortcut shown on the right, e.g. "Ctrl + O"
    property string shortcutText

    readonly property string _icon: iconName || (subMenu?.iconName ?? "")
    // The two-tone icons of pixmaps/redesign/duo (assets.qrc): the others go
    // straight to the icon of the redesign set (no failed load, no warning)
    readonly property var _duoIcons: [
        "add", "album", "audio", "backward-10", "capture-device", "close", "disc", "edit", "export", "faster",
        "favorite", "file", "folder", "folder-add", "folder-open", "forward-10", "go-to", "help", "hotkeys", "info-small", "info",
        "lists", "mark-watched", "next", "open", "paint", "pause", "play", "playback-settings", "playback",
        "playlist", "plugins", "previous", "profile", "queue", "quit", "recent", "record", "remove",
        "settings", "shuffle", "slower", "spectrum", "stop", "streaming", "subtitle", "subtitles", "swap",
        "sync", "tv", "ui", "video", "view", "volume-high", "volume-low", "volume-muted"
    ]
    readonly property bool _hasDuo: _icon !== "" && _duoIcons.indexOf(_icon) >= 0
    // Figma menus: labels 14 #8F8F8F (hovered too), shortcuts 14 #6B6B6B
    readonly property color _textColor: VLCStyle.redesign.color(
        enabled ? VLCStyle.redesign.textMuted : VLCStyle.redesign.textDisabled, "#2e2e2e")

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: VLCStyle.dp(48, VLCStyle.scale)

    leftPadding: VLCStyle.dp(14, VLCStyle.scale)
    rightPadding: VLCStyle.dp(14, VLCStyle.scale)
    spacing: VLCStyle.dp(10, VLCStyle.scale)

    contentItem: RowLayout {
        spacing: root.spacing

        // Two-tone icon of the design's menus (as RedesignMenuStyle), else
        // the icon of the redesign set in the accent color
        Image {
            id: duoIcon
            Layout.preferredWidth: VLCStyle.redesign.iconSize
            Layout.preferredHeight: VLCStyle.redesign.iconSize
            // Keeps its place while loading (async provider)
            visible: root._hasDuo && status !== Image.Error
            sourceSize: Qt.size(width, height)
            opacity: root.enabled ? 1 : 0.4
            source: !root._hasDuo ? ""
                    : SVGColorImage.colorize("qrc:///redesign/duo/" + root._icon + ".svg")
                                   .color1(VLCStyle.redesign.color("#8F8F8F", "#5A5A59"))
                                   // Figma: Accent Dim, the full accent where
                                   // the icon has it (#00FFFF: video)
                                   .color2(VLCStyle.redesign.accent)
                                   .accent(VLCStyle.redesign.accentDim).uri()
        }

        Impl.IconImage {
            Layout.preferredWidth: VLCStyle.redesign.iconSize
            Layout.preferredHeight: VLCStyle.redesign.iconSize
            visible: root._icon !== "" ? (!root._hasDuo || duoIcon.status === Image.Error)
                                       : (root.checkable || root.markable || root.marked)
            sourceSize: Qt.size(width, height)
            fillMode: Image.PreserveAspectFit
            // The check mark is hidden, not unloaded: an IconImage whose
            // source becomes empty goes on showing its last image, and the
            // rows checked before stayed checked
            readonly property bool _mark: root._icon === ""
            opacity: (_mark && !((root.checkable && root.checked) || root.marked)) ? 0
                                                                                  : (root.enabled ? 1 : 0.4)
            color: VLCStyle.redesign.accent
            // (not loaded behind a two-tone icon: some have no PNG)
            source: root._icon !== ""
                    ? (root._hasDuo && duoIcon.status !== Image.Error
                       ? "" : "qrc:///redesign/icons/" + root._icon + "/default.png")
                    : "qrc:///redesign/track-check-active.png"
        }

        Text {
            id: label
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: root.text
            elide: Text.ElideRight
            color: root._textColor
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            // the text box of the design (17 px) centred, on a whole pixel
            // (as RedesignOptionItem)
            verticalAlignment: Text.AlignTop
            topPadding: Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2)
                        + (Math.round(1.2109375 * font.pixelSize) - 1.2109375 * font.pixelSize) / 2
                        + 0.96875 * font.pixelSize - labelMetrics.ascent
            FontMetrics {
                id: labelMetrics
                font: label.font
            }
        }

        Text {
            Layout.fillHeight: true
            visible: root.shortcutText !== "" && !root.subMenu
            text: root.shortcutText
            color: VLCStyle.redesign.color(enabled ? "#6B6B6B" : VLCStyle.redesign.textDisabled, "#4d4d4d")
            font: label.font
            // the item is the text width rounded up: the text ends at the
            // row's padding as in the design (not up to 1 px before it)
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignTop
            topPadding: label.topPadding
        }

        Image {
            Layout.preferredWidth: VLCStyle.redesign.iconSize
            Layout.preferredHeight: VLCStyle.redesign.iconSize
            visible: !!root.subMenu
            sourceSize: Qt.size(width, height)
            source: "qrc:///redesign/icons/chevron-right/default.png"
        }
    }

    background: Rectangle {
        implicitWidth: VLCStyle.dp(284, VLCStyle.scale)
        radius: VLCStyle.redesign.controlRadius
        color: root.highlighted
               ? VLCStyle.redesign.color("#1F1F1F", "#9c9c9b")
               : "transparent"
    }
}
