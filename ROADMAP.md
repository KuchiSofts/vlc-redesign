# Roadmap

Where **VLC Redesign** is going. The goal stays the same: the real VLC, with
[Juxtopposed's design](https://www.figma.com/community/file/1691465999134370980) on every screen, pixel for pixel,
working smoothly on every device.

Have an idea? Open a **[suggestion](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=feature_request.yml)**.
Found a problem? Open a **[bug report](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=bug_report.yml)**.
Issues with a 👍 from many people move up this list.

Status: ✅ done · 🔨 in progress · 📋 to do · 💡 idea

**Contents:** [Where each program stands](#where-each-program-stands) · [Next](#next-088-and-after) ·
[Each platform](#each-platform) · [Released versions](#released-versions) · [Staying close to VLC](#staying-close-to-vlc)

The full plan, with every Figma frame, its status and the design tokens, is the
[master plan](docs/MASTER-PLAN.md).

## Where each program stands

Updated as the work moves (last: 2026-11-26, 0.8.7 published, 0.8.8 started).

| Program | Status | Progress | What is left |
|---|---|---|---|
| **Windows x64** | ✅ released 0.8.7 | ██████████ 100 % | — fixes as the issues bring them |
| **Windows on ARM64** | ✅ released 0.8.7 | ██████████ 100 % | a run by hand on a real ARM64 PC |
| **Linux** (x86_64, ARM64) | ✅ released 0.8.7 | ██████████ 100 % | the Flatpak on Flathub |
| **macOS** (Apple Silicon, Intel) | ✅ released 0.8.7 | ██████████ 100 % | tests on a Mac by hand |
| **Android** (phones) | ✅ released 0.8.7 | ██████████ 100 % | — tested on a real phone (Galaxy class) and emulators |
| **Android** (tablets) | ✅ released 0.8.7 | ██████████ 100 % | — tested on a real tablet (Galaxy Tab class) and emulators |
| **Android TV** | ✅ released 0.8.7 | ██████████ 100 % | — 1:1 with the Figma frames, lighter (no work in the background); tests on more TV boxes |
| **Samsung TV** (*Juxto Media Player*) | ✅ released 0.8.6 | ██████████ 100 % | — tested on a 2017 Samsung TV; the Samsung store (later) |
| **iPhone, iPad** | 💡 later | ░░░░░░░░░░ 0 % | after Android |

## Next: 0.8.8 and after

**0.8.8** is an update release of fixes, made as the issues bring them (each fix is first published through a pull
request, then released).

- 🟡 macOS: the first update from inside the app on the Mac of [#117](https://github.com/KuchiSofts/vlc-redesign/issues/117)
  (0.8.3 installed by hand once: the fix is in the app that updates)
- 🟡 Three pull requests of a contributor that are still drafts, each reviewed (#237 for Linux only, #245,
  #251's macOS part), and #187 (sliders and choice lists), where the author was asked for the steps that show
  the fault

And as the issues bring them:

- 📋 Live TV: the servers tried on real ones (Tvheadend, NextPVR, Channels DVR, an HDHomeRun tuner) and a paid
  Schedules Direct account; on Android and the Samsung TV app a Tvheadend server that takes only Digest sign-in
  plays with its access code (their players send Basic)
- 📋 The subtitle services with keys and accounts tried with real keys (SubSource, SubDL, OpenSubtitles.com and the
  seven others)
- 📋 Linux: the first update of the AppImage from the app, checked from one version to the next on real desktops
- 📋 Samsung TV: a USB drive played on a real TV (the folders of the TV's own storage are checked)
- 📋 Android TV: tests on more TV boxes (Google TV, Fire TV)

After it:

- 📋 Linux: the Flatpak on Flathub
- 📋 macOS and Windows on ARM64: a round by hand on a real Mac and a real ARM64 PC (both are tested on GitHub's
  machines at every build)
- 📋 Windows: tests on more real PCs (Windows 10, Intel graphics, screens of different scales)
- 📋 Code-signed Windows releases (no SmartScreen warning): SignPath Foundation accepted the project (11-20); the
  test signing works on x64 and ARM64; SignPath reviews the setup (11-20), then the release certificate
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

### 0.8.7 (2026-11-26)

**An update release** of the desktop apps and the Android app (the Samsung TV app stays 0.8.6):

- ✅ Six more changes by a contributor (#249, #250, #252 to #255): Home's search with no matches, an empty
  playlist exported, Default Preset and the Video menu's crop, the accent after a reset, a subtitle link that
  answers with a web page, the built-in file browser setting
- ✅ The follow-ups of three of 0.8.6's changes (#238, #241, #246)
- ✅ VideoLAN's latest changes on the desktop (colours across popups and windows) and on Android (the fonts
  cache, libvlc 3.7.8)

### 0.8.6 (2026-11-26)

**An update release** of every app:

- ✅ Ten more changes by a contributor (#236, #238 to #244, #246, #247), each read in full: Browse (Back, the sort
  order), the equalizer preset and Effects' Cancel, subtitle settings and downloads, a play queue that cannot be
  saved, Recent's thumbnails, the services' address bar, Reload of a library folder
- ✅ Android: the app beyond the player read as a whole, nine faults fixed (a bad lyrics file, Folders, Export
  playlist, the remote on a TV, the update check)
- ✅ Samsung TV: the app read as a whole, ten faults fixed (the focus after the player closes, Remove in the
  queue, quick seeks, Repeat all, the volume, the screen saver)
- ✅ VideoLAN's master merged (their update button of the side pane: this interface keeps its own)

### 0.8.5 (2026-11-26)

**An update release** of the desktop apps and the Android app (Samsung TV stays 0.8.4):

- ✅ Forty-four changes by a contributor (#177 to #186, #193 to #203, #205, #206, #208, #209, #214 to #216, #218,
  #220 to #234), each read in full and shown before and after where it can be seen: the keyboard (the sidebar,
  pane edges, About, the Play button, Add to Playlist, the library's search), screen readers, the tray, Open
  Disc, Messages, saved playlists, Continue Watching
- ✅ The floating Play Queue as the design draws the queue, in a window of the main window's style
- ✅ The sidebar, the play queue and the artists pane resized with the keyboard
  ([#45](https://github.com/KuchiSofts/vlc-redesign/discussions/45))
- ✅ Android: the player after a review of 0.8.4's phone change (Customize Player's Landscape layout
  shown again on a phone held sideways, the top bar after a rotation, subtitles above the controls)

### 0.8.4 (2026-11-26)

**An update release** of every app:

- ✅ Nine changes by a contributor (#159, #161 to #163, #165, #169 to #172), each shown to the owner before and
  after and then taken: the queue's length, old update files removed off the start, the waveform drawn once,
  folders sorted and searched off the interface, playback options kept in a saved playlist, Live TV rows and
  quick channel presses, an update's progress, the subtitles dialog in its place
- ✅ Music seek bar: the waveform kept between runs and prepared for the next song
- ✅ Music folders keep their order (a played song is not a watched video)
- ✅ The update card lists what concerns your system, each line whole
- ✅ Linux: sound on Fedora and other systems with a newer PipeWire ([#207](https://github.com/KuchiSofts/vlc-redesign/issues/207))
- ✅ A second VLC window no longer closes on Settings › Library ([#211](https://github.com/KuchiSofts/vlc-redesign/issues/211))
- ✅ Android: the player on a phone held sideways, the seek bar on its own row and a slim top bar ([#213](https://github.com/KuchiSofts/vlc-redesign/issues/213))
- ✅ macOS: an update that takes too long stops cleanly
- ✅ Android TV: the remote reaches the header's buttons and the Favorites row, the focus returns to the file
  played, Play all only where something plays
- ✅ Samsung TV: the "deleted or moved" message (Back, Shuffle, files with special characters in their name)
- ✅ VideoLAN's new way to build the interface's QML brought into our build files

### 0.8.3 (2026-11-25)

**An update release** of every app:

- ✅ macOS: updates install from inside the app ([#117](https://github.com/KuchiSofts/vlc-redesign/issues/117)): the app never saw its update helper
  end (VLC blocks the signal that says so); the waveform seek bar had the same cause
- ✅ The speaker button brings the sound back ([#145](https://github.com/KuchiSofts/vlc-redesign/issues/145))
- ✅ Eleven fixes by a contributor (#147 to #157): the TV guide on resize and after a refresh, channel-number
  searches, the latest channel chosen, the file deleted is the one picked, breadcrumbs on network shares,
  Resync of the open folder, the output device kept and same-name devices, lyrics read off the interface
- ✅ A scroll bar in the player's library panel ([#143](https://github.com/KuchiSofts/vlc-redesign/discussions/143))
- ✅ Samsung TV: the queue into a playlist, a deleted or moved file leaves the queue and the history
- ✅ Android TV: Right moves across the columns of a folder list (only the first column could be reached),
  Play all in a folder's menu, the focus kept after playback
- ✅ Every issue checked on every app: Repeat and Shuffle on Linux, Android and the Samsung TV app too

### 0.8.2 (2026-11-23)

**An update release** of every app, the Samsung TV app's too:

- ✅ Android: the lists and messages of dialogs in the text colour itself (light theme: 5.9:1, was 2.8:1)
- ✅ Samsung TV: the song's cover in its music view, with the desktop's rule for the texts over it
- ✅ Repeat and Shuffle take effect at once ([#138](https://github.com/KuchiSofts/vlc-redesign/issues/138))
- ✅ Folders in the player's library panel ([#139](https://github.com/KuchiSofts/vlc-redesign/issues/139))
- ✅ macOS, from a full check of a user's log and every macOS line: the audio module's errors, the plugin cache
  never written into the signed app, Metal at once

### 0.8.1 (2026-11-23)

**An update release** (the Samsung TV app stays 0.8.0):

- ✅ A deleted or moved file leaves the queue and the history, with one message (desktop and Android)
- ✅ The queue into a playlist from its menu ([#135](https://github.com/KuchiSofts/vlc-redesign/issues/135))
- ✅ Android: sort and Play all (with or without subfolders) in Folders, *Sort by…* in the library's menu
  ([#132](https://github.com/KuchiSofts/vlc-redesign/discussions/132))
- ✅ macOS: the update from a zip of the app, no disk image to open
  ([#117](https://github.com/KuchiSofts/vlc-redesign/issues/117))
- ✅ The in-app *What's new* lists only what you see: a line about the release tooling left 0.8.0's

### 0.8.0 (2026-11-23)

**A full release**, every package (the Samsung TV app's 0.8.0 joined it the same day, after a run on a real TV):

- ✅ A full check of every app: every page, dialog, menu, Settings page and key with the log on; video from 3GP to
  8K and up to 240 frames a second; memory over long playback
- ✅ Android: a Folders tab in Videos and Music; Tracks: Play all and Shuffle
- ✅ Full screen keeps the window buttons; the TV guide updates at once and says Today in the evening
- ✅ The accent colour everywhere; the next subtitle line after a switch; no freeze after a video starts
- ✅ The README's and the wiki's clips recorded again on 0.8.0; VideoLAN's latest changes merged

### 0.7.8 (2026-11-22)

**An update release** (Android stays 0.7.5, the Samsung TV app 0.6.8):

- ✅ The queue panel keeps its width; the queue's total length is right
- ✅ Home in the player's library panel; a library page loads fully after Stop

### 0.7.7 (2026-11-22)

**An update release** (Android stays 0.7.5, the Samsung TV app 0.6.8):

- ✅ Escape closes the welcome dialog again (a regression of 0.7.6)

### 0.7.6 (2026-11-22)

**An update release** (Android stays 0.7.5, the Samsung TV app 0.6.8):

- ✅ The library without a mouse and before anything plays (#124): the keys after Stop and at the start,
  Space starting the queue, the player bar with its Play Queue button shown by default, Ctrl+1 … Ctrl+0
  playing the media played last again
- ✅ The blurred cover art with the software renderer

### 0.7.5 (2026-11-18)

**An update release** (the Samsung TV app stays 0.6.8):

- ✅ macOS: an update that can't be prepared says so within minutes and offers the download (#117)
- ✅ The redesign's texts complete in the 72 languages of Weblate, and in 61 languages of the Android app
- ✅ The update of an installed copy tried on macOS 15 and 26 with every build

### 0.7.4 (2026-11-17)

**An update release** (the Samsung TV app stays 0.6.8):

- ✅ Folders in the sidebar: the library's folders, one inside the other (#112)
- ✅ Home's list view: room under the titles, songs' own covers, real lengths (#111)
- ✅ Songs that versions before 0.6.2 listed as videos are read again once (#111)

### 0.7.3 (2026-11-17)

**An update release** (the Samsung TV app stays 0.6.8):

- ✅ Episodes grouped by their show's name and played in order (#108)
- ✅ Lyrics without times follow the song to their last line (#109)
- ✅ The wheel at the end of the queue no longer changes the volume (#113)
- ✅ Compact mode without a video shows the cone; turning off last.fm no longer errs or freezes
- ✅ Screen readers name every choice of the lists; Linux lists its optical drives
- ✅ Android: What's new no longer shows an old list in other languages

### 0.7.2 (2026-11-16)

**An update release** (the Samsung TV app stays 0.6.8):

- ✅ The music player's lyrics button stays in a smaller window; opening the lyrics takes a third of the memory it
  took for a moment
- ✅ macOS and Linux: the System theme draws no box behind the icons of the bars

### 0.7.1 (2026-11-16)

**An update release** of fixes (issues #100 to #103; the Samsung TV app stays 0.6.8):

- ✅ The music player's title and lyrics stay readable over a bright cover (Windows, Linux, macOS and Android)
- ✅ Lyrics without times show as a page that moves with the song
- ✅ Repeat one has its own icon; Reverse scroll direction turns the volume wheel round too
- ✅ macOS: Help › Report a problem and Suggest an idea open; a Settings search names the page of each group

### 0.7.0 (2026-11-16)

**A full release** (the .deb, .rpm and Flatpak packages again; the Samsung TV app stays 0.6.8):

- ✅ Smaller, smarter updates on Windows: one small update a release (0.8 MB where 0.6.9's was 4.0 MB), and a copy
  several versions behind takes the ones between by itself, each checked against its own release's signature
- ✅ A release page with few files and short notes; a release made with one command
- ✅ About no longer shows an error of the daily update check

### 0.6.9 (2026-11-15)

**An update release** of fixes (the Samsung TV app stays 0.6.8):

- ✅ Pointing at a pane's edge no longer resizes it: only a drag does (a fault of 0.6.8, on Windows, Linux and macOS)
- ✅ Android: Edit Sidebar lines up with the More page's other rows, and its window speaks of the arrows it has

### 0.6.8 (2026-11-15)

**An update release** from the ideas of the Discussions, on every app:

- ✅ Delete File From System moves the file to the Recycle Bin (the Trash on macOS and Linux)
  ([#46](https://github.com/KuchiSofts/vlc-redesign/discussions/46), [#94](https://github.com/KuchiSofts/vlc-redesign/pull/94));
  on Android 11+ a deleted media file goes to the system trash ([Android #13](https://github.com/KuchiSofts/vlc-redesign-android/pull/13))
- ✅ Lyrics from an `.lrc` file next to the song on Windows, Linux and macOS, and in the Samsung TV app's music view
  ([#60](https://github.com/KuchiSofts/vlc-redesign/discussions/60))
- ✅ The sidebar's resize edge shows itself when pointed at and is easier to grab
  ([#45](https://github.com/KuchiSofts/vlc-redesign/discussions/45))

### 0.6.7 (2026-11-15)

**An update release** of fixes (the Samsung TV app stays 0.6.6):

- ✅ Android: a video opened from another app while a song plays is the one played; the texts European Portuguese
  and Traditional Chinese still had in English
  ([Android #12](https://github.com/KuchiSofts/vlc-redesign-android/pull/12))
- ✅ About keeps its buttons in a low window; Hebrew and Arabic texts that began with a Latin word read from the
  right; a memory soak in the macOS CI ([#93](https://github.com/KuchiSofts/vlc-redesign/pull/93))

### 0.6.6 (2026-11-14)

**An update release** (the installers, the portable copies and what the apps update from; the Linux packages stay on
0.6.0), with the Samsung TV app:

- ✅ Android with the desktop's last options it can have: lyrics, effect presets, Keyboard Shortcuts, the other player
  bar buttons, subtitles from a link, added audio tracks and more
  ([Android #11](https://github.com/KuchiSofts/vlc-redesign-android/pull/11))
- ✅ Live TV says why a source could not be loaded in your language, on every app; What's new lists this system's
  changes; Linux has the HTTP proxy settings ([#92](https://github.com/KuchiSofts/vlc-redesign/pull/92))

### 0.6.5 (2026-11-14)

**An update release** (the installers, the portable copies and what the apps update from; the Linux packages stay on
0.6.0; the Samsung TV app on 0.6.1):

- ✅ Android with the desktop's options: nearly 40 settings, the queue's Sort and Add files, Export Playlist, the check
  for updates in About, the computer's keys ([Android #10](https://github.com/KuchiSofts/vlc-redesign-android/pull/10))
- ✅ Updates: the newest release of this system, not GitHub's latest one
  ([#91](https://github.com/KuchiSofts/vlc-redesign/pull/91))

### 0.6.4 (2026-11-14)

**An update release** (the installers, the portable copies and what the apps update from; the Linux packages stay on
0.6.0; Android stays on 0.6.3, the Samsung TV app on 0.6.1):

- ✅ macOS with every feature of Windows: the media keys and Now Playing, the Dock menu (the taskbar's buttons),
  global hotkeys, Show in folder in Finder, Process priority, VLC's own HTTP proxy, adding the files opened from
  Finder to the playlist, Show VLC Redesign from the menu bar icon and the Dock, casting on macOS 15
  ([#90](https://github.com/KuchiSofts/vlc-redesign/pull/90))

### 0.6.3 (2026-11-13)

**An update release** (the installers, the portable copies and what the apps update from; the Linux packages stay on
0.6.0):

- ✅ macOS: files opened from Finder or dropped on the Dock icon play
  ([#84](https://github.com/KuchiSofts/vlc-redesign/issues/84), [#85](https://github.com/KuchiSofts/vlc-redesign/issues/85),
  @xITmasterx); the menu bar has every menu ([#88](https://github.com/KuchiSofts/vlc-redesign/issues/88)); video shows again
  after a failed graphics check ([#89](https://github.com/KuchiSofts/vlc-redesign/issues/89)); Customize Player keeps a
  hidden button's place ([#87](https://github.com/KuchiSofts/vlc-redesign/issues/87))
- ✅ A faster start: the window is up about 0.2 s sooner
- ✅ Screen readers name every text field and settings toggle; outside text is always shown as plain text
- ✅ Android: no crash in Hindi when adding items already in a playlist; Korean texts fixed
- ✅ VideoLAN's VLC merged to `914c465b0c`; the Samsung TV app is unchanged (0.6.1)

### 0.6.2 (2026-11-12)

**An update release** (the installers, the portable copies and what the apps update from; the Linux packages stay on
0.6.0):

- ✅ Songs whose picture is stored as a video track are listed as music, not as videos
  ([#81](https://github.com/KuchiSofts/vlc-redesign/issues/81), @decider07): Windows, Linux and macOS; the Samsung TV
  app never had it; the Android app has it too: its media library is built here with the fix (VideoLAN's has the bug)
- ✅ Settings › Library lists the library's folders, each with Remove, and Add folder (#81)
- ✅ Indonesian: the interface size's texts (from Weblate)
- ✅ VideoLAN's VLC merged to `6b5f85b1d1`

### 0.6.1 (2026-11-12)

**An update release** of fixes (the installers, the portable copies and what the apps update from; the Linux
packages stay on 0.6.0):

- ✅ Published with 23 files on its [release page](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.6.1),
  under one signed list of checksums; the apps update themselves to it (on Windows a small update of 7.2 MB from
  0.6.0, checked from a fresh copy of 0.6.0: it restarted itself as 0.6.1)
- ✅ Windows: closing never waits for a graphics driver that is slow to start (the close could hang)
- ✅ Every option answers: each control of Settings used and read back on the desktop (14 pages), the Samsung TV app
  (on a TV too), and the phone and tablet layouts of Android. On the desktop the lists, the sliders and the
  start-up volume choice now work from a screen reader, *Rescan plugins* shows that it is scanning, and *Add* and
  *Remove* of Extensions are greyed until an add-on is selected
- ✅ Android TV: the remote reaches the mini player and its queue button; the queue opens as a panel over the mini
  player
- ✅ Android: an empty Home says why and asks for access to your files; a screen reader says whether a subtitle
  service or language is on; the feedback page reports to the project's GitHub issues
- ✅ Samsung TV: the catch-up mark no longer covers a channel's logo; a guide download that is cancelled stops at
  once
- ✅ Translations: "Unknown Artist" in 72 languages, a sentence shown twice in 12 languages, one Hebrew word for a
  playlist in every app
- ✅ What's new shows headlines without the changelog's issue references
- ✅ VideoLAN's VLC merged to `82ca0542a1`, VLC for Android to `14a7422459`

### 0.6.0 (2026-11-11)

**A full release** of every app, with every installer and package:

- ✅ Published with 35 files on its [release page](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.6.0):
  Windows x64 and ARM64 (installers, portable copies, small updates from 0.5.6, 0.5.7 and 0.5.8: 10.6 MB from
  0.5.8), Linux x86_64 and ARM64 (AppImage, .deb, .rpm, Flatpak, a folder to run from), macOS (Apple Silicon and
  Intel), the Android APK and the Samsung TV app's source, under one signed list of checksums.
- ✅ In full screen, keys and the remote seek, change the volume or pause without showing the controls, if you
  choose (a slim progress line and the time, or a short message instead), as in VLC 3, in every app
  ([#72](https://github.com/KuchiSofts/vlc-redesign/issues/72), @BarbodF): desktop, Android (phones, tablets, TV)
  and the Samsung TV app
- ✅ Interface size: Settings › Interface › Window › *Interface size*: Automatic, 100 to 200 %, applied at once;
  every window, dialog, menu and popup opens inside its screen and scrolls when it is taller than the window; rows
  and grids of cards fill the page's width with whole cards at every window size; empty pages show the page's icon
- ✅ Stability, in every app: every line of the newest code reviewed, the parsers fuzzed, quitting and Live TV
  bounded, stalled servers stopped, no secret header sent to another server on a redirect, the interface's thread
  free of network and disk work, no QML warning left; the desktop starts faster (window 0.60 → 0.48 s)
- ✅ Smarter automatic subtitles in every app: a film opened as the app starts
  gets them too; a site's advert or the forced lines beside the video no longer stop the search; the video's own
  Subs folder is used; Hebrew and Arabic punctuation written for left-to-right players shows in its place
- ✅ Online subtitles for videos played from the network (a link, SMB, NFS, UPnP/DLNA) on Android, manual and
  automatic, as on the desktop ([#73](https://github.com/KuchiSofts/vlc-redesign/issues/73), @Elshepl); the
  Samsung TV app too
- ✅ The Subtitles panel the same on every device: the desktop's two columns (the languages and their subtitles),
  the search in it and the timing; on the Samsung TV app with the remote; on Android (phones, tablets, TV) opened
  above its button as on the desktop
- ✅ Android TV with the remote alone: every option of Settings pressed (about 170), the page follows the sidebar,
  dialogs start on a button, the number and colour pickers work with the arrows, a link shows its address where
  there is no browser; Live TV as the desktop's cards (or a list), its Cards / List button always there
- ✅ Android installs next to VLC for Android (it was refused before), and Android TV in Hebrew and Arabic is
  mirrored everywhere, its focus where it should be; a channel's name and a programme's title keep their own
  direction there and on the Samsung TV app
- ✅ VideoLAN's master merged to `fb39292a88` (Qt 6.8 required from now on)

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
