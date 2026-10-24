/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
import QtQuick

QtObject {
/* Labels and groups from the settings reference, for options backed by VLC.
   The table is built once: entry() is called for every row of a page (and
   for the rows before it), a table per call made the long pages slow. */
readonly property var items: ({
        // restart: what VLC can only read when it starts (its command line,
        // the interface it opens, its media library): "Applies the next time
        // VLC starts" under the row once it is changed. The other settings
        // apply at once (the log, the web remote...: LiveSettings) or to the
        // next media.
        "verbose": [qsTr("Diagnostics"), qsTr("Show minor warnings"), "inttoggle", null, { advanced: true, info: true }],
        // read by VLC as its command line at the start
        "extra-options": [qsTr("Diagnostics"), qsTr("Additional command line options"), "", null,
                          { advanced: true, info: true, restart: true }],
        "logmode": [qsTr("Diagnostics"), qsTr("Log format"), "", { "text": qsTr("Plain text"), "html": qsTr("HTML") },
                    { advanced: true, info: true }],
        "qt-error-dialogs": [qsTr("Startup"), qsTr("Show confirmation and error dialogs"), "", null, { info: true }],
        "redesign-update-check": [qsTr("Startup"), qsTr("Check for updates automatically"), "", null, { info: true }],
        "file-logging": [qsTr("Diagnostics"), qsTr("Save a log"), "", null, { info: true }],
        "logfile": [qsTr("Diagnostics"), qsTr("Log file"), "savefile", null, { info: true }],
        "log-verbose": [qsTr("Diagnostics"), qsTr("Log details"), "",
                        { "-1": qsTr("Default"), 0: qsTr("Information"), 1: qsTr("Errors only"),
                          2: qsTr("Errors and warnings"), 3: qsTr("Everything (debug)") }, { info: true }],
        "stats": [qsTr("Diagnostics"), qsTr("Collect playback statistics"), "", null, { advanced: true, info: true }],
        "syslog": [qsTr("Developer Options"), qsTr("Write to the system log"), "", null,
                   { advanced: true, heading: true, info: true }],
        "syslog-debug": [qsTr("Developer Options"), qsTr("Include debug messages in the system log"), "", null,
                         { advanced: true, heading: true, info: true }],
        "syslog-ident": [qsTr("Developer Options"), qsTr("System log identity"), "", null,
                         { advanced: true, heading: true, info: true }],
        "syslog-facility": [qsTr("Developer Options"), qsTr("System log facility"), "", null,
                            { advanced: true, heading: true, info: true }],
        // Interface page of the design. The skin is another interface: the
        // one VLC opens with (it can't run beside this one); its file is
        // the one it opens
        "x-custom-skin": [qsTr("Skin"), qsTr("Use a custom skin"), "", null, { info: true, restart: true }],
        "skins2-last": [qsTr("Skin"), qsTr("Skin file"), "openfile", null, { info: true }],
        "embedded-video": [qsTr("Window"), qsTr("Play video inside main window"), "", null, { info: true }],
        "qt-video-autoresize": [qsTr("Window"), qsTr("Resize window to fit video"), "boolchoice",
                                { false: qsTr("Never"), true: qsTr("Always") }, { info: true }],
        "redesign/remember-window-geometry": [qsTr("Window"), qsTr("Remember window size and position"), "", null, { info: false }],
        "video-on-top": [qsTr("Window"), qsTr("Keep window above other windows"), "", null, { info: true }],
        "qt-titlebar": [qsTr("Window"), qsTr("Show window decorations"), "", null, { info: true }],
        "qt-name-in-title": [qsTr("Window"), qsTr("Show the file name in the title bar"), "", null, { info: true }],
        "qt-fullscreen-screennumber": [qsTr("Window"), qsTr("Open fullscreen on"), "", null, { info: true }],
        "qt-minimal-view": [qsTr("Playback Controls"), qsTr("Start in compact mode"), "", null, { info: true }],
        "qt-pin-controls": [qsTr("Playback Controls"), qsTr("Auto-hide the controls"), "boolchoice",
                            { false: qsTr("Always"), true: qsTr("In fullscreen only") }, { info: true }],
        "mouse-hide-timeout": [qsTr("Playback Controls"), qsTr("Hide after"), "slider", null, { info: false, labelsBaseline: 44, rowHeight: 57, controlTop: 1 }],
        "qt-fs-controller": [qsTr("Playback Controls"), qsTr("Show controls in fullscreen"), "", null, { info: true }],
        "qt-system-tray": [qsTr("Notifications and System Tray"), qsTr("Show a system tray icon"), "", null, { info: true }],
        "qt-start-minimized": [qsTr("Notifications and System Tray"), qsTr("Start minimized to the tray"), "", null, { info: true }],
        "qt-notification": [qsTr("Notifications and System Tray"), qsTr("Show a popup when the track changes"), "", null,
                            { info: true }],
        "redesign/notification-duration": [qsTr("Notifications and System Tray"), qsTr("Popup duration"), "slider", null,
                                           { info: false, labelsBaseline: 44, rowHeight: 57, controlTop: 1 }],
        // Figma 3046:10057: the queue under the folders of its media
        "playlist-tree": [qsTr("Playlist"), qsTr("Show the playlist as a tree"), "", null, { info: true }],
        // not in the design: after its sections (the page starts as it draws it)
        "redesign/always-show-player-bar": [qsTr("Player Bar"), qsTr("Always show the player bar"), "", null,
                                            { info: true }],
        "redesign/increase-contrast": [qsTr("Accessibility"), qsTr("Increase text contrast"), "", null, { info: true }],
        "qt-opacity": [qsTr("Window"), qsTr("Window opacity"), "opacity10", null,
                       { advanced: true, info: false, labelsBaseline: 44, rowHeight: 57, controlTop: 1 }],
        "qt-pause-minimized": [qsTr("Window"), qsTr("Pause video when window is minimized"), "", null,
                               { advanced: true, info: false }],
        "qt-auto-raise": [qsTr("Window"), qsTr("Bring VLC to the front when playback starts"), "",
                          { 0: qsTr("Never"), 1: qsTr("Video"), 2: qsTr("Audio"), 3: qsTr("Audio and video") },
                          { advanced: true, info: true }],
        // the cover art of the player for audio (Player.qml), the cone of
        // the compact view
        "qt-bgcone": [qsTr("Window"), qsTr("Show cover art in the window when playing audio"), "", null,
                      { advanced: true, info: false }],
        "qt-embedded-open": [qsTr("Window"), qsTr("Show the file browser inside the open dialog"), "", null, { advanced: true, info: false }],
        "qt-icon-change": [qsTr("Window"), qsTr("Allow the app icon to change"), "", null, { advanced: true, info: false }],
        // one frame in the design: 5 above, 5 between, 5 below
        "qt-fs-opacity": [qsTr("Playback Controls"), qsTr("Fullscreen controls opacity"), "opacity10", null,
                          { advanced: true, info: false, labelsBaseline: 44, rowHeight: 52, controlTop: 1 }],
        "qt-fs-sensitivity": [qsTr("Playback Controls"), qsTr("Fullscreen controls sensitivity"), "sensitivity", null,
                              { advanced: true, info: false, labelsBaseline: 44, rowHeight: 57, controlTop: 1 }],
        // Privacy page of the design
        "save-recentplay": [qsTr("History"), qsTr("Save recently played items"), "", null, { info: true }],
        "x-remember-position": [qsTr("History"), qsTr("Remember the playback position"), "", null, { info: true }],
        "redesign/recentplay-filter": [qsTr("History"), qsTr("Never remember items matching"), "", null, { info: true }],
        "metadata-network-access": [qsTr("Network"), qsTr("Fetch titles and cover art from the internet"), "", null, { info: true }],
        "lastfm-username": [qsTr("Network"), qsTr("Last.fm username")],
        "lastfm-password": [qsTr("Network"), qsTr("Last.fm password"), "password"],
        "scrobbler-url": [qsTr("Network"), qsTr("Last.fm submission URL")],
        "extraintf": [qsTr("Network"), qsTr("Additional interfaces")],
        "control": [qsTr("Network"), qsTr("Control interfaces")],
        // Playback page of the design
        "restore-playback-pos": [qsTr("Startup"), qsTr("Continue where you left off"), "",
                                 { 0: qsTr("Never"), 1: qsTr("Ask"), 2: qsTr("Always") }, { info: true }],
        "playlist-autostart": [qsTr("Startup"), qsTr("Start playing as soon as a file opens"), "", null, { info: true }],
        // the speed of the player: of now and of the next media
        "rate": [qsTr("Startup"), qsTr("Default playback speed"), "slider", null, { info: false }],
        "playlist-cork": [qsTr("Startup"), qsTr("Pause when another app uses audio"), "", null, { info: true }],
        "preferred-resolution": [qsTr("Tracks", "media streams"), qsTr("Preferred video quality"), "",
                                 { "-1": qsTr("Best available"), 1080: qsTr("Full HD (1080p)"), 720: qsTr("HD (720p)"),
                                   576: qsTr("Standard (576p)"), 360: qsTr("Low (360p)"), 240: qsTr("Very low (240p)") },
                                 { info: true }],
        "menu-language": [qsTr("Tracks", "media streams"), qsTr("Preferred menu language"), "", null, { info: true }],
        // counted from 1 as the design writes them; VLC's choice shows "1"
        "audio-track": [qsTr("Tracks", "media streams"), qsTr("Default audio track"), "track", null, { info: true, placeholder: "1" }],
        "sub-track": [qsTr("Tracks", "media streams"), qsTr("Default subtitle track"), "track", null, { info: true, placeholder: "1" }],
        "x-repeat": [qsTr("Order"), qsTr("Repeat"), "", null, { info: true }],
        "random": [qsTr("Order"), qsTr("Shuffle"), "", null, { info: true }],
        "play-and-stop": [qsTr("Order"), qsTr("Stop after each item"), "", null, { info: true }],
        "x-playback-end": [qsTr("Order"), qsTr("When playback finishes"), "", null, { info: true }],
        "x-seek-precise": [qsTr("Seeking"), qsTr("Seek precisely"), "", null, { info: true }],
        "qt-seek-thumbnail": [qsTr("Seeking"), qsTr("Show thumbnail while scrubbing"), "", null, { info: true }],
        "dvd": [qsTr("Files & Discs"), qsTr("Default optical drive"), "", null, { info: true }],
        "avi-index": [qsTr("Files & Discs"), qsTr("Repair damaged AVI files"), "",
                      { 0: qsTr("Ask"), 1: qsTr("Always"), 2: qsTr("Never"), 3: qsTr("When needed") }, { info: true }],
        "mkv-preload-local-dir": [qsTr("Files & Discs"), qsTr("Load matching .mkv files from the same folder"), "", null, { info: true }],
        // Advanced Options of the design (Figma 3073:14122)
        "input-timeshift-path": [qsTr("Time-shift"), qsTr("Time-shift buffer folder"), "directory", null,
                                 { advanced: true, info: true, rowHeight: 42, fieldHeight: 32 }],
        "input-timeshift-granularity": [qsTr("Time-shift"), qsTr("Time-shift buffer size"), "", null,
                                        { advanced: true, info: true }],
        // expert: options the design leaves out, under Advanced Options in
        // Developer mode
        "start-paused": [qsTr("Order"), qsTr("Start paused"), "", null, { advanced: true, expert: true }],
        "play-and-pause": [qsTr("Order"), qsTr("Pause on the last frame of each item"), "", null, { advanced: true, expert: true }],
        "restore-playback-states": [qsTr("Startup"), qsTr("Resume the last playback states (tracks, speed...)"), "", null,
                                    { advanced: true, expert: true }],
        // Video page of the design
        "video": [qsTr("Output"), qsTr("Enable video"), "", null, { info: true }],
        "fullscreen": [qsTr("Output"), qsTr("Start videos in fullscreen"), "", null, { info: true }],
        "disable-screensaver": [qsTr("Output"), qsTr("Stop the screensaver during playback"), "inttoggle", null, { info: true }],
        "video-wallpaper": [qsTr("Output"), qsTr("Play video as desktop wallpaper"), "", null, { info: true }],
        "deinterlace": [qsTr("Deinterlacing"), qsTr("Deinterlacing"), "",
                        { "-1": qsTr("Automatic"), 0: qsTr("Off"), 1: qsTr("On") }, { info: true }],
        "deinterlace-mode": [qsTr("Deinterlacing"), qsTr("Method"), "", { "auto": qsTr("Automatic") }, { info: true }],
        "aspect-ratio": [qsTr("Framing"), qsTr("Aspect ratio"), "", null, { info: true }],
        "crop": [qsTr("Framing"), qsTr("Crop"), "", null, { info: true }],
        "autoscale": [qsTr("Framing"), qsTr("Scale video to the window"), "", null, { info: true }],
        "zoom": [qsTr("Framing"), qsTr("Zoom"), "", null, { info: true }],
        "align": [qsTr("Framing"), qsTr("Position in the window"), "", null, { info: true }],
        "snapshot-path": [qsTr("Snapshots"), qsTr("Save to"), "directory", null, { info: true }],
        "snapshot-prefix": [qsTr("Snapshots"), qsTr("File name prefix"), "", null, { info: true }],
        "snapshot-format": [qsTr("Snapshots"), qsTr("Format"), "",
                            { "png": "PNG", "jpg": "JPG", "tiff": "TIFF", "webp": "WebP" }, { info: true }],
        "snapshot-width": [qsTr("Snapshots"), qsTr("Size"), "", null, { info: true }],
        "snapshot-sequential": [qsTr("Snapshots"), qsTr("Number files sequentially"), "", null, { info: true }],
        "snapshot-preview": [qsTr("Snapshots"), qsTr("Show a preview after capture"), "", null, { info: true }],
        // Advanced Options of the design (Figma 3073:15633): fields of 32 in rows of 42
        "custom-aspect-ratios": [qsTr("Framing"), qsTr("Custom aspect ratios"), "", null,
                                 { advanced: true, info: true, rowHeight: 42, fieldHeight: 32 }],
        "custom-crop-ratios": [qsTr("Framing"), qsTr("Custom crop ratios"), "", null,
                               { advanced: true, info: true, rowHeight: 42, fieldHeight: 32 }],
        "monitor-par": [qsTr("Framing"), qsTr("Monitor pixel aspect ratio"), "", null,
                        { advanced: true, info: true, rowHeight: 42, fieldHeight: 32, placeholder: "1:1" }],
        "vout": [qsTr("Output"), qsTr("Video output module"), "", null, { advanced: true, expert: true }],
        "snapshot-height": [qsTr("Snapshots"), qsTr("Snapshot height (-1: proportional)"), "", null, { advanced: true, expert: true }],
        "deinterlace-filter": [qsTr("Deinterlacing"), qsTr("Deinterlacing filter"), "", null, { advanced: true, expert: true }],
        // Audio page of the design
        "audio": [qsTr("Output"), qsTr("Enable audio"), "", null, { info: true }],
        "mmdevice-audio-device": [qsTr("Output"), qsTr("Output device"), "", { "": qsTr("System default") }, { info: true }],
        "stereo-mode": [qsTr("Output"), qsTr("Speaker setup"), "", { 0: qsTr("Automatic") }, { info: true }],
        "qt-max-volume": [qsTr("Volume"), qsTr("Maximum volume"), "slider", null,
                          { info: false, rowHeight: 67, controlTop: 5 }],
        "volume-save": [qsTr("Volume"), qsTr("Startup volume"), "startupvolume", null, { info: true, rowHeight: 93, controlTop: 10 }],
        "mmdevice-volume": [qsTr("Volume"), qsTr("Startup volume"), "", null, { hidden: true }],
        // in percent, written without the unit as the design does
        "volume-step": [qsTr("Volume"), qsTr("Volume step"), "volumepercent", null, { info: true, rowHeight: 53, controlTop: 11 }],
        "x-filter:normvol": [qsTr("Loudness"), qsTr("Normalize loudness"), "", null, { info: true, rowHeight: 42, controlTop: 5 }],
        "norm-max-level": [qsTr("Loudness"), qsTr("Target level"), "", null, { info: true, rowHeight: 46, controlTop: 6 }],
        "x-night-mode": [qsTr("Loudness"), qsTr("Night mode"), "", null, { info: true, rowHeight: 44, controlTop: 7 }],
        "audio-replay-gain-mode": [qsTr("Loudness"), qsTr("Replay gain"), "",
                                   { "none": qsTr("Off"), "track": qsTr("Track"), "album": qsTr("Album") },
                                   { info: true, rowHeight: 45, controlTop: 5 }],
        "audio-replay-gain-preamp": [qsTr("Loudness"), qsTr("Replay gain preamp"), "", null, { info: true, decimals: 1, rowHeight: 49, controlTop: 7 }],
        "x-filter:equalizer": [qsTr("Effects"), qsTr("Equalizer"), "", null, { info: true }],
        "equalizer-preset": [qsTr("Effects"), qsTr("Equalizer preset"), "", null, { info: true }],
        "equalizer-bands": [qsTr("Effects"), "", "equalizer", null, { info: false }],
        "equalizer-2pass": [qsTr("Effects"), qsTr("Two-pass"), "", null, { info: true }],
        "x-filter:spatializer": [qsTr("Effects"), qsTr("Spatial audio"), "", null, { info: true }],
        "mix-mode": [qsTr("Effects"), qsTr("Spatial audio mode"), "",
                     { 0: qsTr("Automatic"), 2: qsTr("Headphone virtualization") }, { info: true }],
        "x-filter:stereo_widen": [qsTr("Effects"), qsTr("Stereo widening"), "", null, { info: true }],
        "x-filter:compressor": [qsTr("Effects"), qsTr("Compressor"), "", null, { info: true }],
        "pitch-shift": [qsTr("Effects"), qsTr("Pitch shift"), "", null, { info: true, decimals: 2 }],
        "x-filter:chorus_flanger": [qsTr("Effects"), qsTr("Chorus/flanger"), "", null, { info: true }],
        "x-filter:karaoke": [qsTr("Effects"), qsTr("Karaoke"), "", null, { info: true }],
        "audio-language": [qsTr("Tracks", "media streams"), qsTr("Preferred audio language"), "", { "": qsTr("Automatic") }, { info: true }],
        "audio-desync": [qsTr("Tracks", "media streams"), qsTr("Audio delay"), "", null, { info: true, unit: qsTr("ms") }],
        // audio-visual and effect-list (config_model.cpp); Figma 2372:8639
        "x-visualizer": [qsTr("Visualization"), qsTr("Visualizer"), "", null, { info: true }],
        // Advanced Options of the design (Figma 3180:17645)
        "spdif": [qsTr("Output"), qsTr("Pass compressed audio to the receiver"), "", null, { advanced: true, info: true }],
        "force-dolby-surround": [qsTr("Output"), qsTr("Detect Dolby Surround"), "",
                                 { 0: qsTr("Automatic"), 1: qsTr("On"), 2: qsTr("Off") }, { advanced: true, info: true }],
        "soxr-resampler-quality": [qsTr("Output"), qsTr("Resampling quality"), "",
                                   { 0: qsTr("Fastest"), 1: qsTr("Fast"), 2: qsTr("Balanced"), 3: qsTr("High"), 4: qsTr("Best") },
                                   { advanced: true, info: true }],
        "role": [qsTr("Output"), qsTr("Media role"), "", null, { advanced: true, info: true }],
        "x-visual-size": [qsTr("Visualization"), qsTr("Visualization size"), "", null, { advanced: true, info: true }],
        "aout": [qsTr("Output"), qsTr("Audio output module"), "", null, { advanced: true, expert: true }],
        "gain": [qsTr("Volume"), qsTr("Output gain"), "slider", null, { advanced: true, expert: true }],
        "audio-time-stretch": [qsTr("Effects"), qsTr("Keep the pitch when changing the speed"), "", null, { advanced: true, expert: true }],
        "equalizer-preamp": [qsTr("Effects"), qsTr("Equalizer preamp"), "", null, { hidden: true }],
        "audio-filter": [qsTr("Effects"), qsTr("Audio filters"), "", null, { hidden: true }],
        "compressor-ratio": [qsTr("Effects"), qsTr("Compressor ratio"), "", null, { hidden: true }],
        "compressor-threshold": [qsTr("Effects"), qsTr("Compressor threshold"), "", null, { hidden: true }],
        "compressor-makeup-gain": [qsTr("Effects"), qsTr("Compressor makeup gain"), "", null, { hidden: true }],
        // Subtitles page of the design
        "spu": [qsTr("Tracks & Languages"), qsTr("Enable subtitles"), "", null, { info: true }],
        // the default language of SubtitlePreferences (with the second one of
        // SettingsSubtitleLanguages, they set it)
        "sub-language": [qsTr("Tracks & Languages"), qsTr("Preferred language"), "language", { "": qsTr("Automatic") }, { info: true }],
        "subsdec-encoding": [qsTr("Tracks & Languages"), qsTr("Text encoding"), "", { "": qsTr("Automatic") }, { info: true }],
        "sub-autodetect-file": [qsTr("Loading Subtitles"), qsTr("Load subtitle files automatically"), "", null, { info: true }],
        "sub-autodetect-fuzzy": [qsTr("Loading Subtitles"), qsTr("File name matching"), "", null, { info: true }],
        // VLC's default is the platform's sans-serif (Arial on Windows)
        "freetype-font": [qsTr("Appearance"), qsTr("Font"), "", null,
                          { info: true, rowHeight: 47, defaultDisplay: qsTr("System, sans-serif") }],
        "redesign/subtitle-size-mode": [qsTr("Appearance"), qsTr("Size values"), "segmented", null, { info: true, rowHeight: 49 }],
        // Relative (Size values): by name; Pixels: the size in pixels instead
        "sub-text-scale": [qsTr("Appearance"), qsTr("Font size"), "", null, { info: false }],
        "freetype-fontsize": [qsTr("Appearance"), qsTr("Font size"), "", null,
                              { info: false, unit: "px", defaultDisplay: qsTr("Automatic") }],
        "freetype-bold": [qsTr("Appearance"), qsTr("Bold"), "", null, { info: false }],
        "freetype-color": [qsTr("Appearance"), qsTr("Text color"), "color", null, { info: false }],
        "freetype-opacity": [qsTr("Appearance"), qsTr("Text opacity"), "opacity", null, { info: false }],
        "freetype-outline-thickness": [qsTr("Appearance"), qsTr("Outline thickness"), "", null, { info: false }],
        "freetype-outline-color": [qsTr("Appearance"), qsTr("Outline color"), "color", null, { info: false }],
        "x-sub-shadow": [qsTr("Appearance"), qsTr("Shadow"), "", null, { info: false }],
        "freetype-shadow-color": [qsTr("Appearance"), qsTr("Shadow color"), "color", null, { info: false }],
        "freetype-shadow-distance": [qsTr("Appearance"), qsTr("Shadow distance"), "", null, { info: false }],
        "x-sub-background": [qsTr("Appearance"), qsTr("Background"), "", null, { info: false }],
        "freetype-background-color": [qsTr("Appearance"), qsTr("Background color"), "color", null, { info: false }],
        // the opacity of VLC follows the kept one below while Background is Box
        "freetype-background-opacity": [qsTr("Appearance"), qsTr("Background opacity"), "opacity", null, { info: false, hidden: true }],
        "redesign/subtitle-background-opacity": [qsTr("Appearance"), qsTr("Background opacity"), "opacity", null, { info: false }],
        // in percent of the picture (also Shift+Up and Shift+Down while
        // playing): the subtitles shown move as it does
        "sub-position": [qsTr("Appearance"), qsTr("Vertical position"), "slider", null, { info: true }],
        "sub-delay": [qsTr("Timing"), qsTr("Subtitle delay"), "slider", null, { info: false }],
        "osd": [qsTr("On-Screen Messages"), qsTr("Show feedback for shortcuts"), "", null, { info: true }],
        "osd-duration": [qsTr("On-Screen Messages"), qsTr("Show feedback for"), "slider", null, { info: false }],
        "video-title-show": [qsTr("On-Screen Messages"), qsTr("Show filename when a video starts"), "", null, { info: true }],
        "video-title-timeout": [qsTr("On-Screen Messages"), qsTr("Show filename for"), "slider", null, { info: false }],
        // Advanced Options of the design (Figma 6536:14229): fields of 36 in rows of 48
        "sub-autodetect-path": [qsTr("Loading Subtitles"), qsTr("Load subtitles from these folders"), "", null,
                                { advanced: true, info: true, rowHeight: 48 }],
        "freetype-shadow-angle": [qsTr("Appearance"), qsTr("Shadow angle"), "", null, { advanced: true, info: true, rowHeight: 48 }],
        "freetype-text-direction": [qsTr("Appearance"), qsTr("Text direction"), "",
                                    { 0: qsTr("Left to right"), 1: qsTr("Right to left"), 2: qsTr("Automatic") },
                                    { advanced: true, info: true }],
        "freetype-outline-opacity": [qsTr("Appearance"), qsTr("Outline opacity"), "opacity", null, { advanced: true, expert: true }],
        "freetype-shadow-opacity": [qsTr("Appearance"), qsTr("Shadow opacity"), "opacity", null, { advanced: true, expert: true }],
        "freetype-monofont": [qsTr("Appearance"), qsTr("Monospace font"), "", null,
                              { advanced: true, info: true, rowHeight: 48, defaultDisplay: qsTr("System monospace") }],
        "subsdec-align": [qsTr("Appearance"), qsTr("Text alignment"), "", null, { advanced: true, expert: true }],
        "video-title-position": [qsTr("On-Screen Messages"), qsTr("Position of the filename"), "", null, { advanced: true, expert: true }],
        // Controls page of the design
        "extrashort-jump-size": [qsTr("Skip Amounts"), qsTr("Very short skip"), "", null, { info: false, unit: qsTr("s"), rowHeight: 48 }],
        "short-jump-size": [qsTr("Skip Amounts"), qsTr("Short skip"), "", null, { info: false, unit: qsTr("s"), rowHeight: 48 }],
        "medium-jump-size": [qsTr("Skip Amounts"), qsTr("Medium skip"), "", null, { info: false, unit: qsTr("s"), rowHeight: 48 }],
        "long-jump-size": [qsTr("Skip Amounts"), qsTr("Long skip"), "", null, { info: false, unit: qsTr("s"), rowHeight: 48 }],
        "mouse-pause-policy": [qsTr("Mouse"), qsTr("Single click on the video"), "",
                               { "auto": qsTr("Play / Pause"), "disabled": qsTr("Nothing"),
                                 "pause-on-released": qsTr("Play / Pause on release") }, { info: false }],
        "redesign/double-click-action": [qsTr("Mouse"), qsTr("Double click on the video"), "", null, { info: false }],
        "hotkeys-y-wheel-mode": [qsTr("Mouse"), qsTr("Scroll wheel"), "", null, { info: false }],
        "hotkeys-x-wheel-mode": [qsTr("Mouse"), qsTr("Horizontal scroll"), "", null, { info: false }],
        "x-wheel-reverse": [qsTr("Mouse"), qsTr("Reverse scroll direction"), "", null, { info: false }],
        // the web interface starts and stops at once, starts again on
        // another port, password, address
        "x-web-remote": [qsTr("Remotes"), qsTr("Web remote control"), "", null, { info: false }],
        "http-port": [qsTr("Remotes"), qsTr("Port"), "", null, { info: false, rowHeight: 48 }],
        "mouse-events": [qsTr("Mouse"), qsTr("Handle mouse clicks in the video"), "", null, { advanced: true }],
        "http-password": [qsTr("Remotes"), qsTr("Web remote password"), "password", null, { advanced: true }],
        "http-host": [qsTr("Remotes"), qsTr("Web remote address"), "", null, { advanced: true }],
        "qt-disable-volume-keys": [qsTr("Mouse"), qsTr("Ignore the volume keys of the keyboard"), "", null, { advanced: true }],
        // Library page of the design
        "one-instance": [qsTr("Opening Files"), qsTr("Only allow one VLC window"), "", null, { info: true }],
        "one-instance-when-started-from-file": [qsTr("Opening Files"), qsTr("Apply to files opened from the file manager"), "", null, { info: true }],
        "playlist-enqueue": [qsTr("Opening Files"), qsTr("Add to the playlist instead of replacing it"), "", null, { info: true }],
        "recursive": [qsTr("Opening Files"), qsTr("When opening a folder"), "",
                      { "none": qsTr("Only this folder"), "collapse": qsTr("Include subfolders"),
                        "expand": qsTr("Include and expand subfolders") }, { info: true }],
        "show-hiddenfiles": [qsTr("Opening Files"), qsTr("Show hidden files"), "", null, { info: true }],
        "x-fetch-metadata": [qsTr("Metadata & Artwork"), qsTr("Fetch titles and artwork"), "", null, { info: true }],
        // made by VLC when it starts, the interface built with it or without
        "media-library-enabled": [qsTr("Metadata & Artwork"), qsTr("Use the media library"), "", null, { info: true, restart: true }],
        "auto-preparse": [qsTr("Metadata & Artwork"), qsTr("Read media information automatically"), "", null, { advanced: true }],
        "ignore-filetypes": [qsTr("Opening Files"), qsTr("Ignored file extensions"), "", null, { advanced: true }],
        // Network page of the design
        "x-buffering": [qsTr("Buffering"), qsTr("Buffering"), "", null, { info: true }],
        "file-caching": [qsTr("Buffering"), qsTr("Local files"), "", null, { info: false, unit: qsTr("ms"), rowHeight: 48 }],
        "disc-caching": [qsTr("Buffering"), qsTr("Discs"), "", null, { info: false, unit: qsTr("ms"), rowHeight: 48 }],
        "network-caching": [qsTr("Buffering"), qsTr("Network streams"), "", null, { info: false, unit: qsTr("ms"), rowHeight: 48 }],
        "live-caching": [qsTr("Buffering"), qsTr("Live capture"), "", null, { info: false, unit: qsTr("ms"), rowHeight: 48 }],
        "http-proxy": [qsTr("Proxy"), qsTr("HTTP proxy"), "", null, { info: true, placeholder: qsTr("System proxy"), rowHeight: 48 }],
        "http-proxy-pwd": [qsTr("Proxy"), qsTr("HTTP proxy password"), "password", null, { info: true, rowHeight: 48, controlTop: 6 }],
        "gnutls-system-trust": [qsTr("Security"), qsTr("Verify certificates against the system trust store"), "", null, { info: true }],
        "gnutls-priorities": [qsTr("Security"), qsTr("Cipher preference"), "",
                              { "PERFORMANCE": qsTr("Fastest"), "NORMAL": qsTr("Balanced"),
                                "SECURE128": qsTr("Secure (128-bit)"), "SECURE256": qsTr("Most secure (256-bit)") },
                              { info: true }],
        "ipv4-timeout": [qsTr("Connections"), qsTr("Connection timeout"), "", null, { advanced: true, unit: qsTr("ms") }],
        "http-user-agent": [qsTr("Connections"), qsTr("HTTP user agent"), "", null, { advanced: true }],
        "gnutls-dir-trust": [qsTr("Security"), qsTr("Trusted certificates folder"), "", null, { advanced: true }],
        // Performance page of the design
        "dec-dev": [qsTr("Decoding"), qsTr("Hardware-accelerated decoding"), "",
                    { "any": qsTr("Automatic"), "none": qsTr("Disabled") }, { info: true }],
        "x-postproc": [qsTr("Decoding"), qsTr("Video post-processing"), "", null, { info: true }],
        "postproc-q": [qsTr("Decoding"), qsTr("Post-processing quality"), "", null, { info: true, hidden: true }],
        "avcodec-skiploopfilter": [qsTr("Decoding"), qsTr("Skip the H.264 loop filter"), "",
                                   { 0: qsTr("Never"), 1: qsTr("Non-reference frames"), 2: qsTr("Bidirectional frames"),
                                     3: qsTr("Non-key frames"), 4: qsTr("Always") }, { info: true }],
        "avcodec-threads": [qsTr("Decoding"), qsTr("Decoding threads"), "", null, { info: true, rowHeight: 48, controlTop: 6 }],
        "drop-late-frames": [qsTr("Smooth Playback"), qsTr("Drop frames when the system can't keep up"), "", null, { info: false }],
        "high-priority": [qsTr("Smooth Playback"), qsTr("Give VLC real-time priority"), "", null, { info: false }],
        "redesign/process-priority": [qsTr("Smooth Playback"), qsTr("Process priority"), "", null, { info: true, rowHeight: 48, controlTop: 6 }],
        "sout-transcode-venc": [qsTr("Developer"), qsTr("Encoder"), "", null, { advanced: true, heading: true, info: true }],
        "codec": [qsTr("Developer"), qsTr("Decoder"), "", null, { advanced: true, heading: true, info: true }],
        "clock-synchro": [qsTr("Developer"), qsTr("Clock synchronisation"), "", null, { advanced: true, heading: true, info: true }],
        "clock-jitter": [qsTr("Developer"), qsTr("Clock jitter"), "", null, { advanced: true, heading: true, info: true, unit: qsTr("ms"), infoGap: 42, rowHeight: 48, controlTop: 6 }],
        "cr-average": [qsTr("Developer"), qsTr("Clock reference averaging"), "", null, { advanced: true, heading: true, info: true, rowHeight: 48, controlTop: 6 }],
        "input-record-path": [qsTr("Recording"), qsTr("Save recordings to"), "directory", null, { info: true, rowHeight: 48 }],
        "input-record-native": [qsTr("Recording"), qsTr("Record without re-encoding when possible"), "", null, { info: false }],
        "redesign/discover-cast-devices": [qsTr("Casting"), qsTr("Discover cast devices"), "", null, { info: false }],
        // VLC's default is one of them (Medium, Low on ARM): an "Automatic"
        // choice would stand for that value and hide its name
        "sout-chromecast-conversion-quality": [qsTr("Casting"), qsTr("Cast quality"), "",
                                               { 0: qsTr("High"), 1: qsTr("Medium"), 2: qsTr("Low"), 3: qsTr("Low CPU") },
                                               { info: true }],
        "sout-display": [qsTr("Streaming"), qsTr("Play locally while streaming"), "", null, { info: false }],
        "sout-audio": [qsTr("Streaming"), qsTr("Include audio"), "", null, { info: false }],
        "sout-video": [qsTr("Streaming"), qsTr("Include video"), "", null, { info: false }],
        "sout-spu": [qsTr("Streaming"), qsTr("Include subtitles"), "", null, { info: false }],
        "sout-x264-preset": [qsTr("Streaming"), qsTr("Encoding preset"), "", null, { info: true }],
        "sout-x264-tune": [qsTr("Streaming"), qsTr("Tuning"), "", null, { info: true }],
        "sout-chromecast-audio-passthrough": [qsTr("Casting"), qsTr("Send Dolby audio as is"), "", null, { advanced: true }],
        "sout-chromecast-http-port": [qsTr("Casting"), qsTr("Cast HTTP port"), "", null, { advanced: true }],
        "sout-keep": [qsTr("Streaming"), qsTr("Keep the stream output open between items"), "", null, { advanced: true }]
    })

// The entries of the rows ({ section, label, editor... }), kept by key, for
// the table they were made from: the table is made again when the language
// changes (its texts), and the rows asking for their entry then (they read
// the table) get them in the new language
property var cache: ({ items: null, entries: {} })

function entry(key, fallback) {
    if (cache.items !== items) {
        cache.items = items
        cache.entries = {}
    }
    const cached = cache.entries[key]
    if (cached !== undefined && (items[key] !== undefined || cached.fallback === fallback))
        return cached.value
    const value = makeEntry(key, fallback)
    cache.entries[key] = { value: value, fallback: fallback }
    return value
}

function makeEntry(key, fallback) {
    // a row shown again elsewhere: the presentation of its setting, in the
    // section of the design (the scroll rows under Remotes)
    if (key.startsWith("x-mirror:")) {
        const mirrored = key.substring(9)
        const sections = { "input-timeshift-granularity": qsTr("Framing"), "x-visual-size": qsTr("Appearance") }
        return Object.assign({}, makeEntry(mirrored, fallback), { section: sections[mirrored] || qsTr("Remotes") })
    }
    const item = items[key]
    const advanced = key === "verbose" || key === "stats"
    const hidden = ["lastfm-username", "lastfm-password", "scrobbler-url", "extraintf", "control"].includes(key)
    // item[4]: { info: shows the (i) of the design, unit: after the value,
    // advanced: under Advanced Options, hidden, expert: shown in Developer
    // mode only, restart: read when VLC starts only }
    const extra = item && item[4] ? item[4] : {}
    return item ? { section: item[0], label: item[1], editor: item[2] || "", choiceLabels: item[3] || null,
                    advanced: advanced || !!extra.advanced, hidden: hidden || !!extra.hidden,
                    expert: !!extra.expert, restart: !!extra.restart,
                    info: extra.info, unit: extra.unit || "", decimals: extra.decimals,
                    placeholder: extra.placeholder || "", heading: !!extra.heading,
                    rowHeight: extra.rowHeight || 0, controlTop: extra.controlTop, infoGap: extra.infoGap,
                    accessible: extra.accessible || "", fieldHeight: extra.fieldHeight || 0,
                    defaultDisplay: extra.defaultDisplay || "", defaultChoice: extra.defaultChoice || "",
                    labelsBaseline: extra.labelsBaseline }
                : { section: qsTr("More options"), label: fallback, editor: "", choiceLabels: null,
                    advanced: false, hidden: false, expert: false, restart: false }
}

}
