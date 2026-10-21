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
import QtQuick.Controls.Basic as Controls

import VLC.Style
import VLC.Util

// A language among all of them (SubtitlePreferences.languages): a field with
// the one chosen, and a list to type in (its English name, its own name or
// its code), the system's language and English first
T.Button {
    id: root

    property string code: ""
    // a choice of no language at all ("None"), for a secondary language
    property bool allowNone: false
    property string noneText: qsTr("None")
    signal picked(string code)

    // the colours of the place it is in
    property color fieldColor: VLCStyle.redesign.color("#090909", "#c4c4c3")
    property color borderColor: VLCStyle.redesign.color("#191919", "#9c9c9b")
    property color textColor: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#1e1e1e")
    property color mutedColor: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
    property color listColor: VLCStyle.redesign.color("#141414", "#b4b4b3")
    property real fontSize: VLCStyle.dp(14, VLCStyle.scale)
    property real listHeight: VLCStyle.dp(320, VLCStyle.scale)

    readonly property string _name: code === "" ? noneText : SubtitlePreferences.name(code)
    readonly property string _native: code === "" ? "" : SubtitlePreferences.nativeName(code)

    implicitWidth: VLCStyle.dp(280, VLCStyle.scale)
    implicitHeight: VLCStyle.dp(44, VLCStyle.scale)
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.ComboBox
    Accessible.name: _name

    onClicked: {
        popup.open()
    }

    background: Rectangle {
        radius: VLCStyle.dp(6, VLCStyle.scale)
        color: root.fieldColor
        border.width: root.visualFocus || popup.visible ? 2 : 2
        border.color: root.visualFocus || popup.visible ? VLCStyle.redesign.accent : root.borderColor
    }

    contentItem: RowLayout {
        spacing: VLCStyle.dp(8, VLCStyle.scale)
        Text {
            Layout.leftMargin: VLCStyle.dp(14, VLCStyle.scale)
            text: root._name
            color: root.textColor
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.fontSize
            elide: Text.ElideRight
            Layout.maximumWidth: root.width * 0.6
        }
        Text {
            Layout.fillWidth: true
            visible: root._native !== "" && root._native !== root._name
            text: root._native
            color: root.mutedColor
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.fontSize - 1
            elide: Text.ElideRight
        }
        Item { Layout.fillWidth: !(root._native !== "" && root._native !== root._name) }
        Image {
            Layout.rightMargin: VLCStyle.dp(12, VLCStyle.scale)
            source: "qrc:///redesign/icons/chevron-down/default.png"
            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
            opacity: 0.8
        }
    }

    T.Popup {
        id: popup
        y: root.height + VLCStyle.dp(4, VLCStyle.scale)
        width: root.width
        height: root.listHeight
        // kept in the window (moved up, or made lower, near its bottom)
        margins: 0
        padding: VLCStyle.dp(6, VLCStyle.scale)
        focus: true
        closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside
        onOpened: {
            search.text = ""
            search.forceActiveFocus()
            list.positionViewAtIndex(Math.max(0, list.currentIndexOf(root.code)), ListView.Center)
        }

        background: Rectangle {
            radius: VLCStyle.dp(8, VLCStyle.scale)
            color: root.listColor
            border.width: 1
            border.color: root.borderColor
        }

        contentItem: ColumnLayout {
            spacing: VLCStyle.dp(6, VLCStyle.scale)

            T.TextField {
                id: search
                Layout.fillWidth: true
                implicitHeight: VLCStyle.dp(34, VLCStyle.scale)
                leftPadding: VLCStyle.dp(10, VLCStyle.scale)
                rightPadding: VLCStyle.dp(10, VLCStyle.scale)
                placeholderText: qsTr("Search a language")
                placeholderTextColor: root.mutedColor
                color: root.textColor
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.fontSize
                verticalAlignment: TextInput.AlignVCenter
                selectByMouse: true
                background: Rectangle {
                    radius: VLCStyle.dp(6, VLCStyle.scale)
                    color: root.fieldColor
                    border.width: 1
                    border.color: search.activeFocus ? VLCStyle.redesign.accent : root.borderColor
                }
                Keys.onDownPressed: (event) => { list.forceActiveFocus(); list.currentIndex = 0; event.accepted = true }
                onAccepted: {
                    if (list.count > 0)
                        list.choose(list.model[0].code)
                }
            }

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                keyNavigationEnabled: true

                function currentIndexOf(code) {
                    for (let i = 0; i < model.length; ++i)
                        if (model[i].code === code)
                            return i
                    return -1
                }
                function choose(code) {
                    root.picked(code)
                    popup.close()
                }

                // the system's language, English and the one chosen first,
                // then all of them by name; the search keeps those it finds
                model: {
                    const all = SubtitlePreferences.languages
                    const query = search.text.trim().toLowerCase()
                    const first = []
                    if (root.allowNone)
                        first.push({ code: "", name: root.noneText, native: "" })
                    if (query === "") {
                        const suggested = [SubtitlePreferences.systemLanguage(), "en", root.code]
                        for (const code of suggested) {
                            if (code === "" || first.some(e => e.code === code))
                                continue
                            const entry = all.find(e => e.code === code)
                            if (entry)
                                first.push(entry)
                        }
                        return first.concat(all.filter(e => !first.some(f => f.code === e.code)))
                    }
                    const matches = all.filter(e => e.name.toLowerCase().includes(query)
                                                    || e.native.toLowerCase().includes(query)
                                                    || e.code.toLowerCase() === query)
                    // those starting with it first
                    matches.sort((a, b) => (b.name.toLowerCase().startsWith(query) ? 1 : 0)
                                           - (a.name.toLowerCase().startsWith(query) ? 1 : 0))
                    return (root.allowNone && root.noneText.toLowerCase().includes(query) ? first : []).concat(matches)
                }

                delegate: T.ItemDelegate {
                    id: item
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    implicitHeight: VLCStyle.dp(34, VLCStyle.scale)
                    focusPolicy: Qt.StrongFocus
                    highlighted: ListView.isCurrentItem && list.activeFocus
                    Accessible.name: modelData.name + (modelData.native ? ", " + modelData.native : "")
                    onClicked: list.choose(modelData.code)
                    Keys.onReturnPressed: list.choose(modelData.code)
                    background: Rectangle {
                        radius: VLCStyle.dp(4, VLCStyle.scale)
                        color: item.highlighted || item.hovered ? Qt.rgba(VLCStyle.redesign.accent.r, VLCStyle.redesign.accent.g,
                                                                          VLCStyle.redesign.accent.b, 0.16)
                                                               : "transparent"
                    }
                    contentItem: RowLayout {
                        spacing: VLCStyle.dp(8, VLCStyle.scale)
                        Text {
                            Layout.leftMargin: VLCStyle.dp(8, VLCStyle.scale)
                            text: item.modelData.name
                            color: item.modelData.code === root.code ? VLCStyle.redesign.accent : root.textColor
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.fontSize
                            elide: Text.ElideRight
                            Layout.maximumWidth: item.width * 0.55
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: item.modelData.native !== "" && item.modelData.native !== item.modelData.name
                            text: item.modelData.native
                            color: root.mutedColor
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.fontSize - 1
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignRight
                            Layout.rightMargin: VLCStyle.dp(8, VLCStyle.scale)
                        }
                    }
                }
                Controls.ScrollBar.vertical: Controls.ScrollBar { policy: Controls.ScrollBar.AsNeeded }
            }
        }
    }
}
