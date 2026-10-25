# Security

## Reporting a vulnerability

Please do not open a public issue for a security problem. Use GitHub's private report:
**Security → Report a vulnerability** on the repository
([direct link](https://github.com/KuchiSofts/vlc-redesign/security/advisories/new)). You get an answer within a
week, and a fix or a plan within a month for a confirmed problem. You are credited in the release notes unless you
prefer not to be.

A problem in VLC itself (the player, codecs, demuxers) should go to [VideoLAN](https://www.videolan.org/security/)
as well: this project follows VLC 4 and takes its fixes with the weekly sync.

## Supported versions

Only the latest release gets fixes. Older releases update to it in one step: the app offers a small update, or the
installer and the portable zip of the [latest release](https://github.com/KuchiSofts/vlc-redesign/releases/latest).

## What the program does online

Nothing without being asked. Two features go online and are chosen at install (or the first start of a portable
copy), each with its policy shown: checking for updates (GitHub) and finding subtitles for a video that has none in
your languages (the services named in Settings → Subtitles). Both can be turned off in Settings → Privacy. Casting
talks to the TV you choose on your own network. There is no telemetry.

## Checking a download

Every release lists the SHA-256 of its files in `SHA256SUMS.txt`. Builds are not code-signed yet (see
[CODE_SIGNING_POLICY.md](CODE_SIGNING_POLICY.md)); Windows SmartScreen may ask for *More info → Run anyway*.
