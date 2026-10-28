# Building from source

The Windows build of VLC Redesign is a cross-compile in Docker, with VideoLAN's official build image and its prebuilt
libraries. It runs on Windows, in PowerShell, and on GitHub's Linux runners. Every release links to the exact source
it was built from. This page covers the development build, the release script and its files, the GitHub workflows,
and signing.

## What you need

- [Docker Desktop](https://www.docker.com/products/docker-desktop/) and Git.
- For a release (the installer and the small updates): [Inno Setup](https://jrsoftware.org/isinfo.php) 6.7 or later
  (`choco install innosetup`), and Python with `bsdiff4` (`pip install bsdiff4`).

## Build

```powershell
git clone https://github.com/KuchiSofts/vlc-redesign.git
cd vlc-redesign
# Development build (debug, asserts on) into %USERPROFILE%\source\vlc-win64
.\extras\juxtopposed\win64-docker.ps1
# Release: build, then the portable zip, the installer and SHA256SUMS.txt
.\extras\juxtopposed\make-release.ps1
```

The first build takes a while: it builds the Docker image once (`vlc-win64-redesign`, from VideoLAN's
`registry.videolan.org/vlc-debian-win64-posix` image, the one of VideoLAN's own Windows CI) and downloads VideoLAN's
prebuilt libraries (`contrib`) for the source from `artifacts.videolan.org`. VideoLAN publishes those archives only
for its own commits, so the script takes the newest one among the last 20 commits that touched the build paths. Later builds only rebuild what changed: the objects stay in a Docker volume. The result is
a portable VLC Redesign in the output folder: run its `vlc.exe`. At the end the script also writes the plugin cache
(`plugins\plugins.dat`, with `vlc-cache-gen.exe`), without which every start scans all plugins.

> **Good to know.** Close a VLC running from the output folder first: the script stops otherwise, since a running VLC
> locks its files. Files still held by a VLC that crashed are renamed aside (`*.old-<time>`) and deleted on a later
> build.

## `win64-docker.ps1`

`extras\juxtopposed\win64-docker.ps1` builds VLC Redesign for Windows x86_64 and puts a portable package in `-Out`.

| Parameter | Default | What it does |
|---|---|---|
| `-Out` | `%USERPROFILE%\source\vlc-win64` | The folder that receives the portable package. |
| `-Cache` | `%USERPROFILE%\source\vlc-cache` | Where the prebuilt library archives are downloaded and kept. |
| `-Quick` | off | Only rebuilds what changed in a tree already configured (skips configure). |
| `-Lint` | off | Only runs `qmllint` on the QML files changed in the working tree, then stops. |
| `-Volume` | `vlc-win64-build` | The Docker volume that holds the build tree. Use another one for a release build, so the debug tree stays configured. |
| `-Strip` | off | Strips the debug information from the packaged files. |
| *other arguments* | | Passed on to VLC's `build.sh`, for example `-r` for a release build. |

## `make-release.ps1`

`extras\juxtopposed\make-release.ps1` builds a release (in its own Docker volume, `vlc-win64-release`, stripped, in
release mode) and makes in `-Dist`:

| File | What it is |
|---|---|
| `vlc-redesign-<version>-win64.zip` | The portable package |
| `vlc-redesign-<version>-win64-setup.exe` | The installer (Inno Setup) |
| `SHA256SUMS.txt` | The SHA-256 of the files |
| `SHA256SUMS.txt.sig` | Its signature by the project's release key (ECDSA P-256), which the app checks before any update from 0.3.2 on. Made with `extras\juxtopposed\signing\release-key.ps1`, whose key exists only on the release PC: without it, `make-release.ps1` stops (or leaves the signature out with `-NoSumsSignature`, and then the app installs none of these files) |
| `vlc-redesign-<version>-delta-from-<old>.vrdelta` | [Small updates](Install-and-Update#small-updates) from the 3 latest releases whose zip is in `-Dist`; each is checked by applying it to the old release |

| Parameter | Default | What it does |
|---|---|---|
| `-Version` | the version in `modules/gui/qt/util/redesign_version.hpp` | The version of the release. |
| `-Package` | `%USERPROFILE%\source\vlc-redesign-release` | The folder the build goes to, and that is packaged. |
| `-Dist` | `%USERPROFILE%\source\vlc-redesign-dist` | The folder that receives the release files. |
| `-Quick` | off | Only rebuilds what changed. |
| `-SkipBuild` | off | Packages the existing `-Package` folder as it is. |
| `-Signed` | off | Packages a folder whose files are signed already (implies `-SkipBuild`): nothing is built, and no file of it is changed. |

### Examples

```powershell
# Rebuild only what changed, after editing QML or C++
.\extras\juxtopposed\win64-docker.ps1 -Quick
# Check the changed QML files with qmllint, without building
.\extras\juxtopposed\win64-docker.ps1 -Lint
# A release from the tree already built, into its own folders
.\extras\juxtopposed\make-release.ps1 -Quick -Dist D:\dist
```

## On GitHub

| Workflow | When | What it does |
|---|---|---|
| **Windows build** (`build-windows.yml`) | Every push to `main`, every pull request, and by hand | Builds the package in Docker on Ubuntu 24.04, then a *launch test* on Windows Server 2022 starts it and plays a file (keeping crash dumps and logs); the package and the logs are uploaded as artifacts. Run by hand with signing chosen, it has the files signed through SignPath and packages the release from them. |
| **Upstream sync** (`upstream-sync.yml`) | Every Monday at 06:00 UTC, and by hand | Tries to merge VideoLAN's VLC into this fork: a pull request when it applies cleanly, an issue listing the conflicting files otherwise. |
| **Wiki** (`wiki-sync.yml`) | A push that changes `docs/wiki` | Copies `docs/wiki` to this wiki. |
| **Labels** (`labels.yml`) | A push that changes `.github/labels.yml`, and by hand | Keeps the issue labels in step with the repository's list. |

## Signed builds

Signing is prepared and not active yet: the files of the releases are unsigned, and their SHA-256 is listed in
`SHA256SUMS.txt`. The Windows files are to be signed through [SignPath Foundation](https://signpath.org/)'s free
program for open-source projects. SignPath signs only what this repository's GitHub workflow built, so a package
made on your own PC can't be signed, and builds from source stay unsigned.

In the workflow, the package is built in Docker and tested, its files are signed, and `make-release.ps1 -Signed` makes
the zip, the installer and the small updates from the signed files without changing them; the installer is then
signed too. The details: [docs/code-signing.md](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/code-signing.md)
and the [code signing policy](https://github.com/KuchiSofts/vlc-redesign/blob/main/CODE_SIGNING_POLICY.md).

## Other systems

A Linux build script exists (`extras/juxtopposed/linux.sh` with `linux.Dockerfile`: Ubuntu 24.04 and its Qt 6.4),
from the first milestones; it has not been run since the Windows work began, and is not a supported build. The interface lives in
[`modules/gui/qt`](https://github.com/KuchiSofts/vlc-redesign/tree/main/modules/gui/qt). Which other systems may come,
and in which order: [Platforms](Platforms).

---

[Install and update](Install-and-Update) · [Platforms](Platforms) · [Credits and license](Credits-and-License) · [Home](Home)
