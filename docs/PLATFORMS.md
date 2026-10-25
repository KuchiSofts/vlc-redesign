# Platforms (study, 2026-10-25)

Which platforms VLC Redesign can realistically reach after Windows x64, how, at what cost, and in which order. This is
a study, not a promise: nothing below is started. The current state of each platform is in
[`platforms.yml`](../platforms.yml); the tracks and checklists are in [§9 of the master plan](MASTER-PLAN.md#9-other-platforms).

The evidence is VideoLAN's own build system in this tree (`extras/ci/gitlab-ci.yml`, `extras/package/`,
`contrib/src/`, `configure.ac`), the platform code of the Qt module (`modules/gui/qt/`), the code this project added,
and the public sources listed at the end. Efforts are for one developer who knows this tree, and include CI and a
first published build, not polishing every screen.

## Summary

| Platform | Feasible | Approach | Shared with Windows | Effort to a first build | Main blocker |
|---|---|---|---|---|---|
| Windows on ARM64 | **Yes** | Same tree, VideoLAN's llvm-mingw UCRT image, `build.sh -a aarch64` | Everything | 1–2 weeks | Qualcomm GPU drivers untested; one ARM64 test PC or VM |
| Linux x64 | **Yes, with work** | Same tree; Flatpak first, then AppImage; .deb last | QML, C++ models, libVLC | 3–5 weeks | Windows-only parts of this project's code (HTTP, updater, firewall, menus); Qt 6.8 on older distributions |
| macOS (Apple Silicon, Intel) | **Yes, with work** | Same tree with the Qt interface turned on (VideoLAN builds the native Cocoa one) | QML, C++ models, libVLC | 4–6 weeks | Qt is off on macOS upstream and untested there; Apple Developer account for signing and notarization |
| Android phones, tablets, TV | **Separate app** | Fork of VLC for Android (Kotlin, libVLC), screens redrawn in the design | Design tokens, icons, texts; libVLC | 10–16 weeks | A new UI in Kotlin; phone and TV frames do not exist; Google's developer verification; the VLC name on Play |
| ChromeOS | **Yes, through the others** | The Android app (most Chromebooks), or the Linux Flatpak in the Linux container | All of Android or Linux | 2–4 days of testing | None of its own |
| iPhone, iPad, Apple TV | **Separate app** | Fork of VLC for iOS (Swift/Objective-C, VLCKit), screens redrawn | Design tokens, icons, texts; libVLC through VLCKit | 10–16 weeks after Android | GPL code can't go to the App Store; Apple review and the VLC name; Qt has no tvOS |
| Samsung Tizen TV, LG webOS | **No for libVLC; yes for a receiver** | A web app that plays what the desktop casts, with the TV's own player | The design (as CSS); the desktop's casting code | 3–5 weeks (Tizen), +1–2 weeks (webOS) | No native code for third parties; the TV decodes, not VLC |
| Windows 32-bit, Windows 7/8 | **Not worth it** | Possible only with VideoLAN's Qt patches and a second compositor path | — | weeks, then forever | Qt 6 requires Windows 10 64-bit; 32-bit Windows 10 is out of support, Windows 11 is 64-bit only |

