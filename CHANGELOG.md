# Changelog

Changes of **VLC Redesign**, an unofficial VLC 4 build with the interface designed by
[Juxtopposed](https://www.youtube.com/@juxtopposed) ([video](https://www.youtube.com/watch?v=ERFeOgqk324)).
For changes to VLC itself, see [NEWS](NEWS).

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Each section groups changes by
what you see first, with the technical details after. Based on VLC `4.0.0-dev`, merged up to `942cdaea17` (2026-10-19).

**Contents:** [0.2.4](#024--unreleased) · [0.2.3](#023--2026-10-20) · [0.2.2](#022--2026-10-20) · [0.2.1](#021--2026-10-20) · [0.2.0](#020--2026-10-20) · [0.1.0](#010--2026-10-20) · [M3](#m3--dialogs-settings-and-playback-tools--2026-10-13) ·
[M2](#m2--library-playlists-and-subtitles--2026-10-13) · [M1](#m1--first-native-shell-and-player--2026-10-12) ·
[M0](#m0--baseline-and-build--2026-10-12) · [Differences from upstream VLC](#differences-from-upstream-vlc)

---

## [0.2.4] · Unreleased

Updates without the installer: a few hundred kilobytes in the background, and a restart of a second or two.

### Added

- **Small updates**: a release carries the changes from each of the three releases before it, most files as binary
  patches, so an update downloads only what changed (0.2.2 → 0.2.3 would have been 1.9 MB instead of the 55 MB
  installer). It downloads in the background with its progress in the card and in About, is checked against the
  release's `SHA256SUMS.txt`, and the new files are written next to the old ones, each checked against its SHA-256,
  on a worker thread: the player never waits.
- **Restart now** or **Later**: *Restart now* puts the update in place and opens VLC Redesign again on what was playing,
  where it was, in about two seconds. *Later* puts it in place when you close VLC Redesign, or the next time.
- **The portable copy updates itself too**, its `portable` folder (your settings and library) left as it is.
- **Nothing half-done**: if a file can't be replaced (another program holds it), every file goes back as it was and
  the update is tried again next time. If a file of VLC Redesign was changed since it was installed, the update says
  so and the installer does it instead. When no small update fits (more than three releases behind, or installed for
  everyone in Program Files), *Update now* uses the installer, as before.
- Earlier versions can't apply a small update, so the update to 0.2.4 itself comes with the installer (or the zip):
  small updates start with the release after it.

### Fixed

- **Reopening after an update** started the media as a clip from where it was: the time began at 0:00, the length was
  shortened, and the part before couldn't be reached (VLC 4's `:start-time`). It now seeks there, without the
  *Continue where you left off?* card.
- A binding loop in the keyboard shortcut dialog's layout.
- **Memory and threads growing with every video**: each video opened (the next item of a playlist, the next episode)
  left its Direct3D 11 device behind, with its driver's threads and memory (about a hundred threads on an NVIDIA
  card). A playlist of short clips reached 7,000 threads and closed after a few minutes. The device now goes with the
  video: the count stays flat, and VLC starts with 40% fewer threads. VLC 4's own nightly builds do the same.
- **A crash when changing tracks quickly**: the Windows audio output was stopped while its thread still played the
  stream (in VLC 4 itself), and VLC closed after a few seconds of skipping through short items.
- **A video output that can't work here** (Direct3D 9 or OpenGL chosen in *Settings › Video*) showed no picture and
  kept a processor core busy trying again for every frame. The outputs that work are tried next.
- **Folders named in another alphabet** (a Windows user name in Hebrew or Chinese, where VLC Redesign installs): the
  Lua scripts didn't load from there (YouTube and other links, artwork from the web, the services of the sidebar).
- **Hardware decoding first** didn't hold for conversions (casting with subtitles, mosaics): without a hardware decoder
  they lost the picture. It now only applies to playback.
- **Casting**:
  - Stop or Next as the Chromecast paused could freeze the player.
  - A TV that was off was reported as having refused, after a long wait for every step.
  - A TV that joined an MP4 or Ogg stream from its start got its header twice and could refuse it.
  - A subtitle chosen for a sound cast restarted the TV's stream.
  - A Chromecast found over IPv6 hid its working address.
  - Stopping one media also cut the end of the next ones.
  - *Allow* in the cast panel on a Public network asked again and again: it now allows public networks too (the
    button says so) and never adds the rule twice. The firewall check no longer holds up the interface.
- **Subtitles from the services**:
  - An archive could unpack to gigabytes: only its subtitles are unpacked now, 16 MB at most, away from the
    interface.
  - A file name from a service could point outside the cache.
  - A subtitle stayed "loading" when another title was chosen.
  - A search could end before every service had answered.
  - The cache keeps the last 500 media, and drops subtitles unused for 60 days.
- **Updates**: a stalled download waited 30 minutes, and couldn't be cancelled. Now each step waits 30 seconds, and the
  card's × cancels it. Downloads follow the system's proxy, only HTTPS files from GitHub are used, and a small update
  that failed isn't tried again: the installer is.
- **Interface**:
  - In fullscreen, "f" or Esc typed in the library search left fullscreen.
  - The folder panel froze while a network drive didn't answer; it is listed in the background now.
  - *Favorite* on all the playlists crashed with more than a hundred of them.
  - Closing VLC while customizing the player saved what *Cancel* would have dropped.
  - Choosing another toolbar profile changed the privacy settings.
  - *Reset all* forgot the window's place at exit.
  - The OpenSubtitles password showed in clear text in the subtitle association dialog.
  - Tab typed a tab in the one-line fields instead of moving on.
  - Left or Esc in the queue card lost the focus.
  - The minimize, maximize and close buttons answered at their old place after the title bar changed.
  - Scrolling Settings over the equalizer changed its bands.
  - About couldn't scroll in a small window.
  - Some effect options showed sliders from −9 quintillion to 9 quintillion.
- **Lighter and smoother**: the Customize outlines, about 220 hidden editors on the first Settings search keystroke,
  the dialogs kept after closing, the track menus of the toolbar previews and both layouts of each folder entry are no
  longer built for nothing. The music waveform is analysed once per file (it was once per slider, hidden ones
  included), the mini player's bar seeks by key frames while dragged, and seek bar previews skip network files and
  keep 48 MB at most.
- Windows: VLC Redesign needs Windows 10 version 1809 or later (as Qt 6.8 does). The installer says so, and on an older
  Windows `vlc.exe` shows a message instead of nothing.

### Technical

- `vlc-redesign-update.exe` (`extras/juxtopposed/updater`) puts a staged update in place once VLC has quit, keeps the
  old files until all new ones are in, rolls back on failure, refreshes the plugin cache and the version in *Apps*.
- `extras/juxtopposed/release/make-delta.py` builds the `.vrdelta` files (bsdiff patches, zlib blocks, a JSON index
  with every file's SHA-256), `apply-delta.py` is a reference applier, and `make-release.ps1` checks each delta by
  applying it to the old release and comparing every file. Binaries get a PE time stamp of 0, so a file whose code
  didn't change is the same from one release to the next.
- The download reports its progress (`HttpRequest::progress`), and the hidden `--redesign-resume-time` option reopens
  the media at a time.
- Direct3D 11: the DirectComposition visual lets go of the swapchain when the display closes, `UpdateStaging()`
  releases the staging textures of the format before, and `ReleaseD3D11PictureSys()` clears what it releases.
  `vout_wrapper.c` tries the other outputs after a forced one fails; `src/input/decoder.c` marks its decoders as
  reloadable (`decoder-reload`), which avcodec's hardware-first path needs; `modules/lua/vlc.c` reads the scripts and
  their `require`d modules with `vlc_fopen` on Windows.
- `HttpRequest` has a deadline for the whole request besides the timeout of each step, and `httpRequestAsync()` returns
  a function that cancels it.

---

## [0.2.3] · 2026-10-20

A new installer drawn like the app, which can open your media files; seek bar previews decoded ahead; a wiki.

### Added

- **A new installer, drawn like the app**: the library's sidebar with the steps, Inter, the dialogs' buttons, the
  settings' ON / OFF toggles and an orange progress bar, in English and Hebrew (right to left). Three clicks from
  *Welcome* to *Done*: no more "Select install mode" dialog or *Ready* page.
- **Open media files with VLC Redesign**: the installer's *Options* can make it the player of video, music and
  playlists (the ~130 formats VLC plays), wherever no other app was chosen by you; the *Done* page opens Windows'
  *Default apps* for the rest. VLC Redesign is always offered in *Open with* and in *Default apps*.
- **Right-click menu**: *Play with VLC Redesign* and *Add to VLC Redesign queue* on media files and folders
  (an option of the installer).
- **Install for everyone** from the *Options* page: Setup starts again with administrator rights, for Program Files.
- The installer remembers the options of the last install, and the uninstaller removes all of it and gives every file
  type back to the app it had before (checked against a snapshot of the registry, in English and in Hebrew).
- **A [wiki](https://github.com/KuchiSofts/vlc-redesign/wiki)**: every screen and every option of the Settings window
  (with its default and its VLC option), installation, updates, shortcuts, troubleshooting, with screenshots.

### Changed

- **Seek bar previews come much faster**: the frames are kept, a frame the pointer has left behind is still decoded for
  later instead of being thrown away, and while you hover the bar the frames around the pointer are decoded ahead.
  Positions snap to 1/200 of the length (the preview seeks to the nearest key frame anyway).

### Fixed

- **A time of hundreds of hours**: playback started paused at a time (`:start-time` with `--start-paused`) showed a
  time like 623:17:44, as the player's last timer point was read before VLC had sent one (it was never initialized).
  The time now waits for the first point, and a time past the end of the media is taken from the position.

### Documentation

- README: the player in the nine accent colours and the light theme, *Settings and smart features* with screenshots of
  the Subtitles and Playback settings, and the new installer. The compact library's *Home* is no longer a known
  limitation: it lists the recent media.

---

## [0.2.2] · 2026-10-20

The fullscreen button moves to the control bar, and the control bar is easier to customize.

### Changed

- **Fullscreen button in the control bar**: it left the title bar, where it sat next to *Close*, for the right end
  of the bottom bar (suggested in [#1](https://github.com/KuchiSofts/vlc-redesign/issues/1)). Saved profiles get it
  once, at the same place; take it out with *Customize Player* and it stays out. The title bar keeps minimize,
  maximize and close.

### Fixed

- **Customize Player**: a button let go in the gap between two controls of the bar was ignored. It now goes between
  them, like a drop on either control.
- **Updates**: a check made from About no longer pops up the update card behind the About window too, nor again
  once About is closed. A download started from About still shows its progress in the card.

### Documentation

- README: [*Customize the interface*](README.md#customize-the-interface), with a 44-second video.

---

## [0.2.1] · 2026-10-20

Fullscreen fixes, reported in [#1](https://github.com/KuchiSofts/vlc-redesign/issues/1). Thanks!

### Fixed

- **Leaving fullscreen**: fullscreen entered without a video (the title bar's button in the library or for music,
  or F11) could only be left with F11, since the title bar hides in fullscreen and the fullscreen keys only drive
  the video's fullscreen. The *Fullscreen* and *Leave fullscreen* shortcuts (F, Escape, or whatever you set them to)
  now leave it too, from any view.
- **No frame in fullscreen**: the 10 px frame and rounded corners around the video are kept for the window; in
  fullscreen the video fills the screen and the bars float over it.

### Notes

- The fullscreen button sits next to the window buttons as in Juxtopposed's design. To keep it away from *Close*,
  drag it into the control bar: ⋯ → *Customize Player*. F and a double-click on the video also toggle fullscreen.
- CI: the Windows build workflow now completes (the package step could not write its output).

---

## [0.2.0] · 2026-10-20

**Solid on every Windows PC, and it keeps itself up to date.** A full audit of every button, menu and setting,
an installer in the redesign's look, updates from GitHub, and the latest VLC 4 fixes.

### Highlights

- 🚀 **Starts every time, fast**: a second copy no longer hangs on start (it used to wait forever with no window),
  and a fresh install or unzipped copy no longer rescans its ~600 plugins on each start (≈0.4 s to the window).
- 📦 **Installer**: per-user, no administrator rights, styled like the redesign, closes and reopens a running copy.
- 🔄 **Updates from GitHub**: a card when a release is out; an installed copy updates itself after checking the
  installer's SHA-256, and reopens on what was playing.
- ℹ️ **New About window** in the redesign's style, with the design credit and links to Juxtopposed.
- 🧹 **~80 fixes** from an audit of the whole interface, including one crash.
- 🆙 **VLC 4 merged** up to 2026-10-19 (110 commits), and every pull request is now built automatically.

### Added

- **Installer** (`vlc-redesign-<version>-win64-setup.exe`, Inno Setup): installs per user by default (or for all users),
  keeps settings, library and history across updates, regenerates the plugin cache, optional desktop shortcut and
  *Open with* entries for video and audio files that don't change the default player; English and Hebrew.
  A running copy is closed politely (forced after 15 s if needed) and reopened afterwards.
- **Updates**: once a day (Settings → General → *Check for updates automatically*, option `redesign-update-check`) or
  from About, VLC Redesign reads the latest GitHub release. A card offers *What's new*, *Skip*, and *Update now*
  (installed copy) or *Download* (portable copy). Classic menus' *Check for Updates* opens About.
- **About** redrawn in the redesign's style: version and update check, the CC BY 4.0 design credit with links
  (channel, the redesign video, Figma file, website), first implementation, VideoLAN, and the classic lists behind it.
- **Profiles**: New, Rename, Clone and Delete in *Customize Player*; the design's **Movies, Shows, Music and Work**
  profiles for new installs.
- **Settings**: search finds the labels you see (and matching shortcuts); an *Applies after restart* hint on the rows
  that need it; errors are shown and a failed save no longer loses your changes; developer mode reveals the expert
  options and *All VLC options*; process priority is a list; night mode restores the compressor it replaced.
- **Library**: a *Recent* page with recently played videos and songs; *Playlists* shows every playlist; video grouping
  (by name, by folder) is back in the sort menu; the back/forward arrows both work.
- **Player**: the visualizer can be opened from the music view; the right-click *Repeat* is an Off / All / One choice;
  the maximize button shows a restore icon when maximized; Shift+F1 opens About.
- **Shortcuts editor**: a key is added to an action (each key has its own remove), and Escape cancels a capture.
- **Release tooling**: `make-release.ps1` builds the zip, the installer and `SHA256SUMS.txt`; a GitHub workflow builds
  every pull request and `main` for Windows; issue forms for bugs and suggestions; a [roadmap](ROADMAP.md).

### Fixed

**Startup and stability**
- A second VLC Redesign hung on start while another one ran: pipe names collided between processes, so its preparser
  and then its interface failed (`src/win32/filesystem.c`); the preparser now also falls back to in-process parsing.
- An unzipped copy rescanned all plugins on every start (zip stores times in 2-second steps): release files get one
  even-second time before the cache is made, and the installer rebuilds the cache.
- A crash (use-after-free) in the video effects after toggling a filter.
- With the media library off, the whole window stayed blank.
- Playlist export no longer creates a player on the interface thread (a freeze risk): the file is written directly.
- Download threads are cancelled and waited for when VLC quits.

**Player and menus**
- The sleep timer was cancelled at the start of each new track.
- Q (queue) and L (library) never reached the player.
- The Quit button did nothing; the Library button did nothing in the mini player.
- The top bar used the video layout in the music player (⋯ and Library missing or doubled).
- The subtitle *None* check stayed on after picking a track.
- Repeat All came back as Repeat One after a restart (and is repaired for those affected).
- HDR choices left VLC on the libplacebo output for good.
- PiP hid the video where picture-in-picture isn't available; the mini queue's − removed the playing track.
- *Open Recent* ignored *Save recently played items*; *Open Location from Clipboard* now matches Ctrl+V.
- *Customize Player* opened from the library left the player stuck in edit mode.
- Tooltips: "Back 15 seconds" / "Forward 15 seconds", Fullscreen naming and icon; the sleep timer and player menu in the
  light theme.

**Library and subtitles**
- Stream and Convert were swapped in the context menus.
- OpenSubtitles needed a second click (and a second download) to save; your account is kept for the session.
- File names with `( ) ' ! & + , ; = @ $` missed their resume point, artwork and subtitles.
- Creating a playlist without the media library could crash; onboarding without it came back at every start.
- Onboarding overwrote the metadata network access setting; the suggested folder can be cleared.
- Folder menu labels, *Frequently Visited Folders ›*, the new playlist thumbnail, network links without a scheme,
  and search text carried between sections of the compact library.

**Settings and effects**
- Compressor, Spatializer and Stereo Widener sliders stopped after one step.
- Saving video effects made the current crop the default for every video.
- Effects tabs cancelled each other's preview; the dialog didn't wake up when playback started.
- Rows that did nothing were removed or wired (*Ignore ASS/SSA styling*, error dialogs, playlist tree); the cone and
  cast quality rows show the right labels; the web remote warns without a password; saving no longer shows a UI hidden
  with Ctrl+H.
- Profiles: switching to a default profile no longer reverts your theme and settings, and a profile that fails to
  apply at startup no longer replaces your saved ones.
- Readable error messages instead of raw option names, and typos ("kHz", "changes", the Last.fm password label).

### Changed

- New interface strings are listed for translation (`po/POTFILES.in`), and C++ strings use VLC's `qtr()`.
- The default profile of new installs is named *Default*.
- The fork's original M0 CI workflow is replaced by the Windows build workflow.

### Upstream VLC

- Merged 110 commits of VLC master (up to `942cdaea17`, 2026-10-19). The window buttons register themselves for Windows
  11 snap layouts; VLC's new split button is built but the redesign keeps its own buttons.

---

## [0.1.0] · 2026-10-20

**The first Windows release.** The redesign now builds and runs on real Windows, matches the Figma
frames down to the pixel on the measured screens, and no longer freezes.

### Highlights

- 🪟 **Runs on Windows**: a reproducible Docker build and a portable zip.
- 🎯 **Pixel-matched to the design**: text renders like Figma (Inter, same spacing), the light theme follows the *Player - Light* frame.
- 🧊 **No more freezes**: three deadlocks between the interface and the core are fixed.
- 📚 **New library spots**: *Listen Again*, *Frequently Visited Folders*, a folder browser, and a compact library over the video.
- 🎛️ **Settings rebuilt** with the design's controls and many more VLC options.
- 🖼️ **Frame previews** when hovering the seek bar.

### Added

**Player**
- A compact library sidebar over the video: sections, folder breadcrumb, search, sorting, progress and play counts.
- A queue card with *Queue*, *Bookmarks* and *Chapters* tabs.
- Frame previews when hovering the seek bar, for local seekable videos (option `qt-seek-thumbnail`).
- 12 new control bar buttons: ±10 s, picture-in-picture, right sidebar, home, thumbnail, folder name, playlist name, replay, chapters, bookmarks, playlists, main menu, library.
- Control bar editing by drag and drop on the player itself. The timeline can sit on its own row.
- **Q** toggles the queue and **L** the library (only when no VLC hotkey uses the key).
- An *External Tracks* group in the subtitle panel; a refresh for the audio device list; a *None* video track.

**Library**
- *Listen Again* on Home (recently played audio) and *Frequently Visited Folders* (ranked from recent history).
- A folder page with breadcrumbs.
- Playlists: favourite, shuffle play, export to a file.
- Add and remove bookmarks from the interface.
- The subtitle dialog can also attach external audio tracks, and target a whole folder or playlist.

**Settings**
- A settings window without an Apply button: changes are saved when it closes. Search, sidebar, and an *Advanced Options* block on every page.
- The design's controls: toggle, dropdown, segmented control, slider, text field, startup volume, inline equalizer.
- Many more VLC options, regrouped like the design, including repeat / loop, play and exit, fast seek, skins, mouse wheel direction, web remote, night mode, caching presets and post-processing.
- Language list (ISO 639-2), optical drives and x264 presets in dropdowns.
- Actions: rescan plugins, open the configuration folder, reset everything.
- Window geometry memory, notification duration, cast device discovery, process priority (Windows), double-click action, a filter for the recent items list, subtitle size mode and background opacity.
- Interface language for the portable copy, kept in `<vlc folder>\portable\language` instead of the registry an installed VLC uses.

**Dialogs and menus**
- A redesigned **Network Stream** dialog: several URLs at once, optionally saved as a playlist.
- A redesigned right-click menu: playback, *Hide UI* (Ctrl+\\), playback / video / audio / subtitle submenus, playlist, open media, quit. Shortcut labels come from your actual hotkeys.
- A **⋯ main menu**: open actions, *Open Recent*, profiles, keyboard shortcuts, customise player, settings, about.
- Classic VLC menus are redrawn to the design, with two-tone icons.

**Credits and packaging**
- The design credit (CC BY 4.0) in **About**, in its Credits page, and in [CREDITS.md](CREDITS.md).
- The Windows package ships `CREDITS.txt`, `CHANGELOG.txt`, `COPYING.LIB.txt` and the font licenses (`licenses/`).

### Changed

- The video is framed between the bars, with a 10 px inset and rounded corners; a redesigned top bar.
- The skip buttons jump ±15 s.
- Times are always shown with hours; the resume prompt is a compact card; record shows the accent colour instead of blinking.
- The volume slider goes up to 200 %, with fill states to match.
- The bars auto-hide in fullscreen.
- Onboarding: keycaps show your real shortcuts, the theme previews live, and it can be cancelled with *Cancel* or Escape.
- The toolbar editor became a *Customize Player* panel that edits the bar in place.
- The subtitle association and new playlist dialogs use the shared redesign dialog.
- Video cards use file names as titles, dim watched items and show progress as an accent line.
- The library header has an address bar, named sort options and a *View Options* menu.
- Without the main interface (skins), the classic Preferences and Effects windows open.

**Visual fidelity**
- Inter is the application font, rendered with distance fields, no hinting and greyscale antialiasing, the combination that matches Figma's text.
- The bundled Inter has every advance width one unit wider: Qt truncates advances to 1/64 px, and the extra unit gives back the design's spacing.
- DM Mono for keyboard shortcuts.
- Light theme: icons are recoloured to the *Player - Light* frame's greys.
- Most icons re-exported; new player corners and theme preview images; restyled checkboxes, scroll bars, headers and sidebar rows.
- Rounded video and artwork textures crop correctly at any aspect ratio.

### Changed defaults

| Option | Before | Now | Why |
|---|---|---|---|
| `qt-pin-controls` | off | **on** | The design keeps the bars beside the video |
| `qt-video-autoresize` | on | **off** | The window keeps its size when a video starts |
| `qt-max-volume` | 125 | **200** | The design's volume control goes to 200 % |
| Loop hotkey (`key-loop`) | `L` | **Shift+L** | `L` opens the library |
| Next / previous | | also **`<` `>`** | |
| Next / previous frame | | also **`.` `,`** | |
| OSD message duration | fixed | **`osd-duration`** (1000 ms) | Set from the notification duration setting |
| Update checks (`qt-updates-notif`) | on | **off** | VideoLAN's update feed offers the official VLC, not this build |

### Fixed

**Freezes and crashes**
- The interface froze when a hotkey waited on the video output: hotkeys now go to the core from their own thread.
- A deadlock between the player, the input resource and the interface thread: `input_resource_SetInput` now runs in the input thread's `Init`.
- A deadlock when the video window was enabled or disabled: `vlc_window_Enable` / `Disable` are called outside `window_lock`, and `Enable` gets a copy of the configuration.
- A crash on exit with MinGW (Qt's thread-local timer outlived the libstdc++ destructor key).
- An assertion on iPhone HEIC photos (`ES_OUT_DRAIN`); HEIF images show the primary grid instead of each tile.
- Wrong colours and Direct3D 11 device removal when switching video filters; a plane overrun in the interactive filter picture pool.
- Video filters (extract, gradfun, gradient, grain, invert, sepia, sharpen) refuse to open when input and output formats differ, instead of producing garbage.

**Interface**
- The window shrank by 16×8 px on every restart (Windows' resize borders weren't counted). It now opens at 1280×720, centred, the first time, and restores maximised and fullscreen windows to their normal size.
- The waveform seek bar and cover corners were rebuilt without Canvas, which VLC's Qt Quick lacks; the waveform helper gets native paths on Windows.
- Rate and delay changes are announced after the player lock is released.
- Developer mode was on whenever the setting read back as the text `"false"`.
- Importing a profile with a setting that no longer exists failed: the setting is now skipped.
- The effects dialog no longer cancels itself while closing; popups are centred on whole pixels; the title bar stays draggable while a dialog is open.
- The audio effects hint showed an internal name (`audioBridge`).
- The registry language read is type-checked and the key is closed.

### Build

- **Windows Docker build**: `extras/juxtopposed/win64-docker.ps1` (host) runs `win64-docker.sh` in VideoLAN's official `vlc-debian-win64-posix` image with prebuilt contribs. Objects persist in a Docker volume; `-Quick` skips configure; `-Lint` runs qmllint on changed QML; `-Volume` and `-Strip` make a separate stripped release build.
- The SRT, x265 and GME plugins link the static unwinder, so they don't import `libgcc_s_seh-1.dll`, which the plugin loader can't find.
- `vlc-waveform` links its runtime statically on Windows.
- OpenSubtitles uses a WinHTTP client on Windows, so the QtNetwork dependency is gone.
- License headers on every new source file; Meson and Autotools list the same QML files and resources; line endings normalised to LF.

---

## M3 · Dialogs, settings and playback tools · 2026-10-13

*By MrDouglax, in [JuxtopposedVLC](https://github.com/SrDouglax/JuxtopposedVLC).*

### Added
- Redesigned settings on a staged QML model: startup, file logging and diagnostics, Last.fm, developer mode, add-ons, audio device preview with rollback, DPI-scaled navigation, clearing history under Privacy.
- A three-step onboarding: welcome, theme, shortcuts.
- A keyboard shortcut editor with staged changes and a confirmed reset.
- Transactional audio and video effects, equalizer preset curves, effects inline in settings.
- Sleep timer (stop or pause), control bar profiles and a personalisation menu.
- Control bar customisation on the live player; profiles keep a safe subset of settings.
- New controls: favourite, file and artist name, frame rate, resolution, zoom, HDR, divider.
- Mini player with a live queue, picture-in-picture, a dedicated music view, a blurred backdrop.
- A **waveform seek bar** computed by the isolated `vlc-waveform` helper, and a *waveform* audio visualisation.
- HDR output selection, depending on the video backend.
- Advanced subtitle sources (OpenSubtitles, links), series timing presets and a sync timeline in the tracks menu.

### Fixed
- QML delegates, series lifetime, light theme contrast, popup checkmarks, video resizing during effect rollback.

## M2 · Library, playlists and subtitles · 2026-10-13

*By MrDouglax.*

- A compact tracks and playback panel with chapters and bookmarks.
- Create and edit playlists from the library; native *Open media* actions.
- Subtitle choices remembered per media, restored for direct playback too.
- Header layout for narrow windows; video cards, header, sidebar and floating player aligned to the design.

## M1 · First native shell and player · 2026-10-12

*By MrDouglax.*

- The redesigned window shell and player controls (window buttons, adjacent playback controls, design defaults).
- The redesign icon set and the four volume states exported from Figma.
- Fixed opening videos through nested navigation.

## M0 · Baseline and build · 2026-10-12

*By MrDouglax.*

- Linux Docker build, smoke tests and launcher (`extras/juxtopposed/`).
- A Windows CI workflow with MinGW runtime packaging.
- Plans, Figma coverage maps and validation evidence under `.specs/`.

---

## Differences from upstream VLC

Most of the work is in the Qt interface (`modules/gui/qt`). These changes touch the rest of VLC; they're
the ones to watch when merging upstream, and the candidates to propose to VideoLAN.

| File | Change |
|---|---|
| `src/input/input.c` | `input_resource_SetInput` moved to the input thread's `Init` (deadlock) |
| `src/video_output/video_output.c` | Window enable/disable outside `window_lock` (deadlock); pool fallback for other chromas |
| `src/video_output/window.c` | `vlc_window_Enable` passes a copy of the configuration |
| `src/video_output/video_text.c` | OSD duration from `osd-duration` |
| `src/libvlc-module.c` | `osd-duration` and `media-library-enabled` options; hotkey changes (see *Changed defaults*) |
| `src/libvlc.c` | The media library is created only when `media-library-enabled` is on |
| `src/player/medialib.c` | External subtitle and soundtrack files from the media library attached before playback |
| `src/audio_output/filters.c`, `output.c` | The *waveform* visualisation choice |
| `modules/visualization/visual/` | Oscilloscope code moved to its own file; waveform effect |
| `modules/demux/mp4/heif.c` | Primary grid shown; PCR reset after drain (assert) |
| `modules/video_output/win32/direct3d11.cpp` | Unsupported CPU chromas and mismatched pictures refused |
| `modules/video_filter/{extract,gradfun,gradient,grain,invert,sepia,sharpen}.c` | Refuse differing input/output chromas |
| `modules/text_renderer/freetype/freetype.c` | `freetype-fontsize` brought back; default text direction |
| `bin/winvlc.c` | Portable language file read before the registry |
| `src/win32/filesystem.c` | `vlc_pipe()` names pipes per process (`vlc-<pid>-<n>-<time>`), first instance only, no remote clients |
| `src/preparser/preparser.c` | Falls back to the in-process preparser when the external one can't start |
| `bin/Makefile.am`, `bin/meson.build` | Build the `vlc-waveform` helper |
| `extras/package/win32/package.mak` | Credits, changelog and font licenses in the package |

[0.2.1]: https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.2.1
[0.2.0]: https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.2.0
[0.1.0]: https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.1.0
