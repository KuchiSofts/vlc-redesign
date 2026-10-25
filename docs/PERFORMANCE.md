# Performance pass (2026-10-25)

Measured on the developer PC (Windows 11, NVIDIA GPU, Hyper-V VMs running alongside), with the test copies on
Windows' hidden desktop and posted input. Every change was measured against the build before it with the same
tools, alternating builds back to back with nothing else running. A change is kept only if the numbers improve.

## Tools

All are in the session scratchpad (`perfwork\`); the new ones were added for this pass.

| Tool | What it measures |
|---|---|
| `measure_all.sh <tag>` | The whole suite below, into `<tag>\` |
| `starts.py n [app]` | Start: window shown, first frame drawn, settled picture (n starts) |
| `settime.py n` | Settings opened n times from the main menu: longest freeze (WM_NULL ping every 10 ms), time to a settled picture, memory open and closed |
| `dlgopen.py` | Longest freeze opening Keyboard Shortcuts, About, Customize |
| `perfap.py` | Library pages, menus, Settings, memory, CPU playing and paused |
| `bigperf.py`, `scrolltime.py` | The 3,000-song library: page opens, fast scroll, Qt render-loop frame times |
| **`settprobe.py`** (new) | With a probe build: the `PROBE` timing lines of the app during each opening, with the freeze |
| **`probe_patch.py`** (new) | Puts the timing lines in the build clone only (never committed): the steps of the openings, the reading of the options (each option over 2 ms named), and a benchmark that makes each part of Settings alone 4 times (`probe/settings-bench=true` in the ini) |

## Baseline (build 195) and what was found

| Measure | Build 195 | Real cost? |
|---|---|---|
| Start (warm, median) | window 391 ms, drawn 521, settled 696 | normal (same as builds 193 and 196 in A/B) |
| Main menu, first opening | 3 ms freeze | fixed earlier |
| Settings, each opening | 118–190 ms freeze | **yes** |
| Keyboard Shortcuts, each opening | 87–150 ms freeze | **yes** |
| Customize | 57–85 ms freeze | moderate |
| Tracks page (3,000 songs) opening | 45–61 ms of layout | moderate (a page opening) |
| Artists, fast scroll | one 290 ms "gap" | **no**: the end of the list, nothing to draw; the GUI thread never waited over 6 ms |
| Playing / paused | 1.4–2.5 % / 0.6–0.9 % of a core | fine (one 5.9 % reading was noise) |

Where the time went (probe builds):

- **Keyboard Shortcuts**: its model read the options twice (as it was made, then as the dialog opened), and its sort
  translated the names of the groups at each comparison: thousands of gettext lookups, 41–50 ms of each reading.
- **Settings**: its model read every option twice too (20 ms each), and each of its four option lists made its own
  table of labels (`SettingsPresentation`, hundreds of texts, ~10 ms each); the lists hidden on a page made a row
  for each option all the same (~9 ms each). The whole dialog took ~93 ms to make.

## Changes and results

| Change | Before | After | Kept |
|---|---|---|---|
| The two dialogs read their options once as they open (`refresh()`) | Shortcuts 142 ms, Settings 155 ms (b196) | 66 ms, 134 ms (b197) | yes |
| Shortcuts ranks its rows once instead of in every comparison | 67 ms (b197) | **12–20 ms** (b198, b200) | yes |
| Settings rows only for visible lists, by their visibility | 138 ms (b197) | 133 ms (b198): no measurable change | **reverted** |
| One table of labels for the four lists of Settings | dialog made in ~93 ms | ~65 ms (benchmark, b200) | yes |
| No rows for the lists of another page or closed Advanced Options (explicit condition) | | (with the line above) | yes |
| Settings, together (settime, 8 openings) | freeze 112 ms, settled 147 ms, 227 MB (b199) | **92 ms, 122 ms, 224 MB** (b200) | |

From the first measurement to the last (medians):

| Measure | Before (b196) | After (b200) | Change |
|---|---|---|---|
| Keyboard Shortcuts, longest freeze | 108–142 ms | 12 ms | **−90 %** |
| Settings, longest freeze (settime) | 155 ms | 92 ms | **−41 %** |
| Settings, settled picture | 189 ms | 122 ms | −35 % |
| Settings, memory while open | 226–227 MB | 224 MB | −3 MB |
| Start (window shown) | 436 ms | 442 ms | same (noise ±30 ms) |
| Customize, About, library pages, playback CPU | | | unchanged |

`perfap.py`, which opens Settings after browsing the library three times, spreads widely (115–204 ms for the same
build) and did not show the Settings gain (b196 median 132 ms, b200 164 ms over 9 openings each). The probe of
build 200 in the same conditions as settime shows the dialog made in 41–56 ms against ~78 before, with the reading
of the options unchanged (20–25 ms, the new font list included): the settime and benchmark results are the ones
used, and perfap's Settings figure is to be watched in the next runs.

## Bugs found on the way

- **Settings went back at each start**: the player profile in use put its saved settings back over the ones in use;
  anything changed outside Settings (subtitle position and size, Shift+Up, the classic preferences, a VLC ended
  unclosed) was lost. Fixed; values now survive a clean quit, a killed VLC and another start.
- **A language chosen after another one** kept the first one's texts (gettext's cache): fixed.

## Left for later

- Settings still reads every option (20–25 ms) at each opening, the audio devices included (~3 ms).
- Customize (60–80 ms) and the first rows of the Tracks page (45–60 ms of layout) were not changed.