**Recommended order:** Windows ARM64 → Linux (Flatpak, AppImage) → macOS → Android (with Android TV and ChromeOS) →
Samsung TV receiver (then webOS) → iPhone/iPad/Apple TV. Windows 32-bit and Windows 7/8: not planned. The reasons are
in [Order](#recommended-order).

## What the tree already has

**VideoLAN's CI builds VLC 4 for every platform here except the TVs** (`extras/ci/gitlab-ci.yml`): Windows x86, x64 and
ARM64 (`win32-llvm`, `win64`, `win64-arm-llvm`), Debian x64 and arm, Snap, Raspberry Pi, macOS x86_64 and arm64, iOS,
tvOS, watchOS, visionOS, Android (four ABIs, through `libvlcjni`) and WebAssembly. Each job can use prebuilt
contribs from `artifacts.videolan.org` (`vlc-contrib-<triplet>-<sha>.tar.zst`), which is how this project's Windows
build avoids building them.

**But the Qt interface is built for Windows and Linux only:**

| Where | What it says |
|---|---|
| `contrib/src/qt/rules.mak` | Qt 6.8.3 is a contrib only `ifdef HAVE_WIN32`; the macOS line is commented out (`#PKGS += qt`). Qt is built with `FEATURE_network=OFF` |
| `configure.ac` (`--enable-qt`) | On `darwin` the default is `enable_qt="no"` |
| `extras/package/apple/build.conf` | iOS, tvOS and the other Apple mobile targets build with `--disable-qt` |
| `extras/ci/gitlab-ci.yml`, Android jobs | Only libVLC (`libvlcjni/buildsystem/build-libvlc.sh`); no interface at all |
| `modules/gui/qt/maininterface/compositor.cpp` | Compositors: `dcomp`, `platform` and `win7` on Windows; `platform` on Apple; `wayland` and `x11` on Linux. None for Android |
| `maininterface/compositor_platform.cpp` | The `platform` compositor accepts the Qt platforms `cocoa` and `ios`, so video embedding on macOS is written, but no VideoLAN build exercises it |

**This project's own code is partly Windows-only.** It builds elsewhere, but these parts fall back to doing nothing:

| Code | Windows | Elsewhere today |
|---|---|---|
| `util/http_request.cpp` (WinHTTP) | Requests for updates, subtitle search, artwork | `"No HTTP backend in this build"`: every online feature is off. Qt Network is not built (see above), so a second backend is needed: libcurl, or VLC's own `vlc_stream` (its HTTPS goes through GnuTLS on every platform) |
| `util/redesign_updater.cpp` | Installer, small updates | `isInstalledCopy()` and `appFolderWritable()` return false: updates are off. Linux and macOS need their own channel (Flatpak, AppImage update information, a Sparkle-like check) |
| `util/renderer_manager.cpp` | Firewall rule for casting | Not needed on macOS; on Linux, a firewall note instead |
| `menus/redesign_menu_style.cpp`, `mainctx_win32`, `compositor_dcomp*` | DWM corners, acrylic blur, custom title bar | Qt's defaults; the look must be checked frame by frame |
| `player/subtitle_providers.cpp` | Code-page conversion of subtitles | Latin-1 only: needs `iconv` (VLC already depends on it) |
| `extras/juxtopposed/installer/vlc-redesign.iss` | `ArchitecturesAllowed=x64compatible` | — (this already lets the x64 installer run on Windows 11 ARM64 under emulation) |

The Linux build ran once before the Windows work (`extras/juxtopposed/linux.sh`, `linux.Dockerfile`: Ubuntu 24.04,
the distribution's Qt 6.4.2, X11 and Wayland), so the code compiles there; it has not been built on Linux since.

## 1. Linux x64

**Feasibility: yes, with work.** Same tree, same QML and C++ models. VLC's Linux build is the upstream reference
(`debian` and `debian-meson` jobs), and the Qt module has both X11 and Wayland compositors.

**Approach**

1. Bring `linux.Dockerfile` up to Qt 6.8 (the master plan's minimum; Ubuntu 24.04 has 6.4.2): either Qt from the Qt
   online archives (`aqtinstall`) in an Ubuntu 22.04 image, or the Flatpak SDK below. Check that the QML still loads
   with the oldest Qt chosen (`meson.build` asks only for 6.2).
2. Give `http_request.cpp` a non-Windows backend (libcurl, or `vlc_stream_NewURL`), and `subtitle_providers.cpp` an
   `iconv` path. These are the only changes that affect features; the rest is look.
3. Packages, in this order:
   - **Flatpak** (Flathub). The `org.kde.Platform` runtime provides Qt 6 (6.9 and 6.10 are current; 6.8 is being
     retired), so only VLC, its contribs and the medialibrary are built in the manifest. Flathub's own VLC manifest
     (`org.videolan.VLC`, VLC 3) shows which libraries to build. Flathub builds from source without network access,
     needs an AppStream file, and gives updates for free. The app id must be the project's
     (`io.github.kuchisofts.VLCRedesign` or similar), not VideoLAN's.
   - **AppImage**: built on the oldest base supported (glibc 2.35, Ubuntu 22.04) with Qt bundled
     (`linuxdeploy` and its Qt plugin), with update information for `AppImageUpdate`. Hardware decoding uses the
     system's VA-API/VDPAU drivers, which must not be bundled.
   - **.deb/.rpm**: last. Distributions ship their own Qt, and most LTS releases are older than 6.8, so a .deb would
     bundle Qt anyway (then it is an AppImage in another wrapper) or target only Debian 13 / Ubuntu 25.04 and newer.
4. File associations through the `.desktop` file and MIME types; casting needs a firewall note (no rule is added).

**Blockers and risks:** the HTTP and updater gaps above; Wayland differences (no global window positions, client
side decorations, the custom title bar); GPU driver variety (Intel, AMD, NVIDIA proprietary, Mesa); the pixel check of
every frame on a second text renderer (FreeType instead of DirectWrite: the text rendering rules of this project
were tuned on Windows).

**Effort:** 3–5 weeks: 1 week for the build and CI, 1 week for HTTP/iconv/updates, 1–2 weeks for Flatpak and AppImage,
1 week for testing on Ubuntu, Fedora, Debian and Arch.

**Signing and stores:** none required. Flathub signs its repository; AppImages can carry a GPG signature; a .deb
repository needs a GPG key. Flathub reviews the submission (naming, licence, AppStream data).

**GitHub Actions:** `ubuntu-24.04` runners. One job builds in the Docker image (as the Windows job does), one runs
`flatpak-builder` (the `flatpak/flatpak-github-actions` action), one makes the AppImage in an Ubuntu 22.04 container,
and a launch test runs the package under Xvfb with `xdotool` (the tools are already in `linux.Dockerfile`).

## 2. Windows on ARM64

**Feasibility: yes.** VideoLAN builds VLC 4 for ARM64 Windows in every merge request (`win64-arm-llvm`) and publishes
nightlies (`artifacts.videolan.org/vlc/nightly-win64-arm-llvm/`), with Qt: on Windows, Qt is a contrib for every
architecture, so the prebuilt ARM64 contribs include it. Qt 6.8 itself supports Windows on ARM. All of this project's
Windows code (WinHTTP, DirectComposition, DWM, WASAPI, the installer) exists on ARM64 Windows.

**Approach**

1. A second Docker image based on `registry.videolan.org/vlc-debian-llvm-ucrt:20260611225331` (the x64 build uses
   `vlc-debian-win64-posix`, which is GCC: ARM64 needs llvm-mingw), running `extras/package/win32/build.sh -a aarch64`
   with prebuilt contribs (`-p`).
2. `make-release.ps1`, the installer and the small updates take an architecture: `ArchitecturesAllowed=arm64`, file
   names `...-winarm64-setup.exe`, a separate update channel, so an ARM64 copy never receives x64 files.
3. Until then, the x64 build already installs and runs on Windows 11 ARM64 under emulation (`x64compatible`); it is
   slower and its plugins cannot be mixed with native ones.

**Blockers and risks:** no ARM64 PC here (a Snapdragon laptop, or an ARM64 Windows VM on an ARM host; the
`windows-11-arm` GitHub runner covers the launch test); Qualcomm Adreno drivers and DirectComposition: a 2024 report
shows a VLC 4 ARM64 nightly asserting in the Qt compositor on a Surface Laptop 7, so the `dcomp` path must be tested
on real hardware; any x64-only tool used at build or install time (`audiodefault.exe`-style helpers) must be built
for ARM64 too.

**Effort:** 1–2 weeks.

**Signing:** the same SignPath setup as x64 ([code-signing.md](code-signing.md)): one more artifact configuration,
or the same one applied to the ARM64 package.

**GitHub Actions:** a `winarm64` job next to `win64` in `build-windows.yml` (Ubuntu runner, Docker), a `launch-test`
on `windows-11-arm`, and the release job made per architecture.

## 3. macOS (Apple Silicon and Intel)

**Feasibility: yes, with work.** VLC 4 for macOS ships its native Cocoa interface (`modules/gui/macosx`); the Qt
interface is not built there (`enable_qt="no"` on darwin, the Qt contrib commented out). But the Qt module has macOS
code (`qt.cpp` and `compositor_platform.cpp` handle `cocoa`, `meson.build` passes the framework path to `moc`), and
Qt 6.8 supports macOS 12 and newer on both architectures. Running it is a build-system change plus testing, not a port.

**Approach**

1. Build on macOS with `extras/package/macosx/build.sh -a aarch64` (and `-a x86_64`) with `--enable-qt`, using
   VideoLAN's prebuilt macOS contribs, plus Qt 6.8 (either from the Qt archives, or by enabling the commented-out
   contrib line and building it once).
2. Make the bundle: VideoLAN's `package-macosx` targets build `VLC.app` around the Cocoa interface; the redesign needs
   its own app bundle (`Info.plist`, icon, the Qt frameworks and QML imports placed by `macdeployqt`), and the Cocoa
   interface left out so the Qt one is the default.
3. Two .dmg files (arm64, x86_64) or one universal bundle made with `lipo` (doubles the size; VideoLAN ships separate
   ones).
4. macOS behaviour the Windows code does not have: the global menu bar, `Cmd` shortcuts, media keys and Now Playing
   (VLC's macOS code has them in the Cocoa module, not in Qt), full screen in its own Space, the HTTP backend and
   updates (as for Linux).

**Blockers and risks:** the Qt interface with video embedding on macOS has no VideoLAN user, so problems found there
are this project's to fix; the look of the custom title bar against macOS window buttons; text rendering through
Core Text, a third renderer for the pixel check.

**Signing and notarization:** required in practice. An unsigned app downloaded from the internet is blocked by
Gatekeeper, and since macOS 15 the Control-click way around it is gone (users must allow it in System Settings →
Privacy & Security). A notarized app needs the Apple Developer Program (yearly fee; decision 3 of the master plan),
a *Developer ID Application* certificate, the hardened runtime (VideoLAN's `extras/package/macosx/vlc-hardening.entitlements`
is the starting point: it grants `cs.allow-jit` and `cs.disable-library-validation`, which VLC's plugins need), every binary and plugin signed, then
`notarytool submit` and `stapler staple`. SignPath does not cover macOS.

**Effort:** 4–6 weeks: 2 weeks to a running bundle, 1 week for signing and notarization in CI, 1–3 weeks for macOS
behaviour and the frame check.

**GitHub Actions:** `macos-15` runners (Apple Silicon; the Intel build is cross-compiled there, as VideoLAN's
`build.sh -a x86_64` does, since GitHub is retiring its Intel macOS images). Secrets: the certificate as a `.p12`,
its password, an App Store Connect API key for `notarytool`. Launch test: open the .dmg, start the app, play a file.

## 4. Android phones, tablets and TV

**Feasibility: a separate app.** The Qt/QML interface can't reasonably become the Android app:

- VLC's Qt module is an *interface plugin* started by VLC's core inside a desktop process. On Android, VLC is a library
  (`libvlcjni`) inside an Android app, and VideoLAN builds it without any interface.
- There is no compositor for Android (`compositor.cpp`), so video can't be placed under the QML; a new one would have
  to hand an `ANativeWindow` to the video output and follow the activity's surface being destroyed and recreated.
- Qt is not a contrib for Android; it would be built with the NDK (Qt 6.8 supports Android 9, API 28, and newer).
- The interface depends on Qt Widgets (dialogs, menus, preferences) and on a mouse and a large window. A phone needs
  touch layouts, gestures, the back button, Android's media session, notification controls, storage permissions,
  picture-in-picture; Android TV needs D-pad focus everywhere. That is a new UI in either toolkit.

So the realistic path is the one in the master plan: a fork of **VLC for Android** (`code.videolan.org/videolan/vlc-android`,
GPLv2+, Kotlin, on libVLC), keeping its engine, media library, casting and Android integration, and redrawing its
screens in the design. Qt for Android would mean rewriting all of that Android integration to save only the QML
files, which would themselves need phone layouts.

**What is shared:** the design tokens, icons, fonts and texts, once exported from the desktop (`design/`, step 6 of the
master plan) and turned into Android resources by a script; the translations (gettext `.po` converted to
`strings.xml`); libVLC; the casting fixes as behaviour (not as code: the desktop's are C++ in VLC's core, Android's go
through libVLC's renderer API). Nothing in `modules/gui/qt` is reused.

**Approach**

1. Reproduce VLC for Android's own build in Docker (`registry.videolan.org/vlc-debian-android` image, its
   `buildsystem/compile.sh`), then the same build in CI.
2. Rename the package (`org.kuchisofts.vlcredesign`), English by default, the project's name and icon.
3. Redraw the screens in order: player and its controls, library, queue and playlists, settings, then the TV layout.
4. Releases: APK and AAB on GitHub, then F-Droid, then Google Play if decided (decision 4).

**Blockers and risks**

- **Frames:** the Figma file has desktop frames only (decision 6).
- **Upstream merges:** VLC for Android changes often; a redrawn UI conflicts with every upstream UI change. Keep the
  changes in new files and themes as far as possible.
- **Google Play rules:** since 2026-08-31, new apps and updates must target Android 16 (API 36) (Android TV apps:
  Android 14, API 34); apps targeting Android 15+ must support 16 KB memory pages, which every native library (libVLC
  and its contribs) must be built for. A developer account costs a one-time fee.
- **Developer verification:** Google requires developers to register their identity and package names for apps
  installed on certified Android devices. It starts on 2026-09-30 for apps from participating stores in a few
  countries (Google's pages name Indonesia, Singapore and Thailand, the developer site Brazil too); apps installed
  from elsewhere (an APK on GitHub, F-Droid) are not covered at first, and the rollout to every app on certified
  devices worldwide is planned for 2027, with an "advanced flow" that still lets experienced users install
  unverified apps. Verify the developer account before a first public APK. (Checked 2026-10-25; Google has changed
  these pages before.)
- **The VLC name:** "VLC" and the cone are trademarks registered by VideoLAN. A Play listing named like VLC can be
  refused or reported as impersonation; the store name should not start with "VLC".

**Effort:** 10–16 weeks: 1 week for the build, 6–10 weeks for the phone screens, 3–4 weeks for Android TV, 1 week for
store and F-Droid metadata. More if phone frames must be drawn first.

**GitHub Actions:** an Ubuntu runner with the VideoLAN Android image (or the Android SDK/NDK actions), building the AAB
and APKs per ABI; the upload keystore as a secret; an emulator launch test (`reactivecircus/android-emulator-runner`).

## 5. ChromeOS

**Feasibility: yes, through the Android app or the Linux packages; no work of its own.**

- Most Chromebooks run Android apps (ARCVM); VLC for Android already supports ChromeOS (keyboard, mouse, windows),
  so the fork inherits it.
- The Linux development environment (a Debian container) runs Flatpak or AppImage; GPU acceleration there depends on
  the model, so playback is weaker than the Android app.
- Google has confirmed that ChromeOS is being moved onto Android over the coming years, which makes the Android app the
  durable choice.

**Effort:** 2–4 days of testing on a Chromebook (Android app, window resizing, keyboard shortcuts); listing ChromeOS
in `platforms.yml` under Android.

## 6. iPhone, iPad, Apple TV

**Feasibility: a separate app, after Android.** The Qt path is closed for three reasons: Qt 6 does not support tvOS;
VLC's Qt module and this project's code are GPL, and VideoLAN's experience shows that GPL code can't stay in the App
Store (the first VLC for iOS was removed in 2011 after a GPL complaint; it came back in 2013 rewritten, with the app
under MPL-2.0/GPLv2+ and libVLC under LGPL); and the same desktop-UI problems as on Android.

**Approach:** a fork of **VLC for iOS** (`code.videolan.org/videolan/vlc-ios`: Swift and Objective-C on VLCKit, with
iPhone, iPad and Apple TV targets), its screens redrawn with the tokens and the Android screens as the model.
VideoLAN's CI already builds the libraries for iOS and tvOS (`ios-arm64`, `tvos-arm64` with `--disable-qt`).

**Blockers and risks**

- **Licence of new code:** for App Store distribution, the new code must be MPL-2.0 (as VLC for iOS is), not GPL-only:
  nothing can be copied from the GPL desktop or Android code, only redrawn.
- **Apple:** the Developer Program (yearly fee, decision 3), App Review (the guidelines on copies of existing apps and
  on trademarks: a "VLC"-named fork next to VideoLAN's own app is likely to be refused), certificates and provisioning
  profiles. In the EU, alternative marketplaces exist, but their apps are still notarized by Apple.
- **Upstream merges,** as for Android.

**Effort:** 10–16 weeks after Android exists (less design work, as the phone and TV layouts would be drawn by then).

**GitHub Actions:** `macos-15` runners with Xcode; the signing certificate and provisioning profiles as secrets
(`fastlane match` or plain keychain import); TestFlight upload with an App Store Connect API key.

## 7. Samsung Tizen TV and LG webOS

**Feasibility: no for libVLC; yes for a receiver app.** Neither platform has a libVLC build (VideoLAN's CI has no TV
job), and neither lets third parties ship the native code it would need: Samsung TV apps are web apps (HTML and
JavaScript, with .NET as the other option; native C/C++ was for partners only), and LG's webOS TV store takes web
apps from everyone, with native apps a partner matter. A VLC engine on the TV is therefore not possible.

**What is possible** is the receiver already planned in `platforms.yml`: a web app on the TV, drawn in the design,
that plays what the desktop casts with the TV's own player (Samsung's AVPlay API; webOS's HTML5 media with its media
extensions), with the remote keys, and that the desktop finds and controls instead of DLNA (seek, text subtitles,
track choice). The TV decodes, so the formats are the TV's: the desktop keeps converting what the TV can't play, as its
DLNA casting does today (see the Samsung TV notes in the master plan).

**Approach:** Tizen first (the test TV is a Samsung UE55MU8000, Tizen 3.0), then webOS reusing the same web code with
a second packaging and player layer. A small protocol between the desktop and the receiver (HTTP plus a WebSocket on
the local network), with the desktop side in the casting code.

**Blockers and risks:** Tizen needs Developer mode bound to the PC's address (only at the TV, decision 5), Tizen Studio
and a Samsung certificate tied to the TV; store publication needs a Samsung seller account and review (LG: an LG
seller account and review). Old TVs have old browsers (Tizen 3.0: Chromium 47-era), so the web code must be plain.

**Effort:** 3–5 weeks for Tizen (receiver, desktop side, remote keys), 1–2 weeks more for webOS.

**GitHub Actions:** the web app is built and packaged with the Tizen CLI (`tizen package`) in a container; signing
needs the author and distributor certificates as secrets. Testing stays manual on the TV.

## 8. Windows 32-bit, Windows 7 and 8

**Feasibility: technically possible, not worth it.**

- VideoLAN still builds VLC 4 for 32-bit Windows (`win32-llvm`) and keeps Windows 7 working: `build.sh` defaults to
  `WINVER=0x0601`, VideoLAN patches Qt 6.8 for Windows 7 (`contrib/src/qt/0001-Satisfy-Windows-7-compatibility.patch`
  and the DirectWrite patches), and the Qt module has a `win7` compositor.
- But Qt 6 itself supports Windows 10 and 11, 64-bit only; the patches are VideoLAN's to keep alive, not Qt's.
  This project requires Windows 10 1809 and relies on DirectComposition, the acrylic blur and DWM corners, none of
  which exist on Windows 7, so the Windows 7 path would be a second, plainer look to design, test and keep.
- 32-bit Windows exists only as Windows 10, whose support ended on 2025-10-14; Windows 11 is 64-bit only.
- Every extra Windows build doubles the release files, the small updates and the signing.

**Recommendation:** not planned. Users on these systems keep VideoLAN's VLC 3.

## Recommended order

1. **Windows on ARM64** (1–2 weeks): the smallest step, entirely in the current CI and installer, and a growing
   market (Snapdragon PCs). The master plan puts it after macOS; it is moved first here because it needs no new
   account and no port.
2. **Linux** (3–5 weeks): same tree; the HTTP and updater work it needs is also needed by macOS.
3. **macOS** (4–6 weeks): same tree, after decision 3 (the Apple Developer account).
4. **Android, Android TV, ChromeOS** (10–16 weeks): the largest new audience; needs the design tokens exported (step 6)
   and the phone and TV frames (decision 6). Start the Google developer verification early.
5. **Samsung TV receiver, then webOS** (4–7 weeks): builds on the casting work; can run in parallel with Android, since
   it is web code.
6. **iPhone, iPad, Apple TV** (10–16 weeks): reuses Android's screens; the most store risk.
7. **Windows 32-bit, Windows 7/8:** not planned.

Steps 1–3 grow the desktop program, which keeps one version number. Each separate app gets its own version from its
first published build (section 13.4 of the master plan).

## Repository layout

**One repository for the desktop program, sister repositories for each separate app:**

| Repository | Contents | Base | Licence |
|---|---|---|---|
| `KuchiSofts/vlc-redesign` (this one, the hub) | VLC 4 + the Qt interface for Windows x64 and ARM64, Linux, macOS; `platforms.yml`; `design/` (tokens, icons, fonts); the website | VideoLAN's `vlc` | GPLv2+ |
| `KuchiSofts/vlc-redesign-android` | Phones, tablets, Android TV, ChromeOS | VideoLAN's `vlc-android` | GPLv2+ |
| `KuchiSofts/vlc-redesign-ios` | iPhone, iPad, Apple TV | VideoLAN's `vlc-ios` | MPL-2.0 / GPLv2+ (as upstream) |
| `KuchiSofts/vlc-redesign-tv` | The TV receiver: Tizen and webOS packaging of one web app | new | GPLv2+, or MPL-2.0 if it may be reused elsewhere |

Why this split:

- **Desktop platforms share one tree.** Linux, macOS and ARM64 are the same C++ and QML; separate repositories would
  mean merging every fix three times. They differ only in CI jobs and `extras/juxtopposed/` packaging scripts, which
  become per-platform folders (`extras/juxtopposed/linux/`, `macos/`) next to the Windows ones.
- **Each app follows its own upstream.** `vlc-android` and `vlc-ios` are separate VideoLAN repositories with their own
  history, release cycles and merge requests; a fork of each keeps `git merge` from upstream possible. Putting them in
  this tree would mix three unrelated histories.
- **Licences differ.** The iOS code must stay MPL-compatible for the App Store; a separate repository keeps GPL code
  from reaching it by accident.
- **The hub stays the single source** of the shared parts: `design/` is exported from the desktop and published with
  a version tag; each sister repository pins a tag and converts the tokens at build time (Android resources, iOS asset
  catalogs, CSS). `platforms.yml` stays here and already has a `repository` field per platform, so the README's
  platform bar and each sister repository's README read one file. Issues can stay in the hub with a label per
  platform, so users have one place to report.

`platforms.yml` changes this study suggests (not made here): add *ChromeOS* (through Android and Linux) and *LG webOS*
(receiver, idea), and give the separate apps their sister repositories once they exist.

## Sources

In this repository:

- [`extras/ci/gitlab-ci.yml`](../extras/ci/gitlab-ci.yml): VideoLAN's build jobs and images
- [`contrib/src/qt/rules.mak`](../contrib/src/qt/rules.mak), [`contrib/src/main.mak`](../contrib/src/main.mak): Qt 6.8.3, Windows only, no Qt Network
- [`configure.ac`](../configure.ac) (`--enable-qt`), [`extras/package/apple/build.conf`](../extras/package/apple/build.conf), [`extras/package/macosx/`](../extras/package/macosx/), [`extras/package/win32/build.sh`](../extras/package/win32/build.sh)
- [`modules/gui/qt/maininterface/compositor.cpp`](../modules/gui/qt/maininterface/compositor.cpp), [`compositor_platform.cpp`](../modules/gui/qt/maininterface/compositor_platform.cpp)
- [`modules/gui/qt/util/http_request.cpp`](../modules/gui/qt/util/http_request.cpp), [`util/redesign_updater.cpp`](../modules/gui/qt/util/redesign_updater.cpp)
- [`extras/juxtopposed/linux.sh`](../extras/juxtopposed/linux.sh), [`linux.Dockerfile`](../extras/juxtopposed/linux.Dockerfile), [`installer/vlc-redesign.iss`](../extras/juxtopposed/installer/vlc-redesign.iss)
- [`docs/MASTER-PLAN.md`](MASTER-PLAN.md) §2, §4, §9, §17; [`platforms.yml`](../platforms.yml)

Public:

- Qt 6.8 supported platforms: https://doc.qt.io/qt-6.8/supported-platforms.html
- Qt 6.0 development hosts and targets (tvOS and watchOS dropped): https://www.qt.io/blog/qt6-development-hosts-and-targets
- List of platforms supported by Qt: https://en.wikipedia.org/wiki/List_of_platforms_supported_by_Qt
- VideoLAN source repositories: https://code.videolan.org/videolan/vlc, https://code.videolan.org/videolan/vlc-android, https://code.videolan.org/videolan/vlc-ios, https://code.videolan.org/videolan/libvlcjni
- VLC ARM64 Windows nightlies announced: https://mailman.videolan.org/pipermail/vlc-devel/2022-November/143527.html
- VLC nightlies page update (ARM64 LLVM builds): https://mailman.videolan.org/pipermail/www-doc/2024-November/008710.html
- ARM64 nightly asserting in the Qt compositor (Surface Laptop 7): https://forum.videolan.org/viewtopic.php?p=546765
- Windows Store ARM64 status: https://mailman.videolan.org/pipermail/vlc-devel/2024-June/143655.html
- VideoLAN trademarks: https://www.videolan.org/legal.html, https://wiki.videolan.org/Intellectual_Property
- VLC for iOS returns, re-licensed: https://news.slashdot.org/story/13/07/18/1943202/vlc-for-ios-returns-on-july-19-rewritten-and-fully-open-sourced, https://www.cultofmac.com/?p=201703
- Google Play target API level requirements: https://developer.android.com/google/play/requirements/target-sdk, https://support.google.com/googleplay/android-developer/answer/11926878
- Android 16 KB page sizes: https://developer.android.com/guide/practices/page-sizes
- Android developer verification: https://developer.android.com/developer-verification, https://support.google.com/android-developer-console/answer/16561738, https://f-droid.org/2026/02/24/open-letter-opposing-developer-verification.html, https://pinggy.io/blog/f_droid_2_0_android_developer_verification/
- Flatpak runtimes: https://docs.flatpak.org/en/latest/available-runtimes.html; KDE 6.10 runtime: https://discuss.kde.org/t/flatpak-org-kde-sdk-6-10-runtime-is-now-available/41158
- Flathub requirements: https://docs.flathub.org/docs/for-app-authors/requirements
- Apple notarization: https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution
- ChromeOS moving onto Android: https://www.channelnews.com.au/google-confirms-chromeos-and-android-merger-in-major-software-overhaul/
- Tizen TV profile: https://docs.tizen.org/platform/what-is-tizen/profiles/tv; Samsung forum on native TV apps: https://forum.developer.samsung.com/t/native-apps-in-tizen-studio/29393
- LG webOS TV developer forum (web engine): https://forum.webostv.developer.lge.com/t/android-native-app/2455
- Windows 10 end of support: https://www.microsoft.com/windows/end-of-support
