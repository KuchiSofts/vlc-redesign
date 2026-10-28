# Icon SVGs against their PNGs

Written by `design/tools/icon_svgs.py compare`. Each SVG (`<state>.svg` next to its PNG in
`modules/gui/qt/pixmaps/redesign/icons`) is drawn by QtSvg (Qt 6.8, the renderer of the app)
and compared with the PNG of the same state, in levels of 0-255:

- **24**: drawn at the PNG's size, premultiplied RGBA pixel for pixel (max / mean).
- **shape**: mean difference of the alpha averaged over 3 x 3 pixels: the shape without the
  antialiasing. **colour**: largest channel difference of the mean colour where both are opaque.
- **48**: drawn at 48 px against the `@2x` PNG, or (↓) averaged back to 24 against the 1x PNG.
- **Figma**: mean alpha difference of the PNG with Figma's own 1x render of the symbol (the
  frame 42:356), when the state shows the whole symbol; **Qt-Figma** the same for the SVG.
- **offset**: the subpixel position the PNG was cut at (an instance placed between pixels),
  kept in the SVG.
- **app**: the app ships the SVG (`assets.qrc`): the exact and same-drawing states but the
  empty ones. It keeps the PNG at 100 % and takes the SVG on a denser screen.

No SVG drawn by QtSvg is the PNG pixel for pixel: Figma's rasteriser antialiases otherwise, and
most PNGs were cut from the design's screens rather than exported from the symbol. The close
states keep their PNG on every screen: their PNG is drawn heavier or lighter than Figma's own
render of the symbol, so the SVG would change their weight at 150 %.

**224 states: 51 exact, 118 same, 38 close, 17 without SVG; 166 in the app.**

## Without SVG (17)

| State | Why |
|---|---|
| `cast/default` | no symbol of 42:356 has this name |
| `cast/large` | no symbol of 42:356 has this name |
| `repeat/active` | another drawing than the symbols of this name (closest 187:470: shape 9.4) |
| `save/default` | Figma's SVG export of 2459:9542 loses its detail (a boolean shape's stroke) |
| `shuffle/active` | another drawing than the symbols of this name (closest 2461:9618: shape 11.3) |
| `sort/active-accent` | 26 x 25: the glyph with a badge of the screen it was cut from, not a symbol of 42:356 |
| `sort/active-grey` | 26 x 25: the glyph with a badge of the screen it was cut from, not a symbol of 42:356 |
| `sort/active` | 26 x 25: the glyph with a badge of the screen it was cut from, not a symbol of 42:356 |
| `subtitle/default` | Figma's SVG export of 1911:8286, 2461:9663 loses its detail (a boolean shape's stroke) |
| `subtitle/dim` | Figma's SVG export of 1911:8286, 2461:9663 loses its detail (a boolean shape's stroke) |
| `video/default` | Figma's SVG export of 999:4810 loses its detail (a boolean shape's stroke) |
| `video/dim` | Figma's SVG export of 999:4810 loses its detail (a boolean shape's stroke) |
| `view/active` | 26 x 25: the glyph with a badge of the screen it was cut from, not a symbol of 42:356 |
| `vumeter/default-accent` | the speed glyph (2115:7386) with its needle in the accent dim, drawn in this repository for the VU meter (no Figma symbol) |
| `vumeter/default-grey` | the speed glyph (2115:7386) with its needle in the accent dim, drawn in this repository for the VU meter (no Figma symbol) |
| `vumeter/default` | the speed glyph (2115:7386) with its needle in the accent dim, drawn in this repository for the VU meter (no Figma symbol) |
| `window/default` | Figma's SVG export of 2107:6312 loses its detail (a boolean shape's stroke) |

## Exact: the PNG is Figma's render of the symbol (51)

