# Settings

One window for every option of VLC Redesign: **⋯ → Settings**, or <kbd>Ctrl</kbd>+<kbd>P</kbd>.

![The Settings window](images/settings-general.png)

## The sections

- **[General](Settings-General)**: The look of the app (theme, accent colour, language), updates, logs and the reset button.
- **[Interface](Settings-Interface)**: The window, the control bar, notifications and the system tray, and VLC's classic skins.
- **[Privacy](Settings-Privacy)**: What VLC remembers (recent media, resume points), what it fetches from the network, and Last.fm.
- **[Controls](Settings-Controls)**: Keyboard shortcuts, the jump lengths of the skip buttons, the mouse wheel and remotes.
- **[Playback](Settings-Playback)**: Resuming, playback speed, repeat and shuffle, what happens at the end, seeking and discs.
- **[Video](Settings-Video)**: The video output, the picture (deinterlacing, aspect ratio, cropping, zoom) and snapshots.
- **[Audio](Settings-Audio)**: The audio output, the volume at startup, loudness, the equalizer and audio effects, visualizations.
- **[Subtitles](Settings-Subtitles)**: Subtitle languages and encoding, automatic loading, how subtitles look, timing and on-screen messages.
- **[Library](Settings-Library)**: How files open and what VLC reads about them (metadata and artwork).
- **[Network](Settings-Network)**: Buffering of network streams, a proxy, and the security of encrypted connections.
- **[Performance](Settings-Performance)**: Hardware decoding and the options that keep playback smooth on slow computers.
- **[Recording](Settings-Recording)**: Where recordings go, casting to other devices, and streaming.
- **[Extensions](Settings-Extensions)**: Add-ons (extensions, playlists parsers, skins) and the list of VLC's plugins.

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

Also new: settings changed just before quitting VLC are kept (quitting with the window still open dropped them), and
every page starts as the design draws it, compared with its frame pixel for pixel (see [Design check](Design-Check)).

## How the window works

- **Layout.** A search box sits top left, the page title in the middle and a close button (X) on the right. The sidebar has three groups: *App* (General, Interface, Privacy, Controls), *Media* (Playback, Video, Audio, Subtitles, Library) and *System* (Network, Performance, Recording, Extensions). A sidebar entry is disabled when this VLC build has none of that page's options. Extensions is always enabled.
- **No Apply button.** Changes are kept when the window closes (X or Esc). If VLC refuses a change, the window stays open and shows an error bar at the bottom with a **Discard changes** button.
- **Advanced Options.** A collapsible bar at the bottom of every page except Privacy. It always starts closed. Rows marked *(Advanced)* below are inside it.
- **Developer mode.** A toggle on the General page. Rows marked *(Developer mode)* only appear under Advanced Options while it is on. It also adds an **All VLC options** button on General, which opens the classic preferences.
- **[restart].** Once you change one of these rows, a grey note **"Applies after restart"** appears under it.
- **Dimmed rows.** Some rows stay greyed out and inactive until another setting turns them on. Each case is noted below.
- **Availability.** A row only appears if this VLC build has the option behind it. A missing module or platform hides the row. Platform-specific rows are noted below.
- **Search.** The search box looks through every page at once. It matches the label, the section name and the tooltip text. While searching, Advanced and Developer-mode rows are included, but Developer-mode rows still need Developer mode on. The Appearance block, Keyboard Shortcuts (up to 8 matching actions, then "*N* more matching shortcuts / Show all"), Last.fm and Developer mode also show up when their own labels match.

Notation per row: **Label**, then the kind of control, the default, the underlying option key (`vlcrc` option, or `redesign/...` for settings the interface stores in its own Qt settings file), and what it does.

## Behind the scenes

Some rows write several VLC options at once (for example *Night mode* sets the compressor). Everything is saved in VLC's own settings file (`vlcrc`) or, for the redesign's own settings, in `vlc-qt-interface.ini`, both in the `portable` folder next to `vlc.exe`.

