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
| **Windows x64** | ✅ released 0.4.1 | ██████████ 100 % | — (tests go on with every change) |
| **Windows on ARM64** | ✅ released 0.4.1 | ██████████ 100 % | a run by hand on a real ARM64 PC; small updates in the app from 0.5.0 |
| **Linux** (x86_64, ARM64) | ✅ released 0.4.1 | ██████████ 100 % | the Flatpak (built and tested in CI, in 0.5.0); choosing the sound output device; an updater in the app |
| **macOS** (Apple Silicon, Intel) | ✅ released 0.4.1 | ██████████ 100 % | tests on a Mac by hand; choosing the sound output device |
| **Android** (phones) | 🔨 in testing | █████████░ 98 % | the last fixes (below), then a test on real phones and a first release |
| **Android** (tablets) | 🔨 in testing | █████████░ 93 % | the automatic tour of the tablets (29 of 42 cases done, no crash), then a test on a real tablet |
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
- 📋 A test on real phones and a tablet, then the first release: an APK on GitHub Releases (not on Google Play)
- 📋 The designer's ideas the desktop has and the app does not yet: the subtitle sync timeline, subtitle settings
  kept per series, the waveform seek bar for music, frame previews on the seek bar, saved Customize Player profiles,
  folders and playlists pinned to the sidebar

**0.5.0** (desktop) gathers the new features made after 0.4.x: Windows on ARM64 gets the small updates in the app,
the Linux Flatpak, and what the issues and the Android work bring back to the desktop. From Juxtopposed's video, still
to come on the desktop: pinning a folder or a playlist to the sidebar, online subtitles for a whole folder or
playlist, choosing the sound output device on Linux and macOS, and the on-screen message naming the shortcut used.

Fixed since 0.4.1, through a pull request (no need to wait for 0.5.0): the startup volume goes up to 200 %
([#30](https://github.com/KuchiSofts/vlc-redesign/issues/30)).
