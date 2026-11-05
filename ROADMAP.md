# Roadmap

Where **VLC Redesign** is going. The goal stays the same: the real VLC, with
[Juxtopposed's design](https://www.figma.com/community/file/1691465999134370980) on every screen, pixel for pixel,
working smoothly on every device.

Have an idea? Open a **[suggestion](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=feature_request.yml)**.
Found a problem? Open a **[bug report](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=bug_report.yml)**.
Issues with a 👍 from many people move up this list.

Status: ✅ done · 🔨 in progress · 📋 to do · 💡 idea

**Contents:** [Where each program stands](#where-each-program-stands) · [Next](#next-053-and-after) ·
[Each platform](#each-platform) · [Released versions](#released-versions) · [Staying close to VLC](#staying-close-to-vlc)

The full plan, with every Figma frame, its status and the design tokens, is the
[master plan](docs/MASTER-PLAN.md).

## Where each program stands

Updated as the work moves (last: 2026-11-03).

| Program | Status | Progress | What is left |
|---|---|---|---|
| **Windows x64** | ✅ released 0.5.2 | ██████████ 100 % | — 0.5.3: fixes as the issues bring them |
| **Windows on ARM64** | ✅ released 0.5.2 | ██████████ 100 % | a run by hand on a real ARM64 PC |
| **Linux** (x86_64, ARM64) | ✅ released 0.5.2 | ██████████ 100 % | the Flatpak on Flathub |
| **macOS** (Apple Silicon, Intel) | ✅ released 0.5.2 | ██████████ 100 % | tests on a Mac by hand |
| **Android** (phones) | ✅ released 0.5.2 | ██████████ 100 % | — tested on a real phone (Galaxy class) and emulators |
| **Android** (tablets) | ✅ released 0.5.2 | ██████████ 100 % | — tested on a real tablet (Galaxy Tab class) and emulators |
| **Android TV** | 🔨 in progress | ████░░░░░░ 40 % | the player and the pages under the home screen in the design |
| **Samsung TV receiver** | 💡 researched | ░░░░░░░░░░ 0 % | after the Android TV layout |
| **iPhone, iPad** | 💡 later | ░░░░░░░░░░ 0 % | after Android |

## Next: 0.5.3 and after

**0.5.3** is a release of fixes, made as the issues bring them (each fix is first published through a pull request,
with a build to try), and the Android TV layout:

- 🔨 Android TV: the player, its controls and the pages under the home screen in the design, all with a remote
  control (the palette, the Inter font and the accent focus are done in 0.5.2)
- 📋 Linux: the first update of the AppImage from the app, checked from 0.5.2 to 0.5.3 on real desktops
- ✅ The designer's cat lives on the idle screen (its eyes blink, its tail swings, an ear twitches), on every
  version
- ✅ Live TV: the channel banner is an information bar like a cable box's, flush with the player
- ✅ The volume shows in a meter at the side of the window (Settings > Audio > Volume indicator), not as text
- ✅ Settings > Video as the design: the picture sliders without a video, the screensaver a toggle
- ✅ Android: back from a video in a light theme, the library's cards keep their light colours

After it:

- 📋 Linux: the Flatpak on Flathub
- 📋 macOS and Windows on ARM64: a round by hand on a real Mac and a real ARM64 PC (both are tested on GitHub's
  machines at every build)
- 📋 Windows: tests on more real PCs (Windows 10, Intel graphics, screens of different scales)
- 📋 Code-signed Windows releases (no SmartScreen warning): waiting for SignPath's approval
- 💡 The Samsung TV receiver (the TV side of casting, drawn in the design), then iPhone and iPad

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
- 📋 Tests on a Mac by hand; signing by Apple (it needs a developer account); an updater in the app

**Android** (phones and tablets since 0.5.0, TV in progress)

- ✅ Its own app (`org.kuchisofts.vlcredesign`, beside VLC): the design's Home, library, player and Customize
  Player, casting, effects, recording, subtitles in every script and right to left, Live TV, the tablet layout
- ✅ An APK on the [release page](https://github.com/KuchiSofts/vlc-redesign/releases/latest) (not on Google Play);
  the app says when a new version is out
- 🔨 Android TV: the first pass of the design is done (0.5.2); the player and the pages are next

**Later**

- 💡 Samsung TV receiver, iPhone and iPad

## Released versions

Newest first. Every change is in the [changelog](CHANGELOG.md).

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
