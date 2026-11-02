# Roadmap

Where **VLC Redesign** is going, program by program. This page follows the
[roadmap](https://github.com/KuchiSofts/vlc-redesign/blob/main/ROADMAP.md) of the repository, which is updated with
every upload; the full plan, frame by frame, is the
[master plan](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/MASTER-PLAN.md), and every change is in the
[changelog](https://github.com/KuchiSofts/vlc-redesign/blob/main/CHANGELOG.md).

Status: ✅ done · 🔨 in progress · 📋 to do · 💡 idea

## Where each program stands

Updated as the work moves (last: 2026-11-02).

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

- 🔨 Android: the TV layout (a remote control): the palette, the font and the focus are done; the player and the
  pages under the home screen are next

After it:

- 📋 Linux: the Flatpak on Flathub
- 📋 macOS and Windows on ARM64: a round by hand on a real Mac and a real ARM64 PC (both are tested on GitHub's
  machines at every build)
- 💡 The Samsung TV receiver (the TV side of casting, drawn in the design), then iPhone and iPad
