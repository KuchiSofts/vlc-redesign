# Platforms

VLC Redesign runs on **Windows 10 (version 1809 or later) and Windows 11, 64-bit** (x64). That is the only platform
with a build today. The other platforms have been studied: which ones are possible, how, at what cost, and in which
order. Nothing below is started, and the order is a plan, not a promise.

The full study, with the evidence from VideoLAN's build system and this project's code:
[docs/PLATFORMS.md](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/PLATFORMS.md). The status of each
platform is kept in [`platforms.yml`](https://github.com/KuchiSofts/vlc-redesign/blob/main/platforms.yml).

## At a glance

| Platform | Status | Notes |
|---|---|---|
| **Windows 10 and 11, 64-bit (x64)** | **Available** | The installer and the portable zip, with small updates in the app: [Install and update](Install-and-Update) |
| Windows on ARM64 | Planned (next) | The x64 build already runs on Windows 11 ARM64 under emulation |
| Linux (x64) | Planned | Builds from the same source; the first milestones were checked on Linux, the latest work only on Windows |
| Samsung Smart TV (Tizen) | Planned | A receiver for casting, not VLC itself |
| macOS | Idea | |
| Android (phones, tablets, TV) | Idea | |
| iPhone, iPad, Apple TV | Idea | |
| Windows 32-bit, Windows 7, 8, XP | Not planned | See [below](#what-is-not-planned) |

## Windows 10 and 11 today

- **64-bit only**, Windows 10 from version 1809 (build 17763): the interface is built on Qt 6.8, which needs it. The
  installer refuses an older Windows, and `vlc.exe` on one says *This version of VLC media player needs Windows 10
  version 1809 or later.*
- **No graphics card needed**: it also runs in Windows Sandbox, decoding on the processor.
- **On Windows 11 ARM64**, the x64 installer runs and installs (the installer accepts any x64-compatible Windows);
  Windows emulates it, which is slower. A native ARM64 build is the next step below.

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
| Windows on ARM64 | Qualcomm graphics drivers with the interface's DirectComposition path, untested; an ARM64 test PC |
| Linux | The Windows-only parts of this project (HTTP, updater, firewall, menus); Qt 6.8 on older distributions |
| macOS | Qt is off on macOS upstream and untested there; an Apple Developer account for signing and notarization |
| Android | A new interface in Kotlin; the phone and TV frames do not exist in the design; Google's developer verification |
| Samsung and LG TVs | No native code for third parties on either: the TV decodes, so the formats are the TV's |
| iPhone, iPad | GPL code cannot go to the App Store; Apple's review; Qt has no tvOS |

### Versions and repositories

Steps 1 to 3 are the same desktop program, in this repository, with one version number. Each separate app (Android,
iOS, the TV receiver) would get its own repository, following its own VideoLAN upstream, and its own version from its
first published build. Issues stay in this repository, with a label per platform.

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

[Install and update](Install-and-Update) · [Building from source](Building-from-Source) · [Troubleshooting and FAQ](Troubleshooting-and-FAQ) · [Home](Home)
