/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
/****************************************************************************
 * catalog.cpp : Core and Qt options confirmed for the redesign settings
 ****************************************************************************/
#include "catalog.hpp"

namespace PrefsCatalog
{
static const Option entries[] = {
    // Startup of the design: the error popup and the confirmations of the
    // interface read qt-error-dialogs (Dialogs.qml, questionDialog)
    {"general", "qt-error-dialogs"}, {"general", "redesign-update-check"},
    // the card of the new version after an update (whats_new.cpp)
    {"general", "qt:redesign/show-whats-new"},
    {"general", "file-logging"}, {"general", "logfile"},
    {"general", "log-verbose"},
    // extra-options: read by the core at the start (libvlc.c)
    {"general", "verbose"}, {"general", "extra-options"},
    {"general", "logmode"}, {"general", "stats"},
    {"general", "syslog"}, {"general", "syslog-debug"}, {"general", "syslog-ident"},
    {"general", "syslog-facility"},
    // Interface page of the design: Skin, Window, Playback Controls,
    // Notifications and System Tray, then Advanced Options
    {"interface", "skins2-last"},
    {"interface", "embedded-video"}, {"interface", "qt-video-autoresize"},
    {"interface", "qt:redesign/remember-window-geometry"},
    {"interface", "video-on-top"},
    {"interface", "qt-titlebar"}, {"interface", "qt-name-in-title"},
    {"interface", "qt-fullscreen-screennumber"},
    {"interface", "qt-minimal-view"}, {"interface", "qt-pin-controls"},
    {"interface", "mouse-hide-timeout"},
    // the mouse shows the controls in fullscreen (Player.qml), the skins
    // read it too
    {"interface", "qt-fs-controller"},
    {"interface", "qt-system-tray"}, {"interface", "qt-start-minimized"},
    {"interface", "qt-notification"}, {"interface", "qt:redesign/notification-duration"},
    // the Playlist section of the design: the queue under the folders of its
    // media (VLC 4's queue is flat: PlaylistPane.qml draws the folders as
    // headings, the rows move, select and drop as they did)
    {"interface", "playlist-tree"},
    // the bar of the library while nothing plays (MainDisplay.qml): not in
    // the design, after its sections
    {"interface", "qt:redesign/always-show-player-bar"},
    // the dim texts at the contrast of WCAG AA (VLCStyle.redesign.color())
    {"interface", "qt:redesign/increase-contrast"},
    {"interface", "qt-opacity"}, {"interface", "qt-pause-minimized"},
    {"interface", "qt-auto-raise"}, {"interface", "qt-bgcone"},
    {"interface", "qt-embedded-open"}, {"interface", "qt-icon-change"},
    {"interface", "qt-fs-opacity"}, {"interface", "qt-fs-sensitivity"},
    // Privacy page of the design: History (Remember the playback position
    // follows Save recently played items), Network with Last.fm
    {"privacy", "save-recentplay"},
    {"privacy", "qt:redesign/recentplay-filter"},
    {"privacy", "metadata-network-access"},
    {"privacy", "lastfm-username"}, {"privacy", "lastfm-password"},
    {"privacy", "scrobbler-url"}, {"privacy", "extraintf"}, {"privacy", "control"},
    // Playback page of the design: Startup, Tracks, Order, Seeking, Files &
    // Discs, then Advanced Options
    {"playback", "restore-playback-pos"},
    {"playback", "playlist-autostart"}, {"playback", "rate"},
    {"playback", "playlist-cork"},
    {"playback", "preferred-resolution"}, {"playback", "menu-language"},
    {"playback", "audio-track"}, {"playback", "sub-track"},
    {"playback", "repeat"}, {"playback", "loop"}, {"playback", "random"},
    {"playback", "play-and-stop"},
    {"playback", "qt-seek-thumbnail"},
    {"playback", "dvd"}, {"playback", "avi-index"}, {"playback", "mkv-preload-local-dir"},
    {"playback", "start-paused"}, {"playback", "play-and-pause"},
    {"playback", "restore-playback-states"},
    {"playback", "input-timeshift-path"}, {"playback", "input-timeshift-granularity"},
    // Video page of the design: Output, (Picture), Deinterlacing, Framing,
    // Snapshots, then Advanced Options
    {"video", "video"}, {"video", "fullscreen"},
    {"video", "disable-screensaver"}, {"video", "video-wallpaper"},
    {"video", "deinterlace"}, {"video", "deinterlace-mode"},
    {"video", "aspect-ratio"}, {"video", "crop"}, {"video", "autoscale"},
    {"video", "zoom"}, {"video", "align"},
    {"video", "snapshot-path"}, {"video", "snapshot-prefix"},
    {"video", "snapshot-format"}, {"video", "snapshot-width"},
    {"video", "snapshot-sequential"}, {"video", "snapshot-preview"},
    {"video", "vout"}, {"video", "snapshot-height"}, {"video", "deinterlace-filter"},
    {"video", "custom-aspect-ratios"}, {"video", "custom-crop-ratios"}, {"video", "monitor-par"},
    // Audio page of the design: Output, Volume, Loudness, Effects, Tracks,
    // Visualization, then Advanced Options
    {"audio", "audio"}, {"audio", "mmdevice-audio-device"}, {"audio", "stereo-mode"},
    {"audio", "qt-max-volume"}, {"audio", "volume-save"}, {"audio", "mmdevice-volume"},
    // the startup volume of the other outputs (ConfigModel::startupVolumeOption)
    {"audio", "waveout-volume"}, {"audio", "alsa-gain"}, {"audio", "auhal-volume"},
    {"audio", "jack-gain"},
    {"audio", "volume-step"},
    {"audio", "x:x-filter:normvol"}, {"audio", "norm-max-level"}, {"audio", "x:x-night-mode"},
    {"audio", "audio-replay-gain-mode"}, {"audio", "audio-replay-gain-preamp"},
    {"audio", "x:x-filter:equalizer"}, {"audio", "equalizer-preset"}, {"audio", "equalizer-bands"},
    {"audio", "equalizer-2pass"},
    {"audio", "x:x-filter:spatializer"}, {"audio", "mix-mode"},
    {"audio", "x:x-filter:stereo_widen"}, {"audio", "x:x-filter:compressor"},
    {"audio", "pitch-shift"},
    {"audio", "x:x-filter:chorus_flanger"}, {"audio", "x:x-filter:karaoke"},
    {"audio", "audio-language"}, {"audio", "audio-desync"},
    {"audio", "audio-visual"},
    {"audio", "aout"}, {"audio", "gain"}, {"audio", "audio-time-stretch"},
    {"audio", "spdif"}, {"audio", "force-dolby-surround"},
    {"audio", "soxr-resampler-quality"}, {"audio", "role"},
    {"audio", "equalizer-preamp"}, {"audio", "audio-filter"},
    {"audio", "compressor-ratio"}, {"audio", "compressor-threshold"},
    {"audio", "compressor-makeup-gain"},
    // Subtitles page of the design: Tracks & Languages, Loading Subtitles,
    // Appearance, Timing, On-Screen Messages, then Advanced Options
    {"subtitles", "spu"}, {"subtitles", "sub-language"},
    {"subtitles", "subsdec-encoding"},
    {"subtitles", "sub-autodetect-file"},
    {"subtitles", "sub-autodetect-fuzzy"},
    {"subtitles", "freetype-font"}, {"subtitles", "qt:redesign/subtitle-size-mode"},
    {"subtitles", "sub-text-scale"}, {"subtitles", "freetype-fontsize"}, {"subtitles", "freetype-bold"},
    {"subtitles", "freetype-color"}, {"subtitles", "freetype-opacity"},
    {"subtitles", "freetype-outline-thickness"},
    {"subtitles", "freetype-outline-color"},
    {"subtitles", "x:x-sub-shadow"}, {"subtitles", "freetype-shadow-color"},
    {"subtitles", "freetype-shadow-distance"},
    {"subtitles", "x:x-sub-background"}, {"subtitles", "freetype-background-color"},
    {"subtitles", "freetype-background-opacity"}, {"subtitles", "qt:redesign/subtitle-background-opacity"},
    {"subtitles", "sub-position"},
    {"subtitles", "sub-delay"}, {"subtitles", "qt-seek-subtitle-cues"},
    {"subtitles", "osd"}, {"subtitles", "osd-duration"},
    {"subtitles", "video-title-show"}, {"subtitles", "video-title-timeout"},
    {"subtitles", "sub-autodetect-path"},
    {"subtitles", "freetype-outline-opacity"},
    {"subtitles", "freetype-shadow-opacity"},
    {"subtitles", "freetype-shadow-angle"}, {"subtitles", "freetype-text-direction"},
    {"subtitles", "freetype-monofont"}, {"subtitles", "subsdec-align"},
    {"subtitles", "video-title-position"},
    // Controls page of the design: (Keyboard Shortcuts), Skip Amounts, Mouse,
    // Remotes, then Advanced Options
    {"controls", "extrashort-jump-size"}, {"controls", "short-jump-size"},
    {"controls", "medium-jump-size"}, {"controls", "long-jump-size"},
    {"controls", "mouse-pause-policy"}, {"controls", "qt:redesign/double-click-action"},
    {"controls", "hotkeys-y-wheel-mode"},
    {"controls", "hotkeys-x-wheel-mode"},
    {"controls", "http-port"},
    {"controls", "mouse-events"}, {"controls", "http-password"},
    {"controls", "http-host"}, {"controls", "qt-disable-volume-keys"},
    // Library page of the design: Opening Files, Metadata & Artwork, then
    // Advanced Options
    {"library", "one-instance"},
    {"library", "one-instance-when-started-from-file"},
    {"library", "playlist-enqueue"},
    {"library", "recursive"}, {"library", "show-hiddenfiles"},
    {"library", "x:x-fetch-metadata"}, {"library", "media-library-enabled"},
    {"library", "auto-preparse"}, {"library", "ignore-filetypes"},
    // Network page of the design: Buffering, Proxy, Security, then Advanced
    // Options
    {"network", "x:x-buffering"},
    {"network", "file-caching"}, {"network", "disc-caching"},
    {"network", "network-caching"}, {"network", "live-caching"},
    {"network", "http-proxy"}, {"network", "http-proxy-pwd"},
    {"network", "gnutls-system-trust"},
    {"network", "gnutls-priorities"},
    {"network", "ipv4-timeout"}, {"network", "http-user-agent"},
    {"network", "gnutls-dir-trust"},
    // Performance page of the design: Decoding, Smooth Playback (Rescan
    // plugins), then Advanced Options (Developer)
    {"performance", "dec-dev"},
    {"performance", "postproc-q"},
    {"performance", "avcodec-skiploopfilter"},
    {"performance", "avcodec-threads"},
    {"performance", "drop-late-frames"},
    {"performance", "high-priority"},
#ifdef _WIN32
    // (applied with SetPriorityClass: MainCtx::loadPrefs)
    {"performance", "qt:redesign/process-priority"},
#endif
    {"performance", "sout-transcode-venc"}, {"performance", "codec"},
    {"performance", "clock-synchro"}, {"performance", "clock-jitter"},
    {"performance", "cr-average"},
    {"recording", "input-record-path"},
    {"recording", "input-record-native"},
    {"recording", "qt:redesign/discover-cast-devices"},
    {"recording", "sout-chromecast-conversion-quality"},
    {"recording", "sout-display"}, {"recording", "sout-audio"},
    {"recording", "sout-video"},
    {"recording", "sout-spu"},
    {"recording", "sout-x264-preset"}, {"recording", "sout-x264-tune"},
    {"recording", "sout-chromecast-audio-passthrough"},
    {"recording", "sout-chromecast-http-port"},
    {"recording", "sout-keep"},
    // Live TV: the switch of its add-on (General, Startup), then its
    // page: its sources (SettingsTv.qml), Guide, Watching and Lists
    {"general", "iptv-enabled"},
    {"tv", "qt:redesign/iptv-refresh"}, {"tv", "qt:redesign/iptv-guide-days"},
    {"tv", "qt:redesign/iptv-guide-hours"}, {"tv", "qt:redesign/iptv-languages"},
    {"tv", "qt:redesign/iptv-banner"}, {"tv", "qt:redesign/iptv-start-last"},
    {"tv", "qt:redesign/iptv-progress"}, {"tv", "qt:redesign/iptv-next"},
    {"tv", "qt:redesign/iptv-descriptions"}, {"tv", "qt:redesign/iptv-adult"},
    {"extensions", "qt-autoload-extensions"},
};

const Option *options() { return entries; }
size_t count() { return sizeof(entries) / sizeof(entries[0]); }
}
