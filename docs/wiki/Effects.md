# Effects

The **Effects** panel holds the audio effects (equalizer, compressor, spatializer, stereo widener) and the video
effects (picture adjustment, crop, colours, geometry, overlays and more). Every change is heard or seen at once, and
nothing is kept until you **Save**: **Cancel** puts everything back as it was.

## Open it

- **Effects** in the control bar (the equalizer icon): the panel opens above the button. A click on the button
  again, or outside the panel, closes it.
- **Effects and Filters** in the right-click menu (*More Settings*), or <kbd>Ctrl</kbd>+<kbd>E</kbd>: the panel opens in
  the middle of the window.

The audio effects need something playing with sound (*Play audio to preview and edit audio effects.*), the video
effects a video playing (*Play video to preview and edit effects.*). Opened before, the panel waits for them.

## The panel

From top to bottom: the title **Effects** with **Close**, the **preset** field, the tabs **Audio** and **Video**,
the effects of the tab, then **Cancel** and **Save**.

| Button | What it does |
|---|---|
| **Save** | Keeps the effects as they are now. They stay for the next media and the next starts. |
| **Cancel** | Puts every effect back as it was when the panel opened (both tabs). |
| **Close** | Closes the panel. |

Each effect is a section that opens and closes (*Expand settings* / *Collapse settings*), with an **Enable** toggle
and its sliders. A slider shows its value, and **Reset** puts the values of the section back to their defaults.

## Audio

### Equalizer

- **Equalizer preset**: a dropdown that starts with *None*, then VLC's presets, each drawn with its curve: Flat,
  Classical, Club, Dance, Full bass, Full bass and treble, Full treble, Headphones, Large Hall, Live, Party, Pop,
  Reggae, Rock, Ska, Soft, Soft rock, Techno. Moving a band makes it *Custom*.
- **Preamp**: −20 dB to +20 dB.
- **10 bands**, each a slider and a field of its value in dB, from −20 dB to +20 dB: 60 Hz, 170 Hz, 310 Hz,
  600 Hz, 1 kHz, 3 kHz, 6 kHz, 12 kHz, 14 kHz, 16 kHz (or the ISO bands, 31 Hz to 16 kHz, when VLC is set to use
  them).
- **Two-pass equalizer**: filters the sound twice, for a stronger effect.
- **Reset** (*Reset to defaults*).

### Compressor

Evens out loud and quiet parts.

| Slider | Range | Default |
|---|---|---|
| RMS / Peak | 0 to 1 | 0.2 |
| Attack | 1.5 to 400 ms | 25 ms |
| Release | 2 to 800 ms | 100 ms |
| Threshold | −30 to 0 dB | −11 dB |
| Ratio | 1 to 20 : 1 | 4 : 1 |
| Knee Radius | 1 to 10 dB | 5 dB |
| Makeup Gain | 0 to 24 dB | 7 dB |

The *Night mode* of [Settings: Audio](Settings-Audio) sets this compressor too.

### Spatializer

Puts the sound in a room. Sliders shown from 0 to 10 (the size to 11): **Size** (8.5), **Width** (10), **Wet** (4),
**Dry** (5), **Damp** (5).

### Stereo Widener

Makes stereo sound wider.

| Slider | Range | Default |
|---|---|---|
| Delay Time | 1 to 100 ms | 20 ms |
| Feedback Gain | 0 to 90 % | 30 % |
| Crossfeed | 0 to 80 % | 30 % |
| Dry Mix | 0 to 100 % | 80 % |

## Video

Six sections, on two rows: **Essential**, **Crop**, **Colors**, **Geometry**, **Overlay** and **Advanced**. Each
filter has its **Enable** toggle; **Reset** under the section puts the values of all its filters back to their
defaults.

### Essential

