# Roadmap

Where **VLC Redesign** is going. The goal stays the same: the real VLC, with
[Juxtopposed's design](https://www.figma.com/community/file/1691465999134370980) on every screen, pixel for pixel,
working smoothly on every PC.

Have an idea? Open a **[suggestion](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=feature_request.yml)**.
Found a problem? Open a **[bug report](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=bug_report.yml)**.
Issues with a 👍 from many people move up this list.

Status: ✅ done · 🔨 in progress · 📋 to do · 💡 idea

The full plan, with every Figma frame, its status and the design tokens, is the
[master plan](docs/MASTER-PLAN.md).

## Next release: 0.4.0

Everything below belongs to the next release until it is published. What is ready and passes the quality checks
on release day ships.

**0.4.0 ships on three platforms together** (2026-10-30): Windows x64, Linux (x86_64 and ARM64) and macOS (Apple
Silicon and Intel). Windows on ARM64 moves to the next version: in the 0.4.0 tests on a real ARM64 Windows runner it
opened its window and stopped a few seconds later (a crash in the Qt interface after the media library started), and
one of the installer's 400 binaries was not ARM64.

**Done since 0.3.1**

- ✅ The playback speed button can be added to the bar again ([#9](https://github.com/KuchiSofts/vlc-redesign/issues/9));
  VLC's latest work merged ([#8](https://github.com/KuchiSofts/vlc-redesign/issues/8))
- ✅ The light theme settled against Figma: every place that asked for its own light colour checked, the
  classic windows' title bar now as light as the other dialogs
- ✅ A full code review: about 90 fixes (installer and updates, casting, subtitles in older encodings, colours of SD
  video, profiles and settings, the Windows proxy, library and dialogs)
- ✅ Every text of VLC Redesign translated in all 72 languages on Weblate, and about 80 wrong translations fixed

**In progress**

- 🔨 **Live TV**: the channels of your own IPTV playlist (M3U) or Xtream Codes account in a TV page, with what is
  on now and next, a programme guide, catch-up of past programmes, channel up and down, numbers and a channel
  list over the video, and a catalog of free, legal channel lists; an add-on, on by default, that can be left out
- 🔨 *Suggest an idea* and *Report a problem* in the ⋯ menu and in About: the GitHub form opens filled with what
  helps (version, Windows, graphics card, screens), never anything personal
- 🔨 A *What's new* card after an update: the highlights of the new version, once, the first time VLC opens after
  an update (it can be turned off in Settings, General); built, to be checked in the running app
- 🔨 The last checks in the running app: Settings scroll bars, the warning colour, *Show controls in fullscreen*,
  a playlist's own picture, the small classic windows
- 🔨 Subtitle lines marked on the seek bar (from the subtitle file shown), with the line at the hovered time in
  the preview: built, being checked in the app
- 🔨 *Follow the Windows accent color* in Settings, Appearance: built, being checked in the app
- 🔨 Every message, question and classic window in the redesign's look (its own title bar), and Go to Time,
  Plugins, Media Statistics and Folder information redrawn: built, being checked in the app

**To do**

- 📋 Code-signed releases (no SmartScreen warning): waiting for SignPath's approval
- 📋 A public roadmap board ([Discussions](https://github.com/KuchiSofts/vlc-redesign/discussions) are open)
- 📋 Testing on Intel graphics, Windows 10 on real PCs and screens of different scales

**Done before** (0.3.0 and 0.3.1): surround sound on every speaker, casting to TVs, a light theme on every screen,
the classic dialogs in the redesign's look, all 211 settings rows of the design, the library's right-click menus,
playlists with their own picture, the pixel check of all 103 Figma frames, 72 languages chosen in the installer and
changed without a restart. The full list is in the [changelog](CHANGELOG.md).

**Ideas**

- 💡 More ideas welcome in [Discussions](https://github.com/KuchiSofts/vlc-redesign/discussions/categories/ideas)

## Other platforms

Where each platform stands is kept in one file, [`platforms.yml`](platforms.yml): the bar at the top of the README,
the website and the *Also on* line of the release notes and of About are made from it.

- 🔨 A website for every platform (GitHub Pages): it offers the download for your system and lists the others
- ✅ Linux (x86_64 and ARM64), released with 0.4.0: a .deb, an .rpm, an AppImage and a tarball, next to the
  distribution's own VLC, tested in containers on 7 distributions and on GitHub's x86_64 and ARM64 runners
- 🔨 Linux: the Flatpak (the manifest is written), an updater in the app, tests on real desktops
- ✅ macOS (Apple Silicon and Intel), released with 0.4.0: a disk image for each, built and tested by GitHub's Macs,
  signed ad hoc (macOS asks once to open it)
- 🔨 macOS: signing by Apple (it needs a developer account), an updater in the app, tests on a Mac by hand
- 🔨 Windows on ARM64: built, not released with 0.4.0 (the crash in the Qt interface and the one binary that was
  not ARM64, above); it comes in a later version. Until then the x64 build runs on Windows 11 ARM64 under emulation
- 🔨 The design for every platform: its colours, sizes, fonts and icons in [`design/`](design/README.md)
  (`design-1.0`), with CSS, Android and Swift files made from them and a check on every pull request that the
  Windows app still uses the same values
- 💡 Android (phones, tablets, Android TV), Samsung TV receiver, iPhone and iPad

## Staying close to VLC

Every week a GitHub Action merges the new VLC commits into a pull request, and every pull request is built
for Windows. Our changes outside the interface are listed in the
[changelog](CHANGELOG.md#differences-from-upstream-vlc) so they can be checked at each merge, and dropped
when VLC fixes the same thing.
