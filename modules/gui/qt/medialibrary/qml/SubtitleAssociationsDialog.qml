/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl as Impl

import VLC.MainInterface
import VLC.MediaLibrary
import VLC.Style
import VLC.Widgets as Widgets
import VLC.Network as Network

// Figma "Add Subtitles to Playlist/Folder" (1911:7545 a folder, 1919:9278
// OpenSubtitles, and a file): 1048 x 800, "Dark/BG/B1" (#0E0E0E 95 %). The
// title bar holds the target ("To This Folder"), the origin ("From
// Computer") and the folder of the subtitles (or the search and its
// language), 200 / 200 / the rest, 10 apart, 34 high (#151515, radius 6).
// A folder lists its media (30 from the left, rows of 24 every 40) joined
// by a dashed line to the file of the same name (rows of 36, 20 + 8 from
// the middle); a file lists the files named after it, checked ones on
// #090909 with the accent check.
Widgets.RedesignDialog {
    id: root
    objectName: "subtitleAssociationsDialog"

    // what the dialog was opened for: 0 a file, 1 a folder, 2 a playlist
    // (named by a binding: it follows a change of the language)
    property int _targetType: 0
    readonly property string targetKind: _targetType === 1 ? qsTr("To This Folder")
                                       : _targetType === 2 ? qsTr("To This Playlist")
                                       : qsTr("To This File")
    // audio files (the folder menu's "Edit Audio Tracks"): from the computer
    property bool audio: false
    // 0 the computer, 1 a link, 2 OpenSubtitles
    property int originIndex: 0
    readonly property bool online: originIndex === 2
    readonly property bool singleMedia: associations.destinations.length === 1
    // a download is linked to the one media only (match() links files of
    // other names to no media of several): no OpenSubtitles for a folder
    onSingleMediaChanged: if (!singleMedia && originIndex === 2) originIndex = 0
    // "The_Wand_S01E01.mp4" searched as "The Wand S01E01"
    readonly property string mediaQuery: associations.destinations.length === 1
        ? String(associations.destinations[0].label).replace(/\.[^.]*$/, "").replace(/[_.]+/g, " ").trim()
        : ""
    // the result of OpenSubtitles chosen
    property int onlineSelection: -1
    property bool _saveAfterDownload: false

    readonly property bool readyToSave: {
        if (associations.busy || service.busy)
            return false
        if (root.online)
            return onlineSelection >= 0 && root.singleMedia
        const rows = associations.sources
        for (let i = 0; i < rows.length; ++i) {
            if (rows[i].destination >= 0)
                return true
        }
        return false
    }

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.Window
        palette: VLCStyle.palette
    }

    title: root.audio ? qsTr("Add Audio Tracks") : qsTr("Add Subtitles")
    cardColor: VLCStyle.redesign.color("#0F0F0F", "#ABABAA")
    preferredWidth: VLCStyle.dp(1048, VLCStyle.scale)
    bodyHeight: VLCStyle.dp(632, VLCStyle.scale)
    bodyHorizontalPadding: 0
    bodyVerticalPadding: 0
    acceptText: qsTr("Save")
    acceptEnabled: readyToSave
    // the dialog closes once saved
    closeOnAccept: false
    closePolicy: associations.busy ? T.Popup.NoAutoClose : T.Popup.CloseOnEscape

    function openFor(itemId) {
        const serialized = itemId.toString()
        _targetType = serialized.startsWith("VLC_ML_PARENT_FOLDER") ? 1
                    : serialized.startsWith("VLC_ML_PARENT_PLAYLIST") ? 2
                    : 0
        originIndex = 0
        associations.open(itemId)
        open()
    }

    function openForUrl(url) {
        _targetType = 0
        originIndex = 0
        associations.openUrl(url)
        open()
    }

    function openForFolderUrl(url) {
        _targetType = 1
        originIndex = 0
        associations.openFolderUrl(url)
        open()
    }

    onAccepted: {
        if (root.online) {
            root._saveAfterDownload = true
            service.download(root.onlineSelection, apiKeyField.text, usernameField.text, passwordField.text)
        } else {
            associations.save()
        }
    }

    // The OpenSubtitles account stays for the session (in memory, shared by
    // the dialogs; never written to disk)
    onAboutToShow: {
        const account = service.rememberedAccount()
        apiKeyField.text = account.apiKey
        usernameField.text = account.username
        passwordField.text = account.password
    }

    onClosed: {
        associations.cancel()
        service.cancel()
        service.rememberAccount(apiKeyField.text, usernameField.text, passwordField.text)
        onlineSelection = -1
        _saveAfterDownload = false
    }

    MLSubtitleAssociations {
        id: associations
        ml: MediaLib
        audio: root.audio
        onSaved: root.close()
        onChanged: {
            // the file of OpenSubtitles downloaded: saved for the media
            if (root._saveAfterDownload && !busy) {
                root._saveAfterDownload = false
                if (error.length === 0)
                    save()
            }
        }
    }

    MLOpenSubtitles {
        id: service
        onFileReady: (url, filename) => associations.addLinkWithName(url, filename)
        onChanged: if (error.length > 0) root._saveAfterDownload = false
    }

    Connections {
        target: associations
        function onTargetsChanged() { targetPicker.currentIndex = associations.selectedTarget }
    }

    // the address of the folder of the subtitles, as the folder pages show
    // it ("Videos / Shows / The Wand / S01")
    Network.MLFolderModel {
        id: libraryFolders
        ctx: MainCtx
    }
    Instantiator {
        id: libraryFolderUrls
        model: libraryFolders
        delegate: QtObject {
            required property var url
        }
    }
    readonly property string folderPath: {
        const parts = decodeURIComponent(String(associations.folder).replace(/^file:\/\/\/?/, ""))
                          .split("/").filter(part => part.length > 0)
        if (parts.length === 0)
            return ""
        let depth = 0
        for (let i = 0; i < libraryFolderUrls.count; ++i) {
            const entry = libraryFolderUrls.objectAt(i)
            if (!entry)
                continue
            const entryParts = decodeURIComponent(String(entry.url).replace(/^file:\/\/\/?/, ""))
                                   .split("/").filter(part => part.length > 0)
            if (entryParts.length >= parts.length || entryParts.length <= depth)
                continue
            if (entryParts.every((part, j) => part.toLowerCase() === parts[j].toLowerCase()))
                depth = entryParts.length
        }
        const shown = depth > 0 ? [qsTr("Videos")].concat(parts.slice(depth)) : parts.slice(-3)
        // right to left: from the right, the names in Latin letters too (a
        // right-to-left mark at each separator keeps them from reading as
        // one left-to-right run)
        if (Qt.application.layoutDirection === Qt.RightToLeft) {
            const rlm = String.fromCharCode(0x200F)
            return rlm + shown.join(" " + rlm + "/ ")
        }
        return shown.join(" / ")
    }

    readonly property var languages: [
        { label: qsTr("English"), code: "en" }, { label: qsTr("French"), code: "fr" },
        { label: qsTr("German"), code: "de" }, { label: qsTr("Spanish"), code: "es" },
        { label: qsTr("Italian"), code: "it" }, { label: qsTr("Portuguese"), code: "pt" },
        { label: qsTr("Dutch"), code: "nl" }, { label: qsTr("Russian"), code: "ru" },
        { label: qsTr("Japanese"), code: "ja" }, { label: qsTr("Chinese"), code: "zh" },
        { label: qsTr("Korean"), code: "ko" }, { label: qsTr("Arabic"), code: "ar" },
        { label: qsTr("Hebrew"), code: "he" }
    ]

    // ---- components of the design --------------------------------------

    // a line of Inter 14 centred in its box as Figma does
    component CenteredText: Text {
        id: centeredText
        // the box the (first) line is centred in
        property real boxHeight: height
        // from the start of the line (the right in a right to left layout)
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignTop
        elide: Text.ElideRight
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        color: VLCStyle.redesign.color("#969696", "#2e2e2e")
        topPadding: VLCStyle.redesign.textBaseline(Math.round((boxHeight - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                   font.pixelSize) - centeredMetrics.ascent
        FontMetrics {
            id: centeredMetrics
            font: centeredText.font
        }
    }

    // Figma "target" (the dropdowns of the title bar): 200 x 34, #151515,
    // radius 6, padding 5 / 8 / 5 / 12, the text and the chevron
    component HeaderDropdown: T.ComboBox {
        id: dropdown

        property string shownText: displayText

        Layout.preferredWidth: VLCStyle.dp(200, VLCStyle.scale)
        Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
        textRole: "label"
        // (a target chosen while downloading would save, then cancel it)
        enabled: !associations.busy && !service.busy
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        // the chevron at the end: on the left in a right to left layout
        leftPadding: LayoutMirroring.enabled ? VLCStyle.dp(8 + 24, VLCStyle.scale) : VLCStyle.dp(12, VLCStyle.scale)
        rightPadding: LayoutMirroring.enabled ? VLCStyle.dp(12, VLCStyle.scale) : VLCStyle.dp(8 + 24, VLCStyle.scale)

        // a longer text (a translation) elided: in full under the pointer
        T.ToolTip.visible: hovered && !popup.visible && contentItem.truncated
        T.ToolTip.text: shownText
        T.ToolTip.delay: 650

        background: Rectangle {
            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: dropdown.hovered || dropdown.popup.visible ? VLCStyle.redesign.color("#1B1B1B", "#9c9c9b")
                                                              : VLCStyle.redesign.color("#151515", "#b8b8b7")
            border.width: dropdown.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.focusColor
        }
        contentItem: CenteredText {
            text: dropdown.shownText
        }
        indicator: Image {
            x: LayoutMirroring.enabled ? VLCStyle.dp(8, VLCStyle.scale)
                                       : dropdown.width - VLCStyle.dp(8, VLCStyle.scale) - width
            y: Math.round((dropdown.height - height) / 2)
            width: VLCStyle.redesign.iconSize
            height: width
            sourceSize: Qt.size(width, height)
            source: "qrc:///redesign/icons/chevron-down/default.png"
            rotation: dropdown.popup.visible ? 180 : 0
        }
        delegate: T.ItemDelegate {
            required property var modelData
            required property int index

            width: ListView.view ? ListView.view.width : dropdown.width
            implicitHeight: VLCStyle.dp(32, VLCStyle.scale)
            leftPadding: VLCStyle.dp(12, VLCStyle.scale)
            rightPadding: VLCStyle.dp(12, VLCStyle.scale)
            highlighted: dropdown.highlightedIndex === index

            contentItem: CenteredText {
                text: modelData[dropdown.textRole] !== undefined ? modelData[dropdown.textRole] : modelData
                color: index === dropdown.currentIndex ? VLCStyle.redesign.accent
                                                       : VLCStyle.redesign.color("#8F8F8F", "#2e2e2e")
            }
            background: Rectangle {
                radius: VLCStyle.dp(4, VLCStyle.scale)
                color: parent.highlighted || parent.hovered ? VLCStyle.redesign.color("#222222", "#9c9c9b") : "transparent"
            }
        }
        popup: T.Popup {
            // under the box, from its end in a right to left layout
            x: dropdown.mirrored ? dropdown.width - width : 0
            y: dropdown.height + VLCStyle.dp(4, VLCStyle.scale)
            width: Math.max(dropdown.width, VLCStyle.dp(200, VLCStyle.scale))
            implicitHeight: Math.min(contentItem.implicitHeight + topPadding + bottomPadding,
                                     VLCStyle.dp(32 * 10 + 4, VLCStyle.scale))
            padding: VLCStyle.dp(2, VLCStyle.scale)

            contentItem: Item {
                implicitWidth: popupList.implicitWidth
                implicitHeight: popupList.implicitHeight

                ListView {
                    id: popupList
                    anchors.fill: parent
                    // a popup is not under the mirrored interface: its rows
                    // mirror from here
                    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
                    LayoutMirroring.childrenInherit: true
                    clip: true
                    implicitHeight: contentHeight
                    model: dropdown.popup.visible ? dropdown.delegateModel : null
                    currentIndex: dropdown.highlightedIndex
                    boundsBehavior: Flickable.StopAtBounds
                }

                // the wheel moves it by whole rows
                Widgets.RowSnapWheel {
                    anchors.fill: popupList
                    view: popupList
                }
            }
            background: Rectangle {
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: VLCStyle.redesign.color("#191919", "#b8b8b7")
                border.width: 1
                border.color: VLCStyle.redesign.color("#222222", "#9c9c9b")
            }
        }
    }

    // Figma "Frame 122": #151515, radius 6, padding 5 / 8, the icon and
    // what follows 10 apart
    component HeaderField: Rectangle {
        property alias iconSource: fieldIcon.source
        default property alias content: fieldContent.data

        Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
        radius: VLCStyle.dp(6, VLCStyle.scale)
        color: VLCStyle.redesign.color("#151515", "#b8b8b7")

        // (anchored: from the right in a right to left layout)
        Image {
            id: fieldIcon
            anchors.left: parent.left
            anchors.leftMargin: VLCStyle.dp(8, VLCStyle.scale)
            y: VLCStyle.dp(5, VLCStyle.scale)
            width: VLCStyle.redesign.iconSize
            height: width
            sourceSize: Qt.size(width, height)
            smooth: false
        }
        Item {
            id: fieldContent
            anchors.left: fieldIcon.right
            anchors.leftMargin: VLCStyle.dp(10, VLCStyle.scale)
            anchors.right: parent.right
            anchors.rightMargin: VLCStyle.dp(8, VLCStyle.scale)
            height: parent.height
        }
    }

    // a text typed in a field of the title bar
    component HeaderInput: TextInput {
        id: headerInput

        property string placeholderText

        anchors.fill: parent
        // from the start of the field (the right in a right to left layout)
        horizontalAlignment: TextInput.AlignLeft
        verticalAlignment: TextInput.AlignTop
        topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                   font.pixelSize) - inputMetrics.ascent
        clip: true
        selectByMouse: true
        enabled: !associations.busy
        color: VLCStyle.redesign.color("#969696", "#2e2e2e")
        selectionColor: VLCStyle.redesign.accentBackground
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        FontMetrics {
            id: inputMetrics
            font: headerInput.font
        }
        Text {
            y: headerInput.topPadding
            width: parent.width
            visible: headerInput.length === 0 && !headerInput.activeFocus
            text: headerInput.placeholderText
            color: VLCStyle.redesign.color(VLCStyle.redesign.textDisabled, "#7c7c7b")
            font: headerInput.font
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }
    }

    // the subtitle and video icons of the lists, "Dark/FG/A3" (#6B6B6B)
    component DimIcon: Image {
        width: VLCStyle.redesign.iconSize
        height: width
        sourceSize: Qt.size(width, height)
        smooth: false
    }

    // Figma "Vector 77": 1 px "Dark/Divider/Bright" (#373737), dashes of 8
    // spread on the line (half dashes at its ends), centred on y: half on
    // two lines of pixels
    component DashedLine: Item {
        id: dashedLine
        readonly property real period: VLCStyle.dp(16, VLCStyle.scale)
        readonly property int count: Math.max(1, Math.round(width / period))
        readonly property real step: width / count
        height: VLCStyle.dp(2, VLCStyle.scale)
        clip: true
        Repeater {
            model: dashedLine.width > 0 ? dashedLine.count + 1 : 0
            delegate: Rectangle {
                required property int index
                readonly property real start: Math.max(0, index * dashedLine.step - dashedLine.step / 4)
                readonly property real end: Math.min(dashedLine.width, index * dashedLine.step + dashedLine.step / 4)
                x: Math.round(start)
                width: Math.max(1, Math.round(end) - x)
                height: dashedLine.height
                color: VLCStyle.redesign.color(Qt.rgba(55 / 255, 55 / 255, 55 / 255, 0.5), Qt.rgba(0.4, 0.4, 0.4, 0.5))
            }
        }
    }

    // a file of the lists (rows of 36): the icon at 8, the name 14 after,
    // checked on #090909 (radius 6) with the accent check 8 from the end
    component FileRow: T.AbstractButton {
        id: fileRow

        property string label
        // checked: saved (AbstractButton.checked is final)
        property bool selected: false
        property bool showCheck: true
        // a file left out (a folder's media without subtitles)
        property bool dimmed: false

        height: VLCStyle.dp(36, VLCStyle.scale)
        hoverEnabled: true

        background: Rectangle {
            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: fileRow.selected ? VLCStyle.redesign.color("#090909", "#c4c4c3")
                 : fileRow.hovered ? VLCStyle.redesign.color("#141414", "#b8b8b7")
                 : "transparent"
            border.width: fileRow.visualFocus ? 1 : 0
            border.color: VLCStyle.redesign.focusColor
        }
        // (the icon and the check anchored: mirrored in a right to left
        // layout; the name as far from both ends)
        contentItem: Item {
            DimIcon {
                anchors.left: parent.left
                anchors.leftMargin: VLCStyle.dp(8, VLCStyle.scale)
                y: VLCStyle.dp(6, VLCStyle.scale)
                source: root.audio ? "qrc:///redesign/icons/spectrum/dim.png"
                                   : "qrc:///redesign/icons/subtitle/dim.png"
            }
            CenteredText {
                x: VLCStyle.dp(8 + 24 + 14, VLCStyle.scale)
                width: parent.width - x - VLCStyle.dp(8 + 24 + 14, VLCStyle.scale)
                height: parent.height
                text: fileRow.label
                opacity: fileRow.dimmed ? 0.4 : 1
            }
            Impl.ColorImage {
                visible: fileRow.selected && fileRow.showCheck
                anchors.right: parent.right
                anchors.rightMargin: VLCStyle.dp(8, VLCStyle.scale)
                y: VLCStyle.dp(6, VLCStyle.scale)
                width: VLCStyle.redesign.iconSize
                height: width
                sourceSize: Qt.size(width, height)
                source: "qrc:///redesign/icons/check/active.png"
                color: VLCStyle.redesign.accentTint
                smooth: false
            }
        }
    }

    // ---- the title bar: "Frame 206" ------------------------------------

    headerContent: [
        RowLayout {
            // the dialog is a popup, not under the mirrored interface: what
            // it is given mirrors from here (right to left: the target on
            // the right)
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true

            Layout.fillWidth: true
            Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
            spacing: VLCStyle.dp(10, VLCStyle.scale)

            HeaderDropdown {
                id: targetPicker
                objectName: "subtitleTargetPicker"
                model: associations.targets
                currentIndex: associations.selectedTarget
                shownText: currentIndex <= 0 ? root.targetKind
                           : (associations.targets[currentIndex] ? associations.targets[currentIndex].kind + ": "
                                                                   + associations.targets[currentIndex].label : "")
                onActivated: (index) => associations.selectTarget(index)
            }

            HeaderDropdown {
                id: originPicker
                objectName: "subtitleOriginPicker"
                textRole: ""
                model: root.audio ? [qsTr("From Computer")]
                                  : root.singleMedia
                                    ? [qsTr("From Computer"), qsTr("From Link"), qsTr("From OpenSubtitles")]
                                    : [qsTr("From Computer"), qsTr("From Link")]
                currentIndex: root.originIndex
                onActivated: (index) => {
                    root.originIndex = index
                    // Figma: the search starts from the name of the media
                    if (index === 2 && queryField.text.length === 0)
                        queryField.text = root.mediaQuery
                }
            }

            // the folder of the subtitles: another one chosen from here
            T.Button {
                id: folderButton

                objectName: "subtitleFolderButton"
                Layout.fillWidth: true
                Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
                visible: root.originIndex === 0
                enabled: !associations.busy && !service.busy
                padding: 0
                Accessible.name: root.audio ? qsTr("Choose the folder of the audio tracks")
                                            : qsTr("Choose the folder of the subtitles")

                background: Rectangle {
                    radius: VLCStyle.dp(6, VLCStyle.scale)
                    color: folderButton.hovered ? VLCStyle.redesign.color("#1B1B1B", "#9c9c9b")
                                                : VLCStyle.redesign.color("#151515", "#b8b8b7")
                    border.width: folderButton.visualFocus ? 1 : 0
                    border.color: VLCStyle.redesign.focusColor
                }
                // (anchored: from the right in a right to left layout)
                contentItem: Item {
                    Image {
                        id: folderIcon
                        anchors.left: parent.left
                        anchors.leftMargin: VLCStyle.dp(8, VLCStyle.scale)
                        y: VLCStyle.dp(5, VLCStyle.scale)
                        width: VLCStyle.redesign.iconSize
                        height: width
                        sourceSize: Qt.size(width, height)
                        source: "qrc:///redesign/icons/folder/default.png"
                        smooth: false
                    }
                    CenteredText {
                        id: folderText
                        anchors.left: folderIcon.right
                        anchors.leftMargin: VLCStyle.dp(10, VLCStyle.scale)
                        anchors.right: parent.right
                        anchors.rightMargin: VLCStyle.dp(8, VLCStyle.scale)
                        height: parent.height
                        text: root.folderPath
                    }
                }
                // a long path elided: in full under the pointer
                T.ToolTip.visible: hovered && folderText.truncated
                T.ToolTip.text: root.folderPath
                T.ToolTip.delay: 650
                onClicked: associations.chooseFolder()
            }

            // a direct link to a subtitle file
            HeaderField {
                Layout.fillWidth: true
                visible: root.originIndex === 1
                iconSource: "qrc:///redesign/icons/link/default.png"

                HeaderInput {
                    id: linkField
                    objectName: "subtitleLinkField"
                    placeholderText: qsTr("Direct HTTPS link to a subtitle file")
                    onAccepted: if (text.trim().length > 0) associations.addLink(text.trim())
                }
            }

            // OpenSubtitles: the search (400) and its language
            HeaderField {
                Layout.preferredWidth: VLCStyle.dp(400, VLCStyle.scale)
                Layout.fillWidth: true
                visible: root.online
                iconSource: "qrc:///redesign/icons/search/default.png"

                HeaderInput {
                    id: queryField
                    objectName: "openSubtitlesQuery"
                    placeholderText: qsTr("Title or episode")
                    onAccepted: {
                        if (text.trim().length === 0 || apiKeyField.text.trim().length === 0)
                            return
                        root.onlineSelection = -1
                        service.search(text, root.languages[languagePicker.currentIndex].code, apiKeyField.text)
                    }
                }
            }

            HeaderDropdown {
                id: languagePicker
                objectName: "openSubtitlesLanguage"
                visible: root.online
                model: root.languages
                currentIndex: 0
            }
        }
    ]

    // ---- "Frame 124": the lists -----------------------------------------

    Item {
        // (mirrored from here, as the title bar's row)
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        Layout.fillWidth: true
        // lower in a low window (the dialog within it)
        Layout.fillHeight: true
        Layout.preferredHeight: VLCStyle.dp(632, VLCStyle.scale)

        // a folder or a playlist: its media, each joined to its file
        ListView {
            id: pairs

            objectName: "subtitlePairs"
            anchors.fill: parent
            visible: !root.online && !root.singleMedia
            clip: true
            topMargin: VLCStyle.dp(22, VLCStyle.scale)
            boundsBehavior: Flickable.StopAtBounds
            model: visible ? associations.destinations : []

            T.ScrollBar.vertical: Widgets.ScrollBarExt { }

            delegate: Item {
                id: pair

                required property int index
                required property var modelData
                // the file joined to this media, else the one of its name
                readonly property int sourceIndex: {
                    const rows = associations.sources
                    for (let i = 0; i < rows.length; ++i) {
                        if (rows[i].destination === index)
                            return i
                    }
                    const name = String(modelData.label).replace(/\.[^.]*$/, "").toLowerCase()
                    for (let i = 0; i < rows.length; ++i) {
                        if (rows[i].destination < 0
                                && String(rows[i].label).replace(/\.[^.]*$/, "").toLowerCase() === name)
                            return i
                    }
                    return -1
                }
                readonly property bool joined: sourceIndex >= 0
                                               && associations.sources[sourceIndex].destination === index

                width: pairs.width
                height: VLCStyle.dp(40, VLCStyle.scale)

                // "Frame 3": 30 from the left, rows of 24 (8 under the file's)
                // (anchored, the widths from widths: the media on the right
                // in a right to left layout, the files on the left)
                DimIcon {
                    anchors.left: parent.left
                    anchors.leftMargin: VLCStyle.dp(30, VLCStyle.scale)
                    y: VLCStyle.dp(8, VLCStyle.scale)
                    source: "qrc:///redesign/icons/video/dim.png"
                }
                CenteredText {
                    id: mediaName
                    anchors.left: parent.left
                    anchors.leftMargin: VLCStyle.dp(30 + 24 + 14, VLCStyle.scale)
                    y: VLCStyle.dp(8, VLCStyle.scale)
                    width: Math.min(implicitWidth, VLCStyle.dp(519 - 68 - 14, VLCStyle.scale))
                    height: VLCStyle.dp(24, VLCStyle.scale)
                    text: pair.modelData.label
                }
                DashedLine {
                    visible: pair.joined
                    anchors.left: mediaName.right
                    anchors.leftMargin: VLCStyle.dp(14, VLCStyle.scale)
                    y: VLCStyle.dp(8 + 12, VLCStyle.scale) - height / 2
                    width: Math.max(0, VLCStyle.dp(519, VLCStyle.scale)
                                       - (VLCStyle.dp(30 + 24 + 14, VLCStyle.scale) + mediaName.width
                                          + VLCStyle.dp(14, VLCStyle.scale)))
                }

                // "Frame 124": 20 then 8 from the middle, rows of 36
                FileRow {
                    anchors.left: parent.left
                    anchors.leftMargin: VLCStyle.dp(519 + 20, VLCStyle.scale)
                    anchors.right: parent.right
                    anchors.rightMargin: VLCStyle.dp(20, VLCStyle.scale)
                    visible: pair.sourceIndex >= 0
                    showCheck: false
                    dimmed: !pair.joined
                    label: pair.sourceIndex >= 0 ? associations.sources[pair.sourceIndex].label : ""
                    enabled: !associations.busy
                    // unjoined (dimmed): not saved
                    onClicked: associations.assign(pair.sourceIndex, pair.joined ? -1 : pair.index)
                }
            }
        }

        // one media: the files named after it, checked to be saved
        ListView {
            id: files

            objectName: "subtitleFiles"
            anchors.fill: parent
            visible: !root.online && root.singleMedia
            clip: true
            topMargin: VLCStyle.dp(22, VLCStyle.scale)
            leftMargin: VLCStyle.dp(20, VLCStyle.scale)
            rightMargin: VLCStyle.dp(20, VLCStyle.scale)
            spacing: VLCStyle.dp(4, VLCStyle.scale)
            boundsBehavior: Flickable.StopAtBounds
            model: visible ? associations.sources : []

            T.ScrollBar.vertical: Widgets.ScrollBarExt { }

            delegate: FileRow {
                required property int index
                required property var modelData

                width: files.width - files.leftMargin - files.rightMargin
                label: modelData.label
                selected: modelData.destination >= 0
                enabled: !associations.busy
                onClicked: associations.assign(index, selected ? -1 : 0)
            }
        }

        // OpenSubtitles: the account (the API key at least), then the results
        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: VLCStyle.dp(22, VLCStyle.scale)
            anchors.leftMargin: VLCStyle.dp(20, VLCStyle.scale)
            anchors.rightMargin: VLCStyle.dp(20, VLCStyle.scale)
            anchors.bottomMargin: VLCStyle.dp(22, VLCStyle.scale)
            visible: root.online
            spacing: VLCStyle.dp(4, VLCStyle.scale)

            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: VLCStyle.dp(16, VLCStyle.scale)
                visible: service.results.length === 0
                spacing: VLCStyle.dp(10, VLCStyle.scale)

                // the three fields as wide whatever they hold
                Widgets.RedesignTextArea {
                    id: apiKeyField
                    objectName: "openSubtitlesApiKey"
                    Layout.preferredWidth: 1
                    placeholderText: qsTr("OpenSubtitles API key")
                    Accessible.name: placeholderText
                }
                Widgets.RedesignTextArea {
                    id: usernameField
                    objectName: "openSubtitlesUsername"
                    Layout.preferredWidth: 1
                    placeholderText: qsTr("Username")
                    Accessible.name: placeholderText
                }
                // the password masked: a line input of the look of
                // RedesignTextArea (a text area can not hide its text)
                T.TextField {
                    id: passwordField
                    objectName: "openSubtitlesPassword"

                    readonly property real border: VLCStyle.dp(2, VLCStyle.scale)

                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.topMargin: VLCStyle.dp(2, VLCStyle.scale)
                    Layout.bottomMargin: VLCStyle.dp(2, VLCStyle.scale)
                    implicitHeight: VLCStyle.dp(8 + 21 + 8, VLCStyle.scale)
                    topPadding: VLCStyle.redesign.textBaseline(border + VLCStyle.dp(8, VLCStyle.scale), font.pixelSize) - passwordMetrics.ascent
                    bottomPadding: border + VLCStyle.dp(8, VLCStyle.scale)
                    // (swapped in a right to left layout, the text from the right)
                    leftPadding: border + VLCStyle.dp(LayoutMirroring.enabled ? 18 : 12, VLCStyle.scale)
                    rightPadding: border + VLCStyle.dp(LayoutMirroring.enabled ? 12 : 18, VLCStyle.scale)
                    horizontalAlignment: TextInput.AlignLeft
                    verticalAlignment: TextInput.AlignTop
                    echoMode: TextInput.Password
                    placeholderText: qsTr("Password")
                    Accessible.name: placeholderText

                    color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
                    placeholderTextColor: VLCStyle.redesign.color(VLCStyle.redesign.textDisabled, "#7c7c7b")
                    selectionColor: VLCStyle.redesign.color(VLCStyle.redesign.accentBackground, "#dedede")
                    selectedTextColor: VLCStyle.redesign.color(VLCStyle.redesign.textPrimary, "#191919")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    selectByMouse: true

                    FontMetrics {
                        id: passwordMetrics
                        font: passwordField.font
                    }

                    background: Rectangle {
                        radius: VLCStyle.dp(6, VLCStyle.scale)
                        color: VLCStyle.redesign.color("#090909", "#dedede")
                        border.width: passwordField.border
                        border.color: VLCStyle.redesign.color("#191919", "#c4c4c3")
                    }

                    // the placeholder, drawn by the style otherwise
                    Text {
                        x: passwordField.leftPadding
                        y: passwordField.topPadding
                        width: passwordField.width - passwordField.leftPadding - passwordField.rightPadding
                        visible: passwordField.length === 0 && passwordField.preeditText === ""
                        text: passwordField.placeholderText
                        color: passwordField.placeholderTextColor
                        font: passwordField.font
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                    }
                }
            }

            ListView {
                id: results

                objectName: "openSubtitlesResults"
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: VLCStyle.dp(4, VLCStyle.scale)
                boundsBehavior: Flickable.StopAtBounds
                model: service.results

                T.ScrollBar.vertical: Widgets.ScrollBarExt { }

                delegate: FileRow {
                    required property int index
                    required property var modelData

                    width: results.width
                    label: modelData.filename
                    selected: root.onlineSelection === index
                    enabled: !service.busy && !associations.busy
                    onClicked: root.onlineSelection = (selected ? -1 : index)
                }
            }
        }

        // nothing to show: why (a longer text, a translation or an error,
        // on more lines, the block centred)
        CenteredText {
            anchors.centerIn: parent
            width: parent.width - VLCStyle.dp(80, VLCStyle.scale)
            boxHeight: VLCStyle.dp(34, VLCStyle.scale)
            height: boxHeight + (lineCount > 1 ? contentHeight - contentHeight / lineCount : 0)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            elide: Text.ElideNone
            visible: text.length > 0
            color: (associations.error.length > 0 || service.error.length > 0)
                   ? VLCStyle.redesign.accent : VLCStyle.redesign.color("#6B6B6B", "#4d4d4d")
            text: {
                if (associations.error.length > 0)
                    return associations.error
                if (root.online) {
                    if (service.error.length > 0)
                        return service.error
                    if (service.busy)
                        return qsTr("Searching OpenSubtitles…")
                    return service.results.length === 0 && apiKeyField.text.length > 0
                           ? qsTr("Search OpenSubtitles to see the subtitle files.") : ""
                }
                if (associations.busy)
                    return root.audio ? qsTr("Working on audio tracks…") : qsTr("Working on subtitles…")
                if (root.singleMedia ? associations.sources.length === 0
                                     : associations.destinations.length > 0 && associations.sources.length === 0)
                    return root.originIndex === 1 ? qsTr("Enter a direct link to a subtitle file above.")
                         : root.audio ? qsTr("No audio file in this folder: choose another one above.")
                         : qsTr("No subtitle file in this folder: choose another one above.")
                return ""
            }
        }
    }
}
