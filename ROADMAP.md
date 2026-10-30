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

Updated as the work moves (last: 2026-10-30).

| Program | Status | Progress | What is left |
|---|---|---|---|
| **Windows x64** | ✅ released 0.4.1 | ██████████ 100 % | — (tests go on with every change) |
| **Windows on ARM64** | ✅ released 0.4.1 | ██████████ 100 % | a run by hand on a real ARM64 PC; small updates in the app from 0.5.0 |
| **Linux** (x86_64, ARM64) | ✅ released 0.4.1 | ██████████ 100 % | the Flatpak (built and tested in CI, next release); an updater in the app |
| **macOS** (Apple Silicon, Intel) | ✅ released 0.4.1 | ██████████ 100 % | tests on a Mac by hand |
| **Android** (phones) | 🔨 in testing | █████████░ 98 % | the last fixes (below), then a test on real phones and a first release |
| **Android** (tablets) | 🔨 in testing | █████████░ 90 % | the same, then a test on a real tablet |
| **Samsung TV receiver** | 💡 researched | ░░░░░░░░░░ 0 % | after the Android release |
| **iPhone, iPad** | 💡 later | ░░░░░░░░░░ 0 % | after Android |

## Next: the Android app, and 0.5.0

**The Android app** is a separate app (a fork of VLC for Android with the design), in testing on the emulator and on
a real phone. Done: the design's Home, library, player bar and Customize Player (zones, profiles), the compact menus,
casting like the desktop (Chromecast and DLNA TVs), effects, recording, subtitles in every script and right to left,
Live TV (playlists, Xtream Codes, the guide, catch-up, films and series), the tablet layout (sidebar, list and
detail), the designer's credit and *What's new*. An automatic tour of every screen (phone and tablets, dark and
light, English, Hebrew and German) found no crash. Left before its first release:

- ✅ Live TV: a guide from a server that answers in HTTP/1.0 — the failure seen once did not come back (10 loads in a
  row); a failed download now logs where it broke
- ✅ Live TV: the day shown above the guide on a phone (it said *Tomorrow* after *Now* late in the evening)
- ✅ Screens kept in memory after they close: the cause (a listener added at each return to the app, never
  removed) fixed; 50 rounds of relaunches, playback, rotations and settings keep none
- 🔨 The light theme, the borders of the dialogs and the icons as sharp as on the desktop (parity with Windows)
- ✅ The texts of the last rounds (About, fonts, effects, casting, recording, help, Live TV) translated in the 61
  languages the app speaks (1,263 texts)
- ✅ Its own package name, `org.kuchisofts.vlcredesign`: it installs beside VLC and can be published
- ✅ No more offers of VideoLAN's nightly builds (they are VideoLAN's app, not this one)
- 🔨 14 languages of VLC for Android that are barely translated yet (Faroese, Lao, Swahili and others)
- 📋 A test on real phones and a tablet, then the first release (its own version numbers)

**0.5.0** (desktop) gathers the new features made after 0.4.x: Windows on ARM64 gets the small updates in the app,
the Linux Flatpak, and what the issues and the Android work bring back to the desktop.

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
- 🔨 Android (phones and tablets): the app is in testing, see [Next](#next-the-android-app-and-050); Android TV after it
- 💡 Samsung TV receiver, iPhone and iPad

## Staying close to VLC

Every week a GitHub Action merges the new VLC commits into a pull request, and every pull request is built
for Windows. Our changes outside the interface are listed in the
[changelog](CHANGELOG.md#differences-from-upstream-vlc) so they can be checked at each merge, and dropped
when VLC fixes the same thing.
