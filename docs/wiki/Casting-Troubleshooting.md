# Casting: when a TV does not play

What to do when a TV or a Chromecast is not listed, does not play a file, or when Windows asks about its firewall.
How casting works is on the [Casting](Casting) page; what is known of each TV is in [TV compatibility](TV-Compatibility).

## Quick checks

| Symptom | First thing to try |
|---|---|
| The TV is not listed | Turn it on (not standby), same network, and keep the panel open: it keeps looking |
| *Windows Firewall doesn't let...* | **Allow on private networks** on the card, and approve Windows' question |
| *...couldn't read the video...* | Another firewall or security program; ports 7070 (DLNA) and 8010 (Chromecast) |
| The picture stutters on the TV | Subtitles off (no conversion), or a lower *Cast quality* for a Chromecast |
| *...refused to play this video.* | **Try again**: a DLNA TV then gets the film converted |

## The TV is not in the list

- The TV (or Chromecast) must be **on** and on the **same network** as this computer. A TV in standby may not answer.
- Give the search its time: the dot next to *Cast to* pulses while VLC looks, and it keeps looking as long as the
  panel is open, so a TV turned on now shows up by itself. After a few seconds with no device, the panel says so.
- A device you hid from the list (right-click it) stays hidden, also when the network finds it under its short name;
  the list can show the hidden devices again.
- With **Discover cast devices** off (Settings › Recording), the *Renderer* menu (right click on the video →
  *Playback → Renderer*) only looks when you choose *Search for cast devices*. The cast panel always looks.
- VLC Redesign does not look for devices when it starts: only when you open the cast panel (or the *Renderer* menu),
  and, after that first search, quietly 3 seconds into a playing video.

## The TV does not play, or stops

When a cast fails, the card of the player says why, with **Try again** and **Play here** (back to this computer, where
the TV was). In the cast panel, the device reads *Not playing: choose it to try again*.

| The card says | What it means, what to do |
|---|---|
| *TV doesn't answer. Check that it's on, and on the same network as this computer.* | The TV did not answer VLC. Check it is on and on the same network, then **Try again**. |
| *Windows Firewall doesn't let TV connect to this computer to read the video.* | See [the firewall question](#the-firewall-question) below. |
| *TV couldn't read the video from this computer.* | The TV could not read the stream from VLC's server on this computer (port 7070 for a DLNA TV, 8010 for a Chromecast). A firewall or security program other than Windows Firewall can keep it out too. |
| *TV refused to play this video.* | The TV refused the file. **Try again**: a film a DLNA TV refuses as it is gets converted. |
| *TV stopped playing.* | The TV stopped by itself, or from its remote. |
| *TV is playing something else now.* | Another app or phone took the TV or Chromecast. |
| *TV couldn't play the video.* | Any other failure. **Try again**, or **Play here**. |

### A file the TV cannot play

VLC Redesign converts what the TV cannot play, while it plays:

- A DLNA TV gets a film as it is when it lists that format, else converted to H.264 in MPEG-TS. A TV that refuses a
  film as it is, or stops it within its first 15 seconds, gets it converted instead of an error.
- A **subtitle shown** goes into the picture, so the film is converted. To send a film as it is, turn the subtitles off
  before casting.
- A Chromecast gets converted what it cannot play. **Cast quality** (Settings › Recording) trades quality against the
  processor time of the conversion: *Low CPU* for a slow PC.
- A 4K film is made smaller by the graphics card for the TV; films of 50 and 60 frames per second go at 25 and 30.

If the picture stutters on the TV during a conversion, the computer may be too slow for it: turn the subtitles off
(a film the TV plays as it is needs no conversion), or, for a Chromecast, choose a lower *Cast quality*.

## The firewall question

A TV reads the film from a small server VLC Redesign runs on this computer. Windows Firewall keeps other devices of
the network out of a program unless a rule lets them in, so a TV cannot read the film until there is a rule for
VLC Redesign.

- **VLC Redesign asks only when it is needed.** It opens no network ports when it starts, so Windows does not ask
  anything then. When you choose a TV and the firewall would keep it out, VLC Redesign asks Windows at once to add an
  inbound rule named **VLC Redesign** for this program, on private networks (and on public ones when this computer is
  on a public network). Windows asks for an administrator's approval; the rule is made once.
- **Declined?** The card of the player says *Windows Firewall doesn't let TV connect to this computer to read the
  video*, with a button **Allow on private networks** (**Allow on public networks too** on a public network) and
  **Play here**. The button asks Windows again (*Waiting for Windows…* meanwhile). Once the rule is there, the TV
  starts again from where it was.
- **A rule that blocks VLC Redesign** (made when Windows' own question was declined once) wins over any allowing rule:
  the button then replaces the inbound rules of this program with the one of VLC Redesign, in one approval.
- A device on this computer itself (a test device, a receiver app running here) needs no rule.
- To see or remove the rule: *Windows Security → Firewall & network protection → Advanced settings → Inbound Rules*,
  the rule **VLC Redesign**.

## It used to fail, it works since 0.3.0

- A film cast to a TV now plays to its last picture, also with a subtitle file beside it and for the next film of a
  playlist.
- The TV starts once, with the picture and the sound together.
- A Samsung TV keeps a paused cast paused, and plays a film sent as it is (as MPEG-TS) beyond its first second.

If a TV failed with an older version, update to the latest one (see [Small updates](Install-and-Update#small-updates))
before reporting it.

## Report a TV

If a TV or a Chromecast does not work, [open an issue](https://github.com/KuchiSofts/vlc-redesign/issues) with its
brand, model and year, what you cast (the format, with or without subtitles), what the card said, and a log: turn on
*Settings › General › Diagnostics › Save a log* (with *Log details* on *Everything (debug)*), close Settings, then cast
again; see [Save a log](Troubleshooting-and-FAQ#save-a-log). See
[TV compatibility](TV-Compatibility).

---

[Casting](Casting) · [TV compatibility](TV-Compatibility) · [Troubleshooting and FAQ](Troubleshooting-and-FAQ)
