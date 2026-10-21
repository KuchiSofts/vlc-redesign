/****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * SPDX-License-Identifier: GPL-2.0-or-later
 ****************************************************************************/
import QtQuick
import QtQuick.Controls.Basic
import VLC.MainInterface
import VLC.Network
import VLC.Style

// Add-ons of the design (Figma "Settings - extensions"): the title, the
// 784 x 194 list (#090909, 2 px #191919 border, radius 6), the search field
// (210 x 36) and the buttons (Find more extensions, Add, Remove: 32 px, 10 px
// apart) on a 53 px band, then Load add-ons on startup
SettingsSectionTitle {
    id: root
    text: qsTr("Add-ons")

    property var settingsModel
    property bool onlineRequested: false
    // Advanced Options of the page: only the installed add-ons
    property bool installedOnly: false
    readonly property var selectedAddon: addonsView.currentItem
    // The add-ons (null until the page first shows)
    readonly property var addons: addonsLoader.item

    function c(value) { return VLCStyle.redesign.color(value, value) }

    onInstalledOnlyChanged: addonsView.currentIndex = -1

    // The catalog of the add-ons loads (at once, it blocks) when the page
    // shows the first time, not each time Settings opens; kept once loaded
    Loader {
        id: addonsLoader
        active: false
        sourceComponent: ServicesDiscoveryModel {
            ctx: MainCtx
            typeFilter: ServicesDiscoveryModel.TYPE_NONE
            stateFilter: root.installedOnly
                         ? ServicesDiscoveryModel.STATE_INSTALLED
                         : ServicesDiscoveryModel.STATE_NONE
            searchPattern: searchField.text
        }
    }
    Binding {
        target: addonsLoader
        property: "active"
        value: true
        when: root.visible
        restoreMode: Binding.RestoreNone
    }

    Rectangle {
        width: parent.width
        height: root.px(194)
        radius: root.px(6)
        color: root.c("#090909")
        border.width: root.px(2)
        border.color: root.c("#191919")

        ListView {
            id: addonsView
            objectName: "settingsAddonsList"
            anchors.fill: parent
            anchors.margins: root.px(2)
            clip: true
            model: root.addons
            currentIndex: -1
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            delegate: Item {
                id: addonRow
                required property int index
                required property string name
                required property string summary
                required property string author
                required property int state
                required property bool manageable
                required property bool broken
                width: addonsView.width
                height: root.px(52)
                readonly property bool selected: addonsView.currentIndex === index

                Rectangle {
                    anchors.fill: parent
                    visible: addonRow.selected || rowHover.hovered
                    color: addonRow.selected ? root.c("#222222") : root.c("#151515")
                }
                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: root.px(12)
                    anchors.right: stateLabel.left
                    anchors.rightMargin: root.px(10)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: root.px(2)
                    Text {
                        width: parent.width
                        text: addonRow.name
                        color: addonRow.selected ? root.c("#EAEAEA") : root.c("#8F8F8F")
                        elide: Text.ElideRight
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(14)
                    }
                    Text {
                        width: parent.width
                        text: addonRow.summary || addonRow.author
                        color: root.c("#6B6B6B")
                        elide: Text.ElideRight
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: root.px(12)
                    }
                }
                Text {
                    id: stateLabel
                    anchors.right: parent.right
                    anchors.rightMargin: root.px(12)
                    anchors.verticalCenter: parent.verticalCenter
                    text: addonRow.broken ? qsTr("Broken")
                          : addonRow.state === ServicesDiscoveryModel.STATE_INSTALLED
                          ? qsTr("Installed")
                          : addonRow.state === ServicesDiscoveryModel.STATE_INSTALLING
                            ? qsTr("Installing…")
                            : addonRow.state === ServicesDiscoveryModel.STATE_UNINSTALLING
                              ? qsTr("Removing…") : ""
                    color: addonRow.state === ServicesDiscoveryModel.STATE_INSTALLED
                           ? VLCStyle.redesign.accent : root.c("#6B6B6B")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: root.px(12)
                }
                HoverHandler { id: rowHover }
                TapHandler { onTapped: addonsView.currentIndex = addonRow.index }
                ToolTip.visible: rowHover.hovered && !addonRow.manageable
                ToolTip.text: qsTr("This add-on was installed manually and cannot be managed here.")
                ToolTip.delay: 650
            }
        }

        BusyIndicator {
            anchors.centerIn: parent
            running: !!root.addons && root.addons.loading
            visible: running
            width: root.px(32)
            height: root.px(32)
        }
    }

    // 5 px above the band, 10 px below; the controls centred on it
    Item {
        width: parent.width
        height: root.px(53)

        Item {
            y: root.px(5)
            width: parent.width
            height: root.px(38)

            SettingsTextField {
                id: searchField
                objectName: "settingsAddonsSearch"
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: root.px(210)
                placeholderText: qsTr("Search")
                onTextChanged: addonsView.currentIndex = -1
                Accessible.name: qsTr("Search add-ons")
            }

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: root.px(10)

                // Hugs its text: 20 px on each side
                SettingsButton {
                    id: findButton
                    objectName: "settingsFindAddons"
                    width: implicitContentWidth + root.px(40)
                    text: qsTr("Find more extensions")
                    onClicked: {
                        if (!root.addons || root.addons.loading)
                            return
                        root.onlineRequested = true
                        addonsView.currentIndex = -1
                        root.addons.loadFromDefaultRepository()
                    }
                }
                SettingsButton {
                    id: addButton
                    objectName: "settingsInstallAddon"
                    text: qsTr("Add")
                    readonly property bool applicable: !!root.selectedAddon
                        && root.selectedAddon.manageable && !root.selectedAddon.broken
                        && root.selectedAddon.state === ServicesDiscoveryModel.STATE_NOTINSTALLED
                    onClicked: if (applicable) root.addons.installService(addonsView.currentIndex)
                    ToolTip.visible: hovered && !applicable
                    ToolTip.text: qsTr("Select an add-on that is not installed")
                    ToolTip.delay: 650
                }
                SettingsButton {
                    id: removeButton
                    objectName: "settingsRemoveAddon"
                    text: qsTr("Remove")
                    readonly property bool applicable: !!root.selectedAddon && root.selectedAddon.manageable
                        && root.selectedAddon.state === ServicesDiscoveryModel.STATE_INSTALLED
                    onClicked: if (applicable) root.addons.removeService(addonsView.currentIndex)
                    ToolTip.visible: hovered && !applicable
                    ToolTip.text: qsTr("Select an installed add-on")
                    ToolTip.delay: 650
                }
            }
        }
    }

    // Only while the page shows (a search lists every option of VLC), and
    // the editor of the one row shown only
    Repeater {
        model: root.visible ? root.settingsModel : null
        delegate: Loader {
            width: root.width
            // the other options of the page are under Advanced Options
            active: model.key === "qt-autoload-extensions"
            visible: active
            sourceComponent: SettingsOptionRow {
                settingKey: model.key ?? ""
                label: qsTr("Load add-ons on startup")
                help: model.help ?? ""
                showInfo: false
                settingType: model.type ?? ""
                settingValue: model.value
                choices: model.choices || []
                dirty: !!model.dirty
                onCommit: (value) => root.settingsModel.setValue(settingKey, value)
            }
        }
    }
}
