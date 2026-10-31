# Credits

## Design: Juxtopposed

The interface of this build is the work of **Juxtopposed**. This project turns the design into software;
the design itself (layouts, colours, typography, icons, interactions) is theirs.

- ▶ **Video:** [I Redesigned VLC After 25 Years](https://www.youtube.com/watch?v=ERFeOgqk324)
- **Figma file:** [VLC Redesign (Community)](https://www.figma.com/community/file/1691465999134370980)
- **YouTube:** [@juxtopposed](https://www.youtube.com/@juxtopposed)
- **Website:** [juxtopposed.com](https://juxtopposed.com)
- **Figma:** [@juxtopposed](https://www.figma.com/@juxtopposed)
- **X / Twitter:** [@juxtopposed](https://twitter.com/juxtopposed)
- **Bluesky:** [@juxtopposed.bsky.social](https://bsky.app/profile/juxtopposed.bsky.social)
- **Instagram:** [@juxtopposed](https://instagram.com/juxtopposed)
- **GitHub:** [juxtopposed](https://github.com/juxtopposed)

### Attribution (CC BY 4.0)

> The user interface design is based on **"VLC Redesign (Community)" by Juxtopposed**
> (<https://www.figma.com/community/file/1691465999134370980>), licensed under
> **[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)**.
> Changes: the design was implemented in Qt/QML inside VLC 4; icons were redrawn as SVG/PNG and
> modified (two-tone menu icons, light-theme variants); some layouts were adapted to VLC features
> and to window sizes the design doesn't cover. Juxtopposed does not endorse this project.

The same credit is shown in the app, under **⋯ → About VLC**, and in the Credits page of that window.

**App icon:** the icon of every VLC Redesign program and app (Windows, macOS, Linux, Android, the installer, the
site) is Juxtopposed's logo from the same Figma file (the orange cone of the Icon set), used as drawn and only
rendered to each platform's sizes; on Android the launcher puts it on the design's background colour (#0E0E0E).

## Implementation

- **MrDouglax**: the first implementation, [JuxtopposedVLC](https://github.com/SrDouglax/JuxtopposedVLC)
  (milestones M0 to M3: the native shell, player, library, playlists, subtitles, settings, onboarding and
  effects, validated on Linux). This repository keeps that history.
- **This fork**: the Windows build and everything listed under *Unreleased* in the [changelog](CHANGELOG.md).

## Community

Thank you to everyone who tested, reported and translated:

- **Bug reports:**
  - [@Darkdevworker](https://github.com/Darkdevworker): fullscreen ([#1](https://github.com/KuchiSofts/vlc-redesign/issues/1)).
  - [@zaidmonis](https://github.com/zaidmonis): the setup that hung
    ([#2](https://github.com/KuchiSofts/vlc-redesign/issues/2)).
  - [@OldAncient](https://github.com/OldAncient): the audio device switch
    ([#3](https://github.com/KuchiSofts/vlc-redesign/issues/3)) and the library and player bugs
    ([#4](https://github.com/KuchiSofts/vlc-redesign/issues/4)).
  - [@Kinglucas962](https://github.com/Kinglucas962): the missing translations
    ([#5](https://github.com/KuchiSofts/vlc-redesign/issues/5)).
  - [@Brackets124](https://github.com/Brackets124): the playback speed button
    ([#9](https://github.com/KuchiSofts/vlc-redesign/issues/9)).
  - [@MAVIVEHICR](https://github.com/MAVIVEHICR): the startup volume stopped at 125 %
    ([#30](https://github.com/KuchiSofts/vlc-redesign/issues/30)).
- **Ideas:**
  - [@muhammadali-og](https://github.com/muhammadali-og): buttons that show what is open, and Ctrl + wheel for
    the subtitle size ([#10](https://github.com/KuchiSofts/vlc-redesign/issues/10)).
- **Translations:**
  - [@Kinglucas962](https://github.com/Kinglucas962): Brazilian Portuguese
    ([#6](https://github.com/KuchiSofts/vlc-redesign/pull/6)).
  - On [Weblate](https://hosted.weblate.org/projects/vlc-redesign/):
    - Priit Jõerüüt: Estonian.
    - bittin: Swedish.

## VLC

VLC media player is made by the volunteers of the [VideoLAN](https://www.videolan.org/) community.
See [AUTHORS](AUTHORS) for the developers and [THANKS](THANKS) for the libraries VLC uses.
*VLC*, *VLC media player* and the cone logo are trademarks of VideoLAN; this is an unofficial build.

## Fonts

| Font | Authors | License | File |
|---|---|---|---|
| [Inter](https://github.com/rsms/inter) 3.19, modified (every advance width one unit wider, so Qt matches the design's spacing) | The Inter Project Authors | SIL OFL 1.1 | [LICENSE.txt](modules/gui/qt/pixmaps/fonts/LICENSE.txt) |
| [DM Mono](https://github.com/googlefonts/dm-mono) | The DM Mono Project Authors | SIL OFL 1.1 | [DMMono-OFL.txt](modules/gui/qt/pixmaps/fonts/DMMono-OFL.txt) |

The Windows package ships both licenses in its `licenses` folder.

## Licenses at a glance

| What | License |
|---|---|
| VLC and this fork's code | GNU GPL v2 or later ([COPYING](COPYING)) |
| libVLC | GNU LGPL v2.1 or later ([COPYING.LIB](COPYING.LIB)) |
| Interface design and icons derived from it | CC BY 4.0, by Juxtopposed |
| Inter, DM Mono | SIL Open Font License 1.1 |
| Third-party libraries in the Windows build | Their own licenses, see [THANKS](THANKS) and `contrib/` |
