# Contributing to VLC Redesign

Thank you for helping. VLC Redesign is an unofficial VLC 4 build with the interface designed by
[Juxtopposed](https://www.youtube.com/watch?v=ERFeOgqk324); problems with the official VLC go to
[VideoLAN](https://www.videolan.org/vlc/).

## Reporting

Use the issue forms; blank issues are off so that every report has what is needed to act on it.

| Form | For |
|---|---|
| Bug report | Something broken or wrong |
| Feature request | Something missing |
| TV report | How casting to your TV or Chromecast went, good or bad (it fills the [TV compatibility](https://github.com/KuchiSofts/vlc-redesign/wiki/TV-Compatibility) page) |
| Design mismatch | A screen that differs from the Figma design |
| Translation | A wrong or missing text in your language |

Security problems are not issues: see [SECURITY.md](SECURITY.md).

## Translating

Any `.po` editor works, and a new language shows in the language list by itself: see
[docs/TRANSLATING.md](docs/TRANSLATING.md).

## Code

- The branch is `redesign` (the default branch on GitHub is `main`, which mirrors it). Fork, branch from it, and
  open a pull request; every pull request gets a Windows build.
- Keep the design as Juxtopposed drew it: every screen is compared with its Figma frame pixel for pixel
  ([Design check](https://github.com/KuchiSofts/vlc-redesign/wiki/Design-Check)). A change to a screen says which
  frame it follows.
- Interface code is in `modules/gui/qt` (QML and C++); the Windows installer, release tools and tests are in
  `extras/juxtopposed`. The rest is VLC, kept close to upstream so the weekly sync stays easy: fixes to VLC itself go
  in their own commits, with a comment saying what they fix.
- Match the style of the file you touch. Comments say why, not what.
- Every text the user sees goes through `qsTr()` (QML) or `qtr()` / `N_()` (C++), with a context where one English
  word has two meanings.
- Before a pull request: the build passes, the app starts and plays a file, Settings opens, and the change was
  tried in the running program, not only in tests.

## Commits

One change per commit, in the present tense, saying what the user gets ("the layout follows the language") and,
below, why and how. No generated or personal data in the tree.

## Releases

The owner builds releases with `extras/juxtopposed/make-release.ps1`; see
[docs/release](docs/release). The changelog is [CHANGELOG.md](CHANGELOG.md); every change that a user can see is
written there, in plain words, with the commit.
