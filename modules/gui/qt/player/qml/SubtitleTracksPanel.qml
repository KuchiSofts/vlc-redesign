/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.Basic as Controls

import VLC.Dialogs
import VLC.MainInterface
import VLC.Player
import VLC.PlayerControls
import VLC.Style
import VLC.Util

// The subtitles of the media: the languages on the left (the user's first,
// with how many subtitles each has), the subtitles of the chosen one on the
// right: the media's own tracks, then those found online (SubtitleFinder),
// the ones made for this very file first; each says where it comes from (in
// the video, a file next to it, a file opened, the service it was found
// on). A language or a subtitle chosen shows at once.
ColumnLayout {
    id: root
    objectName: "subtitleTracksPanel"
    spacing: VLCStyle.dp(8, VLCStyle.scale)
    property var reveal: null
    readonly property bool localMedia: Player.url.toString().startsWith("file://")
    // a video, here or on a TV (no video output here while casting)
    readonly property bool _video: Player.hasVideoOutput
        || (Player.rendererManager.useRenderer && Player.rendererManager.rendererVideo
            && Player.videoTracks.count > 0)
    readonly property real _rowHeight: VLCStyle.dp(36, VLCStyle.scale)

    // data() calls are not bindings: bumped when the tracks change so the
    // lists below are evaluated again
    property int _tracksRevision: 0
    Connections {
        target: Player.subtitleTracks
        function onDataChanged() { ++root._tracksRevision }
        function onModelReset() { ++root._tracksRevision }
        function onRowsInserted() { ++root._tracksRevision }
        function onRowsRemoved() { ++root._tracksRevision }
    }
    // (where the tracks come from is known a moment after them)
    Connections {
        target: SubtitleFinder
        function onSourcesChanged() { ++root._tracksRevision }
        function onDownloadFailed(error) { root._downloadError = error; downloadErrorTimer.restart() }
    }
    // a subtitle chosen that could not be loaded, said a moment
    property string _downloadError: ""
    Timer {
        id: downloadErrorTimer
        interval: 6000
        onTriggered: root._downloadError = ""
    }

    // Where a track comes from, as said under it
    function _sourceText(source) {
        switch (source ? source.kind : "") {
        case "video": return qsTr("In the video")
        case "file": return source.name ? qsTr("Next to the video · %1").arg(source.name) : qsTr("Next to the video")
        case "online": return source.provider ? qsTr("Downloaded from %1").arg(source.provider) : qsTr("Downloaded")
        default: return source && source.name ? qsTr("Opened · %1").arg(source.name) : qsTr("Opened file")
        }
    }
    function _downloadsText(count) {
        if (!count)
            return ""
        return count >= 1000 ? qsTr("%1k downloads").arg(Math.round(count / 100) / 10) : qsTr("%1 downloads").arg(count)
    }

    // the media's tracks: { row, title, language, external, checked, source }
    readonly property var _localTracks: {
        void root._tracksRevision
        const model = Player.subtitleTracks
        const tracks = []
        for (let i = 0; i < model.count; ++i) {
            const index = model.index(i, 0)
            tracks.push({
                row: i,
                title: model.data(index, Qt.DisplayRole),
                external: model.data(index, Qt.UserRole + 1),
                language: model.data(index, Qt.UserRole + 2) || "",
                source: model.data(index, Qt.UserRole + 3) || ({}),
                checked: model.data(index, Qt.CheckStateRole)
            })
        }
        return tracks
    }
    readonly property bool _anyChecked: _localTracks.some(t => t.checked)

    // the languages: { code, name, native, count, wanted }, the user's
    // first, then the media's own, then the others by name
    readonly property var _languages: {
        const counts = {}
        const order = []
        const add = (code) => {
            if (!(code in counts)) { counts[code] = 0; order.push(code) }
            ++counts[code]
        }
        for (const t of _localTracks)
            add(t.language)
        for (const r of SubtitleFinder.results)
            if (!SubtitleFinder.loadedIds.includes(r.id))
                add(r.language)
        const wanted = [SubtitlePreferences.primary, SubtitlePreferences.secondary].filter(c => c !== "")
        const rank = (code) => {
            const w = wanted.indexOf(code)
            if (w >= 0) return w
            if (code === "") return 1000
            return _localTracks.some(t => t.language === code) ? 10 : 20
        }
        const list = order.map(code => ({
            code: code,
            name: code === "" ? qsTr("Unknown language") : SubtitlePreferences.name(code),
            native: code === "" ? "" : SubtitlePreferences.nativeName(code),
            count: counts[code],
            wanted: wanted.indexOf(code)
        }))
        list.sort((a, b) => rank(a.code) - rank(b.code) || a.name.localeCompare(b.name))
        return list
    }

    // the language shown on the right: chosen, else the one of the track
    // shown, else the user's
    property string chosenLanguage: "\u0001"
    readonly property string _shownLanguage: {
        if (chosenLanguage !== "\u0001" && _languages.some(l => l.code === chosenLanguage))
            return chosenLanguage
        const shown = _localTracks.find(t => t.checked)
        if (shown)
            return shown.language
        const wanted = _languages.find(l => l.wanted >= 0)
        return wanted ? wanted.code : (_languages.length ? _languages[0].code : "\u0001")
    }
    Connections {
        target: Player
        function onInputChanged() { root.chosenLanguage = "\u0001"; titleSearch.visible = false }
    }

    // the subtitles of that language: { kind: "track"|"online", ... }
    readonly property var _variants: {
        const list = []
        for (const t of _localTracks)
            if (t.language === _shownLanguage)
                list.push({ kind: "track", row: t.row, title: t.title, checked: t.checked,
                            detail: _sourceText(t.source) })
        for (const r of SubtitleFinder.results)
            if (r.language === _shownLanguage && !SubtitleFinder.loadedIds.includes(r.id))
                list.push({ kind: "online", id: r.id, title: r.release, exact: r.exact,
                            hearingImpaired: r.hearingImpaired,
                            detail: [r.providerName, _downloadsText(r.downloads),
                                     r.foreignOnly ? qsTr("Foreign parts only") : ""].filter(t => t).join(" · ") })
        return list
    }

    function _selectTrack(row) {
        Player.subtitleTracks.setData(Player.subtitleTracks.index(row, 0), true, Qt.CheckStateRole)
    }
    function _turnOff() {
        for (const t of _localTracks)
            if (t.checked)
                Player.subtitleTracks.setData(Player.subtitleTracks.index(t.row, 0), false, Qt.CheckStateRole)
    }
    // a language: its best subtitle, the media's own first
    function _chooseLanguage(code) {
        chosenLanguage = code
        const track = _localTracks.find(t => t.language === code)
        if (track) {
            if (!track.checked)
                _selectTrack(track.row)
            return
        }
        const online = SubtitleFinder.results.find(r => r.language === code && !SubtitleFinder.loadedIds.includes(r.id))
        if (online)
            SubtitleFinder.load(online.id)
    }

    // the search starts when the menu shows, for a video without results
    onVisibleChanged: {
        if (visible && root._video && SubtitleFinder.supported
            && SubtitleFinder.state === SubtitleFinder.Idle)
            SubtitleFinder.search()
    }

    // The dialog of the library, loaded only with it: its types are not
    // registered otherwise and would keep the player from loading. Built
    // the first time it is asked for, not with every menu
    Loader {
        id: associationsDialog
        property bool requested: false
        visible: false
        active: requested && MainCtx.mediaLibraryAvailable
        source: "qrc:///qt/qml/VLC/MediaLibrary/SubtitleAssociationsDialog.qml"
        onLoaded: item.parent = Controls.Overlay.overlay
    }

    // What was found: the video identified, the search going on, or why
    // there is nothing
    RowLayout {
        Layout.fillWidth: true
        spacing: VLCStyle.dp(4, VLCStyle.scale)

        BusyDot {
            visible: SubtitleFinder.state === SubtitleFinder.Identifying
                     || SubtitleFinder.state === SubtitleFinder.Searching
                     || SubtitleFinder.loadingId !== ""
        }
        Text {
            Layout.fillWidth: true
            text: {
                if (root._downloadError !== "")
                    return root._downloadError
                switch (SubtitleFinder.state) {
                case SubtitleFinder.Identifying: return qsTr("Identifying the video…")
                case SubtitleFinder.Searching: {
                    const busy = SubtitleFinder.services.filter(s => s.busy).map(s => s.name)
                    return busy.length ? qsTr("Searching %1…").arg(busy.join(", ")) : qsTr("Searching subtitles online…")
                }
                case SubtitleFinder.Ready:
                    if (SubtitleFinder.loadingId !== "")
                        return qsTr("Downloading…")
                    return SubtitleFinder.matchTitle !== ""
                           ? qsTr("%1 · %2 found").arg(SubtitleFinder.matchTitle).arg(SubtitleFinder.results.length)
                           : qsTr("%1 found for this file").arg(SubtitleFinder.results.length)
                case SubtitleFinder.NotFound:
                case SubtitleFinder.Failed: return SubtitleFinder.errorText
                default:
                    return SubtitleFinder.supported && root._video ? qsTr("Subtitles of this video")
                                                                             : qsTr("Subtitles")
                }
            }
            color: root._downloadError !== "" ? VLCStyle.redesign.accent
                   : SubtitleFinder.state === SubtitleFinder.Failed || SubtitleFinder.state === SubtitleFinder.NotFound
                   ? VLCStyle.redesign.color("#B8B8B8", "#2e2e2e")
                   : VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
            elide: Text.ElideRight
            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }
        HeaderButton {
            objectName: "subtitleTitleSearchButton"
            iconName: "search"
            description: qsTr("The video is another one: search by its title")
            visible: SubtitleFinder.supported && root._video
            onClicked: {
                titleSearch.visible = !titleSearch.visible
                if (titleSearch.visible)
                    titleField.forceActiveFocus()
            }
        }
        HeaderButton {
            objectName: "subtitleRefreshButton"
            iconName: "refresh"
            description: qsTr("Search subtitles online again")
            visible: SubtitleFinder.supported && root._video
            enabled: SubtitleFinder.state !== SubtitleFinder.Identifying
                     && SubtitleFinder.state !== SubtitleFinder.Searching
            onClicked: SubtitleFinder.search()
        }
        HeaderButton {
            objectName: "associateSubtitleButton"
            iconName: "edit"
            description: root.localMedia && MainCtx.mediaLibraryAvailable
                         ? qsTr("Add subtitles to the library")
                         : qsTr("Library association needs a local media file and media library.")
            enabled: root.localMedia && MainCtx.mediaLibraryAvailable
            onClicked: {
                associationsDialog.requested = true
                associationsDialog.item?.openForUrl(Player.url)
            }
        }
        HeaderButton {
            objectName: "addSubtitleButton"
            iconName: "add"
            description: qsTr("Load a subtitle file for playback")
            onClicked: DialogsProvider.loadSubtitlesFile()
        }
    }

    // The title of the video, when the search took it for another one
    ColumnLayout {
        id: titleSearch
        Layout.fillWidth: true
        visible: false
        spacing: VLCStyle.dp(4, VLCStyle.scale)

        T.TextField {
            id: titleField
            objectName: "subtitleTitleField"
            Layout.fillWidth: true
            implicitHeight: VLCStyle.dp(36, VLCStyle.scale)
            leftPadding: VLCStyle.dp(12, VLCStyle.scale)
            rightPadding: VLCStyle.dp(12, VLCStyle.scale)
            placeholderText: qsTr("Title of the movie or series")
            placeholderTextColor: VLCStyle.redesign.color("#5a5a5a", "#6b6b6b")
            color: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#1e1e1e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            verticalAlignment: TextInput.AlignVCenter
            selectByMouse: true
            background: Rectangle {
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: VLCStyle.redesign.color("#090909", "#c4c4c3")
                border.width: titleField.activeFocus ? 2 : 1
                border.color: titleField.activeFocus ? VLCStyle.redesign.accent
                                                     : VLCStyle.redesign.color("#191919", "#9c9c9b")
            }
            onAccepted: SubtitleFinder.findTitle(text)
            Keys.onEscapePressed: (event) => { titleSearch.visible = false; event.accepted = true }
        }
        Text {
            Layout.fillWidth: true
            visible: SubtitleFinder.titleBusy
            text: qsTr("Searching…")
            color: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
        }
        Repeater {
            model: SubtitleFinder.titleResults
            delegate: RowButton {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                text: modelData.label + (modelData.year ? " (" + modelData.year + ")" : "")
                detail: modelData.detail
                onClicked: {
                    SubtitleFinder.useTitle(index)
                    titleSearch.visible = false
                }
            }
        }
    }

    // Languages | subtitles
    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: VLCStyle.dp(300, VLCStyle.scale)
        spacing: VLCStyle.dp(8, VLCStyle.scale)

        ListView {
            id: languageList
            objectName: "subtitleLanguageList"
            Layout.preferredWidth: Math.round(root.width * 0.38)
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: [{ code: "\u0002", name: qsTr("Off"), native: "", count: 0, wanted: -1 }].concat(root._languages)
            delegate: RowButton {
                required property var modelData
                width: ListView.view.width
                text: modelData.name
                detail: modelData.native !== "" && modelData.native !== modelData.name ? modelData.native : ""
                count: modelData.count
                highlighted: modelData.code === "\u0002" ? !root._anyChecked : root._shownLanguage === modelData.code
                marker: modelData.wanted === 0 ? qsTr("Default") : modelData.wanted === 1 ? qsTr("Second") : ""
                onClicked: {
                    if (modelData.code === "\u0002") {
                        root._turnOff()
                        root.chosenLanguage = "\u0001"
                    } else {
                        root._chooseLanguage(modelData.code)
                    }
                }
            }
            Controls.ScrollBar.vertical: Controls.ScrollBar { policy: Controls.ScrollBar.AsNeeded }
        }

        Rectangle {
            Layout.fillHeight: true
            implicitWidth: 1
            color: VLCStyle.redesign.color("#222222", "#9c9c9b")
        }

        ListView {
            id: variantList
            objectName: "subtitleVariantList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: root._shownLanguage === "\u0001" ? [] : root._variants
            delegate: RowButton {
                required property var modelData
                width: ListView.view.width
                text: modelData.title
                detail: modelData.detail || ""
                checked: modelData.kind === "track" && modelData.checked
                busy: modelData.kind === "online" && SubtitleFinder.loadingId === modelData.id
                // made for this very file (its hash)
                badge: modelData.kind === "online" && modelData.exact ? qsTr("For this file") : ""
                accentBadge: modelData.kind === "online" && modelData.exact
                hearingImpaired: modelData.kind === "online" && modelData.hearingImpaired
                onClicked: {
                    root.chosenLanguage = root._shownLanguage
                    if (modelData.kind === "track")
                        root._selectTrack(modelData.row)
                    else
                        SubtitleFinder.load(modelData.id)
                }
            }
            Controls.ScrollBar.vertical: Controls.ScrollBar { policy: Controls.ScrollBar.AsNeeded }

            Text {
                anchors.centerIn: parent
                width: parent.width - VLCStyle.dp(24, VLCStyle.scale)
                visible: variantList.count === 0
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: SubtitleFinder.state === SubtitleFinder.Identifying || SubtitleFinder.state === SubtitleFinder.Searching
                      ? qsTr("Looking for subtitles…")
                      : root._anyChecked || root._languages.length ? "" : qsTr("No subtitles yet")
                color: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: VLCStyle.dp(4, VLCStyle.scale)
        implicitHeight: 1
        color: VLCStyle.redesign.color("#222222", "#9c9c9b")
    }

    SubtitleSyncTimeline {
        Layout.fillWidth: true
        presets: SubtitleSeriesPreset
        reveal: root.reveal
    }

    component BusyDot: Item {
        implicitWidth: VLCStyle.dp(16, VLCStyle.scale)
        implicitHeight: implicitWidth
        Rectangle {
            anchors.centerIn: parent
            width: VLCStyle.dp(8, VLCStyle.scale)
            height: width
            radius: width / 2
            color: VLCStyle.redesign.accent
            SequentialAnimation on opacity {
                running: parent.visible
                loops: Animation.Infinite
                NumberAnimation { from: 1; to: 0.25; duration: 600; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 0.25; to: 1; duration: 600; easing.type: Easing.InOutQuad }
            }
        }
    }

    component HeaderButton: T.Button {
        id: headerButton

        property string iconName
        property string description

        implicitWidth: VLCStyle.dp(32, VLCStyle.scale)
        implicitHeight: VLCStyle.dp(32, VLCStyle.scale)
        focusPolicy: Qt.StrongFocus
        Accessible.name: description
        T.ToolTip.visible: hovered && description.length > 0
        T.ToolTip.text: description
        T.ToolTip.delay: VLCStyle.delayToolTipAppear
        onActiveFocusChanged: if (activeFocus && root.reveal) root.reveal(this)

        background: Rectangle {
            radius: VLCStyle.redesign.controlRadius
            color: headerButton.hovered || headerButton.down ? VLCStyle.redesign.color("#222222", "#9c9c9b") : "transparent"
            border.width: headerButton.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.focusColor
        }
        contentItem: Image {
            source: "qrc:///redesign/icons/" + headerButton.iconName + "/default.png"
            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
            fillMode: Image.Pad
            opacity: headerButton.enabled ? 1 : 0.4
        }
    }

    // A row of the lists: a check for the subtitle shown, the name, what it
    // is (a badge), a count for a language
    component RowButton: T.Button {
        id: row

        property string detail
        property int count: 0
        // (highlighted: the language shown, a property of the button)
        property string marker
        property string badge
        property bool accentBadge: false
        property bool hearingImpaired: false
        property bool busy: false

        implicitHeight: root._rowHeight
        focusPolicy: Qt.StrongFocus
        Accessible.name: text + (detail ? ", " + detail : "") + (marker ? ", " + marker : "")
        onActiveFocusChanged: if (activeFocus && root.reveal) root.reveal(this)

        background: Rectangle {
            radius: VLCStyle.dp(4, VLCStyle.scale)
            color: row.highlighted ? VLCStyle.redesign.color("#1b1b1b", "#b4b4b3")
                 : row.hovered || row.activeFocus ? VLCStyle.redesign.color("#151515", "#9c9c9b") : "transparent"
            border.width: row.visualFocus ? VLCStyle.dp(1, VLCStyle.scale) : 0
            border.color: VLCStyle.redesign.focusColor
            Rectangle {
                visible: row.highlighted
                width: VLCStyle.dp(3, VLCStyle.scale)
                height: parent.height - VLCStyle.dp(12, VLCStyle.scale)
                anchors.verticalCenter: parent.verticalCenter
                radius: width / 2
                color: VLCStyle.redesign.accent
            }
        }
        contentItem: RowLayout {
            spacing: VLCStyle.dp(6, VLCStyle.scale)
            Item {
                Layout.preferredWidth: VLCStyle.dp(20, VLCStyle.scale)
                Layout.preferredHeight: VLCStyle.dp(20, VLCStyle.scale)
                Layout.leftMargin: VLCStyle.dp(6, VLCStyle.scale)
                Image {
                    anchors.fill: parent
                    visible: row.checked
                    source: "qrc:///redesign/track-check-active.png"
                }
                BusyDot {
                    anchors.centerIn: parent
                    visible: row.busy
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text {
                    Layout.fillWidth: true
                    text: row.text
                    color: row.highlighted || row.checked
                           ? VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#1e1e1e")
                           : VLCStyle.redesign.color(VLCStyle.redesign.textSecondary, "#2e2e2e")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    visible: row.detail !== "" || row.marker !== ""
                    text: row.marker !== "" ? (row.detail !== "" ? row.detail + " · " + row.marker : row.marker)
                                            : row.detail
                    color: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(11, VLCStyle.scale)
                    elide: Text.ElideRight
                }
            }
            Text {
                visible: row.hearingImpaired
                text: qsTr("HI")
                color: VLCStyle.redesign.color("#8F8F8F", "#4d4d4d")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(11, VLCStyle.scale)
                font.weight: Font.DemiBold
                Accessible.name: qsTr("For the hearing impaired")
            }
            Rectangle {
                visible: row.badge !== ""
                Layout.rightMargin: VLCStyle.dp(4, VLCStyle.scale)
                implicitWidth: badgeText.implicitWidth + VLCStyle.dp(10, VLCStyle.scale)
                implicitHeight: VLCStyle.dp(18, VLCStyle.scale)
                radius: VLCStyle.dp(4, VLCStyle.scale)
                color: row.accentBadge ? Qt.rgba(VLCStyle.redesign.accent.r, VLCStyle.redesign.accent.g,
                                                 VLCStyle.redesign.accent.b, 0.18)
                                       : VLCStyle.redesign.color("#1f1f1f", "#b4b4b3")
                Text {
                    id: badgeText
                    anchors.centerIn: parent
                    text: row.badge
                    color: row.accentBadge ? VLCStyle.redesign.accent : VLCStyle.redesign.color("#8F8F8F", "#4d4d4d")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(11, VLCStyle.scale)
                }
            }
            Text {
                visible: row.count > 0
                Layout.rightMargin: VLCStyle.dp(8, VLCStyle.scale)
                text: row.count
                color: VLCStyle.redesign.color("#6B6B6B", "#5a5a59")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
            }
        }
    }
}
