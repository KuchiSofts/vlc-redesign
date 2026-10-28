# Design assets (design-1.0)

The shared design of the VLC Redesign: the values, icons and fonts every platform builds from. The desktop app
is the reference; the other platforms (Android, iPhone and iPad, Apple TV, the Samsung TV receiver, the website)
generate their own files from this folder instead of copying values by hand.

| Path | What |
|---|---|
| [`tokens.json`](tokens.json) | The design tokens: colours (dark and light), accents and their rule, fonts, sizes and spacing, corner radii, icon sizes, shadows, opacities, window sizes |
| [`icons/`](icons/) | The icon set, per name and state (the SVG of every icon from Figma, and the PNG with `@2x` it replaced), with [`index.json`](icons/index.json) and a [preview](icons/README.md) |
| [`fonts/`](fonts/) | Inter 3.19 (modified) and DM Mono, with their licenses and [a note on the change to Inter](fonts/README.md) |
| [`build/`](build/) | Generated: `tokens.css` (CSS variables), `colors.xml` (Android), `Tokens.swift` (Swift) |
| [`tools/`](tools/) | `export.py` (writes `build/`), `sync_assets.py` (copies icons and fonts from the app), `check_qml.py` (the CI check) |

## Version

These assets are versioned on their own: **`design-1.0`**. The version is in `tokens.json`
(`$extensions["org.vlc-redesign"].version`), in `icons/index.json` and in the header of every generated file, and
every release names the design version it was built with. A change of a value or an icon raises it
(`design-1.1`; a renamed or removed token: `design-2.0`).

## tokens.json

The [W3C Design Tokens Community Group format](https://tr.designtokens.org/format/): each token has a `$value`, a
`$type` (`color`, `dimension`, `number`, `fontFamily`, `fontWeight`, `shadow`; a group's `$type` applies to what it
holds) and a `$description`. Values are at 100 % (1 px = 1 Figma px); the desktop app scales them with the
interface scale.

```
color.dark.*   color.light.*      the theme colours, named after the Figma styles (bg.b1 = Dark/BG/B1, fg.a2 = Dark/FG/A2...)
accent.*                          the default accent, the 9 choices (main, dim, bright, brightLight) and the rule
font.*                            families, weights, sizes, line metrics
size.*  radius.*  icon.*          sizes and spacing, corner radii, icon boxes
shadow.*  opacity.*  window.*     shadows, panel opacity, window and dialog sizes
```

`$extensions["org.vlc-redesign"]` of a token gives:

- `figma`: the node of the [Figma file](https://www.figma.com/community/file/1691465999134370980) the value comes
  from (it opens with `https://www.figma.com/design/fQSxoEi1zmnD5fQacw0qGs?node-id=<id, with - for :>`), and
  `figmaStyle` for the colours that are Figma styles;
- `code`: the places in the desktop code that hold the value (file and the text around it). `check_qml.py` reads
  them. Tokens without `code` are values no single line of the code holds (a size computed from others, a
  dialog's height made of its parts); they come from the design tables and the pixel check;
- `darkToken` (light colours): the dark colour it is the light version of.

**The light theme** is derived: the app draws the dark value and `VLCStyle.redesign.color()` turns it into the
light one (measured from Figma *Player - Light*). A platform with real theme support uses `color.dark.*` and
`color.light.*` directly.

**Accents.** The user picks one of 9 accents (or the system's). For any accent: Dim = 0.81 × each channel,
Bright = 0.26 × each channel (dark theme), Bright in the light theme = `#DEDEDE` tinted with 35 % of the accent;
the default orange keeps the design's own Dim and Bright (`#AB6314`, `#371F06`). A system accent is brought into
the luminance range of the 9 (0.128 to 0.429, WCAG) by changing only its HSL lightness. `accent.choice` holds the
computed values for the 9; a platform that offers the system accent applies `accent.rule`.

## Generating platform files

```
python design/tools/export.py            # writes design/build/
python design/tools/export.py --check    # exit 1 when design/build is stale
```

Standard library only (Python 3.8+). It writes:

- **`build/tokens.css`**: every token as a custom property (`--vlc-color-dark-bg-b1: #0E0E0E;`,
  `--vlc-size-top-bar-height: 40px;`, shadows as `box-shadow` values), plus `--vlc-theme-*` for the colours of the
  current theme: dark on `:root`, light under `[data-theme="light"]`. For the website and the Tizen TV app.
- **`build/colors.xml`**: Android colour resources, `@color/vlc_color_dark_bg_b1` (`#AARRGGBB`). Sizes stay in
  `tokens.json` (dp on Android equals the Figma px).
- **`build/Tokens.swift`**: `VLCTokens.Color.darkBgB1`, `VLCTokens.Size.topBarHeight`... with `VLCColor` (sRGB +
  alpha) and `VLCShadow`, no UIKit or SwiftUI import, so the same file serves iOS, iPadOS and tvOS (add a
  `Color(_:)` extension where it is used).

Another platform can read `tokens.json` itself (it is plain JSON) or use a token tool that reads the DTCG format
(Style Dictionary and others). The generated files are committed so a sister repository can take them as they are;
a workflow copies `design/` into each repository when it changes.

## Keeping the desktop app and this folder in step

```
python design/tools/sync_assets.py       # after an icon or a font changes in the app
python design/tools/export.py            # after tokens.json changes
python design/tools/check_qml.py         # what CI runs
```

`check_qml.py` fails when a value in `tokens.json` and the code differ (every `code` place of every token,
VLCStyle's accent list, the light colour map of `VLCStyle.redesign.color()` both ways, the Dim and Bright of each
accent against the rule), when `icons/` or `fonts/` differ from the app's files, or when `build/` is stale. It runs
on every pull request (*Design tokens* job of `.github/workflows/build-windows.yml`). A change of a design value
therefore changes the code, `tokens.json` and `build/` in the same commit.

## License

- **Design values, icons and everything derived from the design** (`tokens.json`, `icons/`, `build/`): derived from
  **"VLC Redesign (Community)" by Juxtopposed** (<https://www.figma.com/community/file/1691465999134370980>),
  licensed under **[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)**. Changes: the values were measured
  and implemented in VLC's Qt interface, a light theme and accent rule were derived, icons were redrawn as SVG/PNG
  and modified (two-tone menu icons, light-theme variants). Juxtopposed does not endorse this project. Keep this
  credit when you use them. See [CREDITS.md](../CREDITS.md).
- **Fonts** (`fonts/`): SIL Open Font License 1.1 (Inter: The Inter Project Authors; DM Mono: The DM Mono Project
  Authors). See [fonts/README.md](fonts/README.md).
- **Tools** (`tools/`): GPL-2.0-or-later, as VLC.
