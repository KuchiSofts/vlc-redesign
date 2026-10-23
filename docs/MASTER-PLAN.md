# VLC Redesign: master plan

Status of every program, every platform and every version, and the plan to finish them. It is the source for
the roadmap, the README, the wiki and the release notes of each program.

Last updated: 2026-10-23. Desktop version: 0.2.6 released, 0.2.7 in progress.

**Contents:** [Principles](#1-principles) · [Programs and platforms](#2-programs-and-platforms) ·
[Where we are](#3-where-we-are) · [Design coverage](#4-design-coverage-figma) ·
[Other implementations](#5-other-implementations-of-the-design) · [Desktop plan](#6-desktop-windows-linux-macos) ·
[Android plan](#7-android-phones-tablets-android-tv) · [Samsung TV plan](#8-samsung-smart-tv-tizen) ·
[Apple mobile](#9-iphone-ipad-apple-tv) · [Pixel-exact process](#10-pixel-exact-process) ·
[Languages](#11-languages) · [GitHub](#12-github-repositories-releases-wiki) ·
[Quality gates](#13-quality-gates-for-every-release) · [Decisions](#14-decisions-to-make) ·
[Next steps](#15-next-steps)

---

## 1. Principles

1. **The design, pixel for pixel.** Every screen follows Juxtopposed's
   [VLC Redesign (Community)](https://www.figma.com/community/file/1691465999134370980) Figma file 1:1: sizes,
   spacing, colours, fonts, icons, states. A screen is done when its capture matches the Figma frame (see
   [section 10](#10-pixel-exact-process)), not when it "looks close".
2. **The real VLC.** The same engine, codecs and formats as VLC. Only the interface changes; fixes to the engine
   are kept small, documented, and offered upstream.
3. **English first.** Every program starts in English, whatever the system language. The language can be
   chosen in the installer and changed in the settings at any time ([section 11](#11-languages)).
4. **Smooth on every machine.** No crash, no hang, no stutter, flat memory over long sessions, on old and new
   PCs, every graphics card vendor (NVIDIA, AMD, Intel) and every supported OS version. Every release passes
   the [quality gates](#13-quality-gates-for-every-release).
5. **Credit and honesty.** The design credit (CC BY 4.0) is in every program, README, store page and release.
   Every program says it is unofficial and not made by VideoLAN; the VLC name and cone are VideoLAN trademarks.
6. **Open source.** Every binary has its source, build scripts and licenses published next to it.

## 2. Programs and platforms

| Program | Platforms | Code base | Status |
|---|---|---|---|
| **VLC Redesign for desktop** | Windows 10/11 x64 | VLC 4 + Qt 6/QML interface (this repository) | ✅ Released (0.2.6) |
| | Windows 11 ARM64 | same | 📋 Planned (0.7) |
| | Linux x86_64 (AppImage, Flatpak, .deb, .rpm) | same | 📋 Planned (0.5); built once before the Windows work |
| | macOS 12+ (Apple Silicon and Intel) | same | 📋 Planned (0.6) |
| **VLC Redesign for Android** | Android 8+ phones and tablets (Samsung Galaxy and others) | fork of VLC for Android (Kotlin, libVLC) | 💡 Research ([section 7](#7-android-phones-tablets-android-tv)) |
| | Android TV / Google TV | same app, TV layout | 💡 Research |
| **Casting to TVs** | Any DLNA TV (Samsung, LG, Sony…) and Chromecast | part of the desktop (and later Android) program | ✅ Works, being hardened on a real Samsung TV |
| **VLC Redesign TV receiver** | Samsung Tizen TVs (2017+) | Tizen web app (the TV's own player) | 💡 Research ([section 8](#8-samsung-smart-tv-tizen)) |
| VLC Redesign for iPhone / iPad / Apple TV | iOS, iPadOS, tvOS | fork of VLC for iOS | 💡 Later ([section 9](#9-iphone-ipad-apple-tv)) |

Legend: ✅ done · 🔨 in progress · 📋 planned · 💡 idea or research.

Supported versions per platform, and why:

| Platform | Minimum | Reason |
|---|---|---|
| Windows | Windows 10 1809, x64 | Qt 6.8 and VLC 4 requirements |
| Linux | glibc 2.35 (Ubuntu 22.04, Debian 12, Fedora 36+) for the AppImage and .deb; any distribution with Flatpak | Qt 6.8 and the VLC 4 build image |
| macOS | 12 Monterey | Qt 6.8 |
| Android | 8.0 (API 26) | keeps the redesign's effects (blur, rounded clipping) cheap; VLC for Android itself supports older |
| Samsung TV | Tizen 3.0 (2017 models, such as the UE55MU8000) | web apps with the AVPlay player |

## 3. Where we are

### Desktop releases

| Version | Date | Highlights |
|---|---|---|
| M0 to M3 | 2026-10-12 to 10-13 | Baseline build, first shell and player, library, playlists, subtitles, dialogs, settings |
| 0.1.0 | 2026-10-20 | First public Windows release |
| 0.2.0 to 0.2.6 | 2026-10-20 to 10-22 | Installer drawn like the app, updates from GitHub (delta, checksum-verified), ~75 fixes from a full audit, fullscreen, customize player, seek previews, wiki |
| 0.2.7 | in progress | See below |

### Done since 0.2.6 (not released yet)

- **Sound:** 5.1/7.1 channels no longer jump between speakers (WASAPI reorder), every surround layout mixed
  correctly to stereo, rear centre kept.
- **Casting:** starts once (no double load), picture and sound together from the middle of a film, the TV gets
  every film to its last picture, playlists and subtitle files work while casting, 4K converted four times faster
  by the graphics card, half the memory, 50/60 fps films at 25/30 on the TV, TVs listed before the panel opens.
- **Conversion:** the crash of every audio + video conversion fixed (also in VLC 4 itself), the end of the film
  kept.
- **Video effects:** 6 effects that crashed are fixed, 40 checked alone and in chains.
- **Damaged files:** a file with broken timestamps no longer stops the playlist; 148 damaged copies of 35 formats
  play without a crash or hang.
- **Speed and memory:** opening a file from Explorer plays at once, no firewall prompt at start, quick first start
  after installing, memory flat over long sessions (1,400 media changes in 25 minutes).
- **Real Samsung TV (2026-10-23):** three problems found by testing on a Samsung UE55MU8000, fixed in the next
  build and to be checked on the TV:
  - pausing a cast for more than about 10 seconds made the TV stop with an error: a paused Samsung TV goes on
    reading until it has about 5 seconds ahead, and gives up when nothing comes for 10 seconds. The stream now gets
    a trickle of empty (null) packets while paused;
  - a film sent as it is (no subtitles shown) was sent as a fragmented MP4, which the TV took for a 1-second file:
    an MPEG-TS profile is now preferred;
  - a TV that stops a film right after it began now gets it converted, instead of an error message.
  - Also found: the TV rejects VLC's `PrepareForConnection` request because its arguments are not in the UPnP
    order. It does no harm (VLC goes on without it), and when the request was accepted in a test the TV failed
    after a pause, so it stays as it is for now.

### Verified so far

- Regression suite (playback, pause/seek, traces, Chromecast and DLNA with test devices) passes on every build.
- Hardware: NVIDIA RTX 4090, AMD Radeon (integrated), no GPU (Windows Sandbox, 2 GB), weak CPU simulated.
- Not yet: Intel graphics, older PCs, Windows 10 on real hardware, high-DPI multi-monitor setups, other TV brands.

## 4. Design coverage (Figma)

The Figma file has 57 design frames (103 frames with states and variants) and 45 components. Every frame group has
real code behind it; the gaps are the ones a user notices first.

| Figma frames | Status | Gap |
|---|---|---|
| Onboarding ×11 | ✅ | Drawn dark in every theme |
| Player ×7 | ✅ | The compact library's Home lists recent media only |
| Player - Music ×2 | ✅ | Resume card wording and missing time ("You've watched till 00:45:08 already") |
| Player - customize ×9 | ✅ | Editing the music bar and the miniplayer not verified |
| Player - hide UI | ✅ | |
| Player - Light | 🔨 | Light theme holes: tracks/subtitles/audio popup, profiles list, onboarding, shortcut popups |
| Add Subtitles to Playlist/Folder ×4 | ✅ | OpenSubtitles needs your own key |
| Keyboard Shortcuts | ✅ | |
| Open network stream | ✅ | |
| new playlist | 🔨 | No custom thumbnail; an empty playlist can't be made |
| Settings ×19 (13 pages) | 🔨 | 5 rows missing: *Additional command line options*, *Show controls in fullscreen*, *Show the playlist as a tree*, *Show cover art when playing audio*, *Show confirmation and error dialogs* |
| HDR Options | 🔨 | HDR10+ and Dolby Vision always disabled |
| Context menus (file, folder, playlist) | 🔨 | Styled native menus, not the design's cards |
| Dialogs outside the Figma file | 📋 | Open Disc, Capture Device, Convert, Media Information, Bookmarks, Messages, advanced preferences still use the classic look |

Pixel check: 2 frames have reference captures in the repository (player, shell); the other frames are checked by
eye. The full frame-by-frame check is part of desktop 0.3 ([section 10](#10-pixel-exact-process)).

## 5. Other implementations of the design

[dsojevic/juxtopposed-vlc](https://github.com/dsojevic/juxtopposed-vlc) is another VLC 4 fork that implements the
same design (one author, started 2026-10-13, 4.0.0-beta.4). It is ahead of us on:

| Their feature | Worth adding | Where in our plan |
|---|---|---|
| Builds for Windows, macOS (arm64, Intel) and Linux from one GitHub Actions workflow, with launch tests of the packaged apps | Yes | 0.5, 0.6, CI |
| 11 languages shipped, language change without restart | Yes | 0.4 |
| Real blur behind floating panels (the design's 50 px blur) | Yes, carefully (touches video output) | 0.3 |
| Classic dialogs restyled with one Qt style hook | Yes, quick win | 0.3 |
| Redesigned Go to time, statistics, folder info, plugins list | Yes | 0.3 |
| Geometry, overlay and advanced video filters in the redesigned panel | Yes | 0.3 |
| Subtitle cues drawn on the seek bar | Nice to have | 0.4 |
| ~85 automated QML and C++ tests | Yes | CI |

We are ahead on: the installer, delta updates with rollback, casting (button, discovery, many fixes), Windows
integration (Explorer, firewall, fast start), surround sound, more subtitle providers, weekly upstream merges.

## 6. Desktop (Windows, Linux, macOS)

One code base (VLC 4 + Qt 6.8 QML) for the three systems. Windows-only parts (installer, updater, firewall card,
WinHTTP subtitle client, Direct3D paths) get an equivalent on each system or are hidden there.

### 0.2.7: Windows fixes (next release)

- Everything in [section 3](#done-since-026-not-released-yet), after the real-TV checks: pause 1 and 5 minutes,
  seek, next file, subtitles, a film sent as it is.
- Release notes, wiki pages for casting.

### 0.3: the rest of the design

- Light theme complete on every screen (the holes in [section 4](#4-design-coverage-figma)).
- Classic dialogs restyled (Open Disc, Capture, Convert, Media Information, Bookmarks, Messages).
- Context menus as the design's cards.
- The 5 missing settings rows; new playlist with a thumbnail and empty playlists; resume card text and time.
- Redesigned Go to time, statistics, plugins list; geometry/overlay/advanced video filters in the panel.
- Blur behind floating panels.
- Pixel check of all 103 frames, recorded in the wiki ([section 10](#10-pixel-exact-process)).

### 0.4: languages and trust

- Translation system, language in the installer and in the settings, without restart; right-to-left layouts
  ([section 11](#11-languages)).
- Code-signed Windows builds (no SmartScreen warning).
- Automated tests in CI: launch, playback, QML warnings, pixel checks.

### 0.5: Linux

- Build in Docker (Ubuntu 22.04 base, Qt 6.8), packages: AppImage, Flatpak (Flathub), .deb, .rpm, .tar.gz.
- X11 and Wayland; PipeWire/PulseAudio; VA-API and VDPAU decoding; Intel, AMD, NVIDIA.
- Linux equivalents: updater (AppImage update information, Flatpak updates), file associations (.desktop),
  casting firewall notes.
- Tested on Ubuntu, Fedora, Debian, Arch (VMs and a real PC).

### 0.6: macOS

- Native build on Apple Silicon and Intel (GitHub-hosted runners), universal or two .dmg files.
- Signed and notarized (needs an Apple Developer account, see [section 14](#14-decisions-to-make)).
- macOS equivalents: menu bar, media keys, Now Playing, VideoToolbox decoding, Sparkle-style updates.

### 0.7: Windows on ARM64

- Build with the ARM64 VLC toolchain; tested on a Snapdragon PC or an ARM64 VM.

### 1.0

- Every Figma frame pixel-checked on Windows, Linux and macOS.
- Rebased on the VLC 4.0 release when VideoLAN publishes it.
- All quality gates green on every platform.

## 7. Android (phones, tablets, Android TV)

The desktop interface can't simply run on a phone: VLC's Qt interface is built for a mouse and a big window. The
plan is a fork of [VLC for Android](https://code.videolan.org/videolan/vlc-android) (GPL, Kotlin, the same libVLC
engine, used by millions), with its screens redrawn in the redesign's look. It runs on every Android phone,
including Samsung Galaxy phones, and on Android TV.

The Figma file has desktop frames only. A phone and a TV layout need frames of their own: made with the design's
colours, fonts, icons and components, and ideally reviewed by Juxtopposed (see [section 14](#14-decisions-to-make)).

| Version | Content |
|---|---|
| A0 | Build VLC for Android in Docker, reproduce its release, rename the package (`org.kuchisofts.vlcredesign`), English by default |
| A1 | Design tokens (colours, accents, fonts, corner radii) and the player screen: control bar, seek bar with previews, volume, gestures |
| A2 | Library, queue, playlists, settings in the redesign's look; light and dark themes, 9 accents |
| A3 | Casting with the desktop's fixes (DLNA, Chromecast), subtitles |
| A4 | Android TV / Google TV layout (remote control navigation) |
| A5 | Release: APK and AAB on GitHub, then F-Droid; Google Play only if the name is accepted (see [section 14](#14-decisions-to-make)) |

## 8. Samsung Smart TV (Tizen)

Samsung TVs run Tizen, not Android. VLC has no Tizen version, and native (C/C++) apps on Samsung TVs are only for
Samsung partners, so the VLC engine can't run on the TV itself. What can be done:

1. **Cast from the PC (works today).** The desktop program sends the film to the TV (DLNA), converted when the TV
   can't play it, with subtitles burned in. This is where the real-TV testing happens (2017 Samsung UE55MU8000).
2. **A TV receiver app (research).** A Tizen web app in the redesign's look, which plays what the PC or phone sends
   with the TV's own player (AVPlay), shows the redesign's on-screen controls, and is controlled from the remote and
   from the desktop program.
   - Installing it on a TV needs Tizen Studio, a Samsung certificate tied to the TV, and *Developer mode* turned on
     with the TV's remote (Apps → 1 2 3 4 5 → the PC's address). Only you can do that step, at the TV. On the test
     TV, Developer mode is already on, but set to the address 127.0.0.1: the PC's address has to be entered there.
   - Publishing it in the Samsung store needs a Samsung seller account and a review.
3. **A Samsung Galaxy phone** is Android: [section 7](#7-android-phones-tablets-android-tv).

| Version | Content |
|---|---|
| T0 | Developer mode on the test TV, hello-world web app installed |
| T1 | Receiver: plays a URL sent from the desktop program, the redesign's controls, remote keys |
| T2 | The desktop program finds the receiver and uses it instead of DLNA (seek, subtitles as text, track choice) |
| T3 | Store submission (if wanted) |

## 9. iPhone, iPad, Apple TV

Later, after Android. A fork of VLC for iOS (dual-licensed MPL/GPL), same approach as Android. Needs an Apple
Developer account and App Store review, where the VLC name may not be accepted.

## 10. Pixel-exact process

How every screen is checked against the design, the same way on every platform:

1. **Reference:** each Figma frame is captured 1:1 from the prototype view
   (`scaling=min-zoom&hide-ui=1&show-proto-sidebar=0`), at 100 % and 200 % scale. The list of all frames and their
   texts is in `.specs/features/redesign/reference/inventory.json`.
2. **Capture:** the app shows the same state (same window size, media, theme, accent) and is captured at the same
   scale. Test media and the library come from a fixed demo folder.
3. **Compare:** a colour metric (the largest channel difference per pixel), not grey levels: grey hides colour
   fringes. A frame passes when no pixel differs by more than a small tolerance, except anti-aliasing at edges,
   which is listed.
4. **Record:** each frame's result, both captures and the difference image go to the wiki's *Design check* page;
   failures become issues.
5. **Keep it:** the comparisons run in CI for every pull request, so a change can't break a screen silently.

Text matching the design needs Inter with the same metrics (measured rules are in the code: distance-field text,
no hinting, Figma's line-box formula).

## 11. Languages

- **English by default** on every platform, whatever the system language: a language is used only when the
  user chose it.
- **Installer:** asks for the language on the first page (English selected), and the app starts in the language
  chosen there.
- **Settings:** *Interface → Language* changes it at once, without restarting.
- **First run:** the welcome can change it too.
- **Translations:** every string goes through VLC's gettext catalogues; new strings are extracted for translators
  (Weblate or Transifex). Shipped first: English, Hebrew; then the languages VLC already has.
- **Right to left:** Hebrew and Arabic mirror the layout (control bar, sidebar, settings) as the design would,
  checked screen by screen.
- Android and the TV receiver follow the same rules (English default, language in settings).

## 12. GitHub: repositories, releases, wiki

### Repositories (by type)

| Repository | Content |
|---|---|
| `KuchiSofts/vlc-redesign` | Desktop program (Windows, Linux, macOS): source, build scripts, installers, CI |
| `KuchiSofts/vlc-redesign-android` | Android program (phones, tablets, TV) |
| `KuchiSofts/vlc-redesign-tv` | Samsung TV receiver (Tizen web app) |
| `KuchiSofts/vlc-redesign-site` | Website (GitHub Pages): downloads for every platform, screenshots, credits |

(Or a GitHub organization holding the same repositories; see [section 14](#14-decisions-to-make).)

### Releases

- One release per program and version, tagged `vX.Y.Z` in its repository; release notes from the changelog, with
  a screenshot or GIF of what is new.
- Assets, named the same way everywhere:

| Platform | Files |
|---|---|
| Windows | `vlc-redesign-X.Y.Z-win64-setup.exe` (installer), `vlc-redesign-X.Y.Z-win64.zip` (portable), delta updates, `SHA256SUMS.txt` |
| Linux | `.AppImage`, `.flatpak` (and Flathub), `.deb`, `.rpm`, `.tar.gz` |
| macOS | `vlc-redesign-X.Y.Z-macos-arm64.dmg`, `-x86_64.dmg` |
| Android | `vlc-redesign-X.Y.Z-android-arm64.apk` (and armv7, x86_64), `.aab` |
| Samsung TV | `vlc-redesign-tv-X.Y.Z.wgt` |

- Every release: checksums, signed binaries where the platform allows, the source archive (GPL), third-party
  licenses.
- Windows also through winget; Linux through Flathub; Android through F-Droid.

### README (each repository)

1. Title, one-line description, download buttons per platform, a demo GIF.
2. Design credit (Juxtopposed, CC BY 4.0) with the video.
3. Unofficial notice (VideoLAN trademarks).
4. Features with screenshots (dark, light, accents, music, casting, settings, customize).
5. Download and install per platform; updates.
6. Known limitations; build from source; contributing; credits; license.

### Screenshots and GIFs to make (per program)

- Player: video playing, seek bar preview, volume cone, hide UI, fullscreen.
- Music: waveform, artwork.
- Library: Home, folders, playlists, queue card.
- Customize: drag and drop on the control bar, profiles.
- Themes: dark, light, the 9 accents.
- Settings: search, a few pages.
- Casting: choosing a TV, the TV playing with subtitles.
- Installer and update card.
- Android and TV: the same set on a phone and on a TV.

All made from the real programs with free demo media (no personal files), at the same window size, as PNG and as
short GIFs with an MP4 next to each.

### Wiki

Home · Download and install (per platform) · First run · Player · Music · Library and playlists · Queue ·
Subtitles · Casting to a TV (DLNA, Chromecast, Samsung notes) · Customize the interface · Themes and accents ·
Settings (one page per section) · Keyboard shortcuts · Languages · Updates · Troubleshooting · Building from
source (per platform) · Design check (frame by frame) · Differences from VLC · Release process · Credits.

### Community files

`CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, issue forms (bug, feature, design mismatch, translation),
pull request template, labels per platform and area, a project board per program, Discussions for questions.

## 13. Quality gates for every release

| Gate | What |
|---|---|
| Build | Every platform builds in CI; the packaged app starts and plays (smoke test) |
| Regression | Local playback, pause/seek, traces, Chromecast and DLNA with test devices |
| Real devices | Cast to the real Samsung TV: start, pause (1 and 5 minutes), seek, next file, subtitles, end of film |
| Formats | Sweep of every test file and container; 148 damaged files without crash or hang |
| Effects | 40 video effects alone and in chains |
| Conversion | Every muxer, audio + video, to the last picture |
| Memory | 25-minute media-change soak and cast soak: flat after warm-up |
| Hardware | NVIDIA, AMD, Intel graphics; no GPU; weak CPU; Windows 10 and 11; high DPI |
| Clean PC | Install and first start on a clean Windows (Sandbox) |
| Design | Pixel check of the frames the release touches |
| Languages | English default; installer and settings language switch |

## 14. Decisions to make

These need your answer before the work they block:

1. **GitHub layout:** separate repositories under KuchiSofts (as above), or a GitHub organization.
2. **Code signing:** a Windows code-signing certificate (yearly cost) to remove the SmartScreen warning.
3. **Apple Developer account** (yearly fee) for a notarized macOS app and any iOS work.
4. **Google Play:** whether to try the store (the VLC name may be refused); F-Droid and GitHub need no account.
5. **Samsung TV app:** turning on Developer mode on the TV (only at the TV), and whether to aim for the Samsung
   store.
6. **Phone and TV layouts:** ask Juxtopposed for phone/TV frames, or draw them ourselves from the design system
   (credited, marked as derived).
7. **Languages to ship first** after English and Hebrew.

## 15. Next steps

1. Check the new Windows build on the Samsung TV (pause, sent as it is, seek delay with the longer lead).
2. Release 0.2.7 when you say so.
3. Start 0.3 with the light theme holes and the classic dialogs.
4. Set up the Linux build in CI (0.5 groundwork) and the pixel-check tool.
5. Android A0: build VLC for Android in Docker.
