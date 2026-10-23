# Roadmap

Where **VLC Redesign** is going. The goal stays the same: the real VLC, with
[Juxtopposed's design](https://www.figma.com/community/file/1691465999134370980) on every screen,
working smoothly on every Windows PC.

Have an idea? Open a **[suggestion](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=feature_request.yml)**.
Found a problem? Open a **[bug report](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=bug_report.yml)**.
Issues with a 👍 from many people move up this list.

Status: ✅ done · 🔨 in progress · 📋 planned · 💡 idea

The full plan for every program (desktop, Android, TV) and every platform is in the
[master plan](docs/MASTER-PLAN.md).

## 0.2: solid on every Windows PC

- ✅ Several copies at once: a second VLC Redesign no longer hangs on start
- ✅ Fast start: the plugin cache stays valid in the zip and after installing
- ✅ Installer with the redesign's look: closes and reopens VLC around an update, keeps your settings, *Open with* without taking over your default player
- ✅ Updates from GitHub: a card when a new release is out; installed copies update themselves (checksum-verified) and reopen where you were
- ✅ About window in the redesign's style, with the design credit and links
- ✅ ~75 fixes from a full audit of every button, menu and setting (see the [changelog](CHANGELOG.md))
- ✅ Latest VLC 4 fixes merged; every pull request is built automatically
- ✅ Fullscreen: leave it from any view, no frame around the video, its button in the control bar instead of next to *Close* ([#1](https://github.com/KuchiSofts/vlc-redesign/issues/1))
- ✅ Customize Player: drops between two controls work; a [video](README.md#customize-the-interface) shows how it works
- ✅ An installer drawn like the app, which can open your media files and add *Play with VLC Redesign* to right-click menus
- ✅ Seek bar previews decoded ahead and kept: a sweep along the bar shows them at once
- ✅ A [wiki](https://github.com/KuchiSofts/vlc-redesign/wiki) with every screen and option
- 🔨 Pixel check of all 103 Figma frames against the app, frame by frame

## 0.3: the rest of the design

- 📋 Redesign the dialogs that still use the classic look: Media Information, Bookmarks, Convert/Stream, Open Disc, Capture Device, Messages
- 📋 Right-click and context menus drawn as the design's cards
- 📋 The compact library's *Home* with the rows of the library's Home (it lists the recent media for now)
- 📋 Profiles as the design shows them (Movies, Shows, Music, Work) checked against frame 6648:20666
- 📋 Visualizer and setting presets checked against frames 2372:8639 and 2372:8834
- 📋 Light theme: every state checked, accessible contrast

## 0.4: everywhere

- 📋 Translations: every new string extracted for VLC's translators; Hebrew and other right-to-left languages checked
- 📋 Code-signed releases (no SmartScreen warning)
- 📋 Windows on ARM64
- 📋 Linux build again (it was validated before the Windows work), then a Flatpak
- 💡 macOS

## Ideas

- 💡 Portable copies that update themselves too (today they open the release page)
- 💡 A *What's new* card after an update
- 💡 Sync the design's accent colour with the Windows accent
- 💡 Upstream our VLC core fixes (pipes, freezes, filters) to VideoLAN

## Staying close to VLC

Every week a GitHub Action merges the new VLC commits into a pull request, and every pull request is built
for Windows. Our changes outside the interface are listed in the
[changelog](CHANGELOG.md#differences-from-upstream-vlc) so they can be checked at each merge, and dropped
when VLC fixes the same thing.
