/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import VLC.Player
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// The subtitle languages (SubtitlePreferences, also asked by the setup): the
// default one, the second one (when there are none in the first), the online
// search, both at once (dual subtitles, off); then the services asked online
// (SubtitleFinder). Kept when the settings are (save)
SettingsSectionTitle {
    id: root
    text: qsTr("Subtitle Languages")

    property string primary: "en"
    property string secondary: ""
    property bool autoSearch: true
    property bool showBoth: false
    // the services turned on, by id
    property var services: ({})

    function begin() {
        primary = SubtitlePreferences.primary
        secondary = SubtitlePreferences.secondary
        autoSearch = SubtitlePreferences.autoSearch
        showBoth = SubtitlePreferences.showBoth
        const on = {}
        for (const service of SubtitleFinder.services)
            on[service.id] = service.enabled
        services = on
    }

    function save() {
        if (primary !== SubtitlePreferences.primary || secondary !== SubtitlePreferences.secondary
            || autoSearch !== SubtitlePreferences.autoSearch || showBoth !== SubtitlePreferences.showBoth
            || !SubtitlePreferences.chosen)
            SubtitlePreferences.set(primary, secondary, autoSearch, showBoth)
        for (const id in services)
            SubtitleFinder.setServiceEnabled(id, services[id])
    }

    function c(value) { return VLCStyle.redesign.color(value, value) }
    readonly property real controlWidth: Math.min(px(320), width * 0.46)

    component Label: Text {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: root.controlWidth + root.px(16)
        anchors.baseline: parent.verticalCenter
        anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
        color: root.c("#8F8F8F")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(14)
        elide: Text.ElideRight
    }

    Item {
        width: parent.width
        height: root.px(52)
        Label { text: qsTr("Default language") }
        Widgets.LanguagePicker {
            objectName: "settingsSubtitlePrimary"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: root.controlWidth
            height: root.px(40)
            code: root.primary
            onPicked: (code) => {
                root.primary = code
                if (root.secondary === code)
                    root.secondary = ""
            }
        }
    }

    Item {
        width: parent.width
        height: root.px(52)
        Label { text: qsTr("Second language (when there are none in the default one)") }
        Widgets.LanguagePicker {
            objectName: "settingsSubtitleSecondary"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: root.controlWidth
            height: root.px(40)
            code: root.secondary
            allowNone: true
            onPicked: (code) => {
                root.secondary = code === root.primary ? "" : code
                if (root.secondary === "")
                    root.showBoth = false
            }
        }
    }

    Item {
        width: parent.width
        height: root.px(46)
        Label { text: qsTr("Find subtitles online when a video has none in these languages") }
        SettingsToggle {
            objectName: "settingsSubtitleAutoSearch"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            checked: root.autoSearch
            onSwitched: (on) => root.autoSearch = on
        }
    }

    Item {
        width: parent.width
        height: root.px(46)
        opacity: root.secondary !== "" ? 1 : 0.4
        Label { text: qsTr("Show both languages at once (dual subtitles)") }
        SettingsToggle {
            objectName: "settingsSubtitleShowBoth"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            enabled: root.secondary !== ""
            checked: root.showBoth
            onSwitched: (on) => root.showBoth = on
        }
    }

    // The services asked online: free, without an account
    Text {
        width: parent.width
        topPadding: root.px(14)
        bottomPadding: root.px(4)
        text: qsTr("Subtitle services")
        color: root.c("#B8B8B8")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(15)
        font.weight: Font.Medium
    }
    Repeater {
        model: SubtitleFinder.services
        delegate: Item {
            required property var modelData
            width: root.width
            height: root.px(52)
            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: root.px(80)
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.px(2)
                Text {
                    width: parent.width
                    text: modelData.name
                    color: root.c("#8F8F8F")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(14)
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: modelData.detail
                    color: root.c("#5F5F5F")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(12)
                    elide: Text.ElideRight
                }
            }
            SettingsToggle {
                objectName: "settingsSubtitleService_" + modelData.id
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                checked: root.services[modelData.id] !== false
                onSwitched: (on) => {
                    const states = Object.assign({}, root.services)
                    states[modelData.id] = on
                    root.services = states
                }
            }
        }
    }

    Text {
        width: parent.width
        topPadding: root.px(4)
        bottomPadding: root.px(8)
        wrapMode: Text.WordWrap
        text: qsTr("To find subtitles, the services turned on get the video's title, its IMDb id, its file name and hash (made from a part of the file), and nothing else.")
        color: root.c("#5F5F5F")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(12)
    }
}
