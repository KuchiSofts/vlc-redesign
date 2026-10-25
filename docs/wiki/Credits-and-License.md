# Credits and license

## Design: Juxtopposed

The interface of VLC Redesign is the work of **Juxtopposed**. This project turns the design into software; the
design itself (layouts, colours, typography, icons, interactions) is theirs.

- ▶ **Video:** [I Redesigned VLC After 25 Years](https://www.youtube.com/watch?v=ERFeOgqk324)
- **Figma file:** [VLC Redesign (Community)](https://www.figma.com/community/file/1691465999134370980)
- **YouTube:** [@juxtopposed](https://www.youtube.com/@juxtopposed) · **Website:** [juxtopposed.com](https://juxtopposed.com)
  · **Figma:** [@juxtopposed](https://www.figma.com/@juxtopposed) · **GitHub:** [juxtopposed](https://github.com/juxtopposed)

> The user interface design is based on **"VLC Redesign (Community)" by Juxtopposed**
> (<https://www.figma.com/community/file/1691465999134370980>), licensed under
> **[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)**.
> Changes: the design was implemented in Qt/QML inside VLC 4; icons were redrawn as SVG/PNG and
> modified (two-tone menu icons, light-theme variants); some layouts were adapted to VLC features
> and to window sizes the design doesn't cover. Juxtopposed does not endorse this project.

The same credit is in the app, under **⋯ → About VLC**, and in the Credits page of that window.

## Implementation

- **MrDouglax**: the first implementation, [JuxtopposedVLC](https://github.com/SrDouglax/JuxtopposedVLC) (the native
  shell, player, library, playlists, subtitles, settings, onboarding and effects, validated on Linux). This
  repository keeps that history.
- **KuchiSofts**: this fork, the Windows build and what the
  [changelog](https://github.com/KuchiSofts/vlc-redesign/blob/main/CHANGELOG.md) lists since.

## VLC

VLC media player is made by the volunteers of the [VideoLAN](https://www.videolan.org/) community. See
[AUTHORS](https://github.com/KuchiSofts/vlc-redesign/blob/main/AUTHORS) for the developers and
[THANKS](https://github.com/KuchiSofts/vlc-redesign/blob/main/THANKS) for the libraries VLC uses.

> [!IMPORTANT]
> VLC Redesign is unofficial: it is not made, affiliated with, or endorsed by VideoLAN. *VLC*, *VLC media player*
> and the cone logo are trademarks of VideoLAN.

## Fonts

| Font | Authors | License |
|---|---|---|
| [Inter](https://github.com/rsms/inter) 3.19, modified (every advance width one unit wider, so Qt matches the design's spacing) | The Inter Project Authors | SIL Open Font License 1.1 |
| [DM Mono](https://github.com/googlefonts/dm-mono) | The DM Mono Project Authors | SIL Open Font License 1.1 |

The Windows package ships both licenses in its `licenses` folder.

## Demo media

The demo video and screenshots of the project use free media:

- The *[Sintel](https://durian.blender.org/)* and *[Big Buck Bunny](https://peach.blender.org/)* trailers,
  © Blender Foundation, [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) (Sintel's first second trimmed).
- "Fluffing a Duck" and "Carefree" by [Kevin MacLeod](https://incompetech.com/) (incompetech.com),
  [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) (generated cover art added).

## License

Like VLC, VLC Redesign is free software under the **GNU General Public License version 2 or later**
([COPYING](https://github.com/KuchiSofts/vlc-redesign/blob/main/COPYING)). Every release links to the exact source
it was built from.

| What | License |
|---|---|
| VLC and this fork's code | GNU GPL v2 or later ([COPYING](https://github.com/KuchiSofts/vlc-redesign/blob/main/COPYING)) |
| libVLC | GNU LGPL v2.1 or later ([COPYING.LIB](https://github.com/KuchiSofts/vlc-redesign/blob/main/COPYING.LIB)) |
| Interface design and the icons derived from it | CC BY 4.0, by Juxtopposed |
| Inter, DM Mono | SIL Open Font License 1.1 |
| Third-party libraries in the Windows build | Their own licenses, see [THANKS](https://github.com/KuchiSofts/vlc-redesign/blob/main/THANKS) and `contrib/` |

The full credits are in [CREDITS.md](https://github.com/KuchiSofts/vlc-redesign/blob/main/CREDITS.md).

---

[Design check](Design-Check) · [Building from source](Building-from-Source) · [Home](Home)
