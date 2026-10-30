# Feature tour

A guided tour of **VLC Redesign 0.4.0**, platform by platform, in short clips. Each clip is the real app, recorded
frame by frame from its window with a drawn pointer: the clicks, wheel turns and keys shown in the corner are the
input the app got. **Click a clip to open its MP4**, the same clip in better quality. Under each clip, a link leads
to the page that explains the feature in full.

> **Design by Juxtopposed.** Every screen in these clips comes from the
> ["VLC Redesign (Community)"](https://www.figma.com/community/file/1691465999134370980) Figma file by
> [Juxtopposed](https://www.youtube.com/@juxtopposed), licensed under
> [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Juxtopposed does not endorse this project.

| Platform | In this tour | Status |
|---|---|---|
| **[Windows](#windows)** | 17 clips: every main feature of 0.4.0 | Released, 0.4.0 (2026-10-30) |
| **[Linux](#linux)** | The AppImage on Arch Linux | Released, 0.4.0 (2026-10-30) |
| **[macOS](#macos)** | A screenshot of the app | Released, 0.4.0 (2026-10-30) |
| **[Android](#android-in-testing-not-part-of-040)** | 8 phone clips and 1 tablet clip | In testing, **not part of 0.4.0** |

Where each platform stands, with its requirements and files: [Platforms](Platforms).

**The whole tour as one film per platform** (silent, 1920 × 1080, the clips at 1.5× speed with a caption each):

| [![The Windows film](https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/films/windows.jpg)](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/films/vlc-redesign-0.4.0-windows.mp4) | [![The Linux and macOS film](https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/films/linux-macos.jpg)](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/films/vlc-redesign-0.4.0-linux-macos.mp4) | [![The Android film](https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/films/android.jpg)](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/films/vlc-redesign-0.4.0-android.mp4) |
|---|---|---|
| **Windows**, 0.4.0: the 17 clips, 2 min 55 s (14.0 MB) | **Linux and macOS**, 0.4.0: 21 s (0.9 MB) | **Android**, in testing: 9 clips, 2 min 2 s (3.9 MB) |

**Media in the clips:** the Blender open movies *Sintel*, *Big Buck Bunny*, *Elephants Dream*, *Tears of Steel* and
*Caminandes* (CC BY), music by Kevin MacLeod (incompetech.com, CC BY 4.0), a made-up TV provider for the Live TV clips
(invented channels, logos and programmes, showing these same open movies), and a test Chromecast that runs on the recording PC (no real device). The subtitles of *Sintel*
in Hebrew, Arabic and Japanese were written for these clips.

---

## Windows

Windows 10 (version 1809 or later) and Windows 11, 64-bit. Version 0.4.0, the installer or the portable zip:
[Install and update](Install-and-Update).

**On this platform:** [First run](#first-run) · [Library](#library) · [Player](#player) · [Subtitles](#subtitles) ·
[Customize](#customize) · [Themes](#themes) · [Settings](#settings) · [Live TV](#live-tv) · [Casting](#casting) ·
[Effects](#effects) · [About](#about) · [Right to left](#right-to-left)

### First run

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/onboarding.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/onboarding.gif" alt="The welcome steps: a media folder, the theme and accent colour, the subtitle languages, what VLC does online, the basic shortcuts, then Home" width="960"></a>

**The welcome steps.** The first start of a new copy goes through the welcome: a media folder for the library, the
theme (Light, Dark or System) and the accent colour, the subtitle languages, what VLC does online (the update check
and online subtitles, each with its own switch), and the basic shortcuts. *Finish* opens Home. The setup can be run
again from Settings, General, Advanced Options, *Welcome setup*.

In depth: [In one minute](Home#in-one-minute) · [Colours and themes](Colours-and-Themes#where-to-change-them) ·
[What goes online](Install-and-Update#privacy) · [The keys of the first-run setup](Keyboard-Shortcuts#the-keys-of-the-first-run-setup)

### Library

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/library.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/library.gif" alt="Home with Continue Watching and the folders, then Videos, Music, Albums and Tracks" width="960"></a>

**Home and the library.** Home opens on *Continue Watching* (the videos you started, with how far you got) and your
folders. The clip then goes through **Videos**, **Music**, and the music pages **Albums** and **Tracks**.

In depth: [Library](Library) · [Home](Library#home) · [Music](Library#music)

### Player

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/player-bar.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/player-bar.gif" alt="The player bar: play and pause, dragging the seek bar with its subtitle marks, the mouse wheel over the volume" width="720"></a>

**The player bar.** Play and pause, then a drag along the seek bar: its thin marks are the lines of the subtitle file
shown (new in 0.4.0), so you see where people talk. Last, the mouse wheel over the volume changes it.

In depth: [The seek bar](The-Player#the-seek-bar) · [Volume](The-Player#volume) · [The control bar](The-Player#the-control-bar)

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/open-state.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/open-state.gif" alt="Buttons that light up while what they open is open: tracks and subtitles, effects, the player menu, the more menu" width="800"></a>

**A button shows what it opened** (new in 0.4.0, suggested in [#10](https://github.com/KuchiSofts/vlc-redesign/issues/10)).
While its panel, popup or menu shows, a button lights up: the accent icon on a tint of the accent, the same look as a
toggle that is on. The clip opens the tracks and subtitles panel, the effects, the player menu and the ⋯ menu.

In depth: [Panels that open above the bar](The-Player#panels-that-open-above-the-bar)

### Subtitles

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/subtitles-he-ar-ja.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/subtitles-he-ar-ja.gif" alt="English, Hebrew, Japanese and Arabic subtitles in turn with the V key" width="720"></a>

**Subtitles in every language.** The <kbd>V</kbd> key goes through the English, Hebrew, Japanese and Arabic subtitles
of the film in turn. Each one is sharp, and Hebrew and Arabic read in the right order, with their brackets and
punctuation in their place.

In depth: [Subtitles](Subtitles) · [Subtitle keys](Subtitles#subtitle-keys)

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/tracks-panel.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/tracks-panel.gif" alt="The tracks panel: Video, Audio and Subtitles tabs, the subtitle languages with their own names, Hebrew picked" width="800"></a>

**The languages and tracks panel.** Its **Video**, **Audio** and **Subtitles** tabs, the subtitle languages of the
video, each written in its own language, and Hebrew picked from the list.

In depth: [The Subtitles tab](Subtitles#the-subtitles-tab) · [Languages and tracks](The-Player#languages-and-tracks)

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/subtitle-size.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/subtitle-size.gif" alt="Ctrl and the mouse wheel over the video change the subtitle size, up to 200 % and down to 50 %" width="720"></a>

**Ctrl + mouse wheel sizes the subtitles** (new in 0.4.0, suggested in [#10](https://github.com/KuchiSofts/vlc-redesign/issues/10)).
Over the video, <kbd>Ctrl</kbd> and the wheel change the subtitle size, as in classic VLC: in the clip up to 200 %,
then down to 50 %, from the next line on. It goes by 10 % near 100 %, then by 25 %, from 25 % to 500 %, and a short
*Subtitle size* message shows the value; <kbd>Ctrl</kbd>+<kbd>0</kbd> goes back to 100 %. Elsewhere in the window,
<kbd>Ctrl</kbd> and the wheel still zoom the whole interface.

In depth: [Position and size](Subtitles#position-and-size) · [Keyboard shortcuts](Keyboard-Shortcuts#subtitles)

### Customize

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/customize-player.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/customize-player.gif" alt="Customize Player: Playback Speed and Cast to TV dragged onto the bar, saved, then 1.5x chosen from the speed button" width="800"></a>

**Customize Player.** **Playback Speed** and **Cast to TV** are dragged from the palette onto the bar, the layout is
saved, then 1.5× is chosen from the new speed button. In 0.4.0 the speed button can be added again
([#9](https://github.com/KuchiSofts/vlc-redesign/issues/9)): it used to leave an empty slot.

In depth: [Customize the player](Customize-the-Player) · [The palette](Customize-the-Player#the-palette) ·
[Speed](The-Player#speed)

### Themes

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/theme-accent.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/theme-accent.gif" alt="Settings, General: the light theme and the blue, green and pink accents applied at once, then the library in light pink" width="960"></a>

**Theme and accent colour.** In Settings, General, the light theme, then the blue, green and pink accents: each
applies at once, with no restart. The clip ends on the library in the light theme with the pink accent.

In depth: [Colours and themes](Colours-and-Themes) · [The accent colours](Colours-and-Themes#the-accent-colours) ·
[Settings: General](Settings-General#appearance)

### Settings

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/settings-search.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/settings-search.gif" alt="Settings: the Interface, Playback and Subtitles pages, then a search for speed across every page" width="960"></a>

**The Settings window.** The **Interface**, **Playback** and **Subtitles** pages, then a search for "speed", which
looks through every page at once.

In depth: [Settings](Settings) · [Search](Settings#search)

### Live TV

New in 0.4.0. Live TV is an add-on, on by default; without it nothing of it shows.

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/tv-catalog.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/tv-catalog.gif" alt="Live TV: the sources, the catalog of free, legal lists, then the channels of a source with their logos, numbers and groups" width="960"></a>

**Sources and the catalog.** *Sources* lists your TV sources; the catalog offers free, legal channel lists (the films
filter, then all); then the channels of a source, with their logos, numbers, groups and what is on now, filtered by
group. The channels are those of a made-up demo provider.

In depth: [The catalog](Live-TV#the-catalog) · [The TV page](Live-TV#the-tv-page) · [Add your TV](Live-TV#add-your-tv)

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/tv-zapping.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/tv-zapping.gif" alt="Live TV: Page Up and Page Down change channel, and the banner shows the number, the programme, its progress and description" width="800"></a>

**Change channel.** <kbd>Page Up</kbd> and <kbd>Page Down</kbd> change channel. At every switch the banner shows the
channel number, the programme on now with its progress, and its description.

In depth: [Watching](Live-TV#watching)

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/tv-guide.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/tv-guide.gif" alt="The Live TV guide with the line of now, each programme's progress, Later and Now, and the Kids group" width="960"></a>

**The guide.** **Guide** at the top of the TV page: the channels down, the hours across, the line of now and each
programme's progress. *Later* moves ahead by the hours shown and *Now* comes back; the clip ends on the Kids group.

In depth: [The guide](Live-TV#the-guide)

### Casting

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/cast.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/cast.gif" alt="Cast to: the devices found, a Chromecast picked, Playing on Living Room TV, pause and play from the bar" width="800"></a>

**Cast to a TV.** The **Cast to** panel lists the devices found (the real ones of the recording network are hidden
here), a Chromecast is picked (a test Chromecast that runs on the recording PC), the player reads *Playing on Living
Room TV*, and the bar goes on controlling it: pause, then play. In 0.4.0 the list keeps looking while it is open.

In depth: [Casting to a TV](Casting) · [Cast](Casting#cast) · [TV compatibility](TV-Compatibility)

### Effects

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/effects.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/effects.gif" alt="Effects over the playing video: the equalizer turned on, two bands raised, the video effects, back to audio" width="800"></a>

**Effects while the video plays.** The Effects panel over the playing video: the equalizer turned on and two of its
bands raised, then the video effects, then back to the audio tab. Every change is heard or seen at once.

In depth: [Effects](Effects) · [Equalizer](Effects#equalizer) · [Video](Effects#video)

### About

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/about.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/about.gif" alt="About: the platform badge Windows x64, What's new in 0.4.0 and the designer's credit" width="960"></a>

**About.** **⋯ → About VLC**: the platform badge (*Windows x64*), *What's new* in 0.4.0, and the credit of the
designer, Juxtopposed.

In depth: [Credits and license](Credits-and-License) · [Small updates](Install-and-Update#small-updates)

### Right to left

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/rtl-hebrew.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/rtl-hebrew.gif" alt="The interface in Hebrew, laid out right to left: the sidebar, Videos, Music, Albums and the mirrored more menu" width="960"></a>

**A right-to-left language.** The interface in Hebrew, laid out right to left: the sidebar on the right, Videos,
Music, Albums, and the ⋯ menu mirrored.

In depth: [Languages](Languages) · [Right to left](Languages#right-to-left)

---

## Linux

**Released, 0.4.0.** A .deb, an .rpm, an AppImage and a tar.xz for x86_64 and ARM64, next to the distribution's own
VLC; a Flatpak comes next. How to install: [Install and update: Linux](Install-and-Update#linux). Requirements and
what is tested: [Platforms: Linux](Platforms#linux).

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/linux.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/linux.gif" alt="The Linux AppImage on Arch Linux: About with Linux x86_64 AppImage, Settings in the dark theme, Home in the light theme" width="960"></a>

**The Linux AppImage on Arch Linux.** About with its badge *Linux x86_64 · AppImage*, Settings in the dark theme, then
Home in the light theme. The same interface as on Windows, from the same code.

In depth: [Platforms: Linux](Platforms#linux)

## macOS

**Released, 0.4.0.** A disk image for Apple Silicon and one for Intel, built and tested by GitHub's Macs, signed ad
hoc (macOS asks once to open it). How to install: [Install and update: macOS](Install-and-Update#macos).
Requirements and what is tested: [Platforms: macOS](Platforms#macos).

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/macos.png"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/macos.png" alt="The macOS app on an Intel Mac, Home" width="960"></a>

**The macOS app (Intel), Home.** A screenshot taken by the launch test of the build (a picture, not a clip).

In depth: [Platforms: macOS](Platforms#macos)

---

## Android (in testing, not part of 0.4.0)

> [!NOTE]
> **The Android app is not part of 0.4.0 and has no download yet.** It is a separate app, a fork of VLC for Android
> with its screens redrawn in the design, in its own repository, and it is in testing. These clips were recorded on
> the Android emulator. What it is and where it stands: [Platforms: Android](Platforms#android).

Portrait phone clips (Home at 400 px wide, the others at 360 px), then the tablet clip.

<table>
<tr>
<td width="50%" valign="top"><a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/home.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/home.gif" alt="Android phone: Home with Continue Watching, folders, new videos and music, then Videos, Music and TV" width="300"></a><br><b>Home and the library.</b> The phone's Home with Continue Watching, folders, new videos and music, then Videos, Music and TV.<br><sub>Desktop: <a href="Library">Library</a></sub></td>
<td width="50%" valign="top"><a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/customize-player.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/customize-player.gif" alt="Android phone: the player's menu, then Customize Player with the bar's three zones, Playback speed and Queue added, saved" width="300"></a><br><b>Player bar and Customize Player.</b> The player's menu, then Customize Player: the bar's three zones, Playback speed and Queue added, saved.<br><sub>Desktop: <a href="Customize-the-Player">Customize the player</a></sub></td>
</tr>
<tr>
<td valign="top"><a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/context-menu.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/context-menu.gif" alt="Android phone: the more button of a video opens a compact menu; a long press selects" width="300"></a><br><b>The compact context menu.</b> The ⋯ of a video opens a compact menu; a long press selects.<br><sub>Desktop: <a href="Library#the-right-click-menu-of-a-media">The right-click menu of a media</a></sub></td>
<td valign="top"><a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/subtitles-hebrew.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/subtitles-hebrew.gif" alt="Android phone: the tracks panel over the video, the subtitle files next to it, Hebrew picked and read right to left" width="300"></a><br><b>Tracks and subtitles in Hebrew.</b> The tracks panel over the video: the subtitle files next to it, Hebrew picked, read right to left.<br><sub>Desktop: <a href="Subtitles">Subtitles</a></sub></td>
</tr>
<tr>
<td valign="top"><a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/tv-zapping.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/tv-zapping.gif" alt="Android phone: a channel of the demo provider, then the next ones with the banner" width="300"></a><br><b>Live TV zapping.</b> A channel of the demo provider, then the next ones: the banner shows the number, the programme, its progress and description.<br><sub>Desktop: <a href="Live-TV#watching">Live TV: Watching</a></sub></td>
<td valign="top"><a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/tv-guide.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/tv-guide.gif" alt="Android phone: the Live TV guide with the line of now, swiped through the hours and the channels" width="300"></a><br><b>Live TV guide.</b> The guide on a phone: the line of now, swiped through the hours and the channels.<br><sub>Desktop: <a href="Live-TV#the-guide">Live TV: The guide</a></sub></td>
</tr>
<tr>
<td valign="top"><a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/cast.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/cast.gif" alt="Android phone: the Cast to list with a test Chromecast, then the card shown while the video plays on the TV" width="300"></a><br><b>Casting card.</b> The Cast to list (a test Chromecast of the recording PC), then the card that shows while the video plays on the TV.<br><sub>Desktop: <a href="Casting">Casting to a TV</a></sub></td>
<td valign="top"><a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/settings.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/settings.gif" alt="Android phone: the settings pages, General with theme and accents, a page, and the search" width="300"></a><br><b>Settings.</b> The settings pages, General with the theme and the accents, a page, and the search.<br><sub>Desktop: <a href="Settings">Settings</a></sub></td>
</tr>
</table>

### Tablet

<a href="https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/media/demo/android/tablet.mp4"><img src="https://raw.githubusercontent.com/KuchiSofts/vlc-redesign/main/docs/media/demo/android/tablet.gif" alt="Android tablet: the sidebar and list plus detail, an artist and an album beside their list, More, and the settings pages beside their list" width="960"></a>

**Tablet layout** (in testing, not part of 0.4.0). The sidebar and a list with its detail beside it: an artist and
an album next to their list, *More*, and the settings pages next to their list.

In depth: [Platforms: Android](Platforms#android)

---

All the clips with their files: [docs/media/demo](https://github.com/KuchiSofts/vlc-redesign/tree/main/docs/media/demo).
Interface design by [Juxtopposed](https://www.youtube.com/@juxtopposed).

[Home](Home) · [Platforms](Platforms) · [The player](The-Player) · [Live TV](Live-TV) · [Credits and license](Credits-and-License)
