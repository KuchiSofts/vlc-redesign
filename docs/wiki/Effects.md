# Effects

The **Effects** panel holds the audio effects (equalizer, compressor, spatializer, stereo widener) and the video
effects (picture adjustment, crop, colours, geometry, overlays and more), with presets that keep a set of effects
under a name. Every change is heard or seen at once, as a preview; **Save** keeps it, and closing the panel any
other way puts everything back.

![The Effects panel above its button over a playing video: the title Effects with Close, the preset field reading Default Preset, the tabs Audio and Video, the Equalizer, Compressor, Spatializer and Stereo Widener each with an OFF toggle and Expand Settings, and Cancel and Save](images/effects.jpg)

## Open it

| From | Where the panel opens |
|---|---|
| **Effects** in the control bar (the equalizer icon) | Right above the button. A second click on the button, a click outside the panel or <kbd>Esc</kbd> closes it, without pausing the video. |
| **More Settings → Effects and Filters** in the right-click menu, or <kbd>Ctrl</kbd>+<kbd>E</kbd> | In the middle of the window, over a dimmed window. |

The audio effects need something playing with sound (*Play audio to preview and edit audio effects.*), the video
effects a video playing (*Play video to preview and edit effects.*). Opened before, the panel waits and starts the
preview when they play.

While the panel is open, the keyboard shortcuts wait: <kbd>Space</kbd> does not pause until it closes.

## The panel

From top to bottom: the title **Effects** with **Close**, the **preset** field, the tabs **Audio** and **Video**,
the effects of the tab, then **Cancel** and **Save**.

