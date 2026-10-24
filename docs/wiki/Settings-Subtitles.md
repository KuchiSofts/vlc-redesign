# Settings: Subtitles

**⋯ → Settings → Subtitles** (or <kbd>Ctrl</kbd>+<kbd>P</kbd>). Subtitle languages and encoding, automatic loading, how subtitles look, timing and on-screen messages.

![Settings: Subtitles](images/settings-subtitles.png)

Rows under **Advanced Options** are in the collapsed bar at the bottom of the page. Rows marked *Developer mode only* appear there once **Developer mode** is on ([General](Settings-General)). See [how the Settings window works](Settings).

### Tracks & Languages
- **Enable subtitles**: toggle. Default: on. Turns all subtitle and sub-picture processing off when disabled. <sub>VLC option: `spu`</sub>
- **Preferred language**: the language list of the design (a dropdown with a *Search a language* field). Default: the language of Windows (English when it has none of the list); the welcome setup asks for it too. The language of the subtitles: the subtitle track VLC picks by default, and the language the online search looks for. *Changed in 0.3.0: this row of the design picks it; before, a "Default language" row at the top of the page did.* With the second language (at the end of the page), it sets VLC's option. <sub>VLC option: `sub-language`</sub>
- **Text encoding**: dropdown: Automatic, System codeset, Universal (UTF-8), Universal (UTF-16), the big- and little-endian UTF-16 variants, Universal, Chinese (GB18030), then many regional encodings (ISO-8859-x, Windows-125x, etc.). Default: Automatic. Character encoding used for text subtitle files. <sub>VLC option: `subsdec-encoding`</sub>

### Loading Subtitles
- **Load subtitle files automatically**: toggle. Default: on. Finds a subtitle file automatically based on the movie's file name. <sub>VLC option: `sub-autodetect-file`</sub>
- **File name matching**: dropdown: Off / Any subtitle file / Files containing the title / Title with extra characters / Exact title. Default: Title with extra characters. How closely a subtitle file name must match the movie name to be loaded. <sub>VLC option: `sub-autodetect-fuzzy`</sub>

### Appearance
- **Font**: text field (font family name). Default: the system sans-serif font (Arial on Windows), shown as "System, sans-serif". Font family used for subtitles. <sub>VLC option: `freetype-font`</sub>
- **Size values**: segmented control: Relative / Pixels. Default: Relative (Pixels if a pixel size is already set). Choose the subtitle size by name (relative to the picture) or in pixels. Switching resets the other kind of size to automatic. <sub>Interface setting: `redesign/subtitle-size-mode`</sub>
- **Font size** (when Size values = Relative): dropdown: Smaller / Small / Automatic / Large / Larger / Largest (50 / 75 / 100 / 125 / 150 / 200 %). Default: Automatic (100 %). Changes the subtitle size where possible. <sub>VLC option: `sub-text-scale`</sub>
- **Font size** (when Size values = Pixels): number field in px. 0 shows as "Automatic". Default: Automatic (0). Text size in pixels of the picture. 0 sizes it from the picture. <sub>VLC option: `freetype-fontsize`</sub>
- **Bold**: toggle. Default: off. Forces bold subtitle text. <sub>VLC option: `freetype-bold`</sub>
- **Text color**: colour field (hex value plus a swatch that opens a colour picker). Default: `#FFFFFF` (white). Colour of the subtitle text. <sub>VLC option: `freetype-color`</sub>
- **Text opacity**: slider from 0 % to 100 %. Default: 100 %. Opacity of the subtitle text (0 = transparent). <sub>VLC option: `freetype-opacity` (stored as 0–255)</sub>
- **Outline thickness**: dropdown: None / Thin / Normal / Thick. Default: Normal. Width of the outline around the letters. <sub>VLC option: `freetype-outline-thickness`</sub>
- **Outline color**: colour field. Default: `#000000` (black). Colour of the text outline. <sub>VLC option: `freetype-outline-color`</sub>
- **Shadow**: toggle. Default: on. On sets `freetype-shadow-opacity` to its default (128), off sets it to 0. Draws a shadow behind the text.
- **Shadow color**: colour field. Default: `#000000`. <sub>VLC option: `freetype-shadow-color`</sub>
- **Shadow distance**: dropdown: Small / Medium / Large. Default: Medium (0.06). How far the shadow is offset. <sub>VLC option: `freetype-shadow-distance`</sub>
- **Background**: dropdown: None / Box. Default: None. Stored through `freetype-background-opacity` (None = 0). Draws a box behind the subtitles.
- **Background color**: colour field. Default: `#000000`. Colour of the subtitle box. <sub>VLC option: `freetype-background-color`</sub>
- **Background opacity**: slider from 0 % to 100 %. Default: 50 %. How opaque the box behind the subtitles is when there is one. The value is kept while there is none. <sub>Interface setting: `redesign/subtitle-background-opacity` (copied to `freetype-background-opacity` while Background is Box)</sub>
- **Vertical position**: slider from *Lower* (−20 %) through *Usual* (0) to *Higher* (60 %), in steps of 10 %. Default: Usual. How high the subtitles are, in percent of the height of the picture above their usual place (below it when negative), the same on any screen and in any window size. The subtitles shown move at once. <sub>VLC option: `sub-position`</sub>

