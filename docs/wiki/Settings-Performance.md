# Settings: Performance

**⋯ → Settings → Performance** (or <kbd>Ctrl</kbd>+<kbd>P</kbd>). Hardware decoding and the options that keep playback smooth on slow computers. The page is titled *Performance Settings*.

Rows under **Advanced Options** are in the collapsed bar at the bottom of the page. The rows there are under a *Developer* heading, but they show whether **Developer mode** is on or not. See [how the Settings window works](Settings).

### Decoding
- **Hardware-accelerated decoding**: dropdown: Automatic / Disabled, then the hardware decoder devices of this build (on Windows, the Direct3D ones, for example). Default: Automatic. Lets the graphics card decode the video when it can, which uses much less processor time. *Automatic* picks the best device available; *Disabled* decodes everything with the processor. Turn it off if a video shows artefacts or green frames with hardware decoding. <sub>VLC option: `dec-dev` (Automatic = `any`, Disabled = `none`)</sub>
- **Video post-processing**: dropdown: Off / Lowest / Low / Medium / High / Higher / Highest. Default: Off. Improves the picture of poorly encoded videos (blocks, ringing), at the cost of processing power. Any level other than Off adds the `postproc` filter to the video filters and sets its quality (1 to 6); Off removes the filter. *(Needs the postproc module.)* <sub>VLC options: `video-filter` (`postproc`) and `postproc-q`</sub>
- **Skip the H.264 loop filter**: dropdown: Never / Non-reference frames / Bidirectional frames / Non-key frames / Always. Default: Never. Skips the loop filter (deblocking) when decoding H.264, on the frames chosen. It usually lowers the quality of the picture, but makes decoding of high-definition video much faster: try it on a computer too slow for a video. <sub>VLC option: `avcodec-skiploopfilter` (0 to 4)</sub>
- **Decoding threads**: number field. Default: 0 (automatic). How many threads the FFmpeg decoder uses; 0 lets it decide from the processor. <sub>VLC option: `avcodec-threads`</sub>

### Smooth Playback
- **Drop frames when the system can't keep up**: toggle. Default: on. Drops the video frames that reach the video output after the time they should have been shown, so the video keeps up with the audio. Off, every frame is shown, even late. <sub>VLC option: `drop-late-frames`</sub>
- **Increase the priority of the process**: toggle. Default: off. While **Process priority** is Normal, raises VLC's priority to Above normal, so other programs disturb playback less. VLC warns that, in some circumstances (bugs), a raised priority can let VLC take all the processor time and make the whole system unresponsive. *(Windows only.)* <sub>VLC option: `high-priority`</sub>
- **Process priority**: dropdown: Normal / Above normal / High / Below normal / Idle. Default: Normal. How much processor time Windows gives VLC compared with the other programs (the Windows priority classes). It applies when the settings are saved. *(Applied on Windows.)* <sub>Interface setting: `redesign/process-priority` (−2 Idle to 2 High)</sub>
- **Rescan plugins**: button **Rescan**. Runs `vlc-cache-gen.exe` on VLC's `plugins` folder to rebuild the plugin cache (`plugins.dat`). Use it after adding or removing plugin files by hand. If the folder of VLC needs administrator rights (an installation under Program Files), it reports "The plugins could not be rescanned: the folder of VLC may need administrator rights."; an installation without `vlc-cache-gen.exe` reports "The plugins can not be rescanned on this installation."

### Advanced Options
- *Developer* heading.
  - **Encoder**: dropdown: Automatic, then the video encoder modules of this build. Default: Automatic. The video encoder module used (with its options) when VLC converts video for **Convert / Save** and streaming. <sub>VLC option: `sout-transcode-venc` (Automatic = `none`)</sub>
  - **Decoder**: dropdown: Automatic (plus a list of decoders already set, shown as it is). Default: Automatic. The list of decoders VLC tries first. VLC warns that only advanced users should change it, as it can break the playback of all your media. <sub>VLC option: `codec` (Automatic = `any`)</sub>
  - **Clock synchronisation**: dropdown: Default / Disable / Enable. Default: Default. Turns off (or forces) the synchronisation to the clock of the input for real-time sources. Try *Disable* if network streams play jerkily. <sub>VLC option: `clock-synchro` (−1 / 0 / 1)</sub>
  - **Clock jitter**: number field, in ms. Default: 5000. The largest input delay jitter VLC's synchronisation tries to compensate. <sub>VLC option: `clock-jitter`</sub>
  - **Clock reference averaging**: number field. Default: 40. How many clock references VLC averages. For a PVR capture card or a very irregular source, VLC advises 10000. <sub>VLC option: `cr-average`</sub>

### Options of VLC behind these settings

| Row | VLC option |
|---|---|
| Hardware-accelerated decoding | `dec-dev` |
| Video post-processing | `video-filter` (`postproc`), `postproc-q` |
| Skip the H.264 loop filter | `avcodec-skiploopfilter` |
| Decoding threads | `avcodec-threads` |
| Drop frames when the system can't keep up | `drop-late-frames` |
| Increase the priority of the process | `high-priority` |
| Process priority | interface setting `redesign/process-priority` |
| Encoder | `sout-transcode-venc` |
| Decoder | `codec` |
| Clock synchronisation | `clock-synchro` |
| Clock jitter | `clock-jitter` |
| Clock reference averaging | `cr-average` |

---

← [Network](Settings-Network) · [All settings](Settings) · [Recording](Settings-Recording) →
