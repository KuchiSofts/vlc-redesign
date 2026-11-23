# Demo clips of VLC Redesign

Short clips of every program and main feature, for the README, the release notes and the wiki
([Feature tour](https://github.com/KuchiSofts/vlc-redesign/wiki/Feature-Tour)). Each one is the real app recorded
frame by frame from its window with a drawn pointer (the clicks, wheel turns and keys shown in the corner are the
input the app got). The clips of the README were recorded again on the 0.8.0 build as animated WebP (960 px wide,
20 fps); the GIFs are from the 0.4.0 build. Each clip has an MP4 of the same clip next to it (1920 x 1080 for the
0.8.0 ones).

Interface design by [Juxtopposed](https://www.youtube.com/@juxtopposed) (the
["VLC Redesign (Community)"](https://www.figma.com/community/file/1691465999134370980) Figma file, CC BY 4.0).

Media in the clips: the Blender open movies *Sintel*, *Big Buck Bunny*, *Elephants Dream*, *Tears of Steel* and
*Caminandes* (CC BY), music by Kevin MacLeod (incompetech.com, CC BY 4.0), a made-up TV provider for the Live TV clips
(invented channels, logos and programmes, showing these same open movies), and a test Chromecast that runs on the recording PC (no real device). The subtitles of *Sintel*
in Hebrew, Arabic and Japanese were written for these clips.

## Windows

| Clip | What it shows |
|---|---|
| [onboarding.webp](onboarding.webp) · [MP4](onboarding.mp4) | The welcome: a media folder, the theme and accent colour (Light, Dark, System), the subtitle languages, what VLC does online, the basic shortcuts. |
| [library.webp](library.webp) · [MP4](library.mp4) | Home with Continue Watching, then Videos, the albums of Music, and Tracks with *Play all* and *Shuffle* ([#130](https://github.com/KuchiSofts/vlc-redesign/issues/130)). |
| [player-bar.webp](player-bar.webp) · [MP4](player-bar.mp4) | The player bar: the bar hides and comes back, play and pause, dragging the seek bar (its marks are the subtitle lines), the mouse wheel over the volume. |
| [open-state.webp](open-state.webp) · [MP4](open-state.mp4) | A button lights up while what it opens is open: tracks and subtitles, effects, the player menu, the ⋯ menu ([#10](https://github.com/KuchiSofts/vlc-redesign/issues/10)). |
| [subtitle-size.webp](subtitle-size.webp) · [MP4](subtitle-size.mp4) | Ctrl + mouse wheel over the video makes the subtitles larger (to 175 %) from the next line on, and Shift + Up raises them; the messages at the top keep their own size. |
| [subtitles-he-ar-ja.webp](subtitles-he-ar-ja.webp) · [MP4](subtitles-he-ar-ja.mp4) | English, Hebrew, Japanese and Arabic subtitles in turn with the V key: sharp, and Hebrew and Arabic in the right order with their brackets and punctuation. |
| [tracks-panel.webp](tracks-panel.webp) · [MP4](tracks-panel.mp4) | The tracks panel: the Video and Subtitles tabs, the subtitle languages of the video with their own names, Hebrew picked (its line on the video). |
| [customize-player.webp](customize-player.webp) · [MP4](customize-player.mp4) | Customize Player: Playback Speed and Picture in Picture dragged onto the bar, saved, then 1.5× chosen from the new speed button. |
| [theme-accent.webp](theme-accent.webp) · [MP4](theme-accent.mp4) | Settings, General: the light theme and the accent colours (blue, green, pink), applied at once, then the library in light pink. |
| [settings-search.webp](settings-search.webp) · [MP4](settings-search.mp4) | Settings: the Interface, Playback, Subtitles and TV pages, then a search for "speed" across every page. |
| [tv-sources.webp](tv-sources.webp) · [MP4](tv-sources.mp4) | Live TV: *Sources* and its tabs (your sources, a link, accounts of media servers, guides), then the channels of a source with their logos, numbers and what is on, by group (a made-up demo provider). |
| [tv-zapping.webp](tv-zapping.webp) · [MP4](tv-zapping.mp4) | Live TV: Page Up and Page Down change channel; the banner shows the number, the programme with its progress and its description at every switch. |
| [tv-guide.webp](tv-guide.webp) · [MP4](tv-guide.mp4) | Live TV: the Guide with the line of now and each programme's progress, *Later* and *Now*, the Kids group, and a programme's details. |
| [cast.webp](cast.webp) · [MP4](cast.mp4) | Cast to a TV: the devices found (real ones hidden here), a Chromecast picked, *Playing on Living Room TV*, pause and play from the bar, then back to *This computer*. |
| [effects.webp](effects.webp) · [MP4](effects.mp4) | Effects over the video: the equalizer turned on, three bands moved (two up, one down), then the video effects. |
| [about.webp](about.webp) · [MP4](about.mp4) | About (the released 0.8.0): the version, the platform badge (*Windows x64*), the designer's credit and the other systems it runs on. |
| [rtl-hebrew.webp](rtl-hebrew.webp) · [MP4](rtl-hebrew.mp4) | Settings, the language list down to Hebrew: the whole interface turns right to left at once, no restart (Settings, the sidebar, Home). |

## Linux and macOS

| Clip | What it shows |
|---|---|
| [linux.gif](linux.gif) · [MP4](linux.mp4) | The Linux AppImage (Arch Linux): About with *Linux x86_64 · AppImage*, Settings in the dark theme, Home in the light theme. |
| [macos.png](macos.png) | The macOS app (Intel), Home, from the smoke test of the build. |

## Android

The Android app (phone and tablet, recorded on the emulator; not part of 0.4.0 yet). Portrait phone clips are 360 px wide (Home 400 px), the tablet clip 960 px.

| Clip | What it shows |
|---|---|
| [android/home.webp](android/home.webp) · [MP4](android/home.mp4) | **Home and the library** (0.8.0). The phone's Home with Continue Watching, folders, new videos and music, then Videos and its Folders tab, Music and the TV guide. |
| [android/customize-player.gif](android/customize-player.gif) · [MP4](android/customize-player.mp4) | **Player bar and Customize Player.** The player's menu, then Customize Player: the bar's three zones, Playback speed and Queue added, saved. |
| [android/context-menu.gif](android/context-menu.gif) · [MP4](android/context-menu.mp4) | **The compact context menu.** The ⋯ of a video opens a compact menu; a long press selects. |
| [android/subtitles-hebrew.gif](android/subtitles-hebrew.gif) · [MP4](android/subtitles-hebrew.mp4) | **Tracks and subtitles in Hebrew.** The tracks panel over the video: the subtitle files next to it, Hebrew picked, read right to left. |
| [android/tv-zapping.gif](android/tv-zapping.gif) · [MP4](android/tv-zapping.mp4) | **Live TV zapping.** A channel of the demo provider, then the next ones: the banner shows the number, the programme, its progress and description. |
| [android/tv-guide.gif](android/tv-guide.gif) · [MP4](android/tv-guide.mp4) | **Live TV guide.** The guide on a phone: the line of now, swiped through the hours and the channels. |
| [android/cast.gif](android/cast.gif) · [MP4](android/cast.mp4) | **Casting card.** The Cast to list (a test Chromecast of the recording PC), then the card that shows while the video plays on the TV. |
| [android/tablet.gif](android/tablet.gif) · [MP4](android/tablet.mp4) | **Tablet layout.** The sidebar and list + detail on a tablet: an artist and an album beside their list, More, and the settings pages beside their list. |
| [android/settings.gif](android/settings.gif) · [MP4](android/settings.mp4) | **Settings.** The settings pages, General with theme and accents, a page, and the search. |
