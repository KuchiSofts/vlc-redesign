# Settings: Playback

**⋯ → Settings → Playback** (or <kbd>Ctrl</kbd>+<kbd>P</kbd>). Resuming, playback speed, repeat and shuffle, what happens at the end, seeking and discs.

Rows under **Advanced Options** are in the collapsed bar at the bottom of the page. Rows marked *Developer mode only* appear there once **Developer mode** is on ([General](Settings-General)). See [how the Settings window works](Settings).

### Startup
- **Continue where you left off**: dropdown: Never / Ask / Always. Default: Ask. Whether a media you played before starts again where it was left off. Ask shows a question when it starts; Always resumes without asking; Never always starts from the beginning. **Remember the playback position** on the [Privacy](Settings-Privacy) page is the same setting as a toggle (off = Never). <sub>VLC option: `restore-playback-pos`</sub>
- **Start playing as soon as a file opens**: toggle. Default: on. Starts playing the play queue as soon as it is loaded. Off, media you open wait in the play queue until you press Play. <sub>VLC option: `playlist-autostart`</sub>
- **Default playback speed**: slider from 0.25× to 2× in steps of 0.25 (every step labelled). Default: 1 (normal speed). The speed media play at. It also changes the speed of what is playing now, which the player keeps for the next media. <sub>VLC option: `rate`</sub>
- **Pause when another app uses audio**: toggle. Default: on. Pauses playback automatically when a call or another audio communication starts, as the system reports it. <sub>VLC option: `playlist-cork`</sub>

### Tracks
- **Preferred video quality**: dropdown: Best available / Full HD (1080p) / HD (720p) / Standard (576p) / Low (360p) / Very low (240p). Default: Best available. When a stream offers several video qualities (adaptive streams, for example), VLC takes the one closest to this number of lines without going over it. Pick a lower one if your computer or your connection is too slow for the higher ones. <sub>VLC option: `preferred-resolution`</sub>
- **Preferred menu language**: dropdown: Auto, then Arabic, Chinese, Czech, Danish, Dutch, English, Finnish, French, German, Greek, Hebrew, Hindi, Hungarian, Indonesian, Italian, Japanese, Korean, Malay, Norwegian, Persian, Polish, Portuguese, Romanian, Russian, Spanish, Swedish, Thai, Turkish, Ukrainian, Vietnamese (plus a value set elsewhere, if it is not one of them). Default: Auto. The language of the menus of DVDs and Blu-ray discs. <sub>VLC option: `menu-language` (ISO 639-2 codes, for example `fre`)</sub>
- **Default audio track**: number field, counted from 1. Default: empty (a grey "1": VLC chooses). The number of the audio track to play when a media has several. Empty or 0 lets VLC choose, for example by **Preferred audio language** ([Audio](Settings-Audio)). <sub>VLC option: `audio-track` (counted from 0; −1 = VLC chooses)</sub>
- **Default subtitle track**: number field, counted from 1. Default: empty (VLC chooses). The number of the subtitle track to show when a media has several. Empty or 0 lets VLC choose, for example by the subtitle language ([Subtitles](Settings-Subtitles)). <sub>VLC option: `sub-track` (counted from 0; −1 = VLC chooses)</sub>

### Order
- **Repeat**: dropdown: Off / One / All. Default: Off. One plays the current item again and again; All starts the play queue again when it ends. It is the repeat VLC starts with; the repeat button of the player changes it too. <sub>VLC options: `repeat` (One) and `loop` (All)</sub>
- **Shuffle**: toggle. Default: off. Plays the items of the play queue in random order. <sub>VLC option: `random`</sub>
- **Stop after each item**: toggle. Default: off. Stops playback after each item of the play queue instead of going on with the next one. <sub>VLC option: `play-and-stop`</sub>
- **When playback finishes**: dropdown: Stop / Quit VLC. Default: Stop. What VLC does when there is nothing more in the play queue: stay open, or quit. <sub>VLC option: `play-and-exit` (Quit VLC = on)</sub>