| State | Node | 24 max / mean | shape | colour | 48 max / mean | Figma | Qt-Figma | offset | app |
|---|---|---|---|---|---|---|---|---|---|
| `add/default` | 463:550 | 3 / 0.08 | 0.11 | 1 | 9 / 0.15 | 0.1 | 0.11 |  | yes |
| `audio/default` | 44:407 | 2 / 0.13 | 0.2 | 0 | 124 / 0.5 | 0.02 | 0.22 |  | yes |
| `backward-15-sec/default` | 45:36 | 80 / 1.45 | 1.24 | 0 | 48 / 1.13 ↓ | 0.05 | 2.14 |  | yes |
| `bookmark/default` | 2127:7779 | 36 / 0.84 | 0.68 | 0 | 21 / 0.72 ↓ | 0.06 | 1.25 |  | yes |
| `capture-device/default` | 2493:9307 | 64 / 2.42 | 3.17 | 0 | 42 / 1.55 ↓ | 0.03 | 3.61 |  | yes |
| `chevron-down/default` | 71:868 | 36 / 0.42 | 0.52 | 0 | 36 / 0.4 ↓ | 0.72 | 0.59 |  | yes |
| `chevron-right/default` | 71:875 | 29 / 0.4 | 0.52 | 0 | 28 / 0.37 ↓ | 0.37 | 0.58 |  | yes |
| `disc/default` | 5884:24330 | 84 / 3.41 | 3.46 | 0 | 207 / 6.56 | 0.09 | 5.08 |  | yes |
| `divider/default` | 71:863 | 2 / 0.06 | 0.07 | 0 | 2 / 0.07 ↓ | 0.0 | 0.07 |  | yes |
| `EQ/default` | 2372:8559 | 24 / 0.47 | 0.51 | 0 | 24 / 0.45 ↓ | 0.64 | 0.19 |  | yes |
| `eq-classical/default` | 2372:8571 | 31 / 0.23 | 0.29 | 0 | 21 / 0.2 ↓ | 0.03 | 0.39 |  | yes |
| `eq-club/default` | 2372:8578 | 32 / 0.28 | 0.37 | 0 | 24 / 0.2 ↓ | 0.02 | 0.45 |  | yes |
| `eq-dance/default` | 2372:8588 | 52 / 0.56 | 0.76 | 0 | 30 / 0.39 ↓ | 0.07 | 0.92 |  | yes |
| `eq-flat/default-grey` | 2372:8559 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ | 0.01 | 0.19 |  | yes |
| `eq-flat/default` | 2372:8559 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ | 0.01 | 0.19 |  | yes |
| `eq-fullbass/default` | 6114:13932 | 31 / 0.39 | 0.44 | 0 | 21 / 0.34 ↓ | 0.12 | 0.57 |  | yes |
| `eq-fullbasstreble/default` | 6114:13938 | 31 / 0.4 | 0.52 | 0 | 29 / 0.33 ↓ | 0.04 | 0.66 |  | yes |
| `eq-fulltreble/default` | 6114:13944 | 38 / 0.43 | 0.58 | 0 | 27 / 0.34 ↓ | 0.03 | 0.7 |  | yes |
| `eq-headphones/default` | 6114:13955 | 31 / 0.46 | 0.56 | 0 | 21 / 0.38 ↓ | 0.09 | 0.68 |  | yes |
| `eq-largehall/default` | 6114:13961 | 31 / 0.33 | 0.47 | 0 | 29 / 0.26 ↓ | 0.05 | 0.54 |  | yes |
| `eq-live/default` | 6114:13973 | 31 / 0.33 | 0.41 | 0 | 21 / 0.28 ↓ | 0.05 | 0.54 |  | yes |
| `eq-party/default` | 6114:13979 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ | 0.01 | 0.19 |  | yes |
| `eq-pop/default` | 6114:13985 | 31 / 0.2 | 0.19 | 0 | 21 / 0.15 ↓ | 0.02 | 0.33 |  | yes |
| `eq-reggae/default` | 6114:13967 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ | 0.01 | 0.19 |  | yes |
| `eq-rock/default` | 6114:13996 | 35 / 0.38 | 0.41 | 0 | 21 / 0.34 ↓ | 0.06 | 0.59 |  | yes |
| `eq-ska/default` | 6114:14002 | 31 / 0.43 | 0.54 | 0 | 23 / 0.36 ↓ | 0.05 | 0.68 |  | yes |
| `eq-soft/default` | 6114:14008 | 34 / 0.44 | 0.57 | 0 | 24 / 0.31 ↓ | 0.05 | 0.72 |  | yes |
| `eq-softrock/default` | 6114:14014 | 34 / 0.4 | 0.55 | 0 | 27 / 0.3 ↓ | 0.04 | 0.67 |  | yes |
| `eq-techno/default` | 6114:14021 | 37 / 0.37 | 0.49 | 0 | 30 / 0.27 ↓ | 0.04 | 0.6 |  | yes |
| `genre/default` | 6191:13585 | 43 / 1.83 | 1.31 | 0 | 30 / 1.14 ↓ | 0.1 | 2.69 |  | yes |
| `hotkeys/default` | 1089:4450 | 32 / 0.84 | 0.87 | 0 | 98 / 1.36 | 0.1 | 1.22 |  | yes |
| `lists/default` | 217:191 | 2 / 0.12 | 0.19 | 0 | 9 / 0.48 | 0.02 | 0.21 |  | yes |
| `minimize/default` | 48:46 | 2 / 0.03 | 0.05 | 0 | 9 / 0.13 | 0.0 | 0.05 |  | yes |
| `more/default` | 247:418 | 38 / 0.47 | 0.56 | 0 | 28 / 0.51 ↓ | 0.69 | 0.86 |  | yes |
| `more/dim` | 247:418 | 38 / 0.39 | 0.56 | 0 | 28 / 0.43 ↓ | 0.69 | 0.86 |  | yes |
| `move/default` | 463:417 | 2 / 0.07 | 0.1 | 0 | 2 / 0.05 ↓ | 0.01 | 0.11 |  | yes |
| `new-folder/default` | 2139:9022 | 46 / 1.02 | 1.06 | 0 | 35 / 0.73 ↓ | 0.04 | 1.53 |  | yes |
| `play/default` | 44:419 | 34 / 0.58 | 0.68 | 0 | 35 / 0.17 | 0.84 | 0.66 |  | yes |
| `plugins/default` | 2516:9369 | 42 / 1.41 | 1.43 | 0 | 42 / 1.35 ↓ | 0.04 | 2.14 |  | yes |
| `recent/default` | 2154:9699 | 48 / 1.48 | 1.29 | 0 | 109 / 0.74 | 0.05 | 2.23 |  | yes |
| `remove/default` | 463:560 | 2 / 0.04 | 0.06 | 0 | 9 / 0.15 | 0.0 | 0.06 |  | yes |
| `scope/default` | 2139:8064 | 2 / 0.2 | 0.29 | 0 | 2 / 0.13 ↓ | 0.02 | 0.31 |  | yes |
| `settings/default` | 42:355 | 34 / 1.96 | 2.04 | 0 | 45 / 1.09 | 0.07 | 2.96 |  | yes |
| `spectrometer/default` | 2138:8026 | 40 / 3.25 | 2.16 | 0 | 37 / 2.53 ↓ | 0.14 | 5.2 |  | yes |
| `spectrum/default` | 2139:8048 | 5 / 0.26 | 0.4 | 0 | 5 / 0.21 ↓ | 0.04 | 0.44 |  | yes |
| `spectrum/dim` | 2139:8048 | 5 / 0.35 | 0.4 | 0 | 5 / 0.21 ↓ | 0.04 | 0.44 |  | yes |
| `subtitles/default` | 42:398 | 32 / 0.95 | 0.9 | 0 | 33 / 0.56 | 0.08 | 1.36 |  | yes |
| `thumbnail/default` | 2404:9198 | 20 / 0.1 | 0.08 | 0 | 22 / 0.1 ↓ | 0.01 | 0.24 |  | yes |
| `ui/default` | 6075:14088 | 29 / 0.73 | 0.8 | 0 | 161 / 1.93 | 0.05 | 1.16 |  | yes |
| `view/default` | 856:3212 | 20 / 0.54 | 0.5 | 0 | 20 / 0.52 ↓ | 0.05 | 0.83 |  | yes |
| `waveform/default` | 2139:8078 | 2 / 0.19 | 0.28 | 0 | 2 / 0.12 ↓ | 0.02 | 0.3 |  | yes |

