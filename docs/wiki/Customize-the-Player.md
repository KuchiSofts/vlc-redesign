# Customize the player

The control bar is edited right on the player: drag buttons onto it, along it and off it. A layout is kept in a
**profile**, with your settings, so you can switch from a bar made for films to one made for music in two clicks.

## Open Customize Player

- **⋯ → Customize Player**, or
- **VLC Settings** (the gear of the bar) → **Customize player**.

From the library, it switches to the player first (and back to the library when you close it, if nothing plays).
A panel opens over the player, titled **Customize Player**: *"Drag items around and out of the box below to re-order
them. Drag items into the box to remove them from the player."* The bar of the player below it shows the zones
**LEFT**, **CENTER** and **RIGHT**, each outlined.

## Move, add and remove buttons

| To | Do this |
|---|---|
| **Add** a button | Drag it from the palette (the grid of the panel) onto the bar. A line in the accent colour shows where it goes: before or after a button, or between two. A button let go in the gap between two controls goes between them. |
| **Move** a button | Drag it along the bar, or into another zone. |
| **Remove** a button | Drag it off the bar into the palette: the palette reads *Drop here to remove from the player*. |
| **Move the seek bar** | Drag the timeline to the row above the bar to give it a row of its own, and back. A control dropped on the timeline goes in the center of the bar. |
| **Keep** the changes | **Save**. It is dimmed until something changed. |
| **Drop** the changes | **Cancel**, the **×** of the panel, or <kbd>Esc</kbd>. Closing VLC while customizing drops them too. |

Two rules of the zones: an *Expanding Spacer* cannot go in the center, and the controls of the center keep their own
width. The shortcuts of the window (<kbd>Ctrl</kbd>+<kbd>O</kbd> and the like) are off while the panel is open.

Customize Player edits the bar of the video player. The music player and the mini player have bars of their own in
each profile (see the table of the built-in profiles below).

The **Fullscreen** button sits at the right end of the bar. Move it or take it out like any other: <kbd>F</kbd>,
<kbd>F11</kbd> and a double click on the video still toggle fullscreen.

## The palette

