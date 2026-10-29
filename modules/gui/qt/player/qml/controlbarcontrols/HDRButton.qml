/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later. */
import QtQuick
import VLC.Dialogs
import VLC.Player
import VLC.Style
RedesignIconButton {
    id: root
    iconSource: "qrc:///redesign/icons/HDR/default.svg"
    text: "HDR"
    description: qsTr("HDR output")
    enabled: !paintOnly && Player.videoTracks.count > 0

    signal requestLockUnlockAutoHide(bool lock)
    signal menuOpened(var menu)

    // above the button, as the languages and tracks
    onClicked: panel.toggle()
    // open while its panel shows
    active: panel.opened

    function forceUnlock() {
        panel.close()
    }

    BarPanel {
        id: panel
        button: root
        sourceComponent: HdrOutputDialog { }
        onPanelOpened: (menu) => {
            root.requestLockUnlockAutoHide(true)
            root.menuOpened(menu)
        }
        onPanelClosed: {
            root.requestLockUnlockAutoHide(false)
            root.forceActiveFocus()
            root.menuOpened(null)
        }
    }
}
