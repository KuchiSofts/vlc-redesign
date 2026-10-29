# Platforms

VLC Redesign **0.4.0** is released for **Windows 10 (version 1809 or later) and Windows 11, 64-bit** (x64). Linux and
macOS builds are in progress, Windows on ARM64 is planned, and the Android app is in testing as a separate app. This
page gives, for each platform, its status, its version, what it needs, its files, what is tested and what is
missing.

The status of each platform is kept in one file,
[`platforms.yml`](https://github.com/KuchiSofts/vlc-redesign/blob/main/platforms.yml): the platform bar of the
README, the [website](https://kuchisofts.github.io/vlc-redesign/) and the *Also on* line of About are made from it.
The study behind the order, with the evidence from VideoLAN's build system:
[docs/PLATFORMS.md](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/PLATFORMS.md). Clips of each platform:
[Feature tour](Feature-Tour).

## At a glance

| Platform | Status | Latest version | Next | Needs |
|---|---|---|---|---|
| **[Windows](#windows)** | **Released** | **0.4.0** (2026-10-29) | 0.5.0 | Windows 10 (1809) or 11, 64-bit |
| [Linux](#linux) | In progress | none yet | | glibc 2.38 (Ubuntu 24.04, Debian 13, Fedora 39 or later), x86_64 or ARM64 |
| [macOS](#macos) | In progress | none yet | | macOS 12 Monterey, Apple Silicon or Intel |
| [Windows on ARM64](#windows-on-arm64) | Planned | none yet | | Windows 11 on ARM64 |
| [Android (phones, tablets, TV)](#android) | Research (an app in testing) | none yet | | Android 8.0 or later |
| [Samsung Smart TV (Tizen)](#samsung-smart-tv-tizen) | Research | none yet | | Tizen 3.0 (2017 models) or later |
| [iPhone, iPad, Apple TV](#iphone-ipad-apple-tv) | Later | none yet | | iOS, iPadOS, tvOS |
| Windows 32-bit, Windows 7, 8, XP | Not planned | | | See [below](#what-is-not-planned) |
| Platform | Status | Notes |
|---|---|---|
| **Windows 10 and 11, 64-bit (x64)** | **Available** | The installer and the portable zip, with small updates in the app: [Install and update](Install-and-Update) |
| Windows on ARM64 | Planned (next) | The x64 build already runs on Windows 11 ARM64 under emulation |
| Linux (x86_64) | 0.4.0 | .deb, .rpm, AppImage, Flatpak and a folder: [below](#linux-x86_64-from-040) |
| Samsung Smart TV (Tizen) | Planned | A receiver for casting, not VLC itself |
| macOS | Idea | |
| Android (phones, tablets, TV) | Idea | |
| iPhone, iPad, Apple TV | Idea | |
| Windows 32-bit, Windows 7, 8, XP | Not planned | See [below](#what-is-not-planned) |

Linux, macOS and Windows on ARM64 are the same desktop program as Windows, from the same code, with one version
number. Android, the TV receiver and the Apple apps are separate apps, each with its own repository and its own
version from its first published build.

## Windows

| | |
## Linux x86_64 (from 0.4.0)

One build, five downloads; each installs next to the distribution's own `vlc` and replaces nothing of it:

| Download | For | Install |
|---|---|---|
| `vlc-redesign_X.Y.Z_amd64.deb` (+ `vlc-redesign-livetv_X.Y.Z_amd64.deb`) | Ubuntu 24.04 and later, Debian 13 | `sudo apt install ./vlc-redesign_*.deb ./vlc-redesign-livetv_*.deb` |
| `vlc-redesign-X.Y.Z-1.x86_64.rpm` (+ `vlc-redesign-livetv-X.Y.Z-1.x86_64.rpm`) | Fedora 40 and later | `sudo dnf install ./vlc-redesign-*.rpm` |
| `vlc-redesign-X.Y.Z-x86_64.AppImage` | any distribution with glibc 2.38 or later (Arch, openSUSE Tumbleweed…) | `chmod +x` it and run it |
| `vlc-redesign-X.Y.Z-x86_64.flatpak` | any distribution with Flatpak | `flatpak install --user vlc-redesign-X.Y.Z-x86_64.flatpak` (the KDE 6.8 runtime comes from Flathub) |
| `vlc-redesign-X.Y.Z-linux-x86_64.tar.xz` | a folder to run from | unpack, run `vlc-redesign/vlc` |

The .deb and .rpm put VLC in `/opt/vlc-redesign` and the command `vlc-redesign`; the menu entry is *VLC Redesign*.
Older systems (Ubuntu 22.04, Debian 12: glibc 2.35 and 2.36) are refused by apt, and the AppImage and the folder
say which glibc they need instead of starting.

**Live TV is an add-on you can leave out**, as in the Windows installer:

- .deb and .rpm: it is the package `vlc-redesign-livetv`. Leave its file out at installation, or remove it later
  (`sudo apt remove vlc-redesign-livetv`, `sudo dnf remove vlc-redesign-livetv`): VLC then has no TV page, button or
  setting, and nothing of Live TV is loaded.
- AppImage, Flatpak and the folder are one file each: turn it off in *Settings › General › Live TV (IPTV) add-on*
  (or start with `--no-iptv-enabled`); off, nothing of it is loaded and the TV page and button are gone.
- Building from the source: `./configure --disable-iptv` leaves the add-on out of the build.

## What is planned, in this order

| # | Platform | How | Effort to a first build |
|---|---|---|---|
| 1 | **Windows on ARM64** | The same source, built with VideoLAN's llvm-mingw image for ARM64 (Snapdragon PCs); its own installer and update channel | 1–2 weeks |
| 2 | **Linux** (x64) | The same source; Flatpak first, then AppImage, a .deb last | 3–5 weeks |
| 3 | **macOS** (Apple Silicon and Intel) | The same source, with VLC's Qt interface turned on (VideoLAN's Mac VLC has its own native interface) | 4–6 weeks |
| 4 | **Android** phones, tablets and TV, and **ChromeOS** | A separate app: VLC for Android with its screens redrawn in the design; most Chromebooks run the Android app, others the Linux Flatpak | 10–16 weeks |
| 5 | **Samsung (Tizen) TVs**, then **LG webOS** | Not VLC itself: a TV web app that plays what the desktop casts to it, with the TV's own player | 4–7 weeks (3–5 for Tizen, 1–2 more for webOS) |
| 6 | **iPhone, iPad, Apple TV** | A separate app: VLC for iOS with its screens redrawn | 10–16 weeks |

Why this order: ARM64 is the smallest step, entirely in the current build and installer. Linux and macOS share the
same work (an HTTP backend for updates and online subtitles, an update channel of their own). Android reaches the
largest new audience. The TV receiver builds on the casting work and can go in parallel with Android, since it is
web code. The Apple apps carry the most store risk.

### What each needs

| Platform | Main blocker |
|---|---|
| **Status** | Released |
| **Version** | 0.4.0, 2026-10-29 ([release page](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.4.0)). Next: 0.5.0 |
| **Needs** | Windows 10 (version 1809, build 17763, or later) or Windows 11, 64-bit. No graphics card needed |
| **Files** | `vlc-redesign-0.4.0-win64-setup.exe` (installer) and `vlc-redesign-0.4.0-win64.zip` (portable), with small updates in the app: [Install and update](Install-and-Update) |

- **64-bit only**, Windows 10 from version 1809: the interface is built on Qt 6.8, which needs it. The installer
  refuses an older Windows, and `vlc.exe` on one says *This version of VLC media player needs Windows 10 version
  1809 or later.*
- **No graphics card needed**: it also runs in Windows Sandbox, decoding on the processor.
- **Updates are signed by the project** from 0.4.0 on: the app installs an update only when the signature of the
  release's checksums by the project's release key is good.
- **Tested:** a regression suite on every build (playback, pause and seek, Chromecast and DLNA test devices);
  NVIDIA and integrated AMD graphics, and no graphics card at all (Windows Sandbox, 2 GB); casting to a real Samsung
  TV ([TV compatibility](TV-Compatibility)).
- **Still to do:** tests on Intel graphics, on Windows 10 on real PCs and on screens of different scales;
  code-signed releases (waiting for SignPath's approval), so that SmartScreen no longer warns.
- **On Windows 11 ARM64**, the x64 installer runs and installs (it accepts any x64-compatible Windows); Windows
  emulates it, which is slower. A native build is [below](#windows-on-arm64).

The tour of every Windows feature in clips: [Feature tour: Windows](Feature-Tour#windows).

## Linux

| | |
|---|---|
| **Status** | In progress: no version published yet |
| **Needs** | glibc 2.38 or later (Ubuntu 24.04, Debian 13, Fedora 39 or later), x86_64 or ARM64 |
| **Files, once published** | `vlc-redesign-X.Y.Z-x86_64.AppImage`, `vlc-redesign_X.Y.Z_amd64.deb`, `vlc-redesign-X.Y.Z-1.x86_64.rpm`, `vlc-redesign-X.Y.Z-linux-x86_64.tar.xz`; for ARM64 `vlc-redesign-X.Y.Z-aarch64.AppImage`, `vlc-redesign_X.Y.Z_arm64.deb`, `vlc-redesign-X.Y.Z-1.aarch64.rpm` |

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/linux.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/linux.gif" alt="The Linux AppImage on Arch Linux: About with Linux x86_64 AppImage, Settings in the dark theme, the Live TV channels, Home in the light theme" width="960"></a>

<sub>The Linux AppImage on Arch Linux: About with *Linux x86_64 · AppImage*, Settings in the dark theme, the Live TV
channels, Home in the light theme. Click for the MP4.</sub>

- **The packages.** An AppImage (with update information for the GitHub releases), a .deb, an .rpm and a tarball,
  for x86_64 and ARM64, are in testing; a Flatpak manifest is written (KDE 6.8 runtime), and the Flatpak comes
  next. The .deb and .rpm install VLC in `/opt/vlc-redesign` with the command `vlc-redesign`, a desktop entry with
  VLC's media types and the logo in every size, so they install next to the distribution's `vlc`.
- **Tested:** the current code builds with Qt 6.8.3 with no warning from the redesign's code; every control of the
  player bar (59) was put in the bar, shown and clicked during playback, with no QML warning; VA-API hardware
  decoding on Intel Iris Xe (H.264, HEVC, VP9); memory stays near 480 MB while going from file to file; playback on
  Wayland (Weston) and the title bar on X11. The x86_64 packages were made and checked (the .deb, the .rpm in
  Fedora, the AppImage). A GitHub workflow builds on x86_64 and ARM64 and tests the packages on a clean runner (the
  .deb installed with apt next to the distribution's VLC, the AppImage, and the .rpm in Fedora 44, each playing a
  clip to its end); it runs once the code is on GitHub.
- **Fixed in 0.4.0 for Linux:** the zoom list, the dialogs with the System theme, the window shadow on X11 that
  caught clicks, the memory growth while going from file to file, AV1 on Ubuntu 24.04 with a recent Intel GPU (dav1d
  decodes it), and the video on Wayland with Intel or AMD graphics (now centred over black).
- **Missing:** the in-app updater on Linux; wider tests on real desktops (Ubuntu, Fedora, Debian, Arch), with X11 and
  Wayland, PipeWire and PulseAudio, Intel, AMD and NVIDIA; Ubuntu 22.04 and Debian 12 (they need a build on an older base); the Flatpak
  built and on Flathub; the pixel check of every frame.

## macOS

| | |
|---|---|
| **Status** | In progress: no version published yet |
| **Needs** | macOS 12 Monterey or later, Apple Silicon or Intel |
| **Files, once published** | `vlc-redesign-X.Y.Z-macos-<arch>.dmg`: one disk image for Apple Silicon and one for Intel |

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/macos.png"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/macos.png" alt="The macOS app on an Intel Mac, Home" width="960"></a>

<sub>The macOS app (Intel), Home, from the launch test of the build.</sub>

- **The app.** `VLC Redesign.app` is built from the same code, with VLC's Qt interface as its only interface, the
  icon drawn from the designer's logo, the Live TV add-on and the translations. Its settings, library and caches are
  apart from an official VLC's.
- **One title bar.** The red, yellow and green buttons sit at the start of VLC Redesign's own top bar, as in other
  Mac apps; full screen in its own Space, tiling and resizing from every edge work. *Show window decorations* in
  Settings brings the separate title bar back. A minimal menu bar sits at the top of the screen, and ⌘ reaches VLC's
  shortcuts.
- **Tested:** GitHub's Macs build both architectures on every pull request and run a launch test: the signatures of
  the bundle, the icon, the translations, the Live TV plugin, the About badge (*macOS Apple Silicon* / *macOS
  Intel*), the credits, playback, the Qt interface with no QML warning, a clean quit, and screenshots light, dark and
  of About. Both architectures built and passed it on 28 October.
- **Signing.** The app is signed ad hoc, not by Apple, until there is an Apple developer account: macOS asks to allow
  it once (*Open Anyway* in System Settings, Privacy & Security).
- **Missing:** signing and notarization by Apple; media keys and Now Playing; the video in the window and
  VideoToolbox checked on a Mac; in-app updates; the pixel check of every frame.

## Windows on ARM64

| | |
|---|---|
| **Status** | Planned: no version published yet |
| **Needs** | Windows 11 on ARM64 |
| **Files, once published** | `vlc-redesign-X.Y.Z-winarm64-setup.exe`, `vlc-redesign-X.Y.Z-winarm64.zip` and its own small updates (`-winarm64-delta-from-…`) |

- **Today:** the x64 build already runs on Windows 11 ARM64 under emulation (see [Windows](#windows)).
- **Built:** the redesign builds for ARM64 with VideoLAN's ARM64 toolchain and libraries, unchanged; every `.exe` and
  `.dll` of the package is ARM64. Video goes through Direct3D 11 (VideoLAN's ARM64 libraries have no OpenGL). The
  installer, the zip and the small updates are made for ARM64 too, and an ARM64 copy takes only the updates of its
  own architecture.
- **Tested:** the package is checked without running it: every binary ARM64, every imported DLL in the package or in
  Windows, the About badge, the Live TV add-on, the translations and the credits. A launch test and an install test
  run on GitHub's Windows 11 ARM64 runners at the upload.
- **Missing:** a run on a real ARM64 PC (Snapdragon), with its graphics driver.

## Android

| | |
|---|---|
| **Status** | Research. The app exists and is **in testing; it is not part of 0.4.0** and has no download yet |
| **Needs** | Android 8.0 or later |
| **What it is** | A separate app: a fork of VLC for Android with its screens redrawn in the design, in its own repository |

The clips below were recorded on the Android emulator. All of them, with captions:
[Feature tour: Android](Feature-Tour#android-in-testing-not-part-of-040).

<table>
<tr>
<td width="33%" valign="top"><a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/home.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/home.gif" alt="Android phone: Home and the library" width="240"></a><br><sub><b>Home and the library</b> on a phone.</sub></td>
<td width="33%" valign="top"><a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/tv-zapping.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/tv-zapping.gif" alt="Android phone: Live TV zapping with the banner" width="240"></a><br><sub><b>Live TV zapping</b> with the banner.</sub></td>
<td width="33%" valign="top"><a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/subtitles-hebrew.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/subtitles-hebrew.gif" alt="Android phone: tracks and subtitles in Hebrew" width="240"></a><br><sub><b>Tracks and subtitles</b> in Hebrew.</sub></td>
</tr>
</table>

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/tablet.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/tablet.gif" alt="Android tablet: the sidebar and list plus detail" width="960"></a>

<sub>The tablet layout: the sidebar and a list with its detail beside it. Click a clip for its MP4.</sub>

- **What the clips show:** Home and the library, the player bar and Customize Player, a compact context menu,
  tracks and subtitles in Hebrew, Live TV zapping and its guide, the casting card, the settings, and the tablet
  layout.
- **The same changes as the desktop where they apply:** the player's buttons and toggles show when what they open is
  open, and <kbd>Ctrl</kbd> + mouse wheel over the video, <kbd>Ctrl</kbd>+<kbd>0</kbd> and a *Subtitle size*
  slider with Reset in the subtitles tab change the subtitle size.
- **Missing before a first release:** the Android TV layout (remote control), the published APK and AAB, then
  F-Droid; Google Play only after a decision on it.

## Samsung Smart TV (Tizen)

| | |
|---|---|
| **Status** | Research |
| **Needs** | Tizen 3.0 (2017 models) or later |
| **What it would be** | The TV side of casting, drawn in the design: a web app on the TV that plays what the desktop casts to it, with the TV's own player. Not VLC itself: Samsung lets only its partners run native code on its TVs |

Casting from the desktop to a Samsung TV works today through DLNA: see [Casting to a TV](Casting) and
[TV compatibility](TV-Compatibility).

## iPhone, iPad, Apple TV

| | |
|---|---|
| **Status** | Later, after Android |
| **Needs** | iOS, iPadOS, tvOS |
| **What it would be** | A separate app: a fork of VLC for iOS with its screens redrawn, reusing the Android work |

## What is not planned

- **Windows 32-bit**, **Windows 7 and 8**, and older versions such as **Windows XP**. Qt 6, which draws the interface,
  supports only 64-bit Windows 10 and 11, and VLC Redesign relies on Windows features that Windows 7 does not have
  (DirectComposition, the acrylic blur, the rounded window corners). 32-bit Windows exists only as Windows 10, whose
  support ended on 2025-10-14, and Windows 11 is 64-bit only. Every extra Windows build would also double the
  release files, the small updates and the signing. On Windows 7, 8 and 32-bit Windows, VideoLAN's VLC 3 remains the
  player to use.

> **Good to know.** Interested in a platform? Say so in
> [Discussions](https://github.com/KuchiSofts/vlc-redesign/discussions) (the **Ideas** category): it helps decide
> what comes first.

---

[Feature tour](Feature-Tour) · [Install and update](Install-and-Update) · [Building from source](Building-from-Source) · [Troubleshooting and FAQ](Troubleshooting-and-FAQ) · [Home](Home)
