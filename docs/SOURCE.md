# Source code of the releases

VLC Redesign is free software: VLC is GPL v2 or later (its library, libvlc, and most modules LGPL v2.1 or later),
and so is everything this project adds. Whoever has a binary of VLC Redesign can have the exact source code it
was built from, under the same licence. This page says where it is.

## The source of a release

Every release X.Y.Z publishes, beside its installers and portable zips:

| What | Where |
|---|---|
| `vlc-redesign-X.Y.Z-source.tar.gz` | The release page on GitHub (*Assets*). The whole tree of the commit the binaries were built from, made by `make-release.ps1` (`git archive`); its SHA-256 is in `SHA256SUMS.txt`, signed with the others |
| The tag `vX.Y.Z` | [github.com/KuchiSofts/vlc-redesign](https://github.com/KuchiSofts/vlc-redesign/tags): the same commit, with its history. `make-release.ps1` stops when the tag names another commit |
| How to build it | `extras/juxtopposed/win64-docker.ps1` (Windows x64 and ARM64, in Docker), `extras/juxtopposed/linux.sh` (Linux), `extras/juxtopposed/macos-build.sh` (macOS); [Platforms](PLATFORMS.md) |

The binaries of the Windows build workflow (GitHub Actions) name their commit in the run; they are tests, not
releases.

## The libraries VLC uses (contribs)

The Windows builds link VLC's third-party libraries (FFmpeg, Qt, libass, x264 and the others, in `contrib/`)
as VideoLAN builds them for its own nightly builds: the archive of VideoLAN's CI for the newest VideoLAN commit
merged (`win64-docker.ps1` downloads it from `artifacts.videolan.org`), plus the libraries this project builds
itself with a fix (`contrib/src/upnp`). The source of each one is named, with its version, URL and SHA-512, in
`contrib/src/<name>/rules.mak` and `contrib/src/<name>/SHA512SUMS` of the release's source; VideoLAN keeps the
tarballs at [download.videolan.org/pub/contrib](https://download.videolan.org/pub/contrib/). `make fetch` in
`contrib/` downloads them all.

**Written offer.** For at least three years after a release, the source of every library in its binaries, as
the tarballs and patches used, is given to anyone who asks, for no more than the cost of making the copy: open an
issue on [GitHub](https://github.com/KuchiSofts/vlc-redesign/issues) titled *Source request X.Y.Z*.

## Other licences in the package

- The interface's design: *VLC Redesign (Community)* by Juxtopposed, CC BY 4.0 (`design/README.md`).
- The fonts: Inter and DM Mono, SIL Open Font License (`design/fonts`).
- Live TV's catalog lists free, legal sources with their links only; the channels and guides are their owners'
  (`modules/misc/iptv/catalog/catalog.json` names each source and its website).
