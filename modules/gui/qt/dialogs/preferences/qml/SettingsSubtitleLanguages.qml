/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import VLC.Player
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// The subtitle languages (SubtitlePreferences, also asked by the setup): the
// default one (picked by Preferred language, the row of the design above),
// the second one (when there are none in the first), the online search, both
// at once (dual subtitles, off); then the languages to search online, in the
// order of the results (each on or off, others added; every other language
// too or not: fewer, a faster search). The services asked follow
// (SettingsSubtitleServices). Kept when the settings are (save). Below the
// sections of the design: the page starts as it draws it
SettingsSectionTitle {
    id: root
    text: qsTr("Second Language & Online Subtitles")

    property string primary: "en"
    property string secondary: ""
    property bool autoSearch: true
    property bool showBoth: false
    // the languages to search: { code, on } in their order
    property var searchRows: []
    property bool everyLanguage: false
    // those on (the default one always): the services of a language follow
    readonly property var searchOn: searchRows.filter(row => row.on || row.code === primary).map(row => row.code)

    function begin() {
        primary = SubtitlePreferences.primary
        secondary = SubtitlePreferences.secondary
        autoSearch = SubtitlePreferences.autoSearch
        showBoth = SubtitlePreferences.showBoth
        searchRows = SubtitlePreferences.searchLanguages.map(row => ({ code: row.code, on: row.on }))
        everyLanguage = SubtitlePreferences.everyLanguage
        _keepChosen()
    }

    function save() {
        if (primary !== SubtitlePreferences.primary || secondary !== SubtitlePreferences.secondary
            || autoSearch !== SubtitlePreferences.autoSearch || showBoth !== SubtitlePreferences.showBoth
            || !SubtitlePreferences.chosen)
            SubtitlePreferences.set(primary, secondary, autoSearch, showBoth)
        SubtitlePreferences.setSearch(searchRows, everyLanguage)
    }

    function pickPrimary(code) {
        if (!code)
            return
        primary = code
        if (secondary === code)
            secondary = ""
    }

    // the default and the second language are in the list (a new one on)
    function _keepChosen() {
        const rows = searchRows.slice()
        if (secondary !== "" && !rows.some(row => row.code === secondary))
            rows.splice(Math.min(1, rows.length), 0, { code: secondary, on: true })
        if (!rows.some(row => row.code === primary))
            rows.unshift({ code: primary, on: true })
        searchRows = rows
    }
    onPrimaryChanged: _keepChosen()
    onSecondaryChanged: _keepChosen()

    function _setRow(index, on) {
        const rows = searchRows.slice()
        rows[index] = { code: rows[index].code, on: on }
        searchRows = rows
    }
    function _move(index, by) {
        const to = index + by
        if (to < 0 || to >= searchRows.length)
            return
        const rows = searchRows.slice()
        const row = rows.splice(index, 1)[0]
        rows.splice(to, 0, row)
        searchRows = rows
    }
    function _remove(index) {
        const rows = searchRows.slice()
        rows.splice(index, 1)
        searchRows = rows
    }
    function _add(code) {
        if (!code)
            return
        const index = searchRows.findIndex(row => row.code === code)
        if (index >= 0)
            _setRow(index, true)
        else
            searchRows = searchRows.concat([{ code: code, on: true }])
    }

    function c(value) { return VLCStyle.redesign.color(value) }
    readonly property real controlWidth: Math.min(px(320), width * 0.46)

    component Label: Text {
        id: rowLabel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: root.controlWidth + root.px(16)
        anchors.baseline: parent.verticalCenter
        anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
        color: root.c("#8F8F8F")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(14)
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
        // the whole text when it is cut short
        HoverHandler { id: labelHover }
        T.ToolTip.visible: labelHover.hovered && rowLabel.truncated
        T.ToolTip.text: rowLabel.text
        T.ToolTip.delay: 650
    }

    // A small button of an icon (move, remove), its description as tooltip
    component IconButton: T.Button {
        id: iconButton
        property string iconSource
        property real iconRotation: 0
        property string description
        implicitWidth: root.px(32)
        implicitHeight: root.px(32)
        focusPolicy: Qt.StrongFocus
        Accessible.name: description
        T.ToolTip.visible: (hovered || visualFocus) && description.length > 0
        T.ToolTip.text: description
        T.ToolTip.delay: VLCStyle.delayToolTipAppear
        background: Rectangle {
            radius: root.px(6)
            color: iconButton.down ? root.c("#090909") : iconButton.hovered ? root.c("#222222") : "transparent"
            border.width: iconButton.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.accent
        }
        contentItem: Image {
            source: iconButton.iconSource
            sourceSize: Qt.size(root.px(20), root.px(20))
            fillMode: Image.Pad
            rotation: iconButton.iconRotation
            opacity: iconButton.enabled ? 1 : 0.3
        }
    }

    Item {
        width: parent.width
        height: root.px(52)
        Label { text: qsTr("Second language (when there are none in the preferred one)") }
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
            on: root.autoSearch
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
            on: root.showBoth
            onSwitched: (on) => root.showBoth = on
        }
    }

    // The languages to search online, in the order of the results
    Text {
        width: parent.width
        topPadding: root.px(14)
        bottomPadding: root.px(4)
        text: qsTr("Languages to search")
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignLeft
        color: root.c("#B8B8B8")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(15)
        font.weight: Font.Medium
    }
    Repeater {
        model: root.searchRows
        delegate: Item {
            id: languageRow
            required property var modelData
            required property int index
            readonly property bool preferred: modelData.code === root.primary
            readonly property string languageName: {
                void SubtitlePreferences.languages
                const name = SubtitlePreferences.name(modelData.code)
                const own = SubtitlePreferences.nativeName(modelData.code)
                return own !== "" && own !== name ? name + " · " + own : name
            }
            objectName: "settingsSearchLanguage_" + modelData.code
            width: root.width
            height: root.px(46)
            Text {
                anchors.left: parent.left
                anchors.right: languageControls.left
                anchors.rightMargin: root.px(12)
                anchors.verticalCenter: parent.verticalCenter
                text: languageRow.languageName
                color: languageRow.modelData.on || languageRow.preferred ? root.c("#8F8F8F") : root.c("#5F5F5F")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(14)
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
            }
            Row {
                id: languageControls
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.px(4)
                IconButton {
                    objectName: "settingsSearchLanguageUp_" + languageRow.modelData.code
                    anchors.verticalCenter: parent.verticalCenter
                    iconSource: "qrc:///redesign/icons/chevron-down/default.svg"
                    iconRotation: 180
                    description: qsTr("Move up")
                    enabled: languageRow.index > 0
                    onClicked: root._move(languageRow.index, -1)
                }
                IconButton {
                    objectName: "settingsSearchLanguageDown_" + languageRow.modelData.code
                    anchors.verticalCenter: parent.verticalCenter
                    iconSource: "qrc:///redesign/icons/chevron-down/default.svg"
                    description: qsTr("Move down")
                    enabled: languageRow.index < root.searchRows.length - 1
                    onClicked: root._move(languageRow.index, 1)
                }
                IconButton {
                    objectName: "settingsSearchLanguageRemove_" + languageRow.modelData.code
                    anchors.verticalCenter: parent.verticalCenter
                    iconSource: SVGColorImage.colorize("qrc:///redesign/duo/close.svg").color1(root.c("#8F8F8F")).uri()
                    description: qsTr("Remove")
                    // (the preferred language stays)
                    enabled: !languageRow.preferred
                    opacity: enabled ? 1 : 0
                    onClicked: root._remove(languageRow.index)
                }
                Item { width: root.px(8); height: 1 }
                SettingsToggle {
                    objectName: "settingsSearchLanguageToggle_" + languageRow.modelData.code
                    anchors.verticalCenter: parent.verticalCenter
                    enabled: !languageRow.preferred
                    on: languageRow.modelData.on || languageRow.preferred
                    Accessible.name: languageRow.languageName
                    onSwitched: (on) => root._setRow(languageRow.index, on)
                }
            }
        }
    }
    Item {
        width: parent.width
        height: root.px(52)
        Widgets.LanguagePicker {
            id: addLanguage
            objectName: "settingsSearchLanguageAdd"
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: root.controlWidth
            height: root.px(40)
            // (its text stays: a language picked is added to the list)
            code: ""
            allowNone: true
            noneText: qsTr("Add a language")
            onPicked: (code) => root._add(code)
        }
    }
    Item {
        width: parent.width
        height: root.px(46)
        Label { text: qsTr("Every other language too") }
        SettingsToggle {
            objectName: "settingsSearchEveryLanguage"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            on: root.everyLanguage
            onSwitched: (on) => root.everyLanguage = on
        }
    }
    Text {
        width: parent.width
        topPadding: root.px(2)
        bottomPadding: root.px(8)
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignLeft
        text: qsTr("Fewer languages: a faster search")
        color: root.c("#5F5F5F")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(12)
    }
}
