/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.Dialogs
import VLC.MainInterface
import VLC.Style

SettingsSectionTitle {
    id: root
    text: qsTr("Appearance")
    property int selectedTheme: 0
    property int initialTheme: 0
    // a color, or "system": Follow the Windows accent color
    property string selectedAccent: VLCStyle.redesign.accentChoice
    property string initialAccent: VLCStyle.redesign.accentChoice
    property string selectedLanguage: "auto"
    property string initialLanguage: "auto"
    property bool selectedRightToLeft: false
    property bool initialRightToLeft: false
    readonly property var languages: themeBridge.interfaceLanguages()
    readonly property bool dark: VLCStyle.palette.isDark
    readonly property bool dirty: selectedTheme !== initialTheme
                                  || selectedAccent.toLowerCase() !== initialAccent.toLowerCase()
                                  || selectedLanguage !== initialLanguage
                                  || selectedRightToLeft !== initialRightToLeft

    readonly property var themeChoices: [
        { text: qsTr("Dark"), index: 2 },
        { text: qsTr("Light"), index: 1 },
        { text: qsTr("System"), index: 0 }
    ]
    readonly property var accentChoices: VLCStyle.redesign.accentChoices

    OnboardingController { id: themeBridge; ctx: MainCtx }

    function begin() {
        themeBridge.begin()
        initialTheme = themeBridge.themeIndex
        selectedTheme = initialTheme
        initialAccent = VLCStyle.redesign.accentChoice
        selectedAccent = initialAccent
        initialLanguage = themeBridge.interfaceLanguage()
        selectedLanguage = initialLanguage
        initialRightToLeft = MainCtx.rightToLeft
        selectedRightToLeft = initialRightToLeft
    }

    function chooseTheme(index) {
        selectedTheme = index
        themeBridge.previewTheme(index)
    }

    // The interface shows the language at once (Save keeps it, Cancel goes
    // back), as the theme, laid out in the direction of the language (its
    // catalog says it: Layout direction below changes it)
    function chooseLanguage(iso) {
        selectedLanguage = iso
        themeBridge.previewInterfaceLanguage(iso)
        chooseRightToLeft(MainCtx.languageRightToLeft())
    }

    // The layout, mirrored at once too
    function chooseRightToLeft(rightToLeft) {
        selectedRightToLeft = rightToLeft
        MainCtx.rightToLeft = rightToLeft
    }

    function chooseAccent(choice) {
        selectedAccent = choice
        VLCStyle.redesign.accentChoice = choice
    }

    // Follow the Windows accent color: on, the accent is the system's
    // ("system"); off, the color chosen before it (the default if none)
    readonly property bool followSystemAccent: selectedAccent === VLCStyle.redesign.systemAccentChoice
    function chooseFollowSystemAccent(on) {
        if (on)
            chooseAccent(VLCStyle.redesign.systemAccentChoice)
        else
            chooseAccent(initialAccent !== VLCStyle.redesign.systemAccentChoice ? initialAccent : "#D37918")
    }

    function resetDefaults() {
        chooseTheme(0)
        chooseAccent("#D37918")
        // English until one is chosen (Windows), else the system's
        // (laid out as the language)
        chooseLanguage(languages.length > 0 ? "en" : "auto")
    }

    function cancel() {
        themeBridge.cancel()
        selectedTheme = initialTheme
        selectedAccent = initialAccent
        VLCStyle.redesign.accentChoice = initialAccent
        if (selectedLanguage !== initialLanguage)
            chooseLanguage(initialLanguage)
        if (selectedRightToLeft !== initialRightToLeft)
            chooseRightToLeft(initialRightToLeft)
    }

    function save() {
        themeBridge.acceptTheme()
        MainCtx.setSettingValue("redesign/accent", selectedAccent)
        if (selectedLanguage !== initialLanguage)
            themeBridge.setInterfaceLanguage(selectedLanguage)
        initialTheme = selectedTheme
        initialAccent = selectedAccent
        initialLanguage = selectedLanguage
        if (selectedRightToLeft !== initialRightToLeft)
            MainCtx.setSettingValue("redesign/right-to-left", selectedRightToLeft)
        initialRightToLeft = selectedRightToLeft
    }

    function c(value) { return VLCStyle.redesign.color(value) }

    SettingsLanguageAction {
        width: parent.width
        languages: root.languages
        language: root.selectedLanguage
        changed: root.selectedLanguage !== root.initialLanguage
        onChosen: (iso) => root.chooseLanguage(iso)
    }

    // Layout direction: the one of the language chosen above, until changed
    // here (a right-to-left language can stay left to right, as some prefer)
    Item {
        width: parent.width
        height: root.px(46)
        Text {
            anchors.left: parent.left
            anchors.right: directionChoice.left
            anchors.rightMargin: root.px(12)
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
            text: qsTr("Layout direction")
            color: root.c("#8F8F8F")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
            // on the start side, whatever the script (mirrored right to left)
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }
        SettingsSegmented {
            id: directionChoice
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            // wider for a long translation, as far as the label leaves room
            width: Math.min(Math.max(root.px(320), neededWidth), parent.width * 0.62)
            options: [{ text: qsTr("Left to right"), value: false },
                      { text: qsTr("Right to left"), value: true }]
            value: root.selectedRightToLeft
            onChosen: (value) => root.chooseRightToLeft(value)
        }
    }

    // Theme: the Values component of the design (Dark, Light, System)
    Item {
        width: parent.width
        height: root.px(46)
        Text {
            anchors.left: parent.left
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
            text: qsTr("Theme")
            color: root.c("#8F8F8F")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
        }
        SettingsSegmented {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(Math.max(root.px(320), neededWidth), parent.width * 0.62)
            options: root.themeChoices.map(choice => ({ text: choice.text, value: choice.index }))
            value: root.selectedTheme
            onChosen: (value) => root.chooseTheme(value)
        }
    }

    // Accent color: 9 cells of 31.56 x 32 (6 px padding, 4 px gap) in a
    // 320 x 36 well; the chosen cell in Accent Bright, the others at 70 %
    Item {
        width: parent.width
        height: root.px(46)
        Text {
            anchors.left: parent.left
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
            text: qsTr("Accent color")
            color: root.c("#8F8F8F")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
        }
        Rectangle {
            id: accentWell
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(root.px(320), parent.width * 0.46)
            height: root.px(36)
            radius: root.px(6)
            color: root.c("#090909")

            Row {
                anchors.fill: parent
                anchors.margins: root.px(2)
                spacing: root.px(4)
                Repeater {
                    model: root.accentChoices
                    delegate: Button {
                        id: swatch
                        required property var modelData
                        readonly property bool chosen: root.selectedAccent.toLowerCase() === modelData.toLowerCase()
                        width: (parent.width - root.px(4) * (root.accentChoices.length - 1)) / root.accentChoices.length
                        height: parent.height
                        padding: root.px(6)
                        // (Basic adds 2 px each side otherwise: 16 wide, not 20)
                        horizontalPadding: root.px(6)
                        onClicked: root.chooseAccent(modelData)
                        Accessible.name: qsTr("Accent color %1").arg(modelData)
                        background: Rectangle {
                            radius: root.px(4)
                            color: swatch.chosen ? Qt.rgba(Qt.color(swatch.modelData).r * 0.26,
                                                           Qt.color(swatch.modelData).g * 0.26,
                                                           Qt.color(swatch.modelData).b * 0.26, 1)
                                                 : "transparent"
                            border.width: swatch.visualFocus ? 1 : 0
                            border.color: VLCStyle.redesign.accent
                        }
                        contentItem: Item {
                            Rectangle {
                                anchors.centerIn: parent
                                width: parent.width
                                height: root.px(20)
                                radius: root.px(1)
                                color: swatch.modelData
                                opacity: swatch.chosen ? 1.0 : 0.7
                            }
                        }
                    }
                }
            }
        }
    }

    // Follow the Windows accent color: a toggle row as Developer mode (42
    // high, the label at 14 in #8F8F8F, the Toggle on the end side); on, the
    // swatches above show no choice, and picking one turns it off
    Item {
        width: parent.width
        height: root.px(42)
        Text {
            anchors.left: parent.left
            anchors.right: followAccentToggle.left
            anchors.rightMargin: root.px(20)
            anchors.baseline: parent.verticalCenter
            anchors.baselineOffset: VLCStyle.redesign.centeredBaseline(font.pixelSize)
            text: followAccentToggle.label
            color: root.c("#8F8F8F")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(14)
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }
        SettingsToggle {
            id: followAccentToggle
            readonly property string label: Qt.platform.os === "windows" ? qsTr("Follow the Windows accent color")
                                                                        : qsTr("Follow the system accent color")
            objectName: "settingsFollowSystemAccent"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            checked: root.followSystemAccent
            onSwitched: (on) => root.chooseFollowSystemAccent(on)
            Accessible.name: label
        }
    }

    // Where VLC follows the system language (not Windows): the one of the
    // session it runs in (a new language of the system comes with a new
    // session)
    Text {
        id: languageNote
        visible: root.languages.length === 0
        width: parent.width
        text: qsTr("VLC follows the system language.")
        color: root.c("#6B6B6B")
        font.family: VLCStyle.redesign.fontFamily
        font.pixelSize: root.px(12)
        elide: Text.ElideRight
    }
}
