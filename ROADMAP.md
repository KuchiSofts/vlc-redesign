# Roadmap

Where **VLC Redesign** is going. The goal stays the same: the real VLC, with
[Juxtopposed's design](https://www.figma.com/community/file/1691465999134370980) on every screen, pixel for pixel,
working smoothly on every device.

Have an idea? Open a **[suggestion](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=feature_request.yml)**.
Found a problem? Open a **[bug report](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=bug_report.yml)**.
Issues with a 👍 from many people move up this list.

Status: ✅ done · 🔨 in progress · 📋 to do · 💡 idea

**Contents:** [Where each program stands](#where-each-program-stands) · [Next](#next-058-and-after) ·
[Each platform](#each-platform) · [Released versions](#released-versions) · [Staying close to VLC](#staying-close-to-vlc)

The full plan, with every Figma frame, its status and the design tokens, is the
[master plan](docs/MASTER-PLAN.md).

## Where each program stands

Updated as the work moves (last: 2026-11-08, 0.5.8 published).

| Program | Status | Progress | What is left |
|---|---|---|---|
| **Windows x64** | ✅ released 0.5.8 | ██████████ 100 % | — fixes as the issues bring them |
| **Windows on ARM64** | ✅ released 0.5.8 | ██████████ 100 % | a run by hand on a real ARM64 PC |
| **Linux** (x86_64, ARM64) | ✅ released 0.5.8 | ██████████ 100 % | the Flatpak on Flathub |
| **macOS** (Apple Silicon, Intel) | ✅ released 0.5.8 | ██████████ 100 % | tests on a Mac by hand |
| **Android** (phones) | ✅ released 0.5.8 | ██████████ 100 % | — tested on a real phone (Galaxy class) and emulators |
| **Android** (tablets) | ✅ released 0.5.8 | ██████████ 100 % | — tested on a real tablet (Galaxy Tab class) and emulators |
| **Android TV** | ✅ released 0.5.8 | ██████████ 100 % | — 1:1 with the Figma frames, lighter (no work in the background); tests on more TV boxes |
| **Samsung TV** (*Juxto Media Player*) | ✅ released 0.5.8 | ██████████ 100 % | — tested on a 2017 Samsung TV; the Samsung store (later) |
| **iPhone, iPad** | 💡 later | ░░░░░░░░░░ 0 % | after Android |

## Next: 0.5.9 and after

**0.5.9** is an update release of fixes, made as the issues bring them (each fix is first published through a pull request,
with a build to try):

- ✅ In full screen, keys and the remote seek, change the volume or pause without showing the controls, if you
  choose (a slim progress line and the time, or a short message instead), as in VLC 3, in every app
  ([#72](https://github.com/KuchiSofts/vlc-redesign/issues/72), @BarbodF): desktop, Android (phones, tablets, TV)
  and the Samsung TV app
- ✅ Stability (the owner: "search every line and code and improve stablity and app loading speed and memory
  leak"), in every app: every line of the newest code reviewed, the parsers fuzzed, quitting and Live TV bounded,
  stalled servers stopped, no secret header sent to another server on a redirect, the interface's thread free of
  network and disk work, no QML warning left; the desktop starts faster (window 0.60 → 0.48 s), memory soaked
- 🔨 Online subtitles for videos played from the network (a link, SMB, NFS, UPnP/DLNA) on Android, manual and
  automatic, as on the desktop ([#73](https://github.com/KuchiSofts/vlc-redesign/issues/73), @Elshepl); checked on
  the Samsung TV app too
- 🟡 The release: the three apps' branches together, built and tested on every platform
- 📋 Live TV: the servers tried on real ones (Tvheadend, NextPVR, Channels DVR, an HDHomeRun tuner) and a paid
  Schedules Direct account; on Android and the Samsung TV app a Tvheadend server that takes only Digest sign-in
  plays with its access code (their players send Basic)
- 📋 The subtitle services with keys and accounts tried with real keys (SubSource, SubDL, OpenSubtitles.com and the
  seven others)
- ✅ Android TV: after the TV sources close, the remote's focus goes back to the page (0.5.8); opened with the
  remote, TV Sources has its tab focused and one Back closes it
- 📋 Linux: the first update of the AppImage from the app, checked from one version to the next on real desktops
- 📋 Samsung TV: a USB drive played on a real TV (the folders of the TV's own storage are checked)
- 📋 Android TV: tests on more TV boxes (Google TV, Fire TV)

After it:

- 📋 Linux: the Flatpak on Flathub
- 📋 macOS and Windows on ARM64: a round by hand on a real Mac and a real ARM64 PC (both are tested on GitHub's
  machines at every build)
- 📋 Windows: tests on more real PCs (Windows 10, Intel graphics, screens of different scales)
- 📋 Code-signed Windows releases (no SmartScreen warning): waiting for SignPath's approval
- 💡 The Samsung TV app in Samsung's store; the TV side of casting (drawn in the design); then iPhone and iPad

## Each platform

Where each platform stands is kept in one file, [`platforms.yml`](platforms.yml): the bar at the top of the README,
the [website](https://kuchisofts.github.io/vlc-redesign/) and the *Also on* line of the release notes and of About
are made from it. The design's colours, sizes, fonts and icons are in [`design/`](design/README.md), with CSS,
Android and Swift files made from them and a check on every pull request that the app still uses the same values.

**Windows** (x64 since 0.1.0, ARM64 since 0.4.1)

- ✅ An installer (no administrator rights) and a portable zip, in 72 languages chosen in the installer
- ✅ Small updates in the app (only the files that changed, signed checksums), on x64 and ARM64
- ✅ Tested at every build on GitHub's x64 and ARM64 runners (installs, starts, plays, uninstalls)
- 📋 A run by hand on a real ARM64 PC (Snapdragon); code signing (SignPath)

**Linux** (x86_64 and ARM64, since 0.4.0)

- ✅ A .deb, an .rpm, an AppImage, a folder (tar.xz) and a Flatpak, next to the distribution's own VLC
- ✅ Tested in containers on Ubuntu 24.04/26.04, Debian 13, Fedora 41/43 and Arch, and on real Intel graphics
  (VA-API)
- ✅ The AppImage updates itself from the app (0.5.2)
- 📋 The Flatpak on Flathub; tests on more real desktops

**macOS** (Apple Silicon and Intel, since 0.4.0)

- ✅ A disk image for each, built and tested by GitHub's Macs, signed ad hoc (macOS asks once to open it)
- ✅ Updates itself from the app (0.5.5)
- 📋 Tests on a Mac by hand; signing by Apple (it needs a developer account)

**Android** (phones and tablets since 0.5.0, TV since 0.5.3)

- ✅ Its own app (`org.kuchisofts.vlcredesign`, beside VLC): the design's Home, library, player and Customize
  Player, casting, effects, recording, subtitles in every script and right to left, Live TV, the tablet layout
- ✅ An APK on the [release page](https://github.com/KuchiSofts/vlc-redesign/releases/latest) (not on Google Play);
  the app says when a new version is out
- ✅ Android TV: every page in the design with the remote (0.5.3): the welcome, Settings, the video and music
  players, details, search, the folder and playlist pages, About

**Samsung TV** (since 0.5.3)

- ✅ *Juxto Media Player*: a player of its own for Samsung TVs (Tizen 3.0 and later), inspired by VLC, in the design: the library, the player,
  Live TV, 17 languages, right to left, the colour keys of the remote; the TV's own player plays the video
- ✅ Installed from a computer (developer mode, a free Samsung certificate), with a step-by-step guide; not in the store
- 📋 A USB drive played on a real TV; the Samsung store

**Later**

- 💡 iPhone and iPad

## Released versions

Newest first. Every change is in the [changelog](CHANGELOG.md).

### 0.5.8 (2026-11-08)

**An update release** (the apps update themselves to it; the Windows installers come with it):

- ✅ SubSource as a subtitle service with your own free key, for its Arabic subtitles in particular
  ([#70](https://github.com/KuchiSofts/vlc-redesign/issues/70), @Elshepl), with SubDL and OpenSubtitles.com, and
  seven more services with your account or key (Titlovi, Assrt, Jimaku, BetaSeries, Subs.ro, Napisy24, Legendas.net),
  in every app: a services page in Settings › Subtitles (each service on or off, removed or added back, keys tested,
  accounts signed in) and the languages to search (fewer: a faster search); the four free services on by default
- ✅ Live TV in every app: your own servers and tuners (Jellyfin, Emby, Tvheadend, NextPVR, Channels DVR, HDHomeRun,
  found on your network on the desktop and Android), Schedules Direct's guide, an M3U behind a login, the automatic
  guide (channels matched to iptv-org's database, the free guides of their countries attached, with Undo), a Guides
  tab and a smarter catalog (460 free, legal sources; *For you* and *Recommended*; every list checked every day)
- ✅ Fixed: the seek bar's frame preview with `--start-time`; on Android 14 a crash of the system's media provider
  with subtitles found online (0.5.7 too); Android's password fields, the TV remote's focus in TV Sources and
  "Country" translated as the music genre

### 0.5.7 (2026-11-08)

**An update release** (the apps update themselves to it; the Windows installers come with it):

- ✅ Hold Space or the video for 2× speed, done in the interface (VideoLAN's core unchanged), from
  [@SHR1F](https://github.com/SHR1F)'s [#64](https://github.com/KuchiSofts/vlc-redesign/pull/64)
- ✅ The design check passes on Windows checkouts ([#62](https://github.com/KuchiSofts/vlc-redesign/pull/62), @SHR1F)
- ✅ A folder of many subtitles refreshes the list once ([#63](https://github.com/KuchiSofts/vlc-redesign/pull/63), @SHR1F)
- ✅ Subtitles from the internet in every app ([#67](https://github.com/KuchiSofts/vlc-redesign/issues/67)): Android
  (phones, tablets, Android TV) and the Samsung TV app as on the desktop, OpenSubtitles.com with your own free key,
  and how to get the key in every app
- ✅ Every free subtitle service asked for any file (also a name that says nothing: the file's fingerprint names
  it), and what each found shown, in every app
- ✅ Hide UI: Space toggles playback once, the double-click follows its setting
  ([#65](https://github.com/KuchiSofts/vlc-redesign/pull/65), @SHR1F)
- ✅ A release waits until every platform is at its version: About's "Also on" line of 0.5.6 still named 0.5.5
  for the others
- ✅ Live TV in the library next to the video: the channels with what is on, their groups, search, the TV page's
  menu; a channel tuned there keeps the keyboard on the list
- ✅ Android: the Live TV information bar stacked on a phone held upright
- ✅ Android TV: the remote reaches every video of the Videos page and every button of the player
- ✅ Published: [v0.5.7](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.5.7), 23 files; the small updates
  9.0 MB (x64) and 8.5 MB (ARM64) from 0.5.6

### 0.5.6 (2026-11-07)

**An update release** (the apps update themselves to it; the Windows installers come with it):

- ✅ Android: no crash on Android 5 to 10 (videos, Settings, subtitles); installs from Android 5.0
- ✅ Android: a screen closed by a rotation no longer stays in memory
- ✅ Windows: every update release carries the installers (a copy installed for every user updates
  with them); the in-app update checked end to end on the published 0.5.4 → 0.5.5
- ✅ Samsung TV: the app is named *Juxto Media Player* (it was *Redesign Player for TV*), 0.5.6
- ✅ VideoLAN master merged to `5e2c7454ec` (the video surface's hot path, Dolby Vision, VP9 on macOS)
- ✅ Published: [v0.5.6](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.5.6), 23 files; the small updates
  4.8 MB (x64) and 4.5 MB (ARM64) from 0.5.5

### 0.5.5 (2026-11-07)

**The last full release before update releases**, with macOS updating itself:

- ✅ macOS: the app updates itself (the release's disk image, checked against the signed checksums)
- ✅ Releases: from 0.5.6 a version of fixes is an update release (the portable copies and what the apps
  update from); installers and packages come with the big changes
- ✅ Windows: the library's lists and the programme guide no longer copy the time zone per item
- ✅ Android TV: lighter (the mini player and the header no longer work in the background); Android on
  VideoLAN's latest, with a new version code
- ✅ Translations: every catalog on the current texts; VideoLAN master merged to `ad11e4bec1`
- ✅ Published: [v0.5.5](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.5.5), 35 files; the small updates
  5.6 MB (x64) and 5.7 MB (ARM64) from 0.5.4

### 0.5.4 (2026-11-06)

**Android TV, 1:1 with the design**, and VideoLAN's latest fixes:

- ✅ Android TV: the desktop's window at the design's size (title bar, sidebar, header), Home, Videos, Music,
  Network, Playlists and the playlist and album pages, the list rows, the menus, Settings, both players and the
  mini player, checked against the Figma frames
- ✅ Android TV: New Playlist and Open Network Stream as the design's dialogs; hold OK (or Menu) for an item's menu
- ✅ VideoLAN master merged to `da8cf042aa`; Estonian corrected (#53)
- ✅ On Windows the app updates itself from 0.5.3 with a 3.6 MB small update (ARM64: 4.0 MB)

### 0.5.3 (2026-11-06)

**Everything together:** Android TV in the design, the first Samsung TV app, and fixes on the desktop:

- ✅ Android TV: every page in the design with the remote (the welcome, Settings, the players, details, search)
- ✅ Samsung TV: *Juxto Media Player*, the design's own player, installed from a computer
- ✅ The designer's cat lives on the idle screen (its eyes blink, its tail swings, an ear twitches), on every
  version; Live TV's channel banner is an information bar like a cable box's
- ✅ The volume shows in a meter at the side of the window (Settings > Audio > Volume indicator)
- ✅ Windows: no grey strips after a restart in full screen ([#48](https://github.com/KuchiSofts/vlc-redesign/issues/48));
  video starts without waiting for the graphics driver; Settings opens faster (#49–#51 by @SHR1F)

The small update in the app is 7.7 MB from 0.5.2 on Windows x64 (15.3 MB on ARM64). See the
[changelog](CHANGELOG.md#053--2026-11-06).

### 0.5.2 (2026-11-02)

**The Linux updater, and the last checks of the design on Android:**

- ✅ Linux: the AppImage updates itself from the app, as on Windows (the signed checksums checked)
- ✅ Android: subtitles of a video opened from Open Network Stream, the subtitle font, a stream's long press, the
  mini player in right-to-left languages (fixed); recording, the context menu with a mouse and the artist banner
  checked on a real phone
- ✅ Android TV: the first pass of the design (the dark palette, Inter, the accent focus)

See the [changelog](CHANGELOG.md#052--2026-11-02).

### 0.5.1 (2026-11-01)

**Fixes found by a test round of 0.5.0 on every platform**, and the Android app's own update check:

- ✅ Android: the app says when a new version is out (Download, What's new, Skip); TalkBack reads the switches
- ✅ Windows: the window keeps its size when VLC is started minimized; Narrator reads and switches the switches
- ✅ Linux: no stop with hardware decoding (VA-API) on a system without libEGL
- ✅ The startup volume goes up to 200 % ([#30](https://github.com/KuchiSofts/vlc-redesign/issues/30))

See the [changelog](CHANGELOG.md#051--2026-11-01).

### 0.5.0 (2026-10-31)

**The first Android app, and every idea of Juxtopposed's video**, on the desktop and on Android:

- ✅ Pin a folder or a playlist to the sidebar; subtitles from the internet for a whole folder, season or playlist;
  the on-screen messages name their shortcut; the sound output device on Linux and macOS too
- ✅ Android: the design's app, with the subtitle sync timeline, subtitle settings per series, frame previews on
  the seek bar, the waveform seek bar for music and Customize Player profiles
- ✅ The name **VLC Media Player Redesign**, as VideoLAN's trademark policy allows a free (GPL) build
- ✅ Linux: works on newer systems (browser, file manager, media keys), plays without VA-API, a Flatpak
- ✅ Windows: small updates on ARM64 too; a hardened build and release pipeline (pinned actions, checked
  downloads, automatic tests of the parsers and of the updates' signature)

See the [changelog](CHANGELOG.md#050--2026-10-31).

### 0.4.1 (2026-10-30)

**Windows on ARM64**: a native installer and portable zip for Windows 11 on ARM64 (Snapdragon PCs), tested on
GitHub's ARM64 Windows runner. See the [changelog](CHANGELOG.md#041--2026-10-30).

### 0.4.0 (2026-10-30)

**Three platforms together**: Windows x64, Linux (x86_64 and ARM64) and macOS (Apple Silicon and Intel).

- ✅ **Live TV**: your own IPTV playlist (M3U) or Xtream Codes account in a TV page, with what is on now and next,
  a programme guide, catch-up, channel up and down and a channel list over the video; an add-on that can be left out
- ✅ *Suggest an idea* and *Report a problem* in the ⋯ menu and in About; a *What's new* card after an update;
  *Follow the Windows accent color*; subtitle lines marked on the seek bar
- ✅ The playback speed button can be added to the bar again ([#9](https://github.com/KuchiSofts/vlc-redesign/issues/9));
  VLC's latest work merged ([#8](https://github.com/KuchiSofts/vlc-redesign/issues/8))
- ✅ The light theme settled against Figma; every message and classic window in the redesign's look
- ✅ A full code review (about 90 fixes); every text translated in all 72 languages on Weblate

See the [changelog](CHANGELOG.md#040--2026-10-30).

### 0.3.1 and earlier (2026-10-12 to 2026-10-25)

Surround sound on every speaker, casting to TVs, a light theme on every screen, the classic dialogs in the
redesign's look, all 211 settings rows of the design, the library's right-click menus, playlists with their own
picture, the pixel check of all 103 Figma frames, 72 languages changed without a restart. See the
[changelog](CHANGELOG.md).

## Staying close to VLC

Every week a GitHub Action merges the new VLC commits into a pull request, and every pull request is built for
every platform. Our changes outside the interface are listed in the
[changelog](CHANGELOG.md#differences-from-upstream-vlc) so they can be checked at each merge, and dropped when VLC
fixes the same thing.

More ideas are welcome as a **[suggestion](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=feature_request.yml)**.
