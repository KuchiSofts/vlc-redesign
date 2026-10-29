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
    // closed once saved (an error keeps it open); the shortcuts work at once
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
        // the keys changed since a failed profile save are applied too
        if (shortcuts.dirty && !shortcuts.apply())
            return
        root.saved = true
        if (!MainCtx.controlbarProfileModel.save()) {
            root.profileError = MainCtx.controlbarProfileModel.lastError
            return
        }
        root.profileError = ""
        root.close()
    }

    onOpened: {
        saved = false
        profileError = ""
        // (the rows stay when the actions are the same: the list starts
        // at its top as a new one did)
        shortcuts.refresh()
        shortcutList.positionViewAtBeginning()
        searchField.text = initialSearch
        initialSearch = ""
        scopePicker.currentIndex = 0
        if (resetRequested) resetConfirmation.open()
        resetRequested = false
    }
    onClosed: {
        capturePopup.close()
        resetConfirmation.close()
        if (!saved)
            shortcuts.cancel()
    }

    ShortcutsModel {
        id: shortcuts
        ctx: MainCtx
        liveTv: Iptv.enabled
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
        // (on the right in its box, right to left)
        horizontalAlignment: Text.AlignLeft
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
                                         : VLCStyle.redesign.color("#090909", "#dedede")
                border.width: addButton.visualFocus ? 1 : 0
                border.color: VLCStyle.redesign.focusColor
            }
            contentItem: Item {
                Image {
                    // at the end of the field (the left, right to left)
                    x: LayoutMirroring.enabled ? VLCStyle.dp(5, VLCStyle.scale)
                                               : parent.width - VLCStyle.dp(5, VLCStyle.scale) - width
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
        // several keys share that room: each its own width when they fit,
        // else the short ones theirs and the others the rest in equal parts
        // (elided)
        Row {
            id: chipRow

            readonly property real room: keyField.width - 2 * VLCStyle.dp(20, VLCStyle.scale)
            readonly property real crossRoom: VLCStyle.dp(16 + 4, VLCStyle.scale)
            // the width of each chip, in the order of the keys
            readonly property var chipWidths: {
                const count = keyField.keyCount
                const lefts = count === 1 ? crossRoom : VLCStyle.dp(4, VLCStyle.scale)
                const natural = []
                for (let i = 0; i < count; ++i)
                    natural.push(lefts + Math.ceil(monoMetrics.advanceWidth(root.displayKeys(keyField.keyList[i])) - 0.05)
                                 + crossRoom)
                const order = natural.map((w, i) => i).sort((a, b) => natural[a] - natural[b])
                let left = room - spacing * Math.max(0, count - 1)
                const widths = new Array(count)
                order.forEach((i, n) => {
                    widths[i] = Math.min(natural[i], Math.floor(left / (count - n)))
                    left -= widths[i]
                })
                return widths
            }

            FontMetrics {
                id: monoMetrics
                font.family: VLCStyle.redesign.monoFontFamily
                font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
            }

            x: VLCStyle.dp(20, VLCStyle.scale) + (room - width) / 2
            y: VLCStyle.dp(4, VLCStyle.scale)
            spacing: VLCStyle.dp(4, VLCStyle.scale)

            Repeater {
                model: keyField.keyList

                delegate: Rectangle {
                    id: chip

                    required property string modelData
                    required property int index

                    readonly property real crossSize: VLCStyle.dp(16, VLCStyle.scale)
                    // after the text: 2, the cross, 2
                    readonly property real crossRoom: chipRow.crossRoom
                    // a lone key keeps that room on its left too: its text
                    // in the middle of the field, as the design
                    readonly property real leftRoom: keyField.keyCount === 1 ? crossRoom
                                                                             : VLCStyle.dp(4, VLCStyle.scale)

                    width: chipRow.chipWidths[index] ?? 0
                    height: VLCStyle.dp(25, VLCStyle.scale)
                    radius: VLCStyle.dp(4, VLCStyle.scale)
                    // the keys apart from each other when there are several
                    // (brighter while they can be removed)
                    color: keyField.keyCount <= 1 ? "transparent"
                           : keyField.editing ? VLCStyle.redesign.color("#191919", "#ababaa")
                           : VLCStyle.redesign.color("#141414", "#f2f2f2")

                    FigmaText {
                        id: keyText
                        // the cross after it: on its left, right to left
                        x: LayoutMirroring.enabled ? chip.crossRoom : chip.leftRoom
                        width: Math.max(0, chip.width - chip.leftRoom - chip.crossRoom)
                        text: root.displayKeys(chip.modelData)
                        color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
                        // DM Mono (bundled): a line of 1.302 em, ascender 0.992
                        font.family: VLCStyle.redesign.monoFontFamily
                        lineRatio: 1.302
                        ascentRatio: 0.992
                    }

                    T.Button {
                        id: removeButton

                        x: LayoutMirroring.enabled ? VLCStyle.dp(2, VLCStyle.scale)
                                                   : chip.width - width - VLCStyle.dp(2, VLCStyle.scale)
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
                            color: removeButton.hovered ? VLCStyle.redesign.color("#222222", "#9c9c9b") : "transparent"
                            border.width: removeButton.visualFocus ? 1 : 0
                            border.color: VLCStyle.redesign.focusColor
                        }
                        contentItem: Image {
                            sourceSize: Qt.size(width, height)
                            source: SVGColorImage.colorize("qrc:///redesign/duo/close.svg")
                                        .color1(removeButton.hovered ? VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
                                                                     : VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")).uri()
                        }
                    }
                }
            }
        }
    }

    // ---- the title bar: the search and the field it looks in ----------

    headerContent: [
        RowLayout {
            // the dialog is a popup, not under the mirrored interface: right
            // to left, its content is mirrored here (All Fields on the left)
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            Layout.fillWidth: true
            Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
            spacing: VLCStyle.dp(10, VLCStyle.scale)

            // #151515, radius 6, padding 5 / 8, the icon then the text 10 after
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: VLCStyle.dp(34, VLCStyle.scale)
                radius: VLCStyle.dp(6, VLCStyle.scale)
                color: VLCStyle.redesign.color("#151515", "#b5b5b4")
                border.width: searchField.activeFocus ? 1 : 0
                border.color: VLCStyle.redesign.focusColor

                Image {
                    id: searchIcon
                    x: LayoutMirroring.enabled ? parent.width - width - VLCStyle.dp(8, VLCStyle.scale)
                                               : VLCStyle.dp(8, VLCStyle.scale)
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
                    // after the icon (on its left, right to left)
                    x: LayoutMirroring.enabled ? VLCStyle.dp(8, VLCStyle.scale)
                                               : VLCStyle.dp(8, VLCStyle.scale) + searchIcon.width + VLCStyle.dp(10, VLCStyle.scale)
                    width: parent.width - VLCStyle.dp(8, VLCStyle.scale) - searchIcon.width - VLCStyle.dp(10, VLCStyle.scale)
                           - VLCStyle.dp(8, VLCStyle.scale)
                    height: parent.height
                    clip: true
                    selectByMouse: true
                    horizontalAlignment: TextInput.AlignLeft
                    verticalAlignment: TextInput.AlignTop
                    topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                               font.pixelSize) - searchMetrics.ascent
                    color: VLCStyle.redesign.color("#969696", "#4d4d4d")
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
                        color: VLCStyle.redesign.color(VLCStyle.redesign.textDisabled, "#929291")
                        font: searchField.font
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
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
                // (the chevron on the left, right to left)
                leftPadding: mirrored ? VLCStyle.dp(8 + 24, VLCStyle.scale) : VLCStyle.dp(12, VLCStyle.scale)
                rightPadding: mirrored ? VLCStyle.dp(12, VLCStyle.scale) : VLCStyle.dp(8 + 24, VLCStyle.scale)

                background: Rectangle {
                    radius: VLCStyle.dp(6, VLCStyle.scale)
                    color: scopePicker.hovered || scopePicker.popup.visible ? VLCStyle.redesign.color("#1B1B1B", "#ababaa")
                                                                            : VLCStyle.redesign.color("#151515", "#b5b5b4")
                    border.width: scopePicker.visualFocus ? 1 : 0
                    border.color: VLCStyle.redesign.focusColor
                }
                contentItem: Text {
                    id: scopeText
                    text: scopePicker.displayText
                    color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                    font: scopePicker.font
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignTop
                    topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                               font.pixelSize) - scopeMetrics.ascent
                    FontMetrics {
                        id: scopeMetrics
                        font: scopeText.font
                    }
                }
                indicator: Image {
                    x: scopePicker.mirrored ? VLCStyle.dp(8, VLCStyle.scale)
                                            : scopePicker.width - VLCStyle.dp(8, VLCStyle.scale) - width
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
                    leftPadding: mirrored ? 0 : VLCStyle.dp(12, VLCStyle.scale)
                    rightPadding: mirrored ? VLCStyle.dp(12, VLCStyle.scale) : 0
                    highlighted: scopePicker.highlightedIndex === index
                    contentItem: Text {
                        text: modelData.label
                        color: index === scopePicker.currentIndex ? VLCStyle.redesign.accent
                                                                  : VLCStyle.redesign.color("#8F8F8F", "#2e2e2e")
                        font: scopePicker.font
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                    }
                    background: Rectangle {
                        radius: VLCStyle.dp(4, VLCStyle.scale)
                        color: parent.highlighted || parent.hovered ? VLCStyle.redesign.color("#222222", "#9c9c9b") : "transparent"
                    }
                }
                popup: T.Popup {
                    // a press outside only closes it (on the box too: it doesn't open
                    // again), and goes no further, as in a native list
                    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside
                    modal: true
                    dim: false
                    y: scopePicker.height + VLCStyle.dp(4, VLCStyle.scale)
                    width: scopePicker.width
                    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding
                    padding: VLCStyle.dp(2, VLCStyle.scale)
                    contentItem: Item {
                        implicitWidth: popupList.implicitWidth
                        implicitHeight: popupList.implicitHeight

                        ListView {
                            id: popupList
                            anchors.fill: parent
                            // a popup: right to left as its box
                            LayoutMirroring.enabled: scopePicker.mirrored
                            LayoutMirroring.childrenInherit: true
                            clip: true
                            implicitHeight: contentHeight
                            model: scopePicker.popup.visible ? scopePicker.delegateModel : null
                            currentIndex: scopePicker.highlightedIndex
                        }

                        // the wheel moves it by whole rows
                        Widgets.RowSnapWheel {
                            anchors.fill: popupList
                            view: popupList
                        }
                    }
                    background: Rectangle {
                        radius: VLCStyle.dp(6, VLCStyle.scale)
                        color: VLCStyle.redesign.color("#191919", "#ababaa")
                        // the frame of the main window
                        border.width: 1
                        border.color: VLCStyle.redesign.color(VLCStyle.redesign.windowBorder)
                    }
                }
            }
        }
    ]

    // ---- "Frame 124": padding 20 / 20 / 22 / 20 -------------------------

    Item {
        id: content

        // (mirrored as the title bar: the actions on the right, right to left)
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
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

            // (the x of the design, from the right right to left)
            FigmaText {
                x: LayoutMirroring.enabled ? parent.width - width : 0
                width: content.labelWidth
                text: qsTr("Action")
                color: VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
            }
            // "Frame 503": Action 488 wide, Hotkey 332, then Global Hotkey;
            // in a narrower window, moved with the fields under them (the
            // Hotkey field by half the width lost, the Global one by all)
            FigmaText {
                readonly property real designX: VLCStyle.dp(488, VLCStyle.scale) - Math.round((root.preferredWidth - root.width) / 2)
                x: LayoutMirroring.enabled ? parent.width - width - designX : designX
                width: VLCStyle.dp(332, VLCStyle.scale)
                text: qsTr("Hotkey")
                color: VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
            }
            FigmaText {
                readonly property real designX: VLCStyle.dp(488 + 332, VLCStyle.scale) - Math.round(root.preferredWidth - root.width)
                x: LayoutMirroring.enabled ? parent.width - width - designX : designX
                // to the end of the row (a longer translation is cut short)
                width: Math.max(0, Math.min(implicitWidth, parent.width - designX))
                text: qsTr("Global Hotkey")
                color: VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
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
                required property bool hasGlobal
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
                    color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                    font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                }

                Item {
                    y: shortcutRow.headerHeight
                    width: parent.width
                    height: VLCStyle.dp(33, VLCStyle.scale)

                    FigmaText {
                        id: actionText
                        x: LayoutMirroring.enabled ? parent.width - width : 0
                        y: VLCStyle.dp(4, VLCStyle.scale)
                        // (never under the hotkey field, in a narrow window)
                        width: Math.max(0, Math.min(content.labelWidth, content.hotkeyX - VLCStyle.dp(8, VLCStyle.scale)))
                        text: shortcutRow.label
                        // the whole name when it is cut short
                        HoverHandler { id: actionHover }
                        T.ToolTip.visible: actionHover.hovered && actionText.truncated
                        T.ToolTip.text: actionText.text
                        T.ToolTip.delay: 650
                        color: shortcutRow.dirty ? VLCStyle.redesign.accent
                                                 : VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
                    }
                    KeyField {
                        objectName: "hotkey_" + shortcutRow.key
                        x: LayoutMirroring.enabled ? parent.width - width - content.hotkeyX : content.hotkeyX
                        // Tab down the list scrolls it: the row stays in
                        // sight and the next rows get made
                        onActiveFocusChanged: if (activeFocus) shortcutList.positionViewAtIndex(shortcutRow.index, ListView.Contain)
                        keyList: shortcutRow.hotkeys
                        accessibleName: qsTr("Hotkey of %1").arg(shortcutRow.label)
                        onAddRequested: root.beginCapture(shortcutRow.key, false, shortcutRow.label)
                        onRemoveRequested: (key) => shortcuts.removeKey(shortcutRow.key, false, key)
                    }
                    KeyField {
                        objectName: "globalHotkey_" + shortcutRow.key
                        // (the keys of live TV are the window's only)
                        visible: shortcutRow.hasGlobal
                        x: LayoutMirroring.enabled ? parent.width - width - content.globalX : content.globalX
                        onActiveFocusChanged: if (activeFocus) shortcutList.positionViewAtIndex(shortcutRow.index, ListView.Contain)
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
            color: VLCStyle.redesign.accent
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
}
