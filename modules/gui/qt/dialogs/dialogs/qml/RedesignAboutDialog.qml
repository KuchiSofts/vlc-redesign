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
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Style
import VLC.Widgets as Widgets

// About VLC Redesign, drawn like the redesign's dialogs (the card of "Open
// network stream": 14 px corners, #191919, Inter): the version and updates,
// the design credit (CC BY 4.0) with links to Juxtopposed, the people VLC
// and this build come from, and the full classic lists one click away.
T.Popup {
    id: root

    readonly property bool darkTheme: VLCStyle.palette.isDark
    readonly property color textPrimary: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#1d1d1d", darkTheme)
    readonly property color textSecondary: VLCStyle.redesign.color(VLCStyle.redesign.textSecondary, "#3d3d3d", darkTheme)
    readonly property color textMuted: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#4d4d4d", darkTheme)

    parent: T.Overlay.overlay
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: parent ? Math.min(parent.width - 2 * VLCStyle.dp(24, VLCStyle.scale), VLCStyle.dp(600, VLCStyle.scale))
                  : VLCStyle.dp(600, VLCStyle.scale)
    height: parent ? Math.min(parent.height - 2 * VLCStyle.dp(24, VLCStyle.scale), implicitHeight) : implicitHeight
    implicitHeight: content.implicitHeight + topPadding + bottomPadding
    padding: VLCStyle.dp(28, VLCStyle.scale)
    modal: true
    focus: true
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside

    // the update state is shown here: no update banner meanwhile
    property bool _holdsBanner: false
    function _releaseBanner() {
        if (!_holdsBanner)
            return
        _holdsBanner = false
        RedesignUpdater.aboutOpen = false
    }
    onOpened: {
        _holdsBanner = true
        RedesignUpdater.aboutOpen = true
    }
    onClosed: _releaseBanner()
    Component.onDestruction: _releaseBanner()

    T.Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.5) }

    background: Rectangle {
        radius: VLCStyle.dp(14, VLCStyle.scale)
        color: VLCStyle.redesign.color("#191919", "#ABABAA", root.darkTheme)
        border.width: 1
        border.color: VLCStyle.redesign.color(VLCStyle.redesign.border, "#8f8f8e", root.darkTheme)
    }

    // A link of the about box: the accent colour, underlined on hover
    component Link: Text {
        id: link
        property string url
        color: VLCStyle.redesign.accent
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
        font.weight: Font.Medium
        font.underline: linkArea.containsMouse
        Accessible.role: Accessible.Link
        Accessible.name: text
        MouseArea {
            id: linkArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: Qt.openUrlExternally(link.url)
        }
    }

    component Body: Text {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        color: root.textSecondary
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
        lineHeight: 1.3
    }

    // A low window: the content scrolls (laid out as when it fits)
    contentItem: Flickable {
        contentWidth: width
        contentHeight: content.implicitHeight
        clip: contentHeight > height
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds

        T.ScrollBar.vertical: Widgets.ScrollBarExt { }

        ColumnLayout {
            id: content
            width: parent.width
            spacing: VLCStyle.dp(18, VLCStyle.scale)

            // The app, its version and its updates
            RowLayout {
                Layout.fillWidth: true
                spacing: VLCStyle.dp(18, VLCStyle.scale)

                Image {
                    Layout.preferredWidth: VLCStyle.dp(64, VLCStyle.scale)
                    Layout.preferredHeight: VLCStyle.dp(64, VLCStyle.scale)
                    source: "qrc:///redesign/welcome.svg"
                    sourceSize: Qt.size(width, height)
                    fillMode: Image.PreserveAspectFit
                    Accessible.ignored: true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: VLCStyle.dp(2, VLCStyle.scale)

                    Text {
                        text: qsTr("VLC Redesign")
                        color: root.textPrimary
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(28, VLCStyle.scale)
                        font.weight: Font.Light
                    }
                    Text {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        text: qsTr("Version %1 · based on VLC %2").arg(RedesignUpdater.currentVersion).arg(AboutModel.version.split(" ")[0])
                        color: root.textMuted
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
                    }
                    Text {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        visible: text.length > 0
                        color: RedesignUpdater.state === RedesignUpdater.Failed ? "#e15033" : VLCStyle.redesign.accent
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
                        text: {
                            switch (RedesignUpdater.state) {
                            case RedesignUpdater.Checking: return qsTr("Checking for updates…")
                            case RedesignUpdater.UpToDate: return qsTr("You have the latest version")
                            case RedesignUpdater.Available: return qsTr("Version %1 is available").arg(RedesignUpdater.latestVersion)
                            case RedesignUpdater.Downloading:
                                return qsTr("Downloading the update… %1%").arg(Math.round(RedesignUpdater.progress * 100))
                            case RedesignUpdater.Preparing: return qsTr("Preparing the update…")
                            case RedesignUpdater.Ready: return qsTr("Version %1 is ready: restart to finish").arg(RedesignUpdater.latestVersion)
                            case RedesignUpdater.Failed: return RedesignUpdater.errorText
                            default: return ""
                            }
                        }
                    }
                }

                Widgets.RedesignButton {
                    Layout.alignment: Qt.AlignTop
                    accent: RedesignUpdater.state === RedesignUpdater.Available || RedesignUpdater.state === RedesignUpdater.Ready
                    text: RedesignUpdater.state === RedesignUpdater.Ready ? qsTr("Restart now")
                          : RedesignUpdater.state === RedesignUpdater.Available
                            ? (RedesignUpdater.canInstall ? qsTr("Update now") : qsTr("Download"))
                            : qsTr("Check for updates")
                    enabled: RedesignUpdater.state !== RedesignUpdater.Checking
                             && RedesignUpdater.state !== RedesignUpdater.Downloading
                             && RedesignUpdater.state !== RedesignUpdater.Preparing
                    onClicked: {
                        if (RedesignUpdater.state === RedesignUpdater.Ready)
                            RedesignUpdater.restartNow()
                        else if (RedesignUpdater.state !== RedesignUpdater.Available)
                            RedesignUpdater.check(true)
                        else if (RedesignUpdater.canInstall)
                            RedesignUpdater.install()
                        else
                            RedesignUpdater.openReleasePage()
                    }
                }
            }

            // The design credit (CC BY 4.0), first and in full
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: designColumn.implicitHeight + 2 * VLCStyle.dp(18, VLCStyle.scale)
                radius: VLCStyle.dp(10, VLCStyle.scale)
                color: VLCStyle.redesign.color("#0E0E0E", "#9c9c9b", root.darkTheme)

                ColumnLayout {
                    id: designColumn
                    x: VLCStyle.dp(18, VLCStyle.scale)
                    y: VLCStyle.dp(18, VLCStyle.scale)
                    width: parent.width - 2 * x
                    spacing: VLCStyle.dp(8, VLCStyle.scale)

                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        textFormat: Text.StyledText
                        text: qsTr("Interface designed by <font color=\"%1\">Juxtopposed</font>").arg(VLCStyle.redesign.accent)
                        color: root.textPrimary
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                        font.weight: Font.Medium
                    }
                    Body {
                        text: qsTr("Every screen comes from the \"VLC Redesign (Community)\" Figma file, licensed under CC BY 4.0. It was adapted to Qt and its icons were redrawn.")
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: VLCStyle.dp(18, VLCStyle.scale)
                        Link { text: qsTr("YouTube channel"); url: "https://www.youtube.com/@juxtopposed" }
                        Link { text: qsTr("The redesign video"); url: "https://www.youtube.com/watch?v=ERFeOgqk324" }
                        Link { text: qsTr("Figma file"); url: "https://www.figma.com/community/file/1691465999134370980" }
                        Link { text: qsTr("Website"); url: "https://juxtopposed.com" }
                        Link { text: qsTr("CC BY 4.0"); url: "https://creativecommons.org/licenses/by/4.0/" }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: VLCStyle.dp(6, VLCStyle.scale)

                RowLayout {
                    spacing: VLCStyle.dp(6, VLCStyle.scale)
                    Body { Layout.fillWidth: false; text: qsTr("First implementation by") }
                    Link { text: "MrDouglax"; url: "https://github.com/SrDouglax/JuxtopposedVLC" }
                }
                RowLayout {
                    spacing: VLCStyle.dp(6, VLCStyle.scale)
                    Body { Layout.fillWidth: false; text: qsTr("Built on VLC media player by") }
                    Link { text: qsTr("the VideoLAN team and contributors"); url: "https://www.videolan.org/" }
                }
                Body {
                    Layout.topMargin: VLCStyle.dp(6, VLCStyle.scale)
                    color: root.textMuted
                    font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
                    text: qsTr("An unofficial build, not affiliated with or endorsed by VideoLAN or Juxtopposed. VLC, VLC media player and the cone logo are trademarks of VideoLAN. Free software under the GNU GPL v2 or later.")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: VLCStyle.dp(4, VLCStyle.scale)
                spacing: VLCStyle.dp(8, VLCStyle.scale)

                Widgets.RedesignButton {
                    text: qsTr("Licenses & authors")
                    onClicked: { root.close(); DialogsProvider.nativeAboutDialog() }
                }
                Widgets.RedesignButton {
                    text: qsTr("GitHub")
                    onClicked: Qt.openUrlExternally(RedesignUpdater.repositoryUrl)
                }
                Item { Layout.fillWidth: true }
                Widgets.RedesignButton {
                    accent: true
                    text: qsTr("Close")
                    onClicked: root.close()
                }
            }
        }
    }

    // Keeps the title bar of the window draggable while the dialog is open
    Widgets.CSDModalDragStrip { dialog: root }
}
