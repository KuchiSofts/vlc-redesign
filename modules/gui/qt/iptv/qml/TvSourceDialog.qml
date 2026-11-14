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
// or file, behind a login or not, an Xtream Codes login, a guide); the
// user's own servers and tuners (Accounts: Jellyfin, Emby, Tvheadend,
// NextPVR, Channels DVR, HDHomeRun, with the fields each takes, Test or Sign
// in, and those found on the network); the guides of every source (Guides);
// or one of the catalog of free, legal channel lists, added in one click
// with its guide, the ones of the user's country and languages first.
Widgets.RedesignDialog {
    id: root

    objectName: "tvSourceDialog"

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    // (the light theme from the map of the design: VLCStyle.redesign.color)
    function c(dark, light) { return VLCStyle.redesign.color(dark, undefined, root.darkTheme) }

    // "sources", "link", "accounts", "guides" or "catalog"; "channel": the
    // guide id of a channel (from its menu)
    property string page: "sources"
    property string _channelKey: ""
    property var _channelGuide: null
    // the accounts page: the kind of server, what Test answered, what the
    // network has
    property string serverKind: "jellyfin"
    property string _test: ""
    property string _testWho: ""
    property bool _testing: false
    property var _found: null
    property bool _looking: false
    // the servers and tuners of the user: the fields each takes, the port
    // its address has when none is typed
    readonly property var accountKinds: [
        { kind: "jellyfin", name: "Jellyfin", port: 8096, fields: ["server", "username", "password", "key"],
          detail: qsTr("The Live TV of your Jellyfin server: its channels and guide") },
        { kind: "emby", name: "Emby", port: 8096, fields: ["server", "username", "password", "key"],
          detail: qsTr("The Live TV of your Emby server: its channels and guide") },
        { kind: "tvheadend", name: "Tvheadend", port: 9981, fields: ["server", "username", "password"],
          detail: qsTr("Your TV server: its channels and guide") },
        { kind: "nextpvr", name: "NextPVR", port: 8866, fields: ["server", "pin"],
          detail: qsTr("Your TV server: its channels and guide") },
        { kind: "channels", name: "Channels DVR", port: 8089, fields: ["server"],
          detail: qsTr("Your TV server on your network: its channels and guide") },
        { kind: "hdhomerun", name: "HDHomeRun", port: 80, fields: ["server"],
          detail: qsTr("A tuner of your network: its channels and free guide") },
        { kind: "schedulesdirect", name: "Schedules Direct", port: 0, fields: ["username", "password"],
          required: ["username", "password"],
          detail: qsTr("A paid guide (the US, Canada and more) for the channels of every source: the lineups of your account") }
    ]
    function _has(field) { return _account.fields.indexOf(field) >= 0 }
    // the programmes of a guide alone (Schedules Direct)
    function _programmesOf(source) {
        return (source.guides || []).reduce((n, g) => n + (g.programmes || 0), 0)
    }
    readonly property var _account: accountKinds.find(k => k.kind === serverKind) || accountKinds[0]
    function _isServer(kind) { return accountKinds.some(k => k.kind === kind) }
    function _kindName(kind) {
        const k = accountKinds.find(a => a.kind === kind)
        return k ? k.name : kind === "xtream" ? "Xtream Codes" : "M3U"
    }
    function _accountSource() {
        return { kind: serverKind, name: accName.text.trim(), server: accServer.text.trim(),
                 username: accUser.text.trim(), password: accPassword.text, key: accKey.text.trim() }
    }
    function look() {
        _looking = true
        Iptv.discover((found) => { root._found = found; root._looking = false })
    }
    // the link page: "m3u", "xtream" or "epg"; the source edited ("": a new one)
    property string kind: "m3u"
    property string editing: ""
    property string _error: ""
    property bool _saving: false
    property var _catalog: null

    // the guide id of a channel: the user's wins over the one of its list
    // and the one the automatic guide found
    function editChannelGuide(key) {
        _clear()
        _channelKey = key
        _channelGuide = null
        Iptv.channelGuide(key, (guide) => {
            root._channelGuide = guide
            channelGuideField.text = guide.user || ""
        })
        page = "channel"
        open()
    }

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
        m3uUser.text = ""
        m3uPassword.text = ""
        accName.text = ""
        accServer.text = ""
        accUser.text = ""
        accPassword.text = ""
        accKey.text = ""
        channelGuideField.text = ""
        _test = ""
        _testWho = ""
    }

    function edit(id) {
        _clear()
        editing = id
        Iptv.getSource(id, (source) => {
            nameField.text = source.name || ""
            // the password and key stay where they are: an empty field keeps them
            if (root._isServer(source.kind)) {
                root.serverKind = source.kind
                accName.text = source.name || ""
                accServer.text = source.server || ""
                accUser.text = source.username || ""
                root.page = "accounts"
                return
            }
            root.kind = source.kind === "xtream" ? "xtream" : "m3u"
            m3uUser.text = source.kind === "m3u" ? source.username || "" : ""
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
    cancelVisible: page !== "link" && page !== "accounts"
    acceptVisible: page === "link" || page === "accounts" || page === "channel"
    acceptText: editing !== "" || page === "channel" ? qsTr("Save")
                : page === "link" && kind === "epg" ? qsTr("Add Guide") : qsTr("Add Source")
    acceptEnabled: !_saving && (page === "channel" ? _channelGuide !== null
                               : page === "accounts"
                                ? (!_has("server") || accServer.text.trim() !== "")
                                  && (editing !== "" || _account.fields.indexOf("key") < 0
                                      || accUser.text.trim() !== "" || accKey.text.trim() !== "")
                                  && (editing !== "" || (_account.required || []).every(
                                          f => (f === "username" ? accUser.text.trim() : accPassword.text) !== ""))
                               : kind === "m3u" ? urlField.text.trim() !== ""
                               : kind === "xtream" ? serverField.text.trim() !== "" && userField.text.trim() !== ""
                                                     && (editing !== "" || passwordField.text !== "")
                               : guidesField.text.trim() !== "" && epgTarget.value !== "")
    extraText: page === "link" || page === "accounts" ? qsTr("Back") : ""
    onExtraClicked: { _clear(); page = "sources" }
    onClosed: _clear()

    onAccepted: {
        _error = ""
        if (page === "channel") {
            Iptv.setChannelGuide(_channelKey, channelGuideField.text.trim())
            close()
            return
        }
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
        const source = page === "accounts" ? _accountSource()
                       : kind === "xtream"
                       ? { kind: "xtream", name: nameField.text.trim(), server: serverField.text.trim(),
                           username: userField.text.trim(), password: passwordField.text, epg: guides }
                       : { kind: "m3u", name: nameField.text.trim(), url: urlField.text.trim(), epg: guides,
                           username: m3uUser.text.trim(), password: m3uPassword.text }
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
        // (the network looked at once, when the accounts first show)
        if (page === "accounts" && _found === null && !_looking)
            look()
    }

    headerContent: [
        RowLayout {
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            Layout.fillWidth: true
            spacing: VLCStyle.dp(10, VLCStyle.scale)

            SettingsSegmented {
                visible: root.page !== "channel"
                Layout.preferredWidth: Math.max(VLCStyle.dp(420, VLCStyle.scale), neededWidth)
                options: [{ text: qsTr("Your Sources (%1)").arg(Iptv.sources.length), value: "sources" },
                          { text: qsTr("Add a Link"), value: "link" },
                          { text: qsTr("Accounts"), value: "accounts" },
                          { text: qsTr("Guides"), value: "guides" },
                          { text: qsTr("Catalog"), value: "catalog" }]
                value: root.page
                onChosen: (value) => {
                    if (value === "link" || value === "accounts")
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
                            textFormat: Text.PlainText
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
                            textFormat: Text.PlainText
                            text: root._kindName(sourceRow.s.kind) + " · " + sourceRow.s.host
                            color: root.c("#6B6B6B", "#4d4d4d")
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.px(12)
                        }
                    }
                    // what the source can do, detected from its channels
                    Row {
                        spacing: root.px(6)
                        TvBadge {
                            text: sourceRow.s.kind === "schedulesdirect"
                                  ? qsTr("%1 programme", "%1 programmes", root._programmesOf(sourceRow.s))
                                        .arg(root._programmesOf(sourceRow.s))
                                  : qsTr("%1 channel", "%1 channels", sourceRow.s.channels).arg(sourceRow.s.channels)
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
                        textFormat: Text.PlainText
                        text: sourceRow.s.busy ? (sourceRow.s.stage === "guide" ? qsTr("Updating the guide… %1 MB")
                                                                                     .arg((sourceRow.s.received / 1048576).toFixed(1))
                                                                                : qsTr("Updating the channels…"))
                              : sourceRow.s.refused ? qsTr("Sign-in refused: check it")
                              : sourceRow.s.error ? Iptv.errorText(sourceRow.s.error, true)
                              : sourceRow.s.updated > 0 ? qsTr("Updated %1 at %2").arg(Iptv.dayText(sourceRow.s.updated))
                                                                             .arg(Iptv.timeText(sourceRow.s.updated))
                              : ""
                        color: (sourceRow.s.error || sourceRow.s.refused) && !sourceRow.s.busy ? root.c("#e15033", "#b03a22")
                                                                                              : root.c("#6B6B6B", "#4d4d4d")
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

        // a playlist behind a login (its server asks who: Basic or Digest)
        GridLayout {
            visible: root.kind === "m3u"
            Layout.fillWidth: true
            columns: 2
            columnSpacing: root.px(16)
            rowSpacing: root.px(10)
            Widgets.RedesignDialogHeading { text: qsTr("User name (optional)") }
            Widgets.RedesignDialogHeading {
                text: qsTr("Password (optional)")
                suffix: root.editing !== "" ? qsTr("(empty: unchanged)") : ""
            }
            Widgets.RedesignTextArea {
                id: m3uUser
                objectName: "tvSourceM3uUser"
                Accessible.name: qsTr("User name (optional)")
                onAccepted: root.accept()
            }
            PasswordField {
                id: m3uPassword
                objectName: "tvSourceM3uPassword"
                Accessible.name: qsTr("Password (optional)")
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

    // ---- the user's servers and tuners ----
    ColumnLayout {
        id: accountsPage
        Layout.fillWidth: true
        visible: root.page === "accounts"
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        spacing: VLCStyle.dp(12, VLCStyle.scale)

        // those found on the network, offered first
        Widgets.RedesignDialogHeading {
            visible: root.editing === ""
            text: qsTr("Found on your network")
            description: root._looking ? qsTr("Looking on your network…")
                         : root._found !== null && root._found.length === 0
                           ? qsTr("Nothing found on your network: type the server's address.") : ""
        }
        Flow {
            visible: root.editing === ""
            Layout.fillWidth: true
            spacing: root.px(8)
            Repeater {
                model: root._found || []
                delegate: Widgets.RedesignButton {
                    required property var modelData
                    objectName: "tvSourceFound_" + modelData.kind
                    text: modelData.name + " · " + root._kindName(modelData.kind)
                          + (modelData.added ? " · " + qsTr("Added") : "")
                    enabled: !modelData.added
                    onClicked: {
                        root._clear()
                        root.serverKind = modelData.kind
                        accServer.text = modelData.address
                        accName.text = modelData.name
                    }
                }
            }
            Widgets.RedesignButton {
                objectName: "tvSourceLookAgain"
                text: qsTr("Look again")
                enabled: !root._looking
                onClicked: root.look()
            }
        }

        // the kind of server, then the fields it takes
        SettingsSegmented {
            visible: root.editing === ""
            Layout.preferredWidth: Math.min(accountsPage.width, Math.max(VLCStyle.dp(480, VLCStyle.scale), neededWidth))
            options: root.accountKinds.map(k => ({ text: k.name, value: k.kind }))
            value: root.serverKind
            onChosen: (value) => { root.serverKind = value; root._test = "" }
        }
        Text {
            Layout.fillWidth: true
            text: root._account.detail
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            color: root.c("#6B6B6B", "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(13)
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: root.px(16)
            rowSpacing: root.px(10)
            // (two halves, whatever the headings say)
            uniformCellWidths: true

            Widgets.RedesignDialogHeading { visible: root._has("server"); text: qsTr("Server address") }
            Widgets.RedesignDialogHeading {
                Layout.columnSpan: root._has("server") ? 1 : 2
                text: qsTr("Name")
                suffix: qsTr("(optional)")
            }
            Widgets.RedesignTextArea {
                id: accServer
                objectName: "tvSourceServerAddress"
                visible: root._has("server")
                placeholderText: qsTr("e.g., %1").arg("http://192.168.1.10:" + root._account.port)
                Accessible.name: qsTr("Server address")
                onAccepted: root.accept()
            }
            Widgets.RedesignTextArea {
                id: accName
                objectName: "tvSourceServerName"
                Layout.columnSpan: root._has("server") ? 1 : 2
                placeholderText: root._account.name
                Accessible.name: qsTr("Name")
                onAccepted: root.accept()
            }

            Widgets.RedesignDialogHeading {
                visible: root._account.fields.indexOf("username") >= 0
                text: root.serverKind === "tvheadend" ? qsTr("User name (optional)") : qsTr("User name")
            }
            Widgets.RedesignDialogHeading {
                visible: root._account.fields.indexOf("password") >= 0 || root._account.fields.indexOf("pin") >= 0
                text: root._account.fields.indexOf("pin") >= 0 ? qsTr("PIN") : qsTr("Password")
                suffix: root.editing !== "" ? qsTr("(empty: unchanged)")
                        : root._account.fields.indexOf("pin") >= 0 ? qsTr("(optional)") : ""
            }
            Widgets.RedesignTextArea {
                id: accUser
                objectName: "tvSourceServerUser"
                visible: root._account.fields.indexOf("username") >= 0
                Accessible.name: qsTr("User name")
                onAccepted: root.accept()
                onTextChanged: root._test = ""
            }
            PasswordField {
                id: accPassword
                objectName: "tvSourceServerPassword"
                visible: root._account.fields.indexOf("password") >= 0 || root._account.fields.indexOf("pin") >= 0
                Accessible.name: root._account.fields.indexOf("pin") >= 0 ? qsTr("PIN") : qsTr("Password")
                onTextChanged: root._test = ""
            }

            Widgets.RedesignDialogHeading {
                Layout.columnSpan: 2
                visible: root._account.fields.indexOf("key") >= 0
                text: qsTr("API key")
                suffix: qsTr("(optional)")
                description: qsTr("Instead of a user: a key made in the server's dashboard.")
            }
            PasswordField {
                id: accKey
                objectName: "tvSourceServerKey"
                Layout.columnSpan: 2
                visible: root._account.fields.indexOf("key") >= 0
                Accessible.name: qsTr("API key")
                onTextChanged: root._test = ""
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: root.px(12)
            Widgets.RedesignButton {
                objectName: "tvSourceServerTest"
                text: root._account.fields.indexOf("username") >= 0 && accUser.text.trim() !== ""
                      ? qsTr("Sign in") : qsTr("Test")
                enabled: !root._testing && (root._has("server") ? accServer.text.trim() !== ""
                                                                : accUser.text.trim() !== "")
                onClicked: {
                    root._testing = true
                    root._test = ""
                    Iptv.testSource(root._accountSource(), root.editing, (answer) => {
                        root._testing = false
                        root._test = answer.result || "noAnswer"
                        root._testWho = answer.who || ""
                    })
                }
            }
            Text {
                Layout.fillWidth: true
                objectName: "tvSourceServerTestResult"
                text: root._testing ? "…"
                      : root._test === "signedIn" ? qsTr("Signed in as %1").arg(root._testWho)
                      : root._test === "works" ? (root._testWho !== "" ? root._testWho + " · " : "") + qsTr("Server found")
                      : root._test === "refused" ? (accUser.text.trim() === "" && accKey.text.trim() !== ""
                                                    ? qsTr("Key refused") : qsTr("Wrong user name or password"))
                      : root._test === "noAnswer" ? qsTr("No answer, try again") : ""
                color: root._test === "signedIn" || root._test === "works" ? VLCStyle.redesign.accent
                                                                           : root.c("#e15033", "#b03a22")
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: root.px(13)
            }
            // an account edited: its password and key forgotten, the source off
            Widgets.RedesignButton {
                objectName: "tvSourceSignOut"
                visible: root.editing !== "" && (root._has("password") || root._has("key") || root._has("pin"))
                text: qsTr("Sign Out")
                onClicked: {
                    Iptv.signOut(root.editing)
                    root._clear()
                    root.page = "sources"
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: root._error !== "" ? root._error
                  : qsTr("Passwords and keys are kept in the keystore of VLC, never written to its log.")
            wrapMode: Text.WordWrap
            color: root._error !== "" ? root.c("#e15033", "#b03a22") : root.c("#404040", "#7c7c7b")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(12)
        }
    }

    // ---- the guide id of a channel ----
    ColumnLayout {
        Layout.fillWidth: true
        visible: root.page === "channel"
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        spacing: VLCStyle.dp(12, VLCStyle.scale)

        Widgets.RedesignDialogHeading {
            plain: true
            text: qsTr("The guide of %1").arg(root._channelGuide ? root._channelGuide.name : "")
            description: qsTr("The id the guides give this channel. Yours comes first; leave it empty to use the one of its list or the one found for it.")
        }
        Text {
            Layout.fillWidth: true
            textFormat: Text.PlainText
            text: qsTr("Its list: %1").arg(root._channelGuide && root._channelGuide.own ? root._channelGuide.own : qsTr("none"))
                  + "  ·  " + qsTr("Found for it: %1").arg(root._channelGuide && root._channelGuide.auto
                                                           ? root._channelGuide.auto : qsTr("none"))
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            color: root.c("#6B6B6B", "#4d4d4d")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(12)
        }
        Widgets.RedesignTextArea {
            id: channelGuideField
            objectName: "tvChannelGuideId"
            Layout.fillWidth: true
            placeholderText: qsTr("e.g., %1").arg("BBCOne.uk")
            Accessible.name: qsTr("Guide id")
            onAccepted: root.accept()
        }
    }

    // ---- the guides of every source ----
    ColumnLayout {
        id: guidesPage
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: root.page === "guides"
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        spacing: VLCStyle.dp(12, VLCStyle.scale)

        Widgets.RedesignDialogHeading {
            text: qsTr("The guides of your sources")
            description: qsTr("Each source's own guides and those you added; the channels are matched by their guide id, then by their name.")
        }
        ListView {
            Layout.preferredHeight: root.bodyHeight - VLCStyle.dp(150, VLCStyle.scale)
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: VLCStyle.dp(8, VLCStyle.scale)
            model: Iptv.sources
            boundsBehavior: Flickable.StopAtBounds
            T.ScrollBar.vertical: Widgets.ScrollBarExt { }
            delegate: Rectangle {
                id: guideRow
                required property var modelData
                readonly property var s: modelData
                readonly property var found: modelData.autoGuide || ({})
                width: ListView.view.width
                height: guideColumn.implicitHeight + root.px(24)
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: root.c("#151515", "#b5b5b4")
                opacity: modelData.enabled ? 1 : 0.6
                Column {
                    id: guideColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: root.px(16)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: root.px(8)
                    // the source: how many of its channels have a guide
                    Item {
                        width: parent.width
                        height: Math.max(headText.implicitHeight, headActions.height)
                        Text {
                            id: headText
                            anchors.left: parent.left
                            anchors.right: headActions.left
                            anchors.rightMargin: root.px(12)
                            anchors.verticalCenter: parent.verticalCenter
                            textFormat: Text.PlainText
                            text: guideRow.s.name + "  ·  " + (guideRow.s.kind === "schedulesdirect"
                                      ? qsTr("A guide for the channels of every source")
                                      : qsTr("Guide for %1 of %2 channels").arg(guideRow.s.withGuide).arg(guideRow.s.channels))
                            elide: Text.ElideRight
                            horizontalAlignment: Text.AlignLeft
                            color: root.c("#C6C6C6", "#2e2e2e")
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: root.px(14)
                            font.weight: Font.Medium
                        }
                        Row {
                            id: headActions
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: root.px(8)
                            Widgets.RedesignButton {
                                objectName: "tvGuideFind"
                                // (a server of the user has its own guide)
                                visible: !guideRow.s.isServer && guideRow.s.kind !== "schedulesdirect"
                                text: qsTr("Find a Guide")
                                enabled: !guideRow.s.busy && guideRow.s.enabled && guideRow.s.channels > 0
                                onClicked: Iptv.findGuide(guideRow.s.id)
                            }
                            Widgets.RedesignButton {
                                text: qsTr("Update")
                                enabled: !guideRow.s.busy && guideRow.s.enabled
                                onClicked: Iptv.refresh(guideRow.s.id)
                            }
                            Widgets.RedesignButton {
                                text: qsTr("Edit")
                                onClicked: root.edit(guideRow.s.id)
                            }
                        }
                    }
                    // what the automatic guide matched
                    Text {
                        width: parent.width
                        visible: (guideRow.found.matched || 0) > 0
                        text: guideRow.s.busy && guideRow.s.stage === "guide"
                              ? qsTr("Updating the guide… %1 MB").arg((guideRow.s.received / 1048576).toFixed(1))
                              : qsTr("Found for you: %1 channel matched by its name or id",
                                     "Found for you: %1 channels matched by their name or id",
                                     guideRow.found.matched || 0).arg(guideRow.found.matched || 0)
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        color: root.c("#6B6B6B", "#4d4d4d")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(12)
                    }
                    // each guide: its state, where it comes from; on/off, Remove
                    Repeater {
                        model: guideRow.s.guides || []
                        delegate: Item {
                            id: guideLine
                            required property var modelData
                            width: guideColumn.width
                            height: Math.max(lineText.implicitHeight, lineActions.height)
                            Text {
                                id: lineText
                                anchors.left: parent.left
                                anchors.right: lineActions.left
                                anchors.rightMargin: root.px(12)
                                anchors.verticalCenter: parent.verticalCenter
                                textFormat: Text.PlainText
                                text: guideLine.modelData.host + "  ·  "
                                      + (!guideLine.modelData.enabled ? qsTr("Off")
                                         : guideLine.modelData.error ? Iptv.errorText(guideLine.modelData.error)
                                         : qsTr("%1 programme", "%1 programmes", guideLine.modelData.programmes)
                                               .arg(guideLine.modelData.programmes))
                                      + "  ·  " + (guideLine.modelData.own ? qsTr("Added by you")
                                                   : guideLine.modelData.auto ? qsTr("Found for you")
                                                   : qsTr("From the playlist"))
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignLeft
                                opacity: guideLine.modelData.enabled ? 1 : 0.6
                                color: guideLine.modelData.error && guideLine.modelData.enabled
                                       ? root.c("#e15033", "#b03a22") : root.c("#6B6B6B", "#4d4d4d")
                                font.family: VLCStyle.redesign.fontFamily
                                font.pixelSize: root.px(12)
                            }
                            Row {
                                id: lineActions
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: root.px(8)
                                Widgets.RedesignButton {
                                    text: guideLine.modelData.enabled ? qsTr("Turn Off") : qsTr("Turn On")
                                    onClicked: Iptv.setGuideEnabled(guideRow.s.id, guideLine.modelData.id,
                                                                    !guideLine.modelData.enabled)
                                }
                                Widgets.RedesignButton {
                                    visible: guideLine.modelData.own || guideLine.modelData.auto
                                    text: qsTr("Remove")
                                    onClicked: Iptv.removeGuide(guideRow.s.id, guideLine.modelData.id)
                                }
                            }
                        }
                    }
                    Text {
                        visible: (guideRow.s.guides || []).length === 0
                        text: qsTr("No guide yet: add one, or one of the catalog's.")
                        color: root.c("#6B6B6B", "#4d4d4d")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(12)
                    }
                }
            }
        }
        Row {
            spacing: root.px(12)
            Widgets.RedesignButton {
                text: qsTr("Add Guide")
                accent: true
                enabled: Iptv.sources.length > 0
                onClicked: { root._clear(); root.kind = "epg"; root.page = "link" }
            }
            Widgets.RedesignButton {
                text: qsTr("Catalog")
                onClicked: root.page = "catalog"
            }
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
        // the user's first: of their country, in their languages
        property string sort: "foryou"
        readonly property string myCountry: (Qt.locale().name.split("_")[1] || "").toUpperCase()
        readonly property var myLanguages: [String(Qt.uiLanguage).split(/[_-]/)[0], Qt.locale().name.split("_")[0]]
                                           .filter(l => l.length > 0)
        function forYou(s) {
            const country = (s.countries || []).indexOf(myCountry) >= 0
            const language = (s.languages || []).some(l => myLanguages.indexOf(l) >= 0)
            // (one that stopped answering last, whatever it is)
            return (country ? 2 : 0) + (language ? 1 : 0) - (s.status === "not answering" ? 10 : 0)
        }
        function recommended(s) {
            return s.status !== "not answering" && ((s.countries || []).indexOf(myCountry) >= 0
                   || (String(s.id).startsWith("iptv-org") && (s.languages || []).some(l => myLanguages.indexOf(l) >= 0)))
        }
        // the values of a field of the sources, once each
        function distinct(field) {
            const seen = {}
            for (const s of all)
                for (const v of (s[field] || []))
                    seen[v] = true
            return Object.keys(seen).sort()
        }
        readonly property var languages: distinct("languages")
        readonly property var countries: distinct("countries")
        readonly property var categories: distinct("categories")
        readonly property var added: Iptv.sources.map(s => s.catalogId).filter(id => !!id)
        // the guides of the catalog already given to a source
        readonly property var addedGuides: Iptv.sources.reduce((all, s) => all.concat(s.epg || []), [])
        readonly property var shown: {
            const text = catalogSearch.text.trim().toLowerCase()
            // the guides apart (Guides), the playlists otherwise
            const list = all.filter(s => capability === "guides" ? s.kind === "epg" : s.kind !== "epg").filter(s =>
                (language === "" || (s.languages || []).indexOf(language) >= 0)
                && (country === "" || (s.countries || []).indexOf(country) >= 0)
                && (category === "" || (s.categories || []).indexOf(category) >= 0)
                && (capability === "" || (capability === "recommended" ? recommended(s)
                                         : capability === "epg" ? (s.epg || []).length > 0
                                         : capability === "catchup" ? !!(s.catchup && s.catchup.supported)
                                         : capability === "vod" ? !!(s.vod || s.series)
                                         : capability === "free" ? s.price === "free" : true))
                && (text === "" || (s.name + " " + (s.description || "")).toLowerCase().indexOf(text) >= 0))
            return list.sort((a, b) => sort === "channels" ? (b.channels || 0) - (a.channels || 0)
                                       : sort === "foryou" ? (forYou(b) - forYou(a)) || (b.channels || 0) - (a.channels || 0)
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
                Layout.minimumWidth: root.px(140)
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
            Item { Layout.fillWidth: true }
            Widgets.RedesignButton {
                text: catalogPage.sort === "name" ? qsTr("A-Z") : catalogPage.sort === "channels" ? qsTr("Most channels")
                                                                                                   : qsTr("For you")
                onClicked: catalogPage.sort = catalogPage.sort === "foryou" ? "channels"
                                              : catalogPage.sort === "channels" ? "name" : "foryou"
            }
        }
        // what the sources can do (a row of its own: the dialog's width)
        SettingsSegmented {
            Layout.preferredWidth: Math.min(catalogPage.width, Math.max(VLCStyle.dp(480, VLCStyle.scale), neededWidth))
            options: [{ text: qsTr("All"), value: "" }, { text: qsTr("Recommended"), value: "recommended" },
                      { text: qsTr("Guide"), value: "epg" },
                      { text: qsTr("Catch-up"), value: "catchup" }, { text: qsTr("Films"), value: "vod" },
                      { text: qsTr("Free"), value: "free" }, { text: qsTr("Guides"), value: "guides" }]
            value: catalogPage.capability
            onChosen: (v) => catalogPage.capability = v
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
                readonly property bool isGuide: modelData.kind === "epg"
                readonly property bool isAdded: isGuide ? catalogPage.addedGuides.indexOf(modelData.url) >= 0
                                                        : catalogPage.added.indexOf(modelData.id) >= 0
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
                            textFormat: Text.PlainText
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
                            textFormat: Text.PlainText
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
                        // what the catalog's last check found (the copy of the repository)
                        TvBadge {
                            visible: card.modelData.status === "not answering"
                            text: qsTr("Not answering")
                        }
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
                        implicitWidth: Math.max(root.px(80), implicitContentWidth + root.px(32))
                        text: card.isAdded ? qsTr("Added") : card.isGuide ? qsTr("Add Guide") : qsTr("Add")
                        accent: !card.isAdded
                        // a guide goes with the first source (the guides serve every source)
                        enabled: !card.isAdded && (!card.isGuide || Iptv.sources.length > 0)
                        onClicked: {
                            if (card.isGuide) {
                                const target = Iptv.sources[0]
                                Iptv.getSource(target.id, (source) => {
                                    Iptv.updateSource(target.id, Object.assign(source, {
                                        epg: (source.epg || []).concat([card.modelData.url]), password: "" }))
                                })
                                return
                            }
                            Iptv.addSource({ kind: card.modelData.kind === "xtream" ? "xtream" : "m3u",
                                            name: card.modelData.name, url: card.modelData.url,
                                            epg: card.modelData.epg || [], catalogId: card.modelData.id })
                        }
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
                // (its room is what the button leaves: wrapped, not wider)
                Layout.preferredWidth: 0
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
