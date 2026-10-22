/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or later.
 *****************************************************************************/
import QtQuick
import QtQuick.Templates as T

import VLC.Dialogs
import VLC.MainInterface
import VLC.Player
import VLC.Style
import VLC.Widgets as Widgets

// The "⋯" menu of the top bars (Figma "menu" and "Profiles"): opening media,
// recent media, profiles, and the player wide dialogs.
Widgets.RedesignMenu {
    id: root

    readonly property var profileModel: MainCtx.controlbarProfileModel

    // Open Recent: the media library keeps the history, and the user can
    // turn it off ("save-recentplay", as the classic menu)
    property bool _recentAllowed: false
    // Created only with a media library: without one the types of
    // VLC.MediaLibrary are not registered, and naming them here would break
    // this menu (used by the player too)
    property var _recentModel: null

    menuWidth: VLCStyle.dp(375, VLCStyle.scale)

    onAboutToShow: {
        _recentAllowed = MainCtx.mediaLibraryAvailable && Player.isRecentPlaySaved()
        if (_recentAllowed && !_recentModel) {
            _recentModel = Qt.createQmlObject("import VLC.MediaLibrary; MLRecentMediaModel { limit: 10 }",
                                              root, "MainMenuRecentModel")
            _recentModel.ml = MainCtx.mediaLibrary
        }
    }

    // The player is edited in place (Figma "Player - customize"): from the
    // library, show it so the editor has a bar to drop the controls on
    // shows the player first if needed (Dialogs.qml)
    function _customizePlayer() {
        DialogsProvider.showToolbarEditorDialog()
    }

    Widgets.RedesignMenuItem {
        text: qsTr("Open File(s)")
        iconName: "open"
        shortcutText: qsTr("Ctrl + O")
        onTriggered: DialogsProvider.simpleOpenDialog()
    }
    Widgets.RedesignMenuItem {
        text: qsTr("Open Folder")
        iconName: "folder-open"
        shortcutText: qsTr("Ctrl + F")
        onTriggered: DialogsProvider.PLOpenDir()
    }
    Widgets.RedesignMenuItem {
        text: qsTr("Open Network Stream")
        iconName: "streaming"
        shortcutText: qsTr("Ctrl + N")
        onTriggered: DialogsProvider.openNetDialog()
    }
    Widgets.RedesignMenuItem {
        text: qsTr("Open Disc")
        iconName: "disc"
        shortcutText: qsTr("Ctrl + D")
        onTriggered: DialogsProvider.openDiscDialog()
    }
    Widgets.RedesignMenuItem {
        text: qsTr("Open Capture Device")
        iconName: "capture-device"
        shortcutText: qsTr("Ctrl + C")
        onTriggered: DialogsProvider.openCaptureDialog()
    }
    Widgets.RedesignMenuItem {
        text: qsTr("Convert")
        iconName: "swap"
        shortcutText: qsTr("Ctrl + R")
        onTriggered: DialogsProvider.openAndTranscodingDialogs()
    }

    Widgets.RedesignMenu {
        id: recentMenu

        title: qsTr("Open Recent")
        iconName: "recent"
        enabled: root._recentAllowed && !!root._recentModel && root._recentModel.count > 0

        Instantiator {
            model: root._recentAllowed ? root._recentModel : null
            delegate: Widgets.RedesignMenuItem {
                required property var model
                text: model.title || model.mrl || ""
                onTriggered: MainCtx.mediaLibrary.addAndPlay([model.id])
            }
            onObjectAdded: (index, object) => recentMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => recentMenu.removeItem(object)
        }
    }

    // Figma "Profiles": a separator of 1 px with no line
    Widgets.RedesignMenuSeparator { line: false }

    Widgets.RedesignMenu {
        id: profileMenu

        title: qsTr("Profile (%1)").arg(root.profileModel.currentModel ? root.profileModel.currentModel.name
                                                                       : qsTr("None"))
        iconName: "profile"

        Instantiator {
            model: root.profileModel
            delegate: Widgets.RedesignMenuItem {
                required property int index
                required property string name
                text: name
                markable: true
                marked: root.profileModel.selectedProfile === index
                onTriggered: {
                    const previous = root.profileModel.selectedProfile
                    if (previous === index || !root.profileModel.setSelectedProfile(index))
                        return
                    // A profile that cannot be saved is not kept selected
                    if (!root.profileModel.save())
                        root.profileModel.setSelectedProfile(previous)
                }
            }
            onObjectAdded: (index, object) => profileMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => profileMenu.removeItem(object)
        }

        Widgets.RedesignMenuSeparator { }

        Widgets.RedesignMenuItem {
            text: qsTr("New")
            iconName: "add"
            // Opens the player editor and asks for the new profile's name
            onTriggered: {
                root._customizePlayer()
                MainCtx.controlbarProfileModel.requestNewProfile()
            }
        }
    }

    Widgets.RedesignMenuItem {
        text: qsTr("Keyboard Shortcuts")
        iconName: "hotkeys"
        onTriggered: DialogsProvider.shortcutsDialog()
    }
    Widgets.RedesignMenuItem {
        text: qsTr("Customize Player")
        iconName: "paint"
        onTriggered: root._customizePlayer()
    }
    Widgets.RedesignMenuItem {
        text: qsTr("Settings")
        iconName: "settings"
        shortcutText: qsTr("Ctrl + P")
        onTriggered: DialogsProvider.prefsDialog()
    }
    Widgets.RedesignMenuItem {
        text: qsTr("About VLC")
        iconName: "info"
        shortcutText: qsTr("Shift + F1")
        onTriggered: DialogsProvider.aboutDialog()
    }
}
