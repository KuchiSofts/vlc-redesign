# Settings: Audio

**⋯ → Settings → Audio** (<kbd>Ctrl</kbd>+<kbd>P</kbd>; the header reads *Audio Defaults*). The audio output and
the speakers, the volume and its limits, loudness levelling, the equalizer and the other audio effects, the preferred
audio language and the visualizations. These are the defaults of every media; the **Effects** panel of the player
([Effects](Effects#audio)) changes the sound of what is playing, with the same VLC options. A row whose VLC module is
missing from the installation is not shown. See [how the Settings window works](Settings#how-the-window-works).

![Settings: Audio](images/settings-audio.png)

## Output

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Enable audio** | Toggle. Default: on. | Off: no sound at all; the audio is not even decoded, which saves processing power. | `audio` |
| **Output device** | Dropdown: *System default*, then the audio devices Windows reports. Default: *System default*. | The device VLC plays through. *System default* is the default device of Windows. | `mmdevice-audio-device` (System default = empty) |
| **Speaker setup** | Dropdown: *Automatic* / *Stereo* / *Reverse stereo* / *Left* / *Right* / *Dolby Surround* / *Mono*. Default: *Automatic*. | How a stereo output is arranged. *Reverse stereo* swaps left and right; *Left* and *Right* play only that channel; *Mono* mixes all the channels into one; *Dolby Surround* marks the stereo output as Dolby Surround encoded. *Automatic* keeps the layout of the media. | `stereo-mode` (Automatic = 0) |

## Volume

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Maximum volume** | Slider, 100 % to 300 %, ticks every 25 %, labels 100 %, 200 %, 300 %. Default: 200 %. | The highest volume the volume slider, the mouse wheel and the volume keys reach. Above 100 % VLC amplifies the sound itself, which can distort it. | `qt-max-volume` |
| **Startup volume** | Two choices: *Remember last volume*, or *Always starts at:* with a percentage field (0 % to 125 %; a value outside is brought into that range). Default: *Remember last volume* (the field shows 100 %). | Whether VLC starts at the volume it had when it was closed, or always at the same volume. Clicking or typing in the field picks *Always starts at*. | `volume-save` (Remember = on), `mmdevice-volume` (the starting volume, 1.0 = 100 %) |
| **Volume step** | Number field, in percent, 0.4 to 100. Default: 5. | How much one volume-up or volume-down step (a key, a wheel notch) changes the volume. | `volume-step` (in 256ths: 5 % = 12.8) |

## Loudness

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Normalize loudness** | Toggle. Default: off. | Turns on VLC's volume normalizer, which brings loud passages down towards the **Target level**. | `audio-filter` (`normvol`) |
| **Target level** | Number field. Default: 2. | When the average power of the last audio buffers is above this value, the volume is lowered. Values from 0.5 to 10 are sensible; a lower value levels more. Used while *Normalize loudness* is on. | `norm-max-level` |
| **Night mode** | Toggle. Default: off. | Narrows the gap between loud and quiet sounds, for listening at a low volume: it turns on the dynamic range compressor with a ratio of 8:1, a threshold of −30 dB and a makeup gain of +9 dB. Turning it off puts the compressor back as it was (on or off, with its values), even in a later session. | `audio-filter` (`compressor`), `compressor-ratio`, `compressor-threshold`, `compressor-makeup-gain` |
| **Replay gain** | Dropdown: *Off* / *Track* / *Album*. Default: *Off*. | Uses the ReplayGain values stored in music files to play every track (*Track*) or every album (*Album*) at the same loudness. Files without ReplayGain values play as they are. | `audio-replay-gain-mode` (`none` / `track` / `album`) |
| **Replay gain preamp** | Number field, in dB, one decimal. Default: 0.0. | Moves the ReplayGain target level (89 dB) up or down, for the files that have ReplayGain values. | `audio-replay-gain-preamp` |

## Effects

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Equalizer** | Toggle. Default: off. | Turns on VLC's 10-band graphic equalizer. | `audio-filter` (`equalizer`) |
| **Equalizer preset** | Dropdown, each preset with its curve: *None*, then *Flat*, *Classical*, *Club*, *Dance*, *Full bass*, *Full bass and treble*, *Full treble*, *Headphones*, *Large Hall*, *Live*, *Party*, *Pop*, *Reggae*, *Rock*, *Ska*, *Soft*, *Soft rock*, *Techno*. Reads *None* while the equalizer is off (so a new profile shows *None*); the preset VLC keeps by default is *Flat*. | Choosing a preset turns the equalizer on, clears the bands set by hand and sets the preamp to the preset's own value; *None* turns the equalizer off. The [Effects](Effects#equalizer) panel has the same 18 presets without *None* (there, the equalizer is turned off with its **Enable** toggle). | `equalizer-preset` |
| **Open Equalizer** / **Close Equalizer** | A bar that opens 11 vertical sliders: *Preamp*, 60 Hz, 170 Hz, 310 Hz, 600 Hz, 1 kHz, 3 kHz, 6 kHz, 12 kHz, 14 kHz, 16 kHz, each −20 to +20 dB with its value shown under it. Default: preamp 12 dB; the bands follow the preset until you move one. | Sets the bands by hand. Drag a slider (0.1 dB steps). The mouse wheel moves the slider clicked last, or any slider with <kbd>Ctrl</kbd> held (otherwise it scrolls the page), 0.5 dB a notch. Keyboard: <kbd>Tab</kbd> to a slider, <kbd>↑</kbd>/<kbd>↓</kbd> (or <kbd>→</kbd>/<kbd>←</kbd>) 0.5 dB, <kbd>Page Up</kbd>/<kbd>Page Down</kbd> 5 dB, <kbd>Home</kbd>/<kbd>End</kbd> to +20 / −20 dB. | `equalizer-preamp`, `equalizer-bands` (empty = the preset's) |
| **Two-pass** | Toggle. Default: off. | Filters the audio twice, for a stronger effect. | `equalizer-2pass` |
| **Spatial audio** | Toggle. Default: off. | Turns on VLC's spatializer, which places the sound in a virtual room (a reverb). Its room settings are in the [Effects](Effects#spatializer) panel. | `audio-filter` (`spatializer`) |
| **Spatial audio mode** | Dropdown: *Automatic* / *Stereo* / *Headphone virtualization* / *4.0* / *5.1* / *7.1*. Default: *Automatic*. | How multichannel audio is mixed for the output: *Automatic* keeps the channels of the media, *Stereo* mixes them down to two, *Headphone virtualization* renders them binaurally so surround sound keeps its directions in headphones, *4.0*, *5.1* and *7.1* mix to that speaker layout. | `mix-mode` (Automatic = 0) |
| **Stereo widening** | Toggle. Default: off. | Widens the stereo image. Its settings are in the [Effects](Effects#stereo-widener) panel. | `audio-filter` (`stereo_widen`) |
| **Compressor** | Toggle. Default: off. | A dynamic range compressor that brings loud and quiet sounds closer, with the values of the [Effects](Effects#compressor) panel (or of *Night mode*). | `audio-filter` (`compressor`) |
| **Pitch shift** | Number field, in semitones, two decimals, −12 to +12. Default: 0.00. | Raises or lowers the pitch without changing the speed. Any value other than 0 also turns on the pitch filter. | `pitch-shift` (and `audio-filter`: `scaletempo_pitch`) |
| **Chorus/flanger** | Toggle. Default: off. | A chorus and flanger effect (a short, modulated delay mixed with the sound). | `audio-filter` (`chorus_flanger`) |
| **Karaoke** | Toggle. Default: off. | Removes what sits in the centre of a stereo mix, usually the voice. | `audio-filter` (`karaoke`) |

## Tracks

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Preferred audio language** | Dropdown: *Automatic*, *System language* (the language of Windows), then Arabic, Chinese, Czech, Danish, Dutch, English, Finnish, French, German, Greek, Hebrew, Hindi, Hungarian, Indonesian, Italian, Japanese, Korean, Malay, Norwegian, Persian, Polish, Portuguese, Romanian, Russian, Spanish, Swedish, Thai, Turkish, Ukrainian, Vietnamese (the language of Windows is not repeated). Default: *Automatic*. | The audio track VLC picks when a media has several languages. *Automatic* keeps the track the media marks as its default. A list of codes set elsewhere (`fre,eng`) is kept and shown as it is. | `audio-language` (ISO 639-2 codes; Automatic = empty) |
| **Audio delay** | Number field, in ms. Default: 0. | Delays the sound against the picture in every media; a negative value plays it earlier. For the media playing now only, use *Audio delay* in the player's languages and tracks panel ([The player](The-Player#languages-and-tracks)) or <kbd>K</kbd> / <kbd>J</kbd>. | `audio-desync` |

## Visualization

![Settings: Audio, the Visualizer list](images/settings-audio-visualizer.png)

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Visualizer** | Dropdown with icons: *Disabled*, *Spectrometer*, *Scope*, *Spectrum*, *Waveform*, *VU meter*, then the other visualizations of this build (for example *Goom effect*). Default: *Disabled*. | Draws an animation while audio without a picture plays. The OpenGL ones (*3D OpenGL spectrum visualization*, *libprojectM effect*) are only listed when the video window uses OpenGL, or while one of them is chosen; *Automatic* only while it is the one chosen. *Changed in 0.3.0: the effects of the visualizer are listed by name (before: one "Visualizer filter").* | `audio-visual`; `effect-list` for Spectrometer to VU meter (`audio-visual` = `visual`) |

## Advanced Options

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Audio output module** *(Developer mode)* | Dropdown of the audio output modules of this build. Default: *Automatic*. | The method VLC uses to output audio; *Automatic* picks the best one (MMDevice, that is WASAPI, on Windows). Hidden when the build offers no choice. | `aout` (Automatic = `any`) |
| **Output gain** *(Developer mode)* | Slider, 0 to 8, its value shown with two decimals. Default: 1.00. | A linear gain applied to all the audio output. | `gain` |
| **Keep the pitch when changing the speed** *(Developer mode)* | Toggle. Default: on. | Plays the audio faster or slower without changing its pitch when the playback speed changes. Off, faster speech sounds higher. | `audio-time-stretch` |
| **Pass compressed audio to the receiver** | Toggle. Default: off. | Sends compressed audio (AC-3, DTS…) as it is to an amplifier or receiver over S/PDIF or HDMI, when the audio output cannot negotiate pass-through by itself. | `spdif` |
| **Detect Dolby Surround** | Dropdown: *Automatic* / *On* / *Off*. Default: *Automatic*. | For a stream that is (or is not) Dolby Surround encoded but detected wrongly. Even on a stream that is not, *On* can improve the sound, especially with the headphone channel mixer. | `force-dolby-surround` (0 / 1 / 2) |
| **Resampling quality** | Dropdown: *Fastest* / *Fast* / *Balanced* / *High* / *Best*. Default: *Balanced*. | The quality of the SoX resampler, used when the sample rate of the media differs from the device's, traded against processor time. Needs the soxr module. | `soxr-resampler-quality` (0 to 4) |
| **Media role** | Dropdown: *Video* / *Music* / *Communication* / *Game* / *Notification* / *Animation* / *Production* / *Accessibility* / *Test*. Default: *Video*. | Tells the operating system what kind of audio VLC plays, for its audio policy. | `role` |
| **Visualization size** | Dropdown: *Match window* / *1280 × 720* / *1920 × 1080* (and a size set another way, shown as it is). Default: *Match window*. | The size the visualizations are drawn at; *Match window* draws them at 800 × 500 and lets the window scale them. The same row is repeated in the Advanced Options of [Subtitles](Settings-Subtitles). | `effect-width`, `effect-height` |

## Good to know

- **Equalizer, Compressor, Spatial audio, Stereo widening, Chorus/flanger, Karaoke and Normalize loudness** are all
  entries of one VLC list, the audio filters. Each toggle adds or removes its own filter and leaves the others.
- **Night mode and Compressor share one filter.** Night mode reads on while the compressor runs with ratio 8:1 and
  threshold −30 dB: turning *Compressor* off ends Night mode too, and changing the ratio or the threshold in the
  Effects panel makes Night mode read off.
- **Above 100 %** the volume is digital amplification. For a quiet film, *Normalize loudness* or *Night mode* raise
  the quiet parts without pushing the loud ones into distortion.
- **No sound?** Check *Enable audio*, then *Output device* (a device that was unplugged), then, in Developer mode,
  *Audio output module* (*Automatic* is right for almost everyone).
- **Dolby or DTS to a receiver:** turn on *Pass compressed audio to the receiver*. For a Chromecast, see *Send Dolby
  audio as is* in [Recording](Settings-Recording).

**Related pages:** [Effects](Effects) · [The player: Volume](The-Player#volume) · [Keyboard shortcuts](Keyboard-Shortcuts#audio) · [Casting](Casting)

---

← [Video](Settings-Video) · [All settings](Settings) · [Subtitles](Settings-Subtitles) →
