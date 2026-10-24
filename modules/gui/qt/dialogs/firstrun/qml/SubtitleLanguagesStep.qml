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
    // false: the setup asks it on a step of its own (with the update check)
    property bool showAutoSearch: true
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
        color: VLCStyle.redesign.color("#191919", "#ababaa")
    }
    Rectangle {
        x: 17
        y: 41
        width: parent.width - 34
        height: 542
        color: VLCStyle.redesign.color("#090909", "#dedede")
    }

    Text {
        id: title
        x: Math.floor((parent.width - width) / 2)
        y: 132
        // (inside the player behind, a longer one cut)
        width: Math.min(implicitWidth, parent.width - 124)
        elide: Text.ElideRight
        text: qsTr("Choose the languages of your subtitles.")
        color: VLCStyle.redesign.color("#969696", "#4d4d4d")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 24
        font.weight: Font.Light
    }
    Text {
        id: intro
        x: Math.floor((parent.width - width) / 2)
        y: 184
        width: 700
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: qsTr("Subtitles in your language show by themselves: from the video, from a file next to it, or found online. When there are none in it, not even online, the second language is used.")
        color: VLCStyle.redesign.color("#6f6f6f", "#5a5a59")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 15
        lineHeight: 1.3
    }

    // What follows moves down under longer texts (a translation), never up
    // from where the design puts it; the two pickers in the reading order
    Row {
        id: pickers
        y: Math.max(262, intro.y + intro.height + 12)
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 40

        Column {
            spacing: 10
            PickerLabel {
                text: qsTr("Default language")
            }
            Widgets.LanguagePicker {
                objectName: "onboardingPrimaryLanguage"
                width: 320
                height: 46
                code: root.primary
                fontSize: 15
                fieldColor: VLCStyle.redesign.color("#090909", "#c4c4c3")
                borderColor: VLCStyle.redesign.color("#191919", "#9c9c9b")
                textColor: VLCStyle.redesign.color("#eaeaea", "#1e1e1e")
                mutedColor: VLCStyle.redesign.color("#6f6f6f", "#6b6b6b")
                listColor: VLCStyle.redesign.color("#141414", "#b4b4b3")
                onPicked: (code) => root.primaryRequested(code)
            }
        }
        Column {
            spacing: 10
            PickerLabel {
                text: qsTr("Second language (when there are none)")
            }
            Widgets.LanguagePicker {
                objectName: "onboardingSecondaryLanguage"
                width: 320
                height: 46
                code: root.secondary
                allowNone: true
                fontSize: 15
                fieldColor: VLCStyle.redesign.color("#090909", "#c4c4c3")
                borderColor: VLCStyle.redesign.color("#191919", "#9c9c9b")
                textColor: VLCStyle.redesign.color("#eaeaea", "#1e1e1e")
                mutedColor: VLCStyle.redesign.color("#6f6f6f", "#6b6b6b")
                listColor: VLCStyle.redesign.color("#141414", "#b4b4b3")
                onPicked: (code) => root.secondaryRequested(code)
            }
        }
    }

    Column {
        id: switches
        y: Math.max(392, pickers.y + pickers.height + 24)
        anchors.horizontalCenter: parent.horizontalCenter
        width: 680
        spacing: 18

        ToggleRow {
            objectName: "onboardingSubtitleAutoSearch"
            visible: root.showAutoSearch
            text: qsTr("Find subtitles online when a video has none in these languages")
            checked: root.autoSearch
            onToggled: root.autoSearchRequested(checked)
        }
    }

    Text {
        x: Math.floor((parent.width - width) / 2)
        // (in the place of the switch when the next step asks it)
        y: root.showAutoSearch ? Math.max(520, switches.y + switches.height + 24) : switches.y
        width: 720
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: qsTr("Online subtitles come from free services (OpenSubtitles, and more for some languages). Change all of it in Settings, Subtitles.")
        color: VLCStyle.redesign.color("#5a5a5a", "#6b6b6b")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 13
    }

    // the name over a picker, as wide as it: a longer one cut, whole in its
    // tooltip
    component PickerLabel: Text {
        id: pickerLabel
        width: 320
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
        color: VLCStyle.redesign.color("#8f8f8f", "#2e2e2e")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: 14
        HoverHandler { id: pickerLabelHover }
        T.ToolTip.visible: pickerLabelHover.hovered && pickerLabel.truncated
        T.ToolTip.text: pickerLabel.text
        T.ToolTip.delay: 650
    }

    // a switch of the redesign and its text (right to left: the switch on
    // the right, on toward the left); a longer text on more lines
    component ToggleRow: T.Switch {
        id: toggle
        width: parent.width
        implicitHeight: Math.max(28, implicitContentHeight + topPadding + bottomPadding)
        focusPolicy: Qt.StrongFocus
        opacity: enabled ? 1 : 0.4
        Accessible.name: text
        indicator: Rectangle {
            x: toggle.mirrored ? toggle.width - width : 0
            anchors.verticalCenter: parent.verticalCenter
            width: 44
            height: 24
            radius: 12
            color: toggle.checked ? VLCStyle.redesign.accent : VLCStyle.redesign.color("#262626", "#9c9c9b")
            border.width: toggle.visualFocus ? 2 : 0
            border.color: VLCStyle.redesign.color("#eaeaea", "#2e2e2e")
            Rectangle {
                x: toggle.checked !== toggle.mirrored ? parent.width - width - 3 : 3
                anchors.verticalCenter: parent.verticalCenter
                width: 18
                height: 18
                radius: 9
                color: "#eaeaea"
                Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
            }
        }
        contentItem: Text {
            leftPadding: toggle.mirrored ? 0 : 60
            rightPadding: toggle.mirrored ? 60 : 0
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            text: toggle.text
            color: VLCStyle.redesign.color("#b4b4b4", "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: 15
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }
    }
}