### Timing
- **Subtitle delay**: slider from −5 s to +5 s in 0.1 s steps. Default: 0 s. Delays or advances all subtitles. <sub>VLC option: `sub-delay` (stored in tenths of a second)</sub>

### On-Screen Messages
- **Show feedback for shortcuts**: toggle. Default: on. Shows on-video messages (OSD), such as volume or seek feedback. <sub>VLC option: `osd`</sub>
- **Show feedback for**: slider from 0.5 s to 5 s in 0.5 s steps. Default: 1 s. How long on-screen messages stay. <sub>VLC option: `osd-duration`</sub>
- **Show filename when a video starts**: toggle. Default: on. Displays the media title on top of the video when it starts. <sub>VLC option: `video-title-show`</sub>
- **Show filename for**: slider from 1 s to 10 s. Default: 5 s. How long the title stays on the video. <sub>VLC option: `video-title-timeout`</sub>

### Second Language & Online Subtitles
*(Not in the design: after its sections. Changed in 0.3.0: this block came first on the page before.)*

![Settings: Subtitles, Second Language & Online Subtitles](images/settings-subtitles-online.png)

- **Second language (when there are none in the preferred one)**: language list, with None. Default: English, or None when the preferred language is English. The subtitles VLC takes when a video has none in the preferred language.
- **Find subtitles online when a video has none in these languages**: toggle. Default: on. Asks the subtitle services below when a video opens without subtitles in the preferred or the second language.
- **Show both languages at once (dual subtitles)**: toggle. Default: off. Shows the subtitles of both languages together. *(Dimmed until a second language is chosen.)*
- **Subtitle services**: a toggle per service, all on: **OpenSubtitles** (all languages, movies and series, finds those made for your file), **Wizdom** (Hebrew, movies and series), **Addic7ed** (series, many languages), **YIFY Subtitles** (movies, many languages). The note under them: "To find subtitles, the services turned on get the video's title, its IMDb id, its file name and hash (made from a part of the file), and nothing else."

### Advanced Options
- **Load subtitles from these folders**: text field. Default: `.\subtitles, .\subs` on Windows (`./Subtitles, ./subtitles, ./Subs, ./subs` elsewhere). Extra folders to search for subtitle files when none is found next to the video. <sub>VLC option: `sub-autodetect-path`</sub>
- **Outline opacity**: slider from 0 % to 100 %. Default: 100 %. *(Developer mode only.)* <sub>VLC option: `freetype-outline-opacity`</sub>
- **Shadow opacity**: slider from 0 % to 100 %. Default: 50 %. *(Developer mode only.)* <sub>VLC option: `freetype-shadow-opacity`</sub>
- **Shadow angle**: number field in degrees (−360 to 360). Default: −45. Direction of the shadow. <sub>VLC option: `freetype-shadow-angle`</sub>
- **Text direction**: dropdown: Left to right / Right to left / Automatic. Default: Automatic. Paragraph base direction for the Unicode bidirectional algorithm, used for right-to-left scripts. *(Needs FriBidi support.)* <sub>VLC option: `freetype-text-direction`</sub>
- **Monospace font**: text field. Default: the system monospace font (Courier New on Windows), shown as "System monospace". Font family used for monospaced subtitle text. <sub>VLC option: `freetype-monofont`</sub>
- **Visualization size**: the same setting as Audio > Advanced Options, repeated here.
- **Text alignment**: dropdown: Auto / Center / Left / Right. Default: Auto. Justification of text subtitles. *(Developer mode only.)* <sub>VLC option: `subsdec-align`</sub>
- **Position of the filename**: dropdown: Center, Left, Right, Top, Bottom, Top-Left, Top-Right, Bottom-Left, Bottom-Right. Default: Bottom. Where the title is shown on the video. *(Developer mode only.)* <sub>VLC option: `video-title-position`</sub>

---

← [Audio](Settings-Audio) · [All settings](Settings) · [Library](Settings-Library) →
