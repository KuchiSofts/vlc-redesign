/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later. */
import QtQuick
import VLC.Dialogs
import VLC.MainInterface
import VLC.Player
import VLC.Playlist

Item {
    id: root
    SleepTimerController { id: sleepTimer; player: Player; playlist: MainPlaylistController }
    PlayerMenu { id: playerMenu; parent: root; x: root.centeredX(width); y: root.centeredY(height) }
    SleepTimerMenu { id: timerMenu; parent: root; x: root.centeredX(width); y: root.centeredY(height); timerController: sleepTimer }
    AudioVisualizationDialog { id: visualization; parent: root; x: root.centeredX(width); y: root.centeredY(height) }
    HdrOutputDialog { id: hdr; parent: root; x: root.centeredX(width); y: root.centeredY(height) }
    OnboardingController {
        id: setupState
        ctx: MainCtx
        onNeedsSetupChanged: if (needsSetup && (!setup.item || !setup.item.visible))
                                 Qt.callLater(() => root.show(setup))
    }
    // centred on whole pixels: the text stays crisp
    function centeredX(width) { return Math.round((root.width - width) / 2) }
    function centeredY(height) { return Math.round((root.height - height) / 2) }
    function show(loader) {
        if (!loader.active) loader.active = true
        else if (loader.item) {
            if (!loader.item.visible) loader.item.open()
        }
    }
    Component.onCompleted: if (setupState.needsSetup) Qt.callLater(() => show(setup))
    Connections {
        target: DialogsProvider
        function onPreferencesRequested() { root.show(preferences) }
        function onPreferencesSectionRequested(category) {
            preferences.requestedCategory = category
            if (preferences.item)
                preferences.item.openSettings(category)
            else
                preferences.active = true
        }
        function onShortcutsRequested() { root.show(shortcuts) }
        function onNetworkStreamRequested() { root.show(networkStream) }
        function onAboutRequested() { root.show(about) }
        function onSetupRequested() { if (!setup.item || !setup.item.visible) root.show(setup) }
        function onEffectsRequested(page) {
            effects.requestedTab = page
            if (effects.item) effects.item.switchTab(page)
            root.show(effects)
        }
        function onPlayerMenuRequested() { playerMenu.open() }
        function onSleepTimerRequested() { timerMenu.open() }
        function onAudioVisualizationRequested() { visualization.open() }
        function onHdrOutputRequested() { hdr.open() }
    }
    Loader {
        id: setup
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/OnboardingDialog.qml"
        onLoaded: { item.parent = root; item.open() }
    }
    Loader {
        id: preferences
        // the page asked for (a menu's Preferences: the page of what it is about)
        property string requestedCategory: "general"
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/SettingsDialog.qml"
        onLoaded: { item.parent = root; item.openSettings(requestedCategory) }
    }
    Loader {
        id: shortcuts
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/ShortcutsDialog.qml"
        onLoaded: { item.parent = root; item.open() }
    }
    Loader {
        id: networkStream
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/NetworkStreamDialog.qml"
        onLoaded: { item.parent = root; item.open() }
    }
    Loader {
        id: about
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/RedesignAboutDialog.qml"
        onLoaded: { item.parent = root; item.open() }
    }
    Loader {
        id: effects
        property int requestedTab: 0
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/EffectsDialog.qml"
        onLoaded: { item.parent = root; item.currentTab = requestedTab; item.open() }
    }
    Connections {
        target: setup.item
        function onVisibleChanged() { DialogsProvider.setRedesignDialogVisible("firstRunDialog", setup.item.visible) }
    }
    Connections {
        target: preferences.item
        function onVisibleChanged() { DialogsProvider.setRedesignDialogVisible("prefsDialog", preferences.item.visible) }
    }
    Connections {
        target: effects.item
        function onVisibleChanged() { DialogsProvider.setRedesignDialogVisible("extendedDialog", effects.item.visible) }
    }
}
