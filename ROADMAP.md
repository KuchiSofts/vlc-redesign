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

## Next release: 0.3.0

Everything below belongs to the next release until it is published: nothing is set aside for a later version.
What is ready and passes the quality checks on release day ships.

**Done since 0.2.6**

- ✅ 5.1 and 7.1 sound in its place on every speaker; every surround layout mixed right to stereo
- ✅ Casting: starts once, every film to its last picture, playlists and subtitles while casting, 4K four times faster,
  checked on a real Samsung TV (pause, films sent as they are)
- ✅ Converting a video no longer crashes; video effects that crashed are fixed; damaged files no longer stop a playlist
- ✅ A file opened from Explorer plays at once; no firewall question at start; quicker first start; quitting at once
- ✅ Memory flat over long sessions
- ✅ The subtitles window and every popup and menu toggle on their button; a click outside only closes them
- ✅ Figma's pixel cat in the player when nothing plays, drawn sharp at any size

**In progress**

- 🔨 Light theme on every screen (tracks window, profiles, welcome, shortcut windows, effect sliders)
- 🔨 The dialogs that still have the classic look: Open Disc, Capture Device, Convert/Stream, Media Information,
  Bookmarks, Messages, advanced preferences
- 🔨 The settings rows of the design still missing; *Always show the player bar*
- 🔨 New playlist with its own thumbnail, and empty playlists; the resume card of the design
- 🔨 Icons and check marks in the chosen accent colour

**To do**

- 📋 Library right-click menus drawn as the design's cards
- 📋 Real blur behind floating panels
- 📋 Pixel check of all 103 Figma frames, published in the wiki
- 📋 Translations, the language chosen in the installer and changed in Settings without a restart
- 📋 Code-signed releases (no SmartScreen warning)
- 📋 *Suggest an idea* and *Report a problem* in the ⋯ menu; Discussions and a public roadmap board

## Other platforms

- 📋 Linux (AppImage, Flatpak, .deb, .rpm)
- 📋 macOS (Apple Silicon and Intel)
- 📋 Windows on ARM64
- 💡 Android (phones, tablets, Android TV), Samsung TV receiver, iPhone and iPad

## Staying close to VLC

Every week a GitHub Action merges the new VLC commits into a pull request, and every pull request is built
for Windows. Our changes outside the interface are listed in the
[changelog](CHANGELOG.md#differences-from-upstream-vlc) so they can be checked at each merge, and dropped
when VLC fixes the same thing.
