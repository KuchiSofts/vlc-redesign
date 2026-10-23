/*****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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
import QtQuick.Layouts
import QtQuick.Templates as T


import VLC.MainInterface
import VLC.Player
import VLC.Style
import VLC.Widgets as Widgets
import VLC.Util

// Offer to continue where the media was left. The redesign shows it next to
// the playback controls rather than in the top bar, where it went unnoticed
// for the few seconds it stays. Figma "Player - Music": the time saved, one
// button (Escape, or the seconds it stays, dismiss it).
FocusScope {
    id: resumePanel

    property bool darkTheme: VLCStyle.palette.isDark

    implicitWidth: card.implicitWidth
    implicitHeight: card.implicitHeight

    visible: false
    enabled: visible

    Accessible.role: Accessible.Dialog
    Accessible.name: qsTr("Resume dialog")

    signal hidden()

    function showResumePanel ()
    {
        resumePanel.visible = true
        continueBtn.forceActiveFocus()
        resumeTimeout.start()
    }

    function hideResumePanel() {
        resumeTimeout.stop()
        resumePanel.visible = false
        Player.acknowledgeRestoreCallback()
        hidden()
    }

    Timer {
        id: resumeTimeout
        interval: 10000
        onTriggered: {
            resumePanel.visible = false
            hidden()
        }
    }

    Connections {
        target: Player
        function onCanRestorePlaybackChanged() {
            if (Player.canRestorePlayback) {
                showResumePanel()
            } else {
                hideResumePanel()
            }
        }
    }

    Component.onCompleted: {
        if (Player.canRestorePlayback) {
            showResumePanel()
        }
    }

    Navigation.cancelAction: function() {
        hideResumePanel()
    }

    // Menu surface of the design: #191919 at 95%, 10px corners
    Rectangle {
        id: card

        anchors.fill: parent

        implicitWidth: row.implicitWidth + 2 * VLCStyle.dp(8, VLCStyle.scale)
        implicitHeight: row.implicitHeight + 2 * VLCStyle.dp(8, VLCStyle.scale)

        radius: VLCStyle.dp(10, VLCStyle.scale)
        color: resumePanel.darkTheme ? "#F2191919" : "#F2ABABAA"
        border.width: 1
        border.color: VLCStyle.redesign.color(VLCStyle.redesign.separator, "#9c9c9b", resumePanel.darkTheme)

        RowLayout {
            id: row

            anchors.centerIn: parent
            spacing: VLCStyle.dp(8, VLCStyle.scale)

            Image {
                Layout.leftMargin: VLCStyle.dp(6, VLCStyle.scale)
                Layout.preferredWidth: VLCStyle.redesign.iconSize
                Layout.preferredHeight: VLCStyle.redesign.iconSize
                source: "qrc:///redesign/icons/replay/default.png"
                sourceSize: Qt.size(width, height)
            }

            T.Label {
                id: label

                Layout.rightMargin: VLCStyle.dp(8, VLCStyle.scale)

                // the design writes the hours: 00:45:08
                text: Player.restoreTime.valid()
                      ? qsTr("You’ve watched till %1 already.")
                            .arg(Player.restoreTime.formatHMS(VLCTick.ForceHours))
                      : qsTr("Continue where you left off?")
                color: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#2e2e2e", resumePanel.darkTheme)
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.redesign.fontMedium
            }

            ResumeButton {
                id: continueBtn

                text: qsTr("Continue Watching")
                primary: true
                focus: true

                AccessibleCompat.id: "resumeContinueButton"

                onClicked: {
                    Player.restorePlaybackPos()
                    hideResumePanel()
                }

                Navigation.parentItem: resumePanel
                Keys.priority: Keys.AfterItem
                Keys.onPressed: (event) => continueBtn.Navigation.defaultKeyAction(event)
            }
        }
    }

    // Accent button like the play button, or a quiet one
    component ResumeButton: T.Button {
        id: button

        property bool primary: false

        implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
        implicitHeight: VLCStyle.redesign.playHeight
        leftPadding: VLCStyle.dp(14, VLCStyle.scale)
        rightPadding: leftPadding

        Accessible.name: text

        contentItem: Text {
            text: button.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: button.primary ? "#0E0E0E"
                                  : VLCStyle.redesign.color(VLCStyle.redesign.textSecondary, "#4d4d4d",
                                                            resumePanel.darkTheme)
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.redesign.fontNormal
            font.weight: button.primary ? Font.DemiBold : Font.Normal
        }

        background: Rectangle {
            radius: VLCStyle.redesign.controlRadius
            color: {
                if (button.primary)
                    return button.down ? VLCStyle.redesign.accentDim : VLCStyle.redesign.accent
                return (button.hovered || button.down)
                    ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b",
                                              resumePanel.darkTheme)
                    : "transparent"
            }
            border.width: button.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.focusColor
        }
    }
}
