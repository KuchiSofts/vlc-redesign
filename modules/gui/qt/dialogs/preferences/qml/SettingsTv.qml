/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick

import VLC.MainInterface
import VLC.Style

// Settings > TV, above its options: the sources of Live TV, each with what
// it holds and Update, then the dialog of the sources (add, edit, remove,
// their order, the catalog). Loaded by the dialog while the add-on is on.
Column {
    id: root

    function px(value) { return VLCStyle.dp(value, VLCStyle.scale) }
    function c(value) { return VLCStyle.redesign.color(value) }

    property var _dialog: null
    function openSources(page) {
        if (!_dialog) {
            const component = Qt.createComponent("qrc:///qt/qml/VLC/Network/TvSourceDialog.qml")
            if (component.status !== Component.Ready) {
                console.warn(component.errorString())
                return
            }
            // over the settings (a popup of the window)
            _dialog = component.createObject(root)
        }
        _dialog.openAt(page)
    }

    SettingsSectionTitle {
        width: parent.width
        text: qsTr("Sources")

        Repeater {
            model: Iptv.sources
            SettingsActionRow {
                required property var modelData
                width: root.width
                label: modelData.name + "  ·  "
                       + (modelData.busy ? qsTr("Updating…")
                          : modelData.error && modelData.channels === 0 ? modelData.error
                          : qsTr("%1 channel", "%1 channels", modelData.channels).arg(modelData.channels)
                            + (modelData.withGuide > 0 ? ", " + qsTr("%1 with a guide", "%1 with a guide",
                                                                     modelData.withGuide).arg(modelData.withGuide) : "")
                            + (modelData.catchupDays > 0 ? ", " + qsTr("catch-up %1 d", "catch-up %1 d",
                                                                       modelData.catchupDays).arg(modelData.catchupDays) : ""))
                buttonText: qsTr("Update")
                buttonWidth: px(100)
                onClicked: Iptv.refresh(modelData.id)
            }
        }

        SettingsActionRow {
            width: root.width
            label: Iptv.sources.length > 0 ? qsTr("Add, edit or remove sources") : qsTr("No source yet")
            buttonText: Iptv.sources.length > 0 ? qsTr("Manage Sources") : qsTr("Add a Source")
            onClicked: root.openSources(Iptv.sources.length > 0 ? "sources" : "link")
        }
        SettingsActionRow {
            width: root.width
            label: qsTr("Free channel lists")
            buttonText: qsTr("Browse the Catalog")
            onClicked: root.openSources("catalog")
        }
        // the languages the guides give (for Guide languages below)
        Text {
            width: root.width
            height: root.px(32)
            visible: Iptv.guideLanguages.length > 0
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
            text: qsTr("The guides give their programmes in: %1").arg(Iptv.guideLanguages.join(", "))
            color: root.c("#6B6B6B")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: root.px(12)
        }
    }
}
