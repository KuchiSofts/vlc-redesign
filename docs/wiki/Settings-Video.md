# Settings: Video

**⋯ → Settings → Video** (or <kbd>Ctrl</kbd>+<kbd>P</kbd>). The video output, the picture (deinterlacing, aspect ratio, cropping, zoom) and snapshots.

Rows under **Advanced Options** are in the collapsed bar at the bottom of the page. Rows marked *Developer mode only* appear there once **Developer mode** is on ([General](Settings-General)). See [how the Settings window works](Settings).

### Output
- **Enable video**: toggle. Default: on. Turns video output off completely, so video is not even decoded. This saves processing power, for example to listen to the sound of a video only. <sub>VLC option: `video`</sub>
- **Start videos in fullscreen**: toggle. Default: off. Starts videos in fullscreen mode. <sub>VLC option: `fullscreen`</sub>
- **Stop the screensaver during playback**: dropdown: Never / When fullscreen / Always. Default: Always. Keeps the screensaver from starting while a video plays: never, only while the video is in fullscreen, or always. <sub>VLC option: `disable-screensaver` (Never = 0, Always = 1, When fullscreen = 2)</sub>
- **Play video as desktop wallpaper**: toggle. Default: off. Shows the video as the background of the desktop instead of in VLC's window. <sub>VLC option: `video-wallpaper`</sub>

