# Credits and license

Who made what in VLC Redesign, and under which licenses. In short: the **design** is Juxtopposed's (CC BY 4.0), the
**engine** is VideoLAN's VLC (GPL v2 or later), and this project joins the two.

## Design: Juxtopposed

The interface of VLC Redesign is the work of **Juxtopposed**. This project turns the design into software; the
design itself (layouts, colours, typography, icons, interactions) is theirs.

- **Video:** [I Redesigned VLC After 25 Years](https://www.youtube.com/watch?v=ERFeOgqk324)
- **Figma file:** [VLC Redesign (Community)](https://www.figma.com/community/file/1691465999134370980)
- **YouTube:** [@juxtopposed](https://www.youtube.com/@juxtopposed) · **Website:** [juxtopposed.com](https://juxtopposed.com)
  · **Figma:** [@juxtopposed](https://www.figma.com/@juxtopposed) · **GitHub:** [juxtopposed](https://github.com/juxtopposed)
  · **X / Twitter:** [@juxtopposed](https://twitter.com/juxtopposed) · **Bluesky:**
  [@juxtopposed.bsky.social](https://bsky.app/profile/juxtopposed.bsky.social) · **Instagram:**
  [@juxtopposed](https://instagram.com/juxtopposed)

> The user interface design is based on **"VLC Redesign (Community)" by Juxtopposed**
> (<https://www.figma.com/community/file/1691465999134370980>), licensed under
> **[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)**.
> Changes: the design was implemented in Qt/QML inside VLC 4; icons were redrawn as SVG/PNG and
> modified (two-tone menu icons, light-theme variants); some layouts were adapted to VLC features
> and to window sizes the design doesn't cover. Juxtopposed does not endorse this project.

The same credit is in the app, under **⋯ → About VLC**, and in the Credits page of that window, and on the first
page of the installer.

## Implementation

- **MrDouglax**: the first implementation, [JuxtopposedVLC](https://github.com/SrDouglax/JuxtopposedVLC) (the native
  shell, player, library, playlists, subtitles, settings, onboarding and effects, validated on Linux). This
  repository keeps that history.
- **KuchiSofts**: this fork, the Windows build, the installer and updates, and what the
  [changelog](https://github.com/KuchiSofts/vlc-redesign/blob/main/CHANGELOG.md) lists since.

## Translators

The 72 languages build on the translations of VLC's own volunteer translators, completed for the texts of the
redesign. Brazilian Portuguese is by [Kinglucas962](https://github.com/Kinglucas962). VLC's translators are named in the
header of each catalog (`po/<code>.po`). To help: [Languages](Languages#translate).

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

The pictures of this wiki and of the README, and the tour video, use free films and music:

| Media | By | License |
|---|---|---|
| *Caminandes: Gran Dillama* | © Blender Foundation | CC BY |
| *[Sintel](https://durian.blender.org/)*, *Tears of Steel*, *[Big Buck Bunny](https://peach.blender.org/)* | © Blender Foundation | [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) |
| *Elephants Dream* | © Blender Foundation | CC BY 2.5 |
| Music | [Kevin MacLeod](https://incompetech.com/) (incompetech.com) | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |

The films appear in pictures only. The music covers were made for the demo.

## Code signing

The releases are not code-signed. Code signing is paused: an application to SignPath Foundation's free program for
open-source projects waits for its answer. The policy it would follow:
[CODE_SIGNING_POLICY.md](https://github.com/KuchiSofts/vlc-redesign/blob/main/CODE_SIGNING_POLICY.md). Until then
the SHA-256 of every file is listed in `SHA256SUMS.txt`.

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
| Demo media in the pictures | Their own CC licenses (above) |

The Windows package and the installer's *Notice and licenses* page carry the notices. VLC is GPL v2 or later; on
some platforms it is in effect GPL v3 because of the licenses of its dependencies, as VideoLAN notes.

The full credits are in [CREDITS.md](https://github.com/KuchiSofts/vlc-redesign/blob/main/CREDITS.md).

---

[Design check](Design-Check) · [Building from source](Building-from-Source) · [Home](Home)
