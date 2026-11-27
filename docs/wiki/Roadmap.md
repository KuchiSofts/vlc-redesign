# Roadmap

Where **VLC Redesign** is going, program by program. This page follows the
[roadmap](https://github.com/KuchiSofts/vlc-redesign/blob/main/ROADMAP.md) of the repository, which is updated with
every upload; the full plan, frame by frame, is the
[master plan](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/MASTER-PLAN.md), and every change is in the
[changelog](https://github.com/KuchiSofts/vlc-redesign/blob/main/CHANGELOG.md).

Status: ✅ done · 🔨 in progress · 📋 to do · 💡 idea

## Where each program stands

Updated as the work moves (last: 2026-11-27, 0.8.9 published, 0.9.0 started).

| Program | Status | Progress | What is left |
|---|---|---|---|
| **Windows x64** | ✅ released 0.8.9 | ██████████ 100 % | — fixes as the issues bring them |
| **Windows on ARM64** | ✅ released 0.8.9 | ██████████ 100 % | a run by hand on a real ARM64 PC |
| **Linux** (x86_64, ARM64) | ✅ released 0.8.9 | ██████████ 100 % | the Flatpak on Flathub |
| **macOS** (Apple Silicon, Intel) | ✅ released 0.8.9 | ██████████ 100 % | tests on a Mac by hand |
| **Android** (phones) | ✅ released 0.8.8 | ██████████ 100 % | — tested on a real phone (Galaxy class) and emulators |
| **Android** (tablets) | ✅ released 0.8.8 | ██████████ 100 % | — tested on a real tablet (Galaxy Tab class) and emulators |
| **Android TV** | ✅ released 0.8.8 | ██████████ 100 % | — 1:1 with the Figma frames, lighter (no work in the background); tests on more TV boxes |
| **Samsung TV** (*Juxto Media Player*) | ✅ released 0.8.6 | ██████████ 100 % | — tested on a 2017 Samsung TV; the Samsung store (later) |
| **iPhone, iPad** | 💡 later | ░░░░░░░░░░ 0 % | after Android |

## Next: 0.9.0 and after

**0.9.0** is an update release of fixes, made as the issues bring them, and of the next contributions,
each reviewed before it is taken.

- 🟡 Fixes from a review of Live TV, Android and the Samsung TV app: a provider's playlist can no longer write
  files or load images from the web, channel switching no longer waits for slow logos, a crash on Android 5
  and 6, the Samsung app's memory with long songs and its queue

And as the issues bring them:

- 📋 Hardening still to do, from the reviews of 11-27: the Windows update helper resumes or rolls back after a
  power cut in the middle of an update; subtitle service passwords in the system's keystore on macOS and Linux;
  a long pause of a cast to a Samsung TV (over an hour) keeps the cast
- 📋 Windows: the media controls (#210) on Windows 10 1809, and the contrast themes (#261, #212) with a real
  contrast theme turned on (both were checked on Windows 11 with the theme off)
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
- 📋 Code-signed Windows releases (no SmartScreen warning): SignPath Foundation accepted the project (11-20); the
  test signing works on x64 and ARM64; SignPath reviews the setup (11-20), then the release certificate
- 💡 The Samsung TV app in Samsung's store; the TV side of casting (drawn in the design); then iPhone and iPad
