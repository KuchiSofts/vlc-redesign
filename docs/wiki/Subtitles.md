# Subtitles

VLC Redesign shows subtitles in your language by itself: from the video, from a file next to it, or found online.
It learns the languages you pick, remembers the subtitle of each file and the timing of each series, and lets you
fix early or late subtitles while you watch.

> The look of the subtitles (font, size, colours, outline, shadow, box) and the languages are also in
> [Settings: Subtitles](Settings-Subtitles).

## The languages and tracks panel

**Languages and tracks** in the control bar opens a panel above the button with three tabs, *Video*, *Audio* and
**Subtitles**; it opens on Subtitles. The Subtitles tab has, from top to bottom:

1. what the online search found (or is doing), with its search button;
2. the **languages** on the left and the **subtitles** of the chosen language on the right;
3. **Subtitle timing**, the sync timeline;
4. **Position and size**.

A language or a subtitle you click shows at once. The panel stays open; a click outside it (or on its button) closes
it.

### The languages

The list on the left starts with **Off** (no subtitles), then:

| Place | Marker | Which languages |
|---|---|---|
| 1 | **Default** | Your default language (*Preferred language* in Settings). |
| 2 | **Second** | Your second language, used when there are none in the default one. |
| 3 | **Often used** | The languages you pick yourself, the most picked first. The marker shows once a language has been picked about twice. |
| 4 | | The other languages of the video's own subtitles. |
| 5 | | The other languages found online, by name. |
| last | | *Unknown language*: tracks that say none. |

Each language shows its name, in the language of the interface and as the language names itself, and how many
subtitles it has.

### The subtitles of a language

The list on the right holds the subtitles of the language chosen: the video's own first, then those found online.
Each says where it comes from:

| Label | Where it comes from |
|---|---|
| **In the video** | A track of the file itself. |
| **Next to the video** · *file* | A subtitle file VLC found next to the video, or in the folders of *Load subtitles from these folders*. |
| **Opened file** · *file* | A file you opened yourself. |
| **Downloaded from** *service* | A subtitle downloaded earlier, from the cache. |
| *service* · *downloads* | A subtitle found online, not downloaded yet: its service and how many times it was downloaded. |

Badges: **For this file** (made for this very file: the same fingerprint), **HI** (*For the hearing impaired*),
*Foreign parts only*.

Two buttons under the lists:

- **Load a subtitle file for playback**: opens a file for this playback.
- **Add subtitles to the library**: the subtitle window of the library, to attach a subtitle (or an audio track)
  for good: *From Computer*, *From Link* (a direct HTTPS link) or *From OpenSubtitles* (with your own API key,
  user name and password), *To This File*, *To This Folder* or *To This Playlist*. It needs a local file and the
  media library (*Library association needs a local media file and media library.*).

## Learning your languages

Each subtitle you pick by hand counts once per media for its language. Older picks fade (each new pick counts 1,
and every earlier one is worth a little less), so what you picked recently counts most.

- A language you pick moves up the list, under your default and second languages, and is marked *Often used*.
- A language you pick **about three times** becomes the **third automatic language**, after the default and the
  second one: VLC shows its track by itself in the next videos, and the online search looks for it.
- If you stop picking it, it fades and gives its place to the language you pick now.

Until you choose subtitle languages yourself, a video's own subtitle track in the language of Windows is shown
(not for English, or every English film would show its English subtitles).

## Subtitles found online

When a video opens with no subtitles in your languages, VLC Redesign looks for some online and loads the best one:
in your default language, else in the second one. This is *Find subtitles online when a video has none in these
languages* ([Settings: Subtitles](Settings-Subtitles)), asked by the installer and by the first-run setup. Off, the
panel searches only when you click its search button.

How it works:

1. **The video is identified**: its file name gives the title, the year, and the season and episode
   (`S01E02`, `1x02`, *Season 1 Episode 2*; the folder name too, *Show Name/Season 1/02.mkv*). IMDb's suggestions,
   else Wikidata, then Stremio's Cinemeta give its IMDb id. The panel shows *Identifying the video…*, then the title
   found (*Breaking Bad · S01E02*).
2. **The services are asked together** (*Searching OpenSubtitles, Wizdom…*). Their subtitles of every language are
   listed as they come, the ones made for this very file first (*3 found for this file*).
3. **A click loads one**: it is downloaded, unpacked (only its subtitles, 16 MB at most), converted to UTF-8, kept
   in the cache and remembered for the file (*Downloading…*, then *Downloaded*).

If the title found is wrong, *The video is another one: search by its title* opens a field, **Title of the movie or
series**, with the titles that match. The choice is remembered for the file.

The services (each can be turned off in [Settings: Subtitles](Settings-Subtitles)):

| Service | What it has |
|---|---|
| **OpenSubtitles** | All languages, movies and series; finds those made for your file. |
| **Wizdom** | Hebrew, movies and series. |
| **Addic7ed** | Series, many languages (through Gestdown). |
| **YIFY Subtitles** | Movies, many languages. |

What is sent: the video's title, its IMDb id, its file name and a fingerprint of the file (made from its first and
last 64 KB), and nothing else. The cache keeps the subtitles of the last 500 media, and drops subtitles not used
for 60 days.

