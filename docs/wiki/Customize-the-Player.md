# Customize the player

The control bar is edited right on the player: drag buttons onto it, along it and off it. A layout is kept in a
**profile**, together with your settings, so you can switch from a bar made for films to one made for music in two
clicks.

![Customize Player open over the player: the title, the line "Drag items around and out of the box below to re-order them. Drag items into the box to remove them from the player.", the palette of items from Spacer to Volume, Cancel and Save, and below it the bar of the player and the top bar outlined with dashed boxes](images/customize.jpg)

## Open Customize Player

- **⋯ → Customize Player**, or
- **VLC Settings** (the gear of the bar) → **Customize player**.

From the library, it switches to the player first, and goes back to the library when it closes (unless something
plays by then). A panel opens over the player, titled **Customize Player**: *"Drag items around and out of the box
below to re-order them. Drag items into the box to remove them from the player."* The bar of the player shows its
zones **LEFT**, **CENTER** and **RIGHT**, each outlined, and every control in a dashed box.

## Move, add and remove buttons

| To | Do this |
|---|---|
| **Add** a button | Drag it from the palette (the grid of the panel) onto the bar. A line in the accent colour shows where it goes: before or after a button, or between two. A button let go in the gap between two controls goes between them. |
| **Move** a button | Drag it along the bar, or into another zone. |
| **Remove** a button | Drag it off the bar into the palette, which reads *Drop here to remove from the player*. |
| **Move the seek bar** | Drag the timeline to the row above the bar to give it a row of its own, and back. A control dropped on the timeline goes in the center zone. |
| **Move a top bar item** | The ⋯ menu, the Library button, the folder and the title of the top bar can be dragged into the control bar too (they become *Menu*, *Library*, *Folder Name* and *File Name*). Once in the bar, they leave the top bar. |
| **Keep** the changes | **Save**. It is dimmed until something changed. |
| **Drop** the changes | **Cancel**, the **×** of the panel (*Cancel and close*), or <kbd>Esc</kbd>. Closing VLC while customizing drops them too. |

**Rules of the zones**

- An *Expanding Spacer* cannot go in the center zone.
- The controls of the center keep their own width.
- While the panel is open, the keyboard shortcuts and the keys of the window (<kbd>Ctrl</kbd>+<kbd>O</kbd> and the
  like) are off.

