# Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io/), certificate by
[SignPath Foundation](https://signpath.org/).

> [!NOTE]
> Signing is being set up. The releases published so far are not code-signed; the notes of every release say
> whether its Windows files are signed. How the signing works: [docs/code-signing.md](docs/code-signing.md).

## What is signed

The Windows release files of VLC Redesign:

- every program and library of the package: `vlc.exe`, the helper programs (`vlc-cache-gen.exe`,
  `vlc-preparser.exe`, `vlc-waveform.exe`, `vlc-redesign-update.exe`), `libvlc.dll`, `libvlccore.dll` and the
  plugins, in the installer and in the portable zip;
- the installer, `vlc-redesign-<version>-win64-setup.exe`.

Only files built from this repository are signed. GitHub Actions builds them from the public source, on
GitHub-hosted runners ([`build-windows.yml`](.github/workflows/build-windows.yml)); SignPath checks that each
signing request comes from that workflow on this repository, and an approver approves each release by hand.
Nothing built elsewhere, on a developer's PC for instance, is signed.

The binaries are built from this repository: a visible fork of [VLC](https://code.videolan.org/videolan/vlc), based
on its 4.0 development branch, whose changes come in through reviewed pull requests
([`upstream-sync.yml`](.github/workflows/upstream-sync.yml)). The open-source libraries VLC uses (its contribs:
FFmpeg, Qt and others) are linked into them from VideoLAN's prebuilt builds of the recipes in
[`contrib/`](contrib), inside VideoLAN's official build image. No third-party program is signed on its own. The
installer is made with [Inno Setup](https://jrsoftware.org/isinfo.php).

Not signed: the uninstaller that the installer writes (`unins000.exe`); the portable zip itself (a zip has no
signature; the programs in it have one); the small updates (`.vrdelta`) and `SHA256SUMS.txt`, which are data that
the updater checks by SHA-256.

## Team roles

| Role                                                                         | Members                                     |
|------------------------------------------------------------------------------|---------------------------------------------|
| Authors: change the source code without further review                      | [KuchiSofts](https://github.com/KuchiSofts) |
| Reviewers: review every change made by anyone else, the merges from VLC too | [KuchiSofts](https://github.com/KuchiSofts) |
| Approvers: approve each signing request                                      | [KuchiSofts](https://github.com/KuchiSofts), owner of [KuchiSofts/vlc-redesign](https://github.com/KuchiSofts/vlc-redesign) |

Every team member must use multi-factor authentication on GitHub and on SignPath. Changes from people outside the
team come as pull requests and are reviewed before they are merged. The build scripts and the CI configuration
(`.github/workflows/`, `extras/`, `contrib/`) decide what gets signed, so they are reviewed like any other code.

## Privacy policy

VLC Redesign has no telemetry, no advertising and no account of its own, and it sends no crash reports. What it
remembers (settings, library, history, subtitle choices, downloaded subtitles) stays on your computer, in the
`portable` folder next to `vlc.exe` (or in `%APPDATA%\VLC Redesign` when that folder can't be written).

It connects to other systems for what you open (a file, a stream, a link you click) and for the features below.
Every service it contacts sees your IP address, as with any connection; the update check and the subtitle searches
name the app and its version (User-Agent `VLC-Redesign/<version>`).

The two features that are on out of the box ask first: neither goes online before you answer. The installer asks
on its *Privacy* page (both on to begin with; a silent install keeps them on unless given `/NOUPDATECHECK` or
`/NOONLINESUBS`), and the portable zip's first-run setup asks on a step of its own; the installed copy doesn't ask
again. An update keeps your answers.

| Feature | Out of the box | What is sent, and to whom | Turned off in |
|---|---|---|---|
| Update check | **On** | At most once a day, a few seconds after the start, and when you click *Check for updates*: a request for the latest release, to GitHub (`api.github.com`). *Update now* downloads the release from GitHub. | The installer's *Privacy* page or the first-run setup; then Settings → General → *Check for updates automatically* |
| Online subtitles | **On** | When a video starts without a subtitle in your languages or its subtitles menu opens, and when you search for subtitles. To identify the video, its title, year, season and episode, read from the file name: to IMDb's suggestions (`v2.sg.media-imdb.com`), Wikidata (`www.wikidata.org`) and Stremio's Cinemeta (`v3-cinemeta.strem.io`). To find its subtitles, its title, IMDb id, season and episode, file name, size and OpenSubtitles hash (made from the first and last 64 KB of the file) and your subtitle languages: to the subtitle services turned on, OpenSubtitles.org (`rest.opensubtitles.org`), Wizdom (`wizdom.xyz`), Addic7ed through Gestdown (`api.gestdown.info`, series) and YIFY Subtitles (`yifysubtitles.ch`, movies). | The installer's *Privacy* page or the first-run setup; then Settings → Subtitles → *Find subtitles online when a video has none in these languages* (off, the subtitles menu searches only when you click its search button); each service under *Subtitle services* |
| Titles and cover art | Off | The artist, album and title of what you play, to the services of VLC's art scripts: MusicBrainz (through VideoLAN's `mb.videolan.org`), Cover Art Archive, Amazon's images, Google Images, Last.fm, Wikimedia. | Settings → Privacy → *Fetch titles and cover art from the internet* |
| Last.fm | Off | The songs you play, with your Last.fm account, to Last.fm. | Settings → Privacy → *Submit played tracks to last.fm* |
| OpenSubtitles.com | Off | Only with your own OpenSubtitles.com API key: your searches, the key and, if you sign in, your account, to `api.opensubtitles.com`. | Remove the key |
| Song fingerprint | Off | Only when you ask for it: an acoustic fingerprint of the song and its length, to AcoustID through VideoLAN's proxy (`fingerprint.videolan.org`). | |
| Casting | Off | Once you have opened the cast menu, VLC Redesign looks for TVs and Chromecasts on your local network (mDNS, UPnP) at each start. Casting sends the media to the device you chose. None of it leaves your local network. | |

The installer connects to nothing. `vlc-redesign-update.exe` only puts in place an update that VLC Redesign has
downloaded and checked.

The services have their own privacy policies, among them
[GitHub](https://docs.github.com/en/site-policy/privacy-policies/github-general-privacy-statement),
[IMDb](https://www.imdb.com/privacy), [Wikimedia](https://foundation.wikimedia.org/wiki/Policy:Privacy_policy)
(Wikidata), [Stremio](https://www.stremio.com/privacy), [MetaBrainz](https://metabrainz.org/privacy) (MusicBrainz,
Cover Art Archive) and [Last.fm](https://www.last.fm/legal/privacy); OpenSubtitles, Wizdom, Gestdown, YIFY
Subtitles, AcoustID, Google and Amazon publish theirs on their sites.

## Reports

A signature problem, or a file that should not have been signed:
[open an issue](https://github.com/KuchiSofts/vlc-redesign/issues). A file signed with a SignPath Foundation
certificate that breaks [SignPath Foundation's conditions](https://signpath.org/terms) can also be reported to
support@signpath.io.