## Subtitle timing (sync timeline)

Under the lists, **Subtitle timing** shows the delay (*+250 ms*) and a slider from **−10 s** to **+10 s** in steps
of 50 ms: earlier on the left, later on the right. Then:

- **Mark voice heard**: click it when you **hear** a line spoken (it reads *Voice marked*).
- **Mark text seen**: click it when you **see** that line's text (it reads *Text marked*).

Once both are marked, in either order, the subtitles move by the time between the two clicks. Click a marked button
again to unmark it. **Reset** puts the delay back to 0.

The keys do the same: <kbd>Shift</kbd>+<kbd>H</kbd> (the sound), <kbd>Shift</kbd>+<kbd>J</kbd> (the text),
<kbd>Shift</kbd>+<kbd>K</kbd> (apply), <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>K</kbd> (reset); <kbd>H</kbd> and
<kbd>G</kbd> move the subtitles later or earlier step by step. A delay for every video is *Subtitle delay* in
[Settings: Subtitles](Settings-Subtitles).

### Presets per series

**Series preset**, under the timeline, keeps the choices of one series for its other episodes:

- **Save** keeps the audio track, the subtitle track (or *Off*), the subtitle delay, the delay of the second
  subtitles and the subtitle speed of the episode playing. The panel reads *Applies to episodes of breaking bad*.
- The next episodes of the series apply it by themselves (*Series preset applied.*). If a track it names is not in
  an episode, it says so (*A saved audio or subtitle track is unavailable in this episode.*).
- **Remove** forgets it (*Series preset removed.*).

A series is the folder of the file plus the name before its `S01E02`; a file without such a name counts its folder
as the series. Presets need a local file (*Play a local episode to save its track and timing choices.*).

## Position and size

**Position and size**, at the end of the Subtitles tab:

| Row | − / + | Range | Reset |
|---|---|---|---|
| **Position** | *Move the subtitles down* / *up*, 1 % of the picture's height a click | −20 % (lower) to +60 % (higher); *Usual* at 0 | back to *Usual* |
| **Size** | *Smaller* / *Larger subtitles*, 10 % a click | 30 % to 300 %; 100 % as usual | back to 100 % |

The buttons repeat while held. The subtitles move and grow on the video as you click, and the panel fades while they
do, so you see them behind it. The change stays for the next videos and the next starts.

Keys: <kbd>Shift</kbd>+<kbd>Up</kbd> / <kbd>Shift</kbd>+<kbd>Down</kbd> move them (1 % a press, the same range),
<kbd>Ctrl</kbd>+wheel makes them larger or smaller and <kbd>Ctrl</kbd>+<kbd>0</kbd> resets the size. The same position is
*Vertical position* in [Settings: Subtitles](Settings-Subtitles).

While the *Continue where you left off?* card shows, the subtitles rise above it and the controls, and go back down
when it closes.

## Font and style

[Settings: Subtitles](Settings-Subtitles) › *Appearance*: the font (a list of the fonts installed), the size
(relative or in pixels), bold, the text colour and opacity, the outline, the shadow, a box behind the text, and the
vertical position. A change reaches the video playing from its next line.

## Remembered choices

- **Per file.** The subtitle you chose for a file (a track, a file or one found online) shows again the next time
  you play it, from the cache when it was downloaded. The title chosen for the online search is kept too. The last
  500 media are remembered.
- **Per series.** See [Presets per series](#presets-per-series).
- **The size and position** stay for every video (see above).

## Dual subtitles

With **Show both languages at once (dual subtitles)** ([Settings: Subtitles](Settings-Subtitles), once a second
language is chosen), the subtitles of the default language and those of the second one show together.
<kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>V</kbd> (*Toggle secondary subtitle control*) makes the subtitle keys (delay,
position) act on the second subtitles; the series preset keeps their delay too.

## Subtitle keys

| Key | Action |
|---|---|
| <kbd>V</kbd> / <kbd>Alt</kbd>+<kbd>V</kbd> | Next / previous subtitle track |
| <kbd>Shift</kbd>+<kbd>V</kbd> | Subtitles on or off |
| <kbd>H</kbd> / <kbd>G</kbd> | Subtitle delay up / down |
| <kbd>Shift</kbd>+<kbd>Up</kbd> / <kbd>Shift</kbd>+<kbd>Down</kbd> | Subtitles higher / lower |
| <kbd>Ctrl</kbd>+wheel, <kbd>Ctrl</kbd>+<kbd>0</kbd> | Subtitles larger or smaller, normal size |
| <kbd>Shift</kbd>+<kbd>H</kbd>, <kbd>Shift</kbd>+<kbd>J</kbd>, <kbd>Shift</kbd>+<kbd>K</kbd>, <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>K</kbd> | Sync: mark the sound, mark the text, apply, reset |
| <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>V</kbd> | Toggle secondary subtitle control |

All of them can be changed: see [Keyboard shortcuts](Keyboard-Shortcuts).

---

[The player](The-Player) · [Settings: Subtitles](Settings-Subtitles) · [Languages](Languages) · [Home](Home)
