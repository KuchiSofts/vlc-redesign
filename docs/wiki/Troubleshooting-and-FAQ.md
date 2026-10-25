# Troubleshooting and FAQ

> Casting problems have their own page: [Casting: when a TV does not play](Casting-Troubleshooting).

## Which Windows does it run on?

**Windows 10 version 1809 or later, or Windows 11, 64-bit.** The interface is built on Qt 6.8, which needs them; the
installer refuses an older Windows, and a portable copy on one says so when it starts.

There is no build for Windows XP, Vista, 7, 8 or 8.1, nor for 32-bit Windows or Windows on ARM. Linux, macOS and
the other systems are not available yet (the list of platforms and their status is in
[`platforms.yml`](https://github.com/KuchiSofts/vlc-redesign/blob/main/platforms.yml)). For those, use the official
[VLC](https://www.videolan.org/vlc/).

## "Windows protected your PC" (SmartScreen)

The releases are not code-signed yet, so Windows SmartScreen may warn about an unknown publisher the first time you
run the installer or `vlc.exe`. Click **More info → Run anyway**.

- Code signing is in progress: free code signing provided by [SignPath.io](https://about.signpath.io/), certificate
  by [SignPath Foundation](https://signpath.org/). Once releases are signed, Windows will show *SignPath Foundation*
  as the publisher. See the
  [code signing policy](https://github.com/KuchiSofts/vlc-redesign/blob/main/CODE_SIGNING_POLICY.md).
- Until then, every release lists the SHA-256 of its files in `SHA256SUMS.txt`. To check a download, in PowerShell:
  `Get-FileHash .\vlc-redesign-0.3.1-win64-setup.exe` and compare the hash with the one in the file.
- You can also build it yourself from the source code.

## Is it the official VLC? Does it replace mine?

No. VLC Redesign is an unofficial fork of VLC 4 (development version) with the interface designed by Juxtopposed;
it is not made, affiliated with, or endorsed by VideoLAN. It installs **next to** an official VLC without touching
it: its own folder, its own settings, library and history, its own volume in Windows. Report its problems
[here](https://github.com/KuchiSofts/vlc-redesign/issues), not to VideoLAN.

## Where are my settings?

Everything is in the **`portable` folder next to `vlc.exe`**:

| Copy | Folder |
|---|---|
| Installed for you (default) | `%LOCALAPPDATA%\Programs\VLC Redesign\portable` |
| Installed for everyone | `C:\Program Files\VLC Redesign\portable` |
| Portable zip | the `portable` folder of the unzipped copy |

In it: `vlcrc` (VLC's settings), `vlc-qt-interface.ini` (the interface's own settings: theme, accent, profiles,
subtitle choices...), `ml.db` (the media library), `language` (the interface language), and the caches (`qtcache`,
downloaded subtitles). Where VLC Redesign cannot make that folder, it uses `%APPDATA%\VLC Redesign`.

**⋯ → Settings → General → Advanced Options → Open configurations folder** opens it.

A new copy whose `portable` folder is empty starts with the settings of the copy you used last (see
[Install and update](Install-and-Update#the-portable-zip)). An empty file named `standalone` in a copy's `portable`
folder keeps it apart. VLC Redesign never reads or changes the settings of an official VLC (`%APPDATA%\vlc`).

## Reset the settings

- **⋯ → Settings → General → Advanced Options → Reset all settings** (*Reset*): every VLC option and every
  interface setting goes back to its default. The language goes back to English.
- **Reset all shortcuts** (*Settings → Controls*) resets the keyboard shortcuts alone.
- To start from nothing: close VLC Redesign, then empty its `portable` folder **and** put an empty file named
  `standalone` in it. Without that file, an empty profile takes the settings of your other copy again.

Uninstalling removes the settings with the program (see [Install and update](Install-and-Update#uninstalling)).

## Save a log

To find out what goes wrong, have VLC write its messages to a file:

1. **⋯ → Settings → General → Diagnostics**: turn on **Save a log**. **Log file** chooses where (empty: `vlc-log.txt`),
   and **Log details** how much (*Everything (debug)* for a bug report). It applies when Settings closes.
2. Do what goes wrong, then close VLC Redesign.
3. Attach the file to your report.

Each line is written to the file at once, so the log keeps its last lines even when VLC crashes: those lines say
where it happened. The log adds to the end of the file; delete it to start a new one.

**Messages** (<kbd>Ctrl</kbd>+<kbd>M</kbd>, or the right-click menu → *More Settings*) shows the same messages live.

## Video problems: no picture, black screen, stutter, wrong colours

VLC Redesign decodes video on the graphics card when it can (*Hardware-accelerated decoding: Automatic*,
[Settings: Performance](Settings-Performance)).

- **Update the graphics driver** first (NVIDIA, AMD or Intel): most picture problems come from an old one.
- **Try without hardware decoding**: set *Hardware-accelerated decoding* to **Disabled**, then play the file again.
  If it plays right, the driver is the cause.
- **The video output**: leave *Video output module* ([Settings: Video](Settings-Video), Developer mode) on its
  default. Direct3D 9 and OpenGL cannot show the picture in this interface; when one is chosen, VLC tries the outputs
  that work.
- **HDR**: the HDR formats need the libplacebo renderer and a display and driver that take HDR; *Auto* or *Off* in
  the HDR panel go back to the usual output, from the next media (see [The player](The-Player#hdr)).
- **A slow PC**: [Settings: Performance](Settings-Performance) has the options that keep playback smooth (skipping
  the H.264 loop filter, dropping late frames, the decoder threads).
- VLC Redesign also runs without a graphics card (it was checked in Windows Sandbox), decoding on the processor.

## Other questions

**A file opened from Explorer does not play.** It plays at once in a running VLC Redesign, in front of the other
windows. If VLC Redesign stopped answering, the file opens in a new one.

**The language changed back to English.** VLC Redesign starts in English until you choose a language in
*Settings → General → Interface language*; *Reset all settings* goes back to English. See [Languages](Languages).

**Fullscreen does not end.** <kbd>Esc</kbd>, <kbd>F</kbd> or <kbd>F11</kbd> leave it, from any view.

**The control bar lost a button.** Open **⋯ → Customize Player** and drag it back from the palette: the bar's own
controls (Play, Volume, Fullscreen…) are at its end. Or switch profile (**⋯ → Profile**). See
[Customize the player](Customize-the-Player#the-bars-own-controls).

**Subtitles do not show.** Open **Languages and tracks → Subtitles**: pick the language and the subtitle, or let the
online search find one. See [Subtitles](Subtitles).

**No sound, or the wrong device.** **Languages and tracks → Audio → Output Device**, with its refresh button; or
[Settings: Audio](Settings-Audio). When Windows' default device changes while playing, the sound moves to it after about a fifth of a second, in sync
with the picture.

**An update failed.** The card says why, with **Retry** and **Release page**; nothing is left half done (see
[Install and update](Install-and-Update#small-updates)).

## Report a bug

Open an issue on [GitHub](https://github.com/KuchiSofts/vlc-redesign/issues/new/choose) and pick a form:

| Form | For |
|---|---|
| **Bug report** | Something that doesn't work as it should. |
| **Design mismatch** | A screen or control that differs from the design by Juxtopposed. |
| **Suggestion** | An idea, in the design or new. |
| **Translation** | A wrong, awkward or missing text in your language, or a new language. |
| **TV report** | How casting to your TV or Chromecast went (it fills [TV compatibility](TV-Compatibility)). |

A bug report asks for:

- the **version** (**⋯ → About VLC** shows it);
- how you installed it (installer, portable zip, built from source) and your **Windows version** (*Windows 11 24H2*);
- what happened, what you expected, and the **steps** to make it happen again;
- for playback, the **media**: format, codec, resolution, or a link to a sample (no copyrighted files);
- screenshots or a short recording, and the [log](#save-a-log) if you have one.

Problems with the official VLC go to [VideoLAN](https://www.videolan.org/vlc/).

---

[Install and update](Install-and-Update) · [Settings: General](Settings-General) · [Casting: when a TV does not play](Casting-Troubleshooting) · [Home](Home)