| Filter | Controls |
|---|---|
| **Image Adjustment** | Hue (−180 to 180), Brightness (0 to 2), Contrast (0 to 2), Saturation (0 to 3), Gamma (0.01 to 10). 1 is unchanged (0 for the hue). |
| **Sharpen Sigma** | How much the picture is sharpened, 0 to 2. |
| **Banding Removal Radius** | Smooths colour banding, radius 4 to 32 (16). |
| **Film Grain Variance** | Adds film grain, 0 to 10 (2). |

### Crop

**Top**, **Bottom**, **Left** and **Right**, each a field in pixels (*px*). The chain buttons link top and bottom, and
left and right, so both sides move together.

### Colors

| Filter | Controls |
|---|---|
| **Color Extraction** | Keeps one colour, the rest grey: the colour to keep. |
| **Color Threshold** | Keeps the colours close to one: the colour, **Saturation** and **Similarity**. |
| **Negate Colors** | Inverts the colours. |
| **Posterize** | Fewer colours: the number of levels (2 to 256, 6). |
| **Gradient** | Draws the edges: the mode (gradient, edge, hough), **Color** and **Cartoon**. |
| **Sepia Intensity** | A sepia tone, 0 to 255. |

### Geometry

| Filter | Controls |
|---|---|
| **Transform** | Rotate by 90, 180 or 270 degrees, flip horizontally or vertically, transpose, anti-transpose. |
| **Rotate** | Any angle, in degrees. |
| **Interactive Zoom** | A zoom you move with the mouse on the video. |
| **Puzzle Game** | The picture cut into a puzzle to put back together: rows and columns. |

### Overlay

| Filter | Controls |
|---|---|
| **Add Logo** | A picture over the video: the file (**Browse**, *.png* or *.jpg*), its position, X and Y, and its opacity. |
| **Add Text** | A text over the video: the text, its position, colour, size, and how long it stays. |
| **Logo Erase** | Hides a logo of the video: a mask picture and its X and Y. |

### Advanced

| Filter | Controls |
|---|---|
| **Anaglyph 3D** | A 3D video for coloured glasses: the colour scheme (red / cyan, red / green, red / blue, green / magenta, magenta / cyan...). |
| **Mirror** | Splits the picture in two like a mirror: vertical or horizontal, and the direction. |
| **Psychedelic** | Shifting colours. |
| **Waves** | Waves across the picture. |
| **Water Effect** | Ripples, as on water. |
| **Motion Detect** | Outlines what moves. |
| **Spatial Blur** | Blurs the picture: its strength. |
| **Motion Blur** | Blurs what moves: the factor (1 to 127, 80). |
| **Anti-Flickering** | Smooths flicker: the softening (0 to 30). |
| **Denoiser** | Removes noise: spatial and temporal strength, for light and for colour. |

**Open synchronization**, at the end of the Video tab, opens VLC's *Track Synchronization* window (audio, subtitle
and video delays).

## Effect presets

The field at the top of the panel keeps sets of effects under a name. Its list has **New Preset**, the presets you
made, and **Default Preset** at the end; the one in use is checked.

| Action | What it does |
|---|---|
| **New Preset** | Asks for a *Preset Name* (*e.g., Late Night Movies*) and saves the effects playing in it (*The preset keeps the audio and video effects in use.*). New names start as *Custom Preset 01*. |
| A preset | Applies its effects. Like any change, **Save** keeps them and **Cancel** drops them. |
| The ⋯ of a preset | **Rename**, **Update with current effects**, **Delete** (*Delete name?* — the effects in use stay as they are). |
| **Default Preset** | Turns every effect off. |

A preset holds the part of the effects that played when it was saved: the audio, the video or both. Applied, it
changes that part only. A preset of video effects alone needs a video playing (*This preset holds video effects
only: play a video to apply it.*), one of audio effects alone a media with sound.

## Effects and profiles

The effects are among the settings a player profile keeps: switching profile (**⋯ → Profile**) brings the effects
saved with it. See [Customize the player](Customize-the-Player#profiles). The equalizer, the compressor and the other
audio effects are also in [Settings: Audio](Settings-Audio) › *Effects*.

---

[The player](The-Player) · [Settings: Audio](Settings-Audio) · [Home](Home)
