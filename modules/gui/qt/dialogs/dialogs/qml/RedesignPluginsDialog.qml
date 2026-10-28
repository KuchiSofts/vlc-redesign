/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import VLC.MainInterface
import VLC.Dialogs
import VLC.Style
import VLC.Widgets as Widgets

// Plugins and extensions (the Tools menu, Settings ▸ Extensions): the
// modules of VLC, searchable and sorted, and the Lua extensions. The
// add-ons are in Settings ▸ Extensions.
Widgets.RedesignDialog {
    id: root

    objectName: "pluginsDialog"

    // 0: the plugins, 1: the extensions
    property int page: 0

    title: qsTr("Plugins and extensions")
    acceptText: qsTr("Close")
    cancelVisible: false
    // the extensions read again from their folders
    extraText: page === 1 ? qsTr("Reload extensions") : ""
    onExtraClicked: extensions.reload()
    preferredWidth: VLCStyle.dp(1048, VLCStyle.scale)
    bodyHeight: VLCStyle.dp(560, VLCStyle.scale)

    PluginListModel {
        id: plugins
        searchPattern: search.text
    }

    ExtensionInfoModel {
        id: extensions
        ctx: MainCtx
    }

    onOpened: search.forceActiveFocus()
    onVisibleChanged: DialogsProvider.setRedesignDialogVisible("pluginDialog", visible)

    headerContent: [
        RowLayout {
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            Layout.fillWidth: true
            spacing: VLCStyle.dp(10, VLCStyle.scale)

            SettingsSegmented {
                Layout.preferredWidth: Math.max(VLCStyle.dp(320, VLCStyle.scale), neededWidth)
                options: [{ text: qsTr("Plugins (%1)").arg(plugins.total), value: 0 },
                          { text: qsTr("Extensions (%1)").arg(extensions.count), value: 1 }]
                value: root.page
                onChosen: (value) => root.page = value
            }

            Item { Layout.fillWidth: true }

            // the order of the plugins
            SettingsSegmented {
                visible: root.page === 0
                Layout.preferredWidth: Math.max(VLCStyle.dp(320, VLCStyle.scale), neededWidth)
                options: [{ text: qsTr("Name"), value: "name" },
                          { text: qsTr("Capability"), value: "capability" },
                          { text: qsTr("Score"), value: "score" }]
                value: plugins.sortCriteria
                onChosen: (value) => plugins.sortCriteria = value
            }
        }
    ]

    // the search of the plugins
    Widgets.RedesignTextArea {
        id: search

        objectName: "pluginsSearch"
        visible: root.page === 0
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        placeholderText: qsTr("Search by name or capability")
        Accessible.name: qsTr("Search plugins")
    }

    // The plugins: the name and its short name, the capability, the score
    ListView {
        id: pluginList

        objectName: "pluginsList"
        visible: root.page === 0
        Layout.fillWidth: true
        Layout.fillHeight: true
        // (as tall as the body leaves: the body takes its height from
        // what it holds)
        Layout.preferredHeight: VLCStyle.dp(1000, VLCStyle.scale)
        clip: true
        model: plugins
        boundsBehavior: Flickable.StopAtBounds
        keyNavigationEnabled: true
        activeFocusOnTab: true
        T.ScrollBar.vertical: Widgets.ScrollBarExt { }

        // the names of the columns
        header: ListRow {
            width: pluginList.width
            heading: true
            first: qsTr("Name")
            second: qsTr("Capability")
            third: qsTr("Score")
        }

        delegate: ListRow {
            required property int index
            required property string name
            required property string shortName
            required property string capability
            required property int score
            required property string help

            width: pluginList.width
            first: name
            firstDetail: shortName !== name ? shortName : ""
            second: capability
            third: String(score)
            current: pluginList.currentIndex === index && pluginList.activeFocus
            toolTipText: help
            onPressed: pluginList.currentIndex = index
        }

        Text {
            anchors.centerIn: parent
            visible: pluginList.count === 0
            text: qsTr("No plugin matches \"%1\"").arg(search.text)
            color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
    }

    // The extensions: the title, the summary, the version and the author
    ListView {
        id: extensionList

        objectName: "extensionsList"
        visible: root.page === 1
        Layout.fillWidth: true
        Layout.fillHeight: true
        // (as tall as the body leaves: the body takes its height from
        // what it holds)
        Layout.preferredHeight: VLCStyle.dp(1000, VLCStyle.scale)
        clip: true
        model: extensions
        spacing: VLCStyle.dp(8, VLCStyle.scale)
        boundsBehavior: Flickable.StopAtBounds
        T.ScrollBar.vertical: Widgets.ScrollBarExt { }

        delegate: Rectangle {
            id: card

            required property string title
            required property string summary
            required property string version
            required property string author
            required property string url
            required property string fileName

            width: extensionList.width
            implicitHeight: cardColumn.implicitHeight + 2 * VLCStyle.dp(12, VLCStyle.scale)
            radius: VLCStyle.dp(6, VLCStyle.scale)
            color: VLCStyle.redesign.color("#151515", "#b5b5b4")

            ColumnLayout {
                id: cardColumn

                LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
                LayoutMirroring.childrenInherit: true
                x: VLCStyle.dp(16, VLCStyle.scale)
                y: VLCStyle.dp(12, VLCStyle.scale)
                width: parent.width - 2 * x
                spacing: VLCStyle.dp(4, VLCStyle.scale)

                RowLayout {
                    Layout.fillWidth: true
                    spacing: VLCStyle.dp(10, VLCStyle.scale)
                    Text {
                        Layout.fillWidth: true
                        text: card.title
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        color: VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(16, VLCStyle.scale)
                    }
                    Text {
                        text: [card.version !== "" ? qsTr("Version %1").arg(card.version) : "",
                               card.author].filter((part) => part !== "").join(" · ")
                        color: VLCStyle.redesign.color("#6B6B6B", "#4d4d4d")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: text !== ""
                    text: card.summary
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignLeft
                    color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                }
                Text {
                    Layout.fillWidth: true
                    text: card.url !== "" ? "<a href=\"%1\">%2</a> · %3".arg(card.url).arg(card.url).arg(card.fileName)
                                          : card.fileName
                    textFormat: card.url !== "" ? Text.StyledText : Text.PlainText
                    linkColor: VLCStyle.redesign.accent
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignLeft
                    color: VLCStyle.redesign.color("#6B6B6B", "#4d4d4d")
                    font.family: VLCStyle.redesign.fontFamily
                    font.pixelSize: VLCStyle.dp(12, VLCStyle.scale)
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                }
            }
        }

        Text {
            anchors.centerIn: parent
            width: parent.width
            visible: extensionList.count === 0
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("No extension is installed. Extensions are Lua scripts in the lua/extensions folder of VLC or of your profile.")
            color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
    }

    // A row of the plugins: three columns (the name 50 %, the capability,
    // the score 80 at the end), 36 high, a line under it
    component ListRow: Item {
        id: row

        property bool heading: false
        property bool current: false
        property string first
        property string firstDetail
        property string second
        property string third
        property string toolTipText: ""

        signal pressed()

        implicitHeight: VLCStyle.dp(36, VLCStyle.scale)

        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        Rectangle {
            anchors.fill: parent
            radius: VLCStyle.dp(4, VLCStyle.scale)
            color: row.current || rowMouse.containsMouse
                   ? VLCStyle.redesign.color("#151515", "#b5b5b4") : "transparent"
            border.width: row.current ? 1 : 0
            border.color: VLCStyle.redesign.focusColor
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: VLCStyle.dp(10, VLCStyle.scale)
            anchors.rightMargin: VLCStyle.dp(10, VLCStyle.scale)
            spacing: VLCStyle.dp(16, VLCStyle.scale)

            Text {
                Layout.fillWidth: true
                Layout.preferredWidth: 5
                textFormat: Text.StyledText
                text: row.firstDetail === "" ? row._plain(row.first)
                                             : "%1 <font color=\"%2\">%3</font>".arg(row._plain(row.first))
                                                   .arg(VLCStyle.redesign.color("#6B6B6B", "#4d4d4d"))
                                                   .arg(row._plain(row.firstDetail))
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                color: row.heading ? VLCStyle.redesign.color("#6B6B6B", "#4d4d4d")
                                   : VLCStyle.redesign.color("#C6C6C6", "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(row.heading ? 12 : 14, VLCStyle.scale)
            }
            Text {
                Layout.fillWidth: true
                Layout.preferredWidth: 4
                text: row.second
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                color: row.heading ? VLCStyle.redesign.color("#6B6B6B", "#4d4d4d")
                                   : VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(row.heading ? 12 : 14, VLCStyle.scale)
            }
            Text {
                Layout.preferredWidth: VLCStyle.dp(64, VLCStyle.scale)
                text: row.third
                horizontalAlignment: Text.AlignRight
                color: row.heading ? VLCStyle.redesign.color("#6B6B6B", "#4d4d4d")
                                   : VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
                font.family: VLCStyle.redesign.fontFamily
                font.pixelSize: VLCStyle.dp(row.heading ? 12 : 14, VLCStyle.scale)
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: VLCStyle.redesign.color("#222222", "#9c9c9b")
        }

        MouseArea {
            id: rowMouse
            anchors.fill: parent
            enabled: !row.heading
            hoverEnabled: true
            onPressed: row.pressed()
        }

        // the description of the module, on hover
        Widgets.ToolTipExt {
            visible: rowMouse.containsMouse && row.toolTipText !== ""
            delay: 650
            text: row.toolTipText
        }

        function _plain(text) {
            return (text || "").replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
        }
    }
}
