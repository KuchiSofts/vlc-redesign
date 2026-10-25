# Settings: Video

**⋯ → Settings → Video** (<kbd>Ctrl</kbd>+<kbd>P</kbd>; the header reads *Video Defaults*). The video output, the
picture adjustment, deinterlacing, framing (aspect ratio, crop, zoom, position) and snapshots. These are the
defaults every video starts with; the player's menus and [Effects](Effects) change them for the video playing.
Rows apply without a restart, most of them from the next video. See
[how the Settings window works](Settings#how-the-window-works).

## Output

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Enable video** | Toggle. Default: on. | Off: no video at all, not even decoded, which saves processing power when you only want the sound of a video. | `video` |
| **Start videos in fullscreen** | Toggle. Default: off. | Each video starts in fullscreen. | `fullscreen` |
| **Stop the screensaver during playback** | Dropdown: *Never* / *When fullscreen* / *Always*. Default: *Always*. | Keeps the screensaver and screen power-saving away while a video plays: never, only while the video is in fullscreen, or always. | `disable-screensaver` (Never = 0, Always = 1, When fullscreen = 2) |
| **Play video as desktop wallpaper** | Toggle. Default: off. | Shows the video as the background of the desktop instead of in a window. | `video-wallpaper` |

## Picture

Not a list of VLC options: the five sliders of the design's Picture panel, driving VLC's image adjustment filter.
They appear while a video plays; otherwise the page reads *Play video to adjust and preview the picture.* in their
place.

| Slider | Range / default | What it does | VLC option |
|---|---|---|---|
| **Hue** | −180 to +180 (degrees). Default: 0. | Turns the colours around the colour wheel. | `hue` |
| **Brightness** | 0 to 2. Default: 1. | Darker below 1, brighter above. | `brightness` |
| **Contrast** | 0 to 2. Default: 1. | Flatter below 1, stronger above. | `contrast` |
| **Saturation** | 0 to 3 (ticks at 1 and 2). Default: 1. | 0 is black and white; above 1 the colours are stronger. | `saturation` |
| **Gamma** | 0.01 to 10 (ticks at each whole value). Default: 1. | Lightens or darkens the mid-tones. | `gamma` |
| **Reset picture** | Button **Reset**. | Puts the five sliders back to their defaults and turns the adjustment off. | |

The video playing changes as you move a slider. The first move turns the adjustment on (`adjust` is added to
`video-filter`). When the window closes, the values and the filter are saved, so the next videos play with them
too. See also [Effects › Video](Effects#video).

## Deinterlacing

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Deinterlacing** | Dropdown: *Off* / *Automatic* / *On*. Default: *Automatic*. | Whether VLC removes the comb lines of interlaced video (TV recordings, DVDs, camcorders). *Automatic* deinterlaces the videos that say they are interlaced. | `deinterlace` (Off = 0, Automatic = −1, On = 1) |
| **Method** | Dropdown: *Automatic* / *Discard* / *Blend* / *Mean* / *Bob* / *Linear* / *X* / *Yadif* / *Yadif (2x)* / *Phosphor* / *Film NTSC (IVTC)*. Default: *Automatic*. | How the lines are combined when deinterlacing applies. *Yadif* and *Yadif (2x)* give the best quality for most sources; *Bob*, *Linear*, *Yadif (2x)* and *Phosphor* double the frame rate, for smoother motion; *Film NTSC (IVTC)* is an inverse telecine for films telecined to NTSC. | `deinterlace-mode` (`auto`, `discard`, `blend`, `mean`, `bob`, `linear`, `x`, `yadif`, `yadif2x`, `phosphor`, `ivtc`) |

## Framing

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Aspect ratio** | Dropdown: *Automatic* / 16:9 / 4:3 / 1:1 / 16:10 / 2.21:1 / 2.35:1 / 2.39:1 / 5:4, plus a value already set if it is another one. Default: *Automatic* (the video's own ratio). | Forces the aspect ratio of every video, for example for a DVD that claims 16:9 but is really 4:3, or a video without that information. | `aspect-ratio` |
| **Crop** | Dropdown: *None* / 16:9 / 4:3 / 1:1 / 16:10 / 2.21:1 / 2.35:1 / 2.39:1 / 5:4, plus a value already set. Default: *None*. | Crops every video to this ratio, for example to remove black bars burnt into the picture. | `crop` |
| **Scale video to the window** | Toggle. Default: on. | The video fits the window, or the screen in fullscreen. Off: it plays at its own size, multiplied by **Zoom**. | `autoscale` |
| **Zoom** | Dropdown: *1:4 Quarter* / *1:2 Half* / *1:1 Original* / *2:1 Double*, plus a value already set. Default: *1:1 Original*. | Zooms the video by this factor. It only has an effect while **Scale video to the window** is off. | `zoom` (0.25 / 0.5 / 1 / 2) |
| **Position in the window** | Dropdown: *Center* / *Left* / *Right* / *Top* / *Bottom* / *Top-Left* / *Top-Right* / *Bottom-Left* / *Bottom-Right*. Default: *Center*. | Where the video sits in its window when it does not fill it. | `align` (0, 1, 2, 4, 8, 5, 6, 9, 10) |

## Snapshots

Take a snapshot with <kbd>Shift</kbd>+<kbd>S</kbd> or the player's menu.

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Save to** | Folder field with **Browse** (*Choose a snapshot folder*). Default: empty; the field shows your Pictures folder (written from your home folder, as `~\Pictures`). | Where snapshots are saved. Empty: VLC saves them in the Windows *Screenshots* folder (normally `Pictures\Screenshots`) when Windows has it, otherwise in Pictures. | `snapshot-path` |
| **File name prefix** | Text field. Default: `vlcsnap-`. | The start of each file name. VLC adds the date and time after it, as in `vlcsnap-2026-10-25-14h03m12s….png` (or a number, see below). | `snapshot-prefix` |
| **Format** | Dropdown: *PNG* / *JPG* / *TIFF* / *WebP*. Default: *PNG*. | The image format. PNG is lossless; JPG and WebP are smaller. | `snapshot-format` |
| **Size** | Dropdown: *Same as video* / *3840 px wide* / *1920 px wide* / *1280 px wide* / *640 px wide*, plus a width already set. Default: *Same as video*. | The width of the snapshots. Choosing a width while **Snapshot height** is −1 sets it to 0, so the height follows the width and the picture keeps its proportions. *Same as video* sets a height of 0 back to −1. A height you typed yourself is left alone. | `snapshot-width` (*Same as video* = −1) |
| **Number files sequentially** | Toggle. Default: off. | Names the snapshots with numbers that follow each other (`vlcsnap-00001.png`…) instead of the date and time. | `snapshot-sequential` |
| **Show a preview after capture** | Toggle. Default: on. | Shows the snapshot for a moment in the top-left corner of the video. | `snapshot-preview` |

## Advanced Options

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Video output module** *(Developer mode)* | Dropdown: *Automatic*, then the video outputs of this build by name, then *Disable*. Default: *Automatic* (`any`). | The method VLC uses to draw video. *Automatic* picks the best one available; *Disable* shows no video. Leave it on *Automatic* unless you are troubleshooting. | `vout` |
| **Snapshot height (0: proportional, -1: original)** *(Developer mode)* | Number field, in pixels. Default: −1. | The height of the snapshots: −1 keeps the video's height, 0 computes it from the width so the picture keeps its proportions, any other number forces that height. **Size** sets it for you (see above). | `snapshot-height` |
| **Deinterlacing filter** *(Developer mode)* | Text field. Default: `deinterlace`. | The module VLC uses to deinterlace. | `deinterlace-filter` |
| **Custom aspect ratios** | Text field. Default: empty. | A comma-separated list (for example `3:2,21:9`) added to the aspect ratios the player offers for a playing video. | `custom-aspect-ratios` |
| **Custom crop ratios** | Text field. Default: empty. | A comma-separated list added to the crop ratios the player offers for a playing video. | `custom-crop-ratios` |
| **Monitor pixel aspect ratio** | Text field. Default: empty (a grey `1:1`, square pixels). | Forces the shape of your screen's pixels. Nearly every screen has square pixels; change it only for one that does not, so pictures keep their proportions. | `monitor-par` |
| **Time-shift buffer size** | Dropdown: *Small* / *Medium* / *Large*. Default: *Medium*. | The same setting as on [Playback](Settings-Playback#advanced-options), shown again here: *Small* 10 MiB, *Medium* 50 MiB, *Large* 200 MiB per time-shift file. | `input-timeshift-granularity` |

## Good to know

- **Black bars burnt into a video:** use **Crop** with the ratio of the film (2.35:1 or 2.39:1 for most
  widescreen films). For one video only, use the player's crop menu instead.
- **Stretched or squeezed picture:** set **Aspect ratio**; if every video looks wrong on one particular screen,
  check **Monitor pixel aspect ratio**.
- **Interlaced lines in motion** (TV recordings): leave **Deinterlacing** on *Automatic* and try **Method**
  *Yadif (2x)*; force *On* for a file that does not say it is interlaced.
- **Smaller snapshots:** choose a **Size** such as *1280 px wide* and **Format** *JPG* or *WebP*.
- **Picture adjustment applies to every video once saved.** Use **Reset picture** to go back to the original
  colours.

---

← [Playback](Settings-Playback) · [All settings](Settings) · [Audio](Settings-Audio) →
