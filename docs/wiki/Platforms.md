# Platforms

VLC Redesign **0.4.1** is released for four platforms together: **Windows 10 (version 1809 or later) and Windows 11,
64-bit** (x64), **Windows 11 on ARM64** (new in 0.4.1), **Linux** (x86_64 and ARM64) and **macOS** (Apple Silicon
and Intel). The Android app is in testing as a separate app. This page gives, for each platform, its status, its version, what it needs, its files, what is tested and what is
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
| **[Windows](#windows)** | **Released** | **0.4.1** (2026-10-30) | 0.5.0 | Windows 10 (1809) or 11, 64-bit |
| **[Linux](#linux)** | **Released** | **0.4.1** (2026-10-30) | 0.5.0 | glibc 2.38 (Ubuntu 24.04, Debian 13, Fedora 39 or later), x86_64 or ARM64 |
| **[macOS](#macos)** | **Released** | **0.4.1** (2026-10-30) | 0.5.0 | macOS 12 Monterey, Apple Silicon or Intel |
| **[Windows on ARM64](#windows-on-arm64)** | **Released** | **0.4.1** (2026-10-30) | 0.5.0 | Windows 11 on ARM64 |
| [Android (phones, tablets, TV)](#android) | Research (an app in testing) | none yet | | Android 8.0 or later |
| [Samsung Smart TV (Tizen)](#samsung-smart-tv-tizen) | Research | none yet | | Tizen 3.0 (2017 models) or later |
| [iPhone, iPad, Apple TV](#iphone-ipad-apple-tv) | Later | none yet | | iOS, iPadOS, tvOS |
| Windows 32-bit, Windows 7, 8, XP | Not planned | | | See [below](#what-is-not-planned) |

Linux, macOS and Windows on ARM64 are the same desktop program as Windows, from the same code, with one version
number: 0.4.0 is the first version for Linux and macOS, 0.4.1 the first for Windows on ARM64. Android, the TV receiver and the Apple apps are separate apps, each with its own repository and its own
version from its first published build.

## Windows

| | |
|---|---|
| **Status** | Released |
| **Version** | 0.4.1, 2026-10-30 ([release page](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.4.1)). Next: 0.5.0 |
| **Needs** | Windows 10 (version 1809, build 17763, or later) or Windows 11, 64-bit. No graphics card needed |
| **Files** | `vlc-redesign-0.4.1-win64-setup.exe` (installer) and `vlc-redesign-0.4.1-win64.zip` (portable), with small updates in the app: [Install and update](Install-and-Update) |

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
- **Small updates in the app and the signed `SHA256SUMS.txt` are for Windows only.**
- **On Windows 11 ARM64**, take the native ARM64 build: [below](#windows-on-arm64). The x64 installer runs there
  too (it accepts any x64-compatible Windows), but Windows emulates it, which is slower.

The tour of every Windows feature in clips: [Feature tour: Windows](Feature-Tour#windows).

## Linux

| | |
|---|---|
| **Status** | Released |
| **Version** | 0.4.1, 2026-10-30 ([release page](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.4.1)); 0.4.0 was the first for Linux. Next: 0.5.0 |
| **Needs** | glibc 2.38 or later (Ubuntu 24.04, Debian 13, Fedora 39 or later), x86_64 or ARM64 |
| **Files** | x86_64: `vlc-redesign_0.4.1_amd64.deb` (155 MB), `vlc-redesign-0.4.1-1.x86_64.rpm` (156 MB), `vlc-redesign-0.4.1-x86_64.AppImage` (166 MB, with its `.zsync`), `vlc-redesign-0.4.1-linux-x86_64.tar.xz` (126 MB), the Live TV add-on `vlc-redesign-livetv_0.4.1_amd64.deb` and `vlc-redesign-livetv-0.4.1-1.x86_64.rpm`, `SHA256SUMS-linux-x86_64.txt`. ARM64, about the same size: `vlc-redesign_0.4.1_arm64.deb`, `vlc-redesign-0.4.1-1.aarch64.rpm`, `vlc-redesign-0.4.1-aarch64.AppImage`, `vlc-redesign-0.4.1-linux-aarch64.tar.xz`, `SHA256SUMS-linux-aarch64.txt`. Which one and how: [Install and update: Linux](Install-and-Update#linux) |

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/linux.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/linux.gif" alt="The Linux AppImage on Arch Linux: About with Linux x86_64 AppImage, Settings in the dark theme, Home in the light theme" width="960"></a>

<sub>The Linux AppImage on Arch Linux: About with *Linux x86_64 · AppImage*, Settings in the dark theme,
Home in the light theme. Click for the MP4.</sub>

- **The packages.** A .deb (Ubuntu 24.04 and later, Debian 13), an .rpm (Fedora), an AppImage (any distribution
  with glibc 2.38 or later, Arch among them) and a folder to run from, for x86_64 and ARM64. The .deb and .rpm
  install VLC in `/opt/vlc-redesign` with the command `vlc-redesign`, a desktop entry *VLC Redesign* with VLC's media
  types and the logo in every size, so they install next to the distribution's `vlc` and replace nothing of it.
  Older systems (Ubuntu 22.04, Debian 12: glibc 2.35 and 2.36) are refused by apt, and the AppImage and the folder
  say which glibc they need instead of starting. No Flatpak in this release: it comes next.
- **Live TV is an add-on you can leave out**, as in the Windows installer: with the .deb and .rpm it is the package
  `vlc-redesign-livetv` (leave its file out, or remove it later: `sudo apt remove vlc-redesign-livetv`,
  `sudo dnf remove vlc-redesign-livetv`); the AppImage and the folder carry it, and *Settings › General › Live TV
  (IPTV) add-on* turns it off. Building from the source: `./configure --disable-iptv` leaves it out.
- **Updates:** no update in the app on Linux; install the new version's package (apt and dnf replace the older
  one), or take the new AppImage or folder.
- **Tested:** in clean containers on 7 distributions (`linux-pkg-test.sh`: install, files and libraries, a UI tour
  under Xvfb, removal): Ubuntu 24.04 and 26.04, Debian 13, Fedora 41 and 43, Arch with the AppImage; apt refuses the
  .deb on Ubuntu 22.04 and Debian 12, as it should. GitHub's x86_64 and ARM64 runners build the packages and test
  them on a clean runner: the .deb installed with apt next to the system's VLC, the AppImage, the .rpm in Fedora,
  each playing a clip. Before that: every control of the player bar (59) put in the bar, shown and clicked during
  playback with no QML warning; VA-API hardware decoding on Intel Iris Xe (H.264, HEVC, VP9); memory near 480 MB
  while going from file to file; playback on Wayland (Weston) and the title bar on X11.
- **Fixed in 0.4.0 for Linux:** the zoom list, the dialogs with the System theme, the window shadow on X11 that
  caught clicks, the memory growth while going from file to file, AV1 on Ubuntu 24.04 with a recent Intel GPU (dav1d
  decodes it), the video on Wayland with Intel or AMD graphics (now centred over black), and Weston 14 and 15 hanging
  VLC at the end of every video.
- **Missing:** the Flatpak (built and on Flathub); an updater in the app; tests by hand on real desktops (Ubuntu,
  Fedora, Debian, Arch), with X11 and Wayland, PipeWire and PulseAudio, Intel, AMD and NVIDIA; Ubuntu 22.04 and
  Debian 12 (they need a build on an older base); the pixel check of every frame.

## macOS

| | |
|---|---|
| **Status** | Released |
| **Version** | 0.4.1, 2026-10-30 ([release page](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.4.1)); 0.4.0 was the first for macOS. Next: 0.5.0 |
| **Needs** | macOS 12 Monterey or later, Apple Silicon or Intel |
| **Files** | `vlc-redesign-0.4.1-macos-arm64.dmg` (Apple Silicon) and `vlc-redesign-0.4.1-macos-x86_64.dmg` (Intel). How to open it the first time: [Install and update: macOS](Install-and-Update#macos) |

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/macos.png"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/macos.png" alt="The macOS app on an Intel Mac, Home" width="960"></a>

<sub>The macOS app (Intel), Home, from the launch test of the build.</sub>

- **The app.** `VLC Redesign.app` is built from the same code, with VLC's Qt interface as its only interface, the
  icon drawn from the designer's logo, the Live TV add-on and the translations. Its settings, library and caches are
  apart from an official VLC's. About shows *macOS Apple Silicon* or *macOS Intel*.
- **One title bar.** The red, yellow and green buttons sit at the start of VLC Redesign's own top bar, as in other
  Mac apps; full screen in its own Space, tiling and resizing from every edge work. *Show window decorations* in
  Settings brings the separate title bar back. A minimal menu bar sits at the top of the screen, and ⌘ reaches VLC's
  shortcuts.
- **Signing.** The app is signed ad hoc, not by Apple, until there is an Apple developer account: the first time,
  macOS refuses to open it, and you allow it once (*System Settings › Privacy & Security › Open Anyway*, or
  right-click › *Open* on macOS 12 to 14).
- **Updates:** no update in the app on macOS; download the new disk image and replace the app.
- **Tested:** GitHub's Macs build both architectures and run a launch test on each: every binary signed, the icon,
  100 translations, the About badge (*macOS Apple Silicon* / *macOS Intel*), the designer's credit, the Live TV
  add-on loaded, headless playback to the end, the Qt interface for 10 seconds playing a clip, one unified title bar
  (the traffic lights), a clean quit, no QML warning and no crash report. It has not been run by hand on a Mac yet.
- **Missing:** signing and notarization by Apple; tests by hand on a Mac; media keys and Now Playing; the video in
  the window and VideoToolbox checked on a Mac; an updater in the app; the pixel check of every frame.

## Windows on ARM64

| | |
|---|---|
| **Status** | Released |
| **Version** | 0.4.1, 2026-10-30 ([release page](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.4.1)), the first for Windows on ARM64. Next: 0.5.0 |
| **Needs** | Windows 11 on ARM64 (Snapdragon PCs) |
| **Files** | `vlc-redesign-0.4.1-winarm64-setup.exe` (installer) and `vlc-redesign-0.4.1-winarm64.zip` (portable): [Install and update: Windows on ARM64](Install-and-Update#windows-on-arm64) |

- **Native:** the redesign builds for ARM64 with VideoLAN's ARM64 toolchain and libraries, unchanged; it is not the
  x64 build under emulation. Video goes through Direct3D 11 (VideoLAN's ARM64 libraries have no OpenGL).
- **Updates:** 0.4.1 is the first ARM64 release, so it has no small update from an earlier version. The next
  versions offer small updates made for ARM64, and an ARM64 copy takes only the updates of its own architecture.
- **Fixed for 0.4.1:** the ARM64 build stopped a few seconds after it started (the table of `longjmp` targets of
  Control Flow Guard, see the [changelog](https://github.com/KuchiSofts/vlc-redesign/blob/main/CHANGELOG.md)), and
  the install test counted Inno Setup's uninstaller, which is always x86, as a binary of the package.
- **Tested:** on GitHub's real ARM64 Windows runner: every binary of the package is ARM64, the plugin cache builds,
  the window opens, a clip plays to its end and VLC quits cleanly; the installer installs, starts, plays and
  uninstalls.
- **Missing:** a run by hand on a real ARM64 PC (Snapdragon), with its graphics driver; code-signed releases
  (waiting for SignPath's approval), so that SmartScreen no longer warns.

## Android

| | |
|---|---|
| **Status** | Research. The app exists and is **in testing; it is not part of 0.4.1** and has no download yet |
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
