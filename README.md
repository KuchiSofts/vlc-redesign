<div align="center">

# VLC Redesign <sub><sup>(unofficial)</sup></sub>

**VLC 4 with the interface designed by [Juxtopposed](https://www.youtube.com/@juxtopposed), built for real and running on Windows.**

[**⬇ Download for Windows**](https://github.com/KuchiSofts/vlc-redesign/releases/latest) ·
[What's new](CHANGELOG.md) ·
[Roadmap](ROADMAP.md) ·
[Credits](CREDITS.md) ·
[Build from source](#build-from-source)

<a href="docs/media/demo.mp4"><img src="docs/media/demo.gif" alt="VLC Redesign in action: seek bar frame previews, light theme, music waveform, and the design credit in About" width="880"></a>

<sub>▶ <a href="docs/media/demo.mp4">Watch the full 85-second tour (MP4)</a></sub>

</div>

---

## 🎨 Design by Juxtopposed

<table>
<tr>
<td width="44%">
<a href="https://www.youtube.com/watch?v=ERFeOgqk324">
<img src="https://img.youtube.com/vi/ERFeOgqk324/maxresdefault.jpg" alt="I Redesigned VLC After 25 Years, by Juxtopposed (YouTube)">
</a>
</td>
<td>

**Every screen of this interface was designed by Juxtopposed.**
This project only turns that design into working software.

▶ **[Watch the video: *I Redesigned VLC After 25 Years*](https://www.youtube.com/watch?v=ERFeOgqk324)**

[YouTube](https://www.youtube.com/@juxtopposed) ·
[Website](https://juxtopposed.com) ·
[Figma](https://www.figma.com/@juxtopposed) ·
[X / Twitter](https://twitter.com/juxtopposed) ·
[Bluesky](https://bsky.app/profile/juxtopposed.bsky.social) ·
[Instagram](https://instagram.com/juxtopposed) ·
[GitHub](https://github.com/juxtopposed)

If you like how this looks, go subscribe to Juxtopposed's channel. That's where it all comes from.

</td>
</tr>
</table>

> The design is **["VLC Redesign (Community)"](https://www.figma.com/community/file/1691465999134370980) by Juxtopposed**,
> licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
> It was adapted to Qt/QML for this build, and its icons were redrawn and modified.
> Juxtopposed does not endorse this project.

<p align="center"><img src="docs/media/about-credit.png" alt="The design credit in the app's About window" width="640"><br>
<sub>The same credit is in the app, under <b>⋯ → About VLC</b>.</sub></p>

---

> [!IMPORTANT]
> **This is an unofficial fork.** It is not made, affiliated with, or endorsed by
> [VideoLAN](https://www.videolan.org/). *VLC*, *VLC media player* and the cone logo are
> trademarks of VideoLAN. For the official player, go to [videolan.org](https://www.videolan.org/vlc/).
> This build is based on the VLC 4.0 development branch: please report problems
> [here](https://github.com/KuchiSofts/vlc-redesign/issues), not to VideoLAN.

## What it is

The real VLC: same engine, same codecs, same "plays everything". Only the interface is new, and it
follows the design screen by screen, down to the pixel on the screens that were measured.

| | |
|---|---|
| **Player** | Framed video with rounded corners, a waveform seek bar for music, frame previews when hovering the timeline, the cone volume control (up to 200 %), skip ±15 s (and optional ±10 s buttons), picture-in-picture, sleep timer, A-B loop |
| **Queue & library** | A queue card with Queue / Bookmarks / Chapters tabs, a compact library over the video, *Listen Again* and *Frequently Visited Folders* on Home, folder browsing with breadcrumbs, playlists you can favourite, shuffle and export |
| **Customise** | Drag and drop the control bar directly on the player ([see how](#customize-the-interface)), save layouts as profiles, Dark / Light / System themes with 9 accent colours |
| **Settings** | A single redesigned settings window with search, the design's toggles, dropdowns and sliders, and many more VLC options exposed than before |
| **Subtitles** | Per-file and per-series subtitle choices remembered, a sync timeline, external subtitle and audio tracks, OpenSubtitles (with your own API key) |
| **Onboarding** | A three-step welcome: media folder, theme, keyboard shortcuts |

See the full list in the **[changelog](CHANGELOG.md)**.

## Screenshots

| | |
|---|---|
| ![Home with Continue Watching and Listen Again](docs/media/home.png) | ![Frame preview while hovering the seek bar](docs/media/player-seek-preview.png) |
| **Home**: Continue Watching, Listen Again, Frequently Visited Folders | **Player**: framed video and frame previews on the seek bar |
| ![Queue card](docs/media/queue.png) | ![Light theme with a blue accent](docs/media/light-theme.png) |
| **Queue card** with Bookmarks and Chapters tabs | **Light theme**, here with a blue accent |
| ![Music view with the waveform seek bar](docs/media/music-waveform.png) | ![Settings](docs/media/settings.png) |
| **Music**: blurred artwork backdrop and a waveform seek bar | **Settings**, redesigned with search |

The demo uses free media: *[Sintel](https://durian.blender.org/)* and *[Big Buck Bunny](https://peach.blender.org/)* trailers
(© Blender Foundation, [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/); Sintel's first second trimmed) and
"Fluffing a Duck" and "Carefree" by [Kevin MacLeod](https://incompetech.com/) ([CC BY 4.0](https://creativecommons.org/licenses/by/4.0/); generated cover art added).

## Colours and themes

Nine accent colours, in the dark or the light theme, or following Windows: **Settings → General → Appearance**.

<p align="center"><img src="docs/media/accents-dark.png" alt="The player in the nine accent colours of the design, dark theme" width="820"></p>
<p align="center"><img src="docs/media/accents-light.png" alt="The player in the light theme, with three of the accent colours" width="820"></p>

## Customize the interface

<p align="center"><a href="docs/media/customize.mp4"><img src="docs/media/customize.gif" alt="Customizing the player: switching profiles, creating one, dragging a button off the control bar and two onto it, then saving" width="800"></a><br>
<sub>▶ <a href="docs/media/customize.mp4">Watch it in full quality (MP4, 44 seconds)</a></sub></p>

The control bar is edited right on the player:

1. Open **⋯ → Customize Player**. From the library, it switches to the player first.
2. Pick a profile at the top (**Default**, **Movies**, **Shows**, **Music**, **Work**), or press **New** to start your own.
   **Rename**, **Clone** and **Delete** manage them.
3. **Drag a button from the grid onto the bar.** An orange line shows where it goes: before or after a button, or between two.
4. **Drag a button off the bar** into the grid to remove it, or along the bar to move it.
5. **Save** keeps the changes. **Cancel** or Escape drops them.

Switch profiles any time with **⋯ → Profile**. The fullscreen button sits at the right end of the bar. Move it or take
it out the same way: F, F11 and a double-click on the video still toggle fullscreen.

## Settings and smart features

One redesigned settings window, with search, in thirteen sections: General, Interface, Privacy, Controls, Playback,
Video, Audio, Subtitles, Library, Network, Performance, Recording and Extensions.
Every option is explained in the **[wiki](https://github.com/KuchiSofts/vlc-redesign/wiki)**.

<table>
<tr>
<td width="50%"><img src="docs/media/settings-subtitles.png" alt="Settings: Subtitles"></td>
<td width="50%"><img src="docs/media/settings-playback.png" alt="Settings: Playback"></td>
</tr>
<tr>
<td><b>Subtitles</b>: languages, size and style, encoding, automatic loading</td>
<td><b>Playback</b>: resume, speed, repeat, what happens at the end</td>
</tr>
</table>

**Smart by default**

- **Subtitles remembered**: the subtitle track you pick stays with the file, and with the episodes of a series; external
  subtitle files next to the video load by themselves; a sync timeline in the tracks menu fixes early or late subtitles,
  with presets per series. OpenSubtitles search with your own API key.
- **Continue where you left off**: a small card offers to resume, or resume always or never (Settings → Playback).
- **Frame previews** on the seek bar, decoded ahead while you hover it, and a **waveform seek bar** for music.
- **Sleep timer** (stop or pause), **A-B loop**, playback speed, **picture-in-picture**, a mini player with its queue.
- **Startup volume** and volume up to 200 %, audio and video effects that apply live and can be undone.
- **Small updates from GitHub**: only what changed, checksum-verified, put in place in a restart of two seconds that
  reopens where you were (installed and portable copies).

## Download

**Platforms:** Windows 0.3.0 ✅ · Linux 🔜 · Samsung TV 🔜 · macOS, Windows ARM64, Android, iPhone 💡 — the
list and its status are in [`platforms.yml`](platforms.yml).

Go to **[Releases](https://github.com/KuchiSofts/vlc-redesign/releases/latest)** and pick one:
Windows 10 or 11, 64-bit. Windows files: [Code signing policy](https://github.com/KuchiSofts/vlc-redesign/blob/main/CODE_SIGNING_POLICY.md).

| | |
|---|---|
| **Installer** · `vlc-redesign-…-win64-setup.exe` | Recommended. Installs for you only (no administrator rights needed), adds a Start menu entry, and keeps itself up to date. Its *Options* page can open your videos, music and playlists with VLC Redesign and add *Play with VLC Redesign* to the right-click menu. |
| **Portable** · `vlc-redesign-…-win64.zip` | Unzip anywhere and run `vlc.exe`. Nothing is installed; delete the folder to remove it. |

Both keep their settings, library and history in their own `portable` folder, so they never touch an
installed official VLC. Requirements: Windows 10 or 11, 64-bit.

### The installer

<p align="center"><img src="docs/media/installer.png" alt="The installer: Welcome with the design credit, Options with the toggles, the installation, Done" width="820"></p>

Drawn like the app itself, in English or Hebrew. Three clicks from *Welcome* to *Done*, and on the way:

- **Open videos, music and playlists with VLC Redesign**: it becomes the player of the formats VLC plays, wherever you
  haven't picked another app yourself. Windows keeps your own choices, so *Done* has a button to Windows' *Default apps*
  to choose VLC Redesign there too.
- **Play with VLC Redesign** and **Add to VLC Redesign queue** in the right-click menu of media files and folders.
- **Desktop shortcut**, and **Install for everyone** (Program Files, asks for administrator rights).

An update keeps your choices. Uninstalling removes all of it and gives every file type back to the app it had before.

### Updates

VLC Redesign checks this repository for a new release once a day (Settings → General → *Check for updates
automatically*, or ⋯ → About → *Check for updates*). When one is out, a card offers **Update now**:

- **A small update** (installed or portable copy): only the files that changed, usually a few megabytes or less,
  downloaded in the background and checked against the release's `SHA256SUMS.txt` and file by file. Then
  **Restart now** puts it in place in about two seconds and reopens VLC Redesign on what was playing, where it was,
  or **Later** puts it in place when you close it. If anything goes wrong, every file goes back as it was.
- **The installer**, when no small update fits (more than three releases behind, or installed for everyone):
  downloaded, checked, and run without questions; it reopens VLC Redesign where you were.
- **Download** (a portable copy too far behind): opens the [latest release](https://github.com/KuchiSofts/vlc-redesign/releases/latest).

Running the installer by hand works too: it closes a running VLC Redesign politely and reopens it afterwards.

> [!NOTE]
> Windows SmartScreen may warn about an unknown publisher, because the build isn't code-signed.
> Click *More info → Run anyway*, or build it yourself from source.
> Every release lists the SHA-256 of its files in `SHA256SUMS.txt`.
> Signed releases are coming: see the [code signing policy](CODE_SIGNING_POLICY.md).

## Known limitations

- **Based on VLC 4.0-dev**, not on the stable VLC 3. Expect the occasional rough edge.
- **Windows only for now.** The first milestones were also validated on Linux, but the latest work has only been tested on Windows. No macOS build.
- **72 languages cover the whole interface**; in VLC's other catalogs (listed as partial) only VLC's own texts are
  translated and the redesign's are in English. Anyone can help: see [Translating](docs/TRANSLATING.md).
- Media Info, Bookmarks, Convert/Stream, Open Disc, Capture and Messages still use the classic VLC windows.
- A few controls in the design have no VLC equivalent and were left out on purpose.
- The waveform seek bar works on local files up to 1 GB and 8 minutes; other media get the regular seek bar.
- HDR output offers Auto / Off / HDR10 / HLG and needs the libplacebo renderer.

The detailed status of every screen is in [`.specs/`](.specs/project/STATE.md) (in Portuguese, from the first implementation).

## Build from source

The Windows build is a cross-compile in Docker, using VideoLAN's official build image and prebuilt
libraries. You need [Docker Desktop](https://www.docker.com/products/docker-desktop/) and Git.

```powershell
git clone https://github.com/KuchiSofts/vlc-redesign.git
cd vlc-redesign
# Development build (debug, asserts on) into %USERPROFILE%\source\vlc-win64
.\extras\juxtopposed\win64-docker.ps1
# Release: build, then the portable zip, the installer and SHA256SUMS.txt
# (needs Inno Setup 6.7+: choco install innosetup)
.\extras\juxtopposed\make-release.ps1
```

The first build takes a while; later ones only rebuild what changed (`-Quick` skips configure).
Linux builds use `extras/juxtopposed/linux.sh` (see [foundation.md](.specs/features/redesign/foundation.md)).
The interface lives in [`modules/gui/qt`](modules/gui/qt).

## Staying in sync with VLC

This repository is a fork of [VLC](https://code.videolan.org/videolan/vlc), forked from the 4.0 development branch
on 2026-10-12. Fixes from VLC are merged in regularly: a weekly GitHub Action tries the merge and opens a pull request
when it applies cleanly, or an issue listing the conflicting files when it doesn't.

## Credits

- **Design:** [Juxtopposed](https://www.youtube.com/@juxtopposed). [Video](https://www.youtube.com/watch?v=ERFeOgqk324) · [Figma file](https://www.figma.com/community/file/1691465999134370980) (CC BY 4.0)
- **First implementation:** [MrDouglax](https://github.com/SrDouglax/JuxtopposedVLC), whose Linux fork this builds on
- **VLC:** the [VideoLAN](https://www.videolan.org/) team and the VLC contributors ([AUTHORS](AUTHORS))
- **Fonts:** [Inter](https://github.com/rsms/inter) and [DM Mono](https://github.com/googlefonts/dm-mono), SIL Open Font License 1.1

Details in [CREDITS.md](CREDITS.md).

## Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io/), certificate by
[SignPath Foundation](https://signpath.org/). What is signed, the team roles and the privacy policy:
[CODE_SIGNING_POLICY.md](CODE_SIGNING_POLICY.md). Until the first signed release, the files are unsigned and their
SHA-256 is listed in `SHA256SUMS.txt`.

## License

Like VLC, this is free software under the **GNU GPL version 2 or later** ([COPYING](COPYING)); libVLC is
under the LGPL 2.1 or later ([COPYING.LIB](COPYING.LIB)). The design is CC BY 4.0, and the fonts are OFL 1.1.
Every release links to the exact source it was built from.

---

<details>
<summary><b>The original VLC README</b></summary>

# VLC media player

**VLC** is a libre and open source **media player** and **multimedia engine**,
focused on **playing everything**, and **running everywhere**.

**VLC** can play most multimedia files, discs, streams, devices and is also able to
convert, encode, **stream** and manipulate streams into numerous formats.

VLC is used by many over the world, on numerous platforms, for very different use cases.

The **engine of VLC** can be embedded into 3rd party applications, and is called *libVLC*.

**VLC** is part of the [VideoLAN project](https://videolan.org) and
is developed and supported by a community of volunteers.

The VideoLAN project was started at the university [École Centrale Paris](https://www.centralesupelec.fr/) who
relicensed VLC under the GPLv2 license in February 2001. Since then, VLC has
been downloaded **billions** of times.

## License

**VLC** is released under the GPLv2 *(or later)* license.
*On some platforms, it is de facto GPLv3, because of the licenses of dependencies*.

**libVLC**, the engine is released under the LGPLv2 *(or later)* license. \
This allows embedding the engine in 3rd party applications, while letting them to be licensed under other licenses.

# Platforms

VLC is available for the following platforms:
- [Windows] *(from 7 and later, including UWP platforms and all versions of Windows 10)*
- [macOS] *(10.10 and later)*
- [GNU/Linux] and affiliated
- [BSD] and affiliated
- [Android] *(4.2 and later)*, including Android TV and Android Auto
- [iOS] *(9 and later)*, including AppleTV and iPadOS
- Haiku, OS/2 and a few others.

[Windows]: https://www.videolan.org/vlc/download-windows.html
[macOS]: https://www.videolan.org/vlc/download-macosx.html
[GNU/Linux]: https://www.videolan.org/vlc/#download
[BSD]: https://www.videolan.org/vlc/download-freebsd.html
[Android]: https://www.videolan.org/vlc/download-android.html
[iOS]: https://www.videolan.org/vlc/download-ios.html

Not all platforms receive the same amount of care, due to our limited resources.

**Nota Bene**: The [Android app](https://code.videolan.org/videolan/vlc-android/) and
the [iOS app](https://code.videolan.org/videolan/vlc-ios/) are located in different repositories
than the main one.

# Contributing & Community

**VLC** is maintained by a community of people, and VideoLAN is not paying any of them.\
The community is composed of developers, helpers, maintainers, designers and writers that want
this open source project to thrive.

The main development of VLC is done in the C language, but this repository also contains
plenty of C++, Obj-C, asm and Rust.

Other repositories linked to vlc are done in languages including Kotlin/Java [(Android)](https://code.videolan.org/videolan/vlc-android/),
Swift [(iOS)](https://code.videolan.org/videolan/vlc-ios/), and C# [(libVLCSharp)](https://code.videolan.org/videolan/libvlcsharp/).

We need help with the following tasks:
- Coding
- Packaging for Windows, macOS and Linux distributions
- Technical writing for the documentation
- Design
- Support
- Community management and communication.

Please contribute :)

We are on IRC. You can find us on the **#videolan** channel on *[Libera.chat]*.

[Libera.chat]: https://libera.chat

## Contributions

Contributions are now done through Merge Requests on our [GitLab repository](https://code.videolan.org/videolan/vlc/).

CI and discussions should be resolved before a Merge Request can be merged.

# libVLC

**libVLC** is an embeddable engine for 3rd party applications and frameworks.

It runs on the same platforms as VLC *(and sometimes on more)* and can provide playback,
streaming and conversion of multimedia files and streams.


**libVLC** has numerous bindings for other languages, such as C++, Python and C#.

# Support

## Links

Some useful links that might help you:

- [VLC web site](https://www.videolan.org/vlc/)
- [Support](https://www.videolan.org/support/)
- [Forums](https://forum.videolan.org/)
- [Wiki](https://wiki.videolan.org/)
- [Developer's Corner](https://wiki.videolan.org/Developers_Corner)
- [VLC hacking guide](https://wiki.videolan.org/Hacker_Guide)
- [Bugtracker](https://code.videolan.org/videolan/vlc/-/issues)
- [VideoLAN web site](https://www.videolan.org/)

## Source Code sitemap
```
ABOUT-NLS          - Notes on the Free Translation Project.
AUTHORS            - VLC authors.
COPYING            - The GPL license.
COPYING.LIB        - The LGPL license.
INSTALL            - Installation and building instructions.
NEWS               - Important modifications between the releases.
README             - Project summary.
THANKS             - VLC contributors.

bin/               - VLC binaries.
bindings/          - libVLC bindings to other languages.
compat/            - compatibility library for operating systems missing
                     essential functionalities.
contrib/           - Facilities for retrieving external libraries and building
                     them for systems that don't have the right versions.
doc/               - Miscellaneous documentation.
extras/analyser    - Code analyser and editor specific files.
extras/buildsystem - Different build system specific files.
extras/misc        - Files that don't fit in the other extras/ categories.
extras/package     - VLC packaging specific files such as spec files.
extras/tools/      - Facilities for retrieving external building tools needed
                     for systems that don't have the right versions.
include/           - Header files.
lib/               - libVLC source code.
modules/           - VLC plugins and modules. Most of the code is here.
po/                - VLC translations.
share/             - Common resource files.
src/               - libvlccore source code.
test/              - Testing system.
```

</details>
