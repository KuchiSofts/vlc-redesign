# Every upload, every release

**The rule:** an upload to the public repository brings the whole project up to date: the code, and with it the
changelog, the roadmap, the plan, the README, the platforms and the website, the wiki, the translations, VideoLAN's
master, the credits, and an answer for every issue. Nothing is left for later.

The rule is enforced. `extras/juxtopposed/release/release-check.py` checks the whole project, and two places run it:

- **the pre-push hook**, on every push to `KuchiSofts/vlc-redesign` (not on pushes to the private test repository).
  A branch push is checked as an upload of work in progress; a `vX.Y.Z` tag push is checked as the release of X.Y.Z.
  Once the changelog dates the version, a branch push is also checked as its release;
- **`make-release.ps1`**, before it builds or packages anything (not in the GitHub workflow, whose packages are tests).

If a check fails, nothing is pushed or packaged; the report says what to fix. The hook is installed in
`.git/hooks/pre-push` and calls `extras/juxtopposed/release/pre-push-check.sh`.

```
python extras/juxtopposed/release/release-check.py                  # the upload of work in progress
python extras/juxtopposed/release/release-check.py --release 0.4.0  # the release of 0.4.0
```

## What the check covers

| Area | Every upload | A release X.Y.Z, in addition |
|---|---|---|
| Version | `redesign_version.hpp` is the version being prepared; `platforms.yml` `next:` is that version | `redesign_version.hpp` is X.Y.Z; `platforms.yml` Windows `version: X.Y.Z`, `next:` after it |
| Changelog | When code changed: `[Unreleased]` has entries, and CHANGELOG.md is part of the upload | A dated `## [X.Y.Z] · YYYY-MM-DD` section with entries, linked in **Contents**; `[Unreleased]` empty |
| Roadmap and plan | When code changed: `ROADMAP.md` and `docs/MASTER-PLAN.md` are part of the upload; both name the version | `README.md` is part of the upload too |
| README, website | `README.md` names the version; the platform bar and `site/` match `platforms.yml` (`render.py --check`) | |
| Wiki | | `docs/wiki/Home.md` and `docs/wiki/Install-and-Update.md` name X.Y.Z |
| Design | `design/tools/check_qml.py`: the app uses the values of `design/tokens.json` | |
| Translations | `po/vlc.pot` is newer than the last change to a text of the interface | |
| VideoLAN | VideoLAN's master is fetched and fully merged; the merged commit is named in the changelog or the plan | |
| Issues | Every open issue appears in the changelog of the version, the roadmap or the plan | Every issue the version fixes gets a reply and is closed (listed after the check) |
| Credits | Everyone whose issue was fixed or closed is in `CREDITS.md` | |
| Working tree | Everything is committed | |
| Owner's roadmap | The detailed roadmap outside the repository (`source\VLC-Redesign-Roadmap.md`) names the version | |

## Before every upload

1. Merge VideoLAN's master (`git fetch origin master`), build, and test what it touched: the controls of the bar,
   the dialogs, playback, the library, a UI tour without QML warnings. Write down the merged commit.
2. New or changed texts: a new file with texts goes into `po/POTFILES.in`; `potgen` (xgettext, plus the `.ui`
   forms of `extras/juxtopposed/translations/ui-pot.py` merged by `msgcat --use-first`), `msgmerge` for every
   catalog, translate the 72 languages, `msgfmt -c` for all, then
   `weblate-sync.py export --upstream <merged VideoLAN commit>`.
3. `CHANGELOG.md` `[Unreleased]`: every change, in the user's words, with its issue number (`[#9](…/issues/9)`).
4. `ROADMAP.md`: what moved to done, what is next; `docs/MASTER-PLAN.md`: the **Now** section and the dashboard.
5. `platforms.yml` changed? `python extras/juxtopposed/platforms/render.py` (README bar, table, `site/`).
6. Every open issue: in the changelog when fixed, otherwise in the roadmap or the plan with its state.
7. `CREDITS.md`: everyone who reported, translated or contributed.
8. `release-check.py`, then push.

## A release X.Y.Z

1. Everything of *Before every upload*.
2. `CHANGELOG.md`: `[Unreleased]` becomes `## [X.Y.Z] · YYYY-MM-DD` with a one-paragraph summary, a new empty
   `[Unreleased]` above it, and the link in **Contents**.
3. `platforms.yml`: `version: X.Y.Z`, `date:`, `next:` the following version; `render.py`.
4. `README.md` (what is new, the screenshots), `docs/wiki/Home.md`, `docs/wiki/Install-and-Update.md`, and every
   wiki page whose feature changed.
5. Commit "Release X.Y.Z"; tag it `vX.Y.Z` ("VLC Redesign X.Y.Z"); `make-release.ps1` (runs the check, builds,
   writes `vlc-redesign-X.Y.Z-source.tar.gz` from that commit, signs `SHA256SUMS.txt`). It stops when the tree has
   uncommitted changes or the tag names another commit.
6. Push the branch to `main`, push the tag (the hook checks both).
7. After the push (the check lists them):
   - the GitHub release: setup, zip, every `.vrdelta`, `vlc-redesign-X.Y.Z-source.tar.gz`, `SHA256SUMS.txt`,
     `SHA256SUMS.txt.sig`, the notes from `release/release-notes.py` (with their *Source code* section), or
     `docs/release/X.Y.Z/release-notes.md` when the version has one;
   - a reply on every issue the release fixes (what was wrong, what changed, the version, how to update), then close it;
   - an announcement in Discussions › Announcements;
   - Weblate: export the texts of the release;
   - check the README, the website and the in-app update on a copy of the previous version;
   - `docs/MASTER-PLAN.md` and `ROADMAP.md`: the version is published, the next one is in progress.
8. Raise `VLC_REDESIGN_VERSION` to the next version as soon as work for it starts.

## Source code (GPL)

Every binary published comes with its source ([SOURCE.md](SOURCE.md)): the source archive of the release on the
same page, the tag on the commit it was made from, and the written offer for the libraries of `contrib/`. A
release without its `-source.tar.gz` is not published. A source request (an issue *Source request X.Y.Z*) is
answered with the contrib tarballs of that release (`make fetch` in `contrib/` of its source).

## Issues

- Every new issue gets a reply, a label (bug, enhancement, translation, question), and a line in the roadmap or
  the plan with its state, before the next upload.
- An issue is closed by the release that fixes it, with a reply that names the version, not before.
- An issue that will not be fixed gets the reason, and stays in the plan as a decision.
- The reporter is credited in `CREDITS.md` and in the changelog entry.