| Button | What it does |
|---|---|
| **Save** | Keeps the effects as they are now (see [What Save keeps](#what-save-keeps)) and closes the panel. Dimmed while nothing plays. |
| **Cancel** | Puts every effect back as it was when the panel opened, on both tabs, and closes the panel. |
| **Close**, a click outside, <kbd>Esc</kbd> | Close the panel. Changes not saved are dropped, as with Cancel. |

Each effect is a section with an **Enable** toggle (*ON* / *OFF*) and a row **Expand Settings** / **Collapse
Settings** that shows its controls. A slider shows its value; **Reset** puts the values of the section back to their
defaults.

### What Save keeps

| Effects | Kept by Save |
|---|---|
| Audio: equalizer, compressor, spatializer, stereo widener | Yes: for the next media and the next starts. |
| Video: **Image Adjustment** (hue, brightness, contrast, saturation, gamma) | Yes: for the next media and the next starts. |
| Video: every other filter, and the **crop** | No: they apply to the video playing only, and end with it. Keep them in an [effect preset](#effect-presets) to bring them back. |

Saved effects are also kept in the player profile in use (see [Effects and profiles](#effects-and-profiles)).

## Audio

### Equalizer

- **Equalizer preset**: a dropdown of VLC's 18 presets, each drawn with its curve: Flat, Classical, Club, Dance,
  Full bass, Full bass and treble, Full treble, Headphones, Large Hall, Live, Party, Pop, Reggae, Rock, Ska, Soft,
  Soft rock, Techno. Moving a band away from the preset makes it read *Custom*. A preset also sets the preamp.
- **Preamp**: −20 dB to +20 dB.
- **10 bands**, each a slider and a field with its value in dB, from −20 dB to +20 dB: 60 Hz, 170 Hz, 310 Hz,
  600 Hz, 1 kHz, 3 kHz, 6 kHz, 12 kHz, 14 kHz, 16 kHz. With VLC's *ISO bands* chosen instead, 31 Hz, 63 Hz,
  125 Hz, 250 Hz, 500 Hz, 1 kHz, 2 kHz, 4 kHz, 8 kHz, 16 kHz.
- **Two-pass equalizer**: filters the sound twice, for a stronger effect.
- **Reset** (*Reset to defaults*).

To turn the equalizer off, use its **Enable** toggle.

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

*Night mode* in [Settings: Audio](Settings-Audio) turns this compressor on with its own values (ratio 8:1, threshold
−30 dB, makeup gain +9 dB).

### Spatializer

Puts the sound in a room. The sliders show 0 to 10 (the size up to 11):

| Slider | Range | Default |
|---|---|---|
| Size | 0 to 11 | 8.5 |
| Width | 0 to 10 | 10 |
| Wet | 0 to 10 | 4 |
| Dry | 0 to 10 | 5 |
| Damp | 0 to 10 | 5 |

### Stereo Widener

Makes stereo sound wider.

| Slider | Range | Default |
|---|---|---|
| Delay Time | 1 to 100 ms | 20 ms |
| Feedback Gain | 0 to 90 % | 30 % |
| Crossfeed | 0 to 80 % | 30 % |
| Dry Mix | 0 to 100 % | 80 % |

## Video

Six sections, chosen with six buttons on two rows: **Essential**, **Crop**, **Colors**, **Geometry**, **Overlay**
and **Advanced**. Each filter has its **Enable** toggle; **Reset** under the section puts the values of all its
filters back to their defaults.

### Essential

| Filter | Controls |
|---|---|
| **Image Adjustment** | Hue (−180 to 180, 0), Brightness (0 to 2, 1), Contrast (0 to 2, 1), Saturation (0 to 3, 1), Gamma (0.01 to 10, 1). The only video filter that Save keeps. |
| **Sharpen Sigma** | How much the picture is sharpened, 0 to 2 (0.05). |
| **Banding Removal Radius** | Smooths colour banding, radius 4 to 32 (16). |
| **Film Grain Variance** | Adds film grain, 0 to 10 (2). |

### Crop

**Top**, **Bottom**, **Left** and **Right**, each a field in pixels (*px*). The chain buttons (*Link top and bottom
crop*, *Link left and right crop*) make both sides move together. The crop applies to the video playing.

### Colors

| Filter | Controls |
|---|---|
| **Color Extraction** | Keeps one colour, the rest grey: the colour to keep. |
| **Color Threshold** | Keeps the colours close to one, the rest grey: the colour, **Saturation** and **Similarity** (0 to 100). |
| **Negate Colors** | Inverts the colours. |
| **Posterize** | Fewer colours: the number of levels (2 to 256, 6). |
| **Gradient** | Draws the edges: the mode (*Gradient*, *Edge*, *Hough*), **Color** and **Cartoon**. |
| **Sepia Intensity** | A sepia tone, 0 to 255 (120). |

### Geometry

| Filter | Controls |
|---|---|
| **Transform** | Rotate by 90, 180 or 270 degrees, flip horizontally or vertically, transpose, anti-transpose. |
| **Rotate** | Any angle, 0 to 359 degrees. |
| **Interactive Zoom** | A zoom you move with the mouse on the video. |
| **Puzzle Game** | The picture cut into a puzzle to put back together: rows and columns. |

### Overlay

| Filter | Controls |
|---|---|
| **Add Logo** | A picture over the video: the file (**Browse**, *.png* or *.jpg*), its position, X and Y, and its opacity. |
| **Add Text** | A text over the video: the text, its position, colour, size (0 to 256) and how long it stays. |
| **Logo Erase** | Hides a logo of the video: a mask picture and its X and Y. |

### Advanced

| Filter | Controls |
|---|---|
| **Anaglyph 3D** | A 3D video for coloured glasses: red / green, red / blue, red / cyan, green / magenta or magenta / cyan. |
| **Mirror** | Splits the picture in two like a mirror: vertical or horizontal, and the direction. |
| **Psychedelic** | Shifting colours. |
| **Waves** | Waves across the picture. |
| **Water Effect** | Ripples, as on water. |
| **Motion Detect** | Outlines what moves. |
| **Spatial Blur** | Blurs the picture: its strength (0.01 to 8). |
| **Motion Blur** | Blurs what moves: the factor (1 to 127, 80). |
| **Anti-Flickering** | Smooths flicker: the softening (0 to 31, 10). |
| **Denoiser** | Removes noise: spatial and temporal strength, for light and for colour (0 to 8 each). |

**Open synchronization**, at the end of the Video tab, drops the changes not saved and opens VLC's *Track
Synchronization* window (audio, subtitle and video delays).

## Effect presets

The field at the top of the panel keeps sets of effects under a name. Its list has **New Preset**, the presets you
made (in the order you made them), and **Default Preset** at the end; the one that matches the effects in use is
checked.

| Action | What it does |
|---|---|
| **New Preset** | Asks for a *Preset Name* (*e.g., Late Night Movies*; it starts as *Custom Preset 01*, *02*...) and saves the effects in use in it: *The preset keeps the audio and video effects in use.* |
| Choose a preset | Applies its effects. Like any change, **Save** keeps them and **Cancel** drops them. |
| The ⋯ of a preset | **Rename** (*The preset keeps its effects.*), **Update with current effects**, **Delete** (*Delete name?* — *The effects in use stay as they are.*). |
| **Default Preset** | Turns every effect off: no filter, no crop. |

How presets behave:

- A preset holds the part of the effects that played when it was saved: the audio, the video or both. Applied, it
  changes that part only.
- A preset of video effects alone needs a video playing (*This preset holds video effects only: play a video to
  apply it.*); one of audio effects alone a media with sound (*This preset holds audio effects only: play a media
  with sound to apply it.*).
- A preset keeps every video filter and the crop, not only the picture adjustment: applying it is the way to get a
  filter back on another video.
- Two presets cannot have the same name: the panel says *A preset named "…" already exists.*

## Effects and profiles

The effects are among the settings a player profile keeps: switching profile (**⋯ → Profile**) brings the effects
saved with it. See [Customize the player](Customize-the-Player#profiles). The equalizer, the compressor (*Night
mode*) and the other audio effects are also in [Settings: Audio](Settings-Audio) › *Effects*; the picture
adjustment in [Settings: Video](Settings-Video).

## Good to know

- **My changes were lost.** The panel was closed without **Save** (a click outside it, **Close** or <kbd>Esc</kbd>
  drop the preview). Click **Save** before closing.
- **A filter is gone on the next video.** Only the audio effects and the picture adjustment are kept by Save; keep
  the others in a preset and apply it.
- **The controls are dimmed.** Nothing plays with sound (audio tab) or no video plays (video tab).
- **Sound too quiet with the equalizer on.** Lower bands and a low preamp reduce the volume; raise the **Preamp**.

---

[The player](The-Player) · [Settings: Audio](Settings-Audio) · [Settings: Video](Settings-Video) · [Customize the player](Customize-the-Player) · [Home](Home)
