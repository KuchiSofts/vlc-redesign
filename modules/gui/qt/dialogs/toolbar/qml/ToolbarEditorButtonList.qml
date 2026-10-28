/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts

import VLC.Player
import VLC.PlayerControls
import VLC.Style
import VLC.Widgets as Widgets
import VLC.Util

// Figma "Customize Player" ("Frame 235"): the items of the palette, 86 wide,
// 24 apart, rows 38 apart (each as high as its highest item)
Flickable {
    id: root
    objectName: "toolbarEditorControlCatalog"

    clip: true
    contentWidth: width
    contentHeight: grid.implicitHeight
    boundsBehavior: Flickable.StopAtBounds

    T.ScrollBar.vertical: Widgets.ScrollBarExt { }

    property alias removeInfoRectVisible: removeInfoRect.visible

    signal controlDragStarted(int id)
    signal controlDragStopped(int id)

    readonly property ColorContext colorContext: ColorContext {
        id: theme
        colorSet: ColorContext.View
    }

    // the items with a box of 28 in the design (32 for the others)
    readonly property var _shortBoxes: [
        ControlListModel.SKIP_FW_BUTTON, ControlListModel.SKIP_BACK_BUTTON, ControlListModel.QUIT_BUTTON,
        ControlListModel.SETTINGS_BUTTON, ControlListModel.HOME_BUTTON, ControlListModel.THUMBNAIL_WIDGET,
        ControlListModel.FILE_NAME_WIDGET, ControlListModel.ARTIST_NAME_WIDGET, ControlListModel.FOLDER_NAME_WIDGET,
        ControlListModel.PLAYLIST_NAME_WIDGET, ControlListModel.INFO_BUTTON, ControlListModel.REPLAY_BUTTON,
        ControlListModel.CHAPTERS_BUTTON, ControlListModel.FAVORITE_BUTTON, ControlListModel.BOOKMARKS_BUTTON
    ]

    MouseArea {
        anchors.fill: parent
        z: -1
        preventStealing: true
    }

    property Component implicitFlickableScrollHandler: DefaultFlickableScrollHandler { }

    // NOTE: This property can be set to null to prevent using a scroll handler:
    property FlickableScrollHandler scrollHandler: {
        if (interactive) {
            // JS ownership:
            return implicitFlickableScrollHandler.createObject(null, { target: root })
        } else {
            return null
        }
    }

    // a control dropped here leaves the player
    DropArea {
        id: dropArea
        width: root.width
        height: Math.max(root.height, root.contentHeight)
        z: 3

        function isFromList() {
            return drag.source.objectName === "buttonsList"
        }

        onDropped: (drop) => {
            if (isFromList())
                return

            if (drop.source.objectName === "playerControl") {
                // after the event: the zone left empty deletes the control
                const fromZone = drop.source.zoneModel
                const from = drop.source.sourceRow
                Qt.callLater(() => {
                    if (from >= 0 && from < fromZone.count)
                        fromZone.remove(from)
                })
                drop.accept(Qt.MoveAction)
                return
            }
            drop.source.dndView.model.remove(drop.source.DelegateModel.itemsIndex)
            drop.accept(Qt.MoveAction)
        }
    }

    // the controls of the player dragged over the palette
    Binding {
        target: removeInfoRect
        property: "visible"
        value: true
        when: dropArea.containsDrag && dropArea.drag.source && dropArea.drag.source.objectName === "playerControl"
    }

    Rectangle {
        id: removeInfoRect
        width: root.width
        height: root.height
        z: 2
        visible: false
        radius: VLCStyle.dp(6, VLCStyle.scale)
        color: VLCStyle.palette.isDark ? Qt.rgba(14 / 255, 14 / 255, 14 / 255, 0.85)
                                       : Qt.rgba(0xc2 / 255, 0xc2 / 255, 0xc1 / 255, 0.85)

        Widgets.DashedRect {
            anchors.fill: parent
            color: VLCStyle.redesign.accent
        }

        Text {
            anchors.centerIn: parent
            width: Math.min(implicitWidth, parent.width - VLCStyle.dp(32, VLCStyle.scale))
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: qsTr("Drop here to remove from the player")
            color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
            font.family: VLCStyle.redesign.fontFamily
            font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
        }
    }

    MouseArea {
        width: root.width
        height: Math.max(root.height, root.contentHeight)
        z: 1
        visible: buttonDragItem.Drag.active
        cursorShape: visible ? Qt.DragMoveCursor : Qt.ArrowCursor
    }

    // its own width: the columns 86 wide, 24 apart (no spare width spread);
    // right to left, from the right edge (the items mirrored in it)
    GridLayout {
        id: grid
        x: LayoutMirroring.enabled ? root.width - width : 0
        columns: Math.max(1, Math.floor((root.width + columnSpacing) / (VLCStyle.dp(86, VLCStyle.scale) + columnSpacing)))
        columnSpacing: VLCStyle.dp(24, VLCStyle.scale)
        rowSpacing: VLCStyle.dp(38, VLCStyle.scale)

        Repeater {
            model: PlayerControlbarControls.paletteList

            delegate: MouseArea {
                id: item

                required property int modelData
                readonly property var entry: PlayerControlbarControls.control(modelData)
                readonly property int mIndex: modelData
                readonly property bool shortBox: root._shortBoxes.indexOf(modelData) >= 0

                Layout.preferredWidth: VLCStyle.dp(86, VLCStyle.scale)
                Layout.preferredHeight: column.implicitHeight
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft

                hoverEnabled: true
                cursorShape: Qt.OpenHandCursor
                objectName: "buttonsList"

                drag.target: buttonDragItem
                drag.smoothed: false

                drag.onActiveChanged: {
                    if (drag.active) {
                        root.controlDragStarted(mIndex)
                        buttonDragItem.text = item.entry.label ?? ""
                        buttonDragItem.iconSource = item.entry.icon ?? ""
                        buttonDragItem.Drag.source = this
                        buttonDragItem.Drag.start()
                    } else {
                        buttonDragItem.Drag.drop()
                        root.controlDragStopped(mIndex)
                    }
                }

                // the item under the cursor, where it drops
                onPressed: (mouse) => {
                    const pos = mapToItem(buttonDragItem.parent, mouseX, mouseY)
                    buttonDragItem.x = pos.x - buttonDragItem.width / 2
                    buttonDragItem.y = pos.y - buttonDragItem.height / 2
                }

                Column {
                    id: column
                    width: parent.width
                    spacing: VLCStyle.dp(16, VLCStyle.scale)

                    // Figma "Icon Wrapper": the dashed box, padding 4 (2 for
                    // the boxes of 28), the icon of 24; 28 wide in the
                    // palette (measured on 2107:5604)
                    Item {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: item.modelData === ControlListModel.WIDGET_SPACER ? VLCStyle.dp(24, VLCStyle.scale)
                                                                               : VLCStyle.dp(28, VLCStyle.scale)
                        height: item.shortBox ? VLCStyle.dp(28, VLCStyle.scale) : VLCStyle.dp(32, VLCStyle.scale)

                        Rectangle {
                            anchors.fill: parent
                            radius: VLCStyle.dp(4, VLCStyle.scale)
                            visible: item.containsMouse && !buttonDragItem.Drag.active
                            color: VLCStyle.redesign.color(VLCStyle.redesign.controlHoverBackground, "#9c9c9b")
                        }

                        Widgets.DashedRect {
                            anchors.fill: parent
                            color: VLCStyle.redesign.color("#6B6B6B", "#6b6b6b")
                        }

                        Image {
                            x: Math.round((parent.width - width) / 2)
                            y: item.shortBox ? VLCStyle.dp(2, VLCStyle.scale) : VLCStyle.dp(4, VLCStyle.scale)
                            width: VLCStyle.redesign.iconSize
                            height: width
                            visible: source.toString().length > 0
                            source: item.entry.icon ?? ""
                            sourceSize: Qt.size(width, height)
                            fillMode: Image.Pad
                            // filtered: the dialog sits on a half pixel as in
                            // the design (whole pixels: the same as unfiltered)
                            smooth: true
                        }
                    }

                    Text {
                        id: label
                        width: parent.width
                        text: PlayerControlbarControls.paletteText[item.modelData] ?? (item.entry.text ?? "")
                        color: VLCStyle.redesign.color(VLCStyle.redesign.textMuted, "#2e2e2e")
                        font.family: VLCStyle.redesign.fontFamily
                        font.pixelSize: VLCStyle.dp(14, VLCStyle.scale)
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        // lines of 17 (Inter 14, line height 100 %)
                        lineHeightMode: Text.FixedHeight
                        lineHeight: Math.round(1.2109375 * font.pixelSize)
                    }
                }
            }
        }
    }
}
