/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.impl as Impl
import VLC.Style

Item {
    id: root
    property string category: "general"
    property var availableCategories: []
    readonly property bool dark: VLCStyle.palette.isDark
    signal selected(string category)

    readonly property var entries: [
        { kind: "heading", label: qsTr("App") },
        { key: "general", label: qsTr("General"), icon: "preferences" },
        { key: "interface", label: qsTr("Interface"), icon: "interface" },
        { key: "privacy", label: qsTr("Privacy"), icon: "privacy" },
        { key: "controls", label: qsTr("Controls"), icon: "hotkeys" },
        { kind: "heading", label: qsTr("Media") },
        { key: "playback", label: qsTr("Playback"), icon: "playback" },
        { key: "video", label: qsTr("Video"), icon: "video" },
        { key: "audio", label: qsTr("Audio"), icon: "audio" },
        { key: "subtitles", label: qsTr("Subtitles"), icon: "subtitles" },
        { key: "library", label: qsTr("Library"), icon: "folder" },
        { key: "tv", label: qsTr("TV"), icon: "tv" },
        { kind: "heading", label: qsTr("System") },
        { key: "network", label: qsTr("Network"), icon: "streaming" },
        { key: "performance", label: qsTr("Performance"), icon: "preferences" },
        { key: "recording", label: qsTr("Recording"), icon: "record" },
        { key: "extensions", label: qsTr("Extensions"), icon: "plugins" }
    ]

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value) }

    // Sidebar of the design: 200 px, #191919 at 95 %, radius 6, 6 px padding,
    // 8 px between the 33 px group titles and the 40 px items
    Rectangle {
        anchors.fill: parent
        radius: root.px(6)
        color: root.c("#191919")
    }

    Flickable {
        id: categoryFlick
        anchors.fill: parent
        anchors.margins: root.px(6)
        contentHeight: categoryList.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        // a low window cuts the list: the bar shows there is more
        ScrollBar.vertical: ScrollBar {
            policy: categoryFlick.contentHeight > categoryFlick.height ? ScrollBar.AlwaysOn
                                                                       : ScrollBar.AlwaysOff
            // Basic's handle is light grey: darker in the light theme
            palette.mid: VLCStyle.palette.isDark ? undefined : "#6b6b6b"
            palette.dark: VLCStyle.palette.isDark ? undefined : "#2e2e2e"
        }

        Column {
            id: categoryList
            width: parent.width
            spacing: root.px(8)

            Repeater {
                model: root.entries
                delegate: Item {
                    id: entry
                    required property var modelData
                    readonly property bool heading: modelData.kind === "heading"
                    readonly property bool current: root.category === modelData.key
                    width: categoryList.width
                    height: heading ? root.px(33) : root.px(40)
                    visible: heading || modelData.key === "extensions"
                             || root.availableCategories.indexOf(modelData.key) >= 0

                    Text {
                        visible: entry.heading
                        anchors.left: parent.left
                        anchors.leftMargin: root.px(10)
                        anchors.right: parent.right
                        anchors.rightMargin: root.px(10)
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        anchors.baseline: parent.verticalCenter
                        // centred in the 33 of the heading (odd)
                        anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize, root.px(33))
                        text: modelData.label || ""
                        color: root.c("#6B6B6B")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(14)
                    }

                    Button {
                        id: item
                        visible: !entry.heading
                        enabled: modelData.key === "extensions"
                                 || root.availableCategories.indexOf(modelData.key) >= 0
                        anchors.fill: parent
                        padding: root.px(4)
                        // the Basic style adds 2 px to the sides otherwise
                        horizontalPadding: root.px(4)
                        onClicked: root.selected(modelData.key)
                        Accessible.name: modelData.label || ""
                        // the whole name of a page cut short (a long translation)
                        ToolTip.visible: item.hovered && itemLabel.truncated
                        ToolTip.text: modelData.label || ""
                        ToolTip.delay: 650
                        background: Rectangle {
                            radius: root.px(2)
                            color: entry.current ? root.c("#222222")
                                   : item.hovered ? (root.dark ? "#1d1d1d" : root.c("#222222")) : "transparent"
                            border.width: item.visualFocus ? 1 : 0
                            border.color: VLCStyle.redesign.accent
                        }
                        // The Icon component of the design: 32 x 32 around the 24 px glyph
                        contentItem: Row {
                            spacing: root.px(8)
                            Item {
                                width: root.px(32)
                                height: root.px(32)
                                // (the active one in the accent colour)
                                Impl.ColorImage {
                                    anchors.centerIn: parent
                                    width: root.px(24)
                                    height: width
                                    sourceSize: Qt.size(width, height)
                                    smooth: !VLCStyle.redesign.svgExact(width, height, Screen.devicePixelRatio)
                                    fillMode: Image.PreserveAspectFit
                                    source: !modelData.icon ? "" : "qrc:///redesign/icons/" + modelData.icon + "/"
                                            + (entry.current ? "active" : "default") + ".svg"
                                    color: entry.current ? VLCStyle.redesign.accentTint : "transparent"
                                }
                            }
                            Text {
                                id: itemLabel
                                // what the item leaves after its icon: a longer
                                // name (a translation: "Конфиденциальность") is
                                // drawn up to 2 px smaller, and only then cut short
                                width: Math.max(0, Math.min(implicitWidth, item.availableWidth - root.px(32) - parent.spacing))
                                fontSizeMode: Text.HorizontalFit
                                minimumPixelSize: root.px(12)
                                elide: Text.ElideRight
                                anchors.baseline: parent.verticalCenter
                                anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
                                text: modelData.label || ""
                                color: entry.current ? root.c("#EAEAEA") : root.c("#8F8F8F")
                                font.family: VLCStyle.redesign.fontFamily
                                font.pixelSize: root.px(14)
                            }
                        }
                    }
                }
            }
        }
    }
}
