# Roadmap

Where **VLC Redesign** is going, program by program. This page follows the
[roadmap](https://github.com/KuchiSofts/vlc-redesign/blob/main/ROADMAP.md) of the repository, which is updated with
every upload; the full plan, frame by frame, is the
[master plan](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/MASTER-PLAN.md), and every change is in the
[changelog](https://github.com/KuchiSofts/vlc-redesign/blob/main/CHANGELOG.md).

Status: ✅ done · 🔨 in progress · 📋 to do · 💡 idea

## Where each program stands

Updated as the work moves (last: 2026-11-08, 0.5.8 published).

| Program | Status | Progress | What is left |
|---|---|---|---|
| **Windows x64** | ✅ released 0.5.8 | ██████████ 100 % | — fixes as the issues bring them |
| **Windows on ARM64** | ✅ released 0.5.8 | ██████████ 100 % | a run by hand on a real ARM64 PC |
| **Linux** (x86_64, ARM64) | ✅ released 0.5.8 | ██████████ 100 % | the Flatpak on Flathub |
| **macOS** (Apple Silicon, Intel) | ✅ released 0.5.8 | ██████████ 100 % | tests on a Mac by hand |
| **Android** (phones) | ✅ released 0.5.8 | ██████████ 100 % | — tested on a real phone (Galaxy class) and emulators |
| **Android** (tablets) | ✅ released 0.5.8 | ██████████ 100 % | — tested on a real tablet (Galaxy Tab class) and emulators |
| **Android TV** | ✅ released 0.5.8 | ██████████ 100 % | — 1:1 with the Figma frames, lighter (no work in the background); tests on more TV boxes |
| **Samsung TV** (*Juxto Media Player*) | ✅ released 0.5.8 | ██████████ 100 % | — tested on a 2017 Samsung TV; the Samsung store (later) |
| **iPhone, iPad** | 💡 later | ░░░░░░░░░░ 0 % | after Android |

## Next: 0.6.0 and after

**0.6.0** is a full release of every app, with every package and installer: the round of stability, smarter
automatic subtitles, and the same Subtitles panel on every device, made since 0.5.8:

- ✅ In full screen, keys and the remote seek, change the volume or pause without showing the controls, if you
  choose (a slim progress line and the time, or a short message instead), as in VLC 3, in every app
  ([#72](https://github.com/KuchiSofts/vlc-redesign/issues/72), @BarbodF): desktop, Android (phones, tablets, TV)
  and the Samsung TV app
- ✅ Interface size (on request): Settings ›
  Interface › Window › *Interface size*: Automatic, 100 to 200 %, applied at once; every window, dialog, menu and
  popup opens inside its screen and scrolls when it is taller than the window; rows and grids of cards fill the
  page's width with whole cards at every window size; empty pages show the page's icon
- ✅ Stability (on request), in every app: every line of the newest code reviewed, the parsers fuzzed, quitting and Live TV bounded,
  stalled servers stopped, no secret header sent to another server on a redirect, the interface's thread free of
  network and disk work, no QML warning left; the desktop starts faster (window 0.60 → 0.48 s), memory soaked
- ✅ Automatic subtitles, in every app: a film opened as the app starts
  gets them too; a site's advert or the forced lines beside the video no longer stop the search; the video's own
  Subs folder is used; Hebrew and Arabic punctuation written for left-to-right players shows in its place
- ✅ Online subtitles for videos played from the network (a link, SMB, NFS, UPnP/DLNA) on Android, manual and
  automatic, as on the desktop ([#73](https://github.com/KuchiSofts/vlc-redesign/issues/73), @Elshepl); the
  Samsung TV app too
- ✅ The Subtitles panel the same on every device: the desktop's two columns (the languages and their subtitles),
  the search in it and the timing; on the Samsung TV app with the remote; on Android (phones, tablets, TV) opened
  above its button as on the desktop
- 🔨 Android the same as the desktop: its options and defaults, two subtitle languages at once, smoother first
  visits of each page
- ✅ Android installs next to VLC for Android (it was refused before), and Android TV in Hebrew and Arabic is
  mirrored everywhere, its focus where it should be
- 🟡 The release: the three apps together, built and tested on every platform
- 📋 Live TV: the servers tried on real ones (Tvheadend, NextPVR, Channels DVR, an HDHomeRun tuner) and a paid
  Schedules Direct account; on Android and the Samsung TV app a Tvheadend server that takes only Digest sign-in
  plays with its access code (their players send Basic)
- 📋 The subtitle services with keys and accounts tried with real keys (SubSource, SubDL, OpenSubtitles.com and the
  seven others)
- 📋 Linux: the first update of the AppImage from the app, checked from one version to the next on real desktops
- 📋 Samsung TV: a USB drive played on a real TV (the folders of the TV's own storage are checked)
- 📋 Android TV: tests on more TV boxes (Google TV, Fire TV)

After it:

- 📋 Linux: the Flatpak on Flathub
- 📋 macOS and Windows on ARM64: a round by hand on a real Mac and a real ARM64 PC (both are tested on GitHub's
  machines at every build)
- 📋 Windows: tests on more real PCs (Windows 10, Intel graphics, screens of different scales)
- 📋 Code-signed Windows releases (no SmartScreen warning): waiting for SignPath's approval
- 💡 The Samsung TV app in Samsung's store; the TV side of casting (drawn in the design); then iPhone and iPad

