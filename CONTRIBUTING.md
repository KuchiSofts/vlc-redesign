# Contributing to VLC Redesign

Thank you for helping. VLC Redesign is an unofficial VLC 4 build with the interface designed by
[Juxtopposed](https://www.youtube.com/watch?v=ERFeOgqk324); problems with the official VLC go to
[VideoLAN](https://www.videolan.org/vlc/).

Everyone taking part follows the [code of conduct](CODE_OF_CONDUCT.md). Where to ask questions is in
[SUPPORT.md](SUPPORT.md); security problems are never public issues: see [SECURITY.md](SECURITY.md).

## Reporting

Use the issue forms; blank issues are off so that every report has what is needed to act on it.

| Form | For |
|---|---|
| Bug report | Something broken or wrong |
| Feature request | Something missing, or an idea |
| Design mismatch | A screen that differs from the Figma design |
| Translation | A wrong or missing text in your language |
| TV report | How casting to your TV or Chromecast went, good or bad (it fills the [TV compatibility](https://github.com/KuchiSofts/vlc-redesign/wiki/TV-Compatibility) page) |
| Live TV source | A free, legal TV source (playlist or programme guide) worth listing |

Search the [open and closed issues](https://github.com/KuchiSofts/vlc-redesign/issues?q=is%3Aissue) first, and say
which version you use (*⋯ menu → About VLC*). Questions and open-ended ideas fit better in
[Discussions](https://github.com/KuchiSofts/vlc-redesign/discussions).

### Labels

Issues are triaged with a small set of labels, kept in [`.github/labels.yml`](.github/labels.yml):

- **Type:** `bug`, `enhancement`, `design`, `performance`, `question`
- **Platform:** `platform:windows`, `platform:linux`, `platform:macos`, `platform:android`
- **Area:** `area:ui`, `area:live-tv`, `area:subtitles`, `area:casting`, `area:translations`, `area:installer`, `area:ci`
- **Status:** `status:needs-info` (waiting for the reporter), `status:confirmed`, `status:fixed-pending-release`
- **Priority:** `priority:high`

An issue waiting for details (`status:needs-info`) may be closed after a few weeks without an answer; it is reopened
as soon as the details arrive.

## Translating

The easiest way is [Weblate](https://hosted.weblate.org/engage/vlc-redesign/), in the browser: no GitHub account and
no tools needed. Weblate sends the translations here as a pull request, which the maintainer merges.

Any `.po` editor works too, and a new language shows in the language list by itself: see
[docs/TRANSLATING.md](docs/TRANSLATING.md). Please don't send `.po` files by pull request for a language that is
being translated on Weblate at the same time: the two would conflict.

## Building

The Windows build is a cross-compile in Docker, with VideoLAN's official build image:

```powershell
git clone https://github.com/KuchiSofts/vlc-redesign.git
cd vlc-redesign
.\extras\juxtopposed\win64-docker.ps1
```

The full guide, with every parameter, the release script and the GitHub workflows, is
[Building from source](https://github.com/KuchiSofts/vlc-redesign/wiki/Building-from-Source)
([also in the tree](docs/wiki/Building-from-Source.md)).

## Testing

Before opening a pull request:

- the build passes, the app starts, plays a file, and Settings opens;
- the change was tried in the running program, not only in tests;
- a changed screen was compared with its Figma frame
  ([Design check](https://github.com/KuchiSofts/vlc-redesign/wiki/Design-Check));
- changed QML files pass the linter: `.\extras\juxtopposed\win64-docker.ps1 -Lint`.

Every pull request is built on GitHub Actions; the result shows on the pull request.

## Pull requests

- **One fix or one feature per pull request.** Small pull requests are reviewed faster; unrelated clean-ups go in
  their own.
- **Link the issue** in the description with `Fixes #123` (or `Refs #123` when it only helps), so that the issue
  closes when the pull request is merged.
- **CI must pass.** A pull request with a failing build is not merged; say so in the description if the failure is
  not caused by your change.
- Fill in the pull request template: what changes for the user, why, and what was checked.
- Keep the design as Juxtopposed drew it: a change to a screen says which Figma frame it follows.
- Interface code is in `modules/gui/qt` (QML and C++); the installer, release tools and tests are in
  `extras/juxtopposed`. The rest is VLC, kept close to upstream so the weekly sync stays easy: fixes to VLC itself go
  in their own commits, with a comment saying what they fix.
- Match the style of the file you touch. Comments say why, not what.
- Every text the user sees goes through `qsTr()` (QML) or `qtr()` / `N_()` (C++), with a context where one English
  word has two meanings.
- A change a user can see gets a line in [CHANGELOG.md](CHANGELOG.md) under *Unreleased*, in plain words.
- Contributions are published under the project's licence, [GPL-2.0-or-later](COPYING).

Pull requests target `main`. The maintainer may squash a pull request into one commit when merging it.

## Commits

One change per commit, in the present tense, saying what the user gets ("the layout follows the language") and,
below, why and how. No generated files, no personal data (names, e-mail addresses, local paths, host names) and no
secrets in the tree.

## Releases

The maintainer builds releases with `extras/juxtopposed/make-release.ps1`; see [docs/release](docs/release). Every
release credits the design by Juxtopposed.
