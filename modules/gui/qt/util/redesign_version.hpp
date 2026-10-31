/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

// Version of the VLC Redesign build, compared with the GitHub releases by
// the updater and read by extras/juxtopposed/make-release.ps1: the release
// it is, or the one it prepares (raised once the work for it starts, so a
// build made in between does not read as the release before).
#define VLC_REDESIGN_VERSION "0.5.1"
// The product's name where the app names itself (window title, tray, message
// boxes, the top bars, About): VideoLAN's trademark policy allows "VLC media
// player" for GPL redistributions, and no use of the name "VideoLAN"
// (videolan.org/legal.html). Not translated: a name. (The installation's own
// names, folders and keys stay "VLC Redesign": an update must find them.)
#define VLC_REDESIGN_NAME "VLC Media Player Redesign"
#define VLC_REDESIGN_REPO "KuchiSofts/vlc-redesign"
#define VLC_REDESIGN_REPO_URL "https://github.com/" VLC_REDESIGN_REPO
