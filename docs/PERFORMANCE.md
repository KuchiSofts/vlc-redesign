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

On the clean build 201 (no timing lines), against build 196, both tools agree: Settings freezes 95 ms instead of
129 (settime, median of 7–8 openings) and 124 ms instead of 150 (perfap, after browsing the library, 6 openings);
Keyboard Shortcuts 5–14 ms. (On build 200 one perfap series had shown no gain: its spread is 115–204 ms for one build.)

## Bugs found on the way

- **Settings went back at each start**: the player profile in use put its saved settings back over the ones in use;
  anything changed outside Settings (subtitle position and size, Shift+Up, the classic preferences, a VLC ended
  unclosed) was lost. Fixed; values now survive a clean quit, a killed VLC and another start.
- **A language chosen after another one** kept the first one's texts (gettext's cache): fixed.

## Left for later

- Settings still reads every option (20–25 ms) at each opening, the audio devices included (~3 ms).
- Customize (60–80 ms) and the first rows of the Tracks page (45–60 ms of layout) were not changed.

# Performance and memory pass 2: Live TV, soaks, CPU (2026-10-29)

Same PC (Windows 11, RTX 4090), test copies in &lt;the QA folder&gt; on the hidden desktop, posted input, sound to the
dummy output. **Before** is the merge of Live TV with only the timing lines added (`854803015b`); **after** is the
head of the branch. Three runs each unless said. The private playlist is a provider's (188 channels, 7 days of
archive, a 55 MB `.xml.gz` guide, 216 MB of XML); the public set is the catalog's *Pluto TV: all regions* and *Plex:
all regions* (5,891 channels, four guides). No address of the private playlist is kept in the logs or here.

## Tools

