# Building from source

The Windows build of VLC Redesign is a cross-compile in Docker, with VideoLAN's official build image and its prebuilt
libraries. It runs on Windows, in PowerShell. Every release links to the exact source it was built from.

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

The first build takes a while: it builds the Docker image once and downloads VideoLAN's prebuilt libraries
(`contrib`) for the source. Later builds only rebuild what changed: the objects stay in a Docker volume. The result is
a portable VLC Redesign in the output folder: run its `vlc.exe`. Close a VLC running from that folder first; the
script stops otherwise, since a running VLC locks its files.

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
| `vlc-redesign-<version>-delta-from-<old>.vrdelta` | [Small updates](Install-and-Update#small-updates) from the 3 latest releases whose zip is in `-Dist`; each is checked by applying it to the old release |

| Parameter | Default | What it does |
|---|---|---|
| `-Version` | the version in `modules/gui/qt/util/redesign_version.hpp` | The version of the release. |
| `-Package` | `%USERPROFILE%\source\vlc-redesign-release` | The folder the build goes to, and that is packaged. |
| `-Dist` | `%USERPROFILE%\source\vlc-redesign-dist` | The folder that receives the release files. |
| `-Quick` | off | Only rebuilds what changed. |
| `-SkipBuild` | off | Packages the existing `-Package` folder as it is. |
| `-Signed` | off | Packages a folder whose files are signed already (implies `-SkipBuild`): nothing is built, and no file of it is changed. |

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

Linux builds use `extras/juxtopposed/linux.sh`. The interface lives in
[`modules/gui/qt`](https://github.com/KuchiSofts/vlc-redesign/tree/main/modules/gui/qt). Which other systems may come,
and in which order: [Platforms](Platforms).

---

[Install and update](Install-and-Update) · [Platforms](Platforms) · [Credits and license](Credits-and-License) · [Home](Home)
