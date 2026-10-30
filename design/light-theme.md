# Light theme: the colour decisions

The light theme of the desktop app is drawn from the dark values: `VLCStyle.redesign.color(dark, light)` returns
`dark` in the dark theme and, in the light theme:

1. transparent, the accent, Accent Dim as they are, and Accent Bright as `#DEDEDE` tinted with 35 % of the accent;
2. **the light value the call gives**, when it gives one;
3. otherwise the general light map (`_lightColors` in `VLCStyle.qml`, measured from Figma *Player - Light*), or the
   dark value itself when the map does not know it.

Until 2026-10-28 step 3 came before step 2: whenever the map knew the dark value, the light value the component
gave was ignored. `design/tools/check_qml.py` found 113 such calls with two literal colours; a full scan found
228 (below). Each was settled against Figma, and `check_qml.py` now fails on any call whose light value differs
from the map's unless it is listed in `LIGHT_EXCEPTIONS` (none today).

## The Figma evidence

The Figma file has one light frame, *Player - Light* (2425:13624, exported 1:1 as
`qa/figma/2425-13624_Player-Light.png`). Its dark twin is *Player* (187:328): the same layout, so the two
exports were compared pixel by pixel (88 % of the pixels map one dark colour to one light colour; the rest is
different content: the list, the title). What the light frame shows:

| Dark (187:328) | Light (2425:13624) | Where |
|---|---|---|
| `#1B1B1B` | `#ABABAA` | top bar, control bar |
| `#191919` | `#ABABAA` | navigation column |
| `#0F0F0F` | `#C2C2C1` | library panel |
| `#090909` | `#DEDEDE` | seek bar track |
| `#8F8F8F` | `#2E2E2E` | window title, folder name, navigation icons, times, crumb, volume, subtitles icons |
| `#6B6B6B` | `#6B6B6B` | *15 items*, sort, view, search, ⋯ and open-outside icons |
| `#727272` | `#4D4D4D` | window buttons, queue and equalizer icons |
| `#371F06` | `#D8BA9A` | seek bar fill (Accent Bright light) |

Not taken from the frame, on purpose: previous, next and settings stay `#8F8F8F` there while every other icon of
the bar turns `#2E2E2E` (they read as disabled; the app draws them as the other icons), and the library rows are
the compact library's own light values (already measured, `CompactLibrary.qml`).

### The Light colour styles of the file

