# Roadmap

Where **VLC Redesign** is going. The goal stays the same: the real VLC, with
[Juxtopposed's design](https://www.figma.com/community/file/1691465999134370980) on every screen, pixel for pixel,
working smoothly on every PC.

Have an idea? Open a **[suggestion](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=feature_request.yml)**.
Found a problem? Open a **[bug report](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=bug_report.yml)**.
Issues with a 👍 from many people move up this list.

Status: ✅ done · 🔨 in progress · 📋 to do · 💡 idea

The full plan, with every Figma frame, its status and the design tokens, is the
[master plan](docs/MASTER-PLAN.md).

## Where each program stands

Updated as the work moves (last: 2026-11-01).

| Program | Status | Progress | What is left |
|---|---|---|---|
| **Windows x64** | ✅ released 0.5.0 | ██████████ 100 % | — 0.5.1: fixes as the issues bring them |
| **Windows on ARM64** | ✅ released 0.5.0 | ██████████ 100 % | a run by hand on a real ARM64 PC |
| **Linux** (x86_64, ARM64) | ✅ released 0.5.0 | ██████████ 100 % | the Flatpak on Flathub; an updater in the app later |
| **macOS** (Apple Silicon, Intel) | ✅ released 0.5.0 | ██████████ 100 % | tests on a Mac by hand |
| **Android** (phones) | ✅ released 0.5.0 | ██████████ 100 % | — tested on a real phone (Galaxy class) and emulators |
| **Android** (tablets) | ✅ released 0.5.0 | █████████░ 98 % | a test on a real tablet (done on 11" and 13" emulators) |
| **Samsung TV receiver** | 💡 researched | ░░░░░░░░░░ 0 % | next after 0.5.1 |
| **iPhone, iPad** | 💡 later | ░░░░░░░░░░ 0 % | after Android |

## Next: 0.5.1 and after

**0.5.1** is a release of fixes, made as the issues bring them (each fix is first published through a pull request,
with a build to try), and the Android app's own update check (it says when a new APK is out). After it:

- 📋 Android: the TV layout (a remote control), and a test on a real tablet (the tablet layout was tested on 11" and
  13" emulators)
- 📋 Linux: the Flatpak on Flathub, and an updater in the app as on Windows
- 📋 macOS and Windows on ARM64: a round by hand on a real Mac and a real ARM64 PC (both are tested on GitHub's
  machines at every build)
- 💡 The Samsung TV receiver (the TV side of casting, drawn in the design), then iPhone and iPad

## 0.5.0 (published 2026-10-31)

**The first Android app, and every idea of Juxtopposed's video**, on the desktop and on Android:

- ✅ Pin a folder or a playlist to the sidebar (*Pin to Sidebar* in their menus, *Unpin* on the row; on Android in
  the tablet sidebar and at the top of Home, moved and hidden in *Edit Sidebar*)
- ✅ Subtitles from the internet for a whole folder, season or playlist, each episode matched on its own
- ✅ The on-screen messages name the shortcut that made them ("Volume: 115%" · "Up")
- ✅ Choose the sound output device on Linux and macOS too (*Settings › Audio › Output device*)
- ✅ Android: the subtitle sync timeline, subtitle settings kept per series, frame previews on the seek bar, the
  waveform seek bar for music kept between starts, Customize Player profiles, the last icons as the desktop's

**The Android app** (a fork of VLC for Android with the design, its own package `org.kuchisofts.vlcredesign`, beside
VLC) has the design's Home, library, player bar and Customize Player, the compact menus, casting like the desktop,
effects, recording, subtitles in every script and right to left, Live TV, the tablet layout and the designer's
credit. An APK on the release page, not on Google Play; tested on a real phone and on emulators.

0.5.0 also brings:

- ✅ The app's name is **VLC Media Player Redesign**, as VideoLAN's trademark policy allows a free (GPL) build
- ✅ Linux: the programs VLC opens (browser, file manager, D-Bus for the media keys and the screen saver) work on
  newer systems; fewer bundled system libraries; the AppImage and the folder play videos on a system without VA-API;
  a Flatpak; tested on Ubuntu 24.04/26.04, Debian 13, Fedora 41/43 and Arch
- ✅ Windows: the firewall rule for casting runs Windows' own netsh by its full path; quitting no longer waits for
  Windows' question about it; stricter checks of updates; small updates for Windows on ARM64 too
- ✅ The startup volume goes up to 200 % ([#30](https://github.com/KuchiSofts/vlc-redesign/issues/30))
- ✅ The build and release pipeline hardened (actions pinned to commits, downloads checked against their hashes),
  new automatic tests of the parsers and of the updates' signature

See the [changelog](CHANGELOG.md#050--2026-10-31).

## 0.4.1 (published 2026-10-30)

**0.4.1, a release of fixes the same day as 0.4.0, brings Windows on ARM64**: a native ARM64 installer and portable
zip for Windows 11 on ARM64 (Snapdragon PCs). The crash at start was the `longjmp` table of Control Flow Guard,
which llvm-mingw never fills; the ARM64 build is linked without it now. Tested on GitHub's real ARM64 Windows runner:
every binary ARM64, a clip played to its end, a clean quit, and the installer installs, starts, plays and
uninstalls. Windows x64, Linux and macOS have the same packages, numbered 0.4.1. See the
[changelog](CHANGELOG.md#041--2026-10-30).

## 0.4.0 (published 2026-10-30)

**0.4.0 shipped on three platforms together** (2026-10-30): Windows x64, Linux (x86_64 and ARM64) and macOS (Apple
Silicon and Intel). Windows on ARM64 moves to the next version: in the 0.4.0 tests on a real ARM64 Windows runner it
opened its window and stopped a few seconds later (a crash in the Qt interface after the media library started), and
one of the installer's 400 binaries was not ARM64.

**Done since 0.3.1**

- ✅ The playback speed button can be added to the bar again ([#9](https://github.com/KuchiSofts/vlc-redesign/issues/9));
  VLC's latest work merged ([#8](https://github.com/KuchiSofts/vlc-redesign/issues/8))
- ✅ The light theme settled against Figma: every place that asked for its own light colour checked, the
  classic windows' title bar now as light as the other dialogs
- ✅ A full code review: about 90 fixes (installer and updates, casting, subtitles in older encodings, colours of SD
  video, profiles and settings, the Windows proxy, library and dialogs)
- ✅ Every text of VLC Redesign translated in all 72 languages on Weblate, and about 80 wrong translations fixed

**In progress**

- 🔨 **Live TV**: the channels of your own IPTV playlist (M3U) or Xtream Codes account in a TV page, with what is
  on now and next, a programme guide, catch-up of past programmes, channel up and down, numbers and a channel
  list over the video, and a catalog of free, legal channel lists; an add-on, on by default, that can be left out
- 🔨 *Suggest an idea* and *Report a problem* in the ⋯ menu and in About: the GitHub form opens filled with what
  helps (version, Windows, graphics card, screens), never anything personal
- 🔨 A *What's new* card after an update: the highlights of the new version, once, the first time VLC opens after
  an update (it can be turned off in Settings, General); built, to be checked in the running app
- 🔨 The last checks in the running app: Settings scroll bars, the warning colour, *Show controls in fullscreen*,
  a playlist's own picture, the small classic windows
- 🔨 Subtitle lines marked on the seek bar (from the subtitle file shown), with the line at the hovered time in
  the preview: built, being checked in the app
- 🔨 *Follow the Windows accent color* in Settings, Appearance: built, being checked in the app
- 🔨 Every message, question and classic window in the redesign's look (its own title bar), and Go to Time,
  Plugins, Media Statistics and Folder information redrawn: built, being checked in the app

**To do**

- 📋 Code-signed releases (no SmartScreen warning): waiting for SignPath's approval
- 📋 A public roadmap board ([Discussions](https://github.com/KuchiSofts/vlc-redesign/discussions) are open)
- 📋 Testing on Intel graphics, Windows 10 on real PCs and screens of different scales

**Done before** (0.3.0 and 0.3.1): surround sound on every speaker, casting to TVs, a light theme on every screen,
the classic dialogs in the redesign's look, all 211 settings rows of the design, the library's right-click menus,
playlists with their own picture, the pixel check of all 103 Figma frames, 72 languages chosen in the installer and
changed without a restart. The full list is in the [changelog](CHANGELOG.md).

**Ideas**

- 💡 More ideas welcome in [Discussions](https://github.com/KuchiSofts/vlc-redesign/discussions/categories/ideas)

## Other platforms

Where each platform stands is kept in one file, [`platforms.yml`](platforms.yml): the bar at the top of the README,
the website and the *Also on* line of the release notes and of About are made from it.

- 🔨 A website for every platform (GitHub Pages): it offers the download for your system and lists the others
- ✅ Linux (x86_64 and ARM64), released with 0.4.0: a .deb, an .rpm, an AppImage and a tarball, next to the
  distribution's own VLC, tested in containers on 7 distributions and on GitHub's x86_64 and ARM64 runners
- 🔨 Linux: the Flatpak (the manifest is written), an updater in the app, tests on real desktops
- ✅ macOS (Apple Silicon and Intel), released with 0.4.0: a disk image for each, built and tested by GitHub's Macs,
  signed ad hoc (macOS asks once to open it)
- 🔨 macOS: signing by Apple (it needs a developer account), an updater in the app, tests on a Mac by hand
- ✅ Windows on ARM64, released with 0.4.1: a native installer and portable zip for Windows 11 on ARM64, tested on
  GitHub's ARM64 Windows runner
- 🔨 Windows on ARM64: a run by hand on a real ARM64 PC (Snapdragon); small updates in the app from 0.5.0 on
- 🔨 The design for every platform: its colours, sizes, fonts and icons in [`design/`](design/README.md)
  (`design-1.0`), with CSS, Android and Swift files made from them and a check on every pull request that the
  Windows app still uses the same values
- ✅ Android (phones and tablets): released with 0.5.0; Android TV after it
- 💡 Samsung TV receiver, iPhone and iPad

## Staying close to VLC

Every week a GitHub Action merges the new VLC commits into a pull request, and every pull request is built
for Windows. Our changes outside the interface are listed in the
[changelog](CHANGELOG.md#differences-from-upstream-vlc) so they can be checked at each merge, and dropped
when VLC fixes the same thing.
