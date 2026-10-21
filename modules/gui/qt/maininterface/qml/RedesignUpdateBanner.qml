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

// A new release of VLC Redesign on GitHub, as a card in the corner of the
// window (the redesign's card colours, 10 px corners): "Update now" for an
// installed copy, "Download" (the release page) for a portable one. After
// "Check for updates" it also says up to date, or why the check failed.
Rectangle {
    id: root

    readonly property bool darkTheme: VLCStyle.palette.isDark
    readonly property int state: RedesignUpdater.state
    readonly property bool busy: state === RedesignUpdater.Downloading || state === RedesignUpdater.Preparing

    function megabytes(bytes) {
        return bytes >= 10 * 1048576 ? qsTr("%1 MB").arg(Math.round(bytes / 1048576))
                                     : qsTr("%1 MB").arg((bytes / 1048576).toFixed(1))
    }

    visible: RedesignUpdater.showBanner
    width: Math.min(parent.width - 2 * VLCStyle.dp(16, VLCStyle.scale), VLCStyle.dp(420, VLCStyle.scale))
    height: column.implicitHeight + 2 * VLCStyle.dp(16, VLCStyle.scale)
    radius: VLCStyle.dp(10, VLCStyle.scale)
    color: VLCStyle.redesign.color("#191919", "#ABABAA", darkTheme)
    border.width: 1
    border.color: VLCStyle.redesign.color(VLCStyle.redesign.border, "#8f8f8e", darkTheme)

    // the transient results of a manual check go away by themselves
    Timer {
        running: root.visible && root.state === RedesignUpdater.UpToDate
        interval: 4000
        onTriggered: RedesignUpdater.dismiss()
    }

    ColumnLayout {
        id: column
        x: VLCStyle.dp(16, VLCStyle.scale)
        y: VLCStyle.dp(16, VLCStyle.scale)
        width: parent.width - 2 * x
        spacing: VLCStyle.dp(6, VLCStyle.scale)

        RowLayout {
            Layout.fillWidth: true
            spacing: VLCStyle.dp(10, VLCStyle.scale)

            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                width: VLCStyle.dp(8, VLCStyle.scale)
                height: width
                radius: width / 2
                color: root.state === RedesignUpdater.Failed ? "#e15033" : VLCStyle.redesign.accent
            }

            Text {
                Layout.fillWidth: true
                elide: Text.ElideRight
                color: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#1d1d1d", root.darkTheme)
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(15, VLCStyle.scale)
                font.weight: Font.Medium
                text: {
                    switch (root.state) {
                    case RedesignUpdater.Checking: return qsTr("Checking for updates…")
                    case RedesignUpdater.UpToDate: return qsTr("VLC Redesign %1 is up to date").arg(RedesignUpdater.currentVersion)
                    case RedesignUpdater.Available: return qsTr("VLC Redesign %1 is available").arg(RedesignUpdater.latestVersion)
                    case RedesignUpdater.Downloading:
                        return qsTr("Downloading VLC Redesign %1… %2%").arg(RedesignUpdater.latestVersion)
                                                                    .arg(Math.round(RedesignUpdater.progress * 100))
                    case RedesignUpdater.Preparing: return qsTr("Preparing VLC Redesign %1…").arg(RedesignUpdater.latestVersion)
                    case RedesignUpdater.Ready: return qsTr("VLC Redesign %1 is ready").arg(RedesignUpdater.latestVersion)
                    case RedesignUpdater.Failed: return qsTr("Couldn't update")
                    default: return ""
                    }
                }
            }

            // close: a × in the muted text colour, a round hover (it stops
            // a download going on)
            T.Button {
                id: closeButton
                visible: root.state !== RedesignUpdater.Preparing
                implicitWidth: VLCStyle.dp(24, VLCStyle.scale)
                implicitHeight: VLCStyle.dp(24, VLCStyle.scale)
                Accessible.name: root.state === RedesignUpdater.Downloading ? qsTr("Cancel") : qsTr("Close")
                onClicked: RedesignUpdater.dismiss()
                background: Rectangle {
                    radius: width / 2
                    color: closeButton.hovered ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b", root.darkTheme)
                                               : "transparent"
                }
                contentItem: Text {
                    text: "×"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#4d4d4d", root.darkTheme)
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(18, VLCStyle.scale)
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: text.length > 0
            wrapMode: Text.WordWrap
            maximumLineCount: 5
            elide: Text.ElideRight
            color: VLCStyle.redesign.color(VLCStyle.redesign.textSecondary, "#3d3d3d", root.darkTheme)
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
            lineHeight: 1.25
            text: {
                switch (root.state) {
                case RedesignUpdater.Available:
                    return RedesignUpdater.summary.length > 0
                           ? RedesignUpdater.summary.split("\n").map(line => "• " + line).join("\n")
                           : qsTr("You have %1.").arg(RedesignUpdater.currentVersion)
                case RedesignUpdater.Downloading:
                case RedesignUpdater.Preparing:
                    return RedesignUpdater.smallUpdate
                           ? qsTr("Only what changed: keep watching, it's checked in the background.")
                           : qsTr("VLC Redesign will close, update and open again where you were.")
                case RedesignUpdater.Ready:
                    return qsTr("It takes a few seconds: restart now, or it's done when you close VLC Redesign.")
                case RedesignUpdater.Failed:
                    return RedesignUpdater.errorText
                default:
                    return ""
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: VLCStyle.dp(4, VLCStyle.scale)
            visible: root.busy
            implicitHeight: VLCStyle.dp(4, VLCStyle.scale)
            radius: height / 2
            color: VLCStyle.redesign.color("#0E0E0E", "#8f8f8e", root.darkTheme)

            Rectangle {
                height: parent.height
                radius: parent.radius
                width: root.state === RedesignUpdater.Preparing ? parent.width
                                                                : parent.width * RedesignUpdater.progress
                color: VLCStyle.redesign.accent
                Behavior on width { NumberAnimation { duration: VLCStyle.duration_short } }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: VLCStyle.dp(6, VLCStyle.scale)
            visible: root.state === RedesignUpdater.Ready
            spacing: VLCStyle.dp(8, VLCStyle.scale)

            Item { Layout.fillWidth: true }

            Widgets.RedesignButton {
                text: qsTr("Later")
                onClicked: RedesignUpdater.dismiss()
            }

            Widgets.RedesignButton {
                accent: true
                text: qsTr("Restart now")
                onClicked: RedesignUpdater.restartNow()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: VLCStyle.dp(6, VLCStyle.scale)
            visible: root.state === RedesignUpdater.Available || root.state === RedesignUpdater.Failed
            spacing: VLCStyle.dp(8, VLCStyle.scale)

            Widgets.RedesignButton {
                visible: root.state === RedesignUpdater.Available && !RedesignUpdater.manual
                text: qsTr("Skip")
                onClicked: RedesignUpdater.skipThisVersion()
            }

            Item { Layout.fillWidth: true }

            Widgets.RedesignButton {
                text: root.state === RedesignUpdater.Failed ? qsTr("Retry") : qsTr("What's new")
                onClicked: root.state === RedesignUpdater.Failed ? RedesignUpdater.check(true)
                                                                 : RedesignUpdater.openReleasePage()
            }

            Widgets.RedesignButton {
                accent: true
                text: root.state === RedesignUpdater.Failed ? qsTr("Release page")
                      : !RedesignUpdater.canInstall ? qsTr("Download")
                      : RedesignUpdater.updateSize > 0 ? qsTr("Update now · %1").arg(root.megabytes(RedesignUpdater.updateSize))
                      : qsTr("Update now")
                onClicked: root.state === RedesignUpdater.Failed || !RedesignUpdater.canInstall
                           ? RedesignUpdater.openReleasePage() : RedesignUpdater.install()
            }
        }
    }
}
