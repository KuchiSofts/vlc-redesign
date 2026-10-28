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
import QtQuick.Templates as T

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
    // (a color, or "system": the Windows accent)
    property string initialAccent: "#d37918"
    property string selectedAccent: "#d37918"
    property bool previousGlobalShortcuts: true
    property bool committed: false
    property bool skipping: false
    property var shortcuts: []
    // the subtitle languages (SubtitlePreferences), saved at the end
    property string subtitlePrimary: "en"
    property string subtitleSecondary: ""
    property bool subtitleAutoSearch: true
    property bool subtitleShowBoth: false
    // What goes online by itself, asked once (a new profile; the installer
    // asks an installed copy): a step of its own, not in the design, before
    // the shortcuts (the last step of the design keeps its Finish)
    property bool askOnline: false
    property bool onlineUpdateCheck: true
    readonly property int onlineStep: askOnline ? 3 : -1
    readonly property int shortcutsStep: askOnline ? 4 : 3
    readonly property int lastStep: shortcutsStep

    Dialogs.OnboardingController {
        id: controller
        ctx: MainCtx
        onCompleted: {
            if (!root.skipping) {
                MainCtx.setSettingValue("redesign/accent", root.selectedAccent)
                SubtitlePreferences.set(root.subtitlePrimary, root.subtitleSecondary,
                                        root.subtitleAutoSearch, root.subtitleShowBoth)
                if (root.askOnline) {
                    MainCtx.setOnlineChoicesMade()
                    // (on: the check comes a few seconds later, as at a start)
                    RedesignUpdater.setAutomaticCheck(root.onlineUpdateCheck)
                }
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
        initialAccent = VLCStyle.redesign.accentChoice
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
        askOnline = MainCtx.onlineChoicesPending()
        onlineUpdateCheck = RedesignUpdater.automaticCheck()
        previousGlobalShortcuts = MainCtx.useGlobalShortcuts
        MainCtx.useGlobalShortcuts = false
        controller.begin()
    }
    onClosed: {
        if (!committed) {
            controller.cancel()
            VLCStyle.redesign.accentChoice = initialAccent
        }
        MainCtx.useGlobalShortcuts = previousGlobalShortcuts
    }
    background: Rectangle {
        radius: 14
        // Figma's onboarding is dark: the light theme as the other dialogs
        color: VLCStyle.redesign.color("#0e0e0e", "#c2c2c1")
        border.color: VLCStyle.redesign.color("#222222", "#9c9c9b")
    }

    contentItem: Item {
        // a popup does not inherit the mirroring of the window (right to left)
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

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
                accentChoice: root.selectedAccent
                onThemeRequested: index => controller.previewTheme(index)
                onAccentRequested: choice => {
                    root.selectedAccent = choice
                    VLCStyle.redesign.accentChoice = choice
                }
            }
            SubtitleLanguagesStep {
                anchors.fill: parent
                visible: root.step === 2
                primary: root.subtitlePrimary
                secondary: root.subtitleSecondary
                autoSearch: root.subtitleAutoSearch
                // asked with the update check on the next step
                showAutoSearch: !root.askOnline
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
            Item {
                id: onlineChoices
                objectName: "onboardingOnlineStep"
                anchors.fill: parent
                visible: root.step === root.onlineStep

                // on the dialog itself, as the shortcuts: the player behind is the
                // theme step's (its title bar and controls), an empty one looked unfinished

                Text {
                    x: Math.floor((parent.width - width) / 2)
                    y: 132
                    // (inside the player behind, a longer one cut)
                    width: Math.min(implicitWidth, parent.width - 124)
                    elide: Text.ElideRight
                    text: qsTr("Choose what VLC does online by itself.")
                    color: VLCStyle.redesign.color("#969696", "#4d4d4d")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: 24
                    font.weight: Font.Light
                }
                Text {
                    id: onlineIntro
                    x: Math.floor((parent.width - width) / 2)
                    y: 184
                    width: 700
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("No telemetry, no ads, no account. These two connect to the internet without being asked each time, and send only what is written under them.")
                    color: VLCStyle.redesign.color("#6f6f6f", "#5a5a59")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: 15
                    lineHeight: 1.3
                }

                // What follows moves down under longer texts (a translation),
                // never up from where the design puts it
                Column {
                    id: onlineColumn
                    y: Math.max(284, onlineIntro.y + onlineIntro.height + 16)
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 680
                    spacing: 28

                    OnlineChoice {
                        objectName: "onboardingUpdateCheck"
                        text: qsTr("Check for updates once a day")
                        detail: qsTr("Asks GitHub (api.github.com) for the latest release of VLC Redesign.")
                        checked: root.onlineUpdateCheck
                        onToggled: (on) => root.onlineUpdateCheck = on
                    }
                    OnlineChoice {
                        objectName: "onboardingOnlineSubtitles"
                        text: qsTr("Find subtitles online automatically")
                        detail: qsTr("When a video has none in your languages: its name and a fingerprint of the file to subtitle services (OpenSubtitles.org, Wizdom, Gestdown, YIFY Subtitles), its name to IMDb, Wikidata and Cinemeta to know the film.")
                        checked: root.subtitleAutoSearch
                        onToggled: (on) => root.subtitleAutoSearch = on
                    }
                }

                // the policy that lists all of it, on GitHub
                T.AbstractButton {
                    id: policyLink
                    objectName: "onboardingPrivacyPolicy"
                    x: Math.floor((parent.width - width) / 2)
                    y: Math.max(500, onlineColumn.y + onlineColumn.height + 16)
                    // (a template has no size of its own)
                    implicitWidth: implicitContentWidth
                    implicitHeight: implicitContentHeight
                    focusPolicy: Qt.StrongFocus
                    text: qsTr("Privacy and code signing policy")
                    Accessible.role: Accessible.Link
                    Accessible.name: text
                    contentItem: Text {
                        text: policyLink.text
                        color: VLCStyle.redesign.accent
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        font.underline: policyLink.hovered || policyLink.visualFocus
                    }
                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    onClicked: Qt.openUrlExternally(RedesignUpdater.repositoryUrl + "/blob/main/CODE_SIGNING_POLICY.md")
                }
                Text {
                    x: Math.floor((parent.width - width) / 2)
                    y: policyLink.y + 30
                    width: Math.min(implicitWidth, parent.width - 124)
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("Both stay in Settings: General for the updates, Subtitles for the subtitles.")
                    color: VLCStyle.redesign.color("#5a5a5a", "#6b6b6b")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: 13
                }
            }
            ShortcutsStep {
                anchors.fill: parent
                visible: root.step === root.shortcutsStep
                shortcuts: root.shortcuts
            }

            Text {
                x: 62
                y: 628
                width: parent.width - 124
                visible: controller.error !== ""
                text: controller.error
                color: VLCStyle.redesign.color("#e57373", "#c62828")
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: 14
            }

            // (right to left: Cancel on the right, Next on the left, Back
            // after it)
            OnboardingButton {
                x: LayoutMirroring.enabled ? parent.width - width - 62 : 62
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
                    VLCStyle.redesign.accentChoice = root.initialAccent
                    controller.skip()
                }
            }
            // Figma 1902:6770: Back 8 before Finish (wider buttons: a longer
            // word)
            OnboardingButton {
                x: LayoutMirroring.enabled ? 62 + nextButton.width + 8
                                           : parent.width - 62 - nextButton.width - 8 - width
                y: 671
                visible: root.step > 0
                text: qsTr("Back")
                objectName: "onboardingBackButton"
                enabled: !controller.busy
                onClicked: --root.step
            }
            OnboardingButton {
                id: nextButton
                x: LayoutMirroring.enabled ? 62 : parent.width - width - 62
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

    // A switch of the online step, and what it sends under its text
    component OnlineChoice: Column {
        id: choice
        property alias text: choiceToggle.text
        property alias checked: choiceToggle.checked
        property string detail
        signal toggled(bool on)
        width: parent ? parent.width : 680
        spacing: 6

        SubtitleLanguagesStep.ToggleRow {
            id: choiceToggle
            Accessible.description: choice.detail
            onToggled: choice.toggled(checked)
        }
        // under the text of the switch (right to left: 60 from the right)
        Text {
            width: parent.width
            leftPadding: LayoutMirroring.enabled ? 0 : 60
            rightPadding: LayoutMirroring.enabled ? 60 : 0
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            text: choice.detail
            color: VLCStyle.redesign.color("#6f6f6f", "#5a5a59")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: 13
            lineHeight: 1.25
        }
    }

    // Keeps the title bar draggable while this modal dialog is open
    Widgets.CSDModalDragStrip { dialog: root }
}
