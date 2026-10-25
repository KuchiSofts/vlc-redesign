# Settings: Playback

**⋯ → Settings → Playback** (<kbd>Ctrl</kbd>+<kbd>P</kbd>; the header reads *Playback Preferences*). Resuming,
starting and speed, preferred tracks, repeat and shuffle, seeking, discs and damaged files. Every row applies
without a restart: the order rows change the current play queue when the window closes, and the track rows apply to
the next media. See [how the Settings window works](Settings#how-the-window-works).

## Startup

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Continue where you left off** | Dropdown: *Never* / *Ask* / *Always*. Default: *Ask*. | What happens when a media you played before opens again. *Ask*: a question offers to continue; *Always*: it continues without asking; *Never*: it always starts from the beginning. **Remember the playback position** on [Privacy](Settings-Privacy#history) is the same setting as a toggle (off = *Never*). | `restore-playback-pos` (0 / 1 / 2) |
| **Start playing as soon as a file opens** | Toggle. Default: on. | Starts playing the play queue as soon as it is loaded. Off: media you open wait in the queue until you press Play. | `playlist-autostart` |
| **Default playback speed** | Slider 0.25× to 2×, in steps of 0.25 (each step labelled). Default: 1 (normal). | The speed media play at. It also changes the speed of what is playing now, which the player keeps for the next media. | `rate` |
| **Pause when another app uses audio** | Toggle. Default: on. | Pauses playback when Windows reports a call or another communication that uses the audio. | `playlist-cork` |

## Tracks

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Preferred video quality** | Dropdown: *Best available* / *Full HD (1080p)* / *HD (720p)* / *Standard (576p)* / *Low (360p)* / *Very low (240p)*. Default: *Best available*. | When a stream offers several video qualities (adaptive streams such as YouTube, HLS or DASH), VLC takes the one closest to this number of lines without going over it. Choose a lower one for a slow computer or connection. | `preferred-resolution` (−1, 1080, 720, 576, 360, 240) |
| **Preferred menu language** | Dropdown: *Auto*, then Arabic, Chinese, Czech, Danish, Dutch, English, Finnish, French, German, Greek, Hebrew, Hindi, Hungarian, Indonesian, Italian, Japanese, Korean, Malay, Norwegian, Persian, Polish, Portuguese, Romanian, Russian, Spanish, Swedish, Thai, Turkish, Ukrainian, Vietnamese. Default: *Auto*. | The language of DVD and Blu-ray menus. A value set elsewhere that is not in the list (a list of codes, for example) is kept and shown as is. | `menu-language` (ISO 639-2 codes such as `fre`) |
| **Default audio track** | Number field, counted from 1. Default: empty (a grey *1*: VLC chooses). | The number of the audio track to play when a media has several. Empty or 0 lets VLC choose, for example by **Preferred audio language** ([Audio](Settings-Audio)). | `audio-track` (counted from 0; −1 = VLC chooses) |
| **Default subtitle track** | Number field, counted from 1. Default: empty (a grey *1*: VLC chooses). | The number of the subtitle track to show when a media has several. Empty or 0 lets VLC choose, for example by the subtitle language ([Subtitles](Settings-Subtitles)). | `sub-track` (counted from 0; −1 = VLC chooses) |

## Order

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Repeat** | Dropdown: *Off* / *One* / *All*. Default: *Off*. | *One* plays the current item again and again; *All* starts the play queue again when it ends. It is the repeat mode VLC starts with, and the repeat button of the player changes it too. | `repeat` (One), `loop` (All) |
| **Shuffle** | Toggle. Default: off. | Plays the items of the play queue in random order. The shuffle button of the player changes it too. | `random` |
| **Stop after each item** | Toggle. Default: off. | Stops after each item of the play queue instead of going on with the next one. | `play-and-stop` |
| **When playback finishes** | Dropdown: *Stop* / *Quit VLC*. Default: *Stop*. | What VLC does when playback stops at the end of the play queue: stay open, or quit. | `play-and-exit` (*Quit VLC* = on) |

## Seeking

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Seek precisely** | Toggle. Default: on. | Seeks to the exact time asked. Off: VLC jumps to the nearest key frame, which is faster but less exact. | `input-fast-seek`, inverted |
| **Show thumbnail while scrubbing** | Toggle. Default: on. | Shows the frame at the pointer above the seek bar while you hover or drag it. | `qt-seek-thumbnail` |

## Files & Discs

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Default optical drive** | Dropdown: the CD/DVD drives of this computer (`D:`, `E:`…), plus the value already set if it is not one of them. Default: empty (shown as *-*). | The drive VLC uses when you open a disc without naming one. | `dvd` |
| **Repair damaged AVI files** | Dropdown: *Ask* / *Always* / *Never* / *When needed*. Default: *Ask*. | Whether VLC rebuilds the index of an AVI file whose index is broken or missing (a damaged, incomplete or unseekable file). *Ask* shows a question when it happens (*Do not play* / *Build index then play* / *Play as is*); *Always* rebuilds the index of every AVI file; *When needed* rebuilds it without asking when it is broken; *Never* plays the file as it is (seeking may then be approximate). The index is built in memory: the file itself is never changed, and a large file can take a while. | `avi-index` (0 / 1 / 2 / 3) |
| **Load matching .mkv files from the same folder** | Toggle. Default: on. | Opens the other Matroska files of the folder to find the linked segments of a video split across several files. VLC advises against it for broken files: turn it off if such files fail to open or open slowly. | `mkv-preload-local-dir` |

## Advanced Options

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Start paused** *(Developer mode)* | Toggle. Default: off. | Pauses each item of the play queue on its first frame. | `start-paused` |
| **Pause on the last frame of each item** *(Developer mode)* | Toggle. Default: off. | Pauses each item on its last frame instead of going on. | `play-and-pause` |
| **Resume the last playback states (tracks, speed...)** *(Developer mode)* | Toggle. Default: off. | When a media continues, VLC also restores its last states: the tracks chosen, the speed, the aspect ratio… | `restore-playback-states` |
| **Time-shift buffer folder** | Folder field with **Browse** (*Choose a time-shift buffer folder*). Default: empty, which uses the Windows temporary folder (`%TEMP%`). | Where VLC writes the temporary files that let you pause and rewind a live stream (time-shift). | `input-timeshift-path` |
| **Time-shift buffer size** | Dropdown: *Small* / *Medium* / *Large*, plus *N MiB* for another size already set. Default: *Medium*. | The maximum size of each temporary time-shift file: *Small* 10 MiB, *Medium* VLC's own size (50 MiB), *Large* 200 MiB. Also shown under the Advanced Options of [Video](Settings-Video#advanced-options). | `input-timeshift-granularity` (bytes; *Medium* = −1) |

## Good to know

- **Quit VLC also quits on Stop.** With **When playback finishes** on *Quit VLC*, VLC quits whenever playback
  stops, including when you press Stop. It also takes priority over **Stop after each item**.
- **No resume question?** **Continue where you left off** needs a saved position: see
  [Privacy](Settings-Privacy#good-to-know) (history and media library).
- **Seeking feels slow** on large or network files? Turn **Seek precisely** off: VLC then lands on the nearest key
  frame at once.
- **Wrong audio language on a film:** prefer **Preferred audio language** ([Audio](Settings-Audio)) to a fixed
  **Default audio track**: track numbers differ from file to file.

---

← [Controls](Settings-Controls) · [All settings](Settings) · [Video](Settings-Video) →
