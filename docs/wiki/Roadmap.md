# Roadmap

Where **VLC Redesign** is going, program by program. This page follows the
[roadmap](https://github.com/KuchiSofts/vlc-redesign/blob/main/ROADMAP.md) of the repository, which is updated with
every upload; the full plan, frame by frame, is the
[master plan](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/MASTER-PLAN.md), and every change is in the
[changelog](https://github.com/KuchiSofts/vlc-redesign/blob/main/CHANGELOG.md).

Status: ✅ done · 🔨 in progress · 📋 to do · 💡 idea

## Where each program stands

Updated as the work moves (last: 2026-10-30).

| Program | Status | Progress | What is left |
|---|---|---|---|
| **Windows x64** | ✅ released 0.4.1 | ██████████ 100 % | code signing (waiting for SignPath's approval) |
| **Windows on ARM64** | ✅ released 0.4.1 | ██████████ 100 % | a run by hand on a real ARM64 PC; small updates in the app from 0.5.0 |
| **Linux** (x86_64, ARM64) | ✅ released 0.4.1 | ██████████ 100 % | the Flatpak; an updater in the app; tests on real desktops |
| **macOS** (Apple Silicon, Intel) | ✅ released 0.4.1 | █████████░ 92 % | signing by Apple (a developer account); tests on a Mac by hand |
| **Android** (phones) | 🔨 in testing | █████████░ 97 % | the last fixes (below), then a test on real phones and a first release |
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
- 🔨 Three screens kept in memory after they close (a leak to remove)
- 🔨 The light theme, the borders of the dialogs and the icons as sharp as on the desktop (parity with Windows)
- 🔨 The texts in the 9 languages that the desktop has and Android not yet
- 📋 A test on real phones and a tablet, then the first release (its own version numbers)

**0.5.0** (desktop) gathers the new features made after 0.4.x: Windows on ARM64 gets the small updates in the app,
the Linux Flatpak, and what the issues and the Android work bring back to the desktop.

An idea? Open a **[suggestion](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=feature_request.yml)**;
issues with a 👍 from many people move up. Each platform in detail: [Platforms](Platforms).
