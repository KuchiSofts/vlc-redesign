# Settings: Recording

**⋯ → Settings → Recording** (or <kbd>Ctrl</kbd>+<kbd>P</kbd>). Where recordings go, casting to other devices, and streaming. The page is titled *Recording, Casting & Streaming Out*.

![Settings: Recording](images/settings-recording.png)

Rows under **Advanced Options** are in the collapsed bar at the bottom of the page. This page has no rows reserved to Developer mode. See [how the Settings window works](Settings).

### Recording
- **Save recordings to**: folder field with a **Browse** button (folder dialog, "Choose a recording folder"). Default: empty, which saves to your Videos folder (the field shows it, for example `~\Videos`). Where the **Record** button saves what it records. <sub>VLC option: `input-record-path`</sub>
- **Record without re-encoding when possible**: toggle. Default: on. Records the stream as it arrives instead of passing it through the stream output, when the stream allows it. <sub>VLC option: `input-record-native`</sub>

### Casting
See [Casting](Casting) for how casting works.
- **Discover cast devices**: toggle. Default: on. Looks for the cast devices of the local network when the *Renderer* menu opens. Off, that menu shows **Search for cast devices** instead, and VLC only looks when you choose it. <sub>Interface setting: `redesign/discover-cast-devices`</sub>
- **Cast quality**: dropdown: High / Medium / Low / Low CPU. Default: Medium (Low on ARM devices). When a video has to be converted for a Chromecast, trades the quality of the conversion against the processor time and bandwidth it takes. *High*: high quality and high bandwidth. *Medium*: medium quality and medium bandwidth. *Low*: low quality and low bandwidth. *Low CPU*: low quality but high bandwidth, the lightest for the processor. <sub>VLC option: `sout-chromecast-conversion-quality` (0 to 3)</sub>

### Streaming
These rows apply when VLC streams or converts media through its stream output (**Convert / Save**, **Stream**).
- **Play locally while streaming**: toggle. Default: off. Also plays the media in VLC while it is streamed. <sub>VLC option: `sout-display`</sub>
- **Include audio**: toggle. Default: on. Sends the audio tracks to the stream output. <sub>VLC option: `sout-audio`</sub>
- **Include video**: toggle. Default: on. Sends the video tracks to the stream output. <sub>VLC option: `sout-video`</sub>
- **Include subtitles**: toggle. Default: on. Sends the subtitle tracks to the stream output. <sub>VLC option: `sout-spu`</sub>
- **Encoding preset**: dropdown: Ultra fast / Super fast / Very fast / Faster / Fast / Medium / Slow / Slower / Very slow / Placebo. Default: Medium. The x264 preset used when VLC encodes video to H.264: a faster preset uses less processor time, a slower one gives a better picture for the same size. Casting picks its own preset. *(Needs the x264 module.)* <sub>VLC option: `sout-x264-preset` (Medium = empty)</sub>
- **Tuning**: dropdown: None / Film / Animation / Grain / Still image / PSNR / SSIM / Fast decoding / Zero latency. Default: None. The x264 tuning used when VLC encodes video to H.264, to suit the kind of picture or the use (Zero latency for live streaming, for example). *(Needs the x264 module.)* <sub>VLC option: `sout-x264-tune` (None = empty)</sub>

### Advanced Options
- **Send Dolby audio as is**: toggle. Default: off. Passes Dolby (E)AC-3 audio to a Chromecast without converting it. Leave it off unless your receiver plays Dolby. <sub>VLC option: `sout-chromecast-audio-passthrough`</sub>
- **Cast HTTP port**: number field. Default: 8010. The port of the local server the Chromecast reads the media from. Change it if another program uses that port or your firewall only opens another one. <sub>VLC option: `sout-chromecast-http-port`</sub>
- **Keep the stream output open between items**: toggle. Default: off. Keeps one stream output running across the items of the play queue, instead of starting a new one for each item. <sub>VLC option: `sout-keep`</sub>

### Options of VLC behind these settings

| Row | VLC option |
|---|---|
| Save recordings to | `input-record-path` |
| Record without re-encoding when possible | `input-record-native` |
| Discover cast devices | interface setting `redesign/discover-cast-devices` |
| Cast quality | `sout-chromecast-conversion-quality` |
| Play locally while streaming | `sout-display` |
| Include audio | `sout-audio` |
| Include video | `sout-video` |
| Include subtitles | `sout-spu` |
| Encoding preset | `sout-x264-preset` |
| Tuning | `sout-x264-tune` |
| Send Dolby audio as is | `sout-chromecast-audio-passthrough` |
| Cast HTTP port | `sout-chromecast-http-port` |
| Keep the stream output open between items | `sout-keep` |

---

← [Performance](Settings-Performance) · [All settings](Settings) · [Extensions](Settings-Extensions) →
