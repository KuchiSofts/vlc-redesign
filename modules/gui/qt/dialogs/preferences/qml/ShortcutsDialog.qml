/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import VLC.Dialogs
import VLC.MainInterface
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

// Figma "Keyboard Shortcuts" (2437:14832): 1048 x 800. The title bar holds
// the search (fill) and "All Fields" (200); the content (padding 20 / 20 /
// 22 / 20, 20 apart): "Action", "Hotkey", "Global Hotkey" (14 #6B6B6B),
// a line, then each group: its title (Inter 16 #969696), its rows (33 high,
// 14 apart: the action, Inter 14 #C6C6C6 in 290, the two keys in 270 wide
// fields at the end and in the middle of the room left), a line.
Widgets.RedesignDialog {
    id: root
    objectName: "redesignShortcutsDialog"

    property bool saved: false
    property string profileError: ""
    property bool resetRequested: false
    property string captureOption: ""
    property string captureAction: ""
    property bool captureGlobal: false
    property int capturedKey: 0
    property int capturedModifiers: 0
    property string conflictAction: ""
    readonly property bool dark: VLCStyle.palette.isDark
    // the scroll bar and the fields look their colours up here
    readonly property ColorContext colorContext: ColorContext {
        colorSet: ColorContext.Window
        palette: VLCStyle.palette
    }
    readonly property var actionLabels: ({
        "key-toggle-fullscreen": qsTr("Toggle fullscreen"),
        "key-leave-fullscreen": qsTr("Exit fullscreen / close dialog"),
        "key-position": qsTr("Show playback position and time"),
        "key-intf-popup-menu": qsTr("Show the menu"),
        "key-intf-show": qsTr("Show interface controls in fullscreen"),
        "key-play-pause": qsTr("Play / pause"),
        "key-stop": qsTr("Stop playback"),
        "key-next": qsTr("Next track"),
        "key-prev": qsTr("Previous track"),
        "key-program-sid-next": qsTr("Next program / Service ID")
    })

    title: qsTr("Keyboard Shortcuts")
    preferredWidth: VLCStyle.dp(1048, VLCStyle.scale)
    bodyHeight: VLCStyle.dp(632, VLCStyle.scale)
    bodyHorizontalPadding: 0
    bodyVerticalPadding: 0
    acceptText: qsTr("Save")
    acceptEnabled: shortcuts.dirty || !!root.profileError
    // the restart notice closes it once saved
    closeOnAccept: false

    // "f" reads F as on a keyboard, "Ctrl+e" Ctrl+E
    function displayKeys(sequence) {
        if (!sequence)
            return ""
        return sequence.split("+").map(part => part.length === 1 ? part.toUpperCase()
                                             : part === "Esc" ? "esc" : part).join("+")
    }

    readonly property var scopes: [
        { label: qsTr("All Fields"), value: "all" },
        { label: qsTr("Action"), value: "action" },
        { label: qsTr("Hotkey"), value: "hotkey" },
        { label: qsTr("Global Hotkey"), value: "global" }
    ]

    function reject() {
        root.close()
    }

    function beginCapture(option, global, action) {
        captureOption = option
        captureGlobal = global
        captureAction = action
        conflictAction = ""
        capturePopup.open()
    }

    function assignCaptured(key, modifiers, replaceConflict) {
        const result = shortcuts.stageKey(captureOption, captureGlobal,
                                          key, modifiers, replaceConflict)
        if (result.ok) {
            capturePopup.close()
            return
        }
        if (result.needsConfirmation) {
            capturedKey = key
            capturedModifiers = modifiers
            conflictAction = result.conflictAction
        }
    }

    // Opens on the actions a search of the settings found
    property string initialSearch: ""
    function openWithSearch(text) {
        initialSearch = text
        open()
    }

    function openReset() {
        if (visible) resetConfirmation.open()
        else { resetRequested = true; open() }
    }

    onAccepted: {
        if (!root.profileError && !shortcuts.apply())
            return
        root.saved = true
        if (!MainCtx.controlbarProfileModel.save()) {
            root.profileError = MainCtx.controlbarProfileModel.lastError
            return
        }
        root.profileError = ""
        restartNotice.open()
    }

    onOpened: {
        saved = false
        profileError = ""
        shortcuts.reload()
        searchField.text = initialSearch
        initialSearch = ""
        scopePicker.currentIndex = 0
        if (resetRequested) resetConfirmation.open()
        resetRequested = false
    }
    onClosed: {
        capturePopup.close()
        resetConfirmation.close()
        restartNotice.close()
        if (!saved)
            shortcuts.cancel()
    }

    ShortcutsModel {
        id: shortcuts
        ctx: MainCtx
        search: searchField.text
        searchField: root.scopes[Math.max(0, scopePicker.currentIndex)].value
    }

    // a line of text placed as Figma does (the glyphs centred on the line
    // height, from the top of the box)
    component FigmaText: Text {
        id: figmaText

        property real lineHeightPx: VLCStyle.dp(25, VLCStyle.scale)
        // the line of the font (Inter: 1.2109 em, ascender 0.96875)
        property real lineRatio: 1.2109375
        property real ascentRatio: 0.96875

        height: lineHeightPx
        verticalAlignment: Text.AlignTop
        elide: Text.ElideRight
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        // the baseline on the whole pixel where Figma's glyphs land
        topPadding: Math.round((lineHeightPx - lineRatio * font.pixelSize) / 2 + ascentRatio * font.pixelSize) - figmaMetrics.ascent

        FontMetrics {
            id: figmaMetrics
            font: figmaText.font
        }
    }

    // "Vector": 1 px "Dark/Divider/Dim" (#222222) centred on its place: half
    // on two lines of pixels
    component Divider: Rectangle {
        height: VLCStyle.dp(2, VLCStyle.scale)
        color: root.dark ? Qt.rgba(34 / 255, 34 / 255, 34 / 255, 0.5) : Qt.rgba(0.61, 0.61, 0.61, 0.5)
    }

    // Figma "Frame 162": 270 x 33, #090909, radius 6, the keys (DM Mono 14,
    // #C6C6C6) in the middle, the edit icon (#404040) 5 from the end. An
    // action has several keys: one chip each, its cross (shown while the
    // field is hovered) removes it; a press on the field adds a key.
    component KeyField: FocusScope {
        id: keyField

        // the keys as VLC writes them ("f", "Ctrl+e")
        property var keyList: []
        // the name read out for the field ("Hotkey of Play / pause")
        property string accessibleName

        signal addRequested()
        signal removeRequested(string key)

        readonly property int keyCount: keyList ? keyList.length : 0
        readonly property string keysText: {
            const shown = []
            for (let i = 0; i < keyCount; ++i)
                shown.push(root.displayKeys(keyList[i]))
            return shown.join(", ")
        }
        // the crosses show while the field is hovered or has the keyboard
        readonly property bool editing: fieldHover.hovered || keyField.activeFocus

        width: VLCStyle.dp(270, VLCStyle.scale)
        height: VLCStyle.dp(33, VLCStyle.scale)

        HoverHandler { id: fieldHover }

        T.Button {
            id: addButton

            anchors.fill: parent
            padding: 0

            Accessible.name: keyField.accessibleName
            Accessible.description: keyField.keyCount > 0
                                    ? qsTr("%1. Press to add a key").arg(keyField.keysText)
                                    : qsTr("No key. Press to add one")

            onClicked: keyField.addRequested()

            background: Rectangle {
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: addButton.hovered ? VLCStyle.redesign.color("#141414", "#e5e5e5")
                                         : VLCStyle.redesign.color("#090909", "#fafafa")
                border.width: addButton.visualFocus ? 1 : 0
                border.color: VLCStyle.redesign.focusColor
            }
            contentItem: Item {
                Image {
                    x: parent.width - VLCStyle.dp(5, VLCStyle.scale) - width
                    y: VLCStyle.dp(4, VLCStyle.scale)
                    width: VLCStyle.redesign.iconSize
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: "qrc:///redesign/icons/edit/dim.png"
                    smooth: false
                }
            }
        }

        // Figma: the keys centred in the 230 between the paddings of 20;
        // several keys share that room (each one elided when they don't fit)
        Row {
            id: chipRow

            readonly property real room: keyField.width - 2 * VLCStyle.dp(20, VLCStyle.scale)

            x: VLCStyle.dp(20, VLCStyle.scale) + (room - width) / 2
            y: VLCStyle.dp(4, VLCStyle.scale)
            spacing: VLCStyle.dp(4, VLCStyle.scale)

            Repeater {
                model: keyField.keyList

                delegate: Rectangle {
                    id: chip

                    required property string modelData

                    readonly property real crossSize: VLCStyle.dp(16, VLCStyle.scale)
                    // after the text: 2, the cross, 2
                    readonly property real crossRoom: crossSize + VLCStyle.dp(4, VLCStyle.scale)
                    // a lone key keeps that room on its left too: its text
                    // in the middle of the field, as the design
                    readonly property real leftRoom: keyField.keyCount === 1 ? crossRoom
                                                                             : VLCStyle.dp(4, VLCStyle.scale)
                    readonly property real maxWidth: (chipRow.room - chipRow.spacing * (keyField.keyCount - 1))
                                                     / Math.max(1, keyField.keyCount)

                    width: Math.min(leftRoom + Math.ceil(keyText.implicitWidth - 0.05) + crossRoom,
                                    Math.floor(maxWidth))
                    height: VLCStyle.dp(25, VLCStyle.scale)
                    radius: VLCStyle.dp(4, VLCStyle.scale)
                    // the keys apart from each other when there are several
                    color: keyField.keyCount > 1 && keyField.editing
                           ? VLCStyle.redesign.color("#191919", "#ececec") : "transparent"

                    FigmaText {
                        id: keyText
                        x: chip.leftRoom
                        width: Math.max(0, chip.width - x - chip.crossRoom)
                        text: root.displayKeys(chip.modelData)
                        color: VLCStyle.redesign.color("#C6C6C6", "#333333")
                        // DM Mono (bundled): a line of 1.302 em, ascender 0.992
                        font.family: VLCStyle.redesign.monoFontFamily
                        lineRatio: 1.302
                        ascentRatio: 0.992
                    }

                    T.Button {
                        id: removeButton

                        x: chip.width - width - VLCStyle.dp(2, VLCStyle.scale)
                        anchors.verticalCenter: parent.verticalCenter
                        width: chip.crossSize
                        height: chip.crossSize
                        padding: 0
                        opacity: keyField.editing ? 1 : 0
                        activeFocusOnTab: true

                        Accessible.name: qsTr("Remove %1").arg(keyText.text)

                        onClicked: keyField.removeRequested(chip.modelData)

                        background: Rectangle {
                            radius: VLCStyle.dp(3, VLCStyle.scale)
                            color: removeButton.hovered ? VLCStyle.redesign.color("#222222", "#dcdcdc") : "transparent"
                            border.width: removeButton.visualFocus ? 1 : 0
                            border.color: VLCStyle.redesign.focusColor
                        }
                        contentItem: Image {
                            sourceSize: Qt.size(width, height)
                            source: SVGColorImage.colorize("qrc:///redesign/duo/close.svg")
                                        .color1(removeButton.hovered ? VLCStyle.redesign.color("#C6C6C6", "#333333")
                                                                     : VLCStyle.redesign.color("#6B6B6B", "#7c7c7b")).uri()
                        }
                    }
                }
            }
        }
    }

    // ---- the title bar: the search and the field it looks in ----------

    headerContent: [
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
            spacing: VLCStyle.dp(10, VLCStyle.scale)

            // #151515, radius 6, padding 5 / 8, the icon then the text 10 after
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: VLCStyle.redesign.color("#151515", "#eeeeee")
                border.width: searchField.activeFocus ? 1 : 0
                border.color: VLCStyle.redesign.focusColor

                Image {
                    id: searchIcon
                    x: VLCStyle.dp(8, VLCStyle.scale)
                    y: VLCStyle.dp(5, VLCStyle.scale)
                    width: VLCStyle.redesign.iconSize
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: "qrc:///redesign/icons/search/default.png"
                    smooth: false
                }
                TextInput {
                    id: searchField

                    objectName: "shortcutsSearch"
                    x: searchIcon.x + searchIcon.width + VLCStyle.dp(10, VLCStyle.scale)
                    width: parent.width - x - VLCStyle.dp(8, VLCStyle.scale)
                    height: parent.height
                    clip: true
                    selectByMouse: true
                    verticalAlignment: TextInput.AlignTop
                    topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                               font.pixelSize) - searchMetrics.ascent
                    color: VLCStyle.redesign.color("#969696", "#222222")
                    selectionColor: VLCStyle.redesign.accentBackground
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    FontMetrics {
                        id: searchMetrics
                        font: searchField.font
                    }
                    Text {
                        y: searchField.topPadding
                        width: parent.width
                        visible: searchField.length === 0
                        text: qsTr("Search Keyboard Shortcuts...")
                        color: VLCStyle.redesign.color(VLCStyle.redesign.textDisabled, "#7c7c7b")
                        font: searchField.font
                        elide: Text.ElideRight
                    }
                }
            }

            // Figma "target": 200 x 34, #151515, the text 12 in, the chevron
            T.ComboBox {
                id: scopePicker

                objectName: "shortcutsSearchField"
                Layout.preferredWidth: VLCStyle.dp(200, VLCStyle.scale)
                Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
                model: root.scopes
                textRole: "label"
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                leftPadding: VLCStyle.dp(12, VLCStyle.scale)
                rightPadding: VLCStyle.dp(8 + 24, VLCStyle.scale)

                background: Rectangle {
                    radius: VLCStyle.dp(6, VLCStyle.scale)
                    color: scopePicker.hovered || scopePicker.popup.visible ? VLCStyle.redesign.color("#1B1B1B", "#e5e5e5")
                                                                            : VLCStyle.redesign.color("#151515", "#eeeeee")
                    border.width: scopePicker.visualFocus ? 1 : 0
                    border.color: VLCStyle.redesign.focusColor
                }
                contentItem: Text {
                    id: scopeText
                    text: scopePicker.displayText
                    color: VLCStyle.redesign.color("#969696", "#292929")
                    font: scopePicker.font
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignTop
                    topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                               font.pixelSize) - scopeMetrics.ascent
                    FontMetrics {
                        id: scopeMetrics
                        font: scopeText.font
                    }
                }
                indicator: Image {
                    x: scopePicker.width - VLCStyle.dp(8, VLCStyle.scale) - width
                    y: Math.round((scopePicker.height - height) / 2)
                    width: VLCStyle.redesign.iconSize
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: "qrc:///redesign/icons/chevron-down/default.png"
                    rotation: scopePicker.popup.visible ? 180 : 0
                }
                delegate: T.ItemDelegate {
                    required property var modelData
                    required property int index

                    width: ListView.view ? ListView.view.width : scopePicker.width
                    implicitHeight: VLCStyle.dp(32, VLCStyle.scale)
                    leftPadding: VLCStyle.dp(12, VLCStyle.scale)
                    highlighted: scopePicker.highlightedIndex === index
                    contentItem: Text {
                        text: modelData.label
                        color: index === scopePicker.currentIndex ? VLCStyle.redesign.accent
                                                                  : VLCStyle.redesign.color("#8F8F8F", "#292929")
                        font: scopePicker.font
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    background: Rectangle {
                        radius: VLCStyle.dp(4, VLCStyle.scale)
                        color: parent.highlighted || parent.hovered ? VLCStyle.redesign.color("#222222", "#e5e5e5") : "transparent"
                    }
                }
                popup: T.Popup {
                    y: scopePicker.height + VLCStyle.dp(4, VLCStyle.scale)
                    width: scopePicker.width
                    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding
                    padding: VLCStyle.dp(2, VLCStyle.scale)
                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: scopePicker.popup.visible ? scopePicker.delegateModel : null
                        currentIndex: scopePicker.highlightedIndex
                    }
                    background: Rectangle {
                        radius: VLCStyle.dp(6, VLCStyle.scale)
                        color: VLCStyle.redesign.color("#191919", "#f7f7f7")
                        border.width: 1
                        border.color: VLCStyle.redesign.color("#222222", "#cccccc")
                    }
                }
            }
        }
    ]

    // ---- "Frame 124": padding 20 / 20 / 22 / 20 -------------------------

    Item {
        id: content

        Layout.fillWidth: true
        // lower in a low window (the dialog within it)
        Layout.fillHeight: true
        Layout.preferredHeight: VLCStyle.dp(632, VLCStyle.scale)

        readonly property real side: VLCStyle.dp(20, VLCStyle.scale)
        readonly property real innerWidth: width - 2 * side
        readonly property real labelWidth: VLCStyle.dp(290, VLCStyle.scale)
        readonly property real fieldWidth: VLCStyle.dp(270, VLCStyle.scale)
        // "space-between": the hotkey in the middle of the room left
        readonly property real hotkeyX: labelWidth + (innerWidth - labelWidth - 2 * fieldWidth) / 2
        readonly property real globalX: innerWidth - fieldWidth

        // "Frame 503": the names of the columns
        Item {
            x: content.side
            y: VLCStyle.dp(20, VLCStyle.scale)
            width: content.innerWidth
            height: VLCStyle.dp(25, VLCStyle.scale)

            FigmaText {
                width: content.labelWidth
                text: qsTr("Action")
                color: VLCStyle.redesign.color("#6B6B6B", "#555555")
            }
            // "Frame 503": Action 488 wide, Hotkey 332, then Global Hotkey;
            // in a narrower window, moved with the fields under them (the
            // Hotkey field by half the width lost, the Global one by all)
            FigmaText {
                x: VLCStyle.dp(488, VLCStyle.scale) - Math.round((root.preferredWidth - root.width) / 2)
                width: VLCStyle.dp(332, VLCStyle.scale)
                text: qsTr("Hotkey")
                color: VLCStyle.redesign.color("#6B6B6B", "#555555")
            }
            FigmaText {
                x: VLCStyle.dp(488 + 332, VLCStyle.scale) - Math.round(root.preferredWidth - root.width)
                text: qsTr("Global Hotkey")
                color: VLCStyle.redesign.color("#6B6B6B", "#555555")
            }
        }

        Divider {
            x: content.side
            y: VLCStyle.dp(20 + 25 + 20, VLCStyle.scale) - height / 2
            width: content.innerWidth
        }

        ListView {
            id: shortcutList

            objectName: "shortcutsList"
            x: content.side
            y: VLCStyle.dp(20 + 25 + 20 + 20, VLCStyle.scale)
            width: content.innerWidth
            // to the end of the content: Figma clips the rows at the frame
            height: parent.height - y
            clip: true
            model: shortcuts
            spacing: 0
            boundsBehavior: Flickable.StopAtBounds
            section.property: "group"
            section.criteria: ViewSection.FullString

            T.ScrollBar.vertical: Widgets.ScrollBarExt { }

            delegate: Item {
                id: shortcutRow

                required property int index
                required property string key
                required property string action
                required property var hotkeys
                required property var globalHotkeys
                required property bool dirty
                // Names of the design for the actions it shows
                readonly property string label: root.actionLabels[key] ?? action
                // the first row of a group carries its title: the first
                // group right under the names of the columns, the next ones
                // after the room under the rows (5), a line, 20 around it
                readonly property bool groupStart: ListView.section !== ListView.previousSection
                readonly property real headerHeight: !groupStart ? 0
                                                     : index === 0 ? VLCStyle.dp(28 + 20, VLCStyle.scale)
                                                                   : VLCStyle.dp(5 + 20 - 14 + 20 + 28 + 20, VLCStyle.scale)

                width: shortcutList.width
                // the row (33) and the 14 under it
                height: headerHeight + VLCStyle.dp(33 + 14, VLCStyle.scale)

                Divider {
                    visible: shortcutRow.groupStart && shortcutRow.index > 0
                    y: VLCStyle.dp(5 + 20 - 14, VLCStyle.scale) - height / 2
                    width: parent.width
                }
                FigmaText {
                    visible: shortcutRow.groupStart
                    y: shortcutRow.headerHeight - VLCStyle.dp(28 + 20, VLCStyle.scale)
                    width: parent.width
                    lineHeightPx: VLCStyle.dp(28, VLCStyle.scale)
                    text: shortcutRow.ListView.section
                    color: VLCStyle.redesign.color("#969696", "#333333")
                    font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                }

                Item {
                    y: shortcutRow.headerHeight
                    width: parent.width
                    height: VLCStyle.dp(33, VLCStyle.scale)

                    FigmaText {
                        y: VLCStyle.dp(4, VLCStyle.scale)
                        width: content.labelWidth
                        text: shortcutRow.label
                        color: shortcutRow.dirty ? VLCStyle.redesign.accent
                                                 : VLCStyle.redesign.color("#C6C6C6", "#333333")
                    }
                    KeyField {
                        objectName: "hotkey_" + shortcutRow.key
                        x: content.hotkeyX
                        keyList: shortcutRow.hotkeys
                        accessibleName: qsTr("Hotkey of %1").arg(shortcutRow.label)
                        onAddRequested: root.beginCapture(shortcutRow.key, false, shortcutRow.label)
                        onRemoveRequested: (key) => shortcuts.removeKey(shortcutRow.key, false, key)
                    }
                    KeyField {
                        objectName: "globalHotkey_" + shortcutRow.key
                        x: content.globalX
                        keyList: shortcutRow.globalHotkeys
                        accessibleName: qsTr("Global hotkey of %1").arg(shortcutRow.label)
                        onAddRequested: root.beginCapture(shortcutRow.key, true, shortcutRow.label)
                        onRemoveRequested: (key) => shortcuts.removeKey(shortcutRow.key, true, key)
                    }
                }
            }
        }

        // what went wrong saving
        FigmaText {
            x: content.side
            y: parent.height - height
            width: content.innerWidth
            visible: shortcuts.error.length > 0 || root.profileError.length > 0
            text: root.profileError || shortcuts.error
            color: "#df8950"
        }
    }

    ShortcutsCapturePopup {
        id: capturePopup
        objectName: "shortcutCapturePopup"
        parent: root.contentItem
        // centred on whole pixels: the text stays crisp
        x: parent ? Math.round((parent.width - width) / 2) : 0
        y: parent ? Math.round((parent.height - height) / 2) : 0
        width: Math.min(VLCStyle.dp(470, VLCStyle.scale), root.width - 30)
        captureGlobal: root.captureGlobal
        captureAction: root.captureAction
        conflictAction: root.conflictAction
        errorText: shortcuts.error
        onKeyCaptured: (key, modifiers) => root.assignCaptured(key, modifiers, false)
        onClearRequested: {
            shortcuts.clearKey(root.captureOption, root.captureGlobal)
            capturePopup.close()
        }
        onReplaceRequested: root.assignCaptured(root.capturedKey,
                                                 root.capturedModifiers, true)
        // back to waiting for another key
        onConflictCancelled: root.conflictAction = ""
    }

    ShortcutsResetPopup {
        id: resetConfirmation
        objectName: "shortcutsResetConfirmation"
        parent: root.contentItem
        // centred on whole pixels: the text stays crisp (from the width it
        // is given, not its own: setting x makes the popup lay out again,
        // which was a binding loop)
        readonly property real givenWidth: Math.min(VLCStyle.dp(420, VLCStyle.scale), root.width - 30)
        x: parent ? Math.round((parent.width - givenWidth) / 2) : 0
        y: parent ? Math.round((parent.height - VLCStyle.dp(172, VLCStyle.scale)) / 2) : 0
        width: givenWidth
        onConfirmed: shortcuts.resetAll()
    }

    ShortcutsRestartNotice {
        id: restartNotice
        parent: root.parent
        x: parent ? Math.round((parent.width - width) / 2) : 0
        y: parent ? Math.round((parent.height - height) / 2) : 0
        onAccepted: root.close()
    }
}
