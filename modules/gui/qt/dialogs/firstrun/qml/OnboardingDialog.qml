/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic as Controls

import VLC.Dialogs as Dialogs
import VLC.MainInterface
import VLC.Style
import VLC.Util
import VLC.Widgets as Widgets

Controls.Popup {
    id: root
    objectName: "onboardingDialog"
    modal: true
    focus: true
    padding: 0
    // Escape closes it, except while the folder is added to the media library
    closePolicy: controller.busy ? Controls.Popup.NoAutoClose : Controls.Popup.CloseOnEscape
    // centred on whole pixels: the text stays crisp
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: Math.min(1090, parent ? parent.width - 24 : 1090)
    height: Math.min(759, parent ? parent.height - 24 : 759)

    property int step: 0
    property var folderUrl
    property bool metadataNetworkAccess: false
    property color initialAccent: "#d37918"
    property color selectedAccent: "#d37918"
    property bool previousGlobalShortcuts: true
    property bool committed: false
    property bool skipping: false
    property var shortcuts: []
    // the subtitle languages (SubtitlePreferences), saved at the end
    property string subtitlePrimary: "en"
    property string subtitleSecondary: ""
    property bool subtitleAutoSearch: true
    property bool subtitleShowBoth: false
    readonly property int lastStep: 3

    Dialogs.OnboardingController {
        id: controller
        ctx: MainCtx
        onCompleted: {
            if (!root.skipping) {
                MainCtx.setSettingValue("redesign/accent", root.selectedAccent.toString())
                SubtitlePreferences.set(root.subtitlePrimary, root.subtitleSecondary,
                                        root.subtitleAutoSearch, root.subtitleShowBoth)
            }
            root.committed = true
            root.close()
        }
    }
    Widgets.FilePicker { id: filePicker; ctx: MainCtx }

    onOpened: {
        committed = false
        skipping = false
        step = 0
        initialAccent = VLCStyle.redesign.accent
        selectedAccent = initialAccent
        // no folder without the media library (none can be added)
        folderUrl = MainCtx.mediaLibraryAvailable ? controller.suggestedFolder : ""
        // the setting as it is: the dialog asks nothing about it, the value
        // changes only when the user chooses (consentRequested)
        metadataNetworkAccess = controller.metadataNetworkAccess
        shortcuts = controller.activeShortcuts()
        subtitlePrimary = SubtitlePreferences.primary
        subtitleSecondary = SubtitlePreferences.secondary
        subtitleAutoSearch = SubtitlePreferences.autoSearch
        subtitleShowBoth = SubtitlePreferences.showBoth
        previousGlobalShortcuts = MainCtx.useGlobalShortcuts
        MainCtx.useGlobalShortcuts = false
        controller.begin()
    }
    onClosed: {
        if (!committed) {
            controller.cancel()
            VLCStyle.redesign.accent = initialAccent
        }
        MainCtx.useGlobalShortcuts = previousGlobalShortcuts
    }
    background: Rectangle {
        radius: 14
        color: "#0e0e0e"
        border.color: "#222222"
    }

    contentItem: Item {
        Item {
            id: stage
            width: 1090
            height: 759
            anchors.centerIn: parent
            scale: Math.min(root.width / width, root.height / height)

            WelcomeStep {
                anchors.fill: parent
                visible: root.step === 0
                folderUrl: root.folderUrl
                folderText: root.folderUrl ? controller.displayPath(root.folderUrl) : ""
                folderAvailable: MainCtx.mediaLibraryAvailable
                metadataNetworkAccess: root.metadataNetworkAccess
                onConsentRequested: allowed => root.metadataNetworkAccess = allowed
                onClearRequested: root.folderUrl = ""
                onBrowseRequested: {
                    const chosen = filePicker.chooseFolder(qsTr("Choose a media folder"))
                    if (chosen.toString() !== "")
                        root.folderUrl = chosen
                }
            }
            ThemeStep {
                anchors.fill: parent
                visible: root.step === 1
                themeIndex: controller.themeIndex
                accent: root.selectedAccent
                onThemeRequested: index => controller.previewTheme(index)
                onAccentRequested: colorValue => {
                    root.selectedAccent = colorValue
                    VLCStyle.redesign.accent = colorValue
                }
            }
            SubtitleLanguagesStep {
                anchors.fill: parent
                visible: root.step === 2
                primary: root.subtitlePrimary
                secondary: root.subtitleSecondary
                autoSearch: root.subtitleAutoSearch
                showBoth: root.subtitleShowBoth
                onPrimaryRequested: (code) => {
                    root.subtitlePrimary = code
                    if (root.subtitleSecondary === code)
                        root.subtitleSecondary = ""
                }
                onSecondaryRequested: (code) => {
                    root.subtitleSecondary = code === root.subtitlePrimary ? "" : code
                    if (root.subtitleSecondary === "")
                        root.subtitleShowBoth = false
                }
                onAutoSearchRequested: (on) => root.subtitleAutoSearch = on
                onShowBothRequested: (on) => root.subtitleShowBoth = on
            }
            ShortcutsStep {
                anchors.fill: parent
                visible: root.step === 3
                shortcuts: root.shortcuts
            }

            Text {
                x: 62
                y: 628
                width: parent.width - 124
                visible: controller.error !== ""
                text: controller.error
                color: "#e57373"
                wrapMode: Text.WordWrap
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: 14
            }

            OnboardingButton {
                x: 62
                y: 671
                // Figma: no Skip on the first run; Cancel when reopened
                visible: root.step === 0 && !controller.needsSetup
                text: qsTr("Cancel")
                objectName: "onboardingSkipButton"
                enabled: !controller.busy
                onClicked: {
                    if (!controller.needsSetup) {
                        root.close()
                        return
                    }
                    root.skipping = true
                    root.selectedAccent = root.initialAccent
                    VLCStyle.redesign.accent = root.initialAccent
                    controller.skip()
                }
            }
            // Figma 1902:6770: Back 8 before Finish
            OnboardingButton {
                x: 820
                y: 671
                visible: root.step > 0
                text: qsTr("Back")
                objectName: "onboardingBackButton"
                enabled: !controller.busy
                onClicked: --root.step
            }
            OnboardingButton {
                x: 928
                y: 671
                primary: true
                text: root.step === root.lastStep ? qsTr("Finish") : qsTr("Next")
                objectName: "onboardingNextButton"
                enabled: !controller.busy
                onClicked: {
                    if (root.step < root.lastStep) {
                        ++root.step
                        return
                    }
                    const selected = root.folderUrl && root.folderUrl.toString() !== ""
                                     ? [root.folderUrl] : []
                    controller.finish(selected, root.metadataNetworkAccess)
                }
            }
        }
    }

    // Keeps the title bar draggable while this modal dialog is open
    Widgets.CSDModalDragStrip { dialog: root }
}
