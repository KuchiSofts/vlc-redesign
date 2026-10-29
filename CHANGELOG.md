# Changelog

Changes of **VLC Redesign**, an unofficial VLC 4 build with the interface designed by
[Juxtopposed](https://www.youtube.com/@juxtopposed) ([video](https://www.youtube.com/watch?v=ERFeOgqk324)).
For changes to VLC itself, see [NEWS](NEWS).

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Each section groups changes by
what you see first, with the technical details after. Based on VLC `4.0.0-dev`, merged up to `12023233ef` (2026-10-29).

**Contents:** [Unreleased](#unreleased) · [0.3.1](#031--2026-10-25) · [0.3.0](#030--2026-10-25) · [0.2.6](#026--2026-10-22) · [0.2.5](#025--2026-10-22) · [0.2.4](#024--2026-10-22) · [0.2.3](#023--2026-10-20) · [0.2.2](#022--2026-10-20) · [0.2.1](#021--2026-10-20) · [0.2.0](#020--2026-10-20) · [0.1.0](#010--2026-10-20) · [M3](#m3--dialogs-settings-and-playback-tools--2026-10-13) ·
[M2](#m2--library-playlists-and-subtitles--2026-10-13) · [M1](#m1--first-native-shell-and-player--2026-10-12) ·
[M0](#m0--baseline-and-build--2026-10-12) · [Differences from upstream VLC](#differences-from-upstream-vlc)

---

## [Unreleased]

### Added

- **Live TV.** Watch the channels of your own IPTV provider like on a TV box: **TV** in the sidebar lists them
  with their logos, numbers and groups, what is on now (with its progress) and what is next, your favourites and
  the ones watched last; **Guide** shows the programmes of every channel across the hours, with the line of now.
  Add a playlist link or file (M3U, its guide found in it), an Xtream Codes login (with its films and series), or
  a guide link of your own (XMLTV, several merged), or one of the free, legal channel lists of the **catalog** in a
  click; *Suggest a Service* proposes a new one on GitHub. While watching: Page Up and Page Down change channel,
  type a number, Backspace goes back to the last channel, Enter shows the channel list over the video, and a
  banner shows what is on at every switch. Past programmes play from the archive when the channel keeps one
  (catch-up), *Watch from the Start* restarts the one on now, and a reminder tells when a programme starts. The
  channels and the guide update in the background and work offline; links and passwords stay in VLC's keystore.
  Live TV is an add-on of its own, on by default: *Live TV (IPTV) add-on* in Settings, General turns it off, the
  installer can leave it out, and without it nothing of it shows. Settings, TV has its sources and options. A guide
  is read once into an index kept beside it: VLC starts with the guide ready in about half a second, even for a
  guide of 200 MB. See [Live TV](https://github.com/KuchiSofts/vlc-redesign/wiki/Live-TV).
- **Edit Sidebar.** Put the pages of the sidebar in your order (drag them, or the arrows), hide the ones you do
  not use, or reset them: right-click the sidebar, or Settings, Interface, Sidebar.
- **Subtitles for films only.** A live TV channel or music shows no subtitle files, online search or sync; the
  tracks button only shows when there is a track to choose.
- **Suggest an idea and Report a problem**, at the end of the ⋯ menu and in About. *Suggest an idea* opens a new
  discussion in the Ideas category of GitHub Discussions, with the version. *Report a problem* first shows what the report contains (versions,
  install type, Windows, CPU, graphics adapter and driver, video output and decoding, screens, theme, accent,
  language), with *Copy* and *Open GitHub*, which opens the bug form filled in. Nothing is sent by VLC, and no
  personal data goes in: no file names or paths, no user or computer name, no library, no log.
- **What's new after an update.** The first time VLC opens after an update (the installer, a small update, or a
  new portable copy that took the settings of an older one), a card lists the highlights of the version, from
  this changelog, with *Full changelog* for the release notes on GitHub. It shows once per version, on the
  library: a file opened with VLC plays first, and a new install shows the welcome instead. *Show what's new
  after an update* in Settings, General turns it off. The notes are in English.
- **Follow the Windows accent color**: a switch in Settings > Appearance, under the accent colours (off by
  default). On, the interface, the classic windows, the right-click menus and the two-tone icons take the accent
  colour of Windows and follow it at once when it changes in Personalization > Colors; picking a colour turns it
  off. A colour too dark or too light is brought to the range of the built-in accents so it stays as readable.
  Outside Windows it follows the system accent (the desktop's, or the orange). Profiles keep the choice.
- **Subtitle lines on the seek bar.** While a subtitle file is shown (one you added, one found next to the video, or
  one downloaded by the subtitle search), thin marks on the seek bar show where its lines are, so you can see where
  people talk. The marks follow the subtitle delay, close lines join into one mark, and the part already played
  covers them. Hovering the bar shows the line at that time in the frame preview (or under the time). SRT, WebVTT,
  ASS/SSA, MicroDVD, SubViewer/SBV, SAMI and VobSub (`.idx`) files are read once, in the background, and kept for
  the next time. Subtitles inside the video file and streams that cannot seek have no marks. Settings, Subtitles,
  Timing: *Show subtitle lines on the seek bar* (`qt-seek-subtitle-cues`, on by default, applies at once).

### Changed

- **One app icon everywhere: the designer's.** The icon of VLC Redesign is now Juxtopposed's own logo from the
  Figma file (the orange cone), as drawn, on no card: `vlc.exe` and its taskbar button, shortcuts and files, the
  installer and uninstaller, the macOS app, the Linux menus (hicolor 16 to 512 px and the SVG), the UPnP server
  as TVs see it, the web interface, the site's favicon and the README. The small sizes (16 to 40 px) are hinted so
  the cone stays sharp; `.ico` has 16, 20, 24, 32, 40, 48, 64, 96, 128 and 256 px, `.icns` 16 to 512@2x, all from
  the vector (`extras/juxtopposed/make-app-icon.py`). The start page, the player's background and the update
  notice draw the same logo in place of upstream's cone, and the seasonal cones (Christmas, April Fools) are gone.
- **A button shows when what it opens is open** (suggested in [#10](https://github.com/KuchiSofts/vlc-redesign/issues/10)).
  Settings, Languages and tracks, Chapters, Bookmarks, Programs, Cast, HDR, Effects, Zoom, the ⋯ menu, the top
  bar's menu and library buttons and Live TV's channel list light up while their panel, popup or menu shows: the
  accent icon on a tint of the accent, with no border, the same look as a toggle that is on (the small line under
  toggles is gone). On Android the player's buttons and toggles do the same.
- **Ctrl+wheel over the video changes the subtitle size** (suggested in [#10](https://github.com/KuchiSofts/vlc-redesign/issues/10)),
  as in classic VLC, instead of zooming the whole interface. It goes by 10 % near 100 %, then by 25 %, from 25 % to
  500 %, and a short *Subtitle size 120%* message shows it; Ctrl+0 goes back to 100 %. Elsewhere in the window
  Ctrl+wheel still zooms the interface. Both are shortcuts in Settings, Keyboard Shortcuts (*Scale up/down subtitles
  text*, and the new *Zoom the interface in/out* in the Interface group), which can now take a turn of the wheel
  with Ctrl, Alt or Shift. The size stepper of the subtitles panel uses the same steps, and ASS/SSA subtitles now
  follow the size while they play. On Android: Ctrl + mouse wheel over the video, Ctrl+0, and a *Subtitle size*
  slider with Reset in the subtitles tab (ASS/SSA keep their size there).
- **Every window and popup has a border like the main window.** Dialogs (Restore the play queue, confirmations,
  Go to Time, Media Statistics, Plugins, Folder information, What's new, Settings, Keyboard Shortcuts, the welcome),
  menus, the panels that open from the player's buttons (tracks and subtitles, the queue card, cast, sleep timer,
  speed, effects), dropdown lists, tooltips and the seek preview now have the same thin 1 px frame as the main
  window, so they stand out from the dimmed interface instead of melting into it. The classic windows, the
  right-click menus and the "VLC could not start" message take the same colour. It is the grey Windows 11 draws
  around the main window (`#4C4A48`, in the dark and the light theme), now set on every window so that the frame
  looks the same whatever the Windows colour settings. On Linux (X11 and Wayland), where the system draws no frame
  around VLC's own title bar, the main window and the menus draw the same border themselves.
- **Sharp icons at every size.** Every icon of the interface is drawn from an SVG of the design, at the size it
  is shown: sharp at 100, 125, 150 and 200 % Windows scaling and when the interface is zoomed (Ctrl + mouse wheel),
  where they were blurry or stayed small. The player bar's buttons now grow with the zoom like the rest. Some icons
  are a little lighter or heavier than before: they now match the Figma design. The app icon (`vlc.exe`, the
  taskbar, the title bar, the tray) has every size Windows asks for.
- **Every message and question looks like the rest of VLC Redesign.** The ones that were plain Windows boxes are
  drawn as the redesign's dialogs (title bar with its close button, heading, the Cancel button on the left and the
  action in the accent on the right), in the window when it shows:
  - after VLC closed unexpectedly: *Restore the play queue?* ("VLC Redesign closed unexpectedly on 28 October 2026
    at 10:16. Restore the play queue it had?") with *Restore* and *Don't restore*, instead of "The application
    closed abruptly" and Yes / No;
  - the confirmations of the library (delete a file, a playlist, remove from a playlist, discard the filtered out
    items) name their action (*Delete*, *Remove*, *Discard*); renaming a playlist and pasting several links use
    the redesign's field;
  - the login, question and progress dialogs VLC asks for while playing (a password for a share or a stream, a
    certificate), and the error banner at the bottom;
  - the warnings of the classic windows (Media Information, Messages, Convert profiles, media library folders,
    Reset Preferences), as a frameless dialog of the same look;
  - before the interface exists: "VLC Redesign could not start" (invalid options, no plugins) and the Windows
    version check, in a small window of the same design, dark or light as Windows is.
- **The classic windows have the redesign's title bar** (Open, Convert, Media Information, Messages, Bookmarks,
  Advanced preferences, VLM, Program Guide, extensions…): the dark bar of the dialogs with the title in Inter and
  the redesign's icons, minimize and maximize only where the window has them. Moving, snapping, double-click to
  maximize, resizing from every edge, the shadow and the rounded corners of Windows 11 work as before.
- **Go to Time, Plugins and extensions, Media Statistics and Folder information are redrawn** as dialogs of the
  redesign, opened from the same places:
  - *Go to Time* (Ctrl+T) takes a time as the control bar does (1:02:10, 62:10 or 130), with *Start* to go back to
    the beginning;
  - *Plugins and extensions* lists the modules, searchable and sorted by name, capability or score (their
    description on hover), and the Lua extensions with their version, author and link;
  - *Media Statistics*, new in the Tools menu: input, video and audio live, with a graph of the input bitrate over
    the last two minutes, and a button to turn statistics on when they're off;
  - *Folder information* (Info in a folder's menu, which only opened File Explorer): where the folder is, its date
    and size, and how many files, videos, music, subtitles and folders it holds.

- **macOS: one title bar.** On a Mac the window had two bars, the system's title bar and VLC Redesign's own
  below it. Now there is one: the red, yellow and green buttons sit at the start of VLC Redesign's top bar, as in
  other Mac apps, and the window keeps everything macOS gives it (full screen in its own Space, tiling, resizing
  from every edge, its shadow). Drag the top bar to move the window, double-click it to zoom. *Show window
  decorations* in Settings, Presentation brings the separate title bar back.

### Fixed

- **Icons sharp at 125, 150 and 200 %.** Every icon of the redesign was drawn for 100 % and stretched at a higher
  Windows scaling (the 1.5 px lines of the queue icon came out 76 % strong over four to six pixels at 200 %): Qt
  draws an image at the screen's density only when the name written in the interface ends in `.svg`, and the icons
  were still named by the PNG files they once were. Now named by their SVG: at 200 % the lines are three pixels,
  two of them full, as the design draws them; at 100 % they match Figma (1.5 px, `#737373` on the dark bar,
  `#4D4D4D` on the light one).
- **Icons as crisp at 125 and 150 % as Qt draws them.** At these scalings an icon whose place falls between two
  screen pixels (a logical pixel is 1.25 or 1.5 of them) was blurred by half a pixel: the queue icon's lines at
  150 % read `32 81 81 32` instead of the `63 100 63` of the drawing. The icons drawn at their size are now copied
  pixel for pixel wherever they sit (no filtering when the size divides into whole screen pixels), without moving
  them; 100, 175 and 200 % unchanged. The Settings sidebar icons and the crop panel's link icon were measured
  against Figma at 100 %: the same amount of ink (not heavier).
- **Cast panel: the arrows no longer stop at *Show the video here too*.** Down from it goes round to the first
  row, Up goes back up the list; opened from the keyboard, the panel puts the keyboard on its first row.
- **Live TV on a DLNA TV: one stream per channel, with its picture and its sound.** A channel restarting its
  picture a few seconds in (after an advert, or the adaptive stream changing its format) made a second stream,
  which the TV loaded again; now the new picture goes on in the same stream. A cast started on a channel already
  playing made a stream with the picture or the sound alone, then another with both (or sent the sound only):
  the first stream of a channel now waits up to 3 s for both.
- **Settings, Audio: *Maximum volume* is drawn as the design draws it again.** Since the slider was made to cover
  VLC's whole range (60 to 300 %), its scale started at 60 % under a label reading 100 %, and 200 % stood right of
  the middle. The slider keeps the design's 100 to 300 % and widens only for a value set below 100 % elsewhere (the
  same for the OSD duration). Found by the final 1:1 check against the Figma frames (Settings - audio, 3073:15879).
- **Online subtitles are loaded by themselves only for the right film.** The automatic search took the title it
  guessed from the file name: a clip named `silent.mp4` got the subtitles of *Silent Hill*. A subtitle now loads by
  itself only when it was made for that very file (the services know it by its fingerprint) or the title is sure:
  one you chose, or the same title with its year, its episode, or of two words or more. A guess from a name alone
  stays in the subtitles panel's list, to pick if it is right.
- **Live TV leaves nothing in the library, also for channels behind a playlist.** A channel whose address is
  itself a playlist (some Plex channels) was saved to the library under the address it pointed to, and showed in
  *Continue Watching* named by that address's query (its access token). What a channel opens now stays out of the
  library and the history, as the channel itself.
- **Screen readers and the keyboard.** Every button and field of the main window, the player bar, its panels, the ⋯
  menu, the library, the TV page and the dialogs has a name (the Settings search field had none); the redesign's
  dialogs are announced by their title (they were unnamed windows); and Tab no longer stays in the multi-line fields
  of *Open Network Stream* and Live TV's sources: Tab and Shift+Tab move to the next and the previous control.
- **Live TV: *All* counts the channels shown.** The *All* group read 5,891 while the page and its list had 5,881:
  it also counted the adult channels, which only show when *Settings › TV* shows them. *All* and the groups now
  count what the list shows, and follow that switch at once.
- **Casting a live TV channel crashed VLC.** A live channel (or any HLS or DASH stream) cast to a Chromecast crashed
  VLC when it ended: changing channel while casting, stopping the cast, quitting. Zapping while casting now works,
  and a channel that restarts its picture after an advert makes the TV reload it once, not twice in a second
  (once with the sound only).
- **Quitting while casting took 16 seconds** and left the Chromecast playing on its own. VLC now quits in about
  2 s and the TV goes back to its idle screen; a playlist cast to its end still shows each film to its last picture.
- **The cast list keeps looking while it is open**: a TV turned on shows up, one turned off goes (it searched for
  20 s only, then showed what it found until it was opened again). After a few seconds with no device it says so.
  A device hidden from the list stays hidden when the network finds it under its short name (a Chromecast is named
  with and without its model). With *Discover cast devices* off, VLC no longer searches by itself while a video
  plays.
- **The casting screen**: on the window's dark (or light) background instead of a flat grey, with the file name
  when the media has no title; the subtitles button stays in the bar while a video plays on the TV (the subtitles
  are chosen here and the TV follows).
- **The cast list in Hebrew and by keyboard**: a device name in Latin letters sits on the reading side with the
  Hebrew rows (it sat at the far end); the arrow keys move from row to row; *Show the video here too* is greyed
  while a Chromecast plays (it has no picture here).

- **Live TV: Page Up, Page Down, the numbers and Backspace change the channel.** VLC's own shortcuts got these
  keys first: Page Up restarted the stream instead of going to the next channel. While a channel plays, the keys
  of the TV now come first; every other key keeps its shortcut, and outside Live TV nothing changes. The keys of
  the channels (next, previous, last channel, channel list, banner) are in **Settings › Shortcuts**, group *Live
  TV*, and can be changed there.
- **Backspace no longer closes VLC** when the player was opened with a file and a message (such as *Restore the
  play queue?*) was over it: the player's "back" went back from a history that was still empty.
- **Hebrew and Arabic: a text not yet translated keeps its punctuation at its end.** *Advanced options...* showed
  its dots first, a label its colon first; such texts now read left to right inside the right-to-left layout.
- **Open Media › Capture Device: the video and audio device lists line up.**
- **Live TV guide: no air date where the episode goes.** Pluto TV's guide showed *19990101080000 +0000* under its
  programmes; the episode (*S17E52*) shows now.
- **No QML error when *Open File(s)* or *Open Folder* is chosen from the ⋯ menu** (seen at zoom 2): the system's
  dialog opens once the menu has closed.
- **No QML warning when Customize Player opens at zoom 1.5 or 2** (`QQuickItem::stackAfter: Cannot stack ...`):
  the lines drawn above the rows of the player bar while it is customized are made inside the bar's background.
- **The classic windows are translated again, and Live TV can be.** In every language, the texts of the classic
  windows read English (Open Media's *Capture mode*, *Device Selection*, *Options*, *Show more options*, *Advanced
  options...*, Convert, Media Information...): the list of texts to translate was made without them, so their
  translations were dropped. The texts of Live TV (the TV page, the guide, the TV sources and their catalog, the
  banner over the video), *Go to Time*, *Plugins and extensions*, *Media Statistics*, *Folder information* and
  *Edit Sidebar* never reached that list. All of them are in it now, and the new texts of 0.4.0 are translated in
  the 72 languages, the five Live TV shortcuts included.
- **No QML error when the player closes while its library panel opens** ("Object destroyed while one of its QML
  signal handlers is in progress", seen in Hebrew): the panel is made in the background.
- **The mouse wheel changes the volume again, and the volume never reads "-100 %".** With an audio output that
  has no volume of its own (the *File* and *Dummy* outputs of Settings, Audio), the wheel over the video and over
  the volume slider did nothing and the slider's value read *-100 %*. Those outputs now apply the volume to the
  sound themselves, so the wheel, the keys and the slider work from 0, and a volume not known yet shows as a dash.
- **About: the name is never cut.** *VLC Redesign* read *VLC Redesi…* (and *VLC …* in German); the update button
  now goes under the name when a translation needs the room. The credit lines wrap as sentences with their links,
  and the system badge is readable in the light theme.
- **Buttons no longer melt into their dialog.** *Cancel*, *Close*, *Licenses & authors*, *GitHub*, *Don't
  restore*, the Settings buttons (*Browse*, *Reset*…) and the classic windows' buttons drew in the colour of the
  panel under them; they have the window's frame colour around them now, in both themes. Icon buttons stay as
  they are. In the light theme the classic windows' selected tabs, rows and main buttons write their text in a
  darker accent, readable on the pale tint.
- **Subtitles in every language read right, whatever their encoding.** A subtitle file saved in an old encoding
  (Hebrew Windows-1255, Arabic 1256, Russian 1251 or KOI8, Greek, Turkish, Polish and Czech, Baltic, Vietnamese,
  Thai, Japanese Shift-JIS, Chinese GBK or Big5, Korean…) showed as gibberish unless the interface was in that same
  language. VLC now recognises the encoding by itself, from the text, the language in the file name
  (`movie.he.srt`) and your preferred subtitle language; UTF-8, UTF-16 and UTF-32 files are read as before. It
  works for SRT, MicroDVD, ASS/SSA, SAMI, WebVTT, the text tracks inside videos and the subtitle lines on the seek
  bar, on every system. *Settings › Subtitles › Encoding* still forces one when you choose it. A character that
  cannot be read shows as `�`, never as a wrong letter.
- **Hebrew and Arabic ASS/SSA subtitles in the right order.** Their full stops, question marks, brackets and
  numbers showed on the wrong side of the line (ASS subtitles were laid out left to right, as the old VSFilter did);
  they now read as SRT subtitles always did.
- **Picture subtitles (DVD, Blu-ray, DVB) stay smooth** on Windows when the video is scaled with the *Point*,
  *Video processor* or *Super Resolution* upscaler: they were blocky there. Text subtitles were already drawn at the
  size of the screen and are unchanged.
- **Light theme: the classic windows' title bar matches the other dialogs.** The classic windows (the
  ones from VLC's older interface, in the redesign's look) had a darker grey title bar (`#9C9C9B`) than the
  redesign's own dialogs; it is the same light grey now (`#DEDEDE`). Nothing else looks different: the 228 places
  where a part of the interface asked for its own light colour and the general light colours won were each checked
  against the Figma *Player - Light* frame, which draws the general colour wherever it shows one (for example
  *15 items* `#6B6B6B`, the times and the folder name `#2E2E2E`), so they now ask for that colour. A light colour a
  part asks for now wins over the general ones, and `design/tools/check_qml.py` fails when one differs from them
  without being listed. The decisions, with the Figma evidence: [design/light-theme.md](design/light-theme.md).
- **The playback speed button can be added to the bar again** ([#9](https://github.com/KuchiSofts/vlc-redesign/issues/9)).
  Dropped on a bar in Customize Player, it left an empty slot that could be neither seen nor removed. The teletext
  button had the same fault. A control that ever fails to load now shows a placeholder that can be dragged out.
  Thanks to @Brackets124 for the report. Every control type was then put in the bar: the folder name after the
  thumbnail stayed hidden (the centre zone measured its room by its own shown controls), the aspect ratio box was
  empty, and the zoom icon was stretched; all three are fixed.
- **Colours of SD video on Windows.** Untagged or BT.601 SD video showed shifted colours with Direct3D 11 (pure red
  came out as (239,4,0)); it now shows as the source, as with OpenGL. HD and HDR are unchanged.
- **Subtitles in older encodings** (Windows-1255 Hebrew, 1251 Russian, 1256 Arabic, Shift-JIS…) downloaded online
  show correctly again instead of as garbage.
- **Installer:** after an all-users install or update, VLC no longer restarts as administrator, and such an install
  no longer creates one shared profile every user can write to (each user gets their own). "Install for everyone"
  keeps the command-line switches; an upgrade keeps your default-app choice working; uninstall removes every file.
- **Updates:** a failed roll back is no longer reported as success; long paths, case-only renames and "nothing to
  apply" are handled; downloads must come from this project's releases, and a signed copy only accepts updates
  signed by the same publisher.
- **Updates are signed by the project.** From 0.4.0 on, every release's checksums carry a signature by the
  project's own release key, and the app installs an update only when that signature is good: a release put on
  GitHub by anyone else (a stolen account or token) is refused with a message, never installed.
- **Casting:** a TV marked unreachable can be retried; a failure to build the stream shows a message instead of
  "connecting" for ever; one file the TV couldn't read no longer makes every later one be converted; quitting
  right after a TV disappears is much quicker; a Chromecast seen two ways no longer vanishes when one goes;
  devices can be hidden from the Cast list (right-click) and shown again.
- **Profiles and settings:** switching profile applies its hotkeys, repeat and shuffle at once; one failed write no
  longer breaks saving for the rest of the session; "Reset all settings" also resets the profiles; sliders cover
  the whole valid range (maximum volume, OSD duration); a refused value in a text field is put back.
- **Library and dialogs:** Open Network Stream saves its links into the named playlist; the folders row of Home
  works with the keyboard; "See all" on folders no longer changes the Videos grouping for good; the waveform
  follows the pointer while dragging; delete and remove always ask first; menus are mirrored in right-to-left
  languages; menus follow the interface scale.
- **Windows proxy:** LAN and localhost addresses go direct again, and exceptions with `*` work as in Windows.
- **Translations:** the 72 languages on Weblate have every text of VLC Redesign again, the settings' help of
  VLC's newer options included; about 80 wrong translations fixed where they show (Hebrew "decoding" that said
  encoding, Persian and Malay key names in English, a reversed Arabic help text…); the Cast list's "Show N hidden
  devices" has its plural forms. The Estonian and Swedish translations made on Weblate are in: thank you, Priit
  Jõerüüt and bittin. Everyone who reported or translated is credited in [CREDITS.md](CREDITS.md#community).
- **Linux:** the zoom control's list showed its levels cut ("1:..."), now their names; with the System theme on a
  desktop that tells no light or dark (no portal setting, X11 window managers) the menus and dialogs were dark over
  a light interface, now they follow it; on X11 the shadow around the window no longer catches the clicks meant
  for the windows below. Going from file to file no longer makes VLC's memory grow by about 90 MB a minute
  (360 MB to 1 GB in 8 minutes): glibc kept the freed pictures of every thread; it now stays near 480 MB.
  An AV1 file no longer makes VLC quit on Ubuntu 24.04 with a recent Intel GPU: VA-API decoding of AV1 crashed
  inside FFmpeg 6, so dav1d decodes AV1 there. On Wayland with Intel or AMD graphics the video was drawn from
  the left with no black bars, the desktop showing through beside it; it is now centred over black.
- **Smaller fixes:** idle HTTP stream clients time out (DLNA streams excepted), stopping a stalled network stream no
  longer hangs, the VU meter reads the whole signal, Lua scripts saved with a BOM load, a TS stream's clock no
  longer jumps when a track is removed, and more (the full list: the review report).

### Technical

- Merged VLC master up to `12023233ef` (2026-10-29), 4 commits: mpg123 1.33.7 in the contribs (and its FPU code on
  every ARM64), the Qt build's moc include folders (`static_dirs.py`) and a missing medialibrary header in
  `Makefile.am`. Nothing of the redesign needed a change; built and tested again (tours, subtitles, Live TV).
- **The icons ship as SVG only.** The 303 PNG files of `pixmaps/redesign` (every one had its SVG, of the same
  size) are gone from the app: the icon interceptor and the image provider go by the SVG alone, the `.png` names
  stay the icons' ids. The PNG exports of Figma are kept in `design/icons`, where `sync_assets.py` and
  `icon_svgs.py` read them.
- **The source of every release.** `make-release.ps1` writes `vlc-redesign-X.Y.Z-source.tar.gz` (the commit
  packaged, `git archive`), lists it in the signed `SHA256SUMS.txt` and stops if the tree has changes or the tag
  names another commit; the release notes link it, the portable zip says where it is, and
  [SOURCE.md](docs/SOURCE.md) has the libraries of `contrib/` and the written offer for their source.
- **`DEMUX_GET_LENGTH` with its second argument.** Since upstream's d3a2408afa the adaptive demux writes the live
  state through a second `bool *` whether the caller passed one or not. The Chromecast demux filter (and the chained
  demux) asked with one argument: the bool went to whatever followed, the filter's own logger pointer, and the
  input crashed in `module_unneed()`'s log at its end (found with cdb: the filter's logger read as `0xabababab`;
  a hardware write breakpoint showed `PlaylistManager::doControl` writing it). Both now pass it.
- **Chromecast output: held again after a stream restart.** When the picture or the sound is removed while the
  other stays (an ES restart), the blocks wait as at the start of a media until both are back (`held_again`): one
  new chain, one `LOAD`. The end of the media plays to the end only after the input's EOF (`cc_eof`): at quit
  the interface goes before the input and could not say that the user stopped.
- **Renderer list**: `StartScan()` clears what the previous discoveries found (`ItemEntry::copies`: they do not
  remove their items when they close), so a device a new discovery loses goes during the scan; `isDevice()`
  matches a hidden name either way round (with or without the model in parentheses); `prepare()` honours
  `redesign/discover-cast-devices`.

- **`po/vlc.pot` with the `.ui` forms and every file of the redesign.** xgettext cannot read Qt Designer forms;
  VideoLAN's `vlc.pot` takes their texts from the `ui_*.h` of a build. `extras/juxtopposed/translations/ui-pot.py`
  writes them as a POT of their own (references to the `.ui` file and line), merged with xgettext's by
  `msgcat --use-first`: 375 texts of 22 forms, whose translations `msgmerge` brings back from the catalogs'
  obsolete entries. `po/POTFILES.in` lists the 37 files with translatable texts that VLC Redesign added or
  changed and that were missing (Live TV, `SettingsTv.qml`, the redesigned dialogs, `SidebarEditDialog.qml`,
  `navigationmodel.cpp`, `modules/misc/iptv/iptv.cpp`...). A `qsTr()` of two concatenated literals is one literal
  (xgettext only reads the first). `po/vlc.pot`: 8,290 texts.
- **Keys before the hotkeys** (`MainCtx.keysBeforeHotkeys`): an item of the interface may take keys before VLC's
  hotkeys and the focused item; the window's event filter asks its `takeKey(key, modifiers)` (done there) and
  `wantsKey(key, modifiers)` (sent on to the items). Live TV over the video is that item while a channel plays.
  Its keys are options of the Qt interface (`qt-key-tv-channel-up`, `-channel-down`, `-previous`, `-list`,
  `-info`, hidden in the classic preferences), listed by the shortcuts model in their own group, without a global
  key, in conflict only with one another. `NavigationHistory::previous()` returns on an empty history.
- **Untranslated texts in a right-to-left interface**: the Qt translator puts a left-to-right mark at both ends of
  a text gettext returned untranslated when it starts or ends with punctuation; the `.ui` texts go through it
  (`Q_` is `QObject::tr` in `Makefile.am`, as uic does for meson).
- **Encoding detection of text subtitles** (`modules/codec/subs_charset.c`): a byte order mark, UTF-16 without
  one, valid UTF-8, else the legacy encoding whose decoding reads most like text (letters of the encoding's script,
  one script per word, letter case, marks after letters, Hebrew final letters, Russian and Greek letter
  frequencies, Thai vowel order, the frequent rows of the CJK encodings, no control characters or undefined bytes),
  the languages of the file name, the track and `sub-language` and the interface code page breaking ties. The
  subtitle demux and the WebVTT demux convert whole files to UTF-8 (`subs_charset_LoadUTF8`, so libass gets UTF-8
  too), subsdec detects the packets of a track that are not UTF-8 (a Matroska "UTF-8" track in Windows-1255), the
  seek bar's cue reader uses the same detection. Test: `extras/juxtopposed/subtitle-charset-test.py` over the corpus
  of `extras/juxtopposed/fixtures/subtitles` (28 encodings, 19 languages and mixed lines, BOM or not, CRLF or LF, five formats,
  files without a language in their name, one-line cues): 212/212 decoded right with glibc iconv (Linux) and with
  GNU libiconv as VLC uses it on Windows.
- **libass: base direction from the text** (style Encoding -1, `ASS_FEATURE_WHOLE_TEXT_LAYOUT`,
  `ASS_FEATURE_BIDI_BRACKETS`): right-to-left lines follow the Unicode bidi algorithm, as freetype does;
  left-to-right lines are unchanged.
- **direct3d11: subpicture regions get a bilinear sampler of their own**; they were drawn with the sampler the video
  quad left bound (a point one with the point, processor and super resolution upscalers). Measured on captures:
  text subtitles are pixel for pixel the same (they are rasterised at the display size and drawn 1:1), PGS drawn
  with `--d3d11-upscale-mode=point` now matches the linear mode.
- **Every upload checked for the whole project.** `extras/juxtopposed/release/release-check.py` checks the version,
  the changelog, the roadmap and the plan, the README, the platforms and the website, the wiki, the design tokens,
  the translations, VideoLAN's master, the credits and every open issue; the pre-push hook runs it for every push
  to this repository (a `vX.Y.Z` tag as the release of X.Y.Z) and `make-release.ps1` before packaging.
  [docs/RELEASE-CHECKLIST.md](docs/RELEASE-CHECKLIST.md) lists the steps around it.
- **macOS port started.** `extras/juxtopposed/macos-build.sh` builds VLC Redesign on a Mac: VideoLAN's prebuilt
  macOS contribs of the newest upstream contrib commit (from source when none), the official Qt 6.8.3 by
  aqtinstall (upstream's macOS contribs have no Qt), VLC configured by upstream's `configure.sh` with
  `--disable-macosx --enable-qt` (the native interface has the higher priority) and libcurl of macOS, then
  `VLC Redesign.app` (`org.kuchisofts.vlcredesign`, macOS 12+, Qt deployed by `macdeployqt`, every library found
  inside the bundle, the icon drawn from the redesign's logo) and `vlc-redesign-X.Y.Z-macos-<arch>.dmg`. It signs
  ad hoc, or with a Developer ID, the hardened runtime and notarization when given one (decision 3).
  `make-info-plist.py` takes upstream's file types, imported rather than exported, with the rank *Alternate*.
  The workflow `build-macos.yml` builds both architectures on every pull request (`macos-14`, `macos-15-intel`:
  GitHub retired `macos-13`) and runs `ci/macos-smoke.sh` (headless playback, the Qt interface for 10 s, no crash,
  no QML warning); it doesn't block a pull request yet. On macOS the window has one unified title bar (below), a
  minimal menu bar sits at the top of the screen, ⌘ reaches VLC's hotkeys as `Command+` (they never
  matched with the Qt interface), CJK text takes the fonts of the interface language, and the updater never
  offers a Windows file outside Windows. The analysis and what is left: `extras/juxtopposed/macos/README.md`.
  Both architectures built and passed the launch test on GitHub's Macs (28 October).
- **Windows on ARM64 build.** `win64-docker.ps1 -Arch aarch64` builds the redesign for ARM64 Windows in VideoLAN's
  llvm-mingw image (`winarm64.Dockerfile`, `vlc-debian-llvm-ucrt:20260611225331`, the image of upstream's
  `win64-arm-llvm` job) with that job's prebuilt contribs (Qt included), UCRT and the Windows 10 API
  (`build.sh -a aarch64 -u -S 0x0A000006`), in the volume `vlc-winarm64-build`, to
  `~\source\vlc-redesign-build-arm64`; a full build takes about 9 minutes here. The redesign's code built for
  ARM64 with clang unchanged; the build scripts no longer assume x86_64 (`win64-docker.sh` reads `ARCH`: build
  and contrib folders, the GCC-only `-lgcc_s` fix, the updater helper built with the toolchain's compiler), a
  git worktree builds too (its main `.git` is mounted), `-Jobs` limits the compile jobs, and a release-mode build
  (`-r`, x64 too) no longer fails at its end on upstream's zip of a folder `-o` never writes. Every `.exe` and
  `.dll` of the package is ARM64 (PE machine `0xAA64`); it has no OpenGL plugins (upstream's ARM64 contribs have
  no OpenGL: video goes through Direct3D 11) and no plugin cache when made on an x64 PC (the copy makes it at its
  first start; an install for everyone makes it during Setup). `make-release.ps1 -Arch aarch64` makes
  `vlc-redesign-X.Y.Z-winarm64.zip` (80 MB for 0.4.0; x64 0.3.1: 107 MB), `vlc-redesign-X.Y.Z-winarm64-setup.exe`
  (45 MB; x64: 59 MB) (the same `.iss` with `/DArm64`:
  `ArchitecturesAllowed=arm64`, the same AppId) and `vlc-redesign-X.Y.Z-winarm64-delta-from-<old>.vrdelta`
  (`make-delta.py --platform winarm64`); `SHA256SUMS.txt` lists the files of both architectures of the version,
  signed as before. The app's updater takes the installer and the small update of its own architecture only (an
  x64 copy keeps the names it always used). The Windows build workflow builds the ARM64 package and runs
  `ci/arm64-smoke.ps1` on a `windows-11-arm` runner (the binaries, the plugin cache, then the launch test); it has
  not run yet, and no ARM64 PC has run the package.
- **macOS unified title bar.** `maininterface/macos_titlebar.cpp` keeps the main window's system frame and makes
  its title bar transparent and untitled, the content under it (`NSWindowStyleMaskFullSizeContentView`), through
  the Objective-C runtime (Qt 6.8 has no flag for it; the Qt plugin links `libobjc` on macOS). The content keeps its
  place on the screen when the style changes, so a saved geometry never grows by a title bar. The top bars (library,
  player, menu bar) start after the traffic lights (`MainCtx.windowButtonsInsetLeft/Right`, read from AppKit, on
  the right in a right to left system language); the redesign's own window buttons are not made on macOS and the
  system draws the resize edges, shadow and border. `qt-titlebar` is now off by default on macOS too. Compiled
  for macOS only for syntax here (Qt 6.8.3 headers, both branches of `objc_msgSend`); the first run on a Mac is the
  public CI at the upload.
- **macOS app and launch test complete for 0.4.0.** `macos-build.sh` requires the Live TV add-on in the app (`-t`
  builds without it; a disk image has no components, Settings turns Live TV off), at least 20 translations, and
  puts the README, licences, credits (Juxtopposed) and the fonts' OFL in `Resources` as the Windows package does;
  it verifies every Mach-O of the bundle is signed. `ci/macos-smoke.sh` also checks the signatures one by one, the
  icon, the translations, the Live TV plugin (loaded: `--list`), the About badge of the processor (*macOS Apple
  Silicon* / *macOS Intel*), the credits and the unified title bar, and captures the main window light and dark
  and About. `build-macos.yml` runs on every pull request and push to main of the public repository (free), and on
  a private one only by hand, Apple Silicon only, within 40 minutes (macOS minutes count ten times there: about
  150-250 of the free 2,000 a month).
- **Windows ARM64 package checks.** `ci/pe-check.py` checks a package without running it: every `.exe` and
  `.dll` has the package's PE machine, every imported DLL (delay-loaded too) is in the package or in Windows, Qt
  and its QML are built into the interface, the About badge, the Live TV add-on, the translations, the credits and
  licences. On the 0.4.0 integration build: the 399 binaries ARM64, every import found (Vulkan's loader comes with
  a graphics driver, as for x64); the x64 build passes too. The Windows build runs it on the ARM64 package. A new
  job `install-test-arm64` makes the ARM64 installer and runs `ci/arm64-install-test.ps1` on `windows-11-arm`:
  silent install, every file of the package installed, the ARM64 smoke test on the installed copy (plugin cache,
  window, a clip played silently), screenshots of the main window and About, uninstall. The `windows-11-arm` jobs
  run on public repositories only, where they are free.
- **Platforms in one file, and a website.** `platforms.yml` is the single source of every platform (status, latest
  version and date, the next version, the files of a release, its page). `extras/juxtopposed/platforms/render.py`
  (standard library only, with a reader for the file's YAML) writes the platform bar and table at the top of the
  README (between `<!-- platforms:start -->` and `<!-- platforms:end -->`), `site/platforms.json` and the platform
  list and download box of `site/index.html`; `--check` fails when they are out of date, `--also-on <id>` prints
  the line `release-notes.py` now ends the notes with, `--build DIR` adds the screenshots of `docs/media` for
  publishing. `site/` is the website: plain HTML, CSS and JavaScript in the app's colours and Inter (a 40 KB
  Latin subset of the app's font), light and dark from the system, the download for the visitor's system (Windows
  on ARM through the client hints; `?os=` to preview another), the design credit, every platform and the project's
  links; no cookies, trackers or requests to other sites. `.github/workflows/platforms.yml` checks pull requests,
  renders on `main` and deploys `site/` to GitHub Pages. About shows *Also on: …* (the other platforms) from
  `site/platforms.json`, built in as `:/redesign/platforms.json` (`RedesignUpdater.alsoOn`): nothing is fetched.
- **Design tokens, icons and fonts in `design/` (`design-1.0`)**, for the other platforms. `design/tokens.json`
  (W3C Design Tokens format, 203 tokens) holds the colours of both themes, the 9 accents and their Dim/Bright
  rule, fonts and line metrics, sizes, radii, icon boxes, shadows, opacities and window sizes, each with its Figma
  node and the places in the code that hold it. `design/icons/` has every icon the app ships (614 files: the Icon
  set's PNG per state with `@2x` and their SVG sources, the SVG and two-tone icons) with `index.json` and a preview; `design/fonts/` the
  modified Inter, DM Mono and their OFL licenses. `design/tools/export.py` writes `design/build/tokens.css`,
  `colors.xml` (Android) and `Tokens.swift`; `sync_assets.py` copies the icons and fonts from the app.
  `design/tools/check_qml.py` (a *Design tokens* job in the Windows build workflow, on every pull request) fails
  when a value in `VLCStyle.qml`, the menu and widget styles or the QML components differs from `tokens.json`
  (205 places), when the light colour map or the accents do not match, or when `design/` is stale.
- **Every icon from an SVG of the design.** Each raster of `pixmaps/redesign` now has its SVG next to it (368):
  the 224 states of the 116 glyphs of the Figma Icon set (42:356), cut from Figma's SVG export of the set by
  `design/tools/icon_svgs.py` (the symbol of the icon's name, the part a two-tone icon shows, Figma's colours of
  the state), and the 41 other pieces (player and window buttons, track, volume and list/grid icons, onboarding
  masks, window corners), each from its Figma node (`design/tools/raster_svgs.json`). Boolean shapes Figma's export
  draws as their outline (save, subtitle, video, window) take the outline Figma fills; cast, absent from the file,
  is drawn after the app's picture. [`design/tools/icon_svgs_report.md`](design/tools/icon_svgs_report.md) compares
  each glyph with its PNG and with Figma's render (51 states are Figma's render, 131 the same drawing, 42 close: the
  PNG was drawn heavier or lighter than Figma, the SVG follows Figma). `icon_svgs.py register` lists them in
  `assets.qrc` and `resources.am`; `design/icons/` carries them. The URL interceptor sends every icon to its SVG
  (the light theme through its provider, with an `.svg` id so Qt asks the size times the device pixel ratio), the
  images showing one draw it at their `sourceSize` (now set from the size shown on the 38 that had none or a fixed
  small one, `Impl.IconImage` and `Impl.ColorImage` included), and the menus, window buttons and message boxes
  draw them through an icon engine (`RedesignIcons`) at each size and scale, cached per size. The PNGs stay as a
  fallback. The unused `grid.svg` (a line) is `line-vertical.svg`.
- **App icon at every size.** `share/icons/vlc.ico` has 16, 20, 24, 32, 40, 48, 64, 96, 128 and 256 px, each
  rendered from the vector (`share/icons/scalable/vlc.svg`, `extras/juxtopposed/make-app-icon.py`); the window,
  taskbar and tray icons take the SVG; the installer's `setup.ico` draws each size at its own size (96 added).
- **Signed checksums (security).** Releases have `SHA256SUMS.txt.sig`: an ECDSA P-256 signature (SHA-256, raw
  r‖s, 64 bytes) of `SHA256SUMS.txt`. The updater downloads it with the checksums and checks it with Windows' CNG
  (`BCryptImportKeyPair` / `BCryptVerifySignature`, `bcrypt.dll` loaded from System32) against the public key in
  `modules/gui/qt/util/redesign_update_key.h` before it reads any checksum. For a release at or after
  `FIRST_SIGNED_VERSION` (0.4.0) a missing or bad signature refuses the update, and the signed checksums must
  name files of that very version; older targets keep the earlier checks. The same-publisher Authenticode check
  stays. `extras/juxtopposed/signing/release-key.ps1` makes the key (`new`; kept DPAPI-encrypted for the
  release PC's user in `%USERPROFILE%\source\vlc-redesign-keys`, outside every repository), signs, verifies and
  exports a backup; `make-release.ps1` signs `SHA256SUMS.txt` (`-NoSumsSignature` in the GitHub workflow, which
  never has the key). `.gitignore` and the pre-push hook refuse key files. `signing/verify-test.c` tests the
  verifier as the app compiles it.
- The button, the placeholder and the high resolution time used the `Popup` and `Control` types without importing
  them (upstream dropped `QtQuick.Controls`); the static Qt of Windows doesn't know them. They come from
  `QtQuick.Controls.Basic` now.
- A two-round code review (105 findings) was checked finding by finding against the code: about 90 were fixed,
  the rest were already fixed, not bugs, or only partly fixable (for instance updates still have no signature of
  their own while the releases aren't code-signed). The build now fails on any `build.sh` error and checks the
  package; the weekly upstream merge gets the Windows build; `.gitattributes` keeps scripts in LF; the minimum Qt
  is 6.8, as the code needs; the Linux and macOS ports get the HTTP backend (libcurl), code-page decoding and the
  language picker.
- Messages: `DialogsProvider::showRedesignMessage()` / `execRedesignMessage()` show a message in the window
  (`RedesignMessageDialog.qml`, hosted by `SetupDialogs.qml`) or, without the interface or while it is hidden, as
  `RedesignMessageBox` (frameless widgets in `RedesignDialog`'s sizes and colours). `questionDialog()`,
  `messageDialog()` and `getTextDialog()` go through it and keep their answers. Before Qt, `bin/winvlc_message.c`
  draws the same design with GDI and DWM (linked with `dwmapi` and `gdi32`). The extension and fingerprint windows
  get `RedesignWidgetTheme`.
- Title bars: `util/redesign_window_frame.cpp`, installed by `RedesignWidgetTheme` on every classic window: a
  widget bar over the window's top contents margin; on Windows a native event filter takes the caption out of the
  frame (`WM_NCCALCSIZE`) and hit-tests the bar as the caption (`WM_NCHITTEST`), as `CSDWin32EventHandler` does for
  the main window. Other systems keep their title bar.
- Tool windows: `dialogs/dialogs/redesign_tools.cpp` (`MediaStatistics`, `PluginListModel`, `ExtensionInfoModel`,
  `FolderInformation`), the dialogs in `SetupDialogs.qml`; `DialogsProvider::gotoTimeDialog()`, `pluginDialog()`,
  `statisticsDialog()` and `folderInfoDialog()` open the classic windows only without the interface (skins).
- **Linux build of the current code** (x86_64, Qt 6.8.3, Docker, `extras/juxtopposed/linux-qt68.sh`): builds with
  no warning from the redesign's code; `linux-bundle.sh` makes a self-contained tarball (VLC, Qt 6.8.3 and the
  libraries of Ubuntu 24.04 it needs) for test PCs, `linux-wslg.sh` runs it on the Windows desktop through WSLg.
  Every control of the player bar (59) was put in the bar, shown and clicked during playback: no QML warning.
- **Linux packages.** `extras/juxtopposed/linux-packages.sh [--arch x86_64|aarch64]` makes from the bundle an
  AppImage (`vlc-redesign-X.Y.Z-ARCH.AppImage`, with update information for the GitHub releases and its `.zsync`),
  a .deb (`vlc-redesign_X.Y.Z_amd64.deb`), an .rpm (`vlc-redesign-X.Y.Z-1.x86_64.rpm`), the versioned tarball,
  `SHA256SUMS-linux-ARCH.txt` and a Flatpak manifest (`org.kuchisofts.VLCRedesign`, KDE 6.8 runtime, written only),
  with appimagetool 1.9.1 and nfpm 2.47.0 checked by SHA-256. The .deb and .rpm install VLC in `/opt/vlc-redesign`
  with the command `vlc-redesign`, a desktop entry with VLC's media types, AppStream metainfo and the logo of
  `design/icons` in the hicolor sizes, so they install next to the distribution's `vlc`; their dependencies come
  from the libraries the bundle takes from the system (glibc 2.38, GL/EGL/GBM/DRM, VA-API, VDPAU). For 0.4.0 on
  x86_64: AppImage 165 MB, .deb and .rpm 155 MB. The bundle no longer has Qt's SQL drivers for database servers
  (their client libraries were missing), is named after its architecture, and `linux-qt68.Dockerfile` takes Qt's
  ARM64 build on aarch64. A new workflow (`build-linux.yml`) builds on x86_64 and ARM64 runners and tests the
  packages on a clean runner: the .deb installed with apt next to the distribution's VLC, the AppImage, and the
  .rpm in Fedora 44, each printing its version and playing a clip to its end without a window or sound.
- Merged VLC master up to `6836ae77ae` (2026-10-27), 110 commits ([#8](https://github.com/KuchiSofts/vlc-redesign/issues/8)):
  upstream's own fix for stopping the Windows audio output under its lock, the playlist recovery question shown
  without blocking, and the rest of upstream's work.

---

## [0.3.1] · 2026-10-25

Panels that open above their button, subtitle languages that learn, a Languages page in Setup, and a round of
fixes found by documenting every option.

### Added

- Merged VLC master up to `32b160d9e9` (2026-10-28), 9 more commits: VideoLAN fixed the same missing imports of
  #9 with `QtQuick.Templates` and added a *broken control* indicator to `Fallback.qml`; their fix replaces ours
  (our own fallback switch in `ControlRepeater` removed, our hidden-control fix kept).
- **Subtitle languages that learn.** Each subtitle you pick counts for its language. The languages panel lists your
  default and second language, then the ones you pick, the most picked first (marked *Often used*), then the
  video's own. A language you pick about three times is then chosen by itself after your default and second ones:
  VLC shows its track, and the online search looks for it. Older choices fade, so recent habits count most.
- **Setup asks for the languages.** A new *Languages* page: the language of the interface (English, British English and the 72 others) and the default
  language of the subtitles, which follows the interface's until you choose it apart. Right to left, the layout
  toggle is there too. A silent install takes `/UILANG=` and `/SUBLANG=` (for instance `/UILANG=he /SUBLANG=en`).

### Changed

- **Effects, VLC Settings and HDR open above their button**, as the languages and tracks do: a click outside
  closes them, and so does the button, and opening one closes the other. The bar stays while one is open, and
  the video isn't dimmed. The menu and the keyboard shortcuts still open them in the middle of the window.

### Fixed

- **"Subtitl…"**: the name of a tab of the languages and tracks panel was cut short even with room for it.
- **A button removed from the bar can come back.** Customize Player's palette now also offers the bar's own controls
  (Play, Previous, Next, Volume, Languages and tracks, Effects, Play Queue, Fullscreen, A-B Loop, the frame steps and
  the menu), after the design's items: once removed, they could not be added again.
- **Settings that said more or less than they did**: an empty HTTP proxy now uses Windows' proxy, as its field says;
  "Apply to files opened from the file manager" is no longer dimmed (it works alone); choosing a snapshot width no
  longer stretches the picture, and the height's label says that 0 keeps the proportions; "Give VLC real-time
  priority" is named as what it does, *Increase the priority of the process*; *Stop the screensaver* keeps its
  *When fullscreen* choice; the click on the video offers one *Play / Pause* (there were two that did the same).
- **The title of a song stays readable on a bright cover**: the design's grey on dark artwork, near white or dark
  on the rest, measured behind the title.
- **The effects button is named Effects**: it opens the audio and the video effects.
- **The library's title bar kept the last song's name** after playback stopped; it shows *VLC Media Player* again.
- **Windows paths written with "/"** ("C:/Videos/film.mkv", as some apps and scripts pass them) were refused by
  VLC's core; "/" is a separator on Windows too.
- **The welcome's subtitle and online steps** showed an empty player frame behind them; they are drawn on the
  dialog, as the shortcuts step.
- **A log file written with *Log to a file* keeps its last lines when VLC crashes.** Windows wrote the log in blocks
  of 4 KB, so a crash lost the lines that say where it happened; each line is now written at once.

### Technical

- The launch test on GitHub keeps a crash dump of `vlc.exe` (Windows Error Reporting) and uploads it with the logs.
  It crashed once at shutdown on GitHub's runner (exit code `0xC000041D`, Windows Server 2022 without a GPU), and
  passed on the next attempt; 110 runs on this PC and in Windows Sandbox didn't crash.

## [0.3.0] · 2026-10-25

Every language at once (right to left too), surround sound, casting to a TV, memory over long sessions, and the
file name of the player bar.

### Fixed

- **No more "restart VLC" after changing a setting**: 21 of the 24 settings that applied only after a restart apply
  when the settings are saved: the log (on, file, format, details), the web remote (on, port, password, address),
  Last.fm, the keyboard shortcuts and the global hotkeys, the tray icon, the track-change popup, the file name in
  the title bar, the window opacity, pausing when minimized, resizing the window to the video, and the default
  playback speed (also for what plays). The output of HDR applies to the next media. Three need VLC to start
  again, as they decide what it is made of (extra command-line options, a skin, the media library), and say so.
  Settings that needed a restart without saying so apply at once too (the wheel modes, start paused, pause when
  another app plays sound, pause on the last frame), and the classic settings no longer ask for a restart after
  Reset.
- **"Give VLC real-time priority" did nothing** from the settings (VLC read it before loading them): it raises the
  priority of VLC now.
- **"Save a log" stopped after the Messages window was opened once**: the window took the log for the rest of the
  session. It takes it only while open.
- **The log file was emptied** each time it opened again: changing its details or format, closing the Messages
  window, or starting VLC wiped what it had (and the file output's *append* overwrote). It adds to the end now, as
  on other systems (an upstream VLC 4 bug of Windows, since `vlc_open()` uses CreateFileW).
- **Video late after a pause at a speed other than 1x**: at 1.5x, 2x or 0.5x (or after any change of speed), the
  picture ran as long behind the sound as the pause had lasted, dropping frames until the next seek. It stays in
  sync now (an upstream VLC 4 bug: the audio output dated its first report after the pause as if it had played
  through it).
- **Changing the speed no longer skips or freezes the picture**: the video changed speed up to a second before or
  after the sound did, dropping frames when faster and holding one when slower; pressing *faster* a few times
  quickly left the picture almost a second behind the sound until the next seek. The picture changes speed with
  the sound now (upstream VLC 4: the clock learned the new speed only from the next report of the audio output).
- **Settings changed outside the Settings window are kept**: each start put back the settings saved with the
  player profile in use, so the subtitle position and size set from the player (or Shift+Up and Shift+Down), the
  classic preferences, and anything changed before VLC ended unexpectedly, went back at the next start.
- **A language chosen after another one shows all its texts**: from Arabic to British English (or between any two
  translated languages) the interface kept the texts of the first one; only the layout changed.
- **Keyboard Shortcuts and Settings open faster**: Keyboard Shortcuts froze the window for about 140 ms as it
  opened, now about 20 ms (it read its options twice, and its sort translated group names thousands of times);
  Settings about 155 ms, now about 135 ms.
- **Casting: the picture area works with the mouse**: while a film plays on a TV, a click on the player pauses or
  plays it, a double click goes fullscreen, and the wheel changes the volume (they did nothing there).
- **Casting: subtitles change more smoothly**: taking the subtitles off no longer makes a Chromecast load the film
  again, a subtitle chosen goes into the running conversion, and the look chosen in Settings (font, colours,
  outline, shadow, bold, position) reaches the TV while the film plays. A note in the top corner (top right, right
  to left) says which subtitles are now on the TV.
- **Messages on the video in one language**: "Piste Subtitle", "Дорожка Audio" and "Audio delay" in a translated
  interface: the kind of track and of subtitles was inserted in English. Each message is a whole sentence in the
  language now. The shortcuts that make subtitles bigger and smaller also had each other's name.
- **The confirmations of Settings in the theme**: *Reset all settings* and *Clear history* opened a light box
  with OK and Cancel over the dark settings; they are the dialogs of the redesign now, with the action named.
- **Chinese in one font**: Simplified Chinese on an English (or Japanese) Windows drew the characters Windows'
  own fallback font lacked in a thinner, greyer one, in the middle of words. Chinese, Japanese and Korean take
  the fonts of their language now, whatever the language of Windows. Hindi and the other Indian scripts, Sinhala, Thai, Lao, Khmer,
  Myanmar and Ethiopic take the fonts of Windows' own interface (Nirmala UI, Leelawadee UI...): Hindi was drawn in
  a thin serif font where Office had installed Arial Unicode MS.
- **Long translations in Settings**: the two choices of the layout direction were cut in Portuguese; the choice
  widens into the row now, and a long name in the menu ("Конфиденциальность") is drawn a little smaller before
  it is cut. The list of languages names each language as it names itself ("Українська", "Slovenčina",
  "العربية"), no longer with stress marks or in lower case.
- **VLC Redesign kept files in the folders of an installed VLC**: Qt's caches (%LOCALAPPDATA%\vlc\cache) and the
  subtitles downloaded from a link of the subtitle associations (%APPDATA%\vlc\subtitles). They are in its own
  profile now, removed with it.
- **"Install for everyone" in the installer did nothing**: Inno Setup 6.7 refuses to start its own program from
  its code before installing, which that link relied on. It goes through Windows now and reopens the installer
  for all users (Program Files).
- **Long translations no longer break the interface**: texts that are longer in German, Russian or Portuguese, or
  written in other scripts, were cut in the middle or ran over the next control. They shorten with "…" and show
  in full in a tooltip, wrap where there is room, or their button, switch, tab or menu widens; checked on every page
  with a test language 40 % longer than English. The four labels of the player palette with a line break could not
  be translated at all.
- **The wheel over an open dropdown moves it by whole rows**: a notch moved the list by about half a row and it
  stopped between two. It moves one row per notch now, in a short glide, always stopping on a row, in every
  dropdown (languages, presets, audio devices, aspect ratio, zoom...).
- **5.1 and 7.1 sound jumped between speakers**: on a surround device (or a virtual one, such as SteelSeries Sonar),
  voices moved between the centre and the side or rear speakers many times a second, heard as a stutter. Every
  channel stays in its place now (checked for mono, stereo, quad, 5.1, 6.1 and 7.1 sources). 6.1 sound on a 7.1
  device also lost its rear centre channel; it plays on both rear speakers now.
- **Surround films on stereo headphones or speakers**: 6.1 sound came out with channels on the wrong sides, the
  centre of 3.0 and 4.0 sound drowned the front, and the rear speakers of quad, 4.1 and 6.0 sound were lost. Every
  layout is mixed the same way now, as 5.1 already was.
- **A file opened from Explorer while VLC played waited its turn**: double-clicking a second video added it to the
  end of the playlist instead of playing it. It plays at once now (in 0.1 s), and the window comes back in front,
  even minimized. Several files selected together played the last one: the first plays now, the others follow.
- **The first film after installing played under two questions**: Windows Firewall asked to let VLC on the network,
  and the welcome dialog covered the picture. VLC no longer opens network ports at start (the media library looked
  for network shares even with none in the library), and the welcome waits for the next time VLC opens without a
  file. Checked on a clean Windows without a graphics card (Windows Sandbox, 2 GB).
- **Opening a file never hangs, and the first one after installing is quick**: a VLC that stopped answering made
  every file opened from Explorer do nothing; the file opens anyway now. Windows checks a new program for up to 10
  seconds the first time it runs, which held the first window after an install or update: the installer has it
  checked right away, and the preparser no longer holds the window (a file opens in 0.75 s).
- **Memory grew with every media change**, about 70 KB each time: over a day of playlists that is hundreds of MB. The
  interface now stays flat over 400 changes.
- **The file name in the player bar was one media behind**: it showed the previous file for as long as the next one
  played. The title, artist and album of a song no longer stay on the next media when it has none.
- **Starting a video is quicker** when it is decoded by the processor (older formats, or PCs without hardware
  decoding): the video output keeps its Direct3D device for the next video instead of making a new one each time.
- **The TV missed the end of a film**: the last seconds a film had (5.6 s, 11 s with subtitles) were dropped when it
  ended. When the next film of a playlist came while the TV still played (VLC sends ahead of it), VLC moved on and
  cut the end the TV had not shown yet (all of a short film). With a subtitle file beside the film, the TV got
  nothing after the last subtitle, often minutes before the end of the film, when the film started while casting
  (the next one of a playlist, or one opened during a cast). The TV now shows every film to its last picture.
- **Casting started twice**: the TV loaded a film, then loaded it again a moment later and missed its first second,
  at the start of every cast, for the next film of a playlist, and when a film went on from where it was left. Cast
  from the middle of a film, the TV got the sound alone first, and the picture only from the next key frame (4 s
  later in a 4K film). The TV now starts once, with the picture and the sound together.
- **Converting a video crashed VLC**: *Media › Convert/Save* of a video with its sound (any format: MP4, MKV, TS,
  AVI, WebM…) ended in a crash and left a broken file. It is the same in VLC 4 itself; VLC 3 was fine. Every
  format converts to the last picture now.
- **Some video effects crashed VLC**: *Edge detection* always, and *Anaglyph*, *Anti-flicker*, *Motion detection*,
  *Ripple* and *Waves* after the *Puzzle* or *Waves* effect, with a video decoded by the graphics card (most videos).
  It is the same in VLC 4 itself. Edge detection showed nothing; it shows the edges now. 40 effects checked alone and
  in chains.
- **A damaged file stopped the playlist**: a video with broken timestamps played to its end and stayed there, and the
  next file never started (the same in VLC 4 itself). It ends now. 148 cut and damaged copies of 35 formats play
  without a crash or a hang.
- **Quitting after a film took up to 10 seconds** right after installing or updating: when a film stopped, the
  progress bar started its sound analyzer (made for songs) on it, and Windows checks a new program before it runs.
  It runs only for a song now, without holding the window, and the installer has it checked.
- **Films of 50 and 60 frames per second juddered on the TV** when they are converted (shown subtitles, or a format the
  TV can't play): they were brought to 24 frames per second; they go at 25 and 30 now, every other picture.
- **A Samsung TV stopped when a cast was paused**: about 10 seconds into a pause, the TV left the film with an
  error. A paused Samsung TV goes on reading the stream until it has about 5 seconds of the film ahead, and gives
  up when nothing comes for 10 seconds. While paused, the stream now gets a trickle of empty packets (which the TV
  drops), and the TV stays paused. Found and checked on a 2017 Samsung UE55MU8000.
- **A film cast as it is played one second on a Samsung TV**: a film the TV can play without conversion (no
  subtitles shown) went as a fragmented MP4, which the TV took for a one-second file. It goes as MPEG-TS now, and a
  TV that stops such a film right after it began gets it converted instead of an error.
- **A cast started in the middle of a film was timed from the wrong picture**: when the film is converted for the TV
  (subtitles shown, or a format the TV can't play), VLC counted the TV's position from the key frame before the
  place the cast started, a picture the TV never gets. The time shown ran behind the TV by up to a key frame
  interval (6 s in a test film), and at the end the TV stayed on its last picture as long before VLC stopped it.
  VLC now counts from the first picture the TV gets: the time matches the TV, which is stopped right after it
  shows the end.
- **The last fifth of a second was missing in MPEG-TS**: a film converted or saved to a .ts file, or cast to a TV,
  lost its last 0.2 s of picture and sound (894 of 900 pictures, the TV stopped on a picture before the last one).
  They are all there now.
- **The player stopped while the TV still showed the end of a film**: VLC reads ahead of the TV, and it reached the
  end of a cast film about 6 s before the TV showed it: the time went blank (--:--), the seek bar stopped working
  and VLC went back to the library, or on to the next film, while the TV still played. The player now plays on,
  at the TV's time, until the TV has shown the last picture; the next film of a playlist starts after that. Paused
  in those last seconds, the TV keeps them for after the pause (it was stopped during the pause).
- **The subtitles window opened again when its button was clicked to close it**, and so did the menus of the ⋯
  button and of the library's Sort, View and More buttons. A click on their button opens and closes them now. A
  click outside a window or a list only closes it: a click on the video no longer pauses the film at the same time,
  and a click on another button does nothing more (the cast window, the sleep timer, the visualizer, HDR, every
  list of the settings, the language list).
- **The light theme left some windows dark or hard to read**: the tracks and subtitles window, the profiles list,
  the welcome screens, the shortcut windows, the cast and visualizer windows, the mini queue, selected text, the
  player customization and the effect sliders follow the theme now (the dark theme looks the same).
- **Keyboard shortcuts with several keys were cut short**: *Next track* has three keys (N, Shift+>, >) and showed
  "N Shif… >". Each key now shows whole, apart from the others.
- **A stray line in the Disc tab of Open Media**: half a separator under *No disc menus* is gone.
- **Thumbnails never came while VLC was set to start paused** (*Start paused* in the advanced preferences, or
  `--start-paused`): each picture of the library's folders, of the list next to the video and of the seek bar's
  previews waited 10 seconds and gave up. They come at once now. It is the same in VLC 4 itself.
- **Changing some video effects removed all of them**: moving the strength of the spatial blur (and any effect option
  read only as the effect starts) turned every video effect off. It is the same in VLC 4 itself.
- **A file opened paused showed no time**: with *Start playing as soon as a file opens* off, the bar read "--:--" for
  the time and the length until Play. It shows 00:00:00 and the length now.
- **The seek bar of paused music stood still on a jump**: the time changed, the waveform's position did not.
- **Listen Again listed the songs played first first**: the last played comes first now, as in Continue Watching.
- **The rows of the player's menu were not in line** (*Hide player UI* further in than the others).
- **HDR10+ and Dolby Vision said nothing about why they cannot be chosen**: the reason now shows under the pointer
  (Windows offers no way for a player to send them to a display).
- **Settings changed just before quitting were lost**: Settings keeps its changes when it closes (there is no Apply
  button), but quitting VLC with the window still open dropped them. They are kept now.
- **The library beside the video listed Home backwards**: Continue Watching and Listen Again showed the media played
  first first; they come in Home's order now, the videos by their file name as Home names them. *Videos* listed the
  music folders of the library too: it lists the folders holding videos, *Music › Songs* the ones holding songs.
- **Raw video files did not open**: a headerless raw file (.yuv, .cif, .qcif...) never played, and a YUV4MPEG2
  file (.y4m) logged an error before another reader took it: VLC lost the frame rate of the file. It is the
  same in VLC itself. Both open now.
- **The hover of the player's buttons and the time of the seek preview, as the design draws them**: the hover box
  was 2 px short and darker than the design's (white at 6 %), the preview's time 1 px small. Both were the last
  two frames of the design not compared pixel for pixel: all 103 are now.
- **The library panel in a small window**: under about 470 px of height its section icons ran over the player's bar
  (the + over the previous button), and in a narrow window the panel was cut on the right. The icons scroll in
  their column now, and the panel is no wider than the window.
- **The buttons of a narrow player's bar changed from one start to the next**: when the bar had no room for all of
  them, which ones stayed depended on timing (full screen there one time, gone the next). They go from the inside
  out now, the same every time: full screen and the playlist stay the longest.
- **The main menu in a small window**: under about 620 px of height (a maximized window on a 1366 x 768 laptop at
  125 %), its last rows were cut off with no sign that it scrolls. A thin bar now shows that it does.
- **The library was listed backwards**: artists, albums, tracks, genres, the playlists, videos and network folders
  started Z-A until another order was picked. They start A-Z now, also where an earlier version kept Z-A; Home's
  lists keep the newest first. An order you picked stays.
- **A folder of the library opened from *Videos* had its path as its name**, without its separators
  ("C:VideosFilms").
- **A time typed to go to went after the time shown**: the field opened with the cursor after the time, so 1:30
  became 00:02:101:30. The whole time is selected now, as in any time field: what you type replaces it.
- **The last rows of a long list in Settings were out of sight**: a dropdown shows eight rows, and the next ones
  (the last equalizer presets, most languages) were cut off where a row ends, with no sign that the list scrolls. A
  thin bar now shows that it does, as in the menus.
- **Two visualizations showed nothing**: projectM and the 3D OpenGL spectrum were offered in the player's
  visualizer, but they draw with OpenGL, which the video of this interface can't take on Windows (it is shown with
  DirectComposition): the music played on a black picture. Neither the player nor Settings offers them any more;
  the five of the design and Goom remain.
- **The resume card covered the subtitles**: for the 10 seconds it offers to continue a film, the card and the
  controls under it hid the start of a long subtitle line. The subtitles rise above them while it shows, and
  go back down when it closes.

### Added

- **Every language at once, right to left too**: choosing the interface language in Settings shows the whole
  interface in it immediately, without a restart: the side bar, the pages, the player, the settings, the menus and
  the dialogs (the classic ones when they open next). Save keeps it, Cancel goes back, as for the theme; the
  welcome and the classic settings do the same. Under the language, *Layout direction* lays the interface out left
  to right (the default, whatever the language) or right to left: everything mirrors at once, the arrows point the
  way one reads, and the playback bar keeps its order as on YouTube (time runs left to right).
- **The translations are in the program** (issue #5): the releases had none, so the language chosen in Settings
  did nothing. 100 languages ship now, and 72 cover every text of the interface: Afrikaans, Albanian, Arabic,
  Armenian, Basque, Belarusian, Bengali, Bosnian, Bulgarian, Catalan, Central Kurdish, Chinese (simplified and
  traditional), Croatian, Czech, Danish, Dutch, Estonian, Finnish, French, Galician, Georgian, German, Greek,
  Gujarati, Hebrew, Hindi, Hungarian, Icelandic, Indonesian, Irish, Italian, Japanese, Kannada, Kazakh, Khmer,
  Korean, Latvian, Lithuanian, Macedonian, Malay, Malayalam, Marathi, Mongolian, Nepali, Norwegian (Bokmål and
  Nynorsk), Occitan, Persian, Polish, Portuguese (Brazil, the redesign's texts contributed by @Kinglucas962 in #6,
  and Portugal), Punjabi, Romanian, Russian, Scottish Gaelic, Serbian, Sinhala, Slovak, Slovenian, Spanish (Spain
  and Latin America), Swedish, Tamil, Telugu, Thai, Turkish, Ukrainian, Urdu, Vietnamese, Walloon and Welsh, with
  British spelling for English (UK). Every language of the list is complete; VLC's other catalogs (Asturian,
  Kabyle, Uzbek...) are listed after them, marked "(partial)": VLC's own texts are translated there, the
  redesign's are in English.
- **Right-to-left languages are laid out right to left**: choosing Hebrew, Arabic, Persian or Urdu in Settings
  (or in the classic settings, or installing in it) mirrors the interface at once; *Layout direction* under the
  language still changes it, and a choice made there is kept. Each language file says which way it is written,
  so a new translation can be right to left too.
- **Anyone can translate**: a language file put in `locale\` appears in the language list by itself (by its own
  name, "(partial)"), and [Translating](docs/TRANSLATING.md) explains how to translate, try and send a language.
- **Move and resize the subtitles while watching**: the subtitle panel of the player has *Position* and *Size*,
  with − and + (and Reset). The subtitles move and grow on the video as you click, and stay so for the next
  videos and the next starts, as Shift+Up and Shift+Down already kept the position; the menu fades while they
  move so they show behind it (Ctrl+wheel's size is kept now
  too). The font, colours, outline, shadow and size chosen in Settings now reach the video already playing, from
  its next line (before: from the next video, the size after a restart). The font of the subtitles is picked in a list of the
  fonts installed (it was a field to type a font name in).
- **The translations read right**: over 700 wrong or outdated texts of the old catalogs were corrected (Arabic
  "OK" said "Yes", "Pause" an obscure word; Ukrainian "Space" was outer space; Page Up and Page Down were swapped
  in Ukrainian and Traditional Chinese), and the counts take the plural form of their number in every language
  ("2 элемента", "5 элементов"). A word with two meanings is translated once per meaning: the German side bar said
  "Pos 1" (the Home key) for Home, and the key names in the shortcuts now follow the keyboards (Entf, Suppr, Canc).
  Minutes and milliseconds are written in each language (5 мин, 5 dk, 5 分钟). The texts of VLC's core the
  interface shows are translated too: the explanations and choices of the settings, the names of the shortcuts,
  the messages on the video, the names of tracks and media information, the dialogs of the player, the network
  and casting.
- **The installer offers the language of Windows**: on a Windows in one of 27 languages (Arabic, Bulgarian,
  Catalan, Czech, Danish, Dutch, Finnish, French, German, Hebrew, Hungarian, Italian, Japanese, Korean, Norwegian,
  Polish, Portuguese of Brazil and of Portugal, Russian, Slovak, Slovenian, Spanish, Swedish, Tamil, Thai, Turkish,
  Ukrainian) it asks first, in that language and in English, whether to install VLC Redesign in it; yes, and the
  installer and the program are in it (Arabic and Hebrew right to left, the program too unless the Layout page
  is turned off; `/LEFTTORIGHT` for a silent install). An English Windows, or a language without a full
  translation, is not asked. An update keeps the language, a silent install is never asked (`/LANG=hebrew`,
  `/RIGHTTOLEFT`). The portable zip starts in English, left to right. Every text of the installer, its license
  notice included, is in each of these languages.
- **Subtitles in the computer's language from the first film**: until subtitle languages are chosen, a film's own
  subtitle track in the language of Windows is shown (not for English, where every English film would show its
  English subtitles), as the online search already looked for it.

- **Asked before going online**: the two features that connect by themselves, the daily update check (GitHub) and
  the automatic online subtitles (the film's name and a fingerprint of the file to subtitle services), are asked
  first. The installer has a *Privacy* page with both, on to begin with, what each one sends and to whom, and a link
  to the privacy and code signing policy (a silent install takes `/NOUPDATECHECK` and `/NOONLINESUBS`); the portable
  zip's first-run setup asks the same on a step of its own, and neither goes online before the answer. An update
  keeps the answers, and both stay in Settings. With online subtitles off, the subtitles menu searches only when its
  search button is clicked.
- **An update keeps your settings, the portable zip too**: a new version unpacked into another folder started with
  the default settings, an empty library and no history. It starts with those of the copy you used last now (the
  installed one or a portable one, 0.2.4 and later), once, when its own are still empty: settings, theme and
  accent, library, playlists, history and downloaded subtitles. The copy it came from stays as it was. The
  installer already kept them. VLC's own settings (VLC 3 included) are never taken. A copy with an empty file
  named `standalone` in its `portable` folder keeps to itself.
- **Always show the player bar** (*Settings › Interface › Player Bar*): the bar stays at the bottom of Home and the
  library while nothing plays; its Play starts the play queue, or opens files when the queue is empty.
- **Increase text contrast** (*Settings › Interface › Accessibility*): the dim texts of the design (hints,
  descriptions, watched videos, the (i) icons) are drawn brighter, to the contrast the WCAG accessibility
  guidelines ask for (AA, 4.5:1). Off by default: the colours of the design stay as they are.
- **A picture for a playlist**: *New playlist* takes a thumbnail of your own, and a playlist can be saved with only
  its title, to fill it later.
- **The library's menus of the design**: a right-click on a video, a song, a folder, a file or a playlist opens the
  menu card of the design, with *Mark as Watched* / *Unwatched* and *Show in File Explorer*.
- **More settings**: *Show confirmation and error dialogs*, *Show controls in fullscreen*, *Show cover art in the
  window when playing audio* and *Additional command line options*.
- **More video effects**: the *Geometry* (transform, rotate, interactive zoom, puzzle), *Overlay* (logo, text, logo
  erase) and *Advanced* (anaglyph 3D, mirror, psychedelic, waves, water, motion detect, blur, motion blur,
  anti-flickering, denoiser) effects of the classic dialog, in the effects of the redesign.
- **Properties of a playlist**: *Properties & Metadata* of a playlist's menu shows its name, its media, its length
  and its file.
- **Type a time to go to**: a click on the time of the bar makes it a field; type 1:02:10, 62:10 or 130 and press
  Enter (as the design draws it).
- **The visualizations of the design in Settings**: *Settings › Audio › Visualizer* offers Spectrometer, Scope,
  Spectrum, Waveform and VU meter with their icons (before: only "Visualizer filter"; the VU meter, which the design
  doesn't draw, with the gauge of its icons), then Goom. projectM and the 3D spectrum are left out: they draw with
  OpenGL, which the video of this interface can't take on Windows.
- **The library beside the video goes everywhere the library does**: Home with the sections of the library's Home
  (a section's arrow lists all of it), *Videos* and its folders, *Music* with Songs, Albums, Artists and Genres, and
  *Network* with Add Network Stream and the Internet services, each with its ⋯ menu (*Rescan library*, *Add folder
  to library*, *Add network stream*).
- **The queue of the mini player is part of its card**, as the design draws it: drag a row by its handle to move it.
- **Show the playlist as a tree** (*Settings › Interface › Playlist*, the last row of the design): the queue shows its
  media under the folder (or the website) they come from.
- **Effect presets** (Figma *Setting Preset*): the Effects window keeps sets of audio and video effects under a name.
  *New Preset* saves the effects playing, a preset applies them (Save keeps them, Cancel drops them), its ⋯
  renames, updates or deletes it, and *Default Preset* turns every effect off.

### Changed

- **Faster to start and to open Settings**: Settings opens in about 0.1 s instead of 1.3 to 1.8 s (a closed
  language list no longer loads the fonts of every script). The window
  shows 0.06 s sooner and is drawn 0.07 s sooner (0.33 s and 0.46 s here): the graphics driver starts while the
  interface loads, and no second graphics device is made at each start to check the one that works. In Hebrew,
  Arabic or Chinese the fonts are read while the interface is created.
- **The file name at the start of a film no longer covers its first subtitle**: it shows at the top of the picture
  now, clear of the subtitles at the bottom (a position you chose yourself in the settings stays).
- **VLC Redesign and an installed VLC share nothing any more**: the volume Windows keeps for the app was the one of
  VLC (VLC 3 too), a copy without its `portable` folder used VLC's settings folder and its language in the
  registry, and the old *File associations* button of the classic settings changed VLC's file types. VLC Redesign
  keeps its own volume, always has its own profile (it makes its `portable` folder, or uses
  `%APPDATA%\VLC Redesign` where it can't), and leaves VLC's files and registry alone. After the update its volume
  starts at 100 %, as any new app's in Windows; the one you set is kept from then on.
- **Uninstalling removes the settings**: the program goes with its settings, library and history, without asking
  (an update keeps them, a new version in another folder starts with them). Portable copies that no longer exist
  are dropped from the list the next copy reads its settings from.
- **Windows 10 version 1809 or later, the portable zip too**: the interface (Qt 6.8) needs it, and the installer
  already asked for it. A portable copy on an older Windows 10 (such as LTSB 2016) now says so when it starts.
- **The settings start as the design draws them**: on the Subtitles page, *Preferred language* picks the language of
  the subtitles, and the second language, the online search and the subtitle services come after the sections of
  the design. Every page was compared with its frame of the design, pixel for pixel.
- **The accent colour everywhere**: check marks, the icon of the current page and of the current settings section
  take the colour chosen (green, blue…), not always orange.
- **The older windows look like the rest**: *Open Media*, *Convert*, *Media Information* and the other classic
  windows are drawn in the dark or light colours of the redesign.
- **The resume card reads as in the design**: "You've watched till 00:45:08 already." and one *Continue Watching*
  button.
- **Videos stopped partway stay bright** in the library: only the ones watched to the end are dimmed, as the design
  shows them.
- **Home, the music player, the library next to the video and Customize Player as the design draws them**: Home's
  header shows the count and Sort, its pictures are no longer dimmed; the music player's cover and title sit where
  the design has them and its waveform is as high; a folder of songs lists "title - artist" with Add; Customize
  Player lists its items in the design's order with its title alone (a profile is renamed, cloned or deleted from
  its row in the profiles' list). A row of a folder shows the pointer, not the media playing.
- **HDR, the visualizer and the equalizer presets choose as the design's dropdowns**: the HDR and visualizer popups
  of the player are the field and list of the design, and the equalizer presets (Settings and the effects) start
  with *None* and draw the curve of each preset.
- **The effects panels as the design draws them**: each opens and closes as a panel, its sliders are the ones of the
  settings, the equalizer has its fields of values, Crop its fields of pixels. *Reset* was cut to "Rese" and
  *Two-pass* had no name: both read in full now.
- **The volume shows its value as the design does**: "100 %" in a box over the slider, while the pointer is on it,
  while it is dragged and a moment after the wheel or the keys change it.
- **The frame preview of the seek bar is the design's thumbnail**: a larger frame (252 px) with the time under it.
- **No light squares in the dark theme**: music, videos and artists without a picture showed VLC's light grey
  placeholder; it is dark now, its note in the accent.
- **The empty player shows Figma's pixel cat**: the player with nothing to play shows the picture of the design,
  the white cat on the floor of a dark room, drawn sharp at any size (it uses no picture file or memory).

- **4K films go to the TV four times faster**: a 4K film converted for the TV is made smaller by the graphics card
  instead of the processor (6 times faster than it plays on an RTX 4090, 2.5 times on an AMD Radeon built into the
  processor, instead of 1.3 to 1.5 times), so slower PCs keep up too.
- **Casting is lighter**: when a film is converted for the TV, each picture goes out as soon as it is made (the
  encoder held a number of them first), and the conversion takes half the memory (280 MB instead of 570 for a 1080p
  film with subtitles).
- **The TV is in the list before you open it**: once you have cast, VLC looks for your TVs quietly while a video plays,
  so they are listed as soon as you click the cast button, with the one you used last at the top.
- **Every program and plugin says VLC Redesign**: Task Manager and a file's Properties showed *VLC media player*,
  *VideoLAN* and 4.0.0.0 for every file of the package, and nothing at all for `vlc-waveform.exe` and
  `vlc-redesign-update.exe`. They all show *VLC Redesign*, *KuchiSofts* and the version of the release now, each with
  its own description, as code signing requires; the copyright of VideoLAN and the VLC authors stays.

### Technical

- Settings at once: `intf_DestroyByName()` stops the interfaces of one module (the web remote, Last.fm, the global
  hotkeys) for `intf_Create()` to start them again; `vlc_LogReload()` opens the log again from the settings;
  `vlc_actions_reload()` makes the key map again. `maininterface/live_settings.cpp` applies what changed after each
  save. The Windows global hotkeys unregister before their window goes (their atoms were never deleted).
- Qt's caches: `QML_DISK_CACHE_PATH` and the pipeline cache files point to the profile's `qtcache` (Qt names its
  cache folder after the program, `vlc`).
- Interface language at runtime: `vlc_settextlanguage()` (core) sets LANGUAGE, which gettext reads at each lookup;
  `MainCtx::applyInterfaceLanguage()` sets it and the default QLocale, sends QEvent::LanguageChange and emits
  `interfaceLanguageChanged()`, on which the QML engine retranslates and the C++ models, the side bar, the shortcuts,
  the tray menu and the classic dialogs refresh (hidden dialogs are made again; `markTranslationBinding()` lets a
  binding reading a C++ text retranslate). `MainCtx::rightToLeft` (redesign/right-to-left) sets the direction; the
  main interface, every popup and every other window bind `LayoutMirroring` to it.
- Translations: built with `-l` (`--enable-nls`); `po/vlc.pot` made again from the tree (it was VLC's of 2017; 7,823
  strings) with the 19 files of the redesign missing from `po/POTFILES.in` and the `tr()` keyword missing from
  `po/Makevars`; the catalogs are made again at each build (gettext remakes them only when vlc.pot changes).
- Translation contexts: the key names of `src/misc/actions.c` are `NC_("key", ...)`, looked up as "key\004Home" by
  `vlc_keycode2str()`; the translator of the Qt interface reads `qsTr(text, context)` as a msgctxt (xgettext
  keyword `qsTr:1,2c,2t`); `%1x`, `%1m` and `%1ms` are marked `xgettext:no-c-format`. Every catalog is merged with
  the new vlc.pot; the Kabyle catalog compiles again (a stray line since 2020, `%.lf` for `%.1f`).
- RowSnapWheel (widgets): the wheel over a list in whole rows, put over the list of every dropdown popup.
- The version is 0.3.0 from now on (`redesign_version.hpp`): it was raised only when a release was cut, so every
  build made since 0.2.6 called itself 0.2.6 in About, in the settings and in the file properties.
- Subtitle clearance: a `sub-clearance` variable on the video output (the bottom of the window the interface
  covers, in 1/10000 of its height; not an option) keeps the subtitles above it, only as high as needed: the
  space under a subtitle counts (4 % of the height for text), the window under the picture too for those placed
  in it, and lines stacked together move as one. When VLC blends them into the picture, in its pixels.
  `Player.subtitleClearance` sets it on every video output, and on new ones.
- Windows file errors (upstream): `vlc_open()` and `vlc_opendir()` set no `errno` (CreateFileW, FindFirstFileExW),
  so every first start logged "cannot open config file (vlcrc): No error" and "cannot open file" gave whatever
  reason was last. The Win32 error becomes `errno` now, as in `vlc_stat()`.
- QML: the `Qt.callLater()` that turn on a transition once a component is complete ran after it was gone when VLC
  quit right after making it (`--play-and-exit`): seven TypeErrors at each such quit, which the launch test missed
  (Qt logs them as `qt warning: (default)`). They check the object first, and the test looks for both forms.
- Settings carried over (`bin/winvlc.c`, before `libvlc_new()`): every start notes its portable profile with the
  time under `HKCU\Software\VLC Redesign\Profiles`. A profile with no `vlcrc`, no `vlc-qt-interface.ini` and no
  `ml.db` gets a copy of the one with settings written last: of the noted ones, the installed copy (the
  `InstallLocation` of its uninstall key) and the copies with "vlc" in their folder's name next to this one, up to
  two folders up (older versions noted nothing: a copy of VLC Redesign has `vlc-redesign-update.exe` or
  `vlc-waveform.exe`). No file is copied over another, lock files and `cache` folders stay behind, the paths of
  the old profile in `vlcrc` and the ini (as is, with doubled `\`, with `/`) name the new one, and the crash
  recovery (`RecoveryFilePath`) of the old copy is dropped. The uninstaller removes only `Capabilities` now, and the
  noted profile of the copy only when its data is removed.
- WASAPI (upstream since 2022): when the device buffer had no room for a whole block, the rest was retried and
  `aout_ChannelReorder()` ran on it again. The 5.1 and 7.1 reorders are involutions, so the rest went out in VLC's
  order, for about 150 blocks a second once the buffer was full (measured from what the device got: the centre
  tone was in the centre 56 % of the time and in the side-left 51 %). Retries have no date: the reorder runs on the
  first pass only. The pts of the rest moves on too, for a stream that starts from it after a device switch.
- `simple` channel mixer: stereo and mono come from a matrix built from the input layout (centre and surrounds -3 dB,
  side and back surrounds together -6 dB each, rear centre -6 dB on both sides, no LFE; mono is the mean of the two
  sides). The per-layout routines read 6.1 in another channel order and had none for quad, 4.1 and 6.0. Measured
  from what VLC writes for 15 layouts.
- `win_msg`: a file opened from Explorer is played with `vlc_playlist_PlayAt()` (`Start()` went on with the current
  item) and raises the window (`intf-show`). `system_Configure()`: the second instance waits 10 s at most, skips a
  hung window (`IsHungAppWindow()`, `SendMessageTimeout()`) and then opens the file itself; the mutex and window are
  named "VLC Redesign ipc", an official VLC 4 took the files. Files that arrive within 0.8 s of the previous ones
  (Explorer starts one VLC per selected file; they arrive 10–20 ms apart) are queued.
- Preparser: the pool thread spawns `vlc-preparser`, not `Submit()`, which runs with the playlist locked (0.5–10 s
  measured for a never-seen executable). The installer runs `vlc.exe --prewarm` (returns at once; `--version`
  opens a console and waits for a key), `vlc-preparser.exe --help` and `vlc-waveform.exe` once.
- Media library: `setDiscoverNetworkEnabled(true)` (every LAN services discovery: mDNS, SAP, UPnP) only when a root
  is not a `file://` folder or is not present, or when such a folder is added. MainCtx and the setup dialog don't
  open the welcome when the core playlist has media at start.
- `trivial` channel mixer: a rear centre goes to the rear left and right of an output that has no rear centre.
  `spdif` waits for the packetizer's sample rate: it asked for a 0 Hz output first, which logged "too low audio
  sample frequency (0)" at the start of every A/52, E-AC-3, DTS and TrueHD media.
- Qt 6.8 `QQuickScrollBar` connects its accessibility actions again on every `QAccessible::setActive(true)`, which
  Windows triggers through UI Automation all the time: `ScrollBarAccessibilityFix` (MainUI) removes the copies, at
  most once a second. `ControlRepeater` connected `forceUnlock` without a receiver, so it was never disconnected.
- `d3d11_fmt`: a device made for an output without a decoder device (default adapter, no decoding) is kept for 5 s
  after its release and handed to the next one with the same creation flags, unless it was removed or its DXGI
  factory is no longer current (HDR switched, a screen plugged in). The Direct3D 11
  output is no longer unloaded, for its timer. NVIDIA's driver kept about 22 KB per device.
- `PlayerController::UpdateName` emitted `nameChanged` before storing the name, and QML reads the property again when
  the signal comes. `UpdateMeta` clears the meta when the media has none yet, and meta changes of a media that is no
  longer current are ignored.
- Windows sockets: `recvfrom`, `sendto`, `send` and `accept` set `errno` when they fail (`EAGAIN` or the Winsock
  code). It kept the thread's previous value: logs said "read error: No error", and a stale `EAGAIN` or `EINTR` made
  `net_Read()` or the file output retry a dead socket forever.
- HTTP output: a stream with DLNA headers and an MPEG-TS type gets null packets (PID 0x1fff, 1.5 KB every 100 ms,
  64 MB at most per wait) when nothing was written for 0.5 s, from a thread of its own; blocks written are never
  split. The Samsung TV read 2 MB of them at once while paused and failed 10 s after they stopped (captured with
  pktmon: no request, no reset, the connection open; the same bytes replayed with more ahead did not fail).
- DLNA stream output: how far ahead of the media's pace it sends is a setting, `sout-dlna-lead` (1000 ms), and the
  input reads that plus 0.5 s ahead while it casts to a DLNA renderer (`UpdatePtsDelay()`, also when the renderer
  is set during playback): with `file-caching` the input had nothing more to send than 1 s. A video profile in
  MPEG-TS is preferred to the first one the renderer lists, and a stream copied as it was that the renderer stops
  within 15 s counts as unreadable (converted).
- Chromecast stream output: at the end of the media the chain is closed first (its muxer and transcoder write what
  they hold), then the TV reads until the fifo is empty or it stops reading for 15 s. At the start of each media the
  first blocks wait until the sound and the picture both sent some (at most 10 s of media or 3 s of waiting): the
  chain made with the first stream was made again with the next, and the TV loaded twice. A stream is added with
  its first block (a subtitle stream before any), and an MP4 file gives about a second of sound before its first
  picture after a seek.
- Input: the wake-up of the main loop is 10 s ahead at most (a broken PCR hours ahead made it wait for that, and the
  rest, whose PCR resets the clock, was never read), and at the end of the media the decoders are waited for twice
  the buffering delay (at the rate), 10 s at least: a picture or a sound with a broken date kept them from emptying.
- Filter loading (`vlc_filter_LoadModule()`): a module that opens with pictures in memory as input and a hardware
  format (no planes) as output, without a video context of its own, is closed and the next candidate tried (`chain`,
  which puts a converter after it). Filters are appended with the chain's output format, and those that don't check
  it (anaglyph, antiflicker, motiondetect, ripple, wave) wrote their pixels into D3D11 pictures without planes.
  `ripple` and `wave` copy no more lines, bytes or planes than both pictures have. `gaussianblur` reads its scale with
  the luma pitch (it used the chroma pitch times 2, past the end) and makes its buffers again for another picture
  size. `edgedetection` takes pictures in memory with the same format out, its internal chain gets pictures of its
  own format (it got the display's), and its output gets the date of its input (it had none: nothing was shown) and
  neutral chroma.
- `d3d11_surface` converter: a D3D11 picture to a CPU picture of another size (or 10 bits to 8: I420, YV12, NV12
  out) is scaled by the video processor into an NV12 texture of the output size, which is copied; the source and
  destination rectangles are the visible areas, and the stream and output color spaces are set the same. It was
  copied at its size (24 MB a 4K P010 picture) and scaled by swscale on one thread. The processor's output view is
  released on close (it was never released).
- Transcode `pcr_sync` (upstream since 2022): a PCR event holds one entry per ES id (it pushed the live ES only, read
  them by id, and the last ES wrote past the vector after a deleted one); the ES waiting for an event that is freed
  go to the next one (it was read after `free()` through their `last_pcr_event`, the heap corruption 0xC0000374 of
  every audio + video conversion); an ES with no frame when the PCR came is not counted and its passing no longer
  decrements the count. The discontinuity of an ES is cleared once recorded (it was cleared on a copy).
- `sout_MuxDeleteStream()`: what the input still has is muxed before it goes, the empty inputs skipped
  (`b_add_stream_any_time` for that call). Muxers wait until every input has a block, so the end of the stream that
  went past the others was dropped: the last pictures of a film converted for a TV.
- `demux_chromecast`: the drain result of `ES_OUT_DRAIN` was assigned to a `ret` declared in the EOF block (upstream
  since `1ebb26bbc9`, 2025), so the demux's EOF was returned and the input ended as soon as all was read, instead of
  when the TV reported FINISHED.
- Input: a renderer's demux filter is put on the master source only. A subtitle slave got `demux_chromecast` too when
  the input started with a renderer set, and its EOF was sent to the stream output as the end of the media. Seeks
  queued before the first `demux_Demux()` (start time, the position to resume from) are done before it: about 250 ms
  were read from the start, decoded and flushed, and a renderer had loaded them. Going to a renderer from here, the
  input seeks (not precisely) to where it played here (`vlc_player_SetRenderer()` gives the player's time in
  `renderer-enter-time`; else where the input read): the streams were started again from where the input read, in
  the middle of a group of pictures, so the video waited for the next key frame while the sound went on.
- Renderers: x264 conversions use `tune=zerolatency` (sliced threads, no lookahead or B-frames): 287 MB instead of
  571 MB, the same processor time and 4.2 Mb/s instead of 2.5 (measured, 1080p with a subtitle). Above 31 fps the
  frame rate is halved (50 to 25, 59.94 to 29.97, 120 to 30) instead of forced to 24; 30 fps comes as 1000000/33333
  from Matroska.
- Qt `Waveform`: the source is a media playing or paused with audio and no video, analyzed after it stays 1 s.
  `vlc-waveform` runs on a `QThread` per analysis (`QProcess` there with `waitForFinished()` steps and a cancel flag),
  joined by a `qAddPostRoutine()`. `CreateProcessW` held the interface thread for 6–10 s for a new executable.
- `RendererManager::prepare()`: one scan per session, 3 s into a playing video, when a scan was done before
  (`redesign/cast-scanned`: Windows asked for the firewall then). Opening the list during a scan gives it 20 s from
  then. The device cast to last (`redesign/cast-last-device`) is inserted at row 0.

## [0.2.6] · 2026-10-22

Fixes from [#4](https://github.com/KuchiSofts/vlc-redesign/issues/4): menus that kept old check marks, the
Visualizer and the right-click menu of the music player, and what *Open Network Stream* takes as a link.

### Fixed

- **Menus kept the check marks of earlier choices**: choosing one profile after another left all of them checked in
  the main menu. A check mark that goes now goes, in every menu of the redesign.
- **The Visualizer could not be changed or turned off** once its window was closed: a visualization plays in the
  video layout, which had no Visualizer button. The button stays above the controls while a music file shows a
  visualization.
- **No menu on a right click** over the music player, nor over a visualization. Both open the player's menu now.
- **Open Network Stream** took anything typed in *Network URL* as links: "my channels" became two broken items of the
  playlist. Only links and paths are taken now, and the dialog says which word isn't one.
- **Profiles no longer turn the media library off**: a profile saved with *Use the media library* off turned it off
  when chosen, and Home, Videos, Music, Playlists and Recent stayed greyed out.

### Technical

- `RedesignMenuItem`: the check mark is hidden (opacity) rather than unloaded: an `IconImage` whose `source` becomes
  empty keeps showing its last image. `marked` and `markable` show a choice kept by a model without making the row
  checkable.
- The visualization's vout is made by the audio filter (`aout_filter_GetVout()`), not through the player: no module
  listened to its mouse. A `TapHandler` on the video surface opens the player's menu when the media has no video track.
- `media-library-enabled` joins the installation's keys that profiles neither capture nor apply.

## [0.2.5] · 2026-10-22

Switching the sound to other speakers or headphones while playing is now seamless, and VLC Redesign starts in
English. The first small update: from 0.2.4 it downloads only what changed.

### Fixed

- **Changing Windows' default sound device while playing** ([#3](https://github.com/KuchiSofts/vlc-redesign/issues/3)):
  - VLC Redesign could close by itself after a few switches, or when the device changed while paused (0.2.3; 0.2.4
    already holds).
  - Every switch kept the second of sound decoded ahead for the previous device, never freed: up to 9 MB each time
    (with a 7.1 device). That sound is now played on the new device.
  - So there's no more gap of over a second at every switch: the sound goes on after a fifth of a second, still in
    sync with the picture. Choosing another device in VLC Redesign's own menu is as smooth.
- **English by default**: VLC Redesign and its installer start in English until you choose a language in Settings,
  whatever the language of Windows (they took it from Windows, or from an installed VLC). *Reset all* comes back to
  English.

### Technical

- `modules/audio_output/mmdevice.c`: `Stop()` no longer drops the stream's queued blocks (never freed); during an
  output restart (`aout-restarting`, set by the core around it) they go to the next `Start()` when the format is the
  same, and are freed otherwise. `src/audio_output/dec.c` keeps the clock across an output restart (only a flush
  resets it), so their dates stay right.
- `src/input/decoder.c` reloads an audio decoder for a new output only for the codecs that can be passed through
  (MPEG audio, AC-3, E-AC-3, DTS, TrueHD, MLP): for any other one the reload restarted the output a second time.
- `bin/winvlc.c` and `interfaceLanguage()`: `portable\language` missing means English; the installer has
  `LanguageDetectionMethod=none`.

## [0.2.4] · 2026-10-22

Updates without the installer: a few hundred kilobytes in the background, and a restart of a second or two. Setup
no longer gets stuck, pausing and seeking no longer freezes the video, and coming back from a TV is clean.

### Added

- **Small updates**: a release carries the changes from each of the three releases before it, most files as binary
  patches, so an update downloads only what changed (0.2.2 → 0.2.3 would have been 1.9 MB instead of the 55 MB
  installer). It downloads in the background with its progress in the card and in About, is checked against the
  release's `SHA256SUMS.txt`, and the new files are written next to the old ones, each checked against its SHA-256,
  on a worker thread: the player never waits.
- **Restart now** or **Later**: *Restart now* puts the update in place and opens VLC Redesign again on what was playing,
  where it was, in about two seconds. *Later* puts it in place when you close VLC Redesign, or the next time.
- **The portable copy updates itself too**, its `portable` folder (your settings and library) left as it is.
- **Nothing half-done**: if a file can't be replaced (another program holds it), every file goes back as it was and
  the update is tried again next time. If a file of VLC Redesign was changed since it was installed, the update says
  so and the installer does it instead. When no small update fits (more than three releases behind, or installed for
  everyone in Program Files), *Update now* uses the installer, as before.
- Earlier versions can't apply a small update, so the update to 0.2.4 itself comes with the installer (or the zip):
  small updates start with the release after it.

### Fixed

- **Setup stuck on "Preparing VLC Redesign for a fast start"** ([#2](https://github.com/KuchiSofts/vlc-redesign/issues/2)):
  it waited, with no limit, for the plugin cache to be made again, which loads every plugin, and on some PCs one never
  finished loading (another program holding or injecting into it). Setup no longer needs that step: the files keep
  their times in every time zone, so the cache of the package is right as installed. The cache tool gives up after
  90 seconds and leaves the user's settings out. Thanks for the report!
- **Slow starts** after unzipping in another time zone, or after plugins changed: the plugin cache no longer matched
  the files, and every start loaded all the plugins. VLC Redesign now saves it again by itself; the next start is fast.
- **Playback**:
  - After a pause, a seek and play, the video stayed frozen while the sound went on (VLC 4's clock).
  - A seek while paused landed elsewhere or showed no picture, and said *Next frame*. The picture of the seek is shown.
  - A previous-frame step could hang the media, and a subtitle shown while paused read the file ahead: play then
    jumped forward.
  - The time stood still for a moment after a seek or a start.
  - The sound could break up while the computer was busy: the Windows audio buffer was 22 ms, it is 200 ms now.
  - Subtitles of a music file could hold VLC Redesign open after quitting.
- **Casting**:
  - Casting asked about the Windows firewall in the app first: Windows now asks at once, only when the rule is
    missing or blocked, and the window doesn't freeze meanwhile.
  - Stopping a cast in the middle showed broken pixels and flickered: playback goes on here where the TV was, with a
    clean picture, for Chromecast and DLNA TVs.
  - A DLNA TV that can't read a file as it is gets it converted, instead of an error.
  - Back from a paused cast, the picture, play and quit work; the time stays while the TV pauses; subtitles come back.
  - The conversion for a TV takes 8 threads at most, and a Chromecast is listed with its name.
- **Interface**:
  - Icons are sharp at 150 % and 200 % display scale, and follow a scale changed while VLC Redesign runs.
  - The last Settings editors that needed the mouse (equalizer, startup volume, colour swatches, the shortcut list)
    work with the keyboard.
- **Reopening after an update** started the media as a clip from where it was: the time began at 0:00, the length was
  shortened, and the part before couldn't be reached (VLC 4's `:start-time`). It now seeks there, without the
  *Continue where you left off?* card.
- A binding loop in the keyboard shortcut dialog's layout.
- **Memory and threads growing with every video**: each video opened (the next item of a playlist, the next episode)
  left its Direct3D 11 device behind, with its driver's threads and memory (about a hundred threads on an NVIDIA
  card). A playlist of short clips reached 7,000 threads and closed after a few minutes. The device now goes with the
  video: the count stays flat, and VLC starts with 40% fewer threads. VLC 4's own nightly builds do the same.
- **A crash when changing tracks quickly**: the Windows audio output was stopped while its thread still played the
  stream (in VLC 4 itself), and VLC closed after a few seconds of skipping through short items.
- **A video output that can't work here** (Direct3D 9 or OpenGL chosen in *Settings › Video*) showed no picture and
  kept a processor core busy trying again for every frame. The outputs that work are tried next.
- **Folders named in another alphabet** (a Windows user name in Hebrew or Chinese, where VLC Redesign installs): the
  Lua scripts didn't load from there (YouTube and other links, artwork from the web, the services of the sidebar).
- **Hardware decoding first** didn't hold for conversions (casting with subtitles, mosaics): without a hardware decoder
  they lost the picture. It now only applies to playback.
- **Casting**:
  - Stop or Next as the Chromecast paused could freeze the player.
  - A TV that was off was reported as having refused, after a long wait for every step.
  - A TV that joined an MP4 or Ogg stream from its start got its header twice and could refuse it.
  - A subtitle chosen for a sound cast restarted the TV's stream.
  - A Chromecast found over IPv6 hid its working address.
  - Stopping one media also cut the end of the next ones.
  - *Allow* in the cast panel on a Public network asked again and again: it now allows public networks too (the
    button says so) and never adds the rule twice. The firewall check no longer holds up the interface.
- **Subtitles from the services**:
  - An archive could unpack to gigabytes: only its subtitles are unpacked now, 16 MB at most, away from the
    interface.
  - A file name from a service could point outside the cache.
  - A subtitle stayed "loading" when another title was chosen.
  - A search could end before every service had answered.
  - The cache keeps the last 500 media, and drops subtitles unused for 60 days.
- **Updates**: a stalled download waited 30 minutes, and couldn't be cancelled. Now each step waits 30 seconds, and the
  card's × cancels it. Downloads follow the system's proxy, only HTTPS files from GitHub are used, and a small update
  that failed isn't tried again: the installer is.
- **Interface**:
  - In fullscreen, "f" or Esc typed in the library search left fullscreen.
  - The folder panel froze while a network drive didn't answer; it is listed in the background now.
  - *Favorite* on all the playlists crashed with more than a hundred of them.
  - Closing VLC while customizing the player saved what *Cancel* would have dropped.
  - Choosing another toolbar profile changed the privacy settings.
  - *Reset all* forgot the window's place at exit.
  - The OpenSubtitles password showed in clear text in the subtitle association dialog.
  - Tab typed a tab in the one-line fields instead of moving on.
  - Left or Esc in the queue card lost the focus.
  - The minimize, maximize and close buttons answered at their old place after the title bar changed.
  - Scrolling Settings over the equalizer changed its bands.
  - About couldn't scroll in a small window.
  - Some effect options showed sliders from −9 quintillion to 9 quintillion.
- **Lighter and smoother**: the Customize outlines, about 220 hidden editors on the first Settings search keystroke,
  the dialogs kept after closing, the track menus of the toolbar previews and both layouts of each folder entry are no
  longer built for nothing. The music waveform is analysed once per file (it was once per slider, hidden ones
  included), the mini player's bar seeks by key frames while dragged, and seek bar previews skip network files and
  keep 48 MB at most.
- Windows: VLC Redesign needs Windows 10 version 1809 or later (as Qt 6.8 does). The installer says so, and on an older
  Windows `vlc.exe` shows a message instead of nothing.

### Technical

- `vlc-redesign-update.exe` (`extras/juxtopposed/updater`) puts a staged update in place once VLC has quit, keeps the
  old files until all new ones are in, rolls back on failure, refreshes the plugin cache and the version in *Apps*.
- `extras/juxtopposed/release/make-delta.py` builds the `.vrdelta` files (bsdiff patches, zlib blocks, a JSON index
  with every file's SHA-256), `apply-delta.py` is a reference applier, and `make-release.ps1` checks each delta by
  applying it to the old release and comparing every file. Binaries get a PE time stamp of 0, so a file whose code
  didn't change is the same from one release to the next.
- The download reports its progress (`HttpRequest::progress`), and the hidden `--redesign-resume-time` option reopens
  the media at a time.
- Direct3D 11: the DirectComposition visual lets go of the swapchain when the display closes, `UpdateStaging()`
  releases the staging textures of the format before, and `ReleaseD3D11PictureSys()` clears what it releases.
  `vout_wrapper.c` tries the other outputs after a forced one fails; `src/input/decoder.c` marks its decoders as
  reloadable (`decoder-reload`), which avcodec's hardware-first path needs; `modules/lua/vlc.c` reads the scripts and
  their `require`d modules with `vlc_fopen` on Windows.
- `HttpRequest` has a deadline for the whole request besides the timeout of each step, and `httpRequestAsync()` returns
  a function that cancels it.
- The installer sets `TimeStampsInUTC` and runs no `vlc-cache-gen`; `bin/cachegen.c` passes `--ignore-config` and
  ends itself after 90 s (a watchdog thread started before the plugins load); `src/modules/bank.c` saves the cache
  again when a plugin had to be loaded from its file or an entry no longer had one.
- `src/clock/clock.c`: the main clock's time stays at the pause date while paused (`vlc_clock_main_now()`), across a
  reset, so the first picture after a paused seek isn't dated in the past. `es_out` shifts the input clock and the
  main clock in the order of the pause; `decoder.c` keeps the frame-step countdown quiet for the picture a paused seek
  asks for, waits for video only for `next_frame_need_data`, and lets an SPU decoder waiting for a vout be interrupted.
- WASAPI: a 200 ms shared-mode buffer (the device's minimum when refused), the first time reported right after a
  start, and the log says when the device ran out of audio.
- Back from a renderer, `INPUT_CONTROL_SET_RENDERER` seeks precisely to the renderer's time (`renderer-time`, in
  stream time) after the ES restart; the DLNA output asks for a conversion when the TV refuses a stream copied as it
  is.

---

## [0.2.3] · 2026-10-20

A new installer drawn like the app, which can open your media files; seek bar previews decoded ahead; a wiki.

### Added

- **A new installer, drawn like the app**: the library's sidebar with the steps, Inter, the dialogs' buttons, the
  settings' ON / OFF toggles and an orange progress bar, in English and Hebrew (right to left). Three clicks from
  *Welcome* to *Done*: no more "Select install mode" dialog or *Ready* page.
- **Open media files with VLC Redesign**: the installer's *Options* can make it the player of video, music and
  playlists (the ~130 formats VLC plays), wherever no other app was chosen by you; the *Done* page opens Windows'
  *Default apps* for the rest. VLC Redesign is always offered in *Open with* and in *Default apps*.
- **Right-click menu**: *Play with VLC Redesign* and *Add to VLC Redesign queue* on media files and folders
  (an option of the installer).
- **Install for everyone** from the *Options* page: Setup starts again with administrator rights, for Program Files.
- The installer remembers the options of the last install, and the uninstaller removes all of it and gives every file
  type back to the app it had before (checked against a snapshot of the registry, in English and in Hebrew).
- **A [wiki](https://github.com/KuchiSofts/vlc-redesign/wiki)**: every screen and every option of the Settings window
  (with its default and its VLC option), installation, updates, shortcuts, troubleshooting, with screenshots.

### Changed

- **Seek bar previews come much faster**: the frames are kept, a frame the pointer has left behind is still decoded for
  later instead of being thrown away, and while you hover the bar the frames around the pointer are decoded ahead.
  Positions snap to 1/200 of the length (the preview seeks to the nearest key frame anyway).

### Fixed

- **A time of hundreds of hours**: playback started paused at a time (`:start-time` with `--start-paused`) showed a
  time like 623:17:44, as the player's last timer point was read before VLC had sent one (it was never initialized).
  The time now waits for the first point, and a time past the end of the media is taken from the position.

### Documentation

- README: the player in the nine accent colours and the light theme, *Settings and smart features* with screenshots of
  the Subtitles and Playback settings, and the new installer. The compact library's *Home* is no longer a known
  limitation: it lists the recent media.

---

## [0.2.2] · 2026-10-20

The fullscreen button moves to the control bar, and the control bar is easier to customize.

### Changed

- **Fullscreen button in the control bar**: it left the title bar, where it sat next to *Close*, for the right end
  of the bottom bar (suggested in [#1](https://github.com/KuchiSofts/vlc-redesign/issues/1)). Saved profiles get it
  once, at the same place; take it out with *Customize Player* and it stays out. The title bar keeps minimize,
  maximize and close.

### Fixed

- **Customize Player**: a button let go in the gap between two controls of the bar was ignored. It now goes between
  them, like a drop on either control.
- **Updates**: a check made from About no longer pops up the update card behind the About window too, nor again
  once About is closed. A download started from About still shows its progress in the card.

### Documentation

- README: [*Customize the interface*](README.md#customize-the-interface), with a 44-second video.

---

## [0.2.1] · 2026-10-20

Fullscreen fixes, reported in [#1](https://github.com/KuchiSofts/vlc-redesign/issues/1). Thanks!

### Fixed

- **Leaving fullscreen**: fullscreen entered without a video (the title bar's button in the library or for music,
  or F11) could only be left with F11, since the title bar hides in fullscreen and the fullscreen keys only drive
  the video's fullscreen. The *Fullscreen* and *Leave fullscreen* shortcuts (F, Escape, or whatever you set them to)
  now leave it too, from any view.
- **No frame in fullscreen**: the 10 px frame and rounded corners around the video are kept for the window; in
  fullscreen the video fills the screen and the bars float over it.

### Notes

- The fullscreen button sits next to the window buttons as in Juxtopposed's design. To keep it away from *Close*,
  drag it into the control bar: ⋯ → *Customize Player*. F and a double-click on the video also toggle fullscreen.
- CI: the Windows build workflow now completes (the package step could not write its output).

---

## [0.2.0] · 2026-10-20

**Solid on every Windows PC, and it keeps itself up to date.** A full audit of every button, menu and setting,
an installer in the redesign's look, updates from GitHub, and the latest VLC 4 fixes.

### Highlights

- 🚀 **Starts every time, fast**: a second copy no longer hangs on start (it used to wait forever with no window),
  and a fresh install or unzipped copy no longer rescans its ~600 plugins on each start (≈0.4 s to the window).
- 📦 **Installer**: per-user, no administrator rights, styled like the redesign, closes and reopens a running copy.
- 🔄 **Updates from GitHub**: a card when a release is out; an installed copy updates itself after checking the
  installer's SHA-256, and reopens on what was playing.
- ℹ️ **New About window** in the redesign's style, with the design credit and links to Juxtopposed.
- 🧹 **~80 fixes** from an audit of the whole interface, including one crash.
- 🆙 **VLC 4 merged** up to 2026-10-19 (110 commits), and every pull request is now built automatically.

### Added

- **Installer** (`vlc-redesign-<version>-win64-setup.exe`, Inno Setup): installs per user by default (or for all users),
  keeps settings, library and history across updates, regenerates the plugin cache, optional desktop shortcut and
  *Open with* entries for video and audio files that don't change the default player; English and Hebrew.
  A running copy is closed politely (forced after 15 s if needed) and reopened afterwards.
- **Updates**: once a day (Settings → General → *Check for updates automatically*, option `redesign-update-check`) or
  from About, VLC Redesign reads the latest GitHub release. A card offers *What's new*, *Skip*, and *Update now*
  (installed copy) or *Download* (portable copy). Classic menus' *Check for Updates* opens About.
- **About** redrawn in the redesign's style: version and update check, the CC BY 4.0 design credit with links
  (channel, the redesign video, Figma file, website), first implementation, VideoLAN, and the classic lists behind it.
- **Profiles**: New, Rename, Clone and Delete in *Customize Player*; the design's **Movies, Shows, Music and Work**
  profiles for new installs.
- **Settings**: search finds the labels you see (and matching shortcuts); an *Applies after restart* hint on the rows
  that need it; errors are shown and a failed save no longer loses your changes; developer mode reveals the expert
  options and *All VLC options*; process priority is a list; night mode restores the compressor it replaced.
- **Library**: a *Recent* page with recently played videos and songs; *Playlists* shows every playlist; video grouping
  (by name, by folder) is back in the sort menu; the back/forward arrows both work.
- **Player**: the visualizer can be opened from the music view; the right-click *Repeat* is an Off / All / One choice;
  the maximize button shows a restore icon when maximized; Shift+F1 opens About.
- **Shortcuts editor**: a key is added to an action (each key has its own remove), and Escape cancels a capture.
- **Release tooling**: `make-release.ps1` builds the zip, the installer and `SHA256SUMS.txt`; a GitHub workflow builds
  every pull request and `main` for Windows; issue forms for bugs and suggestions; a [roadmap](ROADMAP.md).

### Fixed

**Startup and stability**
- A second VLC Redesign hung on start while another one ran: pipe names collided between processes, so its preparser
  and then its interface failed (`src/win32/filesystem.c`); the preparser now also falls back to in-process parsing.
- An unzipped copy rescanned all plugins on every start (zip stores times in 2-second steps): release files get one
  even-second time before the cache is made, and the installer rebuilds the cache.
- A crash (use-after-free) in the video effects after toggling a filter.
- With the media library off, the whole window stayed blank.
- Playlist export no longer creates a player on the interface thread (a freeze risk): the file is written directly.
- Download threads are cancelled and waited for when VLC quits.

**Player and menus**
- The sleep timer was cancelled at the start of each new track.
- Q (queue) and L (library) never reached the player.
- The Quit button did nothing; the Library button did nothing in the mini player.
- The top bar used the video layout in the music player (⋯ and Library missing or doubled).
- The subtitle *None* check stayed on after picking a track.
- Repeat All came back as Repeat One after a restart (and is repaired for those affected).
- HDR choices left VLC on the libplacebo output for good.
- PiP hid the video where picture-in-picture isn't available; the mini queue's − removed the playing track.
- *Open Recent* ignored *Save recently played items*; *Open Location from Clipboard* now matches Ctrl+V.
- *Customize Player* opened from the library left the player stuck in edit mode.
- Tooltips: "Back 15 seconds" / "Forward 15 seconds", Fullscreen naming and icon; the sleep timer and player menu in the
  light theme.

**Library and subtitles**
- Stream and Convert were swapped in the context menus.
- OpenSubtitles needed a second click (and a second download) to save; your account is kept for the session.
- File names with `( ) ' ! & + , ; = @ $` missed their resume point, artwork and subtitles.
- Creating a playlist without the media library could crash; onboarding without it came back at every start.
- Onboarding overwrote the metadata network access setting; the suggested folder can be cleared.
- Folder menu labels, *Frequently Visited Folders ›*, the new playlist thumbnail, network links without a scheme,
  and search text carried between sections of the compact library.

**Settings and effects**
- Compressor, Spatializer and Stereo Widener sliders stopped after one step.
- Saving video effects made the current crop the default for every video.
- Effects tabs cancelled each other's preview; the dialog didn't wake up when playback started.
- Rows that did nothing were removed or wired (*Ignore ASS/SSA styling*, error dialogs, playlist tree); the cone and
  cast quality rows show the right labels; the web remote warns without a password; saving no longer shows a UI hidden
  with Ctrl+H.
- Profiles: switching to a default profile no longer reverts your theme and settings, and a profile that fails to
  apply at startup no longer replaces your saved ones.
- Readable error messages instead of raw option names, and typos ("kHz", "changes", the Last.fm password label).

### Changed

- New interface strings are listed for translation (`po/POTFILES.in`), and C++ strings use VLC's `qtr()`.
- The default profile of new installs is named *Default*.
- The fork's original M0 CI workflow is replaced by the Windows build workflow.

### Upstream VLC

- Merged 110 commits of VLC master (up to `942cdaea17`, 2026-10-19). The window buttons register themselves for Windows
  11 snap layouts; VLC's new split button is built but the redesign keeps its own buttons.

---

## [0.1.0] · 2026-10-20

**The first Windows release.** The redesign now builds and runs on real Windows, matches the Figma
frames down to the pixel on the measured screens, and no longer freezes.

### Highlights

- 🪟 **Runs on Windows**: a reproducible Docker build and a portable zip.
- 🎯 **Pixel-matched to the design**: text renders like Figma (Inter, same spacing), the light theme follows the *Player - Light* frame.
- 🧊 **No more freezes**: three deadlocks between the interface and the core are fixed.
- 📚 **New library spots**: *Listen Again*, *Frequently Visited Folders*, a folder browser, and a compact library over the video.
- 🎛️ **Settings rebuilt** with the design's controls and many more VLC options.
- 🖼️ **Frame previews** when hovering the seek bar.

### Added

**Player**
- A compact library sidebar over the video: sections, folder breadcrumb, search, sorting, progress and play counts.
- A queue card with *Queue*, *Bookmarks* and *Chapters* tabs.
- Frame previews when hovering the seek bar, for local seekable videos (option `qt-seek-thumbnail`).
- 12 new control bar buttons: ±10 s, picture-in-picture, right sidebar, home, thumbnail, folder name, playlist name, replay, chapters, bookmarks, playlists, main menu, library.
- Control bar editing by drag and drop on the player itself. The timeline can sit on its own row.
- **Q** toggles the queue and **L** the library (only when no VLC hotkey uses the key).
- An *External Tracks* group in the subtitle panel; a refresh for the audio device list; a *None* video track.

**Library**
- *Listen Again* on Home (recently played audio) and *Frequently Visited Folders* (ranked from recent history).
- A folder page with breadcrumbs.
- Playlists: favourite, shuffle play, export to a file.
- Add and remove bookmarks from the interface.
- The subtitle dialog can also attach external audio tracks, and target a whole folder or playlist.

**Settings**
- A settings window without an Apply button: changes are saved when it closes. Search, sidebar, and an *Advanced Options* block on every page.
- The design's controls: toggle, dropdown, segmented control, slider, text field, startup volume, inline equalizer.
- Many more VLC options, regrouped like the design, including repeat / loop, play and exit, fast seek, skins, mouse wheel direction, web remote, night mode, caching presets and post-processing.
- Language list (ISO 639-2), optical drives and x264 presets in dropdowns.
- Actions: rescan plugins, open the configuration folder, reset everything.
- Window geometry memory, notification duration, cast device discovery, process priority (Windows), double-click action, a filter for the recent items list, subtitle size mode and background opacity.
- Interface language for the portable copy, kept in `<vlc folder>\portable\language` instead of the registry an installed VLC uses.

**Dialogs and menus**
- A redesigned **Network Stream** dialog: several URLs at once, optionally saved as a playlist.
- A redesigned right-click menu: playback, *Hide UI* (Ctrl+\\), playback / video / audio / subtitle submenus, playlist, open media, quit. Shortcut labels come from your actual hotkeys.
- A **⋯ main menu**: open actions, *Open Recent*, profiles, keyboard shortcuts, customise player, settings, about.
- Classic VLC menus are redrawn to the design, with two-tone icons.

**Credits and packaging**
- The design credit (CC BY 4.0) in **About**, in its Credits page, and in [CREDITS.md](CREDITS.md).
- The Windows package ships `CREDITS.txt`, `CHANGELOG.txt`, `COPYING.LIB.txt` and the font licenses (`licenses/`).

### Changed

- The video is framed between the bars, with a 10 px inset and rounded corners; a redesigned top bar.
- The skip buttons jump ±15 s.
- Times are always shown with hours; the resume prompt is a compact card; record shows the accent colour instead of blinking.
- The volume slider goes up to 200 %, with fill states to match.
- The bars auto-hide in fullscreen.
- Onboarding: keycaps show your real shortcuts, the theme previews live, and it can be cancelled with *Cancel* or Escape.
- The toolbar editor became a *Customize Player* panel that edits the bar in place.
- The subtitle association and new playlist dialogs use the shared redesign dialog.
- Video cards use file names as titles, dim watched items and show progress as an accent line.
- The library header has an address bar, named sort options and a *View Options* menu.
- Without the main interface (skins), the classic Preferences and Effects windows open.

**Visual fidelity**
- Inter is the application font, rendered with distance fields, no hinting and greyscale antialiasing, the combination that matches Figma's text.
- The bundled Inter has every advance width one unit wider: Qt truncates advances to 1/64 px, and the extra unit gives back the design's spacing.
- DM Mono for keyboard shortcuts.
- Light theme: icons are recoloured to the *Player - Light* frame's greys.
- Most icons re-exported; new player corners and theme preview images; restyled checkboxes, scroll bars, headers and sidebar rows.
- Rounded video and artwork textures crop correctly at any aspect ratio.

### Changed defaults

| Option | Before | Now | Why |
|---|---|---|---|
| `qt-pin-controls` | off | **on** | The design keeps the bars beside the video |
| `qt-video-autoresize` | on | **off** | The window keeps its size when a video starts |
| `qt-max-volume` | 125 | **200** | The design's volume control goes to 200 % |
| Loop hotkey (`key-loop`) | `L` | **Shift+L** | `L` opens the library |
| Next / previous | | also **`<` `>`** | |
| Next / previous frame | | also **`.` `,`** | |
| OSD message duration | fixed | **`osd-duration`** (1000 ms) | Set from the notification duration setting |
| Update checks (`qt-updates-notif`) | on | **off** | VideoLAN's update feed offers the official VLC, not this build |

### Fixed

**Freezes and crashes**
- The interface froze when a hotkey waited on the video output: hotkeys now go to the core from their own thread.
- A deadlock between the player, the input resource and the interface thread: `input_resource_SetInput` now runs in the input thread's `Init`.
- A deadlock when the video window was enabled or disabled: `vlc_window_Enable` / `Disable` are called outside `window_lock`, and `Enable` gets a copy of the configuration.
- A crash on exit with MinGW (Qt's thread-local timer outlived the libstdc++ destructor key).
- An assertion on iPhone HEIC photos (`ES_OUT_DRAIN`); HEIF images show the primary grid instead of each tile.
- Wrong colours and Direct3D 11 device removal when switching video filters; a plane overrun in the interactive filter picture pool.
- Video filters (extract, gradfun, gradient, grain, invert, sepia, sharpen) refuse to open when input and output formats differ, instead of producing garbage.

**Interface**
- The window shrank by 16×8 px on every restart (Windows' resize borders weren't counted). It now opens at 1280×720, centred, the first time, and restores maximised and fullscreen windows to their normal size.
- The waveform seek bar and cover corners were rebuilt without Canvas, which VLC's Qt Quick lacks; the waveform helper gets native paths on Windows.
- Rate and delay changes are announced after the player lock is released.
- Developer mode was on whenever the setting read back as the text `"false"`.
- Importing a profile with a setting that no longer exists failed: the setting is now skipped.
- The effects dialog no longer cancels itself while closing; popups are centred on whole pixels; the title bar stays draggable while a dialog is open.
- The audio effects hint showed an internal name (`audioBridge`).
- The registry language read is type-checked and the key is closed.

### Build

- **Windows Docker build**: `extras/juxtopposed/win64-docker.ps1` (host) runs `win64-docker.sh` in VideoLAN's official `vlc-debian-win64-posix` image with prebuilt contribs. Objects persist in a Docker volume; `-Quick` skips configure; `-Lint` runs qmllint on changed QML; `-Volume` and `-Strip` make a separate stripped release build.
- The SRT, x265 and GME plugins link the static unwinder, so they don't import `libgcc_s_seh-1.dll`, which the plugin loader can't find.
- `vlc-waveform` links its runtime statically on Windows.
- OpenSubtitles uses a WinHTTP client on Windows, so the QtNetwork dependency is gone.
- License headers on every new source file; Meson and Autotools list the same QML files and resources; line endings normalised to LF.

---

## M3 · Dialogs, settings and playback tools · 2026-10-13

*By MrDouglax, in [JuxtopposedVLC](https://github.com/SrDouglax/JuxtopposedVLC).*

### Added
- Redesigned settings on a staged QML model: startup, file logging and diagnostics, Last.fm, developer mode, add-ons, audio device preview with rollback, DPI-scaled navigation, clearing history under Privacy.
- A three-step onboarding: welcome, theme, shortcuts.
- A keyboard shortcut editor with staged changes and a confirmed reset.
- Transactional audio and video effects, equalizer preset curves, effects inline in settings.
- Sleep timer (stop or pause), control bar profiles and a personalisation menu.
- Control bar customisation on the live player; profiles keep a safe subset of settings.
- New controls: favourite, file and artist name, frame rate, resolution, zoom, HDR, divider.
- Mini player with a live queue, picture-in-picture, a dedicated music view, a blurred backdrop.
- A **waveform seek bar** computed by the isolated `vlc-waveform` helper, and a *waveform* audio visualisation.
- HDR output selection, depending on the video backend.
- Advanced subtitle sources (OpenSubtitles, links), series timing presets and a sync timeline in the tracks menu.

### Fixed
- QML delegates, series lifetime, light theme contrast, popup checkmarks, video resizing during effect rollback.

## M2 · Library, playlists and subtitles · 2026-10-13

*By MrDouglax.*

- A compact tracks and playback panel with chapters and bookmarks.
- Create and edit playlists from the library; native *Open media* actions.
- Subtitle choices remembered per media, restored for direct playback too.
- Header layout for narrow windows; video cards, header, sidebar and floating player aligned to the design.

## M1 · First native shell and player · 2026-10-12

*By MrDouglax.*

- The redesigned window shell and player controls (window buttons, adjacent playback controls, design defaults).
- The redesign icon set and the four volume states exported from Figma.
- Fixed opening videos through nested navigation.

## M0 · Baseline and build · 2026-10-12

*By MrDouglax.*

- Linux Docker build, smoke tests and launcher (`extras/juxtopposed/`).
- A Windows CI workflow with MinGW runtime packaging.
- Plans, Figma coverage maps and validation evidence under `.specs/`.

---

## Differences from upstream VLC

Most of the work is in the Qt interface (`modules/gui/qt`). These changes touch the rest of VLC; they're
the ones to watch when merging upstream, and the candidates to propose to VideoLAN.

| File | Change |
|---|---|
| `src/input/input.c` | `input_resource_SetInput` moved to the input thread's `Init` (deadlock) |
| `src/video_output/video_output.c` | Window enable/disable outside `window_lock` (deadlock); pool fallback for other chromas |
| `src/video_output/window.c` | `vlc_window_Enable` passes a copy of the configuration |
| `src/video_output/video_text.c` | OSD duration from `osd-duration` |
| `src/libvlc-module.c` | `osd-duration` and `media-library-enabled` options; hotkey changes (see *Changed defaults*) |
| `src/libvlc.c` | The media library is created only when `media-library-enabled` is on |
| `src/player/medialib.c` | External subtitle and soundtrack files from the media library attached before playback |
| `src/audio_output/filters.c`, `output.c` | The *waveform* visualisation choice |
| `modules/visualization/visual/` | Oscilloscope code moved to its own file; waveform effect |
| `modules/demux/mp4/heif.c` | Primary grid shown; PCR reset after drain (assert) |
| `modules/video_output/win32/direct3d11.cpp` | Unsupported CPU chromas and mismatched pictures refused |
| `modules/video_filter/{extract,gradfun,gradient,grain,invert,sepia,sharpen}.c` | Refuse differing input/output chromas |
| `modules/text_renderer/freetype/freetype.c` | `freetype-fontsize` brought back; default text direction |
| `bin/winvlc.c` | Portable language file read before the registry |
| `src/win32/filesystem.c` | `vlc_pipe()` names pipes per process (`vlc-<pid>-<n>-<time>`), first instance only, no remote clients |
| `src/preparser/preparser.c` | Falls back to the in-process preparser when the external one can't start |
| `bin/Makefile.am`, `bin/meson.build` | Build the `vlc-waveform` helper |
| `extras/package/win32/package.mak` | Credits, changelog and font licenses in the package |

[0.2.1]: https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.2.1
[0.2.0]: https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.2.0
[0.1.0]: https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.1.0
