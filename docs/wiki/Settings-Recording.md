# Settings: Recording

**⋯ → Settings → Recording** (<kbd>Ctrl</kbd>+<kbd>P</kbd>; the header reads *Recording, Casting & Streaming Out*).
Where recordings go, how VLC finds and feeds cast devices, and the defaults of VLC's stream output (streaming and
converting). No row of this page is reserved to Developer mode. See
[how the Settings window works](Settings#how-the-window-works).

![Settings: Recording](images/settings-recording.png)

## Recording

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Save recordings to** | Folder field with **Browse** (a folder dialog, *Choose a recording folder*); you can also type a path. Default: empty, which saves to your Videos folder; the field then shows that folder, with your user folder written `~` (`~/Videos`). | Where the **Record** button of the player saves what it records. | `input-record-path` |
| **Record without re-encoding when possible** | Toggle. Default: on. | Records the input stream as it arrives instead of passing it through VLC's stream output, when the source allows it. | `input-record-native` |

## Casting

See [Casting](Casting) for how casting works.

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Discover cast devices** | Toggle. Default: on. | Looks for the cast devices of the local network whenever the *Renderer* menu opens (*Playback → Renderer* in the right-click menu). Off, that menu shows **Search for cast devices** instead, and VLC only looks when you choose it. The **Cast** button of the player always looks when its panel opens. | `redesign/discover-cast-devices` (interface setting) |
| **Cast quality** | Dropdown: *High* / *Medium* / *Low* / *Low CPU*. Default: *Medium* (*Low* on ARM devices). | When a video has to be converted for a Chromecast, trades the quality of the conversion against the processor time and the bandwidth it takes. *High*: high quality, high bandwidth. *Medium*: medium quality and bandwidth. *Low*: low quality, low bandwidth. *Low CPU*: low quality but high bandwidth, the lightest for the processor. DLNA TVs have their own setting (`sout-dlna-conversion-quality`, in *All VLC options*). | `sout-chromecast-conversion-quality` (0 to 3) |

## Streaming

These rows apply when VLC streams or converts media through its stream output (**Convert / Save**, **Stream**).

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Play locally while streaming** | Toggle. Default: off. | Also plays the media in VLC while it is streamed. | `sout-display` |
| **Include audio** | Toggle. Default: on. | Sends the audio tracks to the stream output. | `sout-audio` |
| **Include video** | Toggle. Default: on. | Sends the video tracks to the stream output. | `sout-video` |
| **Include subtitles** | Toggle. Default: on. | Sends the subtitle tracks to the stream output. | `sout-spu` |
| **Encoding preset** | Dropdown: *Ultra fast* / *Super fast* / *Very fast* / *Faster* / *Fast* / *Medium* / *Slow* / *Slower* / *Very slow* / *Placebo*. Default: *Medium*. | The x264 preset used when VLC encodes video to H.264: a faster preset takes less processor time, a slower one gives a better picture for the same size. Casting picks its own preset (*Very fast* or *Ultra fast*, with *Zero latency*). Needs the x264 module. | `sout-x264-preset` (Medium = empty) |
| **Tuning** | Dropdown: *None* / *Film* / *Animation* / *Grain* / *Still image* / *PSNR* / *SSIM* / *Fast decoding* / *Zero latency*. Default: *None*. | The x264 tuning used when VLC encodes video to H.264, to suit the kind of picture or the use (*Zero latency* for live streaming, for example). Needs the x264 module. | `sout-x264-tune` (None = empty) |

## Advanced Options

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Send Dolby audio as is** | Toggle. Default: off. | Passes Dolby (E-)AC-3 audio to a Chromecast without converting it. Leave it off unless the receiver behind the Chromecast plays Dolby. | `sout-chromecast-audio-passthrough` |
| **Cast HTTP port** | Number field. Default: 8010. | The port of the local server a Chromecast reads the media from. Change it if another program uses that port or your firewall only opens another one. DLNA TVs read from their own port (`sout-dlna-http-port`, 7070). | `sout-chromecast-http-port` |
| **Keep the stream output open between items** | Toggle. Default: off. | Keeps one stream output running across the items of the play queue, instead of starting a new one for each item. | `sout-keep` |

## Good to know

- **Where did a recording go?** To the folder of *Save recordings to*: your Videos folder unless you chose another.
- **A TV does not appear in the Renderer menu:** with *Discover cast devices* off, choose *Search for cast devices*;
  the Cast button always searches. See [Casting troubleshooting](Casting-Troubleshooting#the-tv-is-not-in-the-list).
- **A Chromecast stutters on converted videos:** lower *Cast quality*; on a slow computer choose *Low CPU*.
- **No sound on the TV with a Dolby track:** turn *Send Dolby audio as is* off (the default).

**Related pages:** [Casting](Casting) · [Casting troubleshooting](Casting-Troubleshooting) · [TV compatibility](TV-Compatibility) · [The player](The-Player)

---

← [Performance](Settings-Performance) · [All settings](Settings) · [Extensions](Settings-Extensions) →