## Same drawing: shape and colour of the PNG, another rasteriser (118)

| State | Node | 24 max / mean | shape | colour | 48 max / mean | Figma | Qt-Figma | offset | app |
|---|---|---|---|---|---|---|---|---|---|
| `0/default` | 2107:6596 | 0 / 0.0 | 0.0 | 0 | 0 / 0.0 ↓ |  |  |  |  |
| `add/half` | 463:550 | 11 / 0.14 | 0.16 | 0 | 10 / 0.14 ↓ | 12.38 | 12.4 | 0.5, -0.5 | yes |
| `arrow-left/default` | 71:890 | 111 / 1.83 | 2.56 | 0 | 102 / 0.32 | 2.66 | 1.0 |  | yes |
| `arrow-left/disabled` | 71:890 | 111 / 1.1 | 2.56 | 0 | 102 / 0.18 | 2.66 | 1.0 |  | yes |
| `arrow-right/default` | 71:887 | 89 / 1.97 | 2.7 | 0 | 102 / 0.32 | 3.27 | 0.99 |  | yes |
| `arrow-right/disabled` | 71:887 | 89 / 1.18 | 2.7 | 0 | 102 / 0.18 | 3.27 | 0.99 |  | yes |
| `artist/active` | 858:5164 | 137 / 2.1 | 2.81 | 0 | 57 / 1.72 ↓ | 2.32 | 2.55 |  | yes |
| `artist/default` | 858:5155 | 50 / 2.27 | 2.08 | 0 | 104 / 0.79 | 2.27 | 2.82 |  | yes |
| `audio/active` | 496:2439 | 29 / 0.38 | 0.49 | 0 | 29 / 0.38 ↓ | 1.06 | 0.78 |  | yes |
| `backward-10-sec/default` | 45:28 | 73 / 3.25 | 2.23 | 0 | 70 / 2.91 ↓ | 4.56 | 2.31 |  | yes |
| `check/active` | 494:2394 | 58 / 0.94 | 0.72 | 0 | 46 / 0.85 ↓ | 1.63 | 0.91 |  | yes |
| `check/default` | 494:2400 | 0 / 0.0 | 0.0 | 0 | 0 / 0.0 ↓ |  |  |  |  |
| `check/selected` | 2428:14606 | 85 / 0.85 | 0.61 | 0 | 84 / 0.85 ↓ | 1.39 | 0.84 |  | yes |
| `close/default` | 45:40 | 39 / 0.97 | 1.08 | 0 | 8 / 0.14 | 1.85 | 1.1 |  | yes |
| `edit/dim` | 2437:14888 | 41 / 0.88 | 1.32 | 0 | 35 / 0.87 ↓ | 16.84 | 17.79 | 0, 0.5 | yes |
| `eq-classical/default-accent` | 2372:8571 | 27 / 0.11 | 0.17 | 0 | 19 / 0.1 ↓ |  |  |  | yes |
| `eq-classical/default-grey` | 2372:8571 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ |  |  |  | yes |
| `eq-club/default-accent` | 2372:8578 | 32 / 0.16 | 0.25 | 0 | 24 / 0.1 ↓ |  |  |  | yes |
| `eq-club/default-grey` | 2372:8578 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ |  |  |  | yes |
| `eq-dance/default-accent` | 2372:8588 | 52 / 0.43 | 0.62 | 0 | 30 / 0.29 ↓ |  |  |  | yes |
| `eq-dance/default-grey` | 2372:8588 | 31 / 0.15 | 0.16 | 0 | 25 / 0.13 ↓ |  |  |  | yes |
| `eq-flat/default-accent` | 2372:8559 | 0 / 0.0 | 0.0 | 0 | 0 / 0.0 ↓ |  |  |  |  |
| `eq-fullbass/default-accent` | 6114:13932 | 27 / 0.27 | 0.36 | 0 | 23 / 0.24 ↓ |  |  |  | yes |
| `eq-fullbass/default-grey` | 6114:13932 | 31 / 0.21 | 0.26 | 0 | 28 / 0.19 ↓ |  |  |  | yes |
| `eq-fullbasstreble/default-accent` | 6114:13938 | 29 / 0.28 | 0.4 | 0 | 29 / 0.23 ↓ |  |  |  | yes |
| `eq-fullbasstreble/default-grey` | 6114:13938 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ |  |  |  | yes |
| `eq-fulltreble/default-accent` | 6114:13944 | 38 / 0.31 | 0.46 | 0 | 27 / 0.25 ↓ |  |  |  | yes |
| `eq-fulltreble/default-grey` | 6114:13944 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ |  |  |  | yes |
| `eq-headphones/default-accent` | 6114:13955 | 21 / 0.29 | 0.33 | 0 | 18 / 0.26 ↓ |  |  |  | yes |
| `eq-headphones/default-grey` | 6114:13955 | 31 / 0.21 | 0.25 | 0 | 25 / 0.19 ↓ |  |  |  | yes |
| `eq-largehall/default-accent` | 6114:13961 | 30 / 0.19 | 0.3 | 0 | 29 / 0.14 ↓ |  |  |  | yes |
| `eq-largehall/default-grey` | 6114:13961 | 31 / 0.15 | 0.16 | 0 | 25 / 0.13 ↓ |  |  |  | yes |
| `eq-live/default-accent` | 6114:13973 | 21 / 0.23 | 0.31 | 0 | 21 / 0.19 ↓ |  |  |  | yes |
| `eq-live/default-grey` | 6114:13973 | 31 / 0.15 | 0.16 | 0 | 25 / 0.13 ↓ |  |  |  | yes |
| `eq-party/default-accent` | 6114:13979 | 0 / 0.0 | 0.0 | 0 | 0 / 0.0 ↓ |  |  |  | yes |
| `eq-party/default-grey` | 6114:13979 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ |  |  |  | yes |
| `eq-pop/default-accent` | 6114:13985 | 10 / 0.08 | 0.06 | 0 | 9 / 0.05 ↓ |  |  |  | yes |
| `eq-pop/default-grey` | 6114:13985 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ |  |  |  | yes |
| `eq-reggae/default-accent` | 6114:13967 | 0 / 0.0 | 0.0 | 0 | 0 / 0.0 ↓ |  |  |  | yes |
| `eq-reggae/default-grey` | 6114:13967 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ |  |  |  | yes |
| `eq-rock/default-accent` | 6114:13996 | 35 / 0.26 | 0.26 | 0 | 20 / 0.24 ↓ |  |  |  | yes |
| `eq-rock/default-grey` | 6114:13996 | 31 / 0.15 | 0.16 | 0 | 25 / 0.13 ↓ |  |  |  | yes |
| `eq-ska/default-accent` | 6114:14002 | 27 / 0.31 | 0.39 | 0 | 23 / 0.26 ↓ |  |  |  | yes |
| `eq-ska/default-grey` | 6114:14002 | 31 / 0.15 | 0.16 | 0 | 25 / 0.13 ↓ |  |  |  | yes |
| `eq-soft/default-accent` | 6114:14008 | 34 / 0.32 | 0.45 | 0 | 24 / 0.21 ↓ |  |  |  | yes |
| `eq-soft/default-grey` | 6114:14008 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ |  |  |  | yes |
| `eq-softrock/default-accent` | 6114:14014 | 34 / 0.28 | 0.43 | 0 | 27 / 0.2 ↓ |  |  |  | yes |
| `eq-softrock/default-grey` | 6114:14014 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ |  |  |  | yes |
| `eq-techno/default-accent` | 6114:14021 | 37 / 0.25 | 0.37 | 0 | 30 / 0.17 ↓ |  |  |  | yes |
| `eq-techno/default-grey` | 6114:14021 | 31 / 0.12 | 0.12 | 0 | 21 / 0.1 ↓ |  |  |  | yes |
| `favorite/default` | 2430:14629 | 67 / 2.77 | 2.25 | 0 | 81 / 1.74 | 4.01 | 2.98 |  | yes |
| `folder/active` | 87:1031 | 60 / 1.8 | 2.62 | 0 | 61 / 1.64 ↓ | 2.59 | 1.05 |  | yes |
| `folder/large` | 51:808 | 94 / 0.75 | 0.92 | 0 | 83 / 0.72 ↓ |  |  |  | yes |
| `folder/selected` | 467:502 | 58 / 1.71 | 2.21 | 0 | 59 / 1.53 ↓ | 2.23 | 1.05 |  | yes |
| `forward-10-sec/default` | 45:22 | 83 / 1.96 | 1.64 | 0 | 51 / 1.53 ↓ | 2.47 | 2.54 |  | yes |
| `forward-15-sec/default` | 45:32 | 70 / 2.16 | 1.74 | 0 | 43 / 1.87 ↓ | 3.37 | 2.37 |  | yes |
| `fullscreen/active` | 6031:13553 | 67 / 2.65 | 2.59 | 0 | 67 / 2.69 ↓ | 4.13 | 0.84 |  | yes |
| `fullscreen/default` | 48:54 | 67 / 1.68 | 1.81 | 0 | 98 / 0.33 | 2.56 | 0.74 |  | yes |
| `home/active` | 87:1026 | 62 / 1.21 | 1.5 | 0 | 55 / 1.16 ↓ | 1.97 | 0.75 |  | yes |
| `home/selected` | 470:597 | 65 / 1.01 | 1.21 | 0 | 65 / 0.87 ↓ | 1.57 | 0.82 |  | yes |
| `info/default` | 2139:8985 | 76 / 3.18 | 3.0 | 0 | 127 / 2.43 | 4.31 | 2.95 |  | yes |
| `interface/active` | 1088:4044 | 64 / 0.84 | 0.99 | 0 | 40 / 0.75 ↓ | 1.47 | 1.2 |  | yes |
| `interface/default` | 999:4960 | 58 / 1.25 | 0.98 | 0 | 38 / 1.04 ↓ | 1.83 | 1.81 |  | yes |
| `link/active` | 774:1075 | 89 / 1.62 | 2.14 | 0 | 82 / 1.41 ↓ | 1.98 | 2.44 |  | yes |
| `link/default` | 774:1038 | 109 / 2.43 | 2.73 | 0 | 107 / 2.21 ↓ | 3.13 | 2.14 |  | yes |
| `list/default` | 2107:6531 | 43 / 2.19 | 2.14 | 0 | 45 / 2.21 ↓ | 3.42 | 1.19 |  | yes |
| `list/dim` | 2107:6531 | 43 / 1.85 | 2.14 | 0 | 45 / 1.86 ↓ | 3.42 | 1.19 |  | yes |
| `lists/active` | 467:492 | 54 / 1.58 | 2.01 | 0 | 54 / 1.58 ↓ | 2.28 | 0.94 |  | yes |
| `maximize/default` | 48:51 | 63 / 2.71 | 2.62 | 0 | 98 / 0.34 | 4.06 | 0.39 |  | yes |
| `music/default` | 999:4806 | 83 / 1.93 | 2.33 | 0 | 82 / 1.88 ↓ | 3.25 | 1.61 |  | yes |
| `next/default` | 45:12 | 84 / 3.07 | 2.95 | 0 | 98 / 0.17 | 4.07 | 0.76 |  | yes |
| `paint/active` | 2150:9248 | 47 / 1.46 | 2.08 | 0 | 48 / 1.37 ↓ | 2.01 | 1.69 |  | yes |
| `paint/default` | 2150:9212 | 89 / 2.12 | 2.13 | 0 | 89 / 2.06 ↓ | 3.18 | 2.69 |  | yes |
| `play/outline-active` | 4004:12338 | 60 / 2.04 | 2.05 | 0 | 60 / 2.02 ↓ | 7.09 | 7.03 |  | yes |
| `playback/active` | 3180:17978 | 109 / 2.24 | 2.42 | 0 | 53 / 2.09 ↓ | 3.46 | 0.66 |  | yes |
| `playback/default` | 999:4802 | 65 / 1.24 | 0.76 | 0 | 39 / 1.18 ↓ | 1.94 | 0.97 |  | yes |
| `playback-settings/active` | 2238:9232 | 70 / 1.67 | 1.9 | 0 | 68 / 1.7 ↓ | 3.16 | 1.25 |  | yes |
| `playback-settings/default` | 2230:9222 | 58 / 2.72 | 2.61 | 0 | 254 / 2.15 | 3.92 | 2.78 |  | yes |
| `playlist/active` | 858:5098 | 56 / 1.67 | 2.11 | 0 | 37 / 1.26 ↓ | 2.84 | 2.24 |  | yes |
| `preferences/active` | 1803:2965 | 50 / 1.32 | 1.6 | 0 | 46 / 1.2 ↓ | 1.53 | 1.19 |  | yes |
| `previous/default` | 45:18 | 40 / 1.21 | 1.22 | 0 | 98 / 0.18 | 1.68 | 0.72 |  | yes |
| `privacy/active` | 3050:10424 | 45 / 1.85 | 2.19 | 0 | 46 / 1.49 ↓ | 2.68 | 2.4 |  | yes |
| `privacy/default` | 3046:10386 | 89 / 2.21 | 1.71 | 0 | 70 / 1.98 ↓ | 3.22 | 2.32 |  | yes |
| `profile/default` | 2437:14784 | 41 / 1.49 | 1.51 | 0 | 35 / 0.59 | 1.48 | 2.12 |  | yes |
| `recent/active-accent` | 2493:9246 | 58 / 0.35 | 0.5 | 0 | 20 / 0.04 |  |  |  | yes |
| `recent/active-grey` | 2493:9246 | 93 / 2.31 | 2.08 | 0 | 109 / 0.7 |  |  |  | yes |
| `recent/active` | 2154:9699 | 116 / 3.38 | 2.51 | 0 | 119 / 1.71 | 3.69 | 2.23 |  | yes |
| `right-sidebar/default` | 6586:17393 | 46 / 2.69 | 2.14 | 0 | 46 / 2.58 ↓ | 3.67 | 0.64 |  | yes |
| `scope/default-accent` | 2139:8064 | 2 / 0.12 | 0.16 | 0 | 2 / 0.07 ↓ |  |  |  | yes |
| `scope/default-grey` | 2139:8064 | 2 / 0.09 | 0.13 | 0 | 2 / 0.06 ↓ |  |  |  | yes |
| `search/default` | 856:2147 | 79 / 2.31 | 2.41 | 0 | 66 / 0.6 | 2.74 | 2.45 |  | yes |
| `search/dim` | 856:2147 | 79 / 1.94 | 2.41 | 0 | 66 / 0.51 | 2.74 | 2.45 |  | yes |
| `settings/active` | 470:507 | 62 / 1.91 | 2.1 | 0 | 60 / 1.88 ↓ | 3.34 | 1.33 |  | yes |
| `shuffle/default` | 187:474 | 139 / 2.7 | 2.98 | 0 | 75 / 2.28 ↓ | 3.1 | 2.77 |  | yes |
| `sidebar/active` | 87:1041 | 34 / 3.07 | 2.68 | 0 | 58 / 3.11 ↓ | 4.51 | 0.64 |  | yes |
| `sidebar/default` | 69:814 | 48 / 2.33 | 2.56 | 0 | 46 / 2.33 ↓ | 3.34 | 0.64 |  | yes |
| `sort/default` | 463:564 | 68 / 1.66 | 1.69 | 0 | 43 / 1.5 ↓ | 2.45 | 1.57 |  | yes |
| `sort/dim` | 463:564 | 68 / 1.4 | 1.69 | 0 | 43 / 1.27 ↓ | 2.45 | 1.57 |  | yes |
| `spectrometer/default-accent` | 2138:8026 | 40 / 0.86 | 0.93 | 0 | 28 / 0.59 ↓ |  |  |  | yes |
| `spectrometer/default-grey` | 2138:8026 | 38 / 2.39 | 1.78 | 0 | 37 / 1.94 ↓ |  |  |  | yes |
| `spectrum/default-accent` | 2139:8048 | 5 / 0.07 | 0.11 | 0 | 5 / 0.08 ↓ |  |  |  | yes |
| `spectrum/default-grey` | 2139:8048 | 2 / 0.19 | 0.29 | 0 | 2 / 0.13 ↓ |  |  |  | yes |
| `speed/default` | 2115:7386 | 76 / 4.13 | 2.98 | 0 | 76 / 3.91 ↓ | 5.84 | 3.01 |  | yes |
| `stop/default` | 187:322 | 63 / 1.69 | 2.51 | 0 | 63 / 1.75 ↓ | 2.69 | 0.18 |  | yes |
| `streaming/default` | 70:833 | 92 / 3.71 | 2.76 | 0 | 76 / 3.53 ↓ | 5.61 | 2.66 |  | yes |
| `streaming/selected` | 477:1208 | 73 / 3.53 | 2.68 | 0 | 71 / 3.28 ↓ | 4.85 | 3.09 |  | yes |
| `subtitles/active` | 467:497 | 52 / 2.36 | 2.83 | 0 | 42 / 2.26 ↓ | 3.94 | 1.69 |  | yes |
| `swap/default` | 1915:8362 | 73 / 1.97 | 2.42 | 0 | 66 / 1.9 ↓ | 3.31 | 1.52 |  | yes |
| `tv/active` | 477:1101 | 47 / 1.52 | 1.83 | 0 | 47 / 1.48 ↓ | 2.5 | 0.75 |  | yes |
| `tv/default` | 477:1096 | 59 / 2.87 | 2.39 | 0 | 36 / 2.66 ↓ | 3.97 | 1.28 |  | yes |
| `tv/selected` | 477:1107 | 39 / 1.93 | 2.19 | 0 | 37 / 1.84 ↓ | 2.76 | 0.75 |  | yes |
| `ui/default-accent` | 6075:14088 | 29 / 0.29 | 0.47 | 1 | 161 / 0.73 |  |  |  | yes |
| `ui/default-grey` | 6075:14088 | 20 / 0.44 | 0.32 | 0 | 134 / 1.2 |  |  |  | yes |
| `video/active` | 1102:5055 | 72 / 1.93 | 2.59 | 0 | 73 / 1.88 ↓ | 3.22 | 1.06 |  | yes |
| `waveform/default-accent` | 2139:8078 | 2 / 0.09 | 0.12 | 0 | 2 / 0.05 ↓ |  |  |  | yes |
| `waveform/default-grey` | 2139:8078 | 2 / 0.11 | 0.16 | 0 | 2 / 0.07 ↓ |  |  |  | yes |
| `wrench/default` | 2107:6319 | 84 / 3.3 | 2.93 | 0 | 84 / 3.2 ↓ | 5.05 | 1.8 |  | yes |
| `zoom/default` | 2154:9680 | 50 / 2.38 | 2.15 | 0 | 49 / 2.09 ↓ | 2.82 | 2.42 |  | yes |

