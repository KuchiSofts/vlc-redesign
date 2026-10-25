# Settings: Performance

**⋯ → Settings → Performance** (<kbd>Ctrl</kbd>+<kbd>P</kbd>; the header reads *Performance Settings*). Hardware
decoding, the decoder options that help a slow computer, the priority Windows gives VLC, the plugin cache, and a few
expert options of VLC's codecs and clock. The Advanced Options rows are under a *Developer* heading but show whether
Developer mode is on or not. See [how the Settings window works](Settings#how-the-window-works).

## Decoding

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Hardware-accelerated decoding** | Dropdown: *Automatic* / *Disabled*, then the hardware decoder devices of this build (on Windows, the Direct3D 11 and Direct3D 9 ones). Default: *Automatic*. | Lets the graphics card decode the video when it can, which takes much less processor time and power. *Automatic* picks the best device available; *Disabled* decodes everything with the processor. | `dec-dev` (Automatic = `any`, Disabled = `none`) |
| **Video post-processing** | Dropdown: *Off* / *Lowest* / *Low* / *Medium* / *High* / *Higher* / *Highest*. Default: *Off*. | Improves the picture of poorly encoded videos (blocks, ringing) at the cost of processing power. A level other than *Off* adds the `postproc` filter to the video filters with that quality (1 to 6); *Off* removes it. Needs the postproc module. | `video-filter` (`postproc`), `postproc-q` |
| **Skip the H.264 loop filter** | Dropdown: *Never* / *Non-reference frames* / *Bidirectional frames* / *Non-key frames* / *Always*. Default: *Never*. | Skips the loop filter (the deblocking) when VLC's FFmpeg decoder decodes H.264, on the frames chosen. It usually lowers the picture quality, but makes high-definition decoding much faster: try it on a computer too slow for a video, starting with *Non-reference frames*. | `avcodec-skiploopfilter` (0 to 4) |
| **Decoding threads** | Number field. Default: 0 (automatic). | How many threads the FFmpeg decoder uses; 0 lets it decide from the number of processor cores. | `avcodec-threads` |

## Smooth Playback

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Drop frames when the system can't keep up** | Toggle. Default: on. | Drops the video frames that reach the video output after the time they should have been shown, so the picture keeps up with the sound. Off, every frame is shown, even late, and the picture can fall behind. | `drop-late-frames` |
| **Increase the priority of the process** | Toggle. Default: off. | While *Process priority* is *Normal*, raises VLC to *Above normal*, so other programs disturb playback less. VLC warns that in some circumstances (bugs) a raised priority can let it take all the processor time and make the whole system unresponsive. | `high-priority` |
| **Process priority** | Dropdown: *Normal* / *Above normal* / *High* / *Below normal* / *Idle*. Default: *Normal*. | How much processor time Windows gives VLC compared with the other programs (Windows' priority classes). Applied when the settings are saved and each time VLC starts. *Below normal* and *Idle* keep VLC from slowing other work, at the risk of stutter while the computer is busy. | `redesign/process-priority` (interface setting, −2 Idle to 2 High) |
| **Rescan plugins** | Button **Rescan**. | Runs `vlc-cache-gen.exe` on VLC's `plugins` folder to rebuild the plugin cache (`plugins\plugins.dat`), which VLC reads at startup to find its modules quickly. Use it after adding or removing plugin files by hand. Nothing is shown when it works. If the cache could not be written (an installation under *Program Files* needs administrator rights): "The plugins could not be rescanned: the folder of VLC may need administrator rights."; if the program could not start: "The plugins could not be rescanned."; without `vlc-cache-gen.exe`: "The plugins can not be rescanned on this installation." | |

## Advanced Options

Under a **Developer** heading:

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Encoder** | Dropdown: *Automatic*, then the video encoder modules of this build. Default: *Automatic*. | The video encoder module VLC uses (with its options) when it converts video, for **Convert / Save** and streaming. *Automatic* lets VLC pick one for the codec asked for. | `sout-transcode-venc` (Automatic = `none`) |
| **Decoder** | Dropdown: *Automatic* (and a list of decoders already set, shown as it is). Default: *Automatic*. | The decoders VLC tries first. VLC warns that only advanced users should change it, as it can break the playback of all your media; the list itself is set on the command line or in *All VLC options* ([General](Settings-General)). | `codec` (Automatic = `any`) |
| **Clock synchronisation** | Dropdown: *Default* / *Disable* / *Enable*. Default: *Default*. | Turns off (or forces) the synchronisation to the clock of the source for real-time sources. Try *Disable* if network streams play jerkily. | `clock-synchro` (−1 / 0 / 1) |
| **Clock jitter** | Number field, in ms. Default: 5000. | The largest input delay jitter VLC's synchronisation tries to compensate. | `clock-jitter` |
| **Clock reference averaging** | Number field. Default: 40. | How many clock references VLC averages. For a PVR capture card or a very irregular source, VLC advises 10000. | `cr-average` |

## Good to know

- **Green or broken frames, or a black picture, with some videos:** set *Hardware-accelerated decoding* to
  *Disabled*. If that fixes it, a graphics driver update usually lets you turn it back on. See also
  [Troubleshooting and FAQ](Troubleshooting-and-FAQ#video-problems-no-picture-black-screen-stutter-wrong-colours).
- **A 4K or high-bitrate video stutters:** keep hardware decoding on *Automatic*; on an older computer without a
  hardware decoder for that codec, try *Skip the H.264 loop filter* (H.264 only) and keep *Drop frames when the
  system can't keep up* on.
- **Post-processing** helps old, low-bitrate files; on a good encode it only costs processor time.
- **On battery,** hardware decoding saves the most power.
- **The clock options** are for live and network sources with problems; leave them at their defaults otherwise.

**Related pages:** [Settings: Video](Settings-Video) · [Settings: Recording](Settings-Recording) (streaming and the x264 encoder) · [Troubleshooting and FAQ](Troubleshooting-and-FAQ)

---

← [Network](Settings-Network) · [All settings](Settings) · [Recording](Settings-Recording) →
