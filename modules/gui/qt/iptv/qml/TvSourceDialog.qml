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
import VLC.Util
import VLC.Widgets as Widgets
import VLC.Dialogs

// The TV sources (the dialogs of the design, as Open Network Stream): the
// ones added, with what each can do; a link of the user's (an M3U playlist
// or file, an Xtream Codes login, a guide); or one of the catalog of free,
// legal channel lists, added in one click with its guide.
Widgets.RedesignDialog {
    id: root

    objectName: "tvSourceDialog"

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(dark, light) { return VLCStyle.redesign.color(dark, light, root.darkTheme) }

    // "sources", "link" or "catalog"
    property string page: "sources"
    // the link page: "m3u", "xtream" or "epg"; the source edited ("": a new one)
    property string kind: "m3u"
    property string editing: ""
    property string _error: ""
    property bool _saving: false
    property var _catalog: null

    function openAt(which) {
        page = which || "sources"
        if (page === "link" && editing === "")
            _clear()
        open()
    }

    function _clear() {
        editing = ""
        _error = ""
        nameField.text = ""
        urlField.text = ""
        guidesField.text = ""
        serverField.text = ""
        userField.text = ""
        passwordField.text = ""
    }

    function edit(id) {
        _clear()
        editing = id
        Iptv.getSource(id, (source) => {
            root.kind = source.kind === "xtream" ? "xtream" : "m3u"
            nameField.text = source.name || ""
            urlField.text = source.url || ""
            guidesField.text = (source.epg || []).join("\n")
            serverField.text = source.server || ""
            userField.text = source.username || ""
            // the password stays where it is: an empty field keeps it
            passwordField.text = ""
            root.page = "link"
        })
    }

    title: qsTr("TV Sources")
    preferredWidth: VLCStyle.dp(1048, VLCStyle.scale)
    bodyHeight: VLCStyle.dp(560, VLCStyle.scale)
    closeOnAccept: false
    cancelText: qsTr("Close")
    cancelVisible: page !== "link"
    acceptVisible: page === "link"
    acceptText: editing !== "" ? qsTr("Save") : kind === "epg" ? qsTr("Add Guide") : qsTr("Add Source")
    acceptEnabled: !_saving && (kind === "m3u" ? urlField.text.trim() !== ""
                               : kind === "xtream" ? serverField.text.trim() !== "" && userField.text.trim() !== ""
                                                     && (editing !== "" || passwordField.text !== "")
                               : guidesField.text.trim() !== "" && epgTarget.value !== "")
    extraText: page === "link" ? qsTr("Back") : ""
    onExtraClicked: { _clear(); page = "sources" }
    onClosed: _clear()

    onAccepted: {
        _error = ""
        const guides = guidesField.text.split(/\s+/).filter(u => u.length > 0)
        if (kind === "epg") {
            // a guide for a source added before
            Iptv.getSource(epgTarget.value, (source) => {
                const epg = (source.epg || []).concat(guides.filter(g => (source.epg || []).indexOf(g) < 0))
                Iptv.updateSource(epgTarget.value, Object.assign(source, { epg: epg, password: "" }), () => {
                    root._clear()
                    root.page = "sources"
                })
            })
            return
        }
        const source = kind === "xtream"
                       ? { kind: "xtream", name: nameField.text.trim(), server: serverField.text.trim(),
                           username: userField.text.trim(), password: passwordField.text, epg: guides }
                       : { kind: "m3u", name: nameField.text.trim(), url: urlField.text.trim(), epg: guides }
        _saving = true
        const done = (answer) => {
            root._saving = false
            if (answer.error) {
                root._error = qsTr("This source cannot be added: fill in the fields marked as needed.")
                return
            }
            root._clear()
            root.page = "sources"
        }
        if (editing !== "")
            Iptv.updateSource(editing, source, done)
        else
            Iptv.addSource(source, done)
    }

    onPageChanged: {
        if (page === "catalog" && !_catalog)
            Iptv.catalog((catalog) => root._catalog = catalog)
    }

    headerContent: [
        RowLayout {
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            Layout.fillWidth: true
            spacing: VLCStyle.dp(10, VLCStyle.scale)

            SettingsSegmented {
                Layout.preferredWidth: Math.max(VLCStyle.dp(420, VLCStyle.scale), neededWidth)
                options: [{ text: qsTr("Your Sources (%1)").arg(Iptv.sources.length), value: "sources" },
                          { text: qsTr("Add a Link"), value: "link" },
                          { text: qsTr("Catalog"), value: "catalog" }]
                value: root.page
                onChosen: (value) => {
                    if (value === "link")
                        root._clear()
                    root.page = value
                }
            }
            Item { Layout.fillWidth: true }
        }
    ]

    // a field of one line as RedesignTextArea, for the password (hidden)
    component PasswordField: T.TextField {
        id: field
        Layout.fillWidth: true
        implicitHeight: VLCStyle.dp(37, VLCStyle.scale)
        leftPadding: VLCStyle.dp(14, VLCStyle.scale)
        rightPadding: VLCStyle.dp(14, VLCStyle.scale)
        verticalAlignment: TextInput.AlignVCenter
        horizontalAlignment: TextInput.AlignLeft
        echoMode: TextInput.Password
        color: root.c("#C6C6C6", "#2e2e2e")
        selectionColor: VLCStyle.redesign.accentBackground
        selectedTextColor: root.c(VLCStyle.redesign.textPrimary, "#191919")
        placeholderTextColor: root.c(VLCStyle.redesign.textDisabled, "#7c7c7b")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        selectByMouse: true
        background: Rectangle {
            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: root.c("#090909", "#dedede")
            border.width: VLCStyle.dp(2, VLCStyle.scale)
            border.color: field.activeFocus ? VLCStyle.redesign.accentDim : root.c("#191919", "#c4c4c3")
        }
        Text {
            x: field.leftPadding
            anchors.verticalCenter: parent.verticalCenter
            visible: field.length === 0 && field.preeditText === ""
            text: field.placeholderText
            color: field.placeholderTextColor
            font: field.font
        }
        Keys.onReturnPressed: root.accept()
        Keys.onEnterPressed: root.accept()
    }

    Widgets.FilePicker { id: filePicker; ctx: MainCtx }

    // ---- the sources added ----
    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: root.page === "sources"
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        spacing: VLCStyle.dp(12, VLCStyle.scale)

        Widgets.RedesignDialogHeading {
            text: qsTr("Your TV sources")
            description: Iptv.sources.length > 0
                         ? qsTr("The guides of the first sources come first when several name the same channel.")
                         : qsTr("No source yet: add the link of your provider, or one of the catalog.")
        }

        ListView {
            id: sourceList
            // (the body of the dialog is as high as its content)
            Layout.preferredHeight: root.bodyHeight - VLCStyle.dp(150, VLCStyle.scale)
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: VLCStyle.dp(8, VLCStyle.scale)
            model: Iptv.sources
            boundsBehavior: Flickable.StopAtBounds
            T.ScrollBar.vertical: Widgets.ScrollBarExt { }

            delegate: Rectangle {
                id: sourceRow
                required property var modelData
                required property int index
                width: ListView.view.width
                height: VLCStyle.dp(76, VLCStyle.scale)
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: root.c("#151515", "#b5b5b4")
                opacity: modelData.enabled ? 1 : 0.6

                readonly property var s: modelData

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: root.px(16)
                    anchors.right: actions.left
                    anchors.rightMargin: root.px(16)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: root.px(6)

                    Row {
                        spacing: root.px(8)
                        width: parent.width
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: sourceRow.s.name
                            elide: Text.ElideRight
                            width: Math.min(implicitWidth, parent.width * 0.5)
                            color: root.c("#C6C6C6", "#2e2e2e")
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.px(15)
                            font.weight: Font.Medium
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: (sourceRow.s.kind === "xtream" ? "Xtream Codes · " : "M3U · ") + sourceRow.s.host
                            color: root.c("#6B6B6B", "#4d4d4d")
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.px(12)
                        }
                    }
                    // what the source can do, detected from its channels
                    Row {
                        spacing: root.px(6)
                        TvBadge {
                            text: qsTr("%1 channel", "%1 channels", sourceRow.s.channels).arg(sourceRow.s.channels)
                        }
                        TvBadge {
                            visible: sourceRow.s.withGuide > 0
                            icon: "lists"
                            text: qsTr("Guide %1", "Guide %1", sourceRow.s.withGuide).arg(sourceRow.s.withGuide)
                            tip: qsTr("%1 channel has a programme guide", "%1 channels have a programme guide",
                                      sourceRow.s.withGuide).arg(sourceRow.s.withGuide)
                        }
                        TvBadge {
                            visible: sourceRow.s.catchupDays > 0
                            icon: "recent"
                            text: qsTr("Catch-up %1 d", "Catch-up %1 d", sourceRow.s.catchupDays).arg(sourceRow.s.catchupDays)
                        }
                        TvBadge { visible: sourceRow.s.vod; icon: "video"; text: qsTr("Films") }
                        TvBadge { visible: sourceRow.s.series; icon: "playlist"; text: qsTr("Series") }
                    }
                    Text {
                        width: parent.width
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        text: sourceRow.s.busy ? (sourceRow.s.stage === "guide" ? qsTr("Updating the guide… %1 MB")
                                                                                     .arg((sourceRow.s.received / 1048576).toFixed(1))
                                                                                : qsTr("Updating the channels…"))
                              : sourceRow.s.error ? sourceRow.s.error
                              : sourceRow.s.updated > 0 ? qsTr("Updated %1 at %2").arg(Iptv.dayText(sourceRow.s.updated))
                                                                             .arg(Iptv.timeText(sourceRow.s.updated))
                              : ""
                        color: sourceRow.s.error && !sourceRow.s.busy ? root.c("#e15033", "#b03a22") : root.c("#6B6B6B", "#4d4d4d")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(12)
                    }
                }

                Row {
                    id: actions
                    anchors.right: parent.right
                    anchors.rightMargin: root.px(16)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: root.px(8)

                    Widgets.RedesignButton {
                        visible: sourceRow.index > 0
                        implicitWidth: root.px(40)
                        text: "▲"
                        Accessible.name: qsTr("Move up")
                        onClicked: Iptv.moveSource(sourceRow.s.id, sourceRow.index - 1)
                    }
                    Widgets.RedesignButton {
                        text: sourceRow.s.enabled ? qsTr("Turn Off") : qsTr("Turn On")
                        onClicked: Iptv.setSourceEnabled(sourceRow.s.id, !sourceRow.s.enabled)
                    }
                    Widgets.RedesignButton {
                        text: qsTr("Update")
                        enabled: !sourceRow.s.busy && sourceRow.s.enabled
                        onClicked: Iptv.refresh(sourceRow.s.id)
                    }
                    Widgets.RedesignButton {
                        text: qsTr("Edit")
                        onClicked: root.edit(sourceRow.s.id)
                    }
                    Widgets.RedesignButton {
                        text: qsTr("Remove")
                        onClicked: Iptv.removeSource(sourceRow.s.id)
                    }
                }
            }
        }

        Row {
            spacing: root.px(12)
            Widgets.RedesignButton {
                text: qsTr("Add a Link")
                accent: true
                onClicked: { root._clear(); root.page = "link" }
            }
            Widgets.RedesignButton {
                text: qsTr("Update All")
                enabled: Iptv.sources.length > 0 && !Iptv.busy
                onClicked: Iptv.refresh("")
            }
        }
    }

    // ---- a link of the user's ----
    ColumnLayout {
        Layout.fillWidth: true
        visible: root.page === "link"
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        spacing: VLCStyle.dp(14, VLCStyle.scale)

        SettingsSegmented {
            visible: root.editing === ""
            Layout.preferredWidth: Math.max(VLCStyle.dp(480, VLCStyle.scale), neededWidth)
            options: [{ text: qsTr("Playlist (M3U)"), value: "m3u" },
                      { text: qsTr("Xtream Codes"), value: "xtream" },
                      { text: qsTr("Guide (XMLTV)"), value: "epg" }]
            value: root.kind
            onChosen: (value) => root.kind = value
        }

        Widgets.RedesignDialogHeading {
            visible: root.kind !== "epg"
            text: qsTr("Name")
            suffix: qsTr("(optional)")
        }
        Widgets.RedesignTextArea {
            id: nameField
            visible: root.kind !== "epg"
            placeholderText: qsTr("e.g., My TV")
            Accessible.name: qsTr("Name")
            onAccepted: root.accept()
        }

        Widgets.RedesignDialogHeading {
            visible: root.kind === "m3u"
            text: qsTr("Playlist link or file")
            description: qsTr("The M3U link your provider gave you; its guide is found in it when it names one.")
        }
        RowLayout {
            visible: root.kind === "m3u"
            Layout.fillWidth: true
            spacing: root.px(10)
            Widgets.RedesignTextArea {
                id: urlField
                placeholderText: qsTr("e.g., https://example.com/playlist.m3u")
                Accessible.name: qsTr("Playlist link or file")
                onAccepted: root.accept()
            }
            Widgets.RedesignButton {
                text: qsTr("Browse")
                onClicked: {
                    const file = filePicker.chooseOpenFile(qsTr("Choose a playlist"),
                                                           qsTr("Playlists (*.m3u *.m3u8)"))
                    if (file.length)
                        urlField.text = file
                }
            }
        }

        GridLayout {
            visible: root.kind === "xtream"
            Layout.fillWidth: true
            columns: 2
            columnSpacing: root.px(16)
            rowSpacing: root.px(10)

            Widgets.RedesignDialogHeading {
                Layout.columnSpan: 2
                text: qsTr("Server")
                description: qsTr("The address of the provider's panel, with its port (http://host:8080).")
            }
            Widgets.RedesignTextArea {
                id: serverField
                Layout.columnSpan: 2
                placeholderText: qsTr("e.g., http://example.com:8080")
                Accessible.name: qsTr("Server")
                onAccepted: root.accept()
            }
            Widgets.RedesignDialogHeading { text: qsTr("User name") }
            Widgets.RedesignDialogHeading {
                text: qsTr("Password")
                suffix: root.editing !== "" ? qsTr("(empty: unchanged)") : ""
            }
            Widgets.RedesignTextArea {
                id: userField
                Accessible.name: qsTr("User name")
                onAccepted: root.accept()
            }
            PasswordField {
                id: passwordField
                Accessible.name: qsTr("Password")
            }
        }

        // the source a guide goes with
        ColumnLayout {
            visible: root.kind === "epg"
            Layout.fillWidth: true
            spacing: root.px(8)
            Widgets.RedesignDialogHeading {
                text: qsTr("For the source")
                description: Iptv.sources.length > 0 ? qsTr("Its channels are matched by their guide id, then by their name.")
                                                     : qsTr("Add a source first.")
            }
            SettingsSegmented {
                id: epgTarget
                visible: Iptv.sources.length > 0
                Layout.preferredWidth: Math.min(root.availableWidth, Math.max(VLCStyle.dp(320, VLCStyle.scale), neededWidth))
                options: Iptv.sources.map(s => ({ text: s.name, value: s.id }))
                value: Iptv.sources.length > 0 ? Iptv.sources[0].id : ""
                onChosen: (v) => value = v
            }
        }

        Widgets.RedesignDialogHeading {
            text: root.kind === "epg" ? qsTr("Guide links") : qsTr("Guide links")
            suffix: root.kind === "epg" ? "" : qsTr("(optional)")
            description: qsTr("XMLTV guides (.xml or .xml.gz), one per line; several are merged, the first first.")
        }
        Widgets.RedesignTextArea {
            id: guidesField
            multiLine: true
            Layout.preferredHeight: VLCStyle.dp(90, VLCStyle.scale)
            implicitHeight: VLCStyle.dp(90, VLCStyle.scale)
            placeholderText: qsTr("e.g., https://example.com/guide.xml.gz")
            Accessible.name: qsTr("Guide links")
        }

        Text {
            Layout.fillWidth: true
            text: root._error !== "" ? root._error
                  : qsTr("Links and passwords are kept in the keystore of VLC, never written to its log.")
            wrapMode: Text.WordWrap
            color: root._error !== "" ? root.c("#e15033", "#b03a22") : root.c("#404040", "#7c7c7b")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(12)
        }
    }

    // ---- the catalog ----
    ColumnLayout {
        id: catalogPage
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: root.page === "catalog"
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        spacing: VLCStyle.dp(12, VLCStyle.scale)

        readonly property var all: root._catalog ? (root._catalog.sources || []) : []
        property string language: ""
        property string country: ""
        property string category: ""
        property string capability: ""
        property string sort: "name"
        readonly property var languages: [...new Set(all.flatMap(s => s.languages || []))].sort()
        readonly property var countries: [...new Set(all.flatMap(s => s.countries || []))].sort()
        readonly property var categories: [...new Set(all.flatMap(s => s.categories || []))].sort()
        readonly property var added: Iptv.sources.map(s => s.catalogId).filter(id => !!id)
        readonly property var shown: {
            const text = catalogSearch.text.trim().toLowerCase()
            const list = all.filter(s => s.kind !== "epg" || true).filter(s =>
                (language === "" || (s.languages || []).indexOf(language) >= 0)
                && (country === "" || (s.countries || []).indexOf(country) >= 0)
                && (category === "" || (s.categories || []).indexOf(category) >= 0)
                && (capability === "" || (capability === "epg" ? (s.epg || []).length > 0
                                         : capability === "catchup" ? !!(s.catchup && s.catchup.supported)
                                         : capability === "vod" ? !!(s.vod || s.series)
                                         : capability === "free" ? s.price === "free" : true))
                && (text === "" || (s.name + " " + (s.description || "")).toLowerCase().indexOf(text) >= 0))
            return list.sort((a, b) => sort === "channels" ? (b.channels || 0) - (a.channels || 0)
                                                           : String(a.name).localeCompare(String(b.name)))
        }

        // the filters: a search, and choices that cycle through the values
        component FilterButton: Widgets.RedesignButton {
            property string label
            property var values: []
            property string current: ""
            signal picked(string value)
            text: current === "" ? label : label + ": " + current.toUpperCase()
            onClicked: {
                const i = values.indexOf(current)
                picked(i + 1 < values.length ? values[i + 1] : "")
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: root.px(10)
            Widgets.RedesignTextArea {
                id: catalogSearch
                Layout.preferredWidth: root.px(240)
                Layout.fillWidth: false
                placeholderText: qsTr("Search the catalog")
                Accessible.name: qsTr("Search the catalog")
            }
            FilterButton {
                label: qsTr("Language")
                values: catalogPage.languages
                current: catalogPage.language
                onPicked: (v) => catalogPage.language = v
            }
            FilterButton {
                label: qsTr("Country")
                values: catalogPage.countries
                current: catalogPage.country
                onPicked: (v) => catalogPage.country = v
            }
            FilterButton {
                label: qsTr("Category")
                values: catalogPage.categories
                current: catalogPage.category
                text: current === "" ? label : label + ": " + current
                onPicked: (v) => catalogPage.category = v
            }
            SettingsSegmented {
                Layout.preferredWidth: Math.max(VLCStyle.dp(300, VLCStyle.scale), neededWidth)
                options: [{ text: qsTr("All"), value: "" }, { text: qsTr("Guide"), value: "epg" },
                          { text: qsTr("Catch-up"), value: "catchup" }, { text: qsTr("Films"), value: "vod" },
                          { text: qsTr("Free"), value: "free" }]
                value: catalogPage.capability
                onChosen: (v) => catalogPage.capability = v
            }
            Item { Layout.fillWidth: true }
            Widgets.RedesignButton {
                text: catalogPage.sort === "name" ? qsTr("A-Z") : qsTr("Most channels")
                onClicked: catalogPage.sort = catalogPage.sort === "name" ? "channels" : "name"
            }
        }

        GridView {
            id: cards
            Layout.preferredHeight: root.bodyHeight - VLCStyle.dp(120, VLCStyle.scale)
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: catalogPage.shown
            readonly property int columns: Math.max(1, Math.floor(width / root.px(330)))
            cellWidth: Math.floor(width / columns)
            cellHeight: root.px(150)
            boundsBehavior: Flickable.StopAtBounds
            T.ScrollBar.vertical: Widgets.ScrollBarExt { }

            delegate: Item {
                id: card
                required property var modelData
                readonly property bool isAdded: catalogPage.added.indexOf(modelData.id) >= 0
                width: cards.cellWidth
                height: cards.cellHeight

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: root.px(5)
                    radius: root.px(12)
                    color: root.c("#151515", "#b5b5b4")

                    TvLogo {
                        id: cardLogo
                        x: root.px(14)
                        y: root.px(14)
                        width: root.px(44)
                        height: width
                        source: card.modelData.logo ? "image://iptvlogo/" + encodeURIComponent(card.modelData.logo) : ""
                        name: card.modelData.name
                    }
                    Column {
                        anchors.left: cardLogo.right
                        anchors.leftMargin: root.px(12)
                        anchors.right: parent.right
                        anchors.rightMargin: root.px(14)
                        y: root.px(14)
                        spacing: root.px(4)
                        Text {
                            width: parent.width
                            text: card.modelData.name
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignLeft
                            color: root.c("#C6C6C6", "#2e2e2e")
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.px(14)
                            font.weight: Font.Medium
                        }
                        Text {
                            width: parent.width
                            text: card.modelData.description || ""
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignLeft
                            color: root.c("#6B6B6B", "#4d4d4d")
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.px(12)
                        }
                    }
                    Flow {
                        anchors.left: parent.left
                        anchors.leftMargin: root.px(14)
                        anchors.right: addButton.left
                        anchors.rightMargin: root.px(8)
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: root.px(14)
                        spacing: root.px(4)
                        TvBadge {
                            visible: (card.modelData.channels || 0) > 0
                            text: qsTr("%1 channel", "%1 channels", card.modelData.channels || 0).arg(card.modelData.channels || 0)
                        }
                        TvBadge { visible: (card.modelData.epg || []).length > 0; icon: "lists"; text: qsTr("Guide") }
                        TvBadge {
                            visible: !!(card.modelData.catchup && card.modelData.catchup.supported)
                            icon: "recent"
                            text: qsTr("%1 d", "%1 d", card.modelData.catchup ? card.modelData.catchup.days || 0 : 0)
                                     .arg(card.modelData.catchup ? card.modelData.catchup.days || 0 : 0)
                        }
                        TvBadge { visible: !!card.modelData.vod; icon: "video"; text: qsTr("Films") }
                        TvBadge { visible: !!card.modelData.series; icon: "playlist"; text: qsTr("Series") }
                        TvBadge {
                            text: card.modelData.price === "free" ? qsTr("Free") : qsTr("Paid")
                            accent: card.modelData.price === "free"
                        }
                        Repeater {
                            model: (card.modelData.languages || []).slice(0, 3)
                            TvBadge { required property var modelData; text: String(modelData).toUpperCase() }
                        }
                        Repeater {
                            model: (card.modelData.countries || []).slice(0, 2)
                            TvBadge { required property var modelData; text: String(modelData) }
                        }
                    }
                    Widgets.RedesignButton {
                        id: addButton
                        anchors.right: parent.right
                        anchors.rightMargin: root.px(14)
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: root.px(10)
                        implicitWidth: root.px(80)
                        text: card.isAdded ? qsTr("Added") : qsTr("Add")
                        accent: !card.isAdded
                        enabled: !card.isAdded && card.modelData.kind !== "epg"
                        onClicked: Iptv.addSource({ kind: card.modelData.kind === "xtream" ? "xtream" : "m3u",
                                                    name: card.modelData.name, url: card.modelData.url,
                                                    epg: card.modelData.epg || [], catalogId: card.modelData.id })
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                width: Math.min(implicitWidth, parent.width - root.px(40))
                visible: cards.count === 0
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                text: root._catalog === null ? qsTr("Loading the catalog…")
                      : catalogPage.all.length === 0 ? qsTr("The catalog is empty in this version.")
                      : qsTr("Nothing in the catalog matches")
                color: root.c("#6B6B6B", "#4d4d4d")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.redesign.fontMedium
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: root.px(12)
            Text {
                Layout.fillWidth: true
                text: qsTr("Only free, legal sources are listed. Suggestions are reviewed before they reach the catalog.")
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                color: root.c("#404040", "#7c7c7b")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(12)
            }
            Widgets.RedesignButton {
                text: qsTr("Suggest a Service")
                onClicked: Qt.openUrlExternally(Iptv.suggestionUrl())
            }
        }
    }
}
