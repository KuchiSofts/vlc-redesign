# VLC Redesign on macOS

The macOS port of VLC Redesign: Apple Silicon and Intel, macOS 12 Monterey or later (Qt 6.8). Roadmap step 7,
[MASTER-PLAN.md §9](../../../docs/MASTER-PLAN.md#macos). Both architectures build and pass the launch test on
GitHub's Macs (28 October 2026, 0.3.2 tree); the checks added for 0.4.0 (the unified title bar, the bundle checks,
the screenshots) run first on the public repository at the upload
([RELEASE-CHECKLIST.md](../../../docs/RELEASE-CHECKLIST.md)).

| File | What it does |
|------|--------------|
| `extras/juxtopposed/macos-build.sh` | The build on a Mac: contribs, Qt, VLC, `VLC Redesign.app`, the `.dmg` |
| `extras/juxtopposed/macos/make-info-plist.py` | The app's `Info.plist` from upstream's `share/Info.plist.in` |
| `extras/juxtopposed/macos/AppIcon.svg` | The app icon (the redesign's logo on its card), drawn into `AppIcon.icns` |
| `extras/juxtopposed/ci/macos-smoke.sh` | The launch test: the bundle (signatures, icon, translations, Live TV, About badge, credits), headless playback, the Qt interface and its unified title bar, screenshots light, dark and About |
| `.github/workflows/build-macos.yml` | Public repository: both architectures on every pull request and push to main (free). Private: by hand only, Apple Silicon only, 40 minutes at most (macOS minutes count ×10) |
| `modules/gui/qt/maininterface/macos_titlebar.cpp` | The unified title bar (the traffic lights in the redesign's top bar) |

On a Mac with Xcode: `extras/juxtopposed/macos-build.sh` (its own architecture; `-h` lists the options), then
`extras/juxtopposed/ci/macos-smoke.sh build-macos-arm64/vlc-redesign-*.dmg`.

## 1. VLC 4 with the Qt interface on macOS

**Upstream supports the Qt interface on macOS in code, but neither builds nor ships it.**

- The code is there: `modules/gui/qt/qt.cpp` runs Qt's event loop on the main thread with
  `CFRunLoopPerformBlock` (Cocoa needs it) and quits through `applicationShouldTerminate:`; `plugins.hpp` imports
  the cocoa platform and the mac style for a static Qt; the video goes through `CompositorPlatform`
  (`compositor.cpp`, `_WIN32 || __APPLE__`), which hands the vout an `NSView`; `bin/darwinvlc.m` (the macOS main)
  runs a CoreFoundation run loop that any interface can use.
- `configure` turns Qt off on darwin unless asked: `AS_IF([test "${SYS}" = "darwin"], [enable_qt="no"])`.
  `--enable-qt` turns it on.
- The contribs have no Qt for macOS: `contrib/src/qt/rules.mak` has `ifdef HAVE_MACOSX #PKGS += qt` (commented out),
  so VideoLAN's prebuilt macOS contribs (`artifacts.videolan.org/vlc/macos-arm64/`, `macos-x86_64/`) carry none,
  and upstream's macOS CI (`extras/ci/gitlab-ci.yml`, `macos-arm64` / `macos-x86_64`) builds only the native
  `macosx` interface (`extras/package/macosx/configure.sh --enable-macosx`).

**Decision: Qt 6.8.3, the official build** (aqtinstall, as the Linux build), not a Qt built in the contribs:

| | Official Qt 6.8.3 (chosen) | Qt in the contribs |
|-|-|-|
| Time on CI | a 1 GB download, cached | 1–2 h per architecture, Qt's three modules |
| Known to work | Qt's own macOS build | upstream never builds it for macOS (patches made for Windows) |
| Same version as Windows | yes (6.8.3) | yes |
| Linked | shared frameworks, deployed by `macdeployqt` | static, in `libqt_plugin.dylib` (as on Windows) |
| Architectures | one universal build for both | one per architecture |

The contribs (FFmpeg, dav1d, libplacebo, medialibrary…) are VideoLAN's prebuilt ones for the exact tree when they
exist: the archive is named after the last commit that touched `extras/tools`, `contrib`, `extras/ci` or
`extras/package/macosx` (`extras/ci/get-contrib-sha.sh`). The redesign's own commits there have no archive, so the
script takes the newest upstream commit that has one (as the Windows workflow does); without any, it builds them
(an hour or more). The redesign's `contrib/src/upnp` change is then not in the macOS build until it is built from
source.

## 2. The Qt interface by default

The `macosx` interface has priority 200, Qt 151: with both built, VLC starts the native one. The build passes
`--disable-macosx --enable-qt` after upstream's options (`configure.sh` passes them first, so ours win), so Qt is the
only interface, and checks it (`ENABLE_MACOSX_UI` false, no `libmacosx_plugin.dylib`, `libqt_plugin.dylib` there).
Also `--disable-sparkle` (its feed is VideoLAN's and would offer VLC), `--disable-skins2`, `--disable-debug`, and
libcurl of macOS for the interface's online subtitles, artwork and update check (the SDK has its headers, no
pkg-config file: `QT_LIBCURL_CFLAGS`/`QT_LIBCURL_LIBS` given to configure).

Before Qt starts, the interface runs `vlc-qt-check`, found next to the executable in a bundle
(`src/darwin/dirs.m`: `pathForAuxiliaryExecutable`): it is copied to `Contents/MacOS`, or Qt is skipped and VLC
starts no interface.

## 3. The app bundle

```
VLC Redesign.app/Contents/
  Info.plist            make-info-plist.py: org.kuchisofts.vlcredesign, macOS 12, the file types of VLC
  PkgInfo               APPL????
  MacOS/VLC Redesign    bin/darwinvlc.m (installed as bin/vlc)
  MacOS/vlc-qt-check, vlc-preparser, vlc-waveform    helpers (dirs.m; player/waveform.cpp looks next to the executable)
  Frameworks/libvlc*.dylib, libvlccore*.dylib        dirs.m: the bundle's private frameworks folder
  Frameworks/plugins/lib*_plugin.dylib, plugins.dat  flat, as upstream's VLC.app
  Frameworks/lua/                                    Lua extensions and playlist scripts
  Frameworks/Qt*.framework                           macdeployqt
  PlugIns/{platforms,styles,imageformats,iconengines}/ Qt's plugins (macdeployqt)
  Resources/qml/                                     Qt's QML modules (the interface's own QML is in its resources)
  Resources/qt.conf                                  macdeployqt: where Qt finds the above
  Resources/share/{lua,locale,hrtfs,...}             dirs.m: VLC's data
  Resources/AppIcon.icns                             AppIcon.svg in every size (rsvg-convert, sips, iconutil)
```

- **Bundle id `org.kuchisofts.vlcredesign`**, the package name planned for Android. VLC names its folders after the
  bundle id (`dirs.m`): settings in `~/Library/Preferences/org.kuchisofts.vlcredesign`, the library and caches in
  `~/Library/Application Support/` and `~/Library/Caches/` under the same name, apart from VLC's
  `org.videolan.vlc`: VLC and VLC Redesign can be installed side by side.
- **Every library is found in the bundle**: references to the build or to Qt's download become `@rpath/…`, the
  programs search `@executable_path/../Frameworks`, the plugins `@loader_path/..`; the build fails if a binary still
  points out of the bundle.
- **File associations**: upstream's list (109 document types, its UTIs, the streaming URL schemes) with three
  changes: the types are *imported* (VLC owns them; two apps exporting one UTI is undefined), the handler rank is
  `Alternate` (VLC Redesign is offered in *Open With* and can be made the default; it never takes the files of the
  player in use at install), and `http`, `https`, `ftp`, `sftp`, `smb` are left to the browser and Finder. No
  document icons: upstream's are VideoLAN's cone.
- **Icon**: the redesign's logo (`design/icons/svg/logo.svg`) on its card colour in the macOS icon grid. The Windows
  executable still has upstream's `vlc.ico`; one icon for both is a question for the owner.
- **Live TV (IPTV) add-on**: `Frameworks/plugins/libiptv_plugin.dylib` (QtCore, from the bundle's Qt). A disk image
  has no components to choose, unlike the Windows installer: the app always carries it, and *Settings → Startup →
  Live TV (IPTV) add-on* (`iptv-enabled`) turns it off. Deleting the file would break the app's signature;
  `macos-build.sh -t` makes an app without it.
- **Texts**: `Resources/README.txt`, `CREDITS.txt` (Juxtopposed, the design's author, CC BY 4.0), `COPYING.txt`,
  `COPYING.LIB.txt`, `CHANGELOG.txt` and `licenses/` (the fonts' OFL), as in the Windows package.
- **Disk image**: `vlc-redesign-X.Y.Z-macos-arm64.dmg`, `vlc-redesign-X.Y.Z-macos-x86_64.dmg` (§13.5), the app and a
  link to Applications, compressed (UDZO), with a `.sha256` beside it.

## 4. Signing and notarization (decision 3)

Every Mac binary must be signed, and Apple Silicon refuses to run one that is not. Without an Apple Developer ID
(decision 3, a yearly fee) the build signs **ad hoc** (`codesign -s -`):

- It runs on the Mac that built it, and from a CI artifact unpacked with a command-line tool.
- Downloaded with a browser, the disk image is quarantined; Gatekeeper then says the app "cannot be opened because
  Apple cannot check it for malicious software". On macOS 12–14, Control-click the app → *Open* → *Open*. On macOS
  15 Sequoia and later that no longer works: open it once, then *System Settings → Privacy & Security → Open
  Anyway*, and confirm with the password. Or, in Terminal: `xattr -dr com.apple.quarantine "/Applications/VLC
  Redesign.app"`. The release notes and the README must say so as long as the app is not notarized.
- An ad hoc signature changes with every build: the Mac asks again for access to folders and the microphone after
  an update.

With a Developer ID ("Developer ID Application" certificate in the keychain), the same script signs every framework,
library and plugin, then the programs with the **hardened runtime** and upstream's entitlements
(`extras/package/macosx/vlc-hardening.entitlements`: JIT, library validation off, camera, microphone, Apple Events),
signs the disk image and, with `NOTARY_PROFILE` (a `notarytool store-credentials` profile), **notarizes and staples**
it:

```
MACOS_SIGN_IDENTITY="Developer ID Application: <name> (<team>)" NOTARY_PROFILE=vlc-redesign \
    extras/juxtopposed/macos-build.sh
```

In CI that needs secrets for the certificate (`.p12` and its password, imported into a temporary keychain) and for
notarytool (an App Store Connect API key, or Apple ID, team id and an app-specific password). Not wired yet: it
waits for decision 3.

## 5. One universal app or two

**Two builds, one per architecture** (§13.5): VLC's contribs and VideoLAN's prebuilt archives are per architecture,
each runner builds its own natively (the plugin cache and the smoke test need to run what was built), and each
download is half the size. A universal app later is `lipo -create` of every Mach-O of the two apps (Qt's official
frameworks are already universal) into one bundle, then signed again: a step after both builds, if users want one
download.

## 6. The redesign's code on macOS

Fixed here (each guarded by `__APPLE__` / `Q_OS_MACOS`; compiled for Linux in Docker, the macOS branches only
checked for syntax with their guards forced, as no Mac is at hand):

- **Window frame: a unified title bar** (`maininterface/macos_titlebar.cpp`), the way macOS apps draw their own bar.
  The window keeps its system frame, so the traffic lights, full screen in its own Space, tiling, resizing from every
  edge, the shadow and the border stay the system's; its title bar is made transparent and untitled
  (`titlebarAppearsTransparent`, `titleVisibility`) and the content fills the whole window under it
  (`NSWindowStyleMaskFullSizeContentView`). The redesign's top bar (the library's, the player's and the menu bar's) is
  then the only bar: it leaves room for the traffic lights at its start (`MainCtx.windowButtonsInsetLeft/Right`, read
  from AppKit's buttons, on the right in a right to left system language) and drags and zooms the window as on
  Windows (`startSystemMove`). The redesign's own minimize, maximize and close buttons are not made on macOS
  (`CSDButtonModel`); the system draws the resize edges, shadow and border (`platformHandles*WithCSD`). Applied
  before the saved geometry is restored and again whenever the native window is made, shown or leaves full screen;
  the content keeps its place on the screen when it changes (the frame moves), so a saved geometry never grows by a
  title bar. Qt 6.8 has no flag for it (`Qt::ExpandedClientAreaHint` is 6.9); the few AppKit calls go through the
  Objective-C runtime (`objc_msgSend`, the Qt module is C++ only; `-lobjc`). `qt-titlebar` is off by default as on
  the other systems (it was on for macOS until 0.4.0, which drew the system's title bar above the redesign's);
  *Settings → Presentation → Show window decorations* still brings the system's title bar back. The classic windows
  (`redesign_window_frame.cpp`) keep the system's frame.
- **Menu bar**: a minimal one at the top of the screen (`qt.cpp`): the application menu gets *About*, *Preferences…*
  (⌘,) and *Quit* by their roles, a *Media* menu opens a file or a stream, *Help* the help. Its items have no
  shortcuts of their own, so the QML shortcuts are not made ambiguous. The in-window menu bar (`qt-menubar`) is
  unchanged.
- **Command key**: Qt calls ⌘ `Control` and ⌃ `Meta` on macOS. The QML and menu shortcuts written `Ctrl+…`
  therefore already mean ⌘ (no change needed), but VLC's own hotkeys of macOS are written `Command+…`
  (`src/libvlc-module.c`), and the converter turned ⌘ into `Ctrl`: no ⌘ hotkey of VLC worked with the Qt interface.
  Now ⌘ is `Command` and ⌃ is `Ctrl` (`vlchotkeyconverter.cpp`); the onboarding shows ⌘ and "⌘L" for the queue.
  Not changed: `DialogsProvider::sendKey` builds a `QKeySequence` from VLC's text, which cannot read `Command+`.
- **Fonts**: Inter is in the interface's resources on every system. The fallback fonts of the CJK languages follow
  the interface language (PingFang SC/TC, Hiragino Sans, Apple SD Gothic Neo), as on Windows; Core Text finds the
  other scripts.
- **Updater**: it never takes a Windows installer or small update outside Windows, and on macOS a release counts as
  an update only when it has this Mac's disk image; *Download* opens the release page (nothing is installed from
  the app; Sparkle or an in-app installer later).

Checked, nothing to change:

- Every Windows-only part of the redesign is behind `_WIN32` with a fallback that compiles (the Linux build proves
  it): `bin/winvlc.c` (portable profiles, settings carry-over, the language before libvlc, the start-up messages —
  macOS starts through `darwinvlc.m`, the interface language is applied in `qt.cpp` with `vlc_settextlanguage`),
  `redesign_window_frame.cpp` (the classic windows keep the system's frame on macOS), the DirectComposition and
  acrylic compositor, the WASAPI volume, the update helper.
- No X11 or Wayland code outside `QT_HAS_X11`/`QT_HAS_WAYLAND_COMPOSITOR`; `bin/vlc.c`'s `mallopt` is glibc only.
- `Qt.platform.os === "windows"` checks in QML fall back to the generic path.

## 7. Still to do on a Mac

- ✅ The first green build of each architecture (28 October 2026). `continue-on-error` off once the 0.4.0 checks
  pass on the public repository.
- 🟡 The unified title bar on a real Mac: the look in the smoke screenshots, then by hand: clicks on the top bar's
  buttons under the title bar's band, dragging, double-click to zoom, full screen and back, a saved geometry that
  stays the same over restarts, always on top.
- 🔴 Now Playing and media keys (`MPRemoteCommandCenter`, `MPNowPlayingInfoCenter`): the `macosx` interface does it,
  the Qt interface has nothing on macOS (Windows has its SMTC).
- 🔴 The video in the window (CompositorPlatform's `NSView`), VideoToolbox decoding, HDR, AirPlay audio, the
  audio devices of `auhal`.
- 🔴 The dock menu, opening files from Finder while running (`QFileOpenEvent`), drag and drop on the dock icon.
- 🔴 Signed and notarized releases (decision 3), Sparkle or an in-app update.
- 🔴 Pixel check of every frame (Retina: the 2× assets), dark and light with the system appearance (the smoke
  test's screenshots `main-light.png`, `main-dark.png`, `about.png` are the first look).

## 8. What the first CI run will likely hit

1. **Qt's configure checks** (`static_dirs.py`, `check_qml_module.py`) have never run against Qt frameworks in VLC's
   CI: the flags of a framework build (`-F… -framework QtCore`) may need a fix there.
2. **Code of macOS never compiled**: the redesign's `Q_OS_MACOS` branches above, and upstream's Qt-on-macOS code
   paths that the redesign touched (`menus.cpp`, `qt.cpp`).
3. **`macdeployqt`** with a dylib as `-executable`, and the QML import scan of `modules/gui/qt` (its own modules are
   not on disk): Qt Quick may be missing, which the script checks.
4. **The deployment target**: the contribs target macOS 10.13, VLC and Qt 12.0: link warnings, possibly an
   availability error in the redesign's C++.
5. **The launch test**: GitHub's Macs have a display but no GPU for Metal on the Apple Silicon ones; Qt Quick may
   fall back to software or fail (the playback step is reported only). The Qt interface may also stop at the
   first-run onboarding without failing.
6. **The runners**: `macos-14` (Apple Silicon) and `macos-15-intel` (GitHub retired `macos-13` in December 2025;
   its Intel image is now `macos-15-intel`, available until 2027).
