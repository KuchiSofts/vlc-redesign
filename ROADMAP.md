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

## Next release: 0.3.2

Everything below belongs to the next release until it is published. What is ready and passes the quality checks
on release day ships.

**Done since 0.3.1**

- ✅ The playback speed button can be added to the bar again ([#9](https://github.com/KuchiSofts/vlc-redesign/issues/9));
  VLC's latest work merged ([#8](https://github.com/KuchiSofts/vlc-redesign/issues/8))
- ✅ A full code review: about 90 fixes (installer and updates, casting, subtitles in older encodings, colours of SD
  video, profiles and settings, the Windows proxy, library and dialogs)
- ✅ Every text of VLC Redesign translated in all 72 languages on Weblate, and about 80 wrong translations fixed

**In progress**

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
- 🔨 Linux (AppImage, Flatpak, .deb, .rpm)
- 📋 macOS (Apple Silicon and Intel)
- 📋 Windows on ARM64
- 💡 Android (phones, tablets, Android TV), Samsung TV receiver, iPhone and iPad

## Staying close to VLC

Every week a GitHub Action merges the new VLC commits into a pull request, and every pull request is built
for Windows. Our changes outside the interface are listed in the
[changelog](CHANGELOG.md#differences-from-upstream-vlc) so they can be checked at each merge, and dropped
when VLC fixes the same thing.