Customize Player edits the bar of the video player. The music view and the mini player of the library have bars of
their own in each profile (see [The built-in profiles](#the-built-in-profiles)).

**Fullscreen.** The **Fullscreen** button sits at the right end of the bar. Move it or take it out like any other:
taken out of the bar, a Fullscreen button shows next to the window buttons of the top bar instead (while VLC draws
its own window buttons). <kbd>F</kbd>, <kbd>F11</kbd> and a double-click on the video always toggle fullscreen.

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
| **File Name** | The title of the media, as text (taken from the top bar). |
| **Artist Name** | The artist of the song. |
| **Folder Name** | The folder of the media (taken from the top bar). |
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
| **Menu** | The ⋯ main menu, in the bar (taken from the top bar). |

The palette scrolls when the window is too small to show it whole. Each item is described in more detail on
[The player](The-Player#every-button-and-widget).

### The bar's own controls

The design's palette leaves out the controls every bar has. Since 0.3.1, VLC Redesign lists them after the design's
items: **Play**, **Previous**, **Next**, **Volume**, **Languages and tracks**, **Effects**, **Play Queue**,
**Fullscreen**, **A-B Loop**, **Previous Frame**, **Next Frame** and **Menu**. They move and come out like the
others, and a button taken out can be dragged back from the palette at any time, even after saving without it.

Not in the palette:

- **Library**: drag the Library button of the top bar into the bar instead.
- The mini player's cover with the title (*Artwork Info*) and its **Switch Player** button, and VLC's own controls
  (*Slower*, *Faster*, *Aspect Ratio*, the disc and TV controls...). To get one of them back, start a **New**
  profile (it begins with the layout of *Default*), **Clone** a profile that has it, or switch to a built-in
  profile.

## Profiles

A profile keeps:

- the **layouts** of the three bars: the video player, the music player and the mini player of the library;
- the **settings** in use when it was saved: the options of the Settings window, the audio and video effects, the
  keyboard shortcuts (local and global), the theme, the accent colour and Developer mode.

What belongs to the installation is never in a profile, and choosing a profile never changes it: accounts and
passwords (Last.fm, the proxy), file and folder locations (the log, recordings, subtitle folders), the remote
controls and extra interfaces, *one instance* of VLC, the privacy choices (recent media, network access for
metadata) and whether the media library is on.

### The built-in profiles

A new install has the five profiles of the design. Their video player bars:

| Profile | Left | Right |
|---|---|---|
| **Default** | Previous, Play, Next | Volume, Effects, Languages and tracks, VLC Settings, Play Queue, Fullscreen |
| **Movies** (one long video) | Backward 10s, Play, Forward 10s | Volume, Languages and tracks, Chapters, Playback Speed, VLC Settings, Fullscreen |
| **Shows** (episodes in a row) | Previous, Backward 10s, Play, Forward 10s, Next | Volume, Languages and tracks, Play Queue, VLC Settings, Fullscreen |
| **Music** (albums and playlists) | Shuffle, Previous, Play, Next, Repeat | Volume, Favorite, Play Queue, VLC Settings, Fullscreen |
| **Work** (reviews, lectures, recordings) | Previous Frame, Play, Next Frame | Playback Speed, A-B Loop, Snapshot, Bookmark, Volume, VLC Settings, Fullscreen |

Their music player bars:

| Profile | Left | Right |
|---|---|---|
| **Default**, **Movies**, **Shows** | Previous, Backward 15s, Play, Forward 15s, Next | Volume, Effects, Play Queue, Fullscreen |
| **Music** | Shuffle, Previous, Play, Next, Repeat | Volume, Favorite, Effects, Play Queue, Fullscreen |
| **Work** | Backward 15s, Play, Forward 15s | Playback Speed, A-B Loop, Bookmark, Volume, Play Queue, Fullscreen |

Their mini player bars (the player bar at the bottom of the library), each starting with the cover and the title:

| Profile | Center | Right |
|---|---|---|
| **Default** | Shuffle, Previous, Play, Next, Repeat | Volume, Switch Player |
| **Movies** | Backward 10s, Play, Forward 10s | Volume, Switch Player |
| **Shows** | Previous, Play, Next | Volume, Switch Player |
| **Music** | Shuffle, Previous, Play, Next, Repeat | Favorite, Volume, Switch Player |
| **Work** | Backward 15s, Play, Forward 15s | Playback Speed, Volume, Switch Player |

A built-in profile has no settings of its own until you first leave it (or save it): until then, choosing it keeps
the settings in use.

### Switch profile

**⋯ → Profile (*name*)**, or **VLC Settings → Profile (*name*)**, then a profile. The bar changes at once, and the
profile's settings apply. Settings you change after that (in Settings, from the player, with a shortcut) are kept
from one start of VLC Redesign to the next.

### New, Rename, Clone and Delete

| Action | Where | What it does |
|---|---|---|
| **New** | **New profile…** at the end of the profile list of VLC Settings, or **New** in the ⋯ menu's *Profile* list | Asks for a *Profile Name* (*e.g., Late Night*; it suggests *Profile 6* and the like). *"The new profile starts with the default layout and the settings in use."* Customize Player then opens on it: arrange it and **Save** (Cancel drops it). |
| **Rename** | The ⋯ of a profile's row (shown under the pointer) | Asks for a new name. *"The profile keeps its layout and its settings."* |
| **Clone** | The same ⋯ | Asks for a name (it suggests *name (1)*). *"The copy starts with the layout and the settings of this profile."* |
| **Delete** | The same ⋯ | Asks *Delete name?* — *"Its layout and its settings go with it."* The last profile cannot be deleted. |

Two profiles cannot have the same name: a name already taken is refused (*A profile named "…" already exists.*), and
an empty one too (*Enter a name for the profile.*).

## Good to know

- **A button disappeared and is not in the palette.** It is one of VLC's own controls: switch to a built-in
  profile, or **Clone** one that has it.
- **The bar looks crowded.** Buttons hide from the inside of the bar when the window is narrow; put the seek bar on
  its own row to give the buttons more room.
- **Different bars for films and music.** Make a profile per use (*Movies*, *Music*) and switch with **⋯ →
  Profile**; each keeps its own effects and keys too.
- **The look of the bar** (dark, light, accent colour) is set in [Colours and themes](Colours-and-Themes).

---

[The player](The-Player) · [Keyboard shortcuts](Keyboard-Shortcuts) · [Colours and themes](Colours-and-Themes) · [Settings](Settings) · [Home](Home)
