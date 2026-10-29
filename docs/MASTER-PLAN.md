# VLC Redesign: master plan

What is finished, what is left, and in which order, for every program, platform and screen of VLC Redesign.
It is the source for the [roadmap](../ROADMAP.md), the [README](../README.md), the wiki (with its *Design check*
page) and the release notes. Last updated: 2026-10-25, build 195 of the `redesign` branch.

**Status:** 🟢 done · 🟡 in progress or partly done · 🔴 to do or missing · ⚪ idea or research
**Pixel check:** 🟢 measured identical to the Figma frame · ⏳ not measured yet
**In the app ⏳:** merged and built, not yet checked in the running program (6 lines today)

## Now

What is being worked on, updated as it changes (2026-10-29):

- 🟡 **Release 0.4.0 prepared** (2026-10-29): VideoLAN's master merged to `9f393e5719` — **6 commits behind before**
  (HDR static metadata in the swap chain and VideoToolbox, the legacy MinGW check of `d3d11_surface` removed; its
  `CAN_PROCESSOR` guards dropped from our scaler and leak fix too, or they would have compiled out), **0 after**; the
  public `main` (PR #12) and the PR branches #11, #13, #20, #21, #22 merged into `redesign`, so the upload closes
  them; `po/vlc.pot` regenerated, 110 catalogs merged (`msgfmt -c` clean), `po/redesign` 1,991 texts × 72 at 100 %;
  `docs/wiki/Install-and-Update.md` was never in the repository (`.gitignore` `install-*`), now tracked; CHANGELOG
  dated, wiki and `platforms.yml` at 0.4.0. Left: the Windows build and tests of the merge, then make-release.

- 🟡 **Desktop polish before 0.4.0** (branch `feature/desktop-polish`, 2026-10-29):
  - 🟢 Icons at 125-200 % (`3673a7a74d`): the QML named them by their old PNG files; Qt 6 draws an image at the
    device pixel ratio only when the URL *as written* ends in `.svg` (the interceptor's `.svg` comes too late), so
    every icon was drawn for 100 % and stretched. Named by their SVG now. Queue icon strokes (coverage per row):
    200 % `19 57 76 76 57 19` → `50 100 100 50` (exact); 100 % unchanged `76 76` = Figma's 75/75 (`#737373` /
    `#4D4D4D`). The 1.9 px of the light-theme audit was the PNG build `g6da8c7cd1a` (before `f637ae1bfa`).
  - 🟡 At 125 and 150 % the strokes have the right width but sit on fractional device pixels (logical × 1.25/1.5):
    150 % `32 81 81 32` where whole pixels give `13 100 100 13`. Needs the icon snapped to device pixels (scene
    position, not bound in QML): open.
  - 🟢 DLNA live channels (`96287f0157`): a restarted picture goes on in the same stream (one stream per channel,
    was two); the first stream of a channel waits up to 3 s for its sound and picture (a cast on a playing channel
    sent both: PIDs 100 + 200, 3 of 3 runs). Fake DLNA TV, Pluto TV.
  - 🟡 A cast started on a playing live channel can stall the adaptive demuxer (seen once on the old build:
    `fakeesout restarting`, nothing sent for 60 s); not seen in 4 runs since. Pluto reports itself seekable, so
    restarting live inputs on a renderer change was dropped (untested path).

- 🟢 **One app icon everywhere: the designer's** (branch `feature/app-icon`, 2026-10-29; Android: its own
  `feature/app-icon`). The owner's rule: every program and app uses Juxtopposed's app icon. The design has no
  separate app-icon frame; its icon is the logo (the orange cone of the Icon set 42:356, Frame 4, also the
  onboarding's), `design/icons/svg/logo.svg` (the Figma export, its SHA-256 checked). Our old icon put that logo
  on a `#191919` card with a `#373737` line, which the design doesn't have: replaced by the logo as drawn.
  - 🟢 `extras/juxtopposed/make-app-icon.py` draws every size from the vector, hinting 16-40 px:
    `vlc.ico` (16, 20, 24, 32, 40, 48, 64, 96, 128, 256 PNG), `macos/AppIcon.icns` (16 ... 512@2x, copied by
    `macos-build.sh`; no more rsvg/sips), hicolor 16, 22, 24, 32, 48, 64, 128, 256, 512 + SVG (installed by
    `share/Makefile.am` and `linux-packages.sh`), the XPMs, the UPnP server's and the web interface's icons, the
    site's `favicon.ico` and touch icon. The installer's `setup.ico` and side mark (was a play glyph) are the
    same icon (`make-art.py` uses the script). The start page, player background and update notice draw the
    logo in place of upstream's cone; the seasonal cones are off (`qt-icon-change` kept, no effect).
  - 🟢 Checks: the vector is pixel-identical to the Figma export at 1024 and 2048; renders match Figma's own
    frames (onboarding 141 px, title bar 24 px) within 0.5/255; vlc.exe's compiled icon resource is
    byte-identical to `vlc.ico` (windres + MinGW of the build image). Contact sheets (dark/light, 100/150/200 %)
    in the QA folder `app-icon/`. A full Docker rebuild waits for disk space on the build drive.
- 🟡 **Issue [#10](https://github.com/KuchiSofts/vlc-redesign/issues/10)** (branch `feature/issue10`, both apps,
  2026-10-29): open state of the buttons (accent icon on an accent tint, no border, 120 ms; `IconToolButton.active`)
  on the player bar, the top bar and Live TV; Ctrl+wheel over the video sizes the subtitles (VLC's steps, 25-500 %,
  toast), elsewhere zooms the interface (new shortcuts `qt-key-intf-zoom-in/out`), Ctrl+0 through the interface,
  Shortcuts takes wheel turns, libass follows the size live. 4 texts in the 72 languages (msgfmt -c passes, the
  xgettext pot matches). Android: done and checked on the emulator (open state, Ctrl + wheel, Ctrl+0, slider,
  Hebrew; ASS keeps its size under libvlc 3). Desktop built in Docker and tested on Windows: Ctrl+wheel over the video 100 > 120 > 200 % with the toast, ASS follows live, over the player's library it zooms the interface (fixed: the library is drawn over the video), Ctrl+0 back to 100 %, Shortcuts shows both pairs and takes a Ctrl+wheel turn (conflict asked), open state on settings, tracks, the menu and the library button in dark, light, Hebrew at zoom 2; no new QML warning.

- 🟡 **macOS and Windows ARM64 ready for 0.4.0** (branch `feature/mac-arm-final`, 2026-10-29):
  - 🟢 macOS unified title bar in code (`maininterface/macos_titlebar.cpp`): the system frame kept, its title bar
    transparent and untitled, the content under it; the top bars start after the traffic lights; no redesign
    window buttons on macOS; `qt-titlebar` off by default there too. Compiled for macOS for syntax only (Qt 6.8.3
    headers, both `objc_msgSend` branches); 🔴 its first run on a Mac is the public CI at the upload.
  - 🟢 macOS app: Live TV add-on required (`-t` without), 20+ translations, README, licences, credits and the fonts'
    OFL in `Resources`, every Mach-O verified signed. `macos-smoke.sh` checks them plus the icon, the About badge,
    the Live TV plugin loaded and the unified title bar, and captures the main window light and dark and About.
  - 🟢 macOS CI: free on the public repository (every pull request and push to main); private only by hand, Apple
    Silicon only, 40 minutes at most (about 150-250 of the 2,000 monthly minutes); none before 1 November.
  - 🟢 Windows ARM64 checked statically (`ci/pe-check.py` on the integration build): 399 binaries all ARM64, every
    import in the package or in Windows (Vulkan's loader from the graphics driver, as on x64), Qt and its QML built
    in, About badge *Windows ARM64*, Live TV add-on, 100 translations, credits; 21 x64-only plugins are upstream's
    (OpenGL, NVDEC/QSV/AMF, x86 SIMD, projectM), 2 ARM64-only (NEON/SVE deinterlace). No 0.4.0 ARM64 installer
    built yet: the `.iss` installs the package's files plus the Live TV component.
  - 🟢 `install-test-arm64` job (public repository, `windows-11-arm`): the installer made and installed silently,
    every file checked, the ARM64 smoke test on the installed copy, screenshots of the main window and About,
    uninstall. 🔴 Runs first at the upload.
- 🟡 **Last desktop issues before 0.4.0** (branch `feature/leftovers`, 2026-10-29): Live TV's keys win over VLC's
  hotkeys while a channel plays (`MainCtx.keysBeforeHotkeys`; Page Up/Down, digits, Backspace, Enter, I; in
  Settings › Shortcuts, group *Live TV*); Backspace with an empty navigation history no longer ends VLC; the
  library panel of the player is made asynchronously (the `Player.qml:85` QML error); the Capture Device lists line
  up; untranslated texts keep their punctuation at their end in right-to-left (left-to-right marks, `.ui` texts
  through the translator). Checked on Windows with posted keys and with real `SendInput` keys in a test window in
  front (Page Up, Page Down, Backspace, digits, a number erased: 5/5 in English, Hebrew and with real keys); the
  real wheel changes the volume over the video and over the slider with the default output (1.0 → 1.1 → 1.0),
  Ctrl+T, Ctrl+P, Ctrl+N open their dialogs. Found in the end-to-end pass and fixed: the guide's
  `original-air-date` shown as the episode (Pluto TV), a QML error when *Open Folder* opened its system dialog from
  the menu (zoom 2). The two items left there are done in `feature/integration` (below). End-to-end pass of 0.4.0:
  [0.4.0-ui-audit.md](qa/0.4.0-ui-audit.md).
- 🟡 **Integration of 0.4.0** (branch `feature/integration`, 2026-10-29, every feature branch merged):
  - 🟢 The `QQuickItem::stackAfter` warning of Customize Player at zoom 1.5/2 came from the bar's customize lines
    (Repeater delegates that reparented themselves), not from `DashedRect`; fixed, none in the zoom 1.5 and 2 tours.
  - 🟢 Translations: `po/vlc.pot` had lost the 375 texts of the `.ui` forms (the classic windows read English in
    every language: *Capture mode*, *Device Selection*, *Show more options*...), and 37 files of the redesign were
    missing from `po/POTFILES.in` (Live TV, Settings › TV, the redesigned dialogs, Edit Sidebar). Both fixed
    (`translations/ui-pot.py`, a release-check test); the 207 new texts translated in the 72 languages, `msgfmt -c`
    clean in all 110 catalogs. `po/redesign` waits for the Weblate export.
  - 🟢 Builds: Windows x64, Windows ARM64 (built, not run) and Linux x86_64 (Qt 6.8.3). Windows x64 checked end to
    end in a portable test copy (tours in English and Hebrew, dark and light, Settings, zoom 1.5/2, About with its
    badge and the designer credit, the TV page with Pluto TV and Plex, a Windows-1255 Hebrew subtitle): no QML
    warning. Linux under Xvfb: start, Home, TV, Settings, About, a clip, quit, no QML warning. macOS not built.
    Details: [0.4.0-ui-audit.md](qa/0.4.0-ui-audit.md#integration-build-branch-featureintegration-2026-10-29).

- 🟡 **Upstream and the audit's leftovers** (branch `feature/upstream-final`, 2026-10-29): VideoLAN's master
  merged to `12023233ef` — **4 commits behind before** (`32b160d9e9` → `e0baa29690` mpg123 1.33.7 and its ARM64
  FPU patch, then `12023233ef` Qt moc include dirs and a medialibrary header), **0 after**, no conflict, nothing of
  ours changed. Fixed: Live TV's *All* counted the hidden adult channels (5,891 vs 5,881); the automatic online
  subtitles load by themselves only for a hash match or a sure title (`silent.mp4` → *Silent Hill* is now only
  listed); a channel address that is a playlist (Plex catch-up parts) left a library entry named by its token
  (`src/playlist/preparse.c`); accessibility (UI Automation sweep: every control named but the Settings search,
  dialogs named by their title, Tab out of multi-line fields); the 303 PNG icons removed (every one had its SVG of
  the same size; the originals stay in `design/icons`); the GPL source archive in `make-release.ps1`
  ([SOURCE.md](SOURCE.md)); the 0.4.0 release notes drafted (`docs/release/0.4.0`). Proven on the Windows x64
  build: the 7 UI tours, the IPTV parser test (Qt 6.8.3, Docker: PASS), the subtitle corpus (212/212), the
  pause/seek regressions (0 late, 0 dropped, clean quits), the border sweep, right-to-left captures.
  Details: [0.4.0-ui-audit.md](qa/0.4.0-ui-audit.md#upstream-merge-and-leftovers-branch-featureupstream-final-2026-10-29).

- 🟡 **Subtitle quality** (branch `feature/subs-quality`, for 0.4.0): measured first on Windows (d3d11, 480p,
  720p and 1080p sources in a 1440×1000 window and full screen 2560×1440): SRT and ASS were already rasterised at
  the display size (the same crop whatever the source size), picture subtitles were blocky with the point,
  processor and super resolution upscalers (fixed: a sampler of their own). Encodings: a detector for 28
  encodings (`modules/codec/subs_charset.c`) in the subtitle and WebVTT demuxes, subsdec and the seek bar's cue
  reader, 212/212 corpus files right (glibc and GNU libiconv); in VLC, legacy SRT, ASS and a Matroska track
  in Windows-1255 read right with an English interface. ASS right-to-left lines follow the Unicode bidi algorithm
  (libass base direction from the text). Fonts: every script of a mixed test file drew with the system fallback
  (no empty boxes; emoji in black and white). CPU while playing 1080p with SRT or ASS: 2-5 % of one core, as
  before. Left: Linux and macOS outputs only read in the code
  (OpenGL samples subpictures bilinearly, the macOS sample-buffer display draws them in backing pixels), HiDPI
  above 100 % not run (one 100 % screen here), Android (separate code base) listed in the report.

- 🟢 **macOS builds and runs** (2026-10-29): Apple Silicon (macos-14) and Intel (macos-15-intel) both build, make
  their `.dmg` and pass the 9 launch checks (signature, headless playback, the Qt interface loaded and playing,
  no QML warning, clean quit, no crash report); a screenshot shows the library on macOS. Seven fixes on the way
  (workflow env, configure checks, ncurses, prebuilt SDK paths, Qt SQL drivers, signing of the plugin cache and
  other non-code files, an updater `#else`). Left: the unified title bar (done in code in `feature/mac-arm-final`,
  above) run on a Mac; the Intel job needs more than 75 minutes.
- 🟡 **Window borders** (branch `feature/borders`): every QML dialog, menu, panel, dropdown list, tooltip and card
  over the interface draws the main window's 1 px frame (`Widgets.RedesignWindowBorder`, token
  `color.dark.windowBorder` / `color.light.windowBorder`, `#4C4A48`: what Windows 11 drew around the main window in
  both themes), inside its outline and over a title bar that reaches the edges; the main window, the classic
  windows, the QMenus and the "could not start" window get the same colour as DWMWA_BORDER_COLOR
  (`RedesignMenuStyle::windowBorder`). Where the system draws no frame (Linux X11 and Wayland, macOS with the
  interface's own title bar) the main window draws it itself (`MainInterface.qml`) and the widget menus paint it
  (`PE_FrameMenu`, also on Windows 10). Checked 2026-10-29: Windows build (main window, main menu, sort menu, About,
  Keyboard Shortcuts, Settings: `#4C4A48` in dark and light); Linux Qt 6.8 build under Xvfb/X11 (main window on
  all four sides, main menu, sort menu, tooltips, the welcome card, dark and light). Not run: Wayland (same QML
  path), macOS (CI only). Merged; build 13: the ⋯ menu, Network stream, Keyboard Shortcuts, Customize Player,
  Settings and About framed on all four sides.
- 🟡 **Live TV** (branch `feature/iptv`): an add-on of its own, `modules/misc/iptv` (`libiptv_plugin`, QtCore,
  capability `iptv`, the JSON API of `include/vlc_iptv.h`): M3U, Xtream Codes and XMLTV readers, downloads through
  VLC's access modules, the cache on disk, the guide's descriptions kept on disk, catch-up addresses, the catalog
  (built in, a newer one daily). The interface (`modules/gui/qt/iptv`) only looks the plugin up
  (`module_exists`): the TV page (channels, guide, films and series), the sources dialog with the catalog, the
  player's banner, number entry and channel list, Settings > TV. `iptv-enabled` on by default; the installer's
  *Live TV (IPTV) add-on* component leaves the plugin out. A channel zapped makes no library entry
  (`src/player/medialib.c`: no external media for a `no-save-recentplay` item). Test:
  `extras/juxtopposed/iptv-parsers.cpp`; the catalog (118 playlists, 157 guides, verified 2026-10-28). Merged into
  `redesign` 2026-10-29. Left: the owner's "no sound" report (not reproduced), translations, light/RTL captures.

- 🟡 **Final check before 0.4.0** (2026-10-28): VideoLAN's master merged to its latest then (`32b160d9e9`; now
  `12023233ef`, above), `feature/linux-pkg` and
  `feature/macos` merged; the redrawn tool windows looked at in build 10 (Media Statistics did not load: Canvas,
  fixed; Folder information: native path, no time-zone name); the Visualizer opens from Audio › Visualizations and its
  button over a visualization. Build 11 found Media Statistics still not loading (its graph's `top` clashed with
  Qt Quick's own); build 12 checked: Statistics with its live graph, Folder information, no QML warning.
  Every upload now passes `release-check.py` ([RELEASE-CHECKLIST.md](RELEASE-CHECKLIST.md)). The new texts are
  in the 72 languages (1,790 Weblate texts, all at 100 %); the check passes 22 of 22. Left: the upload.
- 🟡 **macOS** (roadmap step 7, branch `feature/macos`): `macos-build.sh` (prebuilt contribs, Qt 6.8.3, the Qt
  interface only, `VLC Redesign.app`, `vlc-redesign-X.Y.Z-macos-<arch>.dmg`), `build-macos.yml` on Apple Silicon and
  Intel runners with a launch test, macOS fixes in the code (title bar, menu bar, ⌘ hotkeys, fonts, updater)
  ([§9](#macos), `extras/juxtopposed/macos/README.md`). Merged. Left: push, the first CI run and its fixes, decision 3.
- 🟡 **Windows on ARM64** (branch `feature/winarm64`): the package builds (`win64-docker.ps1 -Arch aarch64`, every
  binary ARM64), `make-release.ps1 -Arch aarch64` makes the installer, zip and small updates, the updater keeps
  each architecture to its own files ([§9](#windows-on-arm64)). Left: merge, the ARM64 smoke test of the Windows
  build workflow (`windows-11-arm`), a run on a real ARM64 PC.
- 🟡 **Platforms and website** (roadmap step 5, branch `feature/platforms`): `platforms.yml` rewritten as the single
  source (status, version, date, next, download files, page), `extras/juxtopposed/platforms/render.py` renders it
  into the README's platform bar and table, `site/` (the website: OS detection, every platform, screenshots, the
  design credit), the *Also on* line of the release notes and of About; the Platforms workflow deploys the site.
  Left: merge, enable GitHub Pages (Source: GitHub Actions), check About in a build.
- 🟡 **Roadmap step 6, design tokens** (branch `feature/tokens`): `design/` exported (`design-1.0`: tokens.json,
  icons, fonts, CSS/Android/Swift files) with `design/tools/check_qml.py` in CI, passing. The export found
  differences between §6 and the code (listed in §6); they wait for a decision. Settled 2026-10-28 (branch
  `feature/light`): the light values the colour map overrode (228 calls, [design/light-theme.md](../design/light-theme.md))
  and the classic dialogs' light title bar (`#DEDEDE`, as the QML dialogs).

- 🟡 **0.4.0** (was 0.3.2; renumbered 2026-10-29 for the size of the release): the open issues. [#9](https://github.com/KuchiSofts/vlc-redesign/issues/9): the playback speed and
  teletext buttons never loaded on Windows (`Popup` without its import), so dropped on a bar they left an
  invisible slot; fixed, and a control that fails to load shows a placeholder that can be dragged out.
  [#8](https://github.com/KuchiSofts/vlc-redesign/issues/8): VLC master merged (110 commits; conflicts in mmdevice,
  MainCtx, PIPPlayer resolved). Both checked in the build: the speed button shows "1x" in the bar and dropped from
  Customize Player it lands framed and removable; no QML warning in a full UI tour.
- 🟢 **The review report (105 bugs)**: checked one by one and fixed in four groups (installer and tooling, core,
  Qt C++, QML); see the changelog. Checked in the build: SD colours (red 254,0,0 instead of 239,4,0; HD and HDR
  unchanged), the package check, a UI tour without warnings. Still to test by hand: casting on the real test TV, the
  all-users installer, an update from 0.3.1, the proxy exceptions, Linux builds.
- 🟡 **Weblate**: every language has all 1,723 texts again (7,093 translated, `msgfmt -c` clean in all 110
  catalogs), about 80 wrong catalog translations fixed, the volunteers' Estonian and Swedish merged, 12 texts that
  stay the same in every language flagged `ignore-same`. The lock of 2026-10-27 (Weblate couldn't push: no
  credentials) is gone: the component now goes through the Hosted Weblate GitHub App (vlc-redesign only) and is
  open again. Left: the push of `main`, from which Weblate reads the new files.
- 🟢 **0.3.1, a small update** (build 215, released 2026-10-25): what follows, for the copies of 0.2.4 and later through
  the in-app update (only the changed files).
- 🟢 **Bar panels open above their button**: Effects, VLC Settings and HDR now open above the bar, aligned to
  their button, exactly as the languages and tracks: a click outside or on the button closes them, the video
  doesn't pause or dim (BarPanel, any button of Customize can use it). Checked on build 208: open, close by the
  button, close by a click outside, one after another, the playback state never changed.
- 🟢 **Subtitle languages that learn**: each pick counts (fading x 0.9 a pick), the list orders the picked languages
  first ("Often used"), and a language picked ~3 times becomes the third automatic one. Checked on build 208: after
  German picks, German moved up, was marked, VLC's sub-language became `eng,deu`, and a video without English
  opened with its German track by itself.
- 🟢 **Setup's Languages page**: interface language (English, British English and the 72 others) and default subtitle language, the right-to-left toggle
  for a right-to-left language; `/UILANG=` `/SUBLANG=`. Checked in the wizard (Hebrew: the subtitles followed, the
  toggle showed), cancelled before installing.
- 🟢 **"Subtitl…" fixed**: the tab names of the tracks panel are no longer cut short (build 205).
- 🟢 **README**: a 56 s tour of the smart features (HyperFrames, from real captures; animated WebP + MP4), twelve
  feature pictures, the platforms; the wiki documents every option (33 pages).
- 🟢 **The crash at shutdown on GitHub's runner, found and fixed (2026-10-29, PR #21)**: the launch test ended with
  `0xC000041D` in 4 of the last 5 runs. The dumps showed a `WM_NCHITTEST` reaching `CSDWin32EventHandler` (the title
  bar's native event filter, upstream code) after `MainCtx` and its CSD button model were deleted: the handler is a
  child of the window, which outlives the interface. The runner's mouse rests over the window; here it never did,
  so 110 local runs passed. Named by matching the stripped CI binary against a local unstripped build (the static Qt
  code is identical once call targets are masked). Reproduced on a hidden desktop by sending hit tests while VLC
  quits: 7 of 8 crashed before, 0 of 6 after (`0xC000041D` or heap corruption `0xC0000374`). Fix: the handler is
  deleted with `InterfaceWindowHandlerWin32` and keeps `QPointer`s (here and on main). CI: full dumps instead of
  minidumps, and the symbol maps of the stripped binaries as an artifact (main only: this branch's workflow differs).
- 🟢 **Languages**: 72 languages and British English cover the whole interface: every language the list offers
  (the last 13 done on 2026-10-25; in build 197 and later). VLC's other catalogs follow as "(partial)".
- 🟡 **The direction comes with the language**: choosing a right-to-left language (in Settings, the classic
  settings, or the installer) lays the interface out right to left at once, and *Layout direction* under it still
  changes it. Each catalog says its direction (the "layout direction" entry), so a new translation can be right to
  left. 🟢 Checked on build 196: Arabic chosen in Settings mirrors everything at once with *Right to left* selected,
  kept after a restart; British English then lays it out left to right (its texts: fixed in build 198).
- 🟡 **Translators**: a language file added to `locale\` appears in the language list by itself, and
  [TRANSLATING.md](TRANSLATING.md) explains how to translate, try and send a language. In build 196.
- 🟢 **Subtitles**: the player's subtitle panel gets *Position* and *Size* with − and + (Position in steps of 1 % of the
  picture). Both apply live and are kept. Settings' subtitle font, colours and size now reach the video already playing,
  from its next line; the font is picked in a list of the fonts installed (a field to type a name in before; build
  200). 🟢 Checked on build 196: +5 % and 130 % move and enlarge the subtitles on the video at once and
  are saved (that they came back at the next start was the profile bug below). The menu fades while they are moved
  (it covered them).
- 🟡 **Performance pass**: baseline of build 195 measured (start, menus, Settings, dialogs, the 3,000-song library,
  scrolling). Real costs: Settings froze 115–190 ms at each opening, Keyboard Shortcuts 125–150 ms, Customize 60–85 ms,
  the Tracks page 45–60 ms of layout as it opens. The Artists "stall" of the scroll test is the end of the list
  (nothing to draw), not a freeze. Timing lines in a probe build (never committed) showed where the time goes.
  - 🟢 The two dialogs read their options once as they open, not twice: Shortcuts 142 → 66 ms, Settings 155 → 134 ms
    (medians of A/B runs, build 196 against 197; memory the same).
  - 🟢 Keyboard Shortcuts sorted its rows translating the names of its groups at each comparison: ranked once, the
    dialog opens with a 20 ms freeze (b197: 67 ms, b196: 142 ms).
  - ⚪ Settings: making only the rows of the lists shown (by their visibility) changed nothing measurable (133 vs
    138 ms): reverted. Each part of the dialog timed alone (build 199): the whole dialog ~93 ms, its options model
    ~20, each of its four option lists made its own table of labels (~10 ms each), the rows of the two lists hidden
    on a page ~9 ms each.
  - 🟢 One table of labels for the four lists, and no rows for the hidden lists (an explicit condition): Settings
    112 → 92 ms freeze, 147 → 122 ms settled, 3 MB less (b199 → b200). Full report: [PERFORMANCE.md](PERFORMANCE.md).
- 🟢 **Performance and memory pass 2 (2026-10-29, Live TV, soaks, CPU)**, details in [PERFORMANCE.md](PERFORMANCE.md):
  - 🟢 Live TV guides read once into an index: guide ready 0.5–0.6 s after a start instead of 3.9 (216 MB guide;
    5,891 public channels: 3.8 → 0.6 s), the start's CPU 4.9 → 1.1 s; the description file left at every start
    (13 MB a guide) deleted; sources refreshed together (each guide read once, 6 → 4 readings on a cold start of
    two sources); unreachable logos asked again after an hour.
  - 🟢 The guide crashed VLC moving through the hours (3 of 3 runs): the answers' JavaScript callbacks were copied
    and freed on the worker thread. Fixed; the guide asks for its rows at once, keeps its cells and draws them
    without a clip each (16 moves: 5–6 → 3.4 s of CPU, freeze 220 → 70 ms; scrolling: render thread −25 %).
  - 🟢 Soaks flat: 1 h of playback, 300 channel switches, 800 dialog openings (levels off at +5 MB), 203 page
    switches, 40 theme switches (memory, handles, GDI/USER, threads). Idle CPU ~0 % (0.08–0.23 %); paused 0.9 %
    is upstream's 80 ms redisplay (left). Zapping ~0.95 s is the provider's network (network caching changes nothing).
  - ⚪ Open: Page Up/Down (and the digits) of the TV banner may be taken by VLC's hotkeys while a channel plays
    (seen with posted keys only); the guide's fast scroll still keeps the render thread busy.
- 🟢 **Casting (2026-10-25, build 202, fake Chromecast)**: a click on the casting screen plays or pauses (it did
  nothing), a double click toggles fullscreen, the wheel changes the volume (it did nothing); a subtitle changed
  while casting is noted in the top corner where reading starts ("Subtitles on the TV: …", 72 languages). Taking the
  subtitles off no longer makes the TV load the stream again (Chromecast; DLNA already kept it), a subtitle added to
  a converted video goes into the running conversion, and the font, colours, outline, shadow, bold and position of
  the subtitles burnt into a cast follow Settings while the film plays. Limit: a burnt-in subtitle changed while the
  TV holds ~14 s of picture ahead is sent again from where the TV is (VLC's design), not shown instantly; subtitles
  as a separate text track (Chromecast) would switch at once: a larger piece of work, not started.
- 🟢 **Bug found while testing the direction**: choosing a language after another translated one (Arabic, then British
  English) kept the first one's texts; only the layout changed. Fixed (gettext's cache) and checked on build 199:
  Greek then Danish in one session, all of it in Danish.
- 🟡 **Settings kept across restarts and updates**: found while testing the subtitle panel. Every start applied the
  settings saved with the selected player profile over the ones in use, so what was changed outside Settings (the
  subtitle position and size from the player or Shift+Up, the classic preferences, a VLC ended unclosed) went back.
  Fixed and checked on build 199: values written to vlcrc stay after a clean quit, after VLC is killed, and after
  another start (before: all back to their defaults at the first start). Next: an installer update and a portable
  update on top of a changed profile.

## Dashboard

Each square is 10 %: 🟩 done · 🟨 in progress · ⬜ left; squares and percentages are rounded down. Every number is
counted from this document's own tables and checklists (column *From*); ⚪ ideas are not counted.

**Programs and platforms**

| Program         | Progress             | Done    | %     | From                                   |
|-----------------|----------------------|---------|-------|----------------------------------------|
| Windows (0.3.0) | 🟩🟩🟩🟩🟩🟩🟩🟩🟩⬜ | 91/97   | 93 %  | [§8](#8-next-release-030) checklists   |
| Linux           | 🟩⬜⬜⬜⬜⬜⬜⬜⬜⬜ | 1/7     | 14 %  | [§9](#linux)                           |
| macOS           | 🟩🟩🟩🟩⬜⬜⬜⬜⬜⬜ | 2/5     | 40 %  | [§9](#macos)                           |
| Windows ARM64   | 🟩🟩🟩🟩🟩⬜⬜⬜⬜⬜ | 2/4     | 50 %  | [§9](#windows-on-arm64)                |
| Android         | ⬜⬜⬜⬜⬜⬜⬜⬜⬜⬜ | 0/7     | 0 %   | [§9](#android-phones-tablets-tv)       |
| Samsung TV      | 🟩🟩⬜⬜⬜⬜⬜⬜⬜⬜ | 2/7     | 28 %  | [§9](#samsung-smart-tv-tizen)          |
| iPhone / iPad   | ⬜⬜⬜⬜⬜⬜⬜⬜⬜⬜ | 0/3     | 0 %   | [§9](#iphone-ipad-apple-tv)            |

Windows 0.3.0 was released on 2026-10-25 ([v0.3.0](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.3.0));
the bar counts the lines of section 8, the ones left need the owner (SignPath, Discussions) or hardware not here.

**Areas**

| Area                    | Progress             | Done    | %     | From                                   |
|-------------------------|----------------------|---------|-------|----------------------------------------|
| Design frames (Figma)   | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 | 103/103 | 100 % | [§5](#5-design-frame-by-frame) tables  |
| Pixel check             | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 | 103/103 | 100 % | [§5](#5-design-frame-by-frame) tables  |
| Light theme and accents | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 | 17/17   | 100 % | [§8.3](#83-light-theme-and-accents)    |
| Settings rows           | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 | 212/212 | 100 % | [§8.5](#85-settings-rows)              |
| Classic dialogs         | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 | 11/11   | 100 % | [§8.6](#86-classic-dialogs)            |
| Popups and menus        | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 | 15/15   | 100 % | [§8.4](#84-popups-and-menus)           |
| Casting                 | 🟩🟩🟩🟩🟩🟩🟩🟩🟩⬜ | 15/16   | 94 %  | [§7](#7-desktop-for-windows-what-is-done) + [§8.7](#87-casting) |
| Languages               | 🟩🟩🟩🟩🟩🟩🟩🟩🟩⬜ | 8/9     | 89 %  | [§12](#12-languages)                   |
| GitHub and community    | 🟩🟩🟩🟩🟩🟩⬜⬜⬜⬜ | 11/18   | 61 %  | [§13.16](#1316-setup-checklist)        |

**Contents**

1. [Principles](#1-principles)
2. [Programs and platforms](#2-programs-and-platforms)
3. [Where we are](#3-where-we-are)
4. [Roadmap sequence](#4-roadmap-sequence)
5. [Design, frame by frame](#5-design-frame-by-frame)
6. [Design tokens](#6-design-tokens)
7. [Desktop for Windows: what is done](#7-desktop-for-windows-what-is-done)
8. [Next release: 0.3.0](#8-next-release-030)
9. [Other platforms](#9-other-platforms)
10. [Other implementations of the design](#10-other-implementations-of-the-design)
11. [Pixel-exact process](#11-pixel-exact-process)
12. [Languages](#12-languages)
13. [GitHub: one project, many repositories](#13-github-one-project-many-repositories)
14. [Quality gates for every release](#14-quality-gates-for-every-release)
15. [Risks](#15-risks)
16. [Definition of done](#16-definition-of-done)
17. [Decisions for the project owner](#17-decisions-for-the-project-owner)

---

## 1. Principles

1. **The design, pixel for pixel.** Every screen follows Juxtopposed's
   [VLC Redesign (Community)](https://www.figma.com/community/file/1691465999134370980) Figma file 1:1: sizes,
   spacing, colours, fonts, icons, states. A screen is done when its capture matches the frame (section 11).
2. **The real VLC.** The same engine, codecs and formats. Only the interface changes; engine fixes stay small,
   are listed in the [changelog](../CHANGELOG.md#differences-from-upstream-vlc) and offered upstream.
3. **English first.** Every program starts in English; the language is chosen in the installer and in the settings.
4. **Smooth on every machine.** No crash, hang or stutter, flat memory, on old and new PCs, every graphics vendor,
   every supported OS version: every release passes the quality gates (section 14).
5. **Users shape it.** Ideas, votes and reports on GitHub set the order of the work; every report gets an answer.
   The program never sends anything by itself.
6. **Every upload brings the whole project up to date.** The changelog, the roadmap, this plan, the README, the
   platforms and the website, the wiki, the translations, VideoLAN's master, the credits and an answer for every
   issue: the pre-push hook and `make-release.ps1` refuse an upload that leaves one behind
   ([RELEASE-CHECKLIST.md](RELEASE-CHECKLIST.md)).
6. **Credit and honesty.** The design credit (CC BY 4.0) is in every program, README, store page and release. Every
   program says it is unofficial; the VLC name and cone are VideoLAN trademarks.
7. **Open source.** Every binary has its source, build scripts and licenses next to it.

## 2. Programs and platforms

| Program            | Platforms                                  | Code base                               | Status                      |
|--------------------|--------------------------------------------|-----------------------------------------|-----------------------------|
| Desktop            | Windows 10/11 x64                          | VLC 4 + Qt 6.8/QML (`vlc-redesign`)     | 🟢 0.2.6 · 🟡 0.3.0          |
| Desktop            | Windows 11 ARM64                           | same                                    | 🔴 track                     |
| Desktop            | Linux x86_64                               | same                                    | 🔴 track (built once before) |
| Desktop            | macOS 12+ (Apple Silicon, Intel)           | same                                    | 🔴 track                     |
| Android            | Android 8+ phones, tablets, Android TV     | fork of VLC for Android (Kotlin)        | ⚪ track                     |
| Casting            | DLNA TVs (Samsung, LG, Sony…), Chromecast  | part of the desktop program             | 🟢 works on a real TV        |
| TV receiver        | Samsung Tizen TVs (2017+)                  | Tizen web app                           | ⚪ track                     |
| iPhone, iPad, TV   | iOS, iPadOS, tvOS                          | fork of VLC for iOS                     | ⚪ track                     |

The desktop program has version numbers; each other platform gets its own when its first build is published
(section 13.4).

| Platform   | Minimum                                                    | Reason                                         |
|------------|------------------------------------------------------------|------------------------------------------------|
| Windows    | Windows 10 1809, x64                                       | Qt 6.8 and VLC 4 (an older Windows gets a message) |
| Linux      | glibc 2.38 (Ubuntu 24.04, Debian 13, Fedora 39+); Flatpak   | Qt 6.8 and the VLC 4 build image (Ubuntu 24.04) |
| macOS      | 12 Monterey                                                | Qt 6.8                                         |
| Android    | 8.0 (API 26)                                               | keeps blur and rounded clipping cheap          |
| Samsung TV | Tizen 3.0 (2017 models, such as the UE55MU8000)             | web apps with the AVPlay player                |

## 3. Where we are

| Version | Date              | Highlights                                                                                  |
|---------|-------------------|---------------------------------------------------------------------------------------------|
| M0–M3   | 2026-10-12 to 13  | Linux build, first shell and player, library, playlists, subtitles, dialogs, settings (MrDouglax) |
| 0.1.0   | 2026-10-20        | First Windows release: Docker build, text rendered like Figma, three deadlocks fixed         |
| 0.2.0   | 2026-10-20        | Installer, updates from GitHub, About, about 80 audit fixes, VLC 4 merged, CI on every PR     |
| 0.2.1–2 | 2026-10-20        | Fullscreen fixes, fullscreen button in the bar (#1), drops between two controls              |
| 0.2.3   | 2026-10-20        | Installer drawn like the app, file associations, right-click entries, the wiki               |
| 0.2.4   | 2026-10-22        | Small (delta) updates, setup no longer stuck (#2), pause → seek → play freeze fixed          |
| 0.2.5   | 2026-10-22        | Seamless switching of the sound device (#3), English by default                              |
| 0.2.6   | 2026-10-22        | Menu check marks, the Visualizer, right-click on music, *Open Network Stream* links (#4)      |
| 0.3.0   | next              | Section 8                                                                                    |

**Verified so far**

- 🟢 Regression suite (playback, pause/seek, traces, Chromecast and DLNA test devices) on every build
- 🟢 Hardware: NVIDIA RTX 4090, AMD Radeon (integrated), no GPU (Windows Sandbox, 2 GB), weak CPU simulated
- 🟢 A real TV: Samsung UE55MU8000 (2017, Tizen): start, pause, as-is and converted casts
- 🔴 Intel graphics, older PCs, Windows 10 on real hardware, high-DPI multi-monitor, other TV brands

## 4. Roadmap sequence

What comes first, and why. Each step starts when what it needs is done.

| #  | Step                                                                  | Needs first                     | Why in this order                                                   | Status |
|----|-----------------------------------------------------------------------|---------------------------------|---------------------------------------------------------------------|--------|
| 1  | Finish the 0.3.x items (§8): the 6 lines in the app ⏳                 | —                               | Mostly done: finishing costs less than switching                    | 🟡     |
| 2  | Figma references exported, capture script, pixel check of 103 frames  | step 1 (screens stable)         | "Pixel for pixel" needs proof; capturing moving screens is wasted    | 🟢     |
| 3  | Community setup: in-app *Suggest an idea* / *Report a problem* (13.12), hub, Discussions, board | decisions 1 and 9 | Feedback must land in the right place before the announcement | 🟡 |
| 4  | Release 0.3.0 and announce it (0.3.0 and 0.3.1 out 2026-10-25; 0.4.0 next) | quality gates, steps 1 and 3 | Users get the fixes; the community side is ready for them        | 🟡     |
| 5  | `platforms.yml`, platform bar, website (built 2026-10-28; Pages to enable) | step 3                     | In place before a second platform exists, so it shows everywhere     | 🟡     |
| 6  | Design tokens exported (`design/`: tokens, icons, fonts) — `design-1.0` and its CI check in (2026-10-28); every icon an SVG from Figma, drawn sharp at every scale and zoom (2026-10-29); left: the sync workflow to the sister repositories, the §6 differences | step 2 (values checked) | Every other platform reads them; no copying errors | 🟡 |
| 7  | Linux, then macOS, then Windows ARM64 (macOS started 2026-10-28: build script and CI, first run after the push) | CI; decision 3 for macOS        | Same code base: the cheapest new platforms first                     | 🟡     |
| 8  | Android                                                               | step 6, decision 6              | The largest new audience (phones and Android TV)                     | 🔴     |
| 9  | Samsung TV receiver                                                   | step 6, decision 5              | Builds on the casting work and the real-TV testing                   | 🔴     |
| 10 | iPhone, iPad, Apple TV                                                | step 8, decision 3              | Same approach as Android, reusing its screens                        | 🔴     |

## 5. Design, frame by frame

### 5.1 How to read the tables

- **Source:** the Figma design file `fQSxoEi1zmnD5fQacw0qGs` (a copy of the community file, same node ids): 103
  top-level nodes, 57 screens and 46 component groups, with their texts in
  [`inventory.json`](../.specs/features/redesign/reference/inventory.json). A node opens with
  `https://www.figma.com/design/fQSxoEi1zmnD5fQacw0qGs?node-id=<id, with - instead of :>`.
- **Size:** the node's width × height in Figma pixels (1 px = 1 app pixel at 100 %).
- **Implemented in:** file names under [`modules/gui/qt/`](../modules/gui/qt) (each name is unique there), checked
  in the code on the date above.
- **Status:** 🟢 every element and state of the frame is implemented · 🟡 implemented with differences (listed under
  the table) · 🔴 not implemented.
- **Pixel:** 🟢 only where a capture was compared with the frame (section 11). Many frames have measured details in
  the code (baselines, sizes), but no full comparison yet.
- Screens without texts in Figma are described from the coverage notes
  ([`m3-coverage-player.md`](../.specs/features/redesign/m3-coverage-player.md)) and the code comments citing them.

### 5.2 Summary

| Area                  | Frames | 🟢  | 🟡  | 🔴  | Pixel 🟢 | Progress             |
|-----------------------|--------|-----|-----|-----|----------|----------------------|
| Onboarding            | 11     | 11  | 0   | 0   | 11       | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| Player                | 7      | 7   | 0   | 0   | 7        | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| Player - Music        | 2      | 2   | 0   | 0   | 2        | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| Player - customize    | 9      | 9   | 0   | 0   | 9        | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| Player - hide UI      | 1      | 1   | 0   | 0   | 1        | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| Player - Light        | 1      | 1   | 0   | 0   | 1        | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| Add Subtitles         | 4      | 4   | 0   | 0   | 4        | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| Keyboard Shortcuts    | 1      | 1   | 0   | 0   | 1        | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| Open network stream   | 1      | 1   | 0   | 0   | 1        | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| New playlist          | 1      | 1   | 0   | 0   | 1        | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| Settings pages        | 19     | 19  | 0   | 0   | 19       | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| **Screens**           | 57     | 57  | 0   | 0   | 57       | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| **Components**        | 46     | 46  | 0   | 0   | 46       | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |
| **All**               | 103    | 103 | 0   | 0   | 103      | 🟩🟩🟩🟩🟩🟩🟩🟩🟩🟩 |

### 5.3 Onboarding (11)

Shared: `OnboardingDialog.qml`, `OnboardingButton.qml`, `onboarding.cpp`.

| Node       | Name       | Size (px)   | Status | Pixel | What it is                                                        | Implemented in    |
|------------|------------|-------------|--------|-------|-------------------------------------------------------------------|-------------------|
| 1899:3055  | Onboarding | 1090 × 759  | 🟢     | 🟢    | Step 1, Welcome: title, description, media folder (Browse), Next   | WelcomeStep.qml   |
| 1902:3670  | Onboarding | 1090 × 759  | 🟢     | 🟢    | Step 2: Light / Dark / System over the small player, 9 accents     | ThemeStep.qml     |
| 1902:4020  | Onboarding | 1090 × 759  | 🟢     | 🟢    | Step 2, state 2 of 9: another theme or accent, control bar sample  | ThemeStep.qml     |
| 1902:4325  | Onboarding | 1090 × 759  | 🟢     | 🟢    | Step 2, state 3 of 9                                              | ThemeStep.qml     |
| 1902:4634  | Onboarding | 1090 × 759  | 🟢     | 🟢    | Step 2, state 4 of 9                                              | ThemeStep.qml     |
| 1902:4940  | Onboarding | 1090 × 759  | 🟢     | 🟢    | Step 2, state 5 of 9                                              | ThemeStep.qml     |
| 1902:5245  | Onboarding | 1090 × 759  | 🟢     | 🟢    | Step 2, state 6 of 9                                              | ThemeStep.qml     |
| 1902:5550  | Onboarding | 1090 × 759  | 🟢     | 🟢    | Step 2, state 7 of 9                                              | ThemeStep.qml     |
| 1902:6160  | Onboarding | 1090 × 759  | 🟢     | 🟢    | Step 2, state 8 of 9                                              | ThemeStep.qml     |
| 1902:6465  | Onboarding | 1090 × 759  | 🟢     | 🟢    | Step 2, state 9 of 9                                              | ThemeStep.qml     |
| 1902:6770  | Onboarding | 1090 × 759  | 🟢     | 🟢    | Last step: 12 shortcuts with DM Mono keycaps, Back, Finish         | ShortcutsStep.qml |

Notes: drawn light in the light theme (the dark frames unchanged). Beyond Figma: a subtitle languages step
(`SubtitleLanguagesStep.qml`), keycaps showing the user's real shortcuts, *Cancel* when reopened.

### 5.4 Player (7)

| Node       | Name   | Size (px)   | Status | Pixel | What it is                                                         | Implemented in                                                    |
|------------|--------|-------------|--------|-------|--------------------------------------------------------------------|-------------------------------------------------------------------|
| 187:328    | Player | 1920 × 1080 | 🟢     | 🟢    | Player with the compact library open beside the video              | Player.qml, TopBar.qml, ControlBar.qml, CompactLibrary.qml        |
| 247:445    | Player | 1920 × 1080 | 🟢     | 🟢    | Player alone: the video framed between the bars                    | Player.qml, TopBar.qml, ControlBar.qml                            |
| 970:2703   | Player | 1920 × 1080 | 🟢     | 🟢    | Library shell: sidebar, address bar, a folder's grid, mini player  | MainDisplay.qml, SideNavigationPane.qml, LibraryHeader.qml, VideoGridDisplay.qml |
| 970:3308   | Player | 1920 × 1080 | 🟢     | 🟢    | Library: video grid, the mini player playing                       | VideoGridDisplay.qml, VideoGridItem.qml, LibraryMiniPlayer.qml    |
| 970:3792   | Player | 1920 × 1080 | 🟢     | 🟢    | Library: a folder as a list (rows 406 × 32, 10 apart)               | FolderDisplay.qml, LibraryMiniPlayer.qml                          |
| 970:4028   | Player | 1920 × 1080 | 🟢     | 🟢    | Library: a folder as cards, the Videos icon filled                  | FolderDisplay.qml, SideNavigationPane.qml                         |
| 6261:15078 | Player | 1920 × 1080 | 🟢     | 🟢    | Library Home: Continue Watching, Listen Again, Frequently Visited  | HomeDisplay.qml, HomePage.qml, LibraryMiniPlayer.qml              |

Notes: when nothing plays, the video area shows the frames' pixel cat (`IdleScene.qml`): 17 × 17 blocks of
24.0625 px, wall `#2D2B29`, floor gradient `#272523` → `#2D2B29` from y 785, the cat `#FFFFFF`, `#E0E0E0`,
`#000000`, `#FFD6E4`, drawn as blocks (sharp at any size, no texture).

### 5.5 Player - Music (2)

| Node       | Name           | Size (px)   | Status | Pixel | What it is                                                     | Implemented in                                                       |
|------------|----------------|-------------|--------|-------|----------------------------------------------------------------|----------------------------------------------------------------------|
| 4767:23726 | Player - Music | 1920 × 1080 | 🟢     | 🟢    | Music: artwork (738 × 737) over its blur, waveform, resume card | MusicView.qml, AudioContent.qml, WaveformSlider.qml, ResumeDialog.qml |
| 4778:25123 | Player - Music | 1920 × 1080 | 🟢     | 🟢    | The same with the queue                                        | as above, QueueCard.qml                                              |

Notes: the resume card reads as in Figma: "You've watched till 00:45:08 already." (the media library's saved time)
and one *Continue Watching* button; Escape or 10 s close it (checked in the app 2026-10-24). The bars are solid `#191919` at 95 %
over the artwork backdrop: the design's blur would change them by at most 5 % (5.16, item 6).

### 5.6 Player - customize (9)

Shared: `ToolbarEditorDialog.qml` (the 1048 × 655 panel), `ToolbarEditorButtonList.qml`, `EditorDNDView.qml`,
`EditorDNDDelegate.qml`, `CustomizeDragArea.qml`, `DashedRect.qml`, `controlbar_profile_model.cpp`.

| Node       | Name               | Size (px)   | Status | Pixel | What it is                                                  | Implemented in                                   |
|------------|--------------------|-------------|--------|-------|-------------------------------------------------------------|--------------------------------------------------|
| 2107:5604  | Player - customize | 1920 × 1080 | 🟢     | 🟢    | Customize Player open: profile picker, palette, Cancel, Save | shared                                           |
| 2160:6137  | Player - customize | 1920 × 1080 | 🟢     | 🟢    | The timeline dragged to a row of its own                     | ControlBar.qml, ControlRepeater.qml              |
| 2160:6564  | Player - customize | 1920 × 1080 | 🟢     | 🟢    | A control dragged from the palette onto the bar              | shared, ControlBar.qml                           |
| 2160:7005  | Player - customize | 1920 × 1080 | 🟢     | 🟢    | Controls moved along the bar                                 | shared                                           |
| 2160:7437  | Player - customize | 1920 × 1080 | 🟢     | 🟢    | Library toggle and ⋯ menu moved into the bar                  | LibraryButton.qml, MainMenuButton.qml            |
| 2160:7869  | Player - customize | 1920 × 1080 | 🟢     | 🟢    | The edited bar, ready to save                                | shared                                           |
| 6587:19923 | Player - customize | 1920 × 1080 | 🟢     | 🟢    | Another profile being edited                                 | shared, ProfilesMenu.qml                         |
| 2160:8301  | Player - customize | 1920 × 1080 | 🟢     | 🟢    | The player after editing: the active profile's bar           | ControlBar.qml                                   |
| 2160:8732  | Player - customize | 1920 × 1080 | 🟢     | 🟢    | Saved layout: file name in the bar, two rows, library left   | FileNameWidget.qml, MetadataText.qml, CompactLibrary.qml |

Notes: the bars are compared with a profile of the frame written in the settings (the video area is the demo's 16:9
picture, letterboxed where Figma stretches it); the fullscreen button stays in the bar (issue #1) where these frames
put it in the top bar. Editing the music bar is verified (§8.8).

### 5.7 Player - hide UI (1)

| Node       | Name             | Size (px)   | Status | Pixel | What it is                                             | Implemented in                          |
|------------|------------------|-------------|--------|-------|--------------------------------------------------------|-----------------------------------------|
| 3328:11554 | Player - hide UI | 1920 × 1080 | 🟢     | 🟢    | The video alone in its 10 px frame (Ctrl+\\, right-click) | HideUIAction.qml, MinimalView.qml       |

### 5.8 Player - Light (1)

| Node       | Name           | Size (px)   | Status | Pixel | What it is                                          | Implemented in                                     |
|------------|----------------|-------------|--------|-------|-----------------------------------------------------|----------------------------------------------------|
| 2425:13624 | Player - Light | 1920 × 1080 | 🟢     | 🟢    | The light theme, greys measured from this frame      | VLCStyle.qml (`redesign.color()`), redesign_icon_provider.cpp |

Notes: the whole app was checked in light in build 115 (player, tracks and subtitles window, cast window, ⋯ menu,
classic dialogs). 2026-10-28: the light values the colour map overrode settled
([design/light-theme.md](../design/light-theme.md)); the frame against the app per region within 2.4 per channel;
dark unchanged (33 captures, no pixel over 1 of 255). The last fixes, Settings scroll bar handles and the warning colour, are in the app ⏳ (§8.3).

### 5.9 Add Subtitles (4)

Shared: `SubtitleAssociationsDialog.qml` (1048 × 800, `#0E0E0E` at 95 %), `SubtitleSourceControls.qml`,
`RedesignDialog.qml`. OpenSubtitles needs the user's own API key.

| Node       | Name                             | Size (px)   | Status | Pixel | What it is                                       | Implemented in          |
|------------|----------------------------------|-------------|--------|-------|--------------------------------------------------|-------------------------|
| 1911:7545  | Add Subtitles to Playlist/Folder | 1048 × 800  | 🟢     | 🟢    | To a folder, from the computer: a row per video   | shared                  |
| 3336:11929 | Add Subtitles to Playlist/Folder | 1048 × 800  | 🟢     | 🟢    | The same, several subtitles per video             | shared                  |
| 1919:8707  | Add Subtitles to Playlist/Folder | 1048 × 800  | 🟢     | 🟢    | One subtitle for the folder, origin dropdown      | shared                  |
| 1919:9278  | Add Subtitles to Playlist/Folder | 1048 × 800  | 🟢     | 🟢    | To one file, from OpenSubtitles, English          | shared (SubtitleAssociationsDialog.qml) |

### 5.10 Keyboard Shortcuts (1)

| Node       | Name               | Size (px)   | Status | Pixel | What it is                                                 | Implemented in                                              |
|------------|--------------------|-------------|--------|-------|------------------------------------------------------------|-------------------------------------------------------------|
| 2437:14832 | Keyboard Shortcuts | 1048 × 800  | 🟢     | 🟢    | Search, Action / Hotkey / Global Hotkey by group, Save      | ShortcutsDialog.qml, ShortcutsCapturePopup.qml, shortcuts_model.cpp |

### 5.11 Open network stream (1)

| Node       | Name                | Size (px)   | Status | Pixel | What it is                                                 | Implemented in                               |
|------------|---------------------|-------------|--------|-------|------------------------------------------------------------|----------------------------------------------|
| 6216:13614 | Open network stream | 1048 × 631  | 🟢     | 🟢    | Network URL (several links), Playlist Name (optional), Save | NetworkStreamDialog.qml, RedesignTextArea.qml |

### 5.12 New playlist (1)

| Node       | Name         | Size (px)   | Status | Pixel | What it is                                                | Implemented in                                    |
|------------|--------------|-------------|--------|-------|-----------------------------------------------------------|---------------------------------------------------|
| 6259:14954 | new playlist | 713 × 589   | 🟢     | 🟢    | Title and thumbnail, Select Files (drop zone), Save        | PlaylistCreateDialog.qml, mlplaylistlistmodel.cpp |

Notes: a custom thumbnail, picked or dropped, is saved as a PNG of at most 1024 px in the user data folder and
deleted with the playlist; Save needs only a title, so a playlist can be empty (in the app ⏳).

### 5.13 Settings pages (19)

Shared: `SettingsDialog.qml` (1048 × 865), `SettingsSidebar.qml`, `SettingsOptionList.qml`, `SettingsOptionRow.qml`,
`SettingsPresentation.qml` (every label), `catalog.cpp` and `config_model.cpp` (options per page). Every frame's texts
were checked against the code: all rows are there but one (§8.5). The second frame of a page has *Advanced Options*
open. The design's typo "changew" reads "changes".

| Node       | Name                   | Size (px)   | Status | Pixel | What it is                                                     | Implemented in                    |
|------------|------------------------|-------------|--------|-------|----------------------------------------------------------------|-----------------------------------|
| 999:5047   | Settings - global      | 1048 × 865  | 🟢     | 🟢    | Appearance (language, theme, accent), Startup, Diagnostics      | SettingsAppearance.qml, SettingsLanguageAction.qml, SettingsDeveloperMode.qml |
| 3046:9282  | Settings - global      | 1048 × 865  | 🟢     | 🟢    | Advanced: warnings, command line, log, statistics, reset        | as above                          |
| 3046:10057 | Settings - interface   | 1048 × 865  | 🟢     | 🟢    | Skin, Window, Playback Controls, Notifications, Playlist        | shared                            |
| 3062:11379 | Settings - interface   | 1048 × 865  | 🟢     | 🟢    | Advanced: opacity, minimized, cover art, fullscreen controls     | shared                            |
| 3062:11911 | Settings - privacy     | 1048 × 865  | 🟢     | 🟢    | History, Network (cover art, Last.fm)                           | SettingsPrivacyActions.qml, SettingsLastfm.qml |
| 3062:12552 | Settings - playback    | 1048 × 865  | 🟢     | 🟢    | Startup, Tracks, Order, Seeking, Files & Discs                  | shared                            |
| 3072:13408 | Settings - playback    | 1048 × 865  | 🟢     | 🟢    | Advanced: time-shift buffer                                     | shared                            |
| 3073:14187 | Settings - video       | 1048 × 865  | 🟢     | 🟢    | Output, Picture, Deinterlacing, Framing, Snapshots              | SettingsPicture.qml               |
| 3073:15375 | Settings - video       | 1048 × 865  | 🟢     | 🟢    | Advanced: aspect and crop ratios, pixel aspect ratio            | shared                            |
| 3073:15879 | Settings - audio       | 1048 × 865  | 🟢     | 🟢    | Output, Volume, Loudness, Effects, Tracks, Visualization        | SettingsAudioOutput.qml, SettingsStartupVolume.qml, SettingsEqualizer.qml |
| 3180:17398 | Settings - audio       | 1048 × 865  | 🟢     | 🟢    | Advanced: pass-through, Dolby Surround, resampling, media role  | as above                          |
| 3180:17989 | Settings - subtitles   | 1048 × 865  | 🟢     | 🟢    | Languages, Loading, Appearance, Timing, On-Screen Messages      | SettingsSubtitleLanguages.qml     |
| 6536:13908 | Settings - subtitles   | 1048 × 865  | 🟢     | 🟢    | Advanced: folders, ASS/SSA styling, shadow angle, direction     | as above                          |
| 6536:14482 | Settings - controls    | 1048 × 865  | 🟢     | 🟢    | Shortcuts, skip amounts, mouse, web remote                      | SettingsActionRow.qml             |
| 6542:15251 | Settings - library     | 1048 × 865  | 🟢     | 🟢    | Opening files, folders, metadata, media library                 | shared                            |
| 6543:15649 | Settings - network     | 1048 × 865  | 🟢     | 🟢    | Buffering presets, proxy, certificates, ciphers                 | shared                            |
| 6571:15958 | Settings - performance | 1048 × 865  | 🟢     | 🟢    | Decoding, smooth playback, priority, plugins, developer         | shared                            |
| 6571:16429 | Settings - recording   | 1048 × 865  | 🟢     | 🟢    | Recording, Casting, Streaming                                   | shared                            |
| 6575:17137 | Settings - extensions  | 1048 × 865  | 🟢     | 🟢    | Add-ons list (784 × 194), find more, add, remove, on startup    | SettingsExtensions.qml            |

Differences: Interface (3046:10057, 3062:11379): *Show the playlist as a tree* is the queue under the folders of
its media (VLC 4's queue is flat: the folders are headings, the rows move as they did). Three of the four other
rows added in build 115 are checked in the app (§8.5).

### 5.14 Components (46)

**Icons and buttons**

| Node       | Name         | Size (px)  | Status | Pixel | What it is                                     | Implemented in                               |
|------------|--------------|------------|--------|-------|------------------------------------------------|----------------------------------------------|
| 42:356     | Icon         | 548 × 812  | 🟢     | 🟢    | The icon set: 96 names with their states        | pixmaps/redesign/ (icons, duo), redesign_icon_provider.cpp |
| 177:98     | Icon Wrapper | 72 × 122   | 🟢     | 🟢    | Icon button: default, hover                     | RedesignIconButton.qml                       |
| 500:3063   | Button       | 400 × 140  | 🟢     | 🟢    | Dialog button: neutral, accent                  | RedesignButton.qml, SettingsButton.qml       |
| 848:1712   | play         | 94 × 116   | 🟢     | 🟢    | Play, pause                                     | PlayButton.qml                               |
| 536:1014   | Toggle       | 120 × 194  | 🟢     | 🟢    | ON / OFF                                        | CheckButton.qml, SettingsToggle.qml          |

**Player controls**

| Node       | Name         | Size (px)      | Status | Pixel | What it is                                            | Implemented in                                  |
|------------|--------------|----------------|--------|-------|-------------------------------------------------------|-------------------------------------------------|
| 491:1780   | Controls     | 1960 × 11139   | 🟢     | 🟢    | Control bar and its 15 panel states (queue, tracks…)  | ControlBar.qml, QueueCard.qml, TracksMenu.qml, SubtitleTracksPanel.qml |
| 244:184    | volume       | 130 × 646      | 🟢     | 🟢    | Cone volume, 13 states from 0 to 200 %                | VolumeWidget.qml                                |
| 2245:9805  | audio value  | 121 × 37       | 🟢     | 🟢    | The value shown while the volume changes               | VolumeWidget.qml                                |
| 1904:7238  | Current Time | 114 × 164      | 🟢     | 🟢    | Elapsed time: default, hover, click                    | ControlBar.qml                                  |
| 1905:7312  | Duration     | 114 × 164      | 🟢     | 🟢    | Length: default, click (remaining time)                | ControlBar.qml                                  |
| 2151:9673  | thumbnail    | 272 × 189      | 🟢     | 🟢    | Seek bar frame preview with its time                   | SeekFramePreview.qml                            |
| 970:3091   | Miniplayer   | 431 × 2816.375 | 🟢     | 🟢    | Library mini player, 5 states                          | LibraryMiniPlayer.qml, MiniPlayerQueue.qml      |

**Library**

| Node       | Name            | Size (px)   | Status | Pixel | What it is                                          | Implemented in                               |
|------------|-----------------|-------------|--------|-------|-----------------------------------------------------|----------------------------------------------|
| 848:1996   | sidebar         | 4492 × 1028 | 🟢     | 🟢    | Sections column, 10 states                          | SideNavigationPane.qml, CompactLibrary.qml   |
| 856:2215   | Sorting Options | 265 × 776   | 🟢     | 🟢    | Sort menu                                           | LibraryHeader.qml, RedesignOptionItem.qml    |
| 858:3290   | View Options    | 265 × 104   | 🟢     | 🟢    | Grid or list                                        | LibraryHeader.qml, CompactLibrary.qml        |
| 2404:9167  | media           | 377 × 228   | 🟢     | 🟢    | Video card: watched, unwatched, hover of each       | VideoGridItem.qml                            |
| 2459:9352  | playlist        | 377 × 228   | 🟢     | 🟢    | Playlist card: unwatched, hover                     | PlaylistGridDisplay.qml, VideoGridItem.qml   |

Differences: none open (848:1996: the compact library's Home lists the sections of the library's Home, §5.16 item 3).

**Menus**

| Node       | Name                  | Size (px) | Status | Pixel | What it is                                       | Implemented in                                   |
|------------|-----------------------|-----------|--------|-------|--------------------------------------------------|--------------------------------------------------|
| 6062:13354 | menu                  | 375 × 585 | 🟢     | 🟢    | The ⋯ main menu, with shortcuts                   | MainMenu.qml, RedesignMenu.qml, RedesignMenuItem.qml |
| 6648:20666 | Profiles              | 651 × 633 | 🟢     | 🟢    | Profile submenu: Movies, Shows, Music, Work, New  | MainMenu.qml, ProfilesMenu.qml, defaults.cpp     |
| 6065:13616 | right-click menu      | 292 × 683 | 🟢     | 🟢    | Player right-click menu (Qt menu drawn to frame)  | menus.cpp, redesign_menu_style.cpp               |
| 2107:5903  | context menu          | 265 × 296 | 🟢     | 🟢    | Library item menu                                 | MLContextMenu.qml, RedesignContextMenu.qml       |
| 2139:8935  | folder context menu   | 265 × 296 | 🟢     | 🟢    | Folder menu                                       | FolderDisplay.qml, RedesignContextMenu.qml       |
| 2404:13340 | file context menu     | 265 × 584 | 🟢     | 🟢    | File menu                                         | MLContextMenu.qml, FolderDisplay.qml             |
| 2459:9389  | playlist context menu | 265 × 392 | 🟢     | 🟢    | Playlist menu                                     | MLPlaylistContextMenu.qml                        |
| 2107:5935  | option                | 301 × 292 | 🟢     | 🟢    | Menu row: default, hover, selected                | RedesignOptionItem.qml, RedesignMenuItem.qml     |

The four context menus are the design's cards (`RedesignContextMenu.qml`, build 117): the rows, the watched state and
*Show in File Explorer* as the design lists them.

**Add Subtitles parts**

| Node       | Name              | Size (px) | Status | Pixel | What it is                                      | Implemented in                                 |
|------------|-------------------|-----------|--------|-------|-------------------------------------------------|------------------------------------------------|
| 1915:8397  | subtitle multiple | 519 × 239 | 🟢     | 🟢    | A video with several subtitles: default, hover   | SubtitleAssociationsDialog.qml                 |
| 1919:9121  | subtitle one      | 519 × 239 | 🟢     | 🟢    | A video with one subtitle: default, selected     | SubtitleAssociationsDialog.qml                 |
| 1959:5325  | target            | 240 × 520 | 🟢     | 🟢    | To this folder / file, and its dropdown          | SubtitleAssociationsDialog.qml                 |
| 1963:5465  | origin            | 240 × 525 | 🟢     | 🟢    | From the computer / OpenSubtitles, dropdown      | SubtitleSourceControls.qml, OpenSubtitlesPanel.qml |

**Settings controls**

| Node       | Name             | Size (px) | Status | Pixel | What it is                                        | Implemented in                                 |
|------------|------------------|-----------|--------|-------|---------------------------------------------------|------------------------------------------------|
| 3021:9835  | setting category | 228 × 140 | 🟢     | 🟢    | Sidebar item: selected or not                      | SettingsSidebar.qml                            |
| 2372:8300  | Dropdown         | 406 × 369 | 🟢     | 🟢    | Dropdown: default, open                            | SettingsDropdown.qml, ComboBoxExt.qml          |
| 804:1208   | Values           | 253 × 404 | 🟢     | 🟢    | Segmented choice of 2 or 3 values                  | SettingsSegmented.qml                          |
| 3176:16556 | Values           | 360 × 216 | 🟢     | 🟢    | Vertical values (startup volume)                   | SettingsStartupVolume.qml                      |
| 3178:16769 | EQ               | 824 × 628 | 🟢     | 🟢    | Inline equalizer: closed, open                     | SettingsEqualizerPanel.qml, SettingsEqualizer.qml |
| 2372:8487  | EQ presets       | 406 × 460 | 🟢     | 🟢    | Equalizer preset dropdown                          | SettingsEqualizer.qml, EqualizerPanel.qml      |
| 6456:13892 | sub filename     | 406 × 422 | 🟢     | 🟢    | Subtitle file name matching dropdown               | SettingsDropdown.qml                           |
| 2372:8834  | Setting Preset   | 406 × 412 | 🟢     | 🟢    | A preset dropdown: default, open                   | EffectPresetField.qml, EffectPresets.qml       |
| 2372:8639  | Audio Visualizer | 406 × 369 | 🟢     | 🟢    | Visualizer choice: default, open                   | SettingsDropdown.qml, AudioVisualizationDialog.qml |
| 2372:8733  | HDR Options      | 406 × 422 | 🟢     | 🟢    | Auto, Off, HDR10, HDR10+, Dolby Vision, HLG         | HdrOutputDialog.qml, hdr_output.cpp, HDRButton.qml |

Differences: 2372:8834 is drawn on no screen of the design: it is the presets of the Effects window (New Preset,
⋯ Rename / Update / Delete, Default Preset), its colours and rows identical. 2372:8639 is *Settings › Audio › Visualizer* and the player's
visualizer (VU meter and VLC's other visualizations follow the design's four). 2372:8733: HDR10+ and Dolby Vision
are listed disabled, with the reason under the pointer (Windows offers no way for a player to send them).

**Effects panels**

| Node       | Name                         | Size (px)  | Status | Pixel | What it is                     | Implemented in                                         |
|------------|------------------------------|------------|--------|-------|--------------------------------|--------------------------------------------------------|
| 536:1513   | Audio - Compressor Settings  | 400 × 821  | 🟢     | 🟢    | Collapsed, expanded            | EffectsDialog.qml, FilterPanel.qml, SettingsInlineEffects.qml |
| 6114:13610 | Audio - Equalizer            | 400 × 1623 | 🟢     | 🟢    | Preamp and 10 bands            | EqualizerPanel.qml, SettingsEqualizerPanel.qml         |
| 536:1703   | Audio - Spatializer Settings | 400 × 821  | 🟢     | 🟢    | Collapsed, expanded            | EffectsDialog.qml, FilterPanel.qml                     |
| 612:1223   | Audio - Stereo Widener       | 400 × 821  | 🟢     | 🟢    | Collapsed, expanded            | EffectsDialog.qml, FilterPanel.qml                     |
| 536:1855   | Video - Essential            | 400 × 821  | 🟢     | 🟢    | Collapsed, expanded            | VideoEffectsPanel.qml, VideoFilterSection.qml          |
| 543:990    | Video - Crop                 | 400 × 821  | 🟢     | 🟢    | Collapsed, expanded            | CropPanel.qml, CropInput.qml                           |
| 614:1430   | Video - Colors               | 400 × 888  | 🟢     | 🟢    | Collapsed, expanded            | VideoEffectsPanel.qml, VideoFilterSection.qml          |

### 5.15 Screens outside the Figma file

| Screen                                                          | Implemented in                          | Status |
|-----------------------------------------------------------------|-----------------------------------------|--------|
| About: version, update check, design credit                     | RedesignAboutDialog.qml                 | 🟢     |
| Update card: Update now, Restart now, Later                     | RedesignUpdateBanner.qml, UpdatePane.qml | 🟢     |
| Cast panel: TVs, firewall card                                  | CastButton.qml                          | 🟢     |
| Sleep timer, subtitle series presets                            | SleepTimerMenu.qml, SubtitleSeriesPreset.qml | 🟢 |
| Idle player: the pixel cat                                      | IdleScene.qml                           | 🟢     |
| Player bar with nothing playing (*Always show the player bar*)  | ControlBar.qml, SettingsPresentation.qml | 🟢    |
| Installer: Welcome, Options, installing, Done                   | `extras/juxtopposed/installer/`         | 🟢     |
| Classic dialogs (Open Media, Convert, Media Information…)        | redesign_widget_theme.cpp, qvlcframe.cpp | 🟢 §8.6 |

### 5.16 Open differences

| #  | Difference                                                         | Frames                                   | Status |
|----|--------------------------------------------------------------------|------------------------------------------|--------|
| 1  | *Show the playlist as a tree*: the queue under the folders of its media (headings) | 3046:10057, 3062:11379 | 🟢 |
| 2  | Library context menus are the design's cards (they were native menus) | 2107:5903, 2139:8935, 2404:13340, 2459:9389 | 🟢  |
| 3  | The compact library's Home lists the sections of the library's Home (no play counts of playlists or artists) | 848:1996 | 🟢 |
| 4  | HDR10+ and Dolby Vision disabled with their reason (Windows has no way to send them) | 2372:8733 | 🟢 |
| 5  | Setting Preset is drawn on no screen of the design: the Effects window's presets | 2372:8834 | 🟢 |
| 6  | No background blur: panels solid at 95 % (the music backdrop is blurred); what shows through is at most 5 % (13 grey levels, measured: 4 under the ⋯ menu), under the pixel tolerance, so no blur pass is spent on the GPU | every floating panel | 🟢 |
| 7  | A stray separator line in the Disc tab of Open Media               | outside the file                         | 🟢     |

Fixed in build 115: the light theme leftovers, the accent on every check mark and active icon, four settings rows,
the new playlist's thumbnail and empty playlists, the resume card, the classic dialogs.

## 6. Design tokens

The values the app uses at 100 % (1 px = 1 Figma px; `MainCtx.dp()` scales them). **The source of these values
is [`design/tokens.json`](../design/tokens.json)** (`design-1.0`, W3C Design Tokens format, each token with its
Figma node and the places in the code that hold it); the tables below are its summary. In the code they live in
`modules/gui/qt/style/VLCStyle.qml` (object `redesign`), `menus/redesign_menu_style.cpp`,
`util/redesign_widget_theme.cpp` and the components named; `design/tools/check_qml.py` (CI, every pull request)
fails when the code and `tokens.json` differ. Icons and fonts: `design/icons/`, `design/fonts/`; CSS, Android and
Swift files: `design/build/` (`design/README.md`).

**Differences between these tables and the code** (found by the export, 2026-10-28; `tokens.json` follows the
code, the tables are left as they were until each is decided):

- *Dark/BG/A2* `#1F1F1F` is not "Figma only": it is the hovered row of the widget (right-click) menus
  (`RedesignMenuStyle`), `#9C9C9B` in the light theme.
- *Dark/FG/B1* `#161616` is not "Figma only": onboarding primary button text, ThemeStep, sleep timer, the New
  playlist icon wrapper (light `#B8B8B7` there).
- Scroll bar handle: Settings keeps the Basic style's own colours in the dark theme (not `#6B6B6B`); in the light
  theme the handle is `#6B6B6B` and `#2E2E2E` only when pressed. The classic dialogs: `#404040` dark, `#8E8E8D`
  light, `#6B6B6B` hovered.
- ✅ Dialog title bars, light theme, and the light values the colour map overrode: **settled 2026-10-28**
  ([design/light-theme.md](../design/light-theme.md)). The QML dialogs drew `#DEDEDE` (B2), the classic dialogs
  `#9C9C9B`; both draw `#DEDEDE` now. `color(dark, light)` ignored the light value whenever the map knew the dark
  one: 113 calls with two literal colours, 228 in all (through VLCStyle properties, helpers, dynamic values). A
  light value given now wins over the map, and each of the 228 was checked against *Player - Light* (2425:13624,
  pixel-compared with its dark twin 187:328): wherever the frame draws the colour it is the map's (e.g. *15 items*
  `#6B6B6B`, not the `#7C7C7B` given; the crumb and the times `#2E2E2E`, not `#4D4D4D`), so all 228 now write the
  map's value or leave the light value out: the light theme looks as before, the dark theme is unchanged.
  `check_qml.py` fails on a light value that differs from the map's unless listed. Eight dark values the frame does
  not draw (`#EAEAEA`, `#C6C6C6`, `#969696`, `#0E0E0E`, `#222222`, `#404040`, `#151515`, `#373737`) keep the map's
  value.
- ✅ **The opaque Light styles: decided 2026-10-29, keep the frame's colours.** The file has 24 *Light/* colour
  styles (no variables). The light frame draws its panels as *Light/BG/A3 Transparent* (`#D5D5D5` 75 %) and *B1
  Transparent* (`#E7E7E7` 80 %) over the dark video, which gives the `#ABABAA` and `#C2C2C1` the app draws as solid
  colours; the app stays 1:1 with that frame. The opaque styles are not used: several do not work as a theme (FG/1
  and FG/2 keep their dark values, FG/A4 is darker than FG/A3). [design/light-theme.md](../design/light-theme.md).
- Typography: dialog titles are 14 px (`#8F8F8F`, box 17 in a 32 row), not 16 px at 175 %; 16 px is the Settings
  header title. Widget (right-click) menus are 14 px, not 16. Onboarding: "Welcome to VLC!" is 32 px; some
  onboarding texts are 15 px with a line of 1.3.
- The Inter descender is 0.2421875 em (1.2109375 − 0.96875), not 0.2412.
- Seek preview: the frame is 252 wide in a 272 × 189 card (Figma 2151:9673), radius 12, not 224 and 10.
- Accent red, Bright: `#3B150D` is 0.26 × rounded half up; Qt's float rounding can give `#3A150D` (within the
  1-per-channel rounding the table allows).
- Not in the tables: Home folder cards `#1E1E1E` (light `#ABABAA`), the 95 % B1 over the canvas `#0F0F0F`, the
  modal scrim (black 45 %).

### Colours

The light value is what `VLCStyle.redesign.color()` returns for the dark one (measured from *Player - Light*).

| Figma style           | Dark      | Light     | In the app                                                    |
|-----------------------|-----------|-----------|---------------------------------------------------------------|
| Dark/BG/B1            | `#0E0E0E` | `#C2C2C1` | `windowBackground`; Settings, dialogs, compact library at 95 % |
| Dark/BG/B2            | `#090909` | `#DEDEDE` | `seekBackground`; wells; dialog title bars; settings header    |
| Dark/BG/A1            | `#222222` | `#9C9C9B` | `controlHoverBackground`                                       |
| Dark/BG/A2            | `#1F1F1F` | —         | Figma only                                                     |
| Dark/BG/A3            | `#1B1B1B` | `#ABABAA` | `playerControlBackground`, `playerTopbarBackground`, buttons   |
| Dark/BG/A4            | `#191919` | `#ABABAA` | `sidebarBackground`, `controlBackground`; menus at 95 %        |
| Dark/BG/A5            | `#151515` | `#B5B5B4` | hovered rows                                                   |
| Dark/BG/Accent Bright | `#371F06` | tinted    | `accentBackground` (0.26 × accent); light: `#DEDEDE` + accent 35 % |
| Dark/Accent/Main      | `#D37918` | same      | `accent`                                                       |
| Dark/Accent/DIm       | `#AB6314` | same      | `accentDim` (0.81 × accent)                                    |
| Dark/Accent/Logo      | `#D37918` | same      | `logoColor` (the cone)                                         |
| Dark/FG/1             | `#EAEAEA` | `#2E2E2E` | `textPrimary`                                                  |
| Dark/FG/2             | `#C6C6C6` | `#2E2E2E` | settings section titles                                        |
| Dark/FG/A1            | `#969696` | `#4D4D4D` | `textSecondary`                                                |
| Dark/FG/A2            | `#8F8F8F` | `#2E2E2E` | `textMuted`: menu labels, icons                                |
| Dark/FG/A3            | `#6B6B6B` | `#6B6B6B` | menu shortcuts, dimmed items                                   |
| Dark/FG/A4            | `#404040` | `#929291` | `textDisabled`, placeholders                                   |
| Dark/FG/B1            | `#161616` | —         | Figma only                                                     |
| Dark/Divider/Dim      | `#222222` | `#9C9C9B` | `separator`                                                    |
| Dark/Divider/Bright   | `#373737` | `#8E8E8D` | `border`                                                       |
| (warning, not Figma)  | `#E4A580` | `#7A1414` | settings warnings and errors                                   |
| (scroll bar handle)   | `#6B6B6B` | `#2E2E2E` | scroll bars of Settings                                        |
| bg blur               | blur 50   | —         | 🟢 solid panels at 95 %: a blur changes at most 5 % of a pixel  |
| bg blur + shadow      | + shadow  | —         | 🟢 shadow 0 8, blur 15, `#000000` 46 % · 🟢 solid at 95 %        |

### Accents

`VLCStyle.redesign.accentChoices`, in the onboarding's order. Dim (0.81 ×) and Bright (0.26 ×) are computed, except
the default's (the design's values); rounding may change a channel by 1.

| #  | Accent             | Dim       | Bright    | Bright (light) |
|----|--------------------|-----------|-----------|----------------|
| 1  | `#D37918` orange   | `#AB6314` | `#371F06` | `#DABB99`      |
| 2  | `#E15033` red      | `#B64129` | `#3B150D` | `#DFACA2`      |
| 3  | `#C49D24` yellow   | `#9F7F1D` | `#332909` | `#D5C79D`      |
| 4  | `#18A235` green    | `#13832B` | `#062A0E` | `#99C9A3`      |
| 5  | `#18AAD3` cyan     | `#138AAB` | `#062C37` | `#99CCDA`      |
| 6  | `#185DD4` blue     | `#134BAC` | `#061837` | `#99B1DB`      |
| 7  | `#8E41FF` purple   | `#7335CF` | `#251142` | `#C2A7EA`      |
| 8  | `#F051B5` pink     | `#C24293` | `#3E152F` | `#E4ADD0`      |
| 9  | `#AFAFAF` grey     | `#8E8E8E` | `#2E2E2E` | `#CECECE`      |

**Follow the Windows accent color** (not in the design; `redesign/accent` = `system`, `RedesignAccent` in
`util/redesign_accent.cpp`): a toggle row under the swatches in Settings > Appearance, as Developer mode (42 high,
label 14 px `#8F8F8F`, the Toggle at the end), off by default and shown at once; on, no swatch shows as chosen, and
picking one turns it off (off goes back to the colour chosen before, else the orange). The 9 swatches and the
onboarding's 9 discs stay as Figma draws them. The colour is Qt's
`QPalette::Accent` (Qt 6.6+ reads it from the system), on Windows else
`HKCU\Software\Microsoft\Windows\DWM\AccentColor`; it is read again on `ApplicationPaletteChange`,
`colorSchemeChanged`, `WM_SETTINGCHANGE` "ImmersiveColorSet", `WM_DWMCOLORIZATIONCOLORCHANGED` and
`WM_SYSCOLORCHANGE` (after 250 ms), and if Windows changes the registry without Qt's palette following, the
registry wins. Without a system accent (Linux without one, Qt's default highlight `#308CC6`): the redesign orange.
**Readability rule:** the 9 accents are drawn unchanged on both themes and range in WCAG relative luminance from
0.128 (the blue, 3.3:1 on `#0E0E0E`) to 0.429 (the grey). A system colour outside that range keeps its hue and
saturation and gets the HSL lightness that brings its luminance to the nearest end, so it is never darker than the
blue on the dark theme nor lighter than the grey on the light theme; Dim, Bright and the light tint then come from
it as for the others.

### Buttons on their panels (decided 2026-10-29, the owner's report)

Figma draws the neutral buttons in the fill of the panel they sit on, or one step from it: *Cancel*, *Licenses &
authors*, *GitHub*, *Don't restore* `#1B1B1B` on the dialogs' `#191919` (1.0:1), Settings' *Browse* / *Reset* /
*Edit Keyboard Shortcuts* `#191919` on `#0E0E0E` (1.1:1), the classic dialogs' buttons `#191919`; in the light theme
`#ABABAA` on `#ABABAA`. They melted into the panel. Measured (WCAG relative luminance), dark and light:

| Button | Fill on panel | Text on fill | Decision |
|---|---|---|---|
| Neutral (RedesignButton, RedesignDialog's Cancel, About) | 1.0:1 (both themes) | `#8F8F8F` 5.3:1, light `#2E2E2E` 5.9:1 | 🟢 + 1 px frame `#4C4A48` (the window border token): 2.0:1 on `#191919`, 3.8:1 on `#ABABAA` |
| Settings buttons (SettingsButton) | 1.1:1 | 5.3:1 / 5.9:1 | 🟢 + the same frame |
| Classic dialogs (QPushButton) | 1.0–1.1:1 | 5.3:1 / 5.9:1 | 🟢 + the same frame (`redesign_widget_theme.cpp`); default and checked buttons keep their accent fill, flat ones none |
| Accent buttons of the QML dialogs (Save, Close) | `#371F06` on `#191919`, the same in the light theme | accent 4.8:1 | unchanged |
| Accent fill of the classic dialogs, light theme (default button, selected tab, row, selection) | tint `#DABB99` | accent **1.4–2.7:1** ✗ (orange 1.8) | 🟢 text `accent.darker(220)` (`$(onAccent)`): 5.1–6.4:1 on each of the 9 tints (orange 5.6) |
| Settings ActionButton | `#202020` + `#353535` frame | `#D0D0D0` | already framed: unchanged |
| Icon-only buttons (bar, top bar, sidebar toggles, ×, ⋯, sort/view, chips; icon-only QPushButtons of the classic dialogs) | transparent by design (the glyph is the button) | icons `#8F8F8F` 5.3:1 on `#1B1B1B` | **never a frame** (owner rule): readable by the glyph colour and the design's hover, pressed, active and focus states only |

The frame is the one colour both themes share (the light map keeps `#4C4A48`), so a button reads as one in either
theme and over the video.

### Typography

Inter 3.19 (variable) with every advance one unit wider (Qt truncates advances to 1/64 px), distance-field
rendering, no hinting, greyscale antialiasing. A line is 1.2109 em (ascender 0.96875, descender 0.2412) centred in a
box of whole pixels (17 px for 14 px text), placed with `VLCStyle.redesign.textBaseline()`. Keycaps: DM Mono.

| Use                            | Font                  | Size (px)            | Line       |
|--------------------------------|-----------------------|----------------------|------------|
| Interface text, QML menus      | Inter Regular         | 14 (`fontNormal`)    | box 17     |
| Buttons                        | Inter SemiBold        | 14                   | box 17     |
| Slider scales, small labels    | Inter Regular         | 12 (`fontSmall`)     |            |
| Player control texts           | Inter Regular         | 15 (`fontControl`)   |            |
| Dialog titles                  | Inter                 | 16                   | 175 %      |
| Settings section titles        | Inter, `#C6C6C6`      | 16                   | 19         |
| Widget menus (right-click)     | Inter                 | 16                   |            |
| Music title                    | Inter                 | 24                   | box 29     |
| Onboarding titles / texts      | Inter Light           | 24 / 16              | 175 % / —  |
| Keycaps                        | DM Mono Regular       |                      | 1.302 em   |

### Sizes and spacing (px)

| Element              | Values                                                                       | Source                      |
|----------------------|------------------------------------------------------------------------------|-----------------------------|
| Top bar              | 40 high, padding 6; icons in 28 × 28 boxes, 10 apart; cone in 38 × 28        | `topbarHeight`, TopBar.qml  |
| Control bar          | rows 54; controls 28, centred; 20 from the sides; gaps 20 and 8              | `controlHeight` and others  |
| Play button          | 54 × 28 (a 48 pill while customizing)                                        | `playWidth`, PlayButton.qml |
| Seek bar             | 28 high, handle 8, inset 2                                                   | `seekHeight`, `seekHandleWidth` |
| Volume · time        | 88 × 28 · 76 wide                                                            | `volumeWidth`, `timeWidth`  |
| Video frame          | 10 on the sides, 1 under the top bar, corners 9; none in fullscreen          | Player.qml                  |
| Library sidebar      | 217 wide; rows 32 + gap 14 (46); padding 12 / 16                             | `sidebarWidth`              |
| Compact library      | sections column 50 + panel 377                                               | CompactLibrary.qml          |
| Queue card           | 400 wide, header 50                                                          | QueueCard.qml               |
| Library header       | padding 20, row 34                                                           | LibraryHeader.qml           |
| Home                 | sections padded 20, cards 20 around, 17 between; folder cards `#1E1E1E`      | HomePage.qml                |
| Menus                | 265 wide (⋯ 375, right-click 292), padding 4, rows 48 (padding 2 / 14)       | RedesignMenu.qml            |
| Dialogs              | title bar padding 9, title 32; content 22 / 20, gap 20; footer 72, buttons 100 × 32 | RedesignDialog.qml   |
| Text input           | one line 37 (row 41), several 194; padding 8 / 18 / 8 / 12; 2 border inside  | RedesignTextArea.qml        |
| Settings window      | header 50; sidebar 200 in an 8 margin (titles 33, items 40); page 24 / 20     | SettingsDialog.qml          |
| Settings rows        | 42 toggle, 46 dropdown or field, 55–57 slider                                | SettingsOptionRow.qml       |
| Toggle               | 68 × 32 well, 33 × 26 label, 26 × 26 square                                  | CheckButton.qml             |
| Dropdown · segmented | 320 × 36 well, 2 padding, option 316 × 32 · options 32, gap 2                | SettingsDropdown.qml        |
| Slider               | track 320 × 16, fill 12, handle 8 × 24, labels 12; 57 with labels            | SettingsSlider.qml          |
| Text field · colour  | 36 high, 2 border, text 12 in · 320 × 32, swatch 22 × 22                     | SettingsTextField.qml       |
| Mini player          | padding 8, video 375 × 210.94                                                | LibraryMiniPlayer.qml       |
| Seek preview         | frame 224 wide, height from the video                                        | SeekFramePreview.qml        |
| Music cover          | 738 × 737 at 1920 × 1080; glow `#FFFFFF` 3 %, blur 90.5, spread 64           | AudioContent.qml            |
| Shadows              | menus, dialogs: 0 8, blur 15, 46 % · mini player: 0 0, blur 11.8, 38 %       | RedesignMenu.qml            |

### Corner radii, icon sizes, windows (px)

| Radius | Used for                                                       |
|--------|----------------------------------------------------------------|
| 2      | Advanced Options bar, inline equalizer panel, colour swatch    |
| 3      | toggle label and square                                        |
| 4      | grid covers, small inner fills                                 |
| 6      | controls: buttons, fields, wells, menu rows, sidebar, queue card |
| 9      | the video frame's corners                                      |
| 10     | menus, resume card, seek preview, Sorting Options              |
| 12     | Home folder cards, onboarding theme swatches                   |
| 14     | dialogs, Settings, Customize panel, mini player                |
| 23     | the ring of the chosen theme (onboarding)                      |

| Icon size | Used for                                                              |
|-----------|-----------------------------------------------------------------------|
| 24        | standard glyph (`iconSize`): menus, chevrons, settings sidebar         |
| 28 × 28   | top bar and control bar boxes                                         |
| 32 × 32   | dialog close (around a 12 px cross), settings sidebar box, "add" icon  |
| 22        | the play triangle (between paddings of 17 and 15)                     |

| Window                              | Size                                                       |
|-------------------------------------|------------------------------------------------------------|
| Design frames                       | 1920 × 1080                                                |
| Main window, first start            | 1280 × 720 (at most 80 % of the screen), centred           |
| Minimum width · layout steps        | 500 · small up to 600, medium 1000                         |
| Settings                            | 1048 × 865                                                 |
| Add Subtitles, Keyboard Shortcuts   | 1048 × 800 (content 632)                                   |
| Open network stream                 | 1048 × 631 (content 509)                                   |
| New playlist                        | 713 × 589                                                  |
| Onboarding                          | 1090 × 759                                                 |
| Customize Player panel              | 1048 × 655, 23 above the middle                            |
| Visualizer · HDR popups             | 400 × 400 · 400 × 445                                      |

## 7. Desktop for Windows: what is done

Shipped in 0.1.0 to 0.2.6 ([changelog](../CHANGELOG.md)). Work since 0.2.6 is in section 8.

**Player**

- 🟢 Video framed between the bars, rounded corners; no frame in fullscreen; bars auto-hide in fullscreen
- 🟢 Redesigned top bar; Windows 11 snap layouts; maximize shows a restore icon
- 🟢 Skip ±15 s (±10 s buttons optional); times with hours; a click shows the remaining time
- 🟢 Cone volume up to 200 %; startup volume
- 🟢 Frame previews on the seek bar, decoded ahead and kept (48 MB at most, local files)
- 🟢 Waveform seek bar for music (local, up to 1 GB and 8 minutes); waveform visualisation
- 🟢 Music view: artwork, blurred backdrop, lyrics; Visualizer button; right-click on music
- 🟢 Queue card: Queue, Bookmarks, Chapters; bookmarks added and removed in place
- 🟢 Compact library over the video: sections, breadcrumb, search, sort, progress, play counts
- 🟢 Tracks panel: subtitles by language, external tracks, sync timeline, series presets, *None* video track
- 🟢 Picture-in-picture, mini player with queue, sleep timer, A-B loop, speed, record, snapshot
- 🟢 Hide UI (Ctrl+\\); fullscreen from any view (F, F11, Escape, double-click); fullscreen button in the bar
- 🟢 Right-click and ⋯ menus redrawn; classic menus with two-tone icons
- 🟢 Resume card (or resume always / never); HDR output: Auto, Off, HDR10, HLG (libplacebo)

**Library**

- 🟢 Home: Continue Watching, Listen Again, Frequently Visited Folders
- 🟢 Folder browser with breadcrumbs; network folders listed in the background
- 🟢 Recent and Playlists pages; grouping by name or folder; back and forward
- 🟢 Playlists: create, favourite, shuffle play, export
- 🟢 Video cards: file names, watched dimmed, progress as an accent line

**Customize and themes**

- 🟢 Drag and drop on the player: palette, drops between controls, timeline on its own row
- 🟢 Profiles Default, Movies, Shows, Music, Work; New, Rename, Clone, Delete
- 🟢 About 20 new control bar buttons and widgets (±10 s, PiP, file name, frame rate, HDR, zoom…)
- 🟢 Dark, Light, System themes; 9 accents

**Settings**

- 🟢 One window, 13 sections, search (labels and shortcuts), saved on close, *Advanced Options*
- 🟢 The design's controls, all usable with the keyboard
- 🟢 Many more VLC options; developer mode with *All VLC options*; restart hints; errors shown
- 🟢 Rescan plugins, open the configuration folder, reset all
- 🟢 Shortcuts editor: several keys per action, capture, reset

**Subtitles**

- 🟢 Choices remembered per file and per series; files next to the video load by themselves
- 🟢 Association dialog for a file, folder or playlist; external audio tracks
- 🟢 OpenSubtitles with the user's key; safe archives (16 MB, subtitles only); cache of 500 media

**Casting**

- 🟢 Cast button, DLNA and Chromecast discovery
- 🟢 The firewall asked only when needed, never twice
- 🟢 Conversion when the TV can't play a file, subtitles burned in
- 🟢 Back from the TV: clean picture, playback goes on where the TV was

**Sound**

- 🟢 Seamless switching of the default device while playing (#3)
- 🟢 200 ms WASAPI buffer: no break-up on a busy PC

**Installer and updates**

- 🟢 Installer drawn like the app (English, Hebrew); per user, or for everyone
- 🟢 File associations (about 130 formats) without taking over the user's choices; Explorer entries
- 🟢 Uninstall gives every file type back to the app it had before
- 🟢 Daily update check; small updates (binary deltas), SHA-256 checked, rolled back on failure
- 🟢 Signed checksums (BUG-006, 2026-10-28): `SHA256SUMS.txt.sig` by the project's release key (ECDSA P-256),
  checked with CNG before any checksum is used; required from 0.4.0 on (`FIRST_SIGNED_VERSION`). Key and
  commands: `extras/juxtopposed/signing/release-key.ps1`; `make-release.ps1` signs. Tested: sign/verify round
  trip, openssl cross-check, the C verifier on good, tampered, flipped and wrong-key signatures. Still to test: an
  update from a 0.4.0 build to a signed release.
- 🟢 *Restart now* reopens the media where it was in about two seconds; portable copies update too

**Windows and robustness**

- 🟢 Several copies at once; fast start; quick first start after installing
- 🟢 Three interface/core deadlocks fixed; VLC 4's pause → seek → play freeze fixed
- 🟢 Direct3D 11 device leak fixed: threads and memory flat over playlists
- 🟢 Folders named in other alphabets work (Lua scripts)
- 🟢 English by default
- 🟢 Design credit in About, README, `CREDITS.md` and the package; reproducible Docker build; weekly upstream merge

## 8. Next release: 0.3.0

### 8.1 Release policy

Everything in this section belongs to 0.3.0: no later versions are planned before it ships. What is ready and passes
the quality gates when the owner decides to release ships; the rest stays here for the release after. The version
becomes 1.0 when every frame is pixel-checked, the light theme is complete and VLC 4.0 is released.

### 8.2 Playback, stability and player

- 🟢 Surround: 5.1 and 7.1 no longer jump between speakers; every layout mixed right to stereo; 6.1 rear centre kept
- 🟢 Explorer: a file opened while VLC plays starts at once (0.1 s) and raises the window; a hung VLC can't block it
- 🟢 First start: no firewall prompt (no ports opened), the welcome waits, a file opens in 0.75 s
- 🟢 Memory flat over long sessions (1,400 media changes in 25 minutes)
- 🟢 The player bar shows the current media's name, title and artist
- 🟢 Videos decoded by the processor start quicker (Direct3D device kept)
- 🟢 Conversion: the crash of every audio + video conversion fixed (also in VLC 4); the end kept
- 🟢 Video effects: 6 crashes fixed, 40 effects checked alone and in chains
- 🟢 Damaged files: 148 damaged copies of 35 formats play without a crash or hang
- 🟢 Quitting after a film is quick again
- 🟢 The idle player shows Figma's pixel cat, drawn as blocks
- 🟢 Popups close on a click outside without pausing the video
- 🟢 The seek bar no longer runs its loading animation when nothing plays (it redrew the window without end)
- 🟢 An update keeps the settings, the portable zip too: a new copy starts with those of the copy used last
- 🟢 A big library (3,000 songs, 400 episodes): indexed in 100 s; its pages open with at most 42 ms without an
  answer, fast scrolling at most 10 ms; memory flat at about 200 MB; the library lists start A-Z
- 🟢 Small windows, down to the smallest (320 x 360): the main menu shows that it scrolls, the library panel fits
  and its sections scroll, a narrow bar keeps the same buttons at every start (outer ones last)

### 8.3 Light theme and accents

- 🟢 Player chrome and icons recoloured (Player - Light)
- 🟢 Tracks popup, row hovers included
- 🟢 Subtitles panel
- 🟢 Profiles menu
- 🟢 Onboarding: the six files have a light branch (dark unchanged)
- 🟢 Shortcut capture, reset and restart popups; their errors in the accent
- 🟢 Cast and visualization popups
- 🟢 Mini queue
- 🟢 Language list
- 🟢 Text selection colours
- 🟢 Customize drop zone
- 🟢 Effect sliders follow the accent
- 🟢 Every check mark and active icon follows the accent (cast, visualizer, HDR, subtitle associations, option
  items, crop link, folder play outline, compact library sections, queue sort dot, Recent, *This computer*,
  visualizer icons, side navigation, settings sidebar); the default accent pixel-identical · checked in the app with
  a blue accent (settings sidebar, check marks, the visualizer's two-tone icons)
- 🟢 Settings scroll bar handles visible: `#6B6B6B` dark, `#2E2E2E` light · in the app ⏳
- 🟢 Warning colour readable: `#E4A580` dark, `#7A1414` light · in the app ⏳
- 🟢 The whole app checked in light: player, tracks and subtitles window, cast window, ⋯ menu, classic dialogs
- 🟢 Text contrast measured (WCAG 2.1, 2026-10-23): the texts of the design pass AA (4.5:1) on every panel but its
  dim greys: dark `#6B6B6B` 3.3 to 3.7 and `#5F5F5F` 2.8 to 3.1 (hints, descriptions, watched videos), light
  `#6B6B6B` 2.3 to 3.0 on the panels and `#4D4D4D` 4.1 on lists (the `#5A5A59` measured first was never drawn:
  the colour map won, [design/light-theme.md](../design/light-theme.md)). Kept as the design draws them; *Increase text contrast*
  (Interface › Accessibility, off) draws them `#8A8A8A` (dark) and `#333333` (light), at least 4.6:1 on every panel
  and hover background, and the (i) icons `#707070` / `#575756`, at least 3.1:1 (the 3:1 of icons), in
  `VLCStyle.redesign.color()`

### 8.4 Popups and menus

- 🟢 ⋯ main menu drawn as the frame, opens and closes on its button
- 🟢 Profile submenu
- 🟢 Right-click menu drawn as the frame
- 🟢 Subtitles/tracks popup: opens and closes on its button; a click outside only closes it; the video keeps playing
- 🟢 Cast popup, the same
- 🟢 Sleep timer, the same
- 🟢 Visualizer, the same
- 🟢 HDR, the same
- 🟢 Every Settings dropdown, the same
- 🟢 Language picker, the same
- 🟢 Library Sort, View and More, the same
- 🟢 Library item context menu as the design's card
- 🟢 Folder context menu as a card
- 🟢 File context menu as a card
- 🟢 Playlist context menu as a card

### 8.5 Settings rows

The design's settings frames hold 211 rows (counted from their texts, each page once); the app adds one. All of
them are in the app.

- 🟢 206 rows of the design on the 13 pages
- 🟢 *Show confirmation and error dialogs* (Global, `qt-error-dialogs`): no error popup, questions answered yes;
  deleting a file still asks · checked in the app: a media that cannot open shows its error only with it on
- 🟢 *Additional command line options* (Global, advanced; new core option `extra-options`, read at every start; the
  command line wins; `--help`, `--config` and the like ignored) · checked in the app (`--no-qt-bgcone` from vlcrc)
- 🟢 *Show controls in fullscreen* (Interface, `qt-fs-controller`, live) · in the app ⏳
- 🟢 *Show cover art in the window when playing audio* (Interface, advanced, `qt-bgcone`) · checked in the app (off: the
  dark placeholder of the theme)
- 🟢 *Show the playlist as a tree* (Interface › Playlist, `playlist-tree`, live): VLC 4's queue is flat, so the queue
  shows its media under the folder (or the host) they come from, as headings; checked in the app
- 🟢 *Always show the player bar* (Interface › Playback Controls, new, default off, live): the bar stays on Home and
  every page with nothing playing (times `--:--`); its cover and the player switch open the empty player with the
  pixel cat; Play starts the queue or opens files

The dashboard counts 211 of 212 rows (the design's 211 and the new one); the 0.3.0 bar counts these 7 lines.

### 8.6 Classic dialogs

All classic windows go through one hook, `redesign_widget_theme.cpp`, applied from `qvlcframe.cpp`
(`QVLCDialog::setWindowTransientParent()`), and follow Dark / Light and the accent live.

- 🟢 The hook, for every `QVLCDialog` and `QVLCFrame` window
- 🟢 Open Media: File, Disc, Network and Capture Device tabs
- 🟢 Convert / Save
- 🟢 Media Information; its bitrate graph readable in dark
- 🟢 Messages; the log colours readable in dark
- 🟢 Bookmarks window
- 🟢 Advanced preferences (*All VLC options*)
- 🟢 EPG, VLM, help, message boxes
- 🟢 The crash it caused at start (the shared menu style wrapped by a dialog's style sheet, 1 start in 2) fixed;
  6 runs clean
- 🟢 Go to time, statistics, plugins list (the same hook) · in the app ⏳
- 🟢 A stray separator line in the Disc tab (removed, build 121)
- 🟢 Go to time, statistics, plugins list and folder information redrawn in QML (2026-10-28, build ⏳):
  `RedesignGoToTimeDialog`, `RedesignStatisticsDialog` (new Tools entry *Media Statistics*, bitrate graph),
  `RedesignPluginsDialog` (plugins and Lua extensions; add-ons stay in Settings ▸ Extensions),
  `RedesignFolderInfoDialog` (the folder menu's Info); data from `redesign_tools.cpp`. The classic windows remain
  for skins; the Statistics tab of Media Information stays · in the app ⏳
- 🟢 No plain Windows or Qt box left (2026-10-28, build ⏳): every message, question and prompt is a redesign
  dialog. In the window: `RedesignMessageDialog.qml` (from `DialogsProvider::showRedesignMessage()`): the play
  queue to restore after a crash (*Restore the play queue?*, *Restore* / *Don't restore*, not blocking), the
  library's confirmations (named actions), rename playlist, paste links, `messageDialog()`. The core's login,
  question and progress dialogs (`Dialogs.qml`) and the error banner. Elsewhere (a classic window asking, no
  interface, the window hidden): `RedesignMessageBox`, frameless, the same sizes and colours. Before Qt
  (`bin/winvlc_message.c`, GDI + DWM): could not start, the Windows version check, the crash report question
- 🟢 The extension and fingerprint windows through the hook too
- ⏳ In the app: the recovery question after a forced kill, a share's login, the start failure (`--foo`
  does not fail libvlc: rename `plugins` in a test copy), light theme, 150 % scale, right to left
- 🟢 The classic windows get the redesign's title bar (`redesign_window_frame.cpp`, from the hook): #090909, Inter
  14 title, the redesign's icons; Windows: native frame kept (snap, resize, shadow, Windows 11 corners), caption
  removed by `WM_NCCALCSIZE`, the bar hit-tested as `HTCAPTION` · in the app ⏳ (Windows 10 has no top border line;
  the snap layouts flyout on the maximize button is not offered; Linux and macOS keep the system bar)

### 8.7 Casting

- 🟢 The TV starts once, with the picture and the sound together
- 🟢 Every film to its last picture (also with a subtitle file, and the next film of a playlist)
- 🟢 Playlists and subtitle files while casting
- 🟢 4K made smaller by the graphics card: four times faster
- 🟢 Lighter: each picture sent at once, half the memory
- 🟢 50/60 fps films at 25/30 on the TV
- 🟢 TVs listed before the panel opens, the last one used at the top
- 🟢 Samsung (UE55MU8000): a paused cast is kept (a trickle of null packets while paused)
- 🟢 Samsung: a film sent as it is goes as MPEG-TS; a TV that stops it gets it converted
- 🟢 The real-TV gate on the 0.3.0 candidate (build 193, 2026-10-25, the real test TV): converted with subtitles: started in
  3.5 s, paused 1 min and 5 min (paused the whole time), resumed, seeked, next file, the last film to its end; sent as
  it is (AVC_TS_NA_ISO): started mid-film in 2.5 s, seeked to 25 s before the end, the TV played to the end and the
  player stayed playing until it had; quit in 0.5 s. Earlier: 2026-10-24 on build 174 (the real test TV, a Samsung): start, pause 1 min and
  5 min (paused the whole time), seek, resume, next file, the end (stopped at 29.3 of 30 s), converted with
  subtitles; and on build 178 a cast sent as-is, started mid-film, seeked to 25 s before the end: the TV played to
  the end and the player stayed playing until it had (the end of a cast mid-film, ff54815d10, d0e1f85b1e, and the
  last frames of MPEG-TS, 95722c717a, fixed with the fake TV first); the same run repeated on build 193 above
- 🟡 Other brands: a real Google Chromecast Ultra checked 2026-10-25 on the release binary (start mid-film with
  subtitles, pause, play, seek, another subtitle, subtitles off: the device stayed PLAYING through the subtitle
  change, no reload, no error); the living-room Android TV (Mi TV) was off; no LG or Sony on this network: through
  device reports
- 🟢 **Casting final pass (2026-10-29, `feature/cast-final`, fakes)**: a live TV channel (Pluto TV, HLS) cast to a
  Chromecast crashed VLC at its end (zapping, stop, quit): `DEMUX_GET_LENGTH` without its live argument
  (`52ce2e82fc`); zapping while casting now works, and a stream restarting its picture reloads the TV once
  (`1880587aa0`); quitting while casting took 16.5 s and left the TV playing: 2 s, the TV stopped (same commit);
  the cast list searches as long as it is open, hidden devices stay hidden under both names, no search by itself
  with *Discover cast devices* off, the casting screen on the window background with the file name, the subtitles
  button kept while casting (`59935ee51d`). Checked with the fakes: start, pause, resume, seek, volume, back here,
  a playlist to its end (both films to their last picture), quit; DLNA errors 716 (*couldn't read*), the TV
  stopping by itself (*stopped playing*), with *Try again* / *Play here*.
- 🟢 Memory over 20 cast / back rounds (fake Chromecast, 70-min film, hidden desktop): private 286 → 290 MB,
  threads 242–246, handles 1,510–1,530, the time going on each way; a Hebrew subtitle file burnt into the cast in
  the right order (fake Chromecast's stream checked), VLC 1.2 cores while converting 1080p with subtitles; the
  panel and the casting card in light, Hebrew (dark) and zoom 1.5 and 2.
- 🟡 DLNA and live channels (fake TV, Pluto TV): no crash, zapping works, but a channel's picture restarting just
  after it starts makes the TV get a new stream 3–5 s after the first (two `SetAVTransportURI` per channel), and a
  cast started on a channel already playing went out with the sound only. The DLNA output needs the Chromecast's
  hold-again (`1880587aa0`): not done, no build was possible (Docker stopped for the disk clean-up).
- ⬜ the owner's test TV not re-checked on this branch: the test copy (`qapp`) has no firewall rule, and casting would
  have shown the owner Windows' administrator prompt. Last real-TV check: build 193.
- 🟡 A TV that keeps its connection but stops answering (the fake's *off* mode) is noticed only after libupnp's
  30 s timeouts (twice), and a quit then waits for the call in flight: a limit of libupnp's blocking SOAP calls.
- 🟢 A *TV compatibility* wiki page fed by the reports (docs/wiki/TV-Compatibility.md, and the issue form
  `.github/ISSUE_TEMPLATE/tv_report.yml` it links to)

Kept as it is: the Samsung TV rejects VLC's `PrepareForConnection` (arguments not in the UPnP order). It does no harm,
and an accepted request made the TV fail after a pause in a test.

### 8.8 Design and library

- 🟢 Resume card as in Figma: the watched time and one *Continue Watching* button (checked in the app)
- 🟢 New playlist with a custom thumbnail (PNG, 1024 px at most, deleted with the playlist) · in the app ⏳
- 🟢 Empty playlists: Save needs only a title (checked in the app: the playlist shows the dark video placeholder)
- 🟢 The compact library's Home with the sections of the library's Home, in its order
- 🟢 Background blur (50 px) behind floating panels: not drawn, the panels are 95 % opaque (at most 13 grey levels
  show through, under the pixel tolerance; no blur pass for weak graphics cards)
- 🟢 HDR10+ and Dolby Vision listed with the reason they cannot be chosen (Windows offers no way to send them)
- 🟢 Geometry, overlay and advanced video filters in the effects panel
- 🟢 Setting Preset and Audio Visualizer compared with 2372:8834 and 2372:8639 (the presets of the Effects window)
- 🟢 Editing the music bar verified: a control dragged from the palette onto it and saved (`$1` of the profile);
  the mini player card keeps the fixed controls of 970:3091, and Customize Player from the library shows the player
  to edit its bars in place
- 🟢 Pixel check of all 103 frames (§11): the two hover states (177:98, 2151:9673) in Windows Sandbox, where real
  mouse input stays inside it: the button's hover box 30 x 32 at white 6 %, the preview's time 15 px
- 🟢 Subtitle cues drawn on the seek bar: the lines of the subtitle file shown (added, found next to the video,
  downloaded), `#6B6B6B` dark / `#5A5A59` light, 4 px high in the middle of the bar, under the played fill, joined
  under 2 px, painted once per width; the line at the hovered time in the frame preview; Settings, Subtitles,
  Timing *Show subtitle lines on the seek bar* (`qt-seek-subtitle-cues`, live). Embedded text tracks: not drawn
  (VLC 4 reads their lines only as they play) · in the app ⏳
- 🟢 A *What's new* card after an update: the first start of a new version (installer, small update, a portable
  copy that took an older one's settings) shows the bold headlines of its *Added*, *Changed* and *Fixed* in
  CHANGELOG.md (carried in the resources), with *Full changelog*; once a version (`redesign/whats-new-seen`), not
  on a new install (the welcome), after the film when VLC opened with one; Settings, General, *Show what's new
  after an update* (on). The notes stay in English · in the app ⏳
- 🟢 The accent following the Windows accent colour (an option): *Follow the Windows accent color* under the
  swatches in Settings > Appearance (`redesign/accent` = `system`), followed live, made readable (§6 Accents) · in the app ⏳

### 8.9 Quality and hardware

- 🟡 Code-signed builds (decision 2: SignPath Foundation, free for open source): the CI jobs, the artifact
  configurations, `CODE_SIGNING_POLICY.md` and the steps (docs/code-signing.md) are ready, and every binary carries
  VLC Redesign's name and version, and the two features that go online are chosen at install (or at the first
  start of a portable copy) with the policy linked; waiting for the user's application to SignPath
- 🟢 Automated tests in CI: launch, playback, QML warnings (extras/juxtopposed/ci/launch-test.ps1 and its job in
  build-windows.yml): the Windows build and the launch test passed on GitHub's runners with the 0.3.0 push (2026-10-25)
- 🔴 Intel graphics, older PCs, Windows 10 on real hardware, high-DPI multi-monitor (2026-10-24: the app drawn at
  125, 150, 175 and 200 % has the layout of 100 % on every page checked, with sharp text, and its caption buttons
  and top edge answer Windows where they are drawn; still to do: monitors of different scales, a real one above 100 %)
- 🟡 Our VLC core fixes (pipes, freezes, filters, conversion, casting) offered to VideoLAN: prepared, waiting for
  the owner (2026-10-28): 66 patches on VideoLAN's `master` (0c23b0d4b1) in `~\source\vlc-redesign-upstream\patches`,
  one fix each in VLC's commit style, applying as a series (all but four alone too) and building for win64; the
  README there lists each bug, how to reproduce it and the risk, the redesign-only and mixed commits left out, and
  the merge request steps on code.videolan.org. Nothing sent

### 8.10 Release checklist

- 🟡 Every quality gate green (§14): all but Hardware (no Intel or real Windows 10 here)
- 🟢 `CHANGELOG.md`: *Unreleased* became *0.3.0* · 2026-10-25
- 🟢 Release notes with a screenshot or GIF per highlight (docs/release/0.3.0, 13 images from the running app; the
  last changes of the changelog join them when the date is set)
- 🟢 Wiki: casting pages, new settings rows, *Design check* (docs/wiki, in the wiki's own page names; uploaded with
  the release)
- 🟢 Small updates from 0.2.4, 0.2.5 and 0.2.6 built from the release package (2026-10-25, build 203): 21.8 MB each
  (the 72 new translation files go whole), each applied to its old release and every one of the 524 files checked
  against the package; the check itself had to learn to make the new `locale\` folder
- 🟡 Every fixed issue answered with the release link (#5 and #6, 2026-10-25). Discussions were turned on
  2026-10-28 (with the Ideas category, where *Suggest an idea* now opens a new discussion). The announcement is
  posted with the next release. Also on 2026-10-28: the SignPath application was sent by the owner, and Weblate's
  Libre plan is pending approval.

Counted for the dashboard: 8.2 to 8.10, one line each, ⚪ excluded: 92 🟢, 5 🟡, 1 🔴 (98), of which 6 🟢 are
in the app ⏳. The 🟡 and 🔴 left need the upload (CI, the announcement, the replies), SignPath, or hardware not here.
Languages (§12) and the community (§13) have bars of their own.

## 9. Other platforms

Tracks with checklists; a version number comes with the first published build.

### Linux

- 🟢 Built and smoke-tested once before the Windows work (Docker, X11 and Wayland; `.specs/features/redesign/evidence/`)
- 🟢 The current code builds in Docker (Ubuntu 24.04, Qt 6.8.3, `linux-qt68.sh`) with no warning from the
  redesign's code (2026-10-28); `linux-bundle.sh` makes a self-contained tarball, `linux-wslg.sh` runs it through WSLg
- 🟢 Every player-bar control (59) put in the bar, shown and clicked during playback, on X11 through WSLg with the
  GPU (D3D12): no QML warning; zoom list and theme of the widget dialogs fixed
- 🟡 Video on X11 needs `--gl=glx` in the Docker container through WSLg: EGL finds no DRM device there and draws
  black (upstream code; a PC with /dev/dri uses EGL)
- 🟢 Onboarding, library (a folder indexed, thumbnails), dialogs, Settings, effects, queue, fullscreen, dark/light,
  right to left and 1.5× scale shown through WSLg with no QML warning; idle CPU 0.0 %, paused 3.5 %
- 🟢 VA-API on Intel Iris Xe (Ubuntu 24.04 server, tarball): H.264, HEVC 8/10-bit, VP9 0/2 decode in hardware,
  0.3–0.7 s of CPU for 20 s of 4K instead of 5–19 s; AV1 by VA-API crashed inside FFmpeg 6.1.1 of Ubuntu (its own
  `ffmpeg -hwaccel vaapi` too): below libavcodec 61 dav1d decodes AV1 now, every AV1 file plays
- 🟢 Memory: pages and panels opened 77 times, flat (303→308 MB); switching files grew 90 MB a minute (glibc arenas
  per thread), fixed in `bin/vlc.c` (two arenas): 30 min of files, panels and pages stay near 480–510 MB
- 🟢 Wayland (headless Weston, native client, Qt's Wayland compositor for the video): play, pause, seek, quit, no
  QML warning. The video on Wayland goes through the software `wl_shm` output with Mesa (Intel, AMD) even with a
  GPU: VLC's EGL output needs `EGL_KHR_display_reference`, which Mesa 25.2 lacks (seen on WSLg with the GPU).
  `wl_shm` drew the picture from the left with no bars, the desktop showing through on the right: fixed, the
  picture is centred in a subsurface over black (Weston and WSLg, fullscreen, windowed, next file)
- 🟢 Fullscreen on headless Weston keeps the window where it was and runs off the screen; not VLC: it asks for
  the right geometry and buffer (traced), and a plain Qt 6.8 window does the same there. To see on a real desktop
- 🟢 X11 CSD (openbox): drag the title bar to move, resize from each edge and corner, double-click to maximize
  and restore (an earlier "moved instead of resized" was the test clicking a hidden helper window)
- 🟡 Xvfb (no DRI3) can't show VA-API pictures directly: copied to software (1080p 84 % CPU with llvmpipe)
- 🟡 Docker build in CI: `.github/workflows/build-linux.yml` builds on `ubuntu-24.04` (x86_64) and
  `ubuntu-24.04-arm` (aarch64), makes the bundle and the packages, then on a clean runner installs the .deb with apt
  next to the distribution's `vlc`, runs `vlc-redesign --version` and plays a clip to its end headless (dummy
  interface, video and audio), does the same with the AppImage (`--appimage-extract-and-run`) and with the .rpm in
  Fedora 44 (dnf), and removes the .deb. Runs once the branch is pushed; `linux-qt68.Dockerfile` takes Qt's ARM64
  build (`linux_gcc_arm64`) on aarch64, `linux.sh` has `DEPS_ONLY=1`
- 🟡 Packages (2026-10-28, branch `feature/linux-pkg`): `linux-packages.sh [--arch x86_64|aarch64]` makes from the
  bundle `vlc-redesign-X.Y.Z-ARCH.AppImage` (+ `.zsync`), `vlc-redesign_X.Y.Z_amd64.deb`,
  `vlc-redesign-X.Y.Z-1.x86_64.rpm`, `vlc-redesign-X.Y.Z-linux-ARCH.tar.xz` and `SHA256SUMS-linux-ARCH.txt`, with
  appimagetool 1.9.1 (type2 runtime 20251108) and nfpm 2.47.0, downloaded and checked by SHA-256. The .deb and .rpm
  put VLC in `/opt/vlc-redesign` and the command `/usr/bin/vlc-redesign`, the desktop entry, metainfo and icons as
  `org.kuchisofts.VLCRedesign`: no file of the distribution's `vlc`. Dependencies from the libraries the bundle
  takes from the system (glibc 2.38, libstdc++ of GCC 13, GL/EGL/GBM/DRM, VA-API, VDPAU, udev, systemd); the rpm
  asks for sonames and provides nothing (its bundled Qt doesn't satisfy other packages). x86_64 made and checked
  here (dpkg-deb, rpm in Fedora, the AppImage unpacked): AppImage 165 MB, .deb 155 MB, .rpm 155 MB, tarball 126 MB.
  🔴 Installed and run on GitHub, ARM64 built there
- 🟡 Flatpak: `flatpak/org.kuchisofts.VLCRedesign.yml` written by the script (KDE 6.8 runtime; libebml, libmatroska,
  libdvbpsi, taglib, medialibrary of the contribs; network, Wayland/X11, PulseAudio, DRI, the video and music
  folders, `/media`, screen saver and MPRIS only). 🔴 Not built yet (flatpak-builder), then Flathub
- 🟡 glibc 2.38 or later (Ubuntu 24.04, Debian 13, Fedora 39): built on Ubuntu 24.04. Ubuntu 22.04 and Debian 12
  need a build on an older base
- 🔴 X11, Wayland; PipeWire, PulseAudio; VA-API, VDPAU; Intel, AMD, NVIDIA
- 🟡 Updates: the AppImage carries `gh-releases-zsync|KuchiSofts|vlc-redesign|latest|vlc-redesign-*-ARCH.AppImage.zsync`
  (AppImageUpdate); file associations: the desktop entry has VLC's MIME types. 🔴 The in-app updater on Linux,
  casting firewall notes
- 🔴 Tested on Ubuntu, Fedora, Debian, Arch (VMs and a real PC)
- 🔴 Pixel check of every frame

### macOS

Started 2026-10-28 (branch `feature/macos`); the analysis, decisions and what is left:
[`extras/juxtopposed/macos/README.md`](../extras/juxtopposed/macos/README.md). Built and launched on GitHub's Macs
(2026-10-28); the 0.4.0 checks run first at the upload.

- 🟢 Native build on Apple Silicon and Intel: `extras/juxtopposed/macos-build.sh` (VideoLAN's prebuilt macOS
  contribs of the newest upstream contrib commit, the official Qt 6.8.3 by aqtinstall — the contribs of macOS have
  no Qt —, `--disable-macosx --enable-qt` so the Qt interface is the only one, `VLC Redesign.app` with
  `macdeployqt`, every library found inside the bundle); two disk images, not one universal app (contribs per
  architecture; `lipo` later if wanted). The workflow `build-macos.yml` (`macos-14` arm64, `macos-15-intel`
  x86_64: GitHub retired `macos-13`) builds both on every pull request, uploads the `.dmg` and runs
  `ci/macos-smoke.sh` (the bundle's signatures, icon, translations, Live TV plugin, About badge and credits;
  headless playback, the Qt interface and its unified title bar, no crash report, no QML warning; screenshots light,
  dark and About); free on the public repository, by hand only (Apple Silicon, 40 minutes) on a private one;
  `continue-on-error` until the 0.4.0 checks pass once
- 🟢 Bundle `org.kuchisofts.vlcredesign` (its settings, library and caches apart from VLC's), macOS 12+, upstream's
  file types imported with the rank *Alternate*, the icon from the designer's logo (`macos/AppIcon.icns`), the Live TV
  add-on (Settings turns it off: a disk image has no components), the translations, the texts and licences
- 🔴 Signed and notarized (decision 3): ad hoc until then (Gatekeeper: *Open Anyway* in Privacy & Security); the
  script already signs with the hardened runtime and notarizes when given a Developer ID
- 🟡 Menu bar (a minimal native one: About, Preferences, Quit, open, help), a unified title bar (the traffic lights
  in the redesign's top bar, `macos_titlebar.cpp`), ⌘
  hotkeys reach VLC as `Command+`, the CJK fonts of the interface language, the updater shows the release page and
  only for a release with this Mac's disk image (all in code, not run on a Mac); 🔴 media keys, Now Playing,
  VideoToolbox and the video in the window checked, the dock menu, in-app updates
- 🔴 Pixel check of every frame

### Windows on ARM64

- 🟢 Build with VLC's ARM64 toolchain and contribs (2026-10-28, branch `feature/winarm64`):
  `win64-docker.ps1 -Arch aarch64`, llvm-mingw `vlc-debian-llvm-ucrt:20260611225331` (`winarm64.Dockerfile`) with
  the prebuilt contribs of upstream's `win64-arm-llvm` job, UCRT, `-S 0x0A000006`; volume `vlc-winarm64-build`,
  package in `~\source\vlc-redesign-build-arm64`; about 9 minutes. The redesign's code needed no change; every
  binary is ARM64 (`0xAA64`). No OpenGL plugins (none in the ARM64 contribs; Direct3D 11 draws the video)
- 🟡 Installer, zip and updates for ARM64: `make-release.ps1 -Arch aarch64` makes the files of 13.5 and
  `-winarm64-delta-from-` small updates, one `SHA256SUMS.txt` for both architectures; the `.iss` with `/DArm64`
  (`ArchitecturesAllowed=arm64`, the same AppId); the updater takes the files of its own architecture only. Not run
  on ARM64 Windows yet; the package has no plugin cache when made here (made at the first start, or by Setup for an
  install for everyone)
- 🟢 Checked statically (`ci/pe-check.py`, 2026-10-29): every binary ARM64, every import in the package or in
  Windows, Qt and its QML built in, the About badge, the Live TV add-on, the translations, the credits; the
  Windows build runs it on the ARM64 package
- 🔴 Tested on ARM64 Windows: `ci/arm64-smoke.ps1` (binaries, plugin cache, launch test) and
  `ci/arm64-install-test.ps1` (the installer installed, the smoke test on it, screenshots, uninstall) run in the
  Windows build workflow on `windows-11-arm`, public repository only: at the upload

### Android (phones, tablets, TV)

A fork of [VLC for Android](https://code.videolan.org/videolan/vlc-android) (GPL, Kotlin, libVLC) with its screens
redrawn: the desktop's Qt interface is made for a mouse and a big window. The Figma file has desktop frames only, so
phone and TV layouts need frames of their own (decision 6).

- 🔴 Build VLC for Android in Docker and reproduce its release
- 🔴 Package `org.kuchisofts.vlcredesign`, English by default
- 🔴 Design tokens from the hub; the player: control bar, seek previews, volume, gestures
- 🔴 Library, queue, playlists, settings; themes; 9 accents
- 🔴 Casting with the desktop's fixes; subtitles
  - Reviewed 2026-10-29 (code only, no phone): the app builds against libvlc 3.7.7, so the desktop's
    `renderer-*` variables do not exist there. To fix in the phone session, by priority:
    1. **High**: stopping a cast never brings the video back to the phone (`PlaybackService.setRenderer()` tests
       `wasOnRenderer && !hasRenderer()` before `setRenderer(item)`: always false).
    2. **High**: no message when a cast fails (device off, unreachable, format refused): only the generic
       *Invalid location* path. A cast state (connecting / playing / failed + why) with *Try again* / *Play here*.
    3. **High**: the same TV found by two discoveries is listed twice (`RendererItem.equals` compares the native
       object): de-duplicate by name and type in `RendererDelegate`.
    4. **High**: discovery runs for the whole process while the network is up, in the background too, and `stop()`
       never `release()`s the discoverers (a leak per network change): scan only while a picker or the player
       shows, release on stop.
    5. **Medium**: the device playing can vanish unnoticed (`ItemDeleted`, network loss) with no message.
    6. **Medium**: the cast button is hidden until a device is found; no scanning state, no empty state.
    7. **Medium**: no *Sound only*: `RendererItem.flags` has no getter (add `canVideo()`), DLNA TVs get the
       "unknown" icon.
    8. **Medium**: picker rows: highlight not reset on recycled rows, `renderers[position]` vs `dataset` size, only
       the name clickable, *Connected* shown before any connection, one device casts at once with no picker.
    9. **Medium** (to check on the phone): `:no-sout-chromecast-video` is added to every media in
       `PlaylistManager`, whatever the *Audio only* setting.
    10. **Medium**: passthrough default `false` for the instance, `true` for the media; VLC 4 builds skip the
        casting options.
    11. **Medium**: the casting screen (`player_remote_control.xml`) shows a cone only: no device name, no *Stop
        casting*; volume keys change the phone's volume, not the TV's.
    12. **Low**: subtitles blocked while casting (the Chromecast output can burn them in); hard-coded
        *Disabled* / *Enabled* and a debug toast; left/right instead of start/end in the cast layouts;
        `colorPrimary` instead of the accent; a fixed `minWidth="384dp"` dialog.
  - UI parity: none yet. The phone has the old *Displays* dialog (names, a Chromecast or unknown icon, a
    *Disconnect* button); the desktop panel (*Cast to*, scanning dot, *This computer* row, TV / music icons,
    *Sound only*, check mark, *Connecting…*, hide device, empty states, the casting card with *Stop casting*,
    *Try again*, *Play here*) is to be built as a redesign bottom sheet with the desktop's English texts.
- 🔴 Android TV / Google TV layout (remote control)
- 🔴 APK and AAB on GitHub, then F-Droid; Google Play only after decision 4

### Samsung Smart TV (Tizen)

Samsung TVs run Tizen; VLC has no Tizen version and native apps are for Samsung partners only, so the engine can't run
on the TV. A Samsung Galaxy phone is Android (track above).

- 🟢 Casting from the PC (DLNA) works on the UE55MU8000: pause kept, as-is in MPEG-TS, converted when stopped
- 🟢 Developer mode is on on the test TV, but bound to 127.0.0.1
- 🔴 Developer mode set to the PC's address (only at the TV: Apps → 1 2 3 4 5) (decision 5)
- 🔴 Tizen Studio, a Samsung certificate tied to the TV, a hello-world web app installed
- 🔴 Receiver app: plays a URL from the desktop with the TV's player (AVPlay), the redesign's controls, remote keys
- 🔴 The desktop finds the receiver and uses it instead of DLNA (seek, subtitles as text, tracks)
- 🔴 Samsung store submission, if wanted (seller account and review)

### iPhone, iPad, Apple TV

- 🔴 A fork of VLC for iOS (MPL/GPL) built in CI
- 🔴 Screens in the redesign's look, reusing the Android work and the hub's tokens
- 🔴 Apple Developer account and App Store review, where the VLC name may be refused (decision 3)

## 10. Other implementations of the design

[dsojevic/juxtopposed-vlc](https://github.com/dsojevic/juxtopposed-vlc), another VLC 4 fork of the same design (one
author, since 2026-10-13), is ahead on:

| Their feature                                                       | Worth adding | Where here               |
|---------------------------------------------------------------------|--------------|--------------------------|
| Windows, macOS, Linux from one workflow, launch tests of packages   | Yes          | §9 tracks, §8.9 CI       |
| 11 languages, language change without restart                       | Yes          | §12                      |
| Real blur behind floating panels                                    | Yes, carefully | §8.8                   |
| Classic dialogs restyled with one Qt style hook                     | Yes          | §8.6 (done)              |
| Redesigned Go to time, statistics, folder info, plugins list        | Yes          | §8.6 (idea)              |
| Geometry, overlay, advanced video filters in the panel              | Yes          | §8.8                     |
| Subtitle cues on the seek bar                                       | Nice to have | §8.8 (built)             |
| About 85 automated QML and C++ tests                                | Yes          | §8.9                     |

We are ahead on: the installer, delta updates with rollback, casting (a real TV), Windows integration, surround
sound, subtitle providers, weekly upstream merges.

## 11. Pixel-exact process

1. **Reference:** each frame exported 1:1 from Figma at 100 % and 200 % (prototype view:
   `scaling=min-zoom&hide-ui=1&show-proto-sidebar=0`); the frames and their texts are in `inventory.json`.
2. **Capture:** the app in the same state (window size, media, theme, accent), at the same scale, from a fixed demo
   folder.
3. **Compare:** the largest channel difference per pixel (grey levels hide colour fringes). A frame passes when no
   pixel differs beyond a small tolerance, except listed anti-aliased edges.
4. **Record:** result, both captures and the difference image go to the wiki's *Design check* page and to the *Pixel*
   column of section 5; failures become *Design mismatch* issues.
5. **Keep it:** the comparison runs in CI on every pull request.

Text matching needs Inter with the same metrics (distance-field text, no hinting, Figma's line box: section 6).

- 🟢 References and captures of 187:328 (player) and 970:2703 (library shell), compared
- 🟢 Text positions and sizes measured in many other frames (cited in the code)
- 🟢 Captured at 1:1 and compared (tiles of 32 px, best shift of up to 4 px, a pixel differs above 24 levels), 2026-10-23:
  the 18 Settings frames, the onboarding (7 frames), Keyboard Shortcuts, Open network stream and New playlist;
  from the demo library (`<the QA folder>\media\figma`: the media, covers, lengths and playback states of the frames), Home,
  the S01 folder (grid, list, with the mini player), the player (alone, with the library next to it, light, without
  its bars) and the music player (alone, with its folder); Add Subtitles (to a folder, to a file), Customize
  Player, and the file, folder and playlist context menus; 26 components (menus, sort and view options, settings
  controls, the bar's controls, rows, the right-click menu). Fixed from them: the settings dropdown's list (the
  check of the design, 44 px rows, 20 px under the field), the File name matching labels, the submenus 11 px clear
  of their menu, the queue's count, the current time that is typed (1904:7238 state 3). In progress: the effects
  panels, the library panel beside the video (Home rows, Videos and Music levels), the mini player's queue. What is left differs
  by value only (the switches and choices of the test profile), by anti-aliasing, or on purpose:
  - Settings - performance: *Process priority* is a dropdown (Normal, High...), not the number field of the design.
  - Settings - privacy: the design's second "Last.fm username" row is the password (its typo); Settings - recording:
    "include subtitles" reads "Include subtitles".
  - Onboarding, the accent states (1902:4020 to 1902:4940): *Back* stays, as in 1902:3670.
  - Keyboard Shortcuts: an action with several keys shows each (Next track: N, Shift+>, >; the defaults of VLC).
  - Not in the design, after its sections: Subtitles, *Second Language & Online Subtitles*; Interface, *Player Bar*.
  - The fullscreen button is in the control bar, not the title bar (issue #1): the right group of the bar is one
    button further left.
  - The design's mock data: Figma's seek handles are not where their times are (2:10 drawn at 1:57), the title bar
    reads "VLC Media Player" over a playing film, a title has a typo ("The Wand_S01E04").
  - The mini player is where it was put (it is dragged): the design draws it at the left (970:3792) and at the
    right (970:2703, 970:3308); it starts at the left.
  - Player - Light (2425:13624) draws the video to the right edge of the window; its dark frames (187:328, 247:445)
    and the app keep the 10 px frame there.
  - Add Subtitles from OpenSubtitles (1919:9278): its controls row is identical (0 tiles); the answers of the service
    are listed in the rows of 1919:8707 (FileRow). Before the first search VLC asks for the account the service
    requires (API key, username, password), a row the design does not draw.
  - Customize Player: the other eight frames are steps of a drag in the editor (the bars as the user moves items).
- 🟢 Figma references exported: all 103 frames at 1:1 (`<the QA folder>\qa\figma`; the oversized component sheets in tiles
  joined pixel for pixel)
- 🟢 Capture scripts that drive the app into each frame's state: settings, dialogs, onboarding, library and player
  (from the demo library and its playback states), the components, and the hover states in Windows Sandbox
- 🔴 The CI job `pixel-check.yml`: captures, comparison, artifacts, a pull request comment, the wiki page
- 🟢 All 103 frames compared and recorded (2026-10-24, build 175: the last two, hover states, in Windows Sandbox
  with real mouse input inside it, scratchpad hoverrun.py; the preview's time differs in 2 tiles of anti-aliasing
  of the software renderer there, same size, place and colour)

## 12. Languages

- 🟢 English by default in the portable zip and the app, whatever the system language; *Reset all* returns to English
- 🟢 Installer in English and 27 languages (Arabic and Hebrew right to left), each with its license notice
- 🟢 The installer offers the language of Windows (only for a language with a full translation), the app starts in
  it; updates keep it, silent installs never ask (2026-10-24, tested in Windows Sandbox)
- 🟢 *Settings → Global → Appearance → Interface language*, and *Layout direction*: the direction of the language
  chosen (its catalog says it: "layout direction"), changed under it if wanted (2026-10-25, checked on build 196)
- 🟢 The language and the direction change at once, without a restart, everywhere (the welcome and the classic settings
  too; classic dialogs when they open next)
- 🟢 Every string listed for translation: vlc.pot made again from the tree (7,821), the missing files and `tr()` added;
  a word with two meanings has a context (key names, the tracks of Settings), counts are plural entries
- 🟢 Translations compiled into the release build (98 languages, catalogs made again at each build)
- 🔴 Translators' platform (Weblate or Transifex, an account of the project: the owner's step); English and Hebrew first,
  then VLC's languages (decision 7). Until then [TRANSLATING.md](TRANSLATING.md) explains how to translate, try and send
  a language, and a catalog put in `locale\` is listed by itself
- 🟢 72 languages and British spelling complete (2026-10-25), every language of the list, with over 700 corrections of
  the old catalogs. The other catalogs of VLC (Asturian, Kabyle,
  Uzbek...) are listed too, as partial, except those with next to nothing translated (Tagalog, Tetum, Azerbaijani)
- 🟢 Right to left checked page by page (Hebrew, Arabic, and a test language 40 % longer, both directions: no cut, no
  overlap), the playback bar kept left to right as on YouTube
- 🟢 Every complete language captured page by page and reviewed (2026-10-24, builds 191 and 193): the core texts the
  interface shows (settings help and choices, shortcut names, messages on the video, dialogs) translated too, Chinese,
  Japanese and Korean in the fonts of their language, long translations widen or shrink instead of being cut

Android and the TV receiver follow the same rules.

## 13. GitHub: one project, many repositories

The status of every item of this section is in its setup checklist (13.16).

### 13.1 One project or separate repositories?

The desktop program is a fork of `videolan/vlc` (about 113,000 commits). Android would fork `videolan/vlc-android`
(Kotlin, Gradle), iPhone/iPad `videolan/vlc-ios` (Swift, Objective-C), and the TV receiver is a small Tizen web app
of our own.

| Criterion          | One repository, folders                        | Separate repositories                    | Hub and spokes                              |
|--------------------|------------------------------------------------|------------------------------------------|---------------------------------------------|
| Upstream sync      | One upstream only; the others as subtrees: slow, error-prone merges | Each repository merges its own upstream | Same as separate                    |
| Clone size         | Everyone downloads every platform              | Only the platform you work on            | Same as separate                            |
| CI cost and speed  | Path filters; a change can trigger every build | Each builds its own platform             | Same, with shared workflows from the hub    |
| Releases, GPL source | Prefixed tags; one source archive for all    | Each source archive matches its binaries | Same as separate                            |
| Issues, Discussions | One place                                     | Scattered; duplicates                    | One place, in the hub, with platform labels |
| Discoverability    | One page                                       | Several pages to find                    | One entry point: hub README and website     |
| Stars              | One count                                      | Split                                    | Collected on the hub                        |
| Permissions        | Coarse                                         | Per repository                           | Per repository                              |
| Licensing          | GPL, GPL and MPL/GPL mixed in one tree         | Each keeps its upstream's license        | Same as separate                            |
| Verdict            | 🔴                                             | 🟡                                       | 🟢                                          |

**Decision: hub and spokes.**

- **Code** in one repository per upstream fork or platform family: desktop (Windows, Linux and macOS from one code
  base), Android, iOS, TV.
- **One public face**, the hub: either the existing `KuchiSofts/vlc-redesign`, which keeps its stars, or a GitHub
  organization (decision 1). A transfer or a rename keeps stars, issues and redirects old links.
- The hub holds the only Discussions, the only issue tracker for every platform (platform labels; the other
  repositories' `config.yml` has blank issues off and contact links pointing to the hub), the only public roadmap
  (one Projects board across all repositories), the website (GitHub Pages: detects the visitor's OS and offers the
  right download, lists every platform) and the shared design assets (exported once, used by every platform).
- Pull requests stay in the code repositories; an issue that belongs to one moves there with GitHub's transfer.
- The repositories stay plain repositories, not GitHub forks: GitHub search leaves forks out by default. (The hub
  already is one.)
- **Naming and trademark:** an organization name without "VLC" (for example the publisher's) avoids looking
  official; repository names keep `vlc-redesign` and every description says *unofficial*. A request from VideoLAN
  would be met with a rename, and GitHub redirects renamed repositories.

### 13.2 Repositories and their layout

| Repository                        | Content                                                                 |
|-----------------------------------|-------------------------------------------------------------------------|
| `KuchiSofts/vlc-redesign`         | Desktop (Windows, Linux, macOS), and the hub: issues, Discussions, board, website, design assets, wiki |
| `KuchiSofts/vlc-redesign-android` | Android phones, tablets, TV (fork of `videolan/vlc-android`)             |
| `KuchiSofts/vlc-redesign-ios`     | iPhone, iPad, Apple TV (fork of `videolan/vlc-ios`)                      |
| `KuchiSofts/vlc-redesign-tv`      | Samsung TV receiver (Tizen web app)                                     |

```
vlc-redesign/                  desktop code + hub (or the hub alone, in an organization)
├─ modules/gui/qt/             the redesigned interface (QML, C++)
├─ extras/juxtopposed/         build scripts, installer, updater, release scripts
├─ design/                     tokens.json, icons (SVG), fonts: the shared design assets
├─ site/                       the website (GitHub Pages)
├─ platforms.yml               every platform: status, latest version, download link
├─ docs/
│  ├─ MASTER-PLAN.md           this plan
│  ├─ wiki/                    wiki source, synced to the GitHub wiki
│  └─ media/                   screenshots and GIFs
└─ .github/                    issue and discussion forms, labels.yml, workflows

vlc-redesign-android/          fork of videolan/vlc-android
├─ (upstream folders)          kept as they are, merged weekly
├─ redesign/                   our theme and screens, apart from upstream code
├─ design/                     synced from the hub
└─ .github/                    CI, releases, config.yml → hub issues

vlc-redesign-ios/              fork of videolan/vlc-ios
├─ (upstream folders)
├─ Redesign/                   our theme and screens
├─ design/                     synced from the hub
└─ .github/

vlc-redesign-tv/               Tizen web app, our own code
├─ src/                        receiver screens, AVPlay player, remote keys
├─ design/                     tokens as CSS variables, from the hub
├─ config.xml                  Tizen manifest
└─ .github/
```

**Shared design assets** (`design/` in the hub): `tokens.json` (the values of section 6: colours dark and light,
accents, fonts, sizes, radii), icons as SVG, the fonts (OFL). Exported once and checked by the pixel check; desktop CI
compares `VLCStyle.qml` with them; Android, iOS and the TV generate their own files from them; a workflow opens a pull
request in each repository when they change. They have their own version (`design-1.0`), named in every release.
🟡 In place since 2026-10-28 ([`design/README.md`](../design/README.md)): `tokens.json` (203 tokens, 138 of them
checked against 205 places in the code by `design/tools/check_qml.py`, the *Design tokens* job of
`build-windows.yml`), `icons/` (677 files: the 116 glyphs of the Icon set as PNG per state with `@2x` and their
SVG sources, the other raster pieces and their SVGs, the SVG icons, the two-tone menu icons, `index.json` and a
preview), `fonts/` (Inter modified, DM Mono, licenses), `build/` (`tokens.css`, `colors.xml`, `Tokens.swift` from
`design/tools/export.py`). 🟢 Every raster has its SVG from Figma (2026-10-29): the 224 glyph states, cut from
Figma's SVG export of the Icon set by `design/tools/icon_svgs.py` and compared with their PNG in
[`design/tools/icon_svgs_report.md`](../design/tools/icon_svgs_report.md) (51 exact, 131 the same drawing, 42
close), and the 41 other pieces (`design/tools/raster_svgs.json`). The app draws the SVGs at the size shown times the
scale and the zoom (QML, menus, window buttons, message boxes; checked at 100/150/200 % and zoom 3), the PNGs stay
as a fallback; the app icon is a 10-size `.ico` from the vector. The sync workflow comes with the first sister
repository.

### 13.3 Every repository shows every version

- `platforms.yml` in the hub is the single source: name, status, latest version, date, download link, page.
- `extras/juxtopposed/platforms/render.py` (Python, standard library) renders it: the block between
  `<!-- platforms:start -->` and `<!-- platforms:end -->` at the top of the README (the bar, and the table in a
  *details*), `site/platforms.json` and the platform list and download box of `site/index.html` (so the page works
  without JavaScript), `--also-on <id>` for the release notes, `--check` for pull requests, `--build DIR` for Pages.
- The workflow `.github/workflows/platforms.yml` does it on `main` (commits the rendered files back when they
  changed), checks pull requests, and deploys `site/` to GitHub Pages; a pull request in each sister repository
  that replaces the same block comes when the first one exists.
- 🟢 Built 2026-10-28 (Windows 0.3.1 released, 0.4.0 next; Linux in progress; macOS and Windows ARM64 planned;
  Android and Samsung TV research; iPhone/iPad later). After each release: set `version`, `date` and `next` of
  Windows and run the renderer (`release-notes.py` reminds it).
- The platform bar at the top of every README:

```
Windows 0.2.6 · Linux soon · macOS soon · Android soon · Samsung TV soon · iPhone/iPad later
```

| Platform     | Status        | Latest | Get it                 |
|--------------|---------------|--------|------------------------|
| Windows      | 🟢 released   | 0.2.6  | installer · portable   |
| Linux        | 🔴 planned    | —      | coming soon            |
| macOS        | 🔴 planned    | —      | coming soon            |
| Android      | ⚪ research   | —      | coming soon            |
| Samsung TV   | ⚪ research   | —      | coming soon            |
| iPhone/iPad  | ⚪ later      | —      | coming soon            |

- Release notes end with *Also on: …* from the same file (🟢 `release-notes.py` imports the renderer).
- In the app, About and the update card show *Also on: …* from a copy of `platforms.yml` built into each release:
  nothing is fetched for it. 🟡 About: one muted line under *Built on VLC media player by…*
  (`RedesignUpdater.alsoOn`, from `site/platforms.json` in `assets.qrc` as `:/redesign/platforms.json`; About is
  outside the Figma file, §5.15). The update card keeps its three lines for now.

### 13.4 Versions

- Each program has its own SemVer (`MAJOR.MINOR.PATCH`) and tags `vX.Y.Z` in its repository.
- Why not one shared number: programs ship on different days (store reviews, upstream releases); one number would
  force empty releases or hold fixes back, and a number per program tells its users exactly what changed for them.
- The hub shows the matrix: program, latest version, date, upstream base (VLC 4.0-dev commit, VLC for Android
  version…), design assets version.

### 13.5 Releases

| Platform      | Files                                                                                         |
|---------------|-----------------------------------------------------------------------------------------------|
| Windows       | `vlc-redesign-X.Y.Z-win64-setup.exe`, `vlc-redesign-X.Y.Z-win64.zip`, `.vrdelta` small updates, `SHA256SUMS.txt`, `SHA256SUMS.txt.sig` |
| Windows ARM64 | `vlc-redesign-X.Y.Z-winarm64-setup.exe`, `vlc-redesign-X.Y.Z-winarm64.zip`                    |
| Linux         | `vlc-redesign-X.Y.Z-x86_64.AppImage` (+ `.zsync`), `vlc-redesign_X.Y.Z_amd64.deb`, `vlc-redesign-X.Y.Z-1.x86_64.rpm`, `vlc-redesign-X.Y.Z-linux-x86_64.tar.xz`, `SHA256SUMS-linux-x86_64.txt`; the same for `aarch64` (`arm64` .deb); Flathub |
| macOS         | `vlc-redesign-X.Y.Z-macos-arm64.dmg`, `vlc-redesign-X.Y.Z-macos-x86_64.dmg`                   |
| Android       | `vlc-redesign-X.Y.Z-android-arm64.apk` (and armv7, x86_64), `.aab`                            |
| Samsung TV    | `vlc-redesign-tv-X.Y.Z.wgt`                                                                   |

- Every release: checksums, signed binaries where possible, the source archive (GPL) and exact commit, licenses.
- Channels: GitHub first; then winget, Flathub, F-Droid.
- **Release notes come from the changelog**, built by the release script: a one-line summary and a hero image;
  up to five highlights, each with a screenshot or GIF (MP4 beside it); *Fixed / Changed / Added* with linked issues
  and reporters thanked; the download table with sizes; checksums and source; credits (design, contributors,
  translators, testers); *Also on*; a link to the release's Announcements post.

### 13.6 README (every repository)

1. Platform bar (13.3), title, one-line description, download buttons, demo GIF.
2. Design credit (Juxtopposed, CC BY 4.0) with the video; unofficial notice.
3. Features with screenshots; download and install; updates.
4. *Suggest an idea · Report a problem · Roadmap · Discussions* (all in the hub).
5. Known limitations, build from source, contributing, credits, license.

### 13.7 Discussions (hub)

| Category          | Format                  | For                                              | How it is used                                         |
|-------------------|-------------------------|--------------------------------------------------|--------------------------------------------------------|
| 📣 Announcements  | maintainers post        | Releases, plans, calls for testing               | One post per release; a monthly roadmap summary        |
| 💡 Ideas          | open, with votes        | Suggestions and requests                         | Form: problem, idea, frame; accepted → issue on the board |
| 🙋 Q&A            | question / answer       | How do I…, a file, a TV, a setting               | Answers marked; asked twice → a wiki page              |
| 🖼️ Show and tell  | open                    | Setups, profiles, accent and layout combinations | The best go to the README and notes (with permission)  |
| 🎨 Design         | open                    | Frame talk, phone and TV layouts, mismatches     | Each thread names its node id; Juxtopposed invited     |

Category forms: `.github/DISCUSSION_TEMPLATE/ideas.yml`, `design.yml`, `q-a.yml`.

### 13.8 Issue forms (hub)

Blank issues off; each form asks to search first and sets its labels. *Prefilled* fields come from the app's
*Report a problem* link (13.12).

| Form                    | Status | Fields                                                                                  | Labels                      |
|-------------------------|--------|-----------------------------------------------------------------------------------------|-----------------------------|
| Bug report              | 🟢     | version, install type, Windows (today); to add, prefilled: GPU and driver, screens, video output; steps, media, log, screenshots | `bug`         |
| Feature request         | 🟢     | area, what you can't do, the idea, in the design or new; to add: alternatives            | `suggestion`                |
| Design mismatch         | 🔴     | frame (node id, link), theme, accent, scale, app and Figma screenshots, what differs     | `design`                    |
| TV / device report      | 🔴     | brand, model, year, firmware, DLNA or Chromecast, file; works: start, pause 1/5 min, seek, next, subtitles, as-is, end | `device report` |
| Translation             | 🔴     | language, kind (wrong, missing, new language, join), screen, current and suggested text  | `translation`               |

`config.yml` (🟢 today: official VLC, design video; to add: Q&A, Ideas, security policy). A pull request template asks
for what and why, before/after screenshots, the node id, the tests run, and whether code outside `modules/gui/qt`
changed (then *Differences from upstream VLC* is updated).

### 13.9 Labels

Kept in `.github/labels.yml`, synced to every repository by a workflow.

| Group     | Labels                                                                                              |
|-----------|-----------------------------------------------------------------------------------------------------|
| Type      | `bug`, `suggestion`, `design`, `device report`, `translation`, `docs`, `question`                    |
| Platform  | `platform: windows`, `windows-arm64`, `linux`, `macos`, `android`, `samsung-tv`, `apple`             |
| Area      | `area: player`, `library`, `customize`, `settings`, `subtitles`, `casting`, `audio`, `video`, `installer-updates`, `design`, `translations`, `engine` |
| Priority  | `priority: P0` (crash, data loss, security), `P1` (main feature broken), `P2`, `P3`                  |
| Status    | `status: needs triage`, `needs info`, `accepted`, `in progress`, `in next release`, `upstream`, `duplicate`, `won't do` |
| Community | `good first issue`, `help wanted`                                                                   |

### 13.10 Public roadmap

One public GitHub Projects board, **VLC Redesign roadmap**, across all repositories:

- Fields: Status (⚪ Ideas · 🔴 Planned · 🟡 In progress · 🟢 In the next release · 🚀 Released), Platform, Area,
  Priority, Votes, Release.
- Fed by labels: a workflow adds issues labelled `suggestion`, `design` or `status: accepted`; the board's rules move
  merged fixes to *In the next release* and shipped ones to *Released*.
- Votes: a daily workflow copies each issue's 👍 (and its *Ideas* thread's) into *Votes*, since a board can't sort by
  reactions.
- Views: *Board* (by status), *Most wanted* (by votes), *Next release*, one per platform.
- `ROADMAP.md` stays a short summary linking to the board and to this plan.

### 13.11 Triage and answers

| What                                   | Goal                                                                  |
|----------------------------------------|-----------------------------------------------------------------------|
| First answer to a bug                  | 2 days; crash, data loss or security: 1 day                           |
| First answer to an idea or mismatch    | 7 days                                                                |
| Triage                                 | Every Monday after the upstream sync: type, platform, area, priority  |
| Ideas with 10 👍 or more               | Decided within 14 days: accepted, needs design, later, won't do (with reasons) |
| *Needs info* without reply             | Reminder after 7 days, closed kindly after 14 (reopened on reply)     |
| Fixed                                  | Answered with the release that has the fix; closed when it ships      |

Saved replies keep answers consistent; duplicates are linked to the first report, whose votes grow.

### 13.12 In-app *Suggest an idea* / *Report a problem*

- Two entries at the end of the ⋯ menu (`MainMenu.qml`), and in About.
- *Suggest an idea* opens a new *Ideas* discussion in the hub (`discussions/new?category=ideas`), with the version.
- *Report a problem* opens the bug form (`issues/new?template=bug_report.yml`) prefilled through the link: version
  and commit, install type, Windows edition and build, CPU architecture, GPU and driver, video output and decoding,
  screens (resolution, scale, refresh rate, HDR, count), theme, accent, interface language.
- No personal data: no file names or paths, no user or computer name, no address, no library, no logs (a log is
  attached by the user, from *Settings → Global → Diagnostics → Save a log*).
- A card shows exactly what goes into the link, with *Copy* and *Open GitHub*; the user submits on GitHub; the app
  sends nothing itself. The link stays under 8,000 characters.
- 🟡 Done 2026-10-28 (branch `feature/feedback`, not built yet): `util/redesign_feedback.cpp` (singleton
  `RedesignFeedback`), the card `RedesignReportDialog.qml` (opened from `SetupDialogs.qml`), the two links in About.
  Until Discussions are on, *Suggest an idea* opens `issues/new?template=feature_request.yml` (one constant,
  `ideaForm`); the form got an optional `version` field and the bug form an `environment` field (System
  information) that carries the whole text. The log is under *Settings → General → Diagnostics* (the sidebar has
  no *Global*).

### 13.13 Wiki

- Source in `docs/wiki/` (one Markdown file per page, `_Sidebar.md`, `_Footer.md`), reviewed in pull requests;
  `wiki-sync.yml` pushes it to the GitHub wiki on every merge to `main`; the wiki is read-only for the public; a link
  check runs on pull requests touching `docs/`.
- Pages: Home · Download and install · First run · Player · Music · Library and playlists · Queue · Subtitles ·
  Casting (DLNA, Chromecast, Samsung, TV compatibility) · Customize · Themes and accents · Settings (one page per
  section) · Shortcuts · Languages · Updates · Suggestions and reports · Troubleshooting · Building from source ·
  Design check · Differences from VLC · Release process · Credits.
- **Design check** is generated from the frame tables of section 5 (node id as key): size, status, differences,
  latest capture, Figma reference, difference image, build checked.

### 13.14 Screenshots and GIFs

- The pixel-check job runs the packaged app with the demo media at fixed sizes and states: each pull request gets
  fresh captures as artifacts and a comment listing the frames that changed, with difference thumbnails.
- On `main` the captures refresh `docs/media/` (README screenshots, 9 accents in both themes) through a pull request.
- GIFs with an MP4 each (player, previews, customize, casting, installer, update card), recorded from the real
  program with free demo media at one window size; never personal files. The same set for Android and the TV.

### 13.15 Community files and credits

| File                               | Content                                                                         |
|------------------------------------|---------------------------------------------------------------------------------|
| `CONTRIBUTING.md`                  | Build, where code lives, style and `qmllint`, commits (`area: change`), picking an issue, the design rule (cite the node id), the upstream rule |
| `CODE_OF_CONDUCT.md`               | Contributor Covenant 2.1, with a contact (decision 8)                           |
| `SECURITY.md`                      | Latest release supported; private reports through security advisories; answer in 3 days; scope: updater, installer, casting server, subtitle downloads |
| `.github/pull_request_template.md` | See 13.8                                                                        |
| `CREDITS.md`                       | Design, first implementation (MrDouglax), VideoLAN, fonts; to add: contributors, translators, testers with their TV models |

In an organization, a `.github` repository gives every repository these files by default.

### 13.16 Setup checklist

- 🟢 Issue forms: bug report and feature request; blank issues off; contact links
- 🟢 Windows build of every pull request and of `main`
- 🟢 Weekly upstream sync (pull request, or an issue listing conflicts)
- 🟢 Wiki with every screen and option
- 🟢 `CREDITS.md`
- 🟢 `ROADMAP.md` with 👍 voting
- 🔴 Hub: the existing repository or an organization (decision 1)
- 🟢 `platforms.yml` and the platform bar in the README (2026-10-25; the sister repositories read the same file)
- 🟡 Website on GitHub Pages with OS detection (`site/`, 2026-10-28; the owner enables Pages: Settings → Pages →
  Source: GitHub Actions)
- 🟡 Shared design assets in `design/` (2026-10-28: tokens, icons and their SVG sources, fonts, generated
  CSS/Android/Swift, CI check; left: the sync workflow to the sister repositories)
- 🔴 Discussions on, five categories and their forms
- 🟢 Issue forms: design mismatch, TV report, translation (2026-10-25)
- 🟢 `labels.yml` and its sync workflow (ran on the 0.3.0 push)
- 🔴 Roadmap board across repositories, with the votes workflow
- 🟢 `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, pull request template (2026-10-25; saved replies are a
  GitHub setting of the owner's account)
- 🟡 In-app *Suggest an idea* / *Report a problem* (2026-10-28, branch `feature/feedback`, to build and test)
- 🟢 `docs/wiki/` with its *Design check* and *TV compatibility* pages, and `wiki-sync.yml` (ran on the 0.3.0 push)
- 🟡 Release notes script (`extras/juxtopposed/release/release-notes.py`, 2026-10-25); the pixel-check job with
  automatic screenshots needs a Windows runner with a display: later

## 14. Quality gates for every release

| Gate         | What                                                                              | Status |
|--------------|-----------------------------------------------------------------------------------|--------|
| Build        | Every platform builds in CI; the packaged app starts and plays                    | 🟢 Windows: build and launch test green on GitHub (2026-10-25) |
| Regression   | Playback, pause/seek, traces, Chromecast and DLNA test devices; again 2026-10-24 (builds 164, 170, 174, 179 and 181): clean quits, no dropped picture (one to four shown a few ms late at a start or a seek, as in every build), both casts | 🟢     |
| Real devices | Samsung TV: start, pause 1 and 5 min, seek, next file, subtitles, as-is, end; Chromecast Ultra 2026-10-25 | 🟢     |
| Formats      | Every test file and container; 148 damaged files without crash or hang            | 🟢     |
| Effects      | 40 video effects alone and in chains                                              | 🟢     |
| Conversion   | Every muxer, audio + video, to the last picture                                   | 🟢     |
| Memory       | 25-minute media-change soak and cast soak: flat after warm-up; 2026-10-24 (build 163) the same soak against the 23 October build: +0.27 against +0.34 MB/min at 100 changes a minute (driver), handles, GDI and threads flat; the library panel opened and browsed 60 times: flat | 🟢     |
| Hardware     | NVIDIA, AMD, Intel; no GPU; weak CPU; Windows 10 and 11; high DPI; 2026-10-24 (build 164) the nine graphics setups (D3D11, WARP, OpenGL, Qt software, no hardware decoding, GL and D3D9 outputs, the win7 and dummy compositors) play and quit cleanly; no import newer than Windows 10 1809, AVX only in CPU-dispatched code | 🟡 no Intel, no real Windows 10 |
| Clean PC     | Install and first start on a clean Windows (Sandbox); again 2026-10-24 (build 163): window in 1.24 s, 1080p without a GPU 75 % of one core, quit 0.5 s | 🟢     |
| Updates      | Small updates from the three previous releases, checked file by file; 2026-10-25 from the 0.3.0 package | 🟢     |
| Design       | Pixel check of the frames the release touches; frame table updated                | 🟢 all 103, the 2 hover frames in Windows Sandbox (§8.8) |
| Languages    | English by default; installer offers Windows' language; switch at once, right to left | 🟢 |

## 15. Risks

**Trademark.** VideoLAN objects to the VLC name or cone.
Mitigation: *unofficial* everywhere; no store listing without their agreement; a neutral fallback name chosen in
advance; renamed repositories keep their links.

**Store policies.** Google Play, the App Store or Samsung refuse the app.
Mitigation: GitHub and F-Droid first; stores only after decision 4; every listing says unofficial and credits the design.

**Upstream changes.** VLC 4.0-dev or Qt breaks the build or a behaviour.
Mitigation: small weekly merges with CI; our changes kept in `modules/gui/qt`; core changes listed and offered
upstream; releases made from a known-good merge.

**Hardware coverage.** Bugs on Intel graphics, old PCs or other TVs that we can't see.
Mitigation: Sandbox and VM tests; the device report form; testers credited; an Intel test machine; release
candidates announced in Discussions a few days before a release.

**Single maintainer.** Work stalls, knowledge is lost.
Mitigation: this plan, `CONTRIBUTING.md` and the release process in the wiki; a script for every release step;
secrets kept in GitHub, not on one PC; a second maintainer with release rights.

**Design source changes.** The community Figma file changes or disappears.
Mitigation: the dated inventory, reference exports in the repository, the exported design assets.

**Update security.** A download is tampered with.
Mitigation: HTTPS from GitHub only and SHA-256 checks (done); the checksums signed by the project's release key
and checked by the app (done, from 0.4.0); code signing (decision 2); `SECURITY.md`.

## 16. Definition of done

**A frame** is done when every element and state is implemented; it works in dark and light, with the 9 accents,
at 100 % and 200 %; it has keyboard and screen-reader names; its pixel check passes and is recorded in *Design
check*: its row reads 🟢 with Pixel 🟢.

**A feature** is done when it works end to end on a clean PC; its settings survive a restart; it is usable with the
keyboard; its strings are listed for translation; it adds no QML warning; the regression suite covers it; the wiki
and the changelog describe it.

**A platform** is done when it builds in CI from a clean checkout; it installs, starts, plays and updates; it starts
in English; it uses the design assets and its frames are pixel-checked; the quality gates are green on it; its
assets, checksums and source are published; it is in `platforms.yml`.

**A release** is done when the quality gates are green; the changelog has its section; the notes have their media;
the assets are named as in 13.5 and the small updates are checked; it is announced; the fixed issues are answered;
`platforms.yml` and the board are updated.

## 17. Decisions for the project owner

1. **Hub:** keep `KuchiSofts/vlc-redesign` as the hub, or create a GitHub organization (name without "VLC").
2. **Code signing:** decided 2026-10-24: SignPath Foundation's free signing for open source (the publisher shown
   is SignPath Foundation); the user applies, the CI signs releases (docs/code-signing.md).
3. **Apple Developer account** (a yearly fee) for a notarized macOS app and any iOS work.
4. **Google Play:** whether to try (the VLC name may be refused); F-Droid and GitHub need no account.
5. **Samsung TV:** setting the PC's address in Developer mode (only at the TV); whether to aim for the Samsung store.
6. **Phone and TV layouts:** ask Juxtopposed for frames, or draw them from the design system (credited, marked derived).
7. **Languages** to ship first after English and Hebrew.
8. **Community contact:** the address for conduct reports, and who answers security reports.
9. **Turning on** Discussions and the public roadmap board.
