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
import QtQuick.Layouts
import QtQml.Models

import VLC.MainInterface
import VLC.MediaLibrary

import VLC.Widgets as Widgets
import VLC.Style

Widgets.PageLoader {
    id: root

    property int displayMarginBeginning: 0
    property int displayMarginEnd: 0
    property bool enableBeginningFade: true
    property bool enableEndFade: true

    //---------------------------------------------------------------------------------------------
    // Settings
    //---------------------------------------------------------------------------------------------

    pageModel: [{
        name: "all",
        default: true,
        component: componentAll
    }, {
        name: "list",
        component: componentList
    }]

    //---------------------------------------------------------------------------------------------
    // Childs
    //---------------------------------------------------------------------------------------------

    Component {
        id: componentAll

        Widgets.PageExt {
            id: playlistPage

            property alias initialIndex: playlistView.initialIndex

            title: qsTr("Playlists")
            libraryHeader: true
            libraryItemCount: playlistView.model.count

            headerAction: Component {
                RowLayout {
                    spacing: VLCStyle.dp(12, VLCStyle.scale)

                    // Figma library toolbar: "+" creates, "⋯" acts on the selection
                    Widgets.ButtonExt {
                        objectName: "videoNewPlaylistButton"
                        text: qsTr("New playlist")
                        colorContext.palette: VLCStyle.palette
                        padding: 0
                        contentItem: Image {
                            source: "qrc:///redesign/add.svg"
                            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                        }
                        onClicked: playlistView.openCreateDialog()
                    }

                    Widgets.ButtonExt {
                        id: actionsButton

                        objectName: "videoPlaylistActionsButton"
                        text: qsTr("Playlist actions")
                        colorContext.palette: VLCStyle.palette
                        padding: 0
                        contentItem: Image {
                            source: "qrc:///redesign/icons/more/default.png"
                            sourceSize: Qt.size(VLCStyle.redesign.iconSize, VLCStyle.redesign.iconSize)
                        }
                        // under the button, from its right edge in a right to
                        // left layout (a menu is not mirrored by itself)
                        onClicked: actionsMenu.popup(actionsButton,
                                                     LayoutMirroring.enabled ? actionsButton.width - actionsMenu.width : 0,
                                                     actionsButton.height)

                        Widgets.RedesignMenu {
                            id: actionsMenu

                            menuWidth: VLCStyle.dp(240, VLCStyle.scale)

                            Widgets.RedesignMenuItem {
                                objectName: "videoRenamePlaylistButton"
                                text: qsTr("Rename")
                                iconName: "edit"
                                enabled: playlistView.selectionModel.selectedIndexes.length === 1
                                onTriggered: playlistView.model.showDialogRename(
                                                 playlistView.selectionModel.selectedIndexes[0])
                            }
                            Widgets.RedesignMenuItem {
                                objectName: "videoDeletePlaylistButton"
                                text: qsTr("Delete")
                                iconName: "remove"
                                enabled: playlistView.selectionModel.hasSelection
                                onTriggered: playlistView.deleteSelectedPlaylists()
                            }
                        }
                    }
                }
            }

            PlaylistMediaList {
                id: playlistView

                anchors.fill: parent

                focus: true

                isMusic: false
                // the Playlists page of the sidebar: every playlist, songs
                // included (as the compact library of the player)
                playlistType: MLPlaylistListModel.PLAYLIST_TYPE_ALL

                searchPattern: playlistPage.search.pattern
                sortOrder: playlistPage.sort.order
                sortCriteria: playlistPage.sort.criteria

                displayMarginBeginning: root.displayMarginBeginning
                displayMarginEnd: root.displayMarginEnd

                enableBeginningFade: root.enableBeginningFade
                enableEndFade: root.enableEndFade

                onCurrentIndexChanged: History.viewProp.initialIndex = currentIndex

                onShowList: (model, reason) => {
                    History.push([...root.pagePrefix, "list"], { parentId: model.id, title: model.name }, reason)
                }
            }
        }

    }

    Component {
        id: componentList

        PlaylistMediaDisplay {
            id: playlist

            isMusic: false

            searchPattern: playlist.search.pattern
            sortOrder: playlist.sort.order
            sortCriteria: playlist.sort.criteria

            displayMarginBeginning: root.displayMarginBeginning
            displayMarginEnd: root.displayMarginEnd

            enableBeginningFade: root.enableBeginningFade
            enableEndFade: root.enableEndFade

            onCurrentIndexChanged: History.viewProp.initialIndex = currentIndex
        }
    }
}