In the session scratchpad (`perf\`), new for this pass:

| Tool | What it measures |
|---|---|
| `tvbench.py <app> cold\|warm n` | Live TV start: the times its debug lines appear (channels, each guide, all guides), CPU, memory, the description files left; `cold` deletes the add-on's caches first |
| `tvzap.py <app> n` | Click on the banner's channel-up to the first picture of the new channel (`--tracer=json` render times, QueryPerformanceCounter); `SOAKCSV=` samples memory and handles at each switch |
| `guidebench.py`, `tvpage.py` | The guide grid and the TV page: open, fast scroll, Later/Earlier; WM_NULL ping every 20 ms (longest freeze of the interface thread) and the CPU of each thread |
| `openbench.py` | "Play this file" (rc) to the first picture or the first audio decoded |
| `startbench.py` | Start to Home drawn |
| `idlecpu.py` | CPU, GPU (3D engine counter), memory, handles, GDI/USER objects, threads at rest, playing, paused, minimized |
| `uisoak.py pages\|dialogs\|theme n` | Page switches, the dialogs of the ⋯ menu opened and closed (checked against a capture of Home), Light/Dark switches; sampled every 10 s |
| `soakmon.py`, `threadcpu.py` | A process sampled every 10 s; the CPU of each thread, by name |

## Live TV start

| Measure | Before | After |
|---|---|---|
| Private, warm: channels listed | 0.5–0.6 s | 0.5–0.6 s |
| Private, warm: guide ready | 3.9–4.0 s (XML read in 3.6–3.7 s) | **0.5–0.6 s** (its index read in 19–22 ms) |
| Private, warm: CPU of the start | 4.7–5.1 s | **1.1–1.2 s** |
| Description files of the guide on disk | one more (13 MB) at every start | one |
| Private, cold (no cache): guide ready | 4.5–5.2 s | 4.7–5.3 s (one reading is needed; the index is written then) |
| Public, warm: guide ready | 3.8–3.9 s | **0.5–0.6 s** |
| Public, warm: CPU, memory | 4.6–5.0 s, 317–331 MB | 1.5–1.8 s, 305–310 MB |
| Public, warm: description files | four more at every start | four |
| Public, cold: guides read, all ready | 6 readings, 10.4–12.0 s, CPU 7.5–7.9 s | 4 readings, 8.8–9.3 s (13.6 on a slow run), CPU 5.9–6.5 s |
| Memory 4 s after a start (private) | 218–259 MB (still reading the guide) | 206–210 MB |
| TV page at rest after a start | 1.38 s of CPU in 10 s (the guide read again) | 0.02 s |

What changed: the guide is read once into an index next to its file (its channels, languages and days; the
descriptions stay in their own file), and read from it at the next starts as long as the file, the channels, the
languages and the days are the same; the other description files are deleted. The sources due are refreshed
together (every playlist, then each guide once); each source read every guide again before. A logo that could not
be had is asked again after an hour, not at every row shown. The lists keep their rows when the guide comes.

## Zapping

20 switches from the same channel, click to the first picture: **before 920 ms** median (805–1,444), **after 953 ms**
(436–1,460): the same, as nothing on that path was changed. Where the time goes (clock traces): the first data of the
new channel reaches the demuxer 850–950 ms after the click, its picture 50–100 ms later; the rest is the
provider's playlist and first segment over the network. `--network-caching=500` changed nothing (954 ms): the
streams have one variant each (no start bitrate to choose). Not done: opening the next channels ahead of time, which
means a second stream at once (providers often allow one connection) and twice the bandwidth.

## The guide grid

| Measure (3 runs) | Before | After |
|---|---|---|
| Later ×8, Earlier ×8 | **crashed, 3 of 3 runs** | no crash; 3.3–3.6 s CPU, longest freeze 66–76 ms |
| (the same, with the batched requests only) | 4.8–5.9 s CPU, longest freeze 213–247 ms | |
| Fast scroll down and up (60 + 60 notches) | 8.8–9.0 s CPU, render thread 4.7–4.9 s | 7.0–8.1 s CPU, render thread 3.4–3.9 s |
| Opening the guide | 0.39–0.58 s CPU | 0.03–0.30 s CPU |
| Guide open, at rest | | 0.02–0.05 s of CPU in 10 s |

The crash: the function given each answer of the add-on held the guide's JavaScript values and was copied and
freed on the add-on's worker thread; the JavaScript engine allows no other thread to touch them (the crash was in
`QV4::PersistentValueStorage::allocate`). The functions now stay on the interface's thread. The rows ask for
their programmes in one request, keep their cells when the time moves (each six hours crossed made every cell
again) and have no clip of their own (a clip each kept the cells from being drawn together).

## Library, opening media, start

| Measure | Result |
|---|---|
| Start to Home drawn (4 starts each, alternated) | before 0.79–1.07 s, after 0.74–0.87 s: the same within the noise |
| Artists, Albums, Tracks, Genres, Videos (3,000 songs, 400 episodes): opening | longest freeze 25–42 ms |
| The same, fast scroll | longest freeze 4–14 ms; memory 216 → 229 MB |
| Play a film (rc) to its first picture | 1080p 210–230 ms, 4K H.264 200–208 ms (the first opening 300–350 ms) |
| Play a song (rc) to its first audio decoded | 4–19 ms |
| TV page with 5,891 channels: opening | longest freeze 22–28 ms (before and after) |

## CPU

| State | Before | After |
|---|---|---|
| Home, at rest | 0.15 % of a core | 0.23 % |
| Minimized, at rest | 0.00 % | 0.08 % |
| Playing 1080p (D3D11 decoding) | 7.8 % (GPU 3D 0.3 %) | 6.8 % (0.3 %) |
| Paused | 0.91 % | 0.99 % |
| Playing 1080p, minimized | 5.5 % | 3.0 % |
| Playing 4K H.264 | 5.6 % (GPU 0.6 %) | 3.4 % (0.8 %) |
| Stopped | 0.00 % | 0.00 % |
| A TV channel playing, banner hidden | | 7.5 % (driver 1.9, video output 1.3, decoders 2.3, HLS 0.8, interface 0.6) |
| Guide open, at rest | | 0.2–0.5 % |

Nothing is drawn while nothing changes: at rest, no render thread time; the timers of the interface are all single
shots. Paused, the video output draws the last picture again every 80 ms (upstream VLC 4's `VOUT_REDISPLAY_DELAY`,
marked FIXME, for subtitles and OSD drawn while paused) and the graphics driver follows: ~0.7 % of a core. Left as
it is: changing it risks the pictures of a paused seek or a subtitle changed while paused.

## Soaks (memory, handles, GDI/USER objects, threads every 10 s)

| Soak | Private memory | Handles, GDI, USER, threads | Verdict |
|---|---|---|---|
| 1 h of 1080p playback | 300 MB at the start, 297–308 in the middle, 298–300 at the end | 1,453–1,481, 35, 53–56, 231–235 | flat |
| 300 channel switches (211 in one run) | 323–367 MB, no trend | 2,739–2,768, 36, 73–76, 250–252 | flat |
| 8 dialogs of the ⋯ menu ×100 (800 openings, each checked closed) | 269 → 273 → 275 → 275 MB every 20 rounds, 272 MB at rest | 1,300–1,313, 68, 89–90, 136–137 | levels off (caches) |
| 203 page switches | 215–219 MB, 210 MB at rest | 1,146–1,162, 66, 90–91, 133–138 | flat |
| 40 theme switches (Light/Dark) | 224–231 MB, 226 MB at rest | 1,397–1,411, 66, 156–158, 167–171 | flat after a first rise |

## Bugs found on the way

- **The guide crashed VLC** when moved through the hours (above): fixed.
- **Every start left a 13 MB file of descriptions per guide** in the add-on's cache (13 of them, 174 MB, in a
  profile used for a day): the stale ones are deleted now.
- **Quitting crashed** with the first version of that cleanup: Qt's name filters of `QDir::entryList` match with a
  regular expression, whose stack belongs to the thread and was freed after the add-on's module was unloaded. The
  files are matched by hand (fixed before anything was published).
- Not fixed: while a channel plays, a Page Up posted to the window restarted the stream instead of switching the
  channel. The player hands keys to VLC's hotkeys first (`preferHotkeys`), and Page Up is one of them; the TV keys of
  the banner (Page Up/Down, digits, Backspace) may never reach it from a keyboard. Not checked with a real keyboard.

## Left for later

- Fast scrolling of the guide still keeps the render thread busy (3.4–3.9 s for 6 s of scrolling): every row is a
  few dozen items; lighter cells would help.
- A guide is read at ~60 MB/s of XML (QXmlStreamReader) the first time and after each download.
- The paused redisplay every 80 ms (above).