The palette lists the 31 items of the design, in its order, then **Cast to TV**, then the bar's own controls
(see [below](#the-bars-own-controls)):

| Item | What it does in the bar |
|---|---|
| **Spacer** | A fixed space between two buttons. |
| **Divider** | A thin vertical line between groups of buttons. |
| **Playback Speed** | The speed (*1x*); a click opens the speed slider and its presets, the wheel over it changes the speed. |
| **Forward 10s** | Jumps 10 seconds forward. |
| **Backward 10s** | Jumps 10 seconds back. |
| **Forward 15s** | Jumps 15 seconds forward. |
| **Backward 15s** | Jumps 15 seconds back. |
| **Stop** | Stops playback. |
| **Repeat** | Off, Repeat all, Repeat one, in turn. |
| **Shuffle** | Random order for the queue. |
| **Record** | Records what plays; in the accent colour while recording. |
| **Snapshot** | Saves the current picture. |
| **Picture in Picture** | The library, with the video in a small player over it. |
| **HDR** | The HDR output panel (Auto, Off, HDR10, HLG...). |
| **Zoom** | A list of zoom levels of the video. |
| **Right Sidebar** | Shows or hides the queue card. |
| **Quit VLC** | Quits. |
| **VLC Settings** | The menu of the player: profiles, Customize player, Hide player UI, Sleep timer, Preferences. |
| **Go Home** | Home of the library. |
| **Thumbnail** | The artwork of the media. |
| **File Name** | The title of the media, as text. |
| **Artist Name** | The artist of the song. |
| **Folder Name** | The folder of the media. |
| **Playlist Name** | The queue and the place in it, *Play Queue (3/10)*. |
| **Properties** | The media information window. |
| **Replay** | Back to the start of the media. |
| **Chapters** | A menu of the chapters and titles. |
| **Favorite** | Adds the media to the library's favorites, or removes it. |
| **Bookmark** | A menu of the bookmarks of the media. |
| **Bookmarks** | The bookmarks window. |
| **Playlists** | The Playlists page of the library. |
| **Cast to TV** | The cast panel: Chromecasts and DLNA TVs (see [Casting to a TV](Casting)). |
| **Play** | Play / pause. |
| **Previous**, **Next** | The previous or next media of the queue. |
| **Volume** | The volume slider and the mute button. |
| **Languages and tracks** | The subtitles, audio and video tracks panel. |
| **Effects** | The audio and video effects panel. |
| **Play Queue** | Shows or hides the queue card. |
| **Fullscreen** | Enters or leaves fullscreen. |
| **A-B Loop** | Loops a part of the media. |
| **Previous Frame**, **Next Frame** | Steps one picture back or forward. |
| **Menu** | The ⋯ main menu, in the bar. |

Each one is described in more detail on [The player](The-Player#every-button-and-widget).

### The bar's own controls

The design's palette leaves out the controls every bar has. VLC Redesign lists them after the design's items:
**Play**, **Previous**, **Next**, **Volume**, **Languages and tracks**, **Effects**, **Play Queue**,
**Fullscreen**, **A-B Loop**, **Previous Frame**, **Next Frame** and **Menu**. They move and come out like the
others, and a button taken out can be dragged back from the palette at any time, even after saving without it.

The mini player's cover with the title and its **Switch Player** button are not in the palette. To get one of them
back, start a **New** profile (it begins with the layout of *Default*), **Clone** a profile that has it, or switch
to a built-in profile.

## Profiles

A profile keeps:

- the **layouts** of the three bars: the video player, the music player and the mini player;
- the **settings** in use when it was saved: the options of the Settings window, the audio and video effects, the
  keyboard shortcuts, the theme, the accent colour and Developer mode.

What belongs to the installation is never in a profile, and choosing a profile never changes it: accounts and
passwords (Last.fm, the proxy), file and folder locations (the log, recordings, subtitle folders), the remote
controls, the privacy choices (recent media, network access for metadata) and whether the media library is on.

### The built-in profiles

A new install has the five profiles of the design. Their video player bars:

| Profile | Left | Right |
|---|---|---|
| **Default** | Previous, Play, Next | Volume, Effects, Languages and tracks, VLC Settings, Play Queue, Fullscreen |
| **Movies** (one long video) | Backward 10s, Play, Forward 10s | Volume, Languages and tracks, Chapters, Playback Speed, VLC Settings, Fullscreen |
| **Shows** (episodes in a row) | Previous, Backward 10s, Play, Forward 10s, Next | Volume, Languages and tracks, Play Queue, VLC Settings, Fullscreen |
| **Music** (albums and playlists) | Shuffle, Previous, Play, Next, Repeat | Volume, Favorite, Play Queue, VLC Settings, Fullscreen |
| **Work** (reviews, lectures, recordings) | Previous Frame, Play, Next Frame | Playback Speed, A-B Loop, Snapshot, Bookmark, Volume, VLC Settings, Fullscreen |

Their music player bars: *Default*, *Movies* and *Shows* have Previous, Backward 15s, Play, Forward 15s, Next, then
Volume, Effects, Play Queue, Fullscreen; *Music* has Shuffle, Previous, Play, Next, Repeat, then Volume,
Favorite, Effects, Play Queue, Fullscreen; *Work* has Backward 15s, Play, Forward 15s, then Playback Speed,
A-B Loop, Bookmark, Volume, Play Queue, Fullscreen. Each also has a mini player bar to match.

A built-in profile has no settings of its own until you first leave it (or save it): until then, choosing it keeps
the settings in use.

### Switch profile

**⋯ → Profile (*name*)**, or **VLC Settings → Profile (*name*)**, then a profile. The bar changes at once, and the
profile's settings apply. Settings you change after that (in Settings, from the player, with a shortcut) are kept
from one start of VLC Redesign to the next.

### New, Rename, Clone and Delete

| Action | Where | What it does |
|---|---|---|
| **New** | **New profile…** at the end of the profile list | Asks for a *Profile Name* (*e.g., Late Night*). *"The new profile starts with the default layout and the settings in use."* Customize Player then opens on it: arrange it and **Save** (Cancel drops it). |
| **Rename** | The ⋯ of a profile's row (shown under the pointer) | Asks for a new name. *"The profile keeps its layout and its settings."* |
| **Clone** | The same ⋯ | Asks for a name. *"The copy starts with the layout and the settings of this profile."* |
| **Delete** | The same ⋯ | Asks *Delete name?* — *"Its layout and its settings go with it."* The last profile cannot be deleted. |

Two profiles cannot have the same name: a name already taken gets a number, *Profile (1)*.

---

[The player](The-Player) · [Keyboard shortcuts](Keyboard-Shortcuts) · [Settings](Settings) · [Home](Home)
