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
import VLC.Widgets as Widgets

// Modal dialog of the redesign (Figma "Open network stream", "new playlist",
// "Keyboard Shortcuts", "Add Subtitles to Playlist/Folder"): 14 px corners,
// "Frame 120" (the title bar: padding 9, the title row of 32 and what goes
// under it 12 below), "Frame 124" (the content, padding 22 / 20, 20 apart)
// and "Frame 225" (72 high, padding 20: Cancel on the left, the action on
// the right, 100 x 32).
T.Popup {
    id: root

    property string title
    property string acceptText: qsTr("Save")
    // the button on the left (a question: No, Don't restore...)
    property string cancelText: qsTr("Cancel")
    // a dialog with one action (Close) shows no Cancel
    property bool cancelVisible: true
    // a dialog with Cancel only (a progress), or no button at all
    property bool acceptVisible: true
    // the close button of the title bar (a progress that cannot be
    // cancelled has none)
    property bool closeButtonVisible: true
    property bool acceptEnabled: true
    // a second action before the main one (Report a problem: Copy), none
    // when empty
    property string extraText: ""
    property bool darkTheme: VLCStyle.palette.isDark
    // 1048 for Open network stream, 713 for New Playlist
    property real preferredWidth: VLCStyle.dp(1048, VLCStyle.scale)
    // "Frame 124" of a fixed height (Open network stream: 509; Add
    // Subtitles, Keyboard Shortcuts: 632), else as high as its content
    property real bodyHeight: -1
    // the paddings of "Frame 124" (Add Subtitles lays its own out)
    property real bodyHorizontalPadding: VLCStyle.dp(20, VLCStyle.scale)
    property real bodyVerticalPadding: VLCStyle.dp(22, VLCStyle.scale)
    // "Dark/BG/A3 Transparent" (#191919 95 %), Add Subtitles "Dark/BG/B1"
    // (#0E0E0E 95 %): as Figma shows them over its canvas (#191919, #0F0F0F)
    property color cardColor: VLCStyle.redesign.color("#191919", "#ABABAA", root.darkTheme)
    // Content, laid out in a column
    default property alias content: body.data
    // what the title bar holds under the title (Add Subtitles, Keyboard
    // Shortcuts: a row of fields 34 high)
    property alias headerContent: headerExtra.data

    property bool _previousGlobalShortcuts: true

    signal accepted()
    signal extraClicked()

    // false: the dialog closes itself once its action is done (Add Subtitles)
    property bool closeOnAccept: true
    // The field the keyboard goes to (the title of New Playlist): the design
    // opens the dialog with nothing focused (no caret); Tab, or the first
    // letter typed (which it gets), goes into it
    property Item initialField: null

    function accept() {
        if (!root.acceptEnabled)
            return
        root.accepted()
        if (root.closeOnAccept)
            root.close()
    }

    parent: T.Overlay.overlay
    // (no parent for a moment while it is created or destroyed)
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: parent ? Math.min(parent.width - 2 * VLCStyle.dp(24, VLCStyle.scale), preferredWidth) : preferredWidth
    height: parent ? Math.min(parent.height - 2 * VLCStyle.dp(24, VLCStyle.scale), implicitHeight) : implicitHeight
    // The templates do not size the popup from its content
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 0
    modal: true
    focus: true
    closePolicy: T.Popup.CloseOnEscape

    onOpened: {
        _previousGlobalShortcuts = MainCtx.useGlobalShortcuts
        MainCtx.useGlobalShortcuts = false
        if (initialField)
            openingFocus.forceActiveFocus()
    }
    onClosed: MainCtx.useGlobalShortcuts = _previousGlobalShortcuts

    T.Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.45)
    }

    // shadow 0 8 15 #000 46 %
    background: Rectangle {
        radius: VLCStyle.dp(14, VLCStyle.scale)
        color: root.cardColor

        Widgets.RoundedRectangleShadow {
            yOffset: VLCStyle.dp(8, VLCStyle.scale)
            blurRadius: 7.5 * VLCStyle.scale // (dp() rounds to whole pixels)
            color: Qt.rgba(0, 0, 0, 0.46)
        }
    }

    // the frame of the main window, over the content (which reaches the
    // card's edges): the card reads as a window over the interface
    readonly property Item _windowBorder: Widgets.RedesignWindowBorder {
        parent: root.background ? root.background.parent : null
        radius: VLCStyle.dp(14, VLCStyle.scale)
        darkTheme: root.darkTheme
    }

    contentItem: ColumnLayout {
        // right to left (Hebrew, Arabic...): a popup is not under the
        // mirrored interface, its content mirrors itself (the action on the
        // left, Cancel on the right)
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        spacing: 0

        // holds the keyboard while nothing shows focused (initialField)
        Item {
            id: openingFocus
            Layout.preferredWidth: 0
            Layout.preferredHeight: 0
            Keys.onPressed: (event) => {
                const field = root.initialField
                if (!field)
                    return
                if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
                    field.forceActiveFocus(Qt.TabFocusReason)
                    event.accepted = true
                } else if (event.text.length > 0 && event.text.charCodeAt(0) >= 32
                           && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier))) {
                    field.forceActiveFocus()
                    field.insert(field.cursorPosition, event.text)
                    event.accepted = true
                }
            }
        }

        // the controls (scroll bars, fields) look their colours up their
        // parent items: the popup is not one, its content holds the context
        readonly property ColorContext colorContext: ColorContext {
            colorSet: ColorContext.Window
            palette: VLCStyle.palette
        }

        // "Frame 120": "Dark/BG/B2", padding 9, 12 between its rows
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: headerColumn.implicitHeight + 2 * VLCStyle.dp(9, VLCStyle.scale)
            topLeftRadius: VLCStyle.dp(14, VLCStyle.scale)
            topRightRadius: topLeftRadius
            color: VLCStyle.redesign.color("#090909", "#dedede", root.darkTheme)

            ColumnLayout {
                id: headerColumn

                x: VLCStyle.dp(9, VLCStyle.scale)
                y: VLCStyle.dp(9, VLCStyle.scale)
                width: parent.width - 2 * x
                spacing: VLCStyle.dp(12, VLCStyle.scale)

                // "Frame 207": the title (padding left 10) and the close
                // button, 32 high
                Item {
                    Layout.fillWidth: true
                    implicitHeight: VLCStyle.dp(32, VLCStyle.scale)

                    Text {
                        id: titleText

                        // up to the close button (on the left in a right
                        // to left layout)
                        anchors.left: parent.left
                        anchors.leftMargin: VLCStyle.dp(10, VLCStyle.scale)
                        anchors.right: closeButton.visible ? closeButton.left : parent.right
                        height: parent.height
                        text: root.title
                        color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e", root.darkTheme)
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignTop
                        // the text box (17) centred in the row
                        topPadding: VLCStyle.redesign.textBaseline(Math.round((height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                                   font.pixelSize) - titleMetrics.ascent
                        FontMetrics {
                            id: titleMetrics
                            font: titleText.font
                        }
                    }

                    T.Button {
                        id: closeButton

                        anchors.right: parent.right
                        visible: root.closeButtonVisible
                        width: VLCStyle.dp(32, VLCStyle.scale)
                        height: VLCStyle.dp(32, VLCStyle.scale)
                        padding: VLCStyle.dp(4, VLCStyle.scale)

                        Accessible.name: qsTr("Close")

                        background: Rectangle {
                            radius: VLCStyle.dp(4, VLCStyle.scale)
                            color: (closeButton.hovered || closeButton.down)
                                   ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground,
                                                             "#9c9c9b", root.darkTheme)
                                   : "transparent"
                            border.width: closeButton.visualFocus ? 1 : 0
                            border.color: VLCStyle.redesign.focusColor
                        }
                        contentItem: Image {
                            source: "qrc:///redesign/icons/close/default.png"
                            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                            fillMode: Image.Pad
                            smooth: false
                        }
                        onClicked: root.close()
                    }
                }

                ColumnLayout {
                    id: headerExtra

                    Layout.fillWidth: true
                    visible: children.length > 0
                    spacing: 0
                }
            }
        }

        // "Frame 124": padding 22 / 20, 20 between its items
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: root.bodyHeight >= 0 ? root.bodyHeight
                                                 : body.implicitHeight + 2 * root.bodyVerticalPadding

            ColumnLayout {
                id: body

                x: root.bodyHorizontalPadding
                y: root.bodyVerticalPadding
                width: parent.width - 2 * x
                height: root.bodyHeight >= 0 ? Math.min(implicitHeight, parent.height - 2 * y)
                                             : parent.height - 2 * y
                spacing: VLCStyle.dp(20, VLCStyle.scale)
            }
        }

        // "Frame 225": padding 20, space between
        Item {
            Layout.fillWidth: true
            visible: root.cancelVisible || root.acceptVisible || root.extraText !== ""
            implicitHeight: VLCStyle.dp(72, VLCStyle.scale)

            DialogButton {
                objectName: "dialogCancelButton"
                visible: root.cancelVisible
                anchors.left: parent.left
                anchors.leftMargin: VLCStyle.dp(20, VLCStyle.scale)
                y: VLCStyle.dp(20, VLCStyle.scale)
                text: root.cancelText
                onClicked: root.close()
            }

            DialogButton {
                objectName: "dialogExtraButton"
                visible: root.extraText !== ""
                anchors.right: acceptButton.visible ? acceptButton.left : parent.right
                anchors.rightMargin: VLCStyle.dp(acceptButton.visible ? 8 : 20, VLCStyle.scale)
                y: VLCStyle.dp(20, VLCStyle.scale)
                text: root.extraText
                onClicked: root.extraClicked()
            }

            DialogButton {
                id: acceptButton
                objectName: "dialogAcceptButton"
                visible: root.acceptVisible
                anchors.right: parent.right
                anchors.rightMargin: VLCStyle.dp(20, VLCStyle.scale)
                y: VLCStyle.dp(20, VLCStyle.scale)
                text: root.acceptText
                accent: true
                enabled: root.acceptEnabled
                onClicked: root.accept()
            }
        }
    }

    // Figma "Cancel" / "Save": 100 x 32, radius 6, Inter SemiBold 14;
    // "Cancel" #1B1B1B and #8F8F8F, "Save" #371F06 and the accent; a
    // disabled "Save" is a grey one (#222222) at half its opacity
    component DialogButton: T.Button {
        id: button

        property bool accent: false

        width: Math.max(VLCStyle.dp(100, VLCStyle.scale),
                        buttonText.implicitWidth + 2 * VLCStyle.dp(16, VLCStyle.scale))
        height: VLCStyle.dp(32, VLCStyle.scale)
        opacity: enabled ? 1 : 0.5

        Accessible.name: text

        // Figma: the text box (its advance rounded up) centred
        contentItem: Item {
            Text {
                id: buttonText

                x: (parent.width - width) / 2
                width: Math.ceil(exactMetrics.advanceWidth / 16 - text.length * font.pixelSize / 2816 - 0.001)
                height: parent.height
                text: button.text
                verticalAlignment: Text.AlignTop
                color: (button.accent && button.enabled) ? VLCStyle.redesign.accent
                                                         : VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e", root.darkTheme)
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                font.weight: Font.DemiBold
                topPadding: VLCStyle.redesign.textBaseline(Math.round((button.height - Math.round(1.2109375 * font.pixelSize)) / 2),
                                                           font.pixelSize) - buttonMetrics.ascent
                FontMetrics {
                    id: buttonMetrics
                    font: buttonText.font
                }
                // the advance unrounded: measured 16 times larger
                TextMetrics {
                    id: exactMetrics
                    font.family: buttonText.font.family
                    font.weight: buttonText.font.weight
                    font.pixelSize: buttonText.font.pixelSize * 16
                    text: buttonText.text
                }
            }
        }

        background: Rectangle {
            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: (button.accent && button.enabled)
                   ? (button.down || button.hovered ? Qt.lighter(VLCStyle.redesign.accentBackground, 1.15)
                                                    : VLCStyle.redesign.accentBackground)
                   : !button.enabled ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b", root.darkTheme)
                   : (button.down || button.hovered) ? VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b", root.darkTheme)
                   : VLCStyle.redesign.color(VLCStyle.redesign.playerControlBackground, "#ababaa", root.darkTheme)
            // The neutral fill is the card's (#1B1B1B on #191919, the same grey
            // in the light theme): the frame of the window around it, so the
            // button never melts into the dialog (docs/MASTER-PLAN.md §6)
            border.width: button.visualFocus || !button.accent ? 1 : 0
            border.color: button.visualFocus ? VLCStyle.redesign.focusColor
                                             : VLCStyle.redesign.color(VLCStyle.redesign.windowBorder, undefined, root.darkTheme)
        }
    }

    // Keeps the title bar of the window draggable while the dialog is open
    Widgets.CSDModalDragStrip { dialog: root }
}
