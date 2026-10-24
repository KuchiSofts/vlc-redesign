/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.MainInterface
import VLC.Style

// Interface language (General > Appearance): a dropdown as in the design
// where VLC has its own language setting (Windows), shown at once; elsewhere
// VLC follows the system language
Item {
    id: root
    objectName: "interfaceLanguageAction"
    implicitHeight: languageRow.visible ? languageRow.implicitHeight : VLCStyle.dp(46, VLCStyle.scale)

    // [{ value: iso, label: name }], empty when VLC follows the system
    property var languages: []
    property string language: "auto"
    property bool changed: false
    readonly property bool dark: VLCStyle.palette.isDark

    signal chosen(string iso)

    SettingsOptionRow {
        id: languageRow
        visible: root.languages.length > 0
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        showInfo: false
        settingKey: "interface-language"
        label: qsTr("Interface language")
        settingType: "string"
        choices: root.languages
        settingValue: root.language
        dirty: root.changed
        onCommit: (value) => root.chosen(value)
    }

    Label {
        visible: root.languages.length === 0
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: VLCStyle.dp(14, VLCStyle.scale)
        text: qsTr("Interface language")
        color: root.dark ? "#949494" : "#494949"
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
    }

    Label {
        visible: root.languages.length === 0
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: VLCStyle.dp(14, VLCStyle.scale)
        text: qsTr("System language (%1)").arg(Qt.locale().name)
        color: root.dark ? "#8c8c8c" : "#555555"
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
    }
}
