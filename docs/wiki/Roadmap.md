# Roadmap

Where **VLC Redesign** is going, program by program. This page follows the
[roadmap](https://github.com/KuchiSofts/vlc-redesign/blob/main/ROADMAP.md) of the repository, which is updated with
every upload; the full plan, frame by frame, is the
[master plan](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/MASTER-PLAN.md), and every change is in the
[changelog](https://github.com/KuchiSofts/vlc-redesign/blob/main/CHANGELOG.md).

Status: ✅ done · 🔨 in progress · 📋 to do · 💡 idea

## Where each program stands

Updated as the work moves (last: 2026-11-01).

| Program | Status | Progress | What is left |
|---|---|---|---|
| **Windows x64** | ✅ released 0.5.1 | ██████████ 100 % | — 0.5.2: fixes as the issues bring them |
| **Windows on ARM64** | ✅ released 0.5.1 | ██████████ 100 % | a run by hand on a real ARM64 PC |
| **Linux** (x86_64, ARM64) | ✅ released 0.5.1 | ██████████ 100 % | the Flatpak on Flathub; an updater in the app later |
| **macOS** (Apple Silicon, Intel) | ✅ released 0.5.1 | ██████████ 100 % | tests on a Mac by hand |
| **Android** (phones) | ✅ released 0.5.1 | ██████████ 100 % | — tested on a real phone (Galaxy class) and emulators |
| **Android** (tablets) | ✅ released 0.5.1 | █████████░ 98 % | a test on a real tablet (done on 11" and 13" emulators) |
| **Samsung TV receiver** | 💡 researched | ░░░░░░░░░░ 0 % | after 0.5.2 |
| **iPhone, iPad** | 💡 later | ░░░░░░░░░░ 0 % | after Android |

## Next: 0.5.2 and after

**0.5.2** is a release of fixes, made as the issues bring them (each fix is first published through a pull request,
with a build to try), and the last checks of the design on Android:

- 📋 Android, on a real phone: the long press and a mouse in the context menu, recording into the shared Movies
  folder, the subtitle fonts of Samsung's list, the artist banner with real artwork

After it:

- 📋 Android: the TV layout (a remote control), and a test on a real tablet (the tablet layout was tested on 11" and
  13" emulators)
- 📋 Linux: the Flatpak on Flathub, and an updater in the app as on Windows
- 📋 macOS and Windows on ARM64: a round by hand on a real Mac and a real ARM64 PC (both are tested on GitHub's
  machines at every build)
- 💡 The Samsung TV receiver (the TV side of casting, drawn in the design), then iPhone and iPad

What each release brought: the [roadmap](https://github.com/KuchiSofts/vlc-redesign/blob/main/ROADMAP.md#051-published-2026-11-01)
and the [changelog](https://github.com/KuchiSofts/vlc-redesign/blob/main/CHANGELOG.md).
