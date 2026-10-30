# Settings

The Settings window holds every option of VLC Redesign that the design shows, plus a few of VLC's own options under
**Advanced Options**. Open it with **⋯ → Settings** or <kbd>Ctrl</kbd>+<kbd>P</kbd>.

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/settings-search.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/settings-search.gif" alt="Settings: the Interface, Playback and Subtitles pages, then a search for speed across every page" width="960"></a>

<sub>The Interface, Playback and Subtitles pages, then a search for "speed" across every page. Click for the MP4 · every clip: [Feature tour](Feature-Tour#settings).</sub>

![The Settings window, General page](images/settings.jpg)

## The pages

The sidebar groups the 13 pages under three headings. The title in the window's header is the one in the third column.

| Group | Page | Header title | What it holds |
|---|---|---|---|
| App | **[General](Settings-General)** | Global Settings | Language, layout direction, theme and accent colour; update check and dialogs; logs, Developer mode, reset. |
| App | **[Interface](Settings-Interface)** | Interface Settings | The window, the playback controls, notifications and the tray icon, the playlist tree, the player bar, text contrast, skins. |
| App | **[Privacy](Settings-Privacy)** | Privacy Settings | History and resume points, what VLC fetches from the internet, Last.fm. |
| App | **[Controls](Settings-Controls)** | Mouse, Keyboard & Remote Controls | Keyboard shortcuts, skip lengths, the mouse on the video, the web remote. |
| Media | **[Playback](Settings-Playback)** | Playback Preferences | Resuming, start and speed, preferred tracks, repeat and shuffle, seeking, discs and damaged files. |
| Media | **[Video](Settings-Video)** | Video Defaults | Video output, picture adjustment, deinterlacing, framing (aspect ratio, crop, zoom), snapshots. |
| Media | **[Audio](Settings-Audio)** | Audio Defaults | Output device, volume, loudness, equalizer and effects, audio tracks, visualizations. |
| Media | **[Subtitles](Settings-Subtitles)** | Subtitle Defaults | Subtitle languages and encoding, automatic loading, the look of subtitles, timing, on-screen messages. |
| Media | **[Library](Settings-Library)** | Playlists, Folders & Files | How files open (one window, queue or replace, folders), metadata and artwork, the media library. |
| System | **[Network](Settings-Network)** | Network Connections & Security | Buffering, the HTTP proxy, certificates and ciphers. |
| System | **[Performance](Settings-Performance)** | Performance Settings | Hardware decoding, post-processing, threads, process priority. |
| System | **[Recording](Settings-Recording)** | Recording, Casting & Streaming Out | Where recordings go, casting quality, stream output. |
| System | **[Extensions](Settings-Extensions)** | Add-ons & Scripts | Add-ons and VLC's plugin list. |

## How the window works

### Layout

- The header has a **Search** box on the left, the page title in the middle and a close button (X) on the right.
- Each page is divided into sections (*Appearance*, *Startup*, *Window*…). Most rows have an (i) icon: hover it
  (or the label) to read what the option does. When a long translation cuts a label short, the tooltip shows the
  whole label first.
- A page without any option in this build is left out of the sidebar. **Extensions** is always listed. If a page
  has nothing to show, it reads *No settings are available for this category on this VLC installation*.
- A row only appears when this build of VLC has the option behind it. Rows that depend on a module or on the
  operating system are noted on each page.

### Saving: no Apply button

- Changes are kept when the window closes: the X button, <kbd>Esc</kbd>, or quitting VLC while the window is open.
- The theme, accent colour, interface language and layout direction preview at once. So do the **Picture** sliders
  of the [Video](Settings-Video) page and the subtitles' **Vertical position**. Every other change is applied when
  the window closes. Most of them then take effect at once (the log, the web remote, Last.fm, the keyboard
  shortcuts, the tray icon, the window opacity, the default speed and so on). Options that VLC reads when a media
  opens (aspect ratio, deinterlacing, tracks…) take effect with the next media.
- If VLC refuses a change, the window stays open and an error bar appears at the bottom of the page. A typical
  message is *"… changed outside the settings. Discard your changes to see its new value."*. When saving failed,
  the bar has a **Discard changes** button: it drops every pending change of the option rows and closes the window. The Appearance
  choices (language, layout direction, theme, accent colour) and Developer mode are still kept.

### Applies the next time VLC starts

Only three rows need VLC to start again, because they decide what VLC is made of. Once you change one, the grey
note **"Applies the next time VLC starts"** appears under it:

- **Additional command line options** ([General](Settings-General) › Advanced Options)
- **Use a custom skin** ([Interface](Settings-Interface) › Skin)
- **Use the media library** ([Library](Settings-Library) › Metadata & Artwork)

### Advanced Options

A bar at the bottom of every page except Privacy. Click it to open or close the extra rows. It is closed each time
the window opens and each time you change page. A page whose bar has nothing under it shows *No advanced options on
this page.*

### Developer mode

The **Developer mode** toggle ([General](Settings-General#diagnostics) › Diagnostics) shows the expert rows,
marked *Developer mode* on these pages, under the Advanced Options of their page. It also adds **All VLC options**
to the General page, which opens VLC's classic preferences with every option of VLC and its modules.

### Dimmed and empty rows

A dimmed row (drawn at 30 % opacity) cannot be changed until another setting allows it. A dimmed toggle shows OFF.

| Row | Dimmed while |
|---|---|
| [Interface](Settings-Interface) › **Fullscreen controls sensitivity** | **Show controls in fullscreen** is off. |
| [Library](Settings-Library) › **Add to the playlist instead of replacing it** | Both **Only allow one VLC window** and **Apply to files opened from the file manager** are off. |
| [Network](Settings-Network) › **Local files**, **Discs**, **Network streams**, **Live capture** | **Buffering** is not *Custom* (the fields are also empty). |

Some fields stay empty on purpose: the web remote's **Port** while **Web remote control** is off
([Controls](Settings-Controls)), and the three Last.fm fields while **Submit played tracks to last.fm** is off
([Privacy](Settings-Privacy)). A field shows its value again while you edit it.

### Search

The search box looks through every page at once; the header then reads *Search results*. It matches the text of
a row's label, its section name and its (i) help text, not VLC's internal option names (search *Shuffle*, not
*random*).

- Rows under Advanced Options are included. Developer-mode rows are included only while Developer mode is on.
- A row shown on two pages (the scroll-wheel rows of Controls, the time-shift size of Video) is listed once.
- The Appearance block, Developer mode, the Last.fm block and the subtitle *Second Language & Online Subtitles*
  block appear when their own labels match.
- Keyboard shortcuts are searched by the name of their action. Up to 8 matching actions are listed with their key;
  a button opens the action in the Keyboard Shortcuts window. More matches add a row *"N more matching shortcuts"*
  with **Show all**.
- No match: *No matching settings*.
- The Advanced Options bar, the Picture sliders, the Extensions page and **Clear history now** are not shown while
  searching.

## Where the settings are stored

| File | Holds |
|---|---|
| `vlcrc` | VLC's own options: everything marked with a VLC option name such as `snapshot-format` on these pages. |
| `vlc\vlc-qt-interface.ini` | The interface's own settings: the rows marked `redesign/…`, the theme, profiles, window state. |

Both live in VLC Redesign's settings folder: the `portable` folder next to `vlc.exe`, or `%APPDATA%\VLC Redesign`
when that folder cannot be used. **General › Advanced Options › Open configurations folder** opens it. See also
[Where are my settings?](Troubleshooting-and-FAQ#where-are-my-settings).

### Rows that stand for several options

Some rows of the design are not a single VLC option. They read and write the options behind them:

| Row | Options written |
|---|---|
| Privacy › Remember the playback position | `restore-playback-pos` (off = Never; on = Ask, or keeps Always) |
| Playback › Repeat | `repeat` (One) and `loop` (All) |
| Playback › When playback finishes | `play-and-exit` |
| Playback › Seek precisely | `input-fast-seek`, inverted |
| Controls › Reverse scroll direction | `hotkeys-y-wheel-mode` and `hotkeys-x-wheel-mode` (Seek stored as 3 instead of 2) |
| Controls › Web remote control | `http` in `extraintf` |
| Privacy › Submit played tracks to last.fm | `audioscrobbler` in `extraintf` (removed from `control` too when turned off) |
| Interface › Use a custom skin | `intf` (`skins2`) |
| Video › Size | `snapshot-width`, and `snapshot-height` when needed (see [Video](Settings-Video#snapshots)) |
| Audio › Night mode | the compressor in `audio-filter` with its ratio, threshold and makeup gain |

## Changed in 0.3.1

Found while documenting every option:

| Page | Row | Change |
|---|---|---|
| [Video](Settings-Video) › Output | **Stop the screensaver during playback** | A list: *Never / When fullscreen / Always*. The toggle it replaced showed *When fullscreen* as on and lost it when switched. |
| [Video](Settings-Video) › Snapshots | **Size** | Choosing a width no longer stretches the snapshot: the height follows it (0). |
| [Video](Settings-Video) › Advanced Options | **Snapshot height (0: proportional, -1: original)** | The label now says what 0 and −1 do. |
| [Controls](Settings-Controls) › Mouse | **Single click on the video** | One *Play / Pause* choice and *Nothing*; the two choices that did the same are merged. |
| [Performance](Settings-Performance) › Smooth Playback | **Increase the priority of the process** | Renamed from *Give VLC real-time priority*, which is not what it does. |
| [Library](Settings-Library) › Opening Files | **Apply to files opened from the file manager** | No longer dimmed: it works on its own. *Add to the playlist instead of replacing it* is available when either one-window option is on. |
| [Network](Settings-Network) › Proxy | **HTTP proxy** | Empty now really uses the Windows proxy, as its field says. |
| [Network](Settings-Network) › Buffering | **Buffering** | The preset row no longer shows an (i) icon. |

## New in 0.3.0

| Page | Row | What it does |
|---|---|---|
| [General](Settings-General) › Startup | **Show confirmation and error dialogs** (on) | The errors of VLC at the bottom of the window, and the questions before removing media or playlists. Off: neither (deleting a file from the disk still asks). |
| [General](Settings-General) › Advanced Options | **Additional command line options** | Options VLC reads at every start, as if written on its command line. |
| [Interface](Settings-Interface) › Playback Controls | **Show controls in fullscreen** (on) | The controls show in fullscreen when the mouse moves; off, only the keyboard shows them. |
| [Interface](Settings-Interface) › Playlist | **Show the playlist as a tree** (off) | The play queue under the folders (or websites) its media come from. |
| [Interface](Settings-Interface) › Player Bar | **Always show the player bar** (off) | The bar stays at the bottom of Home and the library while nothing plays. |
| [Interface](Settings-Interface) › Accessibility | **Increase text contrast** (off) | The dim texts of the design drawn brighter, to the WCAG AA contrast. |
| [Interface](Settings-Interface) › Advanced Options | **Show cover art in the window when playing audio** (on) | Renamed from *Show the VLC cone when there is no video*. |
| [Audio](Settings-Audio) › Visualization | **Visualizer** | Spectrometer, Scope, Spectrum, Waveform and VU meter by name, then VLC's other visualizations. |
| [Audio](Settings-Audio) › Effects | **Equalizer preset** | Starts with *None*; each preset shows its curve. |
| [Subtitles](Settings-Subtitles) › Tracks & Languages | **Preferred language** | The row of the design picks the language of the subtitles. |
| [Subtitles](Settings-Subtitles) › Second Language & Online Subtitles | *the block* | The second language, the online search and the subtitle services, now after the sections of the design. |

Also new in 0.3.0: almost every setting applies without a restart (only the three rows listed
[above](#applies-the-next-time-vlc-starts) need one), settings changed just before quitting VLC are kept, and every
page is drawn as the design draws it, compared with its frame pixel for pixel (see [Design check](Design-Check)).

---

[General](Settings-General) →
