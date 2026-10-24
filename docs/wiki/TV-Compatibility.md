# TV compatibility

Which TVs and Chromecasts VLC Redesign has been checked with, and what each one does differently. This page grows with
your reports: if your TV is not here, [tell us how it went](#report-your-tv).

How to cast: [Casting](Casting). When something fails: [Casting: when a TV does not play](Casting-Troubleshooting).

## Checked

| Device | Kind | Checked with | Result |
|---|---|---|---|
| **Samsung UE55MU8000** (2017, Tizen) | DLNA TV | The real TV of the quality checks: start, pause of 1 and 5 minutes, seek, next file, subtitles, a film sent as it is, a converted film, the end of a film | Works, with the quirks below |
| Chromecast | Chromecast | A test device that speaks the Chromecast protocol, in the checks of every build | Works; no real Chromecast reported yet |
| DLNA TV | DLNA TV | A test device that reads the stream as a TV does, in the checks of every build | Works |

**Not checked yet:** LG, Sony, Android TV, other Samsung models, real Chromecasts. Reports are welcome.

## Known quirks

### Samsung (Tizen, 2017)

- **Pausing.** A paused Samsung TV goes on reading the stream until it has about 5 seconds of the film ahead, and gives
  up when nothing comes for 10 seconds: it left the film with an error about 10 seconds into a pause. Since 0.3.0,
  while a cast is paused, VLC Redesign sends a trickle of empty packets that the TV drops, and the TV stays paused.
- **A film sent as it is.** A film the TV can play without conversion (no subtitles shown) went as a fragmented MP4,
  which the TV took for a one-second file. Since 0.3.0 it goes as MPEG-TS, and a TV that stops such a film right after
  it began gets it converted instead of an error.
- **PrepareForConnection.** The TV rejects VLC's *PrepareForConnection* request (its arguments are not in the UPnP
  order). It does no harm and is kept as it is: in a test, an accepted request made the TV fail after a pause.
- **The firewall.** Like any DLNA TV, it reads the film from this computer: Windows Firewall must let it in
  ([the firewall question](Casting-Troubleshooting#the-firewall-question)).

### Every DLNA TV

- VLC Redesign sends the film at its pace, one second ahead of it. The lead is VLC's option `sout-dlna-lead`, in
  milliseconds (500 to 10000, 1000 by default): more makes the TV wait longer before it shows a seek. To change it,
  write for example `--sout-dlna-lead=3000` in *Settings › General › Advanced Options › Additional command line
  options* (read at the next start).
- Films of 50 and 60 frames per second are converted to 25 and 30 when they need a conversion (subtitles shown, or a
  format the TV cannot play).

### Chromecast

- The video plays only on the Chromecast; *Show the video here too* is for DLNA TVs.
- What the Chromecast cannot play is converted; *Cast quality* (Settings › Recording) sets how.

## Report your TV

[Fill in a TV report](https://github.com/KuchiSofts/vlc-redesign/issues/new?template=tv_report.yml) (an issue titled
*TV report: brand model*), with:

- **The TV**: brand, model, year (on the label at its back, or in its settings).
- **How it is connected**: Wi-Fi or cable, the same router as the computer.
- **What you cast**: the file format (MP4, MKV…), its size (1080p, 4K), with or without subtitles.
- **What worked**: start, pause (how long), seek, the next film of a playlist, subtitles, the end of the film.
- **What did not**, and what the card of the player said.
- **A log**: turn on *Settings › General › Diagnostics › Save a log*, restart VLC Redesign, cast again, and attach the
  log file.

---

[Casting](Casting) · [Casting: when a TV does not play](Casting-Troubleshooting) · [Home](Home)
