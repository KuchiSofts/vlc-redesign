/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Templates as T

import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// The subtitle languages: a default one, and a second one for when there
// are none in it (in the video, next to it, nor online), and whether
// subtitles are found online by themselves (SubtitlePreferences, saved when
// the setup finishes)
Item {
    id: root
    property string primary: "en"
    property string secondary: ""
    property bool autoSearch: true
    property bool showBoth: false
    signal primaryRequested(string code)
    signal secondaryRequested(string code)
    signal autoSearchRequested(bool on)
    signal showBothRequested(bool on)

    // the player behind, as on the other steps
    Rectangle {
        x: 17
        y: 19
        width: parent.width - 34
        height: 22
        color: "#191919"
    }
    Rectangle {
        x: 17
        y: 41
        width: parent.width - 34
        height: 542
        color: "#090909"
    }

    Text {
        id: title
        x: Math.floor((parent.width - width) / 2)
        y: 132
        text: qsTr("Choose the languages of your subtitles.")
        color: "#969696"
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 24
        font.weight: Font.Light
    }
    Text {
        x: Math.floor((parent.width - width) / 2)
        y: 184
        width: 700
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: qsTr("Subtitles in your language show by themselves: from the video, from a file next to it, or found online. When there are none in it, not even online, the second language is used.")
        color: "#6f6f6f"
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 15
        lineHeight: 1.3
    }

    Row {
        y: 262
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 40

        Column {
            spacing: 10
            Text {
                text: qsTr("Default language")
                color: "#8f8f8f"
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: 14
            }
            Widgets.LanguagePicker {
                objectName: "onboardingPrimaryLanguage"
                width: 320
                height: 46
                code: root.primary
                fontSize: 15
                fieldColor: "#090909"
                borderColor: "#191919"
                textColor: "#eaeaea"
                mutedColor: "#6f6f6f"
                listColor: "#141414"
                onPicked: (code) => root.primaryRequested(code)
            }
        }
        Column {
            spacing: 10
            Text {
                text: qsTr("Second language (when there are none)")
                color: "#8f8f8f"
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: 14
            }
            Widgets.LanguagePicker {
                objectName: "onboardingSecondaryLanguage"
                width: 320
                height: 46
                code: root.secondary
                allowNone: true
                fontSize: 15
                fieldColor: "#090909"
                borderColor: "#191919"
                textColor: "#eaeaea"
                mutedColor: "#6f6f6f"
                listColor: "#141414"
                onPicked: (code) => root.secondaryRequested(code)
            }
        }
    }

    Column {
        y: 392
        anchors.horizontalCenter: parent.horizontalCenter
        width: 680
        spacing: 18

        ToggleRow {
            objectName: "onboardingSubtitleAutoSearch"
            text: qsTr("Find subtitles online when a video has none in these languages")
            checked: root.autoSearch
            onToggled: root.autoSearchRequested(checked)
        }
    }

    Text {
        x: Math.floor((parent.width - width) / 2)
        y: 520
        width: 720
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: qsTr("Online subtitles come from free services (OpenSubtitles, and more for some languages). Change all of it in Settings, Subtitles.")
        color: "#5a5a5a"
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 13
    }

    // a switch of the redesign and its text
    component ToggleRow: T.Switch {
        id: toggle
        width: parent.width
        implicitHeight: 28
        focusPolicy: Qt.StrongFocus
        opacity: enabled ? 1 : 0.4
        Accessible.name: text
        indicator: Rectangle {
            x: 0
            anchors.verticalCenter: parent.verticalCenter
            width: 44
            height: 24
            radius: 12
            color: toggle.checked ? VLCStyle.redesign.accent : "#262626"
            border.width: toggle.visualFocus ? 2 : 0
            border.color: "#eaeaea"
            Rectangle {
                x: toggle.checked ? parent.width - width - 3 : 3
                anchors.verticalCenter: parent.verticalCenter
                width: 18
                height: 18
                radius: 9
                color: "#eaeaea"
                Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
            }
        }
        contentItem: Text {
            leftPadding: 60
            verticalAlignment: Text.AlignVCenter
            text: toggle.text
            color: "#b4b4b4"
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: 15
            elide: Text.ElideRight
        }
    }
}