## Close: the symbol, its PNG drawn heavier or lighter (38)

| State | Node | 24 max / mean | shape | colour | 48 max / mean | Figma | Qt-Figma | offset | app |
|---|---|---|---|---|---|---|---|---|---|
| `accessibility/default` | 999:4914 | 51 / 4.24 | 3.09 | 0 | 56 / 4.42 ↓ | 7.17 | 4.8 |  |  |
| `album/default` | 6206:13629 | 63 / 4.44 | 5.87 | 0 | 63 / 4.41 ↓ | 6.41 | 1.38 |  |  |
| `bookmarks/default` | 2127:7548 | 100 / 4.08 | 4.89 | 0 | 82 / 4.02 ↓ | 6.05 | 1.19 |  |  |
| `chapters/default` | 2127:7531 | 63 / 3.49 | 4.24 | 0 | 63 / 3.49 ↓ | 5.27 | 0.61 |  |  |
| `collapse-sidebar/default` | 858:5703 | 79 / 3.53 | 4.4 | 0 | 72 / 3.47 ↓ | 5.17 | 1.21 |  |  |
| `edit/default` | 2437:14888 | 63 / 2.96 | 3.73 | 0 | 63 / 2.97 ↓ | 4.84 | 1.48 |  |  |
| `expand-sidebar/default` | 858:5697 | 117 / 3.7 | 4.03 | 0 | 63 / 3.55 ↓ | 5.04 | 1.22 |  |  |
| `favorite/active` | 2430:14666 | 106 / 2.61 | 4.23 | 0 | 54 / 0.8 ↓ | 1.45 | 3.81 |  |  |
| `file/default` | 2461:9644 | 198 / 4.69 | 4.4 | 0 | 198 / 4.58 ↓ | 8.35 | 3.58 |  |  |
| `folder/default` | 51:808 | 73 / 4.36 | 5.48 | 0 | 74 / 4.28 ↓ | 6.32 | 1.47 |  |  |
| `grid/default` | 2107:6523 | 60 / 4.32 | 4.79 | 0 | 60 / 4.37 ↓ | 6.41 | 1.57 |  |  |
| `grid/dim` | 2107:6523 | 60 / 3.62 | 4.79 | 0 | 60 / 3.68 ↓ | 6.41 | 1.57 |  |  |
| `HDR/default` | 2146:9032 | 81 / 7.11 | 7.81 | 0 | 66 / 6.95 ↓ | 10.77 | 2.99 |  |  |
| `home/default` | 70:828 | 67 / 2.9 | 3.05 | 0 | 44 / 0.37 | 4.02 | 1.34 |  |  |
| `hotkeys/active` | 1089:4464 | 44 / 3.21 | 3.89 | 0 | 44 / 3.3 ↓ | 6.66 | 2.64 |  |  |
| `open/default` | 330:267 | 137 / 2.73 | 3.72 | 0 | 74 / 2.54 ↓ | 3.82 | 1.62 |  |  |
| `open/dim` | 330:267 | 137 / 2.28 | 3.72 | 0 | 74 / 2.13 ↓ | 3.82 | 1.62 |  |  |
| `pause/default` | 44:427 | 60 / 3.38 | 4.06 | 0 | 59 / 3.34 ↓ | 5.05 | 0.33 |  |  |
| `PiP/default` | 2139:8439 | 78 / 4.09 | 5.79 | 0 | 63 / 4.35 ↓ | 7.19 | 1.41 |  |  |
| `playlist/default` | 42:357 | 94 / 3.56 | 3.99 | 0 | 122 / 1.1 | 4.36 | 2.12 |  |  |
| `plugins/active` | 2516:9375 | 90 / 2.97 | 3.34 | 0 | 96 / 2.48 ↓ | 3.85 | 2.26 |  |  |
| `preferences/default` | 1803:2965 | 93 / 3.57 | 5.32 | 0 | 93 / 3.69 ↓ | 6.44 | 1.19 |  |  |
| `queue/default` | 2127:7523 | 63 / 3.48 | 3.53 | 0 | 98 / 2.17 | 4.89 | 0.72 |  |  |
| `quit/default` | 6073:14046 | 119 / 3.57 | 4.57 | 0 | 176 / 1.27 | 4.76 | 1.62 |  |  |
| `record/active` | 6571:16751 | 92 / 3.73 | 3.53 | 0 | 92 / 3.63 ↓ | 5.55 | 2.73 |  |  |
| `record/default` | 2115:7482 | 75 / 4.1 | 3.35 | 0 | 75 / 4.04 ↓ | 5.85 | 2.73 |  |  |
| `refresh/default` | 1622:3137 | 114 / 5.12 | 4.95 | 0 | 86 / 5.06 ↓ | 8.16 | 5.56 |  |  |
| `repeat/default` | 187:470 | 109 / 2.56 | 3.14 | 0 | 63 / 2.13 ↓ | 3.2 | 2.01 |  |  |
| `replay/default` | 1919:9572 | 87 / 3.6 | 3.81 | 0 | 84 / 3.65 ↓ | 6.26 | 4.7 |  |  |
| `snapshot/default` | 2115:7492 | 81 / 5.02 | 5.8 | 0 | 76 / 4.93 ↓ | 7.5 | 1.54 |  |  |
| `songs/active` | 87:1009 | 86 / 3.39 | 4.83 | 0 | 86 / 3.37 ↓ | 5.68 | 1.28 |  |  |
| `songs/default` | 71:851 | 86 / 5.17 | 5.93 | 0 | 86 / 5.13 ↓ | 7.77 | 2.14 |  |  |
| `songs/selected` | 470:592 | 87 / 3.39 | 4.46 | 0 | 73 / 3.26 ↓ | 4.96 | 1.28 |  |  |
| `speed/active` | 2115:7391 | 74 / 2.92 | 3.06 | 0 | 74 / 2.76 ↓ | 4.83 | 3.47 |  |  |
| `streaming/active` | 477:1202 | 92 / 3.82 | 3.31 | 0 | 84 / 3.53 ↓ | 5.86 | 3.09 |  |  |
| `videos/active` | 87:1015 | 64 / 2.63 | 3.89 | 0 | 63 / 2.5 ↓ | 4.25 | 1.11 |  |  |
| `videos/default` | 71:857 | 63 / 5.35 | 5.89 | 0 | 63 / 5.06 ↓ | 7.43 | 1.88 |  |  |
| `videos/selected` | 470:587 | 77 / 3.57 | 4.81 | 0 | 63 / 3.44 ↓ | 5.22 | 1.11 |  |  |

