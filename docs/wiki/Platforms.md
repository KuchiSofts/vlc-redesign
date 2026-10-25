# Platforms

VLC Redesign runs on **Windows 10 (version 1809 or later) and Windows 11, 64-bit** (x64). That is the only platform
with a build today. Other platforms have been studied: which ones are possible, how, and in which order. Nothing below
is started, and the order is a plan, not a promise.

The full study: [docs/PLATFORMS.md](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/PLATFORMS.md).

## What exists

| Platform | Status |
|---|---|
| Windows 10 and 11, 64-bit (x64) | **Available**: the installer and the portable zip, see [Install and update](Install-and-Update). |

## What is planned, in this order

| # | Platform | How | Effort to a first build |
|---|---|---|---|
| 1 | **Windows on ARM64** | The same source, built for ARM64 (Snapdragon PCs) | 1–2 weeks |
| 2 | **Linux** (x64) | The same source; Flatpak first, then AppImage, a .deb last | 3–5 weeks |
| 3 | **macOS** (Apple Silicon and Intel) | The same source, with VLC's Qt interface turned on (VideoLAN's Mac VLC has its own native one) | 4–6 weeks |
| 4 | **Android** phones, tablets and TV, and **ChromeOS** | A separate app: VLC for Android with its screens redrawn in the design; most Chromebooks run the Android app | 10–16 weeks |
| 5 | **Samsung (Tizen) TVs**, then **LG webOS** | Not VLC itself: a small TV app that plays what the desktop casts to it | 4–7 weeks |
| 6 | **iPhone, iPad, Apple TV** | A separate app: VLC for iOS with its screens redrawn | 10–16 weeks |

Steps 1 to 3 are the same desktop program, with one version number. Each separate app would get its own version from
its first published build.

## What is not planned

- **Windows 32-bit**, **Windows 7 and 8**, and older versions such as **Windows XP**. Qt 6, which draws the interface,
  supports only 64-bit Windows 10 and 11, and VLC Redesign relies on Windows features that Windows 7 does not have
  (DirectComposition, the acrylic blur, the rounded window corners). 32-bit Windows exists only as Windows 10, whose
  support ended on 2025-10-14, and Windows 11 is 64-bit only. On Windows 7, 8 and 32-bit Windows, VideoLAN's VLC 3
  remains the player to use.

---

[Install and update](Install-and-Update) · [Building from source](Building-from-Source) · [Home](Home)
