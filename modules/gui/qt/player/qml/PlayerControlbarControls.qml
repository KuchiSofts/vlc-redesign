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

pragma Singleton

import QtQml


import VLC.MainInterface
import VLC.PlayerControls
import VLC.Widgets as Widgets
import VLC.Style

QtObject {
    readonly property string controlPath : "qrc:///qt/qml/VLC/PlayerControls/"

    readonly property var controlList: [
        { id: ControlListModel.PLAY_BUTTON, file: "PlayButton.qml", label: VLCIcons.play_filled, text: qsTr("Play") },
        { id: ControlListModel.STOP_BUTTON, file: "StopButton.qml", label: VLCIcons.stop, icon: "qrc:///redesign/icons/stop/default.png", text: qsTr("Stop") },
        { id: ControlListModel.OPEN_BUTTON, file: "OpenButton.qml", label: VLCIcons.eject, text: qsTr("Open") },
        { id: ControlListModel.PREVIOUS_BUTTON, file: "PreviousButton.qml", label: VLCIcons.previous, text: qsTr("Previous") },
        { id: ControlListModel.NEXT_BUTTON, file: "NextButton.qml", label: VLCIcons.next, text: qsTr("Next") },
        { id: ControlListModel.SLOWER_BUTTON, file: "SlowerButton.qml", label: VLCIcons.slower, text: qsTr("Slower") },
        { id: ControlListModel.FASTER_BUTTON, file: "FasterButton.qml", label: VLCIcons.faster, text: qsTr("Faster") },
        { id: ControlListModel.FULLSCREEN_BUTTON, file: "FullscreenButton.qml", label: VLCIcons.fullscreen, icon: "qrc:///redesign/icons/fullscreen/default.png", text: qsTr("Fullscreen") },
        { id: ControlListModel.SETTINGS_BUTTON, file: "SettingsButton.qml", label: VLCIcons.settings, icon: "qrc:///redesign/icons/settings/default.png", text: qsTr("VLC Settings") },
        { id: ControlListModel.EXTENDED_BUTTON, file: "ExtendedSettingsButton.qml", label: VLCIcons.effect_filter, text: qsTr("Extended panel") },
        { id: ControlListModel.PLAYLIST_BUTTON, file: "PlaylistButton.qml", label: VLCIcons.playlist, text: qsTr("Play Queue") },
        { id: ControlListModel.SNAPSHOT_BUTTON, file: "SnapshotButton.qml", label: VLCIcons.snapshot, icon: "qrc:///redesign/icons/snapshot/default.png", text: qsTr("Snapshot") },
        { id: ControlListModel.RECORD_BUTTON, file: "RecordButton.qml", label: VLCIcons.record, icon: "qrc:///redesign/icons/record/default.png", text: qsTr("Record") },
        { id: ControlListModel.ATOB_BUTTON, file: "AtoBButton.qml", label: VLCIcons.atob, text: qsTr("A-B Loop") },
        { id: ControlListModel.FRAME_NEXT_BUTTON, file: "FrameNextButton.qml", label: VLCIcons.next_frame, text: qsTr("Next Frame") },
        { id: ControlListModel.FRAME_PREV_BUTTON, file: "FramePrevButton.qml", label: VLCIcons.previous_frame, text: qsTr("Previous Frame") },
        { id: ControlListModel.REVERSE_BUTTON, file: "ReverseButton.qml", label: VLCIcons.play_reverse, text: qsTr("Trickplay Reverse") },
        { id: ControlListModel.SKIP_BACK_BUTTON, file: "SkipBackButton.qml", label: VLCIcons.skip_back, icon: "qrc:///redesign/icons/backward-15-sec/default.png", text: qsTr("Backward 15s") },
        { id: ControlListModel.SKIP_FW_BUTTON, file: "SkipForwardButton.qml", label: VLCIcons.skip_for, icon: "qrc:///redesign/icons/forward-15-sec/default.png", text: qsTr("Forward 15s") },
        { id: ControlListModel.QUIT_BUTTON, file: "QuitButton.qml", label: VLCIcons.clear, icon: "qrc:///redesign/icons/quit/default.png", text: qsTr("Quit VLC") },
        { id: ControlListModel.RANDOM_BUTTON, file: "RandomButton.qml", label: VLCIcons.shuffle, icon: "qrc:///redesign/icons/shuffle/default.png", text: qsTr("Shuffle") },
        { id: ControlListModel.LOOP_BUTTON, file: "LoopButton.qml", label: VLCIcons.repeat_all, icon: "qrc:///redesign/icons/repeat/default.png", text: qsTr("Repeat") },
        { id: ControlListModel.INFO_BUTTON, file: "InfoButton.qml", label: VLCIcons.info, icon: "qrc:///redesign/icons/info/default.png", text: qsTr("Properties") },
        { id: ControlListModel.LANG_BUTTON, file: "LangButton.qml", label: VLCIcons.audiosub, text: qsTr("Open subtitles") },
        { id: ControlListModel.BOOKMARK_BUTTON, file: "BookmarkButton.qml", label: VLCIcons.bookmark, icon: "qrc:///redesign/icons/bookmark/default.png", text: qsTr("Bookmark") },
        { id: ControlListModel.CHAPTER_PREVIOUS_BUTTON, file: "ChapterPreviousButton.qml", label: VLCIcons.dvd_prev, text: qsTr("Previous chapter") },
        { id: ControlListModel.CHAPTER_NEXT_BUTTON, file: "ChapterNextButton.qml", label: VLCIcons.dvd_next, text: qsTr("Next chapter") },
        { id: ControlListModel.VOLUME, file: "VolumeWidget.qml", label: VLCIcons.volume_high, text: qsTr("Volume Widget") },
        { id: ControlListModel.NAVIGATION_BOX, file: "NavigationBoxButton.qml", label: VLCIcons.ic_fluent_arrow_move, text: qsTr("Navigation Box") },
        { id: ControlListModel.NAVIGATION_BUTTONS, file: "NavigationWidget.qml", label: VLCIcons.dvd_menu, text: qsTr("Navigation") },
        { id: ControlListModel.DVD_MENUS_BUTTON, file: "DvdMenuButton.qml", label: VLCIcons.dvd_menu, text: qsTr("DVD menus") },
        { id: ControlListModel.PROGRAM_BUTTON, file: "ProgramButton.qml", label: VLCIcons.tv, text: qsTr("Program Button") },
        { id: ControlListModel.TELETEXT_BUTTONS, file: "TeletextButton.qml", label: VLCIcons.tvtelx, text: qsTr("Teletext") },
        // the renderers of VLC, as a cast button (the id saved in the profiles)
        { id: ControlListModel.RENDERER_BUTTON, file: "CastButton.qml", label: VLCIcons.renderer, icon: "qrc:///redesign/icons/cast/default.png", text: qsTr("Cast to TV") },
        { id: ControlListModel.ASPECT_RATIO_COMBOBOX, file: "AspectRatioWidget.qml", label: VLCIcons.aspect_ratio, text: qsTr("Aspect Ratio") },
        { id: ControlListModel.WIDGET_SPACER, file: "SpacerWidget.qml", label: VLCIcons.space, icon: "", text: qsTr("Spacer") },
        { id: ControlListModel.WIDGET_SPACER_EXTEND, file: "ExpandingSpacerWidget.qml", label: VLCIcons.space, text: qsTr("Expanding Spacer") },
        { id: ControlListModel.PLAYER_SWITCH_BUTTON, file: "PlayerSwitchButton.qml", label: VLCIcons.fullscreen, text: qsTr("Switch Player") },
        { id: ControlListModel.ARTWORK_INFO, file: "ArtworkInfoWidget.qml", label: VLCIcons.info, text: qsTr("Artwork Info") },
        { id: ControlListModel.PLAYBACK_SPEED_BUTTON, file: "PlaybackSpeedButton.qml", label: "1x", icon: "qrc:///redesign/icons/speed/default.png", text: qsTr("Playback Speed") },
        { id: ControlListModel.HDR_BUTTON, file: "HDRButton.qml", label: "HDR", icon: "qrc:///redesign/icons/HDR/default.png", text: qsTr("HDR") },
        { id: ControlListModel.FILE_NAME_WIDGET, file: "FileNameWidget.qml", label: VLCIcons.info, icon: "qrc:///redesign/icons/info/default.png", text: qsTr("File Name") },
        { id: ControlListModel.ARTIST_NAME_WIDGET, file: "ArtistNameWidget.qml", label: VLCIcons.info, icon: "qrc:///redesign/icons/info/default.png", text: qsTr("Artist Name") },
        { id: ControlListModel.DURATION_WIDGET, file: "DurationWidget.qml", label: VLCIcons.time, text: qsTr("Duration") },
        { id: ControlListModel.FRAME_RATE_WIDGET, file: "FrameRateWidget.qml", label: VLCIcons.info, text: qsTr("FPS") },
        { id: ControlListModel.RESOLUTION_WIDGET, file: "ResolutionWidget.qml", label: VLCIcons.info, text: qsTr("Resolution") },
        { id: ControlListModel.ZOOM_WIDGET, file: "ZoomWidget.qml", label: VLCIcons.search, icon: "qrc:///redesign/icons/zoom/default.png", text: qsTr("Zoom") },
        { id: ControlListModel.FAVORITE_BUTTON, file: "FavoriteButton.qml", label: VLCIcons.bookmark, icon: "qrc:///redesign/icons/favorite/default.png", text: qsTr("Favorite") },
        { id: ControlListModel.DIVIDER_WIDGET, file: "DividerWidget.qml", label: VLCIcons.space, icon: "qrc:///redesign/icons/divider/default.png", text: qsTr("Divider") },
        { id: ControlListModel.HIGH_RESOLUTION_TIME_WIDGET, file: "HighResolutionTimeWidget.qml", label: VLCIcons.time, text: qsTr("High Resolution Time") },
        { id: ControlListModel.SKIP_FW_10_BUTTON, file: "SkipForward10Button.qml", label: VLCIcons.skip_for, icon: "qrc:///redesign/icons/forward-10-sec/default.png", text: qsTr("Forward 10s") },
        { id: ControlListModel.SKIP_BACK_10_BUTTON, file: "SkipBack10Button.qml", label: VLCIcons.skip_back, icon: "qrc:///redesign/icons/backward-10-sec/default.png", text: qsTr("Backward 10s") },
        { id: ControlListModel.PIP_BUTTON, file: "PipButton.qml", label: VLCIcons.fullscreen, icon: "qrc:///redesign/icons/PiP/default.png", text: qsTr("Picture in Picture") },
        // the ⋯ menu and the library toggle of the top bar, moved into the bar
        { id: ControlListModel.MENU_BUTTON, file: "MainMenuButton.qml", label: VLCIcons.ellipsis, icon: "qrc:///redesign/icons/more/default.png", text: qsTr("Menu") },
        { id: ControlListModel.BACK_BUTTON, file: "LibraryButton.qml", label: VLCIcons.playlist, icon: "qrc:///redesign/icons/sidebar/default.png", text: qsTr("Library") },
        { id: ControlListModel.RIGHT_SIDEBAR_BUTTON, file: "RightSidebarButton.qml", label: VLCIcons.playlist, icon: "qrc:///redesign/icons/right-sidebar/default.png", text: qsTr("Right Sidebar") },
        { id: ControlListModel.HOME_BUTTON, file: "HomeButton.qml", label: VLCIcons.home, icon: "qrc:///redesign/icons/home/default.png", text: qsTr("Go Home") },
        { id: ControlListModel.THUMBNAIL_WIDGET, file: "ThumbnailWidget.qml", label: VLCIcons.info, icon: "qrc:///redesign/icons/thumbnail/default.png", text: qsTr("Thumbnail") },
        { id: ControlListModel.FOLDER_NAME_WIDGET, file: "FolderNameWidget.qml", label: VLCIcons.info, icon: "qrc:///redesign/icons/info/default.png", text: qsTr("Folder Name") },
        { id: ControlListModel.PLAYLIST_NAME_WIDGET, file: "PlaylistNameWidget.qml", label: VLCIcons.info, icon: "qrc:///redesign/icons/info/default.png", text: qsTr("Playlist Name") },
        { id: ControlListModel.REPLAY_BUTTON, file: "ReplayButton.qml", label: VLCIcons.skip_back, icon: "qrc:///redesign/icons/replay/default.png", text: qsTr("Replay") },
        { id: ControlListModel.CHAPTERS_BUTTON, file: "ChaptersButton.qml", label: VLCIcons.bookmark, icon: "qrc:///redesign/icons/chapters/default.png", text: qsTr("Chapters") },
        { id: ControlListModel.BOOKMARKS_BUTTON, file: "BookmarksButton.qml", label: VLCIcons.bookmark, icon: "qrc:///redesign/icons/bookmarks/default.png", text: qsTr("Bookmarks") },
        { id: ControlListModel.PLAYLISTS_BUTTON, file: "PlaylistsButton.qml", label: VLCIcons.playlist, icon: "qrc:///redesign/icons/playlist/default.png", text: qsTr("Playlists") }
    ]

    // Figma "Customize Player": the names of the palette broken as the design
    // breaks them
    readonly property var paletteText: ({
        [ControlListModel.SKIP_FW_10_BUTTON]: qsTr("Forward
10s"),
        [ControlListModel.SKIP_BACK_10_BUTTON]: qsTr("Backward
10s"),
        [ControlListModel.SKIP_FW_BUTTON]: qsTr("Forward
15s"),
        [ControlListModel.SKIP_BACK_BUTTON]: qsTr("Backward
15s")
    })

    // Figma "Customize Player": the items of the palette, in its order
    readonly property var paletteList: [
        ControlListModel.WIDGET_SPACER, ControlListModel.DIVIDER_WIDGET, ControlListModel.PLAYBACK_SPEED_BUTTON,
        ControlListModel.SKIP_FW_10_BUTTON, ControlListModel.SKIP_BACK_10_BUTTON, ControlListModel.SKIP_FW_BUTTON,
        ControlListModel.SKIP_BACK_BUTTON, ControlListModel.STOP_BUTTON, ControlListModel.LOOP_BUTTON,
        ControlListModel.RANDOM_BUTTON, ControlListModel.RECORD_BUTTON, ControlListModel.SNAPSHOT_BUTTON,
        ControlListModel.PIP_BUTTON, ControlListModel.HDR_BUTTON, ControlListModel.ZOOM_WIDGET,
        ControlListModel.RIGHT_SIDEBAR_BUTTON, ControlListModel.QUIT_BUTTON, ControlListModel.SETTINGS_BUTTON,
        ControlListModel.HOME_BUTTON, ControlListModel.THUMBNAIL_WIDGET, ControlListModel.FILE_NAME_WIDGET,
        ControlListModel.ARTIST_NAME_WIDGET, ControlListModel.FOLDER_NAME_WIDGET, ControlListModel.PLAYLIST_NAME_WIDGET,
        ControlListModel.INFO_BUTTON, ControlListModel.REPLAY_BUTTON, ControlListModel.CHAPTERS_BUTTON,
        ControlListModel.FAVORITE_BUTTON, ControlListModel.BOOKMARK_BUTTON, ControlListModel.BOOKMARKS_BUTTON,
        ControlListModel.PLAYLISTS_BUTTON,
        // not in the design's palette: after its 31 items (Figma 2107:5604)
        ControlListModel.RENDERER_BUTTON
    ]

    function control(id) {
        const entry = controlList.find( function(e) { return ( e.id === id ) } )

        if (entry === undefined) {
            console.warn("control delegate id " + id +  " doesn't exist")
            return { source: controlPath + "Fallback.qml" }
        }

        entry.source = controlPath + entry.file

        return entry
    }
}