The file has no variables (one empty collection) but 24 *Light/* colour styles beside the 25 *Dark/* ones (read
2026-10-28 from an editable copy of the community file, same node ids):

| Style | Dark | Light | Used in *Player - Light* |
|---|---|---|---|
| FG/1 | `#EAEAEA` | `#EAEAEA` | no (the light style keeps the dark value) |
| FG/2 | `#C6C6C6` | `#C6C6C6` | no (the light style keeps the dark value) |
| FG/A1 | `#969696` | `#3C3C3C` | no (the frame still uses Dark/FG/A1 for the disabled forward arrow) |
| FG/A2 | `#8F8F8F` | `#2E2E2E` | yes: icons, title, folder name, times |
| FG/A3 | `#6B6B6B` | `#6B6B6B` | yes: *15 items*, the panel's icons |
| FG/A4 | `#404040` | `#3B3B3B` | no |
| FG/B1 | `#161616` | `#161616` | the play icon uses Dark/FG/B1 |
| BG/A1 | `#222222` | `#EDEDED` | no |
| BG/A2 | `#1F1F1F` | `#DDDDDD` | no |
| BG/A3 | `#1B1B1B` | `#D5D5D5` | as *A3 Transparent* (`#D5D5D5` at 75 %): top bar, control bar, navigation column |
| BG/A4 | `#191919` | `#B3B3B3` | no |
| BG/A5 | `#151515` | `#888888` | yes: the volume track (Dark/BG/B2 in the dark frame) |
| BG/B1 | `#0E0E0E` | `#E7E7E7` | as *B1 Transparent* (`#E7E7E7` at 80 %): the library panel |
| BG/B2 | `#090909` | `#DEDEDE` | yes: the seek track |
| Divider/Dim | `#222222` | `#ACACAC` | yes: the control bar's top line (the panel's separator keeps Dark/Divider/Dim) |
| Divider/Bright | `#373737` | `#9B9B9B` | yes: the top bar's divider |
| Accent Main / Dim / Bright / Logo | `#D37918`... | `#E8851A`, `#965916`, `#FFA13B`, `#C2711A` | Main on the seek handle and the sort dot, Logo |
| BG/Accent Bright, Accent Dark | `#371F06`, `#1A1611` | `#D8BA9A`, `#C6AB89` | Accent Bright on the seek fill |

**The frame's greys are translucent panels over the video.** The frame's background is a dark image (`#2D2B29`
where it shows), and the light panels are *A3 Transparent* and *B1 Transparent* over it: `#D5D5D5` at 75 % over
`#2D2B29` gives `#ABABAA`, `#E7E7E7` at 80 % gives `#C2C2C1`, exactly the colours measured and used by the app's
map. The app draws those composites as solid colours on every screen, which matches the only light frame 1:1. The
opaque *Light/* styles are much lighter (`#E7E7E7`, `#D5D5D5`, `#EDEDED`...); drawing them, or the panels
translucent over the video, would be a new light theme, no longer the frame's colours. **Decided 2026-10-29: the
frame's colours stay** (§6 of the master plan). The light styles the frame does not use are also not a reliable mapping:
FG/1 and FG/2 keep their dark values (white text on light grey), FG/A4 `#3B3B3B` is darker than FG/A3 `#6B6B6B`
(disabled darker than dim), and the volume track is A5 in light but B2 in dark.

So, where the frame does not draw a colour, the map's value stands: it is the grey ramp of the frame (`#2E2E2E`,
`#4D4D4D`, `#6B6B6B`, `#8F8F8F`, `#929291`, `#9C9C9B`, `#ABABAA`, `#B5B5B4`, `#C2C2C1`, `#DEDEDE`), and the light
values the components gave (`#5A5A59`, `#7C7C7B`, `#1E1E1E`, `#C4C4C3`, `#B8B8B7`, `#FAFAFA`...) are neither in the
frame nor among the Light styles.

## Decisions

All 228 follow the map: every value the frame draws is the map's, never the light value the call gave. No call
needed a third value. The light theme therefore looks as before; the code now says what it draws.

Outside `color()`: the title bar of the classic (widget) dialogs was `#9C9C9B` in the light theme and the QML
dialogs' `#DEDEDE` (B2, the seek track's light value); both are `#DEDEDE` now (`redesign_widget_theme.cpp`,
`color.light.titleBar.classic`).

| Dark | Light (map) | Calls | Light values they gave | Figma *Player - Light* (2425:13624) |
|---|---|---:|---|---|
| `#8F8F8F` | `#2E2E2E` | 57 | `#4d4d4d` ×19, `#7c7c7b` ×13, `#5a5a59` ×10, `theme.fg.primary` ×6, `theme.fg.secondary` ×4, `root.colorContext.fg.primary` ×2, `#8f8f8f` ×1, `#292929` ×1, `colorContext.fg.primary` ×1 | Measured: the folder name *S01*, the window title, the nav icons, *00:02:10* / *00:25:40*, the crumb *The Wand* read `#2E2E2E` (`#8F8F8F` in *Player* 187:328) |
| `#6B6B6B` | `#6B6B6B` | 40 | `#5a5a59` ×19, `#4d4d4d` ×13, `#555555` ×3, `#7c7c7b` ×2, `root.colorContext.fg.secondary` ×1, `#9c9c9b` ×1, `theme.fg.secondary` ×1 | Measured: *15 items*, the sort, view and search icons, ⋯ and the open-outside icon read `#6B6B6B` (unchanged) |
| `#EAEAEA` | `#2E2E2E` | 22 | `#1e1e1e` ×10, `#191919` ×5, `#1d1d1d` ×4, `colorContext.fg.primary` ×1, `control.colorFocus` ×1, `theme.fg.primary` ×1 | Not drawn in the frame; `#2E2E2E` is its darkest text (FG/1, the map) |
| `#191919` | `#ABABAA` | 16 | `#c4c4c3` ×6, `#9c9c9b` ×4, `#ececec` ×1, `#f7f7f7` ×1, `#b5b5b4` ×1, `theme.bg.primary` ×1, `#b8b8b7` ×1, `#c2c2c1` ×1 | Measured: the navigation column, `#191919` in 187:328, reads `#ABABAA` |
| `#969696` | `#4D4D4D` | 14 | `#2e2e2e` ×4, `#3d3d3d` ×3, `theme.fg.primary` ×2, `#222222` ×1, `#292929` ×1, `#333333` ×1, `theme.fg.secondary` ×1, `colorContext.fg.highlight` ×1 | Not drawn; the A1 grey of the frame is `#4D4D4D` (window buttons and the queue icon, `#727272` in 187:328) (the map) |
| `#0E0E0E` | `#C2C2C1` | 9 | `theme.bg.primary` ×6, `#9c9c9b` ×1, `#8f8f8e` ×1, `theme.fg.primary` ×1 | Not drawn (the window background is under the panels); B1 `#C2C2C1` as the panel (the map) |
| `#090909` | `#DEDEDE` | 9 | `#c4c4c3` ×5, `#fafafa` ×1, `root.colorContext.bg.secondary` ×1, `theme.bg.primary` ×1, `#9c9c9b` ×1 | Measured: the seek bar track, `#090909` in 187:328, reads `#DEDEDE` |
| `#1B1B1B` | `#ABABAA` | 9 | `#9c9c9b` ×2, `#b4b4b3` ×2, `#b8b8b7` ×2, `VLCStyle.palette.lightGrey200` ×1, `#e5e5e5` ×1, `root.colorContext.bg.primary` ×1 | Measured: the top bar and the control bar, `#1B1B1B` in 187:328, read `#ABABAA` |
| `#404040` | `#929291` | 9 | `#7c7c7b` ×5, `theme.fg.secondary` ×2, `#404040` ×1, `#9c9c9b` ×1 | Not drawn (disabled, placeholders); A4 `#929291` (the map) |
| `#151515` | `#B5B5B4` | 9 | `#b8b8b7` ×3, `#eeeeee` ×2, `#9c9c9b` ×2, `theme.bg.primary` ×1, `theme.bg.secondary` ×1 | Not drawn; A5 `#B5B5B4` (the map) |
| `#222222` | `#9C9C9B` | 7 | `#dcdcdc` ×1, `#e5e5e5` ×1, `#cccccc` ×1, `colorContext.bg.primary` ×1, `theme.bg.highlight` ×1, `theme.border` ×1, `#b8b8b7` ×1 | Not drawn (hover); A1 `#9C9C9B` (the map) |
| `#C6C6C6` | `#2E2E2E` | 5 | `#333333` ×3, `theme.fg.primary` ×2 | Not drawn in the frame; `#2E2E2E` is its darkest text (the map) |
| `#373737` | `#8E8E8D` | 3 | `#8f8f8e` ×2, `theme.border` ×1 | Not drawn; Divider/Bright `#8E8E8D` (the map) |
| `#0F0F0F` | `#C2C2C1` | 2 | `#ababaa` ×1, `#dedede` ×1 | Measured: the library panel, `#0F0F0F` in 187:328, reads `#C2C2C1` |

Not drawn in the frame (map kept, the Light style differs): `#969696` (Light/FG/A1 `#3C3C3C`), `#0E0E0E`
(Light/BG/B1 `#E7E7E7`), `#222222` (Light/BG/A1 `#EDEDED`, Light/Divider/Dim `#ACACAC`), `#404040` (Light/FG/A4
`#3B3B3B`), `#151515` (Light/BG/A5 `#888888`), `#373737` (Light/Divider/Bright `#9B9B9B`). Their light style keeps
the dark value: `#EAEAEA`, `#C6C6C6`. They keep the map's value, as decided for the opaque Light styles.

## Checked in the app (build `4.0.0-dev-40347-g6da8c7cd1a`, 2026-10-28)

The build with this change against the one before it (`e8db0b3b8f`, the same interface code without it), both
with the Figma demo library at 1920 × 1080 and the design's scale, in light and in dark: Home, the S01 folder, the
⋯ menu, a card's menu, the player, the player with the library panel, the music player, the 13 Settings pages,
Open network stream, Keyboard shortcuts, New playlist and a classic window (Media Information); then Customize
Player, the gear menu, the tracks and subtitles panel, the Effects panel and the queue. 33 captures per theme.

- **Dark:** no pixel differs by more than 1 of 255 in any capture (the rendering's own noise), the classic window
  identical: the dark theme is unchanged.
- **Light:** the same, except the classic window (the 600 × 50 px of its title bar, `#9C9C9B` → `#DEDEDE`).
- **Light against Figma *Player - Light***, the player with the library panel, mean colour per region (Figma → app):
  top bar `#ABABAA` → `#ABABAA`, navigation column `#ABAAAA` → `#ABABAA`, library panel `#B9B8B7` → `#B9B9B7`,
  panel header `#C2C2C1` → `#C2C2C1`, control bar `#AAAAA9` → `#ABABAA`, seek track `#DEDEDE` → `#DCDCDC`, seek
  fill `#D8BA9A` → `#DABB99`, video `#2D2B29` → `#2C2A28`: at most 2.4 per channel. Mean difference of the whole
  frame 1.93 of 255 (the rest: the mock list and title of the frame).
- Not captured: hover states (posted mouse moves give none), the onboarding, the cast and visualizer popups.
- No QML warning from the app's processes in either theme.

## Every call

Lines as of the change. *Light it gave*: the value the call passed and that was ignored; *Light now*: what it
passes now (the map's value) or *the map* where the dark value is chosen at run time and the light value was left
out. The dark theme is unchanged in every row.

| Place | Dark | Light it gave | Light now |
|---|---|---|---|
| `dialogs/dialogs/qml/Dialogs.qml:360` | `textPrimary` (`#EAEAEA`) | `"#191919"` | `#2E2E2E` |
| `dialogs/dialogs/qml/Dialogs.qml:369` | `"#191919"` (`#191919`) | `"#c4c4c3"` | `#ABABAA` |
| `dialogs/dialogs/qml/RedesignAboutDialog.qml:25` | `textPrimary` (`#EAEAEA`) | `"#1d1d1d"` | `#2E2E2E` |
| `dialogs/dialogs/qml/RedesignAboutDialog.qml:26` | `textSecondary` (`#969696`) | `"#3d3d3d"` | `#4D4D4D` |
| `dialogs/dialogs/qml/RedesignAboutDialog.qml:27` | `textMuted` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `dialogs/dialogs/qml/RedesignAboutDialog.qml:62` | `border` (`#373737`) | `"#8f8f8e"` | `#8E8E8D` |
| `dialogs/dialogs/qml/RedesignAboutDialog.qml:221` | `"#0E0E0E"` (`#0E0E0E`) | `"#9c9c9b"` | `#C2C2C1` |
| `dialogs/dialogs/qml/RedesignPluginsDialog.qml:204` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `dialogs/dialogs/qml/RedesignPluginsDialog.qml:227` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `dialogs/dialogs/qml/RedesignPluginsDialog.qml:289` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `dialogs/dialogs/qml/RedesignPluginsDialog.qml:293` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `dialogs/dialogs/qml/RedesignPluginsDialog.qml:304` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `dialogs/dialogs/qml/RedesignPluginsDialog.qml:313` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `dialogs/dialogs/qml/RedesignReportDialog.qml:71` | `"#191919"` (`#191919`) | `"#c4c4c3"` | `#ABABAA` |
| `dialogs/dialogs/qml/RedesignReportDialog.qml:98` | `textPrimary` (`#EAEAEA`) | `"#191919"` | `#2E2E2E` |
| `dialogs/dialogs/qml/RedesignReportDialog.qml:113` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `dialogs/firstrun/qml/SubtitleLanguagesStep.qml:82` | `"#090909"` (`#090909`) | `"#c4c4c3"` | `#DEDEDE` |
| `dialogs/firstrun/qml/SubtitleLanguagesStep.qml:83` | `"#191919"` (`#191919`) | `"#9c9c9b"` | `#ABABAA` |
| `dialogs/firstrun/qml/SubtitleLanguagesStep.qml:84` | `"#eaeaea"` (`#EAEAEA`) | `"#1e1e1e"` | `#2E2E2E` |
| `dialogs/firstrun/qml/SubtitleLanguagesStep.qml:102` | `"#090909"` (`#090909`) | `"#c4c4c3"` | `#DEDEDE` |
| `dialogs/firstrun/qml/SubtitleLanguagesStep.qml:103` | `"#191919"` (`#191919`) | `"#9c9c9b"` | `#ABABAA` |
| `dialogs/firstrun/qml/SubtitleLanguagesStep.qml:104` | `"#eaeaea"` (`#EAEAEA`) | `"#1e1e1e"` | `#2E2E2E` |
| `dialogs/firstrun/qml/WelcomeStep.qml:112` | `"#191919"` (`#191919`) | `"#c4c4c3"` | `#ABABAA` |
| `dialogs/firstrun/qml/WhatsNewDialog.qml:32` | `textPrimary` (`#EAEAEA`) | `"#1d1d1d"` | `#2E2E2E` |
| `dialogs/firstrun/qml/WhatsNewDialog.qml:33` | `textSecondary` (`#969696`) | `"#3d3d3d"` | `#4D4D4D` |
| `dialogs/firstrun/qml/WhatsNewDialog.qml:34` | `textMuted` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `dialogs/preferences/qml/SettingsAudioOutput.qml:56` | `"#8F8F8F"` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `dialogs/preferences/qml/SettingsAudioOutput.qml:143` | `"#8F8F8F"` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `dialogs/preferences/qml/SettingsEqualizer.qml:25` | `"#8F8F8F"` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `dialogs/preferences/qml/SettingsEqualizer.qml:44` | `"#8F8F8F"` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `dialogs/preferences/qml/SettingsEqualizer.qml:116` | `"#1b1b1b"` (`#1B1B1B`) | `VLCStyle.palette.lightGrey200` | `#ABABAA` |
| `dialogs/preferences/qml/SettingsEqualizer.qml:126` | `"#8F8F8F"` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `dialogs/preferences/qml/SettingsEqualizer.qml:188` | `"#8F8F8F"` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `dialogs/preferences/qml/SettingsEqualizer.qml:204` | `"#8F8F8F"` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `dialogs/preferences/qml/SettingsInlineEffects.qml:82` | `"#8F8F8F"` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `dialogs/preferences/qml/SettingsPrivacyActions.qml:45` | `"#8F8F8F"` (`#8F8F8F`) | `"#8F8F8F"` | `#2E2E2E` |
| `dialogs/preferences/qml/SettingsPrivacyActions.qml:56` | `"#404040"` (`#404040`) | `"#404040"` | `#929291` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:234` | `"#090909"` (`#090909`) | `"#fafafa"` | `#DEDEDE` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:313` | `"#191919"` (`#191919`) | `"#ececec"` | `#ABABAA` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:322` | `"#C6C6C6"` (`#C6C6C6`) | `"#333333"` | `#2E2E2E` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:347` | `"#222222"` (`#222222`) | `"#dcdcdc"` | `#9C9C9B` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:354` | `"#C6C6C6"` (`#C6C6C6`) | `"#333333"` | `#2E2E2E` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:355` | `"#6B6B6B"` (`#6B6B6B`) | `"#7c7c7b"` | `#6B6B6B` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:380` | `"#151515"` (`#151515`) | `"#eeeeee"` | `#B5B5B4` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:411` | `"#969696"` (`#969696`) | `"#222222"` | `#4D4D4D` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:424` | `textDisabled` (`#404040`) | `"#7c7c7b"` | `#929291` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:449` | `"#1B1B1B"` (`#1B1B1B`) | `"#e5e5e5"` | `#ABABAA` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:450` | `"#151515"` (`#151515`) | `"#eeeeee"` | `#B5B5B4` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:457` | `"#969696"` (`#969696`) | `"#292929"` | `#4D4D4D` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:491` | `"#8F8F8F"` (`#8F8F8F`) | `"#292929"` | `#2E2E2E` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:499` | `"#222222"` (`#222222`) | `"#e5e5e5"` | `#9C9C9B` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:536` | `"#191919"` (`#191919`) | `"#f7f7f7"` | `#ABABAA` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:538` | `"#222222"` (`#222222`) | `"#cccccc"` | `#9C9C9B` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:578` | `"#6B6B6B"` (`#6B6B6B`) | `"#555555"` | `#6B6B6B` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:588` | `"#6B6B6B"` (`#6B6B6B`) | `"#555555"` | `#6B6B6B` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:596` | `"#6B6B6B"` (`#6B6B6B`) | `"#555555"` | `#6B6B6B` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:658` | `"#969696"` (`#969696`) | `"#333333"` | `#4D4D4D` |
| `dialogs/preferences/qml/ShortcutsDialog.qml:680` | `"#C6C6C6"` (`#C6C6C6`) | `"#333333"` | `#2E2E2E` |
| `dialogs/toolbar/qml/ToolbarEditor.qml:64` | `"#0e0e0e"` (`#0E0E0E`) | `theme.bg.primary` | `#C2C2C1` |
| `dialogs/toolbar/qml/ToolbarEditor.qml:208` | `"#151515"` (`#151515`) | `theme.bg.primary` | `#B5B5B4` |
| `dialogs/toolbar/qml/ToolbarEditor.qml:210` | `"#373737"` (`#373737`) | `theme.border` | `#8E8E8D` |
| `dialogs/toolbar/qml/ToolbarEditor.qml:258` | `"#969696"` (`#969696`) | `theme.fg.secondary` | `#4D4D4D` |
| `dialogs/toolbar/qml/ToolbarEditorButtonList.qml:138` | `textMuted` (`#8F8F8F`) | `theme.fg.secondary` | `#2E2E2E` |
| `dialogs/toolbar/qml/ToolbarEditorButtonList.qml:226` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `dialogs/toolbar/qml/ToolbarEditorButtonList.qml:248` | `textMuted` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `dialogs/toolbar/qml/ToolbarEditorDialog.qml:126` | `"#1b1b1b"` (`#1B1B1B`) | `root.colorContext.bg.primary` | `#ABABAA` |
| `dialogs/toolbar/qml/ToolbarEditorDialog.qml:161` | `"#090909"` (`#090909`) | `root.colorContext.bg.secondary` | `#DEDEDE` |
| `dialogs/toolbar/qml/ToolbarEditorDialog.qml:192` | `textMuted` (`#8F8F8F`) | `root.colorContext.fg.primary` | `#2E2E2E` |
| `dialogs/toolbar/qml/ToolbarEditorDialog.qml:244` | `"#6B6B6B"` (`#6B6B6B`) | `root.colorContext.fg.secondary` | `#6B6B6B` |
| `dialogs/toolbar/qml/ToolbarEditorDialog.qml:304` | `textMuted` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `dialogs/toolbar/qml/ToolbarEditorDialog.qml:353` | `"#191919"` (`#191919`) | `"#b5b5b4"` | `#ABABAA` |
| `maininterface/qml/LocalTopbar.qml:130` | `hovered ¦¦ down ¦¦ mainMenu.opened ? controlHoverBackground : "transpa` | `colorContext.bg.primary` | the map (light value left out) |
| `maininterface/qml/LocalTopbar.qml:172` | `hovered ¦¦ down ¦¦ checked ? controlHoverBackground : "transparent"` | `colorContext.bg.primary` | the map (light value left out) |
| `maininterface/qml/LocalTopbar.qml:194` | `enabled ? textMuted : textDisabled` | `colorContext.fg.primary` | the map (light value left out) |
| `maininterface/qml/LocalTopbar.qml:195` | `hovered ¦¦ down ? controlHoverBackground : "transparent"` | `colorContext.bg.primary` | the map (light value left out) |
| `maininterface/qml/LocalTopbar.qml:240` | `textMuted` (`#8F8F8F`) | `theme.fg.primary` | `#2E2E2E` |
| `maininterface/qml/MainDisplay.qml:350` | `windowBackground` (`#0E0E0E`) | `theme.bg.primary` | `#C2C2C1` |
| `maininterface/qml/NoMedialibHome.qml:43` | `windowBackground` (`#0E0E0E`) | `theme.bg.primary` | `#C2C2C1` |
| `maininterface/qml/NoMedialibHome.qml:51` | `textPrimary` (`#EAEAEA`) | `colorContext.fg.primary` | `#2E2E2E` |
| `maininterface/qml/NoMedialibHome.qml:54` | `!control.enabled ? controlBackground : control.down ? accentDim : acce` | `control.colorContext.bg.primary` | the map (light value left out) |
| `maininterface/qml/NoMedialibHome.qml:57` | `textPrimary` (`#EAEAEA`) | `control.colorFocus` | `#2E2E2E` |
| `maininterface/qml/RedesignUpdateBanner.qml:44` | `border` (`#373737`) | `"#8f8f8e"` | `#8E8E8D` |
| `maininterface/qml/RedesignUpdateBanner.qml:84` | `textPrimary` (`#EAEAEA`) | `"#1d1d1d"` | `#2E2E2E` |
| `maininterface/qml/RedesignUpdateBanner.qml:122` | `textMuted` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `maininterface/qml/RedesignUpdateBanner.qml:136` | `textSecondary` (`#969696`) | `"#3d3d3d"` | `#4D4D4D` |
| `maininterface/qml/RedesignUpdateBanner.qml:167` | `"#0E0E0E"` (`#0E0E0E`) | `"#8f8f8e"` | `#C2C2C1` |
| `maininterface/qml/SideNavigationDelegate.qml:133` | `control.down ¦¦ control.hovered ? controlHoverBackground : "transparen` | `theme.bg.primary` | the map (light value left out) |
| `maininterface/qml/SideNavigationDelegate.qml:188` | `textMuted` (`#8F8F8F`) | `"#5a5a59"` | `#2E2E2E` |
| `maininterface/qml/SideNavigationDelegate.qml:218` | `!control.enabled ? textDisabled : control.checked ? accent : textMuted` | `(control.highlighted ¦¦ control.checked) ? theme.accent : theme.fg.primary` | the map (light value left out) |
| `maininterface/qml/SideNavigationDelegate.qml:251` | `!control.enabled ? textDisabled : control.checked ? "#C6C6C6" : textMu` | `control.checked ? theme.fg.secondary : theme.fg.primary` | the map (light value left out) |
| `maininterface/qml/SideNavigationPane.qml:123` | `windowBackground` (`#0E0E0E`) | `theme.bg.primary` | `#C2C2C1` |
| `maininterface/qml/SideNavigationPane.qml:128` | `sidebarBackground` (`#191919`) | `theme.bg.primary` | `#ABABAA` |
| `medialibrary/qml/FolderDisplay.qml:407` | `card.played ? "#6B6B6B" : textSecondary` | `card.played ? "#5a5a59" : "#4d4d4d"` | the map (light value left out) |
| `medialibrary/qml/FolderDisplay.qml:503` | `card.played ? "#6B6B6B" : textSecondary` | `card.played ? "#5a5a59" : "#4d4d4d"` | the map (light value left out) |
| `medialibrary/qml/FolderDisplay.qml:548` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `medialibrary/qml/MediaView.qml:195` | `textSecondary` (`#969696`) | `"#2e2e2e"` | `#4D4D4D` |
| `medialibrary/qml/MediaView.qml:198` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `medialibrary/qml/PlaylistCreateDialog.qml:154` | `"#191919"` (`#191919`) | `"#c4c4c3"` | `#ABABAA` |
| `medialibrary/qml/PlaylistCreateDialog.qml:275` | `textDisabled` (`#404040`) | `"#7c7c7b"` | `#929291` |
| `medialibrary/qml/PlaylistPropertiesDialog.qml:67` | `textMuted` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `medialibrary/qml/PlaylistPropertiesDialog.qml:82` | `textPrimary` (`#EAEAEA`) | `"#1d1d1d"` | `#2E2E2E` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:76` | `"#0F0F0F"` (`#0F0F0F`) | `"#ABABAA"` | `#C2C2C1` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:233` | `"#969696"` (`#969696`) | `"#2e2e2e"` | `#4D4D4D` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:267` | `"#1B1B1B"` (`#1B1B1B`) | `"#9c9c9b"` | `#ABABAA` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:268` | `"#151515"` (`#151515`) | `"#b8b8b7"` | `#B5B5B4` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:340` | `"#191919"` (`#191919`) | `"#b8b8b7"` | `#ABABAA` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:355` | `"#151515"` (`#151515`) | `"#b8b8b7"` | `#B5B5B4` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:393` | `"#969696"` (`#969696`) | `"#2e2e2e"` | `#4D4D4D` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:406` | `textDisabled` (`#404040`) | `"#7c7c7b"` | `#929291` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:462` | `"#090909"` (`#090909`) | `"#c4c4c3"` | `#DEDEDE` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:557` | `"#1B1B1B"` (`#1B1B1B`) | `"#9c9c9b"` | `#ABABAA` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:558` | `"#151515"` (`#151515`) | `"#b8b8b7"` | `#B5B5B4` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:818` | `textDisabled` (`#404040`) | `"#7c7c7b"` | `#929291` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:820` | `textPrimary` (`#EAEAEA`) | `"#191919"` | `#2E2E2E` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:834` | `"#191919"` (`#191919`) | `"#c4c4c3"` | `#ABABAA` |
| `medialibrary/qml/SubtitleAssociationsDialog.qml:890` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `medialibrary/qml/VideoGridItem.qml:58` | `watched ? "#6B6B6B" : textSecondary` | `colorContext.fg.primary` | the map (light value left out) |
| `medialibrary/qml/VideoGridItem.qml:62` | `textSecondary` (`#969696`) | `colorContext.fg.highlight` | `#4D4D4D` |
| `player/qml/AudioVisualizationDialog.qml:69` | `"#8F8F8F"` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `player/qml/AudioVisualizationDialog.qml:187` | `"#8F8F8F"` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `player/qml/CompactLibrary.qml:670` | `textMuted` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `player/qml/CompactLibrary.qml:852` | `"#6B6B6B"` (`#6B6B6B`) | `"#7c7c7b"` | `#6B6B6B` |
| `player/qml/CompactLibrary.qml:875` | `textMuted` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `player/qml/CompactLibrary.qml:1326` | `textMuted` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `player/qml/CompactLibrary.qml:1342` | `textMuted` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `player/qml/ControlBar.qml:245` | `textMuted` (`#8F8F8F`) | `theme.fg.primary` | `#2E2E2E` |
| `player/qml/ControlBar.qml:562` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/ControlRepeater.qml:188` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/HdrOutputDialog.qml:63` | `"#8F8F8F"` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `player/qml/HdrOutputDialog.qml:144` | `"#8F8F8F"` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `player/qml/HdrOutputDialog.qml:164` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `player/qml/Player.qml:1130` | `"#8F8F8F"` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `player/qml/Player.qml:1154` | `textPrimary` (`#EAEAEA`) | `"#1e1e1e"` | `#2E2E2E` |
| `player/qml/Player.qml:1178` | `"#8F8F8F"` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `player/qml/Player.qml:1278` | `textPrimary` (`#EAEAEA`) | `"#1e1e1e"` | `#2E2E2E` |
| `player/qml/PlayerControlLayout.qml:311` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/QueueCard.qml:117` | `textMuted` (`#8F8F8F`) | `"#5a5a59"` | `#2E2E2E` |
| `player/qml/QueueCard.qml:183` | `textMuted` (`#8F8F8F`) | `"#5a5a59"` | `#2E2E2E` |
| `player/qml/QueueCard.qml:221` | `textMuted` (`#8F8F8F`) | `"#5a5a59"` | `#2E2E2E` |
| `player/qml/QueueCard.qml:257` | `textMuted` (`#8F8F8F`) | `"#5a5a59"` | `#2E2E2E` |
| `player/qml/QueueCard.qml:266` | `textMuted` (`#8F8F8F`) | `"#5a5a59"` | `#2E2E2E` |
| `player/qml/QueueCard.qml:322` | `textMuted` (`#8F8F8F`) | `"#5a5a59"` | `#2E2E2E` |
| `player/qml/QueueCard.qml:328` | `textMuted` (`#8F8F8F`) | `"#5a5a59"` | `#2E2E2E` |
| `player/qml/SliderBar.qml:100` | `seekBackground` (`#090909`) | `theme.bg.primary` | `#DEDEDE` |
| `player/qml/SubtitleSyncTimeline.qml:177` | `"#8F8F8F"` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `player/qml/SubtitleTracksPanel.qml:275` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/SubtitleTracksPanel.qml:341` | `textPrimary` (`#EAEAEA`) | `"#1e1e1e"` | `#2E2E2E` |
| `player/qml/SubtitleTracksPanel.qml:349` | `"#090909"` (`#090909`) | `"#c4c4c3"` | `#DEDEDE` |
| `player/qml/SubtitleTracksPanel.qml:352` | `"#191919"` (`#191919`) | `"#9c9c9b"` | `#ABABAA` |
| `player/qml/SubtitleTracksPanel.qml:362` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/SubtitleTracksPanel.qml:462` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/SubtitleTracksPanel.qml:616` | `"#8F8F8F"` (`#8F8F8F`) | `"#7c7c7b"` | `#2E2E2E` |
| `player/qml/SubtitleTracksPanel.qml:702` | `"#1b1b1b"` (`#1B1B1B`) | `"#b4b4b3"` | `#ABABAA` |
| `player/qml/SubtitleTracksPanel.qml:703` | `"#151515"` (`#151515`) | `"#9c9c9b"` | `#B5B5B4` |
| `player/qml/SubtitleTracksPanel.qml:741` | `textPrimary` (`#EAEAEA`) | `"#1e1e1e"` | `#2E2E2E` |
| `player/qml/SubtitleTracksPanel.qml:742` | `textSecondary` (`#969696`) | `"#2e2e2e"` | `#4D4D4D` |
| `player/qml/SubtitleTracksPanel.qml:753` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/SubtitleTracksPanel.qml:763` | `"#8F8F8F"` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `player/qml/SubtitleTracksPanel.qml:782` | `"#8F8F8F"` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `player/qml/SubtitleTracksPanel.qml:791` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/Title.qml:52` | `textMuted` (`#8F8F8F`) | `theme.fg.primary` | `#2E2E2E` |
| `player/qml/TopBar.qml:345` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/TopBar.qml:385` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/TopBar.qml:406` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/TopBar.qml:431` | `hovered ¦¦ down ? controlHoverBackground : "transparent"` | `colorContext.bg.primary` | the map (light value left out) |
| `player/qml/TopBar.qml:455` | `textMuted` (`#8F8F8F`) | `theme.fg.primary` | `#2E2E2E` |
| `player/qml/TopBar.qml:471` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/TopBar.qml:515` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/WaveformSlider.qml:119` | `"#404040"` (`#404040`) | `"#9c9c9b"` | `#929291` |
| `player/qml/controlbarcontrols/CastButton.qml:101` | `textPrimary` (`#EAEAEA`) | `"#1e1e1e"` | `#2E2E2E` |
| `player/qml/controlbarcontrols/CastButton.qml:195` | `"#8F8F8F"` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `player/qml/controlbarcontrols/CastButton.qml:241` | `textPrimary` (`#EAEAEA`) | `"#1e1e1e"` | `#2E2E2E` |
| `player/qml/controlbarcontrols/CastButton.qml:255` | `"#8F8F8F"` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `player/qml/controlbarcontrols/CastButton.qml:318` | `"#1b1b1b"` (`#1B1B1B`) | `"#b4b4b3"` | `#ABABAA` |
| `player/qml/controlbarcontrols/CastButton.qml:319` | `"#151515"` (`#151515`) | `"#9c9c9b"` | `#B5B5B4` |
| `player/qml/controlbarcontrols/CastButton.qml:348` | `textPrimary` (`#EAEAEA`) | `"#1e1e1e"` | `#2E2E2E` |
| `player/qml/controlbarcontrols/CastButton.qml:357` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `player/qml/controlbarcontrols/MetadataText.qml:44` | `textMuted` (`#8F8F8F`) | `root.colorContext.fg.primary` | `#2E2E2E` |
| `player/qml/controlbarcontrols/PlayButton.qml:195` | `windowBackground` (`#0E0E0E`) | `theme.fg.primary` | `#C2C2C1` |
| `player/qml/controlbarcontrols/PlayButton.qml:257` | `"#6B6B6B"` (`#6B6B6B`) | `"#9c9c9b"` | `#6B6B6B` |
| `player/qml/controlbarcontrols/RedesignIconButton.qml:32` | `checked ? accent : textMuted` | `colorContext.fg.primary` | the map (light value left out) |
| `player/qml/controlbarcontrols/RedesignIconButton.qml:37` | `controlHoverBackground` (`#222222`) | `colorContext.bg.primary` | `#9C9C9B` |
| `player/qml/controlbarcontrols/VolumeWidget.qml:133` | `textMuted` (`#8F8F8F`) | `colorContext.fg.primary` | `#2E2E2E` |
| `player/qml/controlbarcontrols/VolumeWidget.qml:341` | `"#0F0F0F"` (`#0F0F0F`) | `"#dedede"` | `#C2C2C1` |
| `playlist/qml/PlaylistDelegate.qml:142` | `controlHoverBackground` (`#222222`) | `theme.bg.highlight` | `#9C9C9B` |
| `playlist/qml/PlaylistDelegate.qml:212` | `textMuted` (`#8F8F8F`) | `theme.fg.secondary` | `#2E2E2E` |
| `playlist/qml/PlaylistDelegate.qml:323` | `"#C6C6C6"` (`#C6C6C6`) | `theme.fg.primary` | `#2E2E2E` |
| `playlist/qml/PlaylistDelegate.qml:345` | `textMuted` (`#8F8F8F`) | `theme.fg.secondary` | `#2E2E2E` |
| `playlist/qml/PlaylistPane.qml:128` | `sidebarBackground` (`#191919`) | `"#c2c2c1"` | `#ABABAA` |
| `playlist/qml/PlaylistPane.qml:171` | `textPrimary` (`#EAEAEA`) | `theme.fg.primary` | `#2E2E2E` |
| `playlist/qml/PlaylistPane.qml:199` | `textMuted` (`#8F8F8F`) | `theme.fg.secondary` | `#2E2E2E` |
| `playlist/qml/PlaylistPane.qml:511` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `playlist/qml/PlaylistPane.qml:700` | `textMuted` (`#8F8F8F`) | `"#5a5a59"` | `#2E2E2E` |
| `widgets/qml/CSDWindowButtonSet.qml:132` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `widgets/qml/CSDWindowButtonSet.qml:180` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `widgets/qml/EmptyLabel.qml:100` | `"#C6C6C6"` (`#C6C6C6`) | `theme.fg.primary` | `#2E2E2E` |
| `widgets/qml/LanguagePicker.qml:31` | `"#090909"` (`#090909`) | `"#c4c4c3"` | `#DEDEDE` |
| `widgets/qml/LanguagePicker.qml:32` | `"#191919"` (`#191919`) | `"#9c9c9b"` | `#ABABAA` |
| `widgets/qml/LanguagePicker.qml:33` | `textPrimary` (`#EAEAEA`) | `"#1e1e1e"` | `#2E2E2E` |
| `widgets/qml/LanguagePicker.qml:34` | `"#6B6B6B"` (`#6B6B6B`) | `"#5a5a59"` | `#6B6B6B` |
| `widgets/qml/LibraryHeader.qml:195` | `windowBackground` (`#0E0E0E`) | `theme.bg.primary` | `#C2C2C1` |
| `widgets/qml/LibraryHeader.qml:204` | `separator` (`#222222`) | `theme.border` | `#9C9C9B` |
| `widgets/qml/LibraryHeader.qml:321` | `textDisabled` (`#404040`) | `theme.fg.secondary` | `#929291` |
| `widgets/qml/LibraryHeader.qml:357` | `crumb.current ? textSecondary : crumb.hovered ? textPrimary : textMute` | `theme.fg.primary` | the map (light value left out) |
| `widgets/qml/LibraryHeader.qml:395` | `textMuted` (`#8F8F8F`) | `theme.fg.primary` | `#2E2E2E` |
| `widgets/qml/LibraryHeader.qml:419` | `"#6B6B6B"` (`#6B6B6B`) | `theme.fg.secondary` | `#6B6B6B` |
| `widgets/qml/LibraryHeader.qml:527` | `"#151515"` (`#151515`) | `theme.bg.secondary` | `#B5B5B4` |
| `widgets/qml/LibraryHeader.qml:568` | `textDisabled` (`#404040`) | `theme.fg.secondary` | `#929291` |
| `widgets/qml/LibraryHeader.qml:570` | `textMuted` (`#8F8F8F`) | `theme.fg.primary` | `#2E2E2E` |
| `widgets/qml/PageExt.qml:219` | `windowBackground` (`#0E0E0E`) | `theme.bg.primary` | `#C2C2C1` |
| `widgets/qml/PageExt.qml:234` | `textSecondary` (`#969696`) | `theme.fg.primary` | `#4D4D4D` |
| `widgets/qml/RedesignButton.qml:37` | `textMuted` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `widgets/qml/RedesignButton.qml:49` | `playerControlBackground` (`#1B1B1B`) | `"#b8b8b7"` | `#ABABAA` |
| `widgets/qml/RedesignDialog.qml:160` | `"#090909"` (`#090909`) | `"#9c9c9b"` | `#DEDEDE` |
| `widgets/qml/RedesignDialog.qml:327` | `textMuted` (`#8F8F8F`) | `"#4d4d4d"` | `#2E2E2E` |
| `widgets/qml/RedesignDialog.qml:353` | `controlHoverBackground` (`#222222`) | `"#b8b8b7"` | `#9C9C9B` |
| `widgets/qml/RedesignDialog.qml:355` | `playerControlBackground` (`#1B1B1B`) | `"#b8b8b7"` | `#ABABAA` |
| `widgets/qml/RedesignDialogHeading.qml:65` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `widgets/qml/RedesignDialogHeading.qml:79` | `"#6B6B6B"` (`#6B6B6B`) | `"#4d4d4d"` | `#6B6B6B` |
| `widgets/qml/RedesignInfoRow.qml:69` | `textPrimary` (`#EAEAEA`) | `"#191919"` | `#2E2E2E` |
| `widgets/qml/RedesignMenuItem.qml:46` | `enabled ? textMuted : textDisabled` | `"#2e2e2e"` | the map (light value left out) |
| `widgets/qml/RedesignMenuItem.qml:72` | `"#8F8F8F"` (`#8F8F8F`) | `"#5A5A59"` | `#2E2E2E` |
| `widgets/qml/RedesignMenuItem.qml:127` | `enabled ? "#6B6B6B" : textDisabled` | `"#4d4d4d"` | the map (light value left out) |
| `widgets/qml/RedesignOptionItem.qml:67` | `root.enabled ? textMuted : textDisabled` | `"#4d4d4d"` | the map (light value left out) |
| `widgets/qml/RedesignTextArea.qml:49` | `textDisabled` (`#404040`) | `"#7c7c7b"` | `#929291` |
| `widgets/qml/RedesignTextArea.qml:51` | `textPrimary` (`#EAEAEA`) | `"#191919"` | `#2E2E2E` |
| `widgets/qml/RedesignTextArea.qml:84` | `"#191919"` (`#191919`) | `"#c4c4c3"` | `#ABABAA` |
| `widgets/qml/ViewHeader.qml:97` | `textSecondary` (`#969696`) | `theme.fg.primary` | `#4D4D4D` |