### Seeking
- **Seek precisely**: toggle. Default: on. Seeks to the exact time asked. Off, VLC jumps to the nearest key frame instead, which is faster but less exact. <sub>VLC option: `input-fast-seek` (inverted: Seek precisely on = `input-fast-seek` off)</sub>
- **Show thumbnail while scrubbing**: toggle. Default: on. Shows the frame at the hovered position above the seek bar. <sub>VLC option: `qt-seek-thumbnail`</sub>

### Files & Discs
- **Default optical drive**: dropdown of the CD/DVD drives of the computer (`D:`, `E:`…), and the value already set if it is not one of them. Default: empty (shown as "-"). The drive (or file) VLC uses when you open a disc without naming one. <sub>VLC option: `dvd`</sub>
- **Repair damaged AVI files**: dropdown: Ask / Always / Never / When needed. Default: Ask. Whether VLC builds a new index for an AVI file whose index is broken or missing (a file that is damaged, incomplete or can't be seeked). Ask shows a question when it happens. <sub>VLC option: `avi-index`</sub>
- **Load matching .mkv files from the same folder**: toggle. Default: on. Opens the other Matroska files of the same folder to find the linked segments of a video split into several files. VLC advises against it for broken files: turn it off if such files fail to open. <sub>VLC option: `mkv-preload-local-dir`</sub>

### Advanced Options
- **Start paused**: toggle. Default: off. Pauses each item of the play queue on its first frame. *(Developer mode only.)* <sub>VLC option: `start-paused`</sub>
- **Pause on the last frame of each item**: toggle. Default: off. Pauses each item of the play queue on its last frame instead of going on. *(Developer mode only.)* <sub>VLC option: `play-and-pause`</sub>
- **Resume the last playback states (tracks, speed...)**: toggle. Default: off. When a media continues, VLC also takes back its last states, such as the tracks chosen, the speed and the aspect ratio. *(Developer mode only.)* <sub>VLC option: `restore-playback-states`</sub>
- **Time-shift buffer folder**: folder field with a **Browse** button ("Choose a time-shift buffer folder"). Default: empty, which uses the temporary folder of Windows. Where VLC writes the temporary files that let you pause and rewind a live stream (time-shift). <sub>VLC option: `input-timeshift-path`</sub>
- **Time-shift buffer size**: dropdown: Small (10 MiB) / Medium / Large (200 MiB), and the size already set if it is another one. Default: Medium (VLC's own size, 50 MiB). The maximum size of each temporary time-shift file. *(Also shown under the Advanced Options of [Video](Settings-Video).)* <sub>VLC option: `input-timeshift-granularity` (in bytes; Medium = −1)</sub>

### Options of VLC behind these settings

| Row | VLC option |
|---|---|
| Continue where you left off | `restore-playback-pos` |
| Start playing as soon as a file opens | `playlist-autostart` |
| Default playback speed | `rate` |
| Pause when another app uses audio | `playlist-cork` |
| Preferred video quality | `preferred-resolution` |
| Preferred menu language | `menu-language` |
| Default audio track | `audio-track` |
| Default subtitle track | `sub-track` |
| Repeat | `repeat`, `loop` |
| Shuffle | `random` |
| Stop after each item | `play-and-stop` |
| When playback finishes | `play-and-exit` |
| Seek precisely | `input-fast-seek` (inverted) |
| Show thumbnail while scrubbing | `qt-seek-thumbnail` |
| Default optical drive | `dvd` |
| Repair damaged AVI files | `avi-index` |
| Load matching .mkv files from the same folder | `mkv-preload-local-dir` |
| Start paused | `start-paused` |
| Pause on the last frame of each item | `play-and-pause` |
| Resume the last playback states (tracks, speed...) | `restore-playback-states` |
| Time-shift buffer folder | `input-timeshift-path` |
| Time-shift buffer size | `input-timeshift-granularity` |

---

← [Controls](Settings-Controls) · [All settings](Settings) · [Video](Settings-Video) →
