/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import VLC.Dialogs
import VLC.MainInterface
import VLC.Player
import VLC.Style
import VLC.Widgets as Widgets

T.Popup {
    id: root
    objectName: "audioEffectsDialog"
    property int currentTab: 0
    property bool previousGlobalShortcuts: true
    property string profileError: ""
    readonly property var filterTabs: [
        { id: "equalizer", title: qsTr("Equalizer") },
        { id: "compressor", title: qsTr("Compressor") },
        { id: "spatializer", title: qsTr("Spatializer") },
        { id: "stereo_widen", title: qsTr("Stereo Widener") }
    ]

    function openNative(page) {
        if (audioBridge.transactionActive && !audioBridge.cancel()) return
        if (videoEffects.transactionActive && !videoEffects.cancel()) return
        root.close()
        DialogsProvider.nativeEffectsDialog(page)
    }

    function c(value) { return VLCStyle.redesign.color(value, value) }

    // The preview of each tab stays until Save or Cancel of the dialog
    function switchTab(index) {
        if (currentTab === index) return
        currentTab = index
        beginCurrentTab()
    }

    function beginCurrentTab() {
        if (currentTab === 0) audioBridge.begin()
        else videoEffects.begin()
    }

    function saveEffects() {
        if (audioBridge.transactionActive && !audioBridge.save()) { currentTab = 0; return false }
        if (videoEffects.transactionActive && !videoEffects.save()) { currentTab = 1; return false }
        return true
    }

    function cancelEffects() {
        const audioRestored = audioBridge.cancel()
        const videoRestored = videoEffects.cancel()
        return audioRestored && videoRestored
    }

    AudioEffects { id: audioBridge; ctx: MainCtx }
    VideoEffects { id: videoEffects; ctx: MainCtx }

    // Opened before playback (or before its video): the preview starts with
    // the output, or follows a new one
    Connections {
        target: Player
        enabled: root.opened
        function onPlayingStateChanged() {
            if (root.currentTab === 0 ? audioBridge.activeAudio : videoEffects.activeVideo)
                root.beginCurrentTab()
            presetField.refresh()
        }
        function onHasVideoOutputChanged() {
            if (root.currentTab === 1 && videoEffects.activeVideo)
                root.beginCurrentTab()
            presetField.refresh()
        }
    }

    width: Math.min(VLCStyle.dp(400, VLCStyle.scale), parent ? parent.width : VLCStyle.dp(400, VLCStyle.scale))
    height: Math.min(VLCStyle.dp(780, VLCStyle.scale), parent ? Math.max(0, parent.height - VLCStyle.dp(48, VLCStyle.scale)) : VLCStyle.dp(780, VLCStyle.scale))
    padding: 0
    modal: true
    focus: true
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside
    // in the middle of the window (the menu, a shortcut); off: placed by
    // whoever opens it (above the button of the control bar)
    property bool centered: true
    anchors.centerIn: centered ? T.Overlay.overlay : undefined
    // above the bar, the video stays as it is (the languages and tracks)
    dim: centered

    onOpened: {
        profileError = ""
        previousGlobalShortcuts = MainCtx.useGlobalShortcuts
        MainCtx.useGlobalShortcuts = false
        beginCurrentTab()
        presetField.refresh()
        tabs.itemAt(currentTab).forceActiveFocus()
    }
    onClosed: {
        if (audioBridge.transactionActive) audioBridge.cancel()
        if (videoEffects.transactionActive) videoEffects.cancel()
        MainCtx.useGlobalShortcuts = previousGlobalShortcuts
    }

    background: Rectangle {
        color: root.c(VLCStyle.redesign.windowBackground)
        radius: VLCStyle.dp(6, VLCStyle.scale)
        border.color: root.c(VLCStyle.redesign.separator)
    }

    contentItem: ColumnLayout {
        spacing: 0

        // a popup does not inherit the mirroring of the window (right to left)
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: VLCStyle.dp(54, VLCStyle.scale)
            Layout.leftMargin: VLCStyle.dp(20, VLCStyle.scale)
            Layout.rightMargin: VLCStyle.dp(12, VLCStyle.scale)
            Text {
                text: qsTr("Effects")
                Layout.fillWidth: true
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                color: root.c(VLCStyle.redesign.textPrimary)
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(18, VLCStyle.scale)
                font.weight: Font.Medium
            }
            T.Button {
                // the 32 of the design ("Close" runs a little past it): a
                // longer word widens it, into the room of the title
                implicitWidth: Math.max(32, closeLabel.implicitWidth - VLCStyle.dp(8, VLCStyle.scale))
                implicitHeight: 32
                text: qsTr("Close")
                focusPolicy: Qt.StrongFocus
                onClicked: root.close()
                contentItem: Text {
                    id: closeLabel
                    horizontalAlignment: Text.AlignLeft
                    text: parent.text
                    color: root.c(VLCStyle.redesign.textSecondary)
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(13, VLCStyle.scale)
                }
                background: Item {}
            }
        }

        // Figma 2372:8834 "Setting Preset": the presets of the effects, audio
        // and video together, above their tabs, 20 in as the panels
        EffectPresetField {
            id: presetField
            Layout.fillWidth: true
            Layout.leftMargin: VLCStyle.dp(20, VLCStyle.scale)
            Layout.rightMargin: VLCStyle.dp(20, VLCStyle.scale)
            Layout.bottomMargin: VLCStyle.dp(8, VLCStyle.scale)
            audio: audioBridge
            video: videoEffects
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 0
            Repeater {
                id: tabs
                model: [qsTr("Audio"), qsTr("Video")]
                delegate: T.Button {
                    id: tabButton
                    required property int index
                    required property string modelData
                    objectName: index === 0 ? "effectsAudioTab" : "effectsVideoTab"
                    Layout.fillWidth: true
                    implicitHeight: VLCStyle.dp(45, VLCStyle.scale)
                    focusPolicy: Qt.StrongFocus
                    onClicked: root.switchTab(index)
                    background: Rectangle {
                        color: tabButton.hovered ? root.c(VLCStyle.redesign.controlHoverBackground) : "transparent"
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width - VLCStyle.dp(32, VLCStyle.scale)
                            height: VLCStyle.dp(2, VLCStyle.scale)
                            color: root.currentTab === index ? VLCStyle.redesign.accent : "transparent"
                        }
                    }
                    contentItem: Text {
                        text: tabButton.modelData
                        color: root.currentTab === index ? VLCStyle.redesign.accent : root.c(VLCStyle.redesign.textMuted)
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: root.c(VLCStyle.redesign.separator) }

        Flickable {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: body.implicitHeight + VLCStyle.dp(40, VLCStyle.scale)
            boundsBehavior: Flickable.StopAtBounds
            T.ScrollBar.vertical: T.ScrollBar {}

            // The panels of the effects 20 px in, 360 px wide in the 400 px
            // dialog (Figma 536:1513)
            ColumnLayout {
                id: body
                width: scroll.width - VLCStyle.dp(40, VLCStyle.scale)
                x: VLCStyle.dp(20, VLCStyle.scale)
                y: VLCStyle.dp(20, VLCStyle.scale)
                spacing: VLCStyle.dp(20, VLCStyle.scale)

                Text {
                    Layout.fillWidth: true
                    visible: root.currentTab === 0 && !audioBridge.activeAudio
                    text: audioBridge.lastError || qsTr("Play audio to preview and edit audio effects.")
                    color: root.c(VLCStyle.redesign.textSecondary)
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignLeft
                }

                Repeater {
                    model: root.currentTab === 0 && audioBridge.transactionActive ? root.filterTabs : []
                    FilterSection {
                        required property var modelData
                        Layout.fillWidth: true
                        effects: audioBridge
                        filterId: modelData.id
                        title: modelData.title
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: root.currentTab === 1 && !videoEffects.activeVideo
                    text: videoEffects.lastError || qsTr("Play video to preview and edit effects.")
                    color: root.c(VLCStyle.redesign.textSecondary)
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignLeft
                }

                VideoEffectsPanel {
                    Layout.fillWidth: true
                    visible: root.currentTab === 1 && videoEffects.transactionActive
                    effects: videoEffects
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.currentTab === 1
                    spacing: VLCStyle.dp(16, VLCStyle.scale)
                    T.Button {
                        text: qsTr("Open synchronization")
                        Layout.fillWidth: true
                        implicitHeight: VLCStyle.dp(42, VLCStyle.scale)
                        onClicked: root.openNative(2)
                        background: Rectangle { color: root.c(VLCStyle.redesign.controlBackground); radius: VLCStyle.dp(6, VLCStyle.scale) }
                        contentItem: Text {
                            text: parent.text
                            color: root.c(VLCStyle.redesign.textPrimary)
                            font.family: VLCStyle.redesign.fontFamily
                            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: VLCStyle.dp(16, VLCStyle.scale)
            Layout.rightMargin: VLCStyle.dp(16, VLCStyle.scale)
            visible: !!root.profileError
                     || (root.currentTab === 0 ? audioBridge.lastError.length > 0 && audioBridge.transactionActive
                                               : videoEffects.lastError.length > 0 && videoEffects.transactionActive)
            text: root.profileError || (root.currentTab === 0 ? audioBridge.lastError : videoEffects.lastError)
            color: VLCStyle.redesign.color("#FF8989", "#B3261E")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            Accessible.role: Accessible.AlertMessage
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: root.c(VLCStyle.redesign.separator) }
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: VLCStyle.dp(60, VLCStyle.scale)
            Layout.leftMargin: VLCStyle.dp(16, VLCStyle.scale)
            Layout.rightMargin: VLCStyle.dp(16, VLCStyle.scale)
            spacing: VLCStyle.dp(8, VLCStyle.scale)
            T.Button {
                text: qsTr("Cancel")
                Layout.fillWidth: true
                implicitHeight: VLCStyle.dp(36, VLCStyle.scale)
                onClicked: {
                    if (root.cancelEffects()) root.close()
                }
                background: Rectangle { color: root.c(VLCStyle.redesign.controlBackground); radius: VLCStyle.dp(5, VLCStyle.scale) }
                contentItem: Text {
                    text: parent.text
                    color: root.c(VLCStyle.redesign.textSecondary)
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
            T.Button {
                text: qsTr("Save")
                Layout.fillWidth: true
                implicitHeight: VLCStyle.dp(36, VLCStyle.scale)
                enabled: !!root.profileError || audioBridge.transactionActive || videoEffects.transactionActive
                onClicked: {
                    if (!root.profileError && !root.saveEffects())
                        return
                    if (!MainCtx.controlbarProfileModel.save()) {
                        root.profileError = MainCtx.controlbarProfileModel.lastError
                        return
                    }
                    root.profileError = ""
                    root.close()
                }
                background: Rectangle { color: parent.enabled ? VLCStyle.redesign.accent : root.c(VLCStyle.redesign.controlBackground); radius: VLCStyle.dp(5, VLCStyle.scale) }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "#0E0E0E" : root.c(VLCStyle.redesign.textDisabled)
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
        }
    }

    // Keeps the title bar draggable while this modal dialog is open
    Widgets.CSDModalDragStrip { dialog: root }
}