### Picture
*(Not a list of VLC options: the sliders of the Picture panel of the design, with VLC's image adjustment filter. They show while a video plays; otherwise the page shows "Play video to adjust and preview the picture." in their place.)*
- **Hue**: slider from −180 to +180 (degrees). Default: 0. Turns the colours around the colour wheel. <sub>VLC option: `hue`</sub>
- **Brightness**: slider from 0 to 2. Default: 1. <sub>VLC option: `brightness`</sub>
- **Contrast**: slider from 0 to 2. Default: 1. <sub>VLC option: `contrast`</sub>
- **Saturation**: slider from 0 to 3 (ticks at 1 and 2). Default: 1. 0 makes the picture black and white. <sub>VLC option: `saturation`</sub>
- **Gamma**: slider from 0.01 to 10 (ticks at each whole value). Default: 1. <sub>VLC option: `gamma`</sub>
- **Reset picture**: button **Reset**. Puts the five sliders back to their defaults and turns the adjustment off.

You see a change at once on the video playing. Moving any slider turns on the image adjustment (`adjust` is added to `video-filter`); the values and the filter are kept when the window closes, so the next videos play with them too.

### Deinterlacing
- **Deinterlacing**: dropdown: Automatic / Off / On. Default: Automatic. Whether VLC removes the comb lines of interlaced video (TV recordings, DVDs). Automatic deinterlaces the videos that say they are interlaced. <sub>VLC option: `deinterlace` (Automatic = −1, Off = 0, On = 1)</sub>
- **Method**: dropdown: Automatic / Discard / Blend / Mean / Bob / Linear / X / Yadif / Yadif (2x) / Phosphor / Film NTSC (IVTC). Default: Automatic. The deinterlacing method used for video processing. Film NTSC (IVTC) is an inverse telecine, for films that were telecined to NTSC. <sub>VLC option: `deinterlace-mode`</sub>

### Framing
- **Aspect ratio**: dropdown: Automatic / 16:9 / 4:3 / 1:1 / 16:10 / 2.21:1 / 2.35:1 / 2.39:1 / 5:4 (plus a value already set if it is another one). Default: Automatic (the ratio of the video). Forces the aspect ratio of the source, for example for a DVD that claims to be 16:9 but is really 4:3, or a video without aspect ratio information. <sub>VLC option: `aspect-ratio`</sub>
- **Crop**: dropdown: None / 16:9 / 4:3 / 1:1 / 16:10 / 2.21:1 / 2.35:1 / 2.39:1 / 5:4 (plus a value already set). Default: None. Crops the picture to this ratio, for example to remove the black bars burnt into a video. <sub>VLC option: `crop`</sub>
- **Scale video to the window**: toggle. Default: on. Lets the video scale to fit the window or the screen in fullscreen. Off, it plays at its own size (with the zoom below). <sub>VLC option: `autoscale`</sub>
- **Zoom**: dropdown: 1:4 Quarter / 1:2 Half / 1:1 Original / 2:1 Double (plus a value already set). Default: 1:1 Original. Zooms the video by this factor. <sub>VLC option: `zoom` (0.25 / 0.5 / 1 / 2)</sub>
- **Position in the window**: dropdown: Center / Left / Right / Top / Bottom / Top-Left / Top-Right / Bottom-Left / Bottom-Right. Default: Center. Where the video sits in its window when it does not fill it. <sub>VLC option: `align`</sub>

### Snapshots
- **Save to**: folder field with a **Browse** button ("Choose a snapshot folder"). Default: empty, which saves in your Pictures folder (the field shows that folder). Where the snapshots of the video are saved. <sub>VLC option: `snapshot-path`</sub>
- **File name prefix**: text field. Default: `vlcsnap-`. The start of the name of each snapshot file; VLC adds the time (or a number, see below) after it. <sub>VLC option: `snapshot-prefix`</sub>
- **Format**: dropdown: PNG / JPG / TIFF / WebP. Default: PNG. The image format of the snapshots. <sub>VLC option: `snapshot-format`</sub>
- **Size**: dropdown: Same as video / 3840 px wide / 1920 px wide / 1280 px wide / 640 px wide (plus a width already set). Default: Same as video. The width of the snapshots. Choosing a width sets **Snapshot height** to 0 (proportional), so the picture is not stretched; *Same as video* sets it back to −1 (the height of the video). <sub>VLC option: `snapshot-width` (Same as video = −1)</sub>
- **Number files sequentially**: toggle. Default: off. Names the snapshots with numbers that follow each other instead of the time. <sub>VLC option: `snapshot-sequential`</sub>
- **Show a preview after capture**: toggle. Default: on. Shows the snapshot for a moment in the top-left corner of the video after it is taken. <sub>VLC option: `snapshot-preview`</sub>

### Advanced Options
- **Video output module**: dropdown: Automatic, then the video output modules of this build, then Disable. Default: Automatic (`any`), which picks the best method available. The method VLC uses to draw video. *(Developer mode only.)* <sub>VLC option: `vout`</sub>
- **Snapshot height (0: proportional, -1: original)**: number field, in pixels. Default: −1. The height of the snapshots: −1 keeps the height of the video, 0 computes it from the width so the picture keeps its proportions. **Size** sets it for you: 0 with a width, −1 with *Same as video*. *(Developer mode only.)* <sub>VLC option: `snapshot-height`</sub>
- **Deinterlacing filter**: text field. Default: `deinterlace`. The deinterlacing module VLC uses. *(Developer mode only.)* <sub>VLC option: `deinterlace-filter`</sub>
- **Custom aspect ratios**: text field. Default: empty. A comma-separated list of aspect ratios (for example `3:2,21:9`) that VLC adds to the aspect ratios it offers for a playing video. <sub>VLC option: `custom-aspect-ratios`</sub>
- **Custom crop ratios**: text field. Default: empty. A comma-separated list of crop ratios that VLC adds to the crop ratios it offers for a playing video. <sub>VLC option: `custom-crop-ratios`</sub>
- **Monitor pixel aspect ratio**: text field. Default: empty (a grey `1:1`: square pixels). Forces the aspect ratio of the pixels of your screen. Most screens have square pixels (1:1); change it only for a screen whose pixels are not square, so pictures keep their proportions. <sub>VLC option: `monitor-par`</sub>
- **Time-shift buffer size**: the same setting as [Playback](Settings-Playback) > Advanced Options, repeated here. Dropdown: Small (10 MiB) / Medium (50 MiB) / Large (200 MiB). Default: Medium. <sub>VLC option: `input-timeshift-granularity`</sub>

### Options of VLC behind these settings

| Row | VLC option |
|---|---|
| Enable video | `video` |
| Start videos in fullscreen | `fullscreen` |
| Stop the screensaver during playback | `disable-screensaver` |
| Play video as desktop wallpaper | `video-wallpaper` |
| Hue, Brightness, Contrast, Saturation, Gamma | `hue`, `brightness`, `contrast`, `saturation`, `gamma` (with `adjust` in `video-filter`) |
| Reset picture | *(an action: the five above to their defaults, `adjust` removed)* |
| Deinterlacing | `deinterlace` |
| Method | `deinterlace-mode` |
| Aspect ratio | `aspect-ratio` |
| Crop | `crop` |
| Scale video to the window | `autoscale` |
| Zoom | `zoom` |
| Position in the window | `align` |
| Save to | `snapshot-path` |
| File name prefix | `snapshot-prefix` |
| Format | `snapshot-format` |
| Size | `snapshot-width` |
| Number files sequentially | `snapshot-sequential` |
| Show a preview after capture | `snapshot-preview` |
| Video output module | `vout` |
| Snapshot height (0: proportional, -1: original) | `snapshot-height` |
| Deinterlacing filter | `deinterlace-filter` |
| Custom aspect ratios | `custom-aspect-ratios` |
| Custom crop ratios | `custom-crop-ratios` |
| Monitor pixel aspect ratio | `monitor-par` |
| Time-shift buffer size | `input-timeshift-granularity` |

---

← [Playback](Settings-Playback) · [All settings](Settings) · [Audio](Settings-Audio) →
