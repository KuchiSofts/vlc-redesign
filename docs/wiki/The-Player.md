# The player

The player opens when a video or a song plays. The video sits in a frame with rounded corners between the top bar
and the control bar; in fullscreen it fills the screen and the bars float over it. Songs open the music view: the
cover over a blurred copy of itself, the title and the artist, and a waveform seek bar.

> The buttons of the control bar change with the profile in use (**⋯ → Profile**) and can be moved, added and taken
> out: see [Customize the player](Customize-the-Player). This page describes every button the bar can hold.

## The top bar

| Part | What it does |
|---|---|
| **Library** (left) | Shows or hides the library next to the video (see [The library next to the video](#the-library-next-to-the-video)). <kbd>L</kbd> does the same. |
| Title (middle) | The name of what plays. Its tooltip, *Show … in the library*, opens it in the library. |
| **⋯** (right) | The main menu: open files, folders, discs, network streams and capture devices, *Convert*, *Open Recent*, *Profile*, *Customize Player*, *Keyboard Shortcuts*, *Settings* and *About VLC*. |
| Window buttons | Minimize, maximize (or restore) and close, when *Show window decorations* is off ([Settings: Interface](Settings-Interface)). |

## The control bar

The default bar (profile *Default*) has Previous, Play/Pause and Next on the left, and the volume, *Effects*,
*Languages and tracks*, *VLC Settings*, *Play Queue* and *Fullscreen* on the right. The music view has its own bar
(Previous, Back 15 s, Play/Pause, Forward 15 s, Next; then the volume, *Effects*, *Play Queue* and
*Fullscreen*). The bar has three zones, *left*, *center* and *right*; the seek bar sits between the zones, or on a
row of its own above them.

When a window is too narrow for every button, buttons are hidden from the inside of the bar outwards, the same way
every time: *Fullscreen* and *Play Queue* stay the longest.

### Every button and widget

Names are those of the *Customize Player* palette. The column *Palette* says whether the button can be dragged in
from the palette: the items of the design, then the bar's own controls, so a button taken out can be added back
(see [Customize the player](Customize-the-Player#the-bars-own-controls)). The others are VLC's own controls.

| Button | Palette | What it does |
|---|---|---|
| **Play** | yes | Play / pause. |
| **Previous**, **Next** | yes | The previous or next media of the play queue. |
| **Backward 15s**, **Forward 15s** | yes | Jump 15 seconds back or forward. |
| **Backward 10s**, **Forward 10s** | yes | Jump 10 seconds back or forward. |
| **Replay** | yes | Back to the start of the media. |
| **Stop** | yes | Stops playback. |
| **Slower**, **Faster** | no | Lower or raise the playback speed one step. |
| **Playback Speed** | yes | Shows the speed (*1x*); a click opens the speed popup (see [Speed](#speed)). The wheel over it changes the speed. |
| **Previous Frame**, **Next Frame** | yes | Step one picture back or forward (pauses). |
| **A-B Loop** | yes | Loops a part of the media (see [A-B loop](#a-b-loop)). |
| **Repeat** | yes | Cycles Off, Repeat all, Repeat one; lit while on, its tooltip says *Repeat one* in that mode. |
| **Shuffle** | yes | Plays the queue in random order; lit while on. |
| **Record** | yes | Records what plays to a file; the button is in the accent colour while recording. Where recordings go: [Settings: Recording](Settings-Recording). |
| **Snapshot** | yes | Saves the current picture. Where: [Settings: Video](Settings-Video). |
| **Picture in Picture** | yes | Goes back to the library with the video in a small player over it (see [Picture in picture](#picture-in-picture)). |
| **HDR** | yes | Opens the HDR output panel (see [HDR](#hdr)). |
| **Zoom** | yes | A list of zoom levels for the video. Dimmed without a video (*Zoom is unavailable without video*). |
| **Aspect Ratio** | no | A list of aspect ratios for the video. |
| **Right Sidebar** | yes | Shows or hides the queue card, as *Play Queue* does. |
| **Play Queue** | yes | Shows or hides the queue card (<kbd>Q</kbd>, <kbd>Ctrl</kbd>+<kbd>L</kbd>). |
| **Languages and tracks** | yes | Opens the languages and tracks panel: subtitles, audio and video tracks (see [Subtitles](Subtitles)). |
| **Effects** | yes | Opens the effects panel: the audio and video effects (see [Effects](Effects)). |
| **VLC Settings** | yes | Opens the menu of the player (see [The VLC Settings menu](#the-vlc-settings-menu)). |
| **Fullscreen** | yes | Enters or leaves fullscreen. |
| **Quit VLC** | yes | Quits. |
| **Go Home** | yes | Opens Home in the library. |
| **Playlists** | yes | Opens the Playlists page of the library. |
| **Thumbnail** | yes | The artwork of the media. |
| **File Name** | yes | The title of the media, as text in the bar. |
| **Artist Name** | yes | The artist of the song (*Artist unavailable* when there is none). |
| **Folder Name** | yes | The folder the media is in. |
| **Playlist Name** | yes | The play queue and the place of the media in it, *Play Queue (3/10)*. |
| **Properties** | yes | Opens the media information window (<kbd>Ctrl</kbd>+<kbd>I</kbd>). |
| **Chapters** | yes | A menu of the chapters and titles of the media. |
| **Favorite** | yes | Adds the media to the favorites of the library, or removes it (*Available for items in your library*). |
| **Bookmark** | yes | A menu of the bookmarks of the media, above the button. |
| **Bookmarks** | yes | Opens the bookmarks window (<kbd>Ctrl</kbd>+<kbd>B</kbd>). |
| **Cast to TV** | yes | Opens the cast panel (see [Casting to a TV](Casting)). |
| **Spacer**, **Divider** | yes | A fixed space, and a thin line, between buttons. |
| **Volume** | yes | The volume (see [Volume](#volume)). |
| **Menu** | yes | The ⋯ main menu, in the bar. |
| **Library** | no | The library toggle of the top bar, in the bar; in the accent colour while the library shows. |
| **Previous chapter**, **Next chapter** | no | Go to the previous or next chapter. |
| **Duration**, **FPS**, **Resolution** | no | The length, the frame rate or the size of the video, as text. |
| **High Resolution Time** | no | The time with frames (00:00:00:00). |
| **Open** | no | Opens the open dialog. |
| **DVD menus**, **Navigation**, **Navigation Box**, **Program Button**, **Teletext**, **Trickplay Reverse**, **Expanding Spacer**, **Artwork Info**, **Switch Player** | no | VLC's own controls: the menus of a disc, the arrows of a disc menu, the programs of a TV stream, teletext, reverse playback, a space that fills what is left, and in the mini player the cover with the title and the button that opens the player. |

## The seek bar

- **Click** to go there, or **drag**: playback pauses while you drag (no bursts of sound), the picture follows,
  and playing goes on when you let go. A click near the start of a chapter goes to that chapter.
- **Frame previews.** While the pointer is over the seek bar of a local video, a frame of the video at that place
  shows above it, with its time. The frames around the pointer are decoded ahead, so they come fast. Network
  media have no preview. The setting is *Show thumbnail while scrubbing* in [Settings: Playback](Settings-Playback)
  (option `qt-seek-thumbnail`).
- **Chapters.** A media with chapters shows them as sections of the bar while the pointer is over it (and three
  seconds after); the time tooltip adds the name of the chapter.
- **Waveform.** A song shows its waveform in the bar: the bars already played are lit. It is made for local
  files up to 1 GB and 8 minutes; other media get the plain seek bar.
- **The times.** The time played is on the left, the length on the right, always with hours (00:02:10).
  - Click the **time played** to type a time to go to: `1:02:10`, `62:10` or `130` (hours, minutes,
    seconds), then <kbd>Enter</kbd>. <kbd>Esc</kbd> or a click elsewhere leaves it. <kbd>Ctrl</kbd>+<kbd>T</kbd>
    opens VLC's *Go to time* dialog.
  - Click the **length** to show the **time left** instead (`-00:41:12`); click again for the length. At a speed
    other than 1x, the tooltip of the time left gives the real time left at that speed.
- Right to left (Hebrew, Arabic...), time still runs from left to right on the bar, as on most players.

## Volume

- The volume goes from 0 to **200 %** (the *Maximum volume* of [Settings: Audio](Settings-Audio), 100 % to 300 %).
  The slider's fill grows like a cone, as the design draws it, and its icon shows four states (mute, low, medium,
  high).
- While the pointer is on it, while you drag it, and a moment after the wheel or a key changes it, the value shows
  in a box over the slider (*100 %*). Dragging near 100 % snaps to it.
- The speaker icon mutes and unmutes (<kbd>M</kbd>).
- Keys: <kbd>Ctrl</kbd>+<kbd>Up</kbd> / <kbd>Ctrl</kbd>+<kbd>Down</kbd>, and <kbd>Up</kbd> / <kbd>Down</kbd> while a
  video plays. The wheel over the video changes it too (see [Mouse on the video](#mouse-on-the-video)).
- The volume at startup is in [Settings: Audio](Settings-Audio) too.

## The queue card

**Play Queue** (or <kbd>Q</kbd>, or <kbd>Ctrl</kbd>+<kbd>L</kbd>) opens a card over the right of the video, with three
tabs:

| Tab | What it shows |
|---|---|
| **Queue** | The play queue: drag rows to reorder them, select, right-click for the menu of a row, double-click to play. With *Show the playlist as a tree* ([Settings: Interface](Settings-Interface)) the media are listed under the folder or website they come from. |
| **Bookmarks** | The bookmarks of the media, numbered, with their time. **+** adds one at the current time; a row's remove button deletes it; a click goes there. Bookmarks are kept in the media library: without it the tab reads *Bookmarks need the media library*. |
| **Chapters** | The chapters of the media: number, name and start. The current one stands out; a click goes there. *No chapters in this media* otherwise. |

## Panels that open above the bar

Five buttons open a panel right above them: **Languages and tracks**, **Effects**, **VLC Settings**, **HDR**
and **Cast to TV**. While one is open the bar stays, and the video is not dimmed. A click on the button again, or
anywhere outside, closes it (that click does nothing else: it does not pause the video); opening another closes
the first. Opened from a menu or a shortcut (*Effects and Filters*, <kbd>Ctrl</kbd>+<kbd>E</kbd>), the effects open in
the middle of the window instead.

### Languages and tracks

Three tabs, **Video**, **Audio** and **Subtitles** (Subtitles first), and a footer:

- **Subtitles**: the languages, the subtitles of the chosen one, the online search, the timing and the position
  and size of the subtitles. All of it is on the [Subtitles](Subtitles) page.
- **Audio**: *Embedded Tracks* (the audio tracks, and *None*), *Output Device* (the sound devices, with a refresh
  button), and *Audio delay* with − / + and *Reset*.
- **Video**: *Embedded Tracks* (the video tracks, and *None*).
- **Playback** (footer): the speed (*Current: 1.00×*, and 0.5× 0.75× 1× 1.25× 1.5× 2×), the chapters, and
  *Manage bookmarks…*.
- **Show Preferences** (footer): closes the panel and opens the page of Settings the tab is about (Subtitles,
  Audio, Video or Playback).

### Effects

The equalizer, compressor, spatializer and stereo widener, the video effects, and the effect presets. See
[Effects](Effects).

### The VLC Settings menu

| Row | What it does |
|---|---|
| **Profile (*name*)** | The profiles: a click on one switches to it; **New profile…** makes one. See [Customize the player](Customize-the-Player). |
| **Customize player** | Opens *Customize Player*. |
| **Hide player UI** / **Show player UI** | The compact view: the video alone, without the bars (<kbd>Ctrl</kbd>+<kbd>H</kbd> or <kbd>Ctrl</kbd>+<kbd>\\</kbd>). |
| **Sleep timer** | See [Sleep timer](#sleep-timer). |
| **Preferences** | Opens [Settings: Playback](Settings-Playback). |

### HDR

The output of HDR video: **Auto**, **Off**, **HDR10**, **HDR10+**, **Dolby Vision** and **HLG**. HDR10+ and Dolby
Vision are listed but cannot be chosen: the pointer over them says why (*Windows offers no way for a player to send
this format to a display.*). The HDR formats use the libplacebo renderer and a display and driver that take HDR;
*Auto* and *Off* go back to the renderer used before. A choice applies to the **next** media (*HDR output saved.
Applies to the next media.*). Without the renderer, the panel says *HDR unavailable*.

### Cast

The TVs and Chromecasts of your network. See [Casting to a TV](Casting).

## Speed

- **Playback Speed** opens a popup with a slider from **0.25** to **4.00** and *Presets:* 0.25, 0.5, 0.75, 0.96,
  1.0, 1.04, 1.25, 1.5, 1.75 and 2. The wheel over the button changes the speed without opening it.
- **Slower** and **Faster**, and the speeds of the *Playback* footer of the languages and tracks panel.
- Keys: <kbd>+</kbd> / <kbd>-</kbd> (a step), <kbd>]</kbd> / <kbd>[</kbd> (a fine step), <kbd>=</kbd> (back to 1x).
- The default speed is in [Settings: Playback](Settings-Playback). Picture and sound change speed together.

## A-B loop

**A-B Loop** (in the bar of the *Work* profile): a first click marks **A** at the current time (the icon half lit),
a second click marks **B** and the part between them plays in a loop (the icon lit), a third click ends the loop.

## Sleep timer

**VLC Settings → Sleep timer**: *After* a number of **minutes** (1 to 720, 30 to begin with), then **Stop
playback** or **Pause playback**, and **Start timer**. While it runs, the window shows *Remaining: …* and the button
reads **Cancel timer**. The timer goes on across the media of the queue.

## Picture in picture

**Picture in Picture** (or the library button while a video plays) shows the library with the video in a small
player over it:

- Drag it anywhere in the window.
- A click plays or pauses; a double click goes back to the player.
- On hover: *Previous item*, *Play or pause*, *Next item*, *close video* and *maximize player* (back to the
  player).

## The mini player

In the library, what plays stays at hand:

- **The mini player card** (a video, or a song when the library shows): the picture or the cover, a seek bar, the
  title and the controls. **Show queue** opens the queue on the card: drag a row by its handle to move it, click a
  row to select it (**−** removes it), double-click to play it, **+** adds files.
- **The player bar** at the bottom of the library: the cover with the title, Shuffle, Previous, Play, Next, Repeat,
  the volume and *Switch Player* (back to the player). Its buttons follow the profile. With *Always show the player
  bar* ([Settings: Interface](Settings-Interface)) it stays while nothing plays: its Play starts the queue, or opens
  files when the queue is empty.

## The library next to the video

**Library** in the top bar (or <kbd>L</kbd>) opens a compact library over the left of the video, to pick something
without leaving the player: Home with the sections of the library's Home, *Videos* and its folders, *Music* (Songs,
Albums, Artists, Genres) and *Network*. Each place has a search, a sort, a grid or list view, *Play all* / *Add all
to queue*, and a ⋯ menu (*Rescan library*, *Add folder to library…*, *Add network stream…*). *Open in the library*
leaves the player for the same place in the library. See [Library](Library).

## Mouse on the video

| Action | What it does | Where to change it |
|---|---|---|
| Click | Play / pause. | *Single click on the video* ([Settings: Controls](Settings-Controls)) |
| Double click | Fullscreen. | *Double click on the video*: Nothing / Toggle fullscreen |
| Wheel | Volume. | *Scroll wheel*: Ignore / Volume control / Position control / Position control reversed |
| Horizontal wheel | Position (seek). | *Horizontal scroll*, and *Reverse scroll direction* |
| <kbd>Ctrl</kbd>+wheel | Subtitles bigger or smaller. | Keyboard Shortcuts |
| Right click | The player's menu: Play/Pause, Stop, Previous / Next Item, Record, Hide UI, the *Playback*, *Video*, *Audio* and *Subtitle* menus, *More Settings* (Effects and Filters, Track Synchronization, Media information, Program Guide, Plugins and extensions, Preferences, View, Help), *Playlist* (Play Queue, Shuffle, Repeat Off / All / One, Quit at the end of play queue), *Open Media*, *Quit VLC*. | |

The same click, double click and wheel work on the card shown while a film plays on a TV. A right click on the
music view or on a visualization opens the same menu.

## Fullscreen

**Fullscreen**, <kbd>F</kbd>, <kbd>F11</kbd> or a double click enter it; the same, or <kbd>Esc</kbd>, leave it. The bars
hide after the mouse stops (*Hide after*, [Settings: Interface](Settings-Interface)) and come back when it moves, or
with <kbd>I</kbd>. Which screen is used: *Open fullscreen on*.

## Music

- The cover and the title and artist sit in the middle, over a blurred copy of the cover; the waveform is the seek
  bar.
- **The title stays readable.** In the dark theme, VLC measures how bright the artwork is behind the title line:
  the title keeps the design's grey where that grey reads at a contrast of 3:1 or more, and turns near white or
  dark (the more readable of the two) over a bright cover. The light theme keeps the colours of its design.
- A song with synchronized lyrics in its tags shows the current line under the title; the lyrics button opens all
  of them, which follow the song (*Sync lyrics to playback*).
- **Visualizer**: while a song shows a visualization, a button above the controls picks it: *Disabled*,
  Spectrometer, Scope, Spectrum, Waveform, VU meter, Goom. The same list is in [Settings: Audio](Settings-Audio).

## When a media starts

- **Continue where you left off?** A card next to the controls offers to go on from where a media was left
  (*You've watched till 00:45:08 already.*, **Continue Watching**). It stays about 10 seconds; <kbd>Esc</kbd> closes it.
  Ask, always or never: [Settings: Playback](Settings-Playback).
- The file name shows at the top of the picture for a few seconds (*Show filename when a video starts*,
  [Settings: Subtitles](Settings-Subtitles)).
- With nothing to play, the player shows the pixel cat of the design.

---

[Library](Library) · [Customize the player](Customize-the-Player) · [Subtitles](Subtitles) · [Effects](Effects) · [Keyboard shortcuts](Keyboard-Shortcuts) · [Home](Home)
