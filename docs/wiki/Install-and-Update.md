# Install and update

VLC Redesign comes as an **installer** and as a **portable zip**, both on the
[latest release](https://github.com/KuchiSofts/vlc-redesign/releases/latest). It needs **Windows 10 (version 1809 or
later) or Windows 11, 64-bit**. It installs next to an official VLC without touching it: its settings, library and
history stay in its own `portable` folder. This page goes through the installer page by page, the switches of a
silent install, the portable copy, the updates and uninstalling.

## The release files

The files of version 0.4.0 ([release page](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v0.4.0)):

| File | Size | What it is |
|---|---|---|
| `vlc-redesign-0.4.0-win64-setup.exe` | @SETUP@ | **The installer.** Recommended: no administrator rights, a Start menu entry, file types, small updates. |
| `vlc-redesign-0.4.0-win64.zip` | @ZIP@ | **The portable copy**: unzip anywhere and run `vlc.exe`. |
| `vlc-redesign-0.4.0-delta-from-0.3.1.vrdelta` | @D031@ | A small update from 0.3.1, used by the app itself (see [Small updates](#small-updates)). |
| `vlc-redesign-0.4.0-delta-from-0.3.0.vrdelta`, `...-from-0.2.6.vrdelta` | @DOLD@ | The small updates from 0.3.0 and 0.2.6. |
| `SHA256SUMS.txt` | | The SHA-256 of every file of the release, to check a download. |
| `SHA256SUMS.txt.sig` | | The signature of `SHA256SUMS.txt`: from 0.4.0 on, the app installs only an update whose list is signed. |

You never need to download a `.vrdelta` file yourself: the app picks the one for your version.

> **Good to know.** Windows SmartScreen may warn about an unknown publisher the first time, because the releases are
> not code-signed yet: see
> [Troubleshooting and FAQ](Troubleshooting-and-FAQ#windows-protected-your-pc-smartscreen). To check a download, in
> PowerShell: `Get-FileHash .\vlc-redesign-0.4.0-win64-setup.exe`, and compare with `SHA256SUMS.txt`.

## The installer

Drawn like the app: the steps in a sidebar card on the left (**Welcome, Options, Privacy, Install, Done**), Inter,
the ON / OFF toggles of the settings, the orange accent, always dark. A new install takes a few clicks and about a
minute.

| Page | Shown | What it asks |
|---|---|---|
| [Welcome](#welcome) | Always | Nothing: the credit, the notice, the version |
| [Options](#options) | Always | The folder, file types, right-click menu, desktop shortcut |
| [Languages](#languages) | Once per copy (a new install, or a copy with no language yet) | Interface and subtitle language; right to left |
| [Privacy](#privacy) | Once per copy (skipped when the choices were made before) | The update check and online subtitles |
| [Installing](#installing-and-done) | Always | Nothing: the progress and a tip |
| [Done](#installing-and-done) | Always | Default apps; open VLC Redesign now |

![The installer's Welcome page, with the design credit](images/installer-welcome.png)

### Welcome

*Welcome to VLC Redesign. The real VLC 4 engine with the interface designed by Juxtopposed. Setup takes about a
minute.*

- A card **Interface designed by Juxtopposed**: *Every screen comes from the "VLC Redesign (Community)" Figma file,
  licensed under CC BY 4.0. Juxtopposed does not endorse this project.*, with links to **Watch the video**, the
  **YouTube channel** and the **Figma file**.
- The notice: *An unofficial build, not made or endorsed by VideoLAN. VLC and the cone logo are trademarks of
  VideoLAN. It installs next to an official VLC without touching it.*
- Links to **Notice and licenses** (shown in the installer) and **Source code**.
- *Version 0.4.0 · free software under the GNU GPL v2 or later.*

### Options

*Where it goes, and what it opens.*

| Row | What it does | New install |
|---|---|---|
| **Install to** + **Browse** | The folder. Default: `%LOCALAPPDATA%\Programs\VLC Redesign` (*For you only, without administrator rights.*). An update shows *Updates the copy in this folder.* | |
| **Install for everyone** | A link under the folder: Setup starts again with administrator rights (`/ALLUSERS`) to install in `C:\Program Files\VLC Redesign` (*For everyone on this PC.*). | |
| **Open videos with VLC Redesign** | Makes it the player of the video types VLC plays (64 types: MP4, MKV, AVI, WebM, MOV, TS...), wherever you have not chosen another app yourself. | ON |
| **Open music with VLC Redesign** | The same for music (51 types: MP3, FLAC, M4A, Opus, WAV...). | ON |
| **Open playlists with VLC Redesign** | The same for playlists (14 types: M3U, M3U8, PLS, XSPF, CUE...). | ON |
| **Play with VLC Redesign in right-click menus** | Adds *Play with VLC Redesign* and *Add to VLC Redesign queue* to the right-click menu of media files and of folders. | ON |
| **Desktop shortcut** | A shortcut on the desktop. | ON |

VLC Redesign is always offered in *Open with* and in Windows' *Default apps* for the types VLC plays, whatever you
choose. Windows keeps the default apps you chose yourself: Setup never takes a type away from another app you
picked. An update shows the choices of the last install. A folder that is not a full path (such as `C:\...` or
`\\server\...`) is refused with *Choose a folder to install to.*

### Languages

*The language of the app and of your subtitles.* Shown once per copy: on a new install, and on an update of a copy
that has no interface language yet (an install from before 0.3.1). A copy that has a language keeps it.

![The installer's Languages page: interface language and subtitle language](images/installer-languages.png)

| Row | What it does | To begin with |
|---|---|---|
| **Interface language** | The language of the app: *English*, *English (United Kingdom)* and the 72 other complete languages, each by its own name. | Setup's language (or `/UILANG=`) |
| **Subtitle language** | The default language of the subtitles. *Subtitles in this language show by themselves: from the video, a file next to it, or online. Change both any time in Settings.* | The interface language, until you choose another |
| **Right-to-left layout** | Only shown when the interface language is written right to left: **Hebrew, Arabic, Persian, Urdu or Central Kurdish**. *The sidebar on the right and the controls in reverse order, for reading from right to left. Off: the design's layout, from left to right.* | ON (OFF with `/LEFTTORIGHT`) |

All three can be changed later in **Settings → General → Appearance** (interface language, layout direction) and
**Settings → Subtitles** (the subtitle language): see [Languages](Languages) and
[Settings: Subtitles](Settings-Subtitles).

### Privacy

*Two features go online by themselves. Each sends only this:*

| Toggle | What it sends | New install |
|---|---|---|
| **Check for updates once a day** | *A request for the latest release, to GitHub (api.github.com).* | ON |
| **Find subtitles online automatically** | *The film's name and a fingerprint of the file, to subtitle services (OpenSubtitles.org, Wizdom, Gestdown, YIFY); its name to IMDb, Wikidata and Cinemeta.* | ON |

A card says *No telemetry, no ads, no account. Nothing else goes online unless you ask for it. Change both later in
Settings.*, with a link to the **Privacy and code signing policy**. An update skips the page when the choices were
made before (in Setup or in the welcome steps of the app). Both stay in Settings:
[General](Settings-General#startup) for the updates, [Subtitles](Settings-Subtitles) for the subtitles. The answers
are written to `portable\setup-choices.ini`, which VLC Redesign reads into its settings at its next start and then
deletes; the welcome steps then do not ask them again.

### Installing and Done

*Installing. This takes about a minute.*, with a progress bar and a tip: *Make the player yours. Drag buttons into
and out of the control bar with ⋯ → Customize Player, and keep your layouts as profiles.*

If VLC Redesign is open from the folder being installed to, Setup says *VLC Redesign is open. Setup will close it now
and open it again when it's done.* It asks VLC to close, ends it after 15 seconds if it has not, and opens it again
afterwards on what was playing. If it still does not close: *VLC Redesign didn't close. Close it and click Retry.*

*VLC Redesign is ready. Find it in the Start menu.*

- A card **Make it your default player**: *Windows keeps the apps you chose before. Open Default apps and choose VLC
  Redesign for your media.*, with the button **Open Default apps**.
- **Open VLC Redesign now** (ON): starts it when you click Finish.
- *Enjoy the design by Juxtopposed.* and a **Subscribe on YouTube** link.

At the end Setup also starts VLC Redesign and its helpers once, hidden, so that Windows' first check of a new
program happens then and not when you open your first file.

### The language of Setup

Setup starts in English. When Windows is shown in one of the languages Setup speaks, it first asks, in that
language and in English side by side (*Windows is in German. Install VLC Redesign in German?*); **Yes** starts Setup
again in it, and the app is then installed in that language too. Hebrew and Arabic are laid out right to left.

Setup speaks **29 languages**: English, Arabic, Armenian, Brazilian Portuguese, Bulgarian, Catalan, Czech, Danish,
Dutch, Finnish, French, German, Hebrew, Hungarian, Italian, Japanese, Korean, Norwegian, Polish, Portuguese, Russian,
Slovak, Slovenian, Spanish, Swedish, Tamil, Thai, Turkish and Ukrainian. The app itself speaks more (see
[Languages](Languages)): the *Interface language* list offers all of them, whatever the language of Setup. An update
and the uninstaller run in the language of the install.

### Per user or for everyone

| | For you (default) | For everyone |
|---|---|---|
| Folder | `%LOCALAPPDATA%\Programs\VLC Redesign` | `C:\Program Files\VLC Redesign` |
| Administrator rights | No | Yes (asked when you click *Install for everyone*) |
| Settings | In the `portable` folder of the install | In the `portable` folder of the install, writable by every user (one profile shared by all) |
| File types | For your Windows account | For every account |
| Updates | Small updates | The installer (it needs the rights) |

An update keeps the mode of the install.

### Silent install

The standard switches of Inno Setup work (`/SILENT`, `/VERYSILENT`, `/SUPPRESSMSGBOXES`, `/DIR="..."`, `/ALLUSERS`,
`/LOG="..."`). A silent install asks nothing: a new copy starts in English, with both privacy features on; a copy
that already has settings keeps them. These switches change that:

| Switch | What it does |
|---|---|
| `/UILANG=he` | The interface language of the app: a code of its languages (`he`, `de`, `pt_BR`, `en_GB`, `zh_CN`...). A copy that has a language already keeps it. |
| `/SUBLANG=en` | The default language of the subtitles (the same codes). |
| `/LANG=hebrew` | Setup in that language, and the app too (a copy that has a language keeps it). The names: `english`, `hebrew`, `arabic`, `german`, `french`, `brazilianportuguese`, `portuguese`... |
| `/RIGHTTOLEFT` | The app laid out right to left. |
| `/LEFTTORIGHT` | The app laid out left to right. Without either, the app follows its language. |
| `/NOUPDATECHECK` | No daily update check. |
| `/NOONLINESUBS` | No automatic online subtitles. |

Examples:

```
vlc-redesign-0.4.0-win64-setup.exe /VERYSILENT /UILANG=he /SUBLANG=en
vlc-redesign-0.4.0-win64-setup.exe /VERYSILENT /ALLUSERS /NOUPDATECHECK /NOONLINESUBS
```

On a Setup that shows its pages, `/UILANG=` and `/SUBLANG=` preselect the lists, `/LEFTTORIGHT` starts the
right-to-left toggle off, and `/NOUPDATECHECK` and `/NOONLINESUBS` start their toggles off.

## The portable zip

1. Unzip it anywhere (a USB drive works).
2. Run `vlc.exe`.

Nothing is installed: the settings, library and history go in the `portable` folder next to `vlc.exe`. It starts in
English, left to right, and its welcome steps ask for a media folder, the theme and accent, the subtitle languages
and the two online features, and show the basic keys. File types are not registered; use *Open with* in Explorer, or
the installer, for that. To remove it, delete its folder.

### Settings carried over

A new copy (installed or portable) whose `portable` folder is still empty starts with the settings of the copy you
used last: settings, theme and accent, library, playlists, history and downloaded subtitles. The copy it came from
stays as it was. Each copy notes its profile under `HKCU\Software\VLC Redesign\Profiles` for that.

> **Good to know.** An empty file named `standalone` in a copy's `portable` folder keeps that copy to itself: it
> neither takes the settings of another copy nor gives its own.

## Small updates

VLC Redesign looks for a new release at most once a day, a few seconds after it starts (**Check for updates
automatically**, [Settings: General](Settings-General#startup), on by default), and whenever you click **Check for
updates** in **⋯ → About VLC**. When one is out, a card offers it:

| Card | Buttons |
|---|---|
| *VLC Redesign 0.4.0 is available* · *You have 0.3.1.* | **What's new**, **Skip**, **Update now · @D031@** (or **Download**) |
| *Downloading VLC Redesign 0.4.0… 40%* · *Only what changed: keep watching, it's checked in the background.* | **×** (cancels) |
| *Preparing VLC Redesign 0.4.0…* | |
| *VLC Redesign 0.4.0 is ready* · *It takes a few seconds: restart now, or it's done when you close VLC Redesign.* | **Restart now**, **Later** |
| *Couldn't update* and why | **Retry**, **Release page** |

**Skip** hides that version from the daily check; a newer one, or **Check for updates** by hand, shows it again.

### What happens

- **A small update** (a copy installed for you, or a portable copy): only the files that changed since your version,
  many of them as binary patches. The `.vrdelta` file is downloaded in the background, checked against the release's
  `SHA256SUMS.txt`, and every new file is checked against its own SHA-256. **Restart now** puts it in place in about
  two seconds and opens VLC Redesign again on what was playing, where it was; **Later** puts it in place when you
  close VLC Redesign. Your `portable` folder is never touched.
- **If anything goes wrong** (a file held by another program, for example), every file goes back as it was and the
  update is tried again next time. If a file of VLC Redesign was changed since it was installed, the card says the
  small update can't be used: update with the installer (**Release page**).
- **The installer** is used when no small update fits: a version more than three releases behind (or older than
  0.2.4), or a copy installed for everyone. The card then says *VLC Redesign will close, update and open again where
  you were.* The installer is downloaded, checked against its checksum, and run without questions; it reopens VLC
  Redesign where you were.
- **Download** (a portable copy with no small update to take): opens the release page.

Only HTTPS files from GitHub are used, downloads follow Windows' proxy settings, and each step gives up after
30 seconds without data. Running a newer installer by hand works too: it closes a running VLC Redesign politely and
opens it again afterwards.

| Your copy | How it updates |
|---|---|
| Installed for you | Small update; the installer when too far behind |
| Installed for everyone | The installer (asks for administrator rights) |
| Portable | Small update; otherwise download the new zip from the release page |

## Uninstalling

**Windows Settings → Apps → Installed apps → VLC Redesign → Uninstall** (or *Add or remove programs* on Windows 10).

- If VLC Redesign is open, the uninstaller closes it first.
- It removes the program **with its settings, library and history** (the `portable` folder), without asking, and
  the files that small updates added.
- It gives every file type back to the app it had before, and removes the right-click menu entries and the
  *Default apps* entry.
- The copy is taken out of the list a new copy reads its settings from (`HKCU\Software\VLC Redesign\Profiles`).

To keep your settings, copy the `portable` folder somewhere else before uninstalling. A portable copy: delete its
folder.

An update never removes your settings, and a new version in another folder starts with them (see
[Settings carried over](#settings-carried-over)).

---

[Languages](Languages) · [Troubleshooting and FAQ](Troubleshooting-and-FAQ) · [Settings: General](Settings-General) · [Home](Home)
