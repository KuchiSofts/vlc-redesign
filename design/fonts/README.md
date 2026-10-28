# Fonts

The two fonts of the VLC Redesign, as the desktop app ships them (`modules/gui/qt/pixmaps/fonts`, copied by
`design/tools/sync_assets.py`; `check_qml.py` fails if they drift apart).

| File | Font | Used for | License |
|---|---|---|---|
| `InterVariable.ttf` | [Inter](https://github.com/rsms/inter) 3.19, variable, **modified** | Every text of the interface | SIL OFL 1.1 ([LICENSE.txt](LICENSE.txt)) |
| `DMMono-Regular.ttf` | [DM Mono](https://github.com/googlefonts/dm-mono) Regular | The keycaps of Keyboard Shortcuts | SIL OFL 1.1 ([DMMono-OFL.txt](DMMono-OFL.txt)) |

## The Inter modification

`InterVariable.ttf` is Inter 3.19 with **every advance width one font unit wider**, nothing else changed. Figma
lays the design out with Inter 3.19 and rounds its text boxes up; Qt truncates advances to 1/64 px, so with the
original file every line of text comes out a fraction of a pixel shorter than in the design and wraps or elides in
other places. One unit more per glyph makes Qt's layout match the design's. The base is 3.19 because that is the
build the design's text boxes match.

The font keeps its name and the SIL Open Font License 1.1. Its copyright and the note above are in
[LICENSE.txt](LICENSE.txt). Other platforms use this same file so the spacing is the same everywhere; one whose
layout does not truncate advances should compare it with the original Inter 3.19 against the Figma frames
before choosing.

How the app draws it: distance-field rendering, no hinting, greyscale antialiasing; a line is 1.2109375 em
(ascender 0.96875, descender 0.2421875) centred in a box of whole pixels (17 px for 14 px text). DM Mono: a line
of 1.302 em (ascender 0.992, descender 0.31). The same values are in `design/tokens.json` (`font.metrics`).
