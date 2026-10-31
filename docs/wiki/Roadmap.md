# Roadmap

Where **VLC Redesign** is going, program by program. This page follows the
[roadmap](https://github.com/KuchiSofts/vlc-redesign/blob/main/ROADMAP.md) of the repository, which is updated with
every upload; the full plan, frame by frame, is the
[master plan](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/MASTER-PLAN.md), and every change is in the
[changelog](https://github.com/KuchiSofts/vlc-redesign/blob/main/CHANGELOG.md).

Status: ✅ done · 🔨 in progress · 📋 to do · 💡 idea

## Where each program stands

Updated as the work moves (last: 2026-10-31).

| Program | Status | Progress | What is left |
|---|---|---|---|
| **Windows x64** | ✅ released 0.4.1 | ██████████ 100 % | — 0.5.0 is ready in testing (below) |
| **Windows on ARM64** | ✅ released 0.4.1 | ██████████ 100 % | small updates in the app from 0.5.0; a run by hand on a real ARM64 PC |
| **Linux** (x86_64, ARM64) | ✅ released 0.4.1 | ██████████ 100 % | 0.5.0: the Flatpak, the sound output device, the fixes below; an updater in the app later |
| **macOS** (Apple Silicon, Intel) | ✅ released 0.4.1 | ██████████ 100 % | 0.5.0: the sound output device; tests on a Mac by hand |
| **Android** (phones) | 🔨 in testing | █████████░ 99 % | a test on a real phone, then the first release on GitHub |
| **Android** (tablets) | 🔨 in testing | █████████░ 98 % | a test on a real tablet |
| **Samsung TV receiver** | 💡 researched | ░░░░░░░░░░ 0 % | after the Android release |
| **iPhone, iPad** | 💡 later | ░░░░░░░░░░ 0 % | after Android |

## Next: 0.5.0 and the Android app

**Every idea of Juxtopposed's video now exists**, on the desktop and on Android:

- ✅ Pin a folder or a playlist to the sidebar (*Pin to Sidebar* in their menus, *Unpin* on the row; on Android in
  the tablet sidebar and at the top of Home, moved and hidden in *Edit Sidebar*)
- ✅ Subtitles from the internet for a whole folder, season or playlist, each episode matched on its own
- ✅ The on-screen messages name the shortcut that made them ("Volume: 115%" · "Up")
- ✅ Choose the sound output device on Linux and macOS too (*Settings › Audio › Output device*)
- ✅ Android: the subtitle sync timeline, subtitle settings kept per series, frame previews on the seek bar, the
  waveform seek bar for music kept between starts, Customize Player profiles, the last icons as the desktop's

**0.5.0** also brings:

- ✅ The app's name is **VLC Media Player Redesign**, as VideoLAN's trademark policy allows a free (GPL) build
- ✅ Linux: the programs VLC opens (browser, file manager, D-Bus for the media keys and the screen saver) work on
  newer systems; fewer bundled system libraries; the AppImage and the folder play videos on a system without VA-API;
  tested on Ubuntu 24.04/26.04, Debian 13, Fedora 41/43, Arch and as a Flatpak
- ✅ Windows: the firewall rule for casting runs Windows' own netsh by its full path; quitting no longer waits for
  Windows' question about it; stricter checks of updates
- ✅ The build and release pipeline hardened (actions pinned to commits, downloads checked against their hashes),
  new automatic tests of the parsers and of the updates' signature

**The Android app** (a fork of VLC for Android with the design) has the design's Home, library, player bar and
Customize Player, the compact menus, casting like the desktop, effects, recording, subtitles in every script and right
to left, Live TV, the tablet layout and the designer's credit. An automatic tour of every screen (phone and tablets,
dark and light, English, Hebrew and German, large text) finds no crash. Left before its first release:

- ✅ Its own package name, `org.kuchisofts.vlcredesign`: it installs beside VLC
- ✅ Every text of the redesign translated in every language the app speaks
- 📋 A test on a real phone and a tablet, then the first release: an APK on GitHub Releases (not on Google Play)
