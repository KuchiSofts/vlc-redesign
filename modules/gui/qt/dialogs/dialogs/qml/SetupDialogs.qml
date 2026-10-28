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
                                 Qt.callLater(root.showSetupOnStart)
    }
    // VLC opened to play something (a file opened from Explorer): the film
    // first, the welcome the next time VLC opens without one (it covered
    // the picture of the first file played)
    function showSetupOnStart() {
        if (!setupState.playlistHasMedia())
            root.show(setup)
    }
    // What's new after an update: once a version, on the library once the
    // window is up (not over the welcome of a new install, which records the
    // version instead). VLC opened to play something: once it has stopped
    WhatsNewController { id: whatsNewState; ctx: MainCtx }
    function showWhatsNew() {
        if (!whatsNewState.pending || setupState.needsSetup
                || Player.playingState !== Player.PLAYING_STATE_STOPPED)
            return
        if (whatsNew.item && whatsNew.item.visible)
            return
        if (whatsNewState.showWanted())
            root.show(whatsNew)
    }
    // the library drawn first; a file opened with VLC waits for its end
    Timer {
        id: whatsNewTimer
        interval: 1500
        onTriggered: if (!setupState.playlistHasMedia()) root.showWhatsNew()
    }
    Connections {
        target: Player
        enabled: whatsNewState.pending
        function onPlayingStateChanged() {
            if (Player.playingState === Player.PLAYING_STATE_STOPPED)
                Qt.callLater(root.showWhatsNew)
        }
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
    // A dialog closed is let go with its pages and models, made again when
    // asked for (unless it was opened again meanwhile)
    function release(loader) {
        Qt.callLater(() => {
            if (loader.item && !loader.item.visible && loader.item.busy !== true)
                loader.active = false
        })
    }
    Component.onCompleted: {
        if (setupState.needsSetup)
            Qt.callLater(root.showSetupOnStart)
        else if (whatsNewState.pending)
            whatsNewTimer.start()
    }
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
        id: whatsNew
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/WhatsNewDialog.qml"
        onLoaded: {
            item.parent = root
            item.controller = whatsNewState
            item.closed.connect(() => root.release(whatsNew))
            item.open()
        }
    }
    Loader {
        id: preferences
        // the page asked for (a menu's Preferences: the page of what it is about)
        property string requestedCategory: "general"
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/SettingsDialog.qml"
        onLoaded: {
            item.parent = root
            // opened again on the page it was left on
            item.closed.connect(() => {
                preferences.requestedCategory = preferences.item.selectedCategory
                root.release(preferences)
            })
            item.openSettings(requestedCategory)
        }
    }
    Loader {
        id: shortcuts
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/ShortcutsDialog.qml"
        onLoaded: { item.parent = root; item.closed.connect(() => root.release(shortcuts)); item.open() }
    }
    Loader {
        id: networkStream
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/NetworkStreamDialog.qml"
        onLoaded: { item.parent = root; item.closed.connect(() => root.release(networkStream)); item.open() }
    }
    // Closed while its playlist was still being saved: let go once it is
    Connections {
        target: networkStream.item
        function onBusyChanged() { if (!networkStream.item.busy) root.release(networkStream) }
    }
    Loader {
        id: about
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/RedesignAboutDialog.qml"
        onLoaded: { item.parent = root; item.closed.connect(() => root.release(about)); item.open() }
    }
    // Report a problem (the ⋯ menu, About)
    Loader {
        id: report
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/RedesignReportDialog.qml"
        onLoaded: { item.parent = root; item.closed.connect(() => root.release(report)); item.open() }
    }
    Connections {
        target: RedesignFeedback
        function onReportRequested() { root.show(report) }
    }
    Loader {
        id: effects
        property int requestedTab: 0
        anchors.fill: parent
        active: false
        source: "qrc:/qt/qml/VLC/Dialogs/EffectsDialog.qml"
        onLoaded: {
            item.parent = root
            item.currentTab = requestedTab
            item.closed.connect(() => root.release(effects))
            item.open()
        }
    }
    Connections {
        target: setup.item
        function onVisibleChanged() { DialogsProvider.setRedesignDialogVisible("firstRunDialog", setup.item.visible) }
    }
    Connections {
        target: preferences.item
        function onVisibleChanged() { if (preferences.item) DialogsProvider.setRedesignDialogVisible("prefsDialog", preferences.item.visible) }
    }
    Connections {
        target: effects.item
        function onVisibleChanged() { if (effects.item) DialogsProvider.setRedesignDialogVisible("extendedDialog", effects.item.visible) }
    }
}
