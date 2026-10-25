# Languages

VLC Redesign speaks **72 languages** besides English, each covering every text of the interface, and changes from
one to another at once, without a restart. Right-to-left languages are laid out right to left.

## Choose the language

**⋯ → Settings → General → Appearance → Interface language** (<kbd>Ctrl</kbd>+<kbd>P</kbd>).

- The whole interface changes at once: the sidebar, the pages, the player, the settings, the menus and the dialogs
  (VLC's classic windows the next time they open). There is no restart.
- As for the theme, the new language is kept when Settings closes. Choosing the language in VLC's classic
  preferences changes it at once too.
- Each language is listed by its own name (*Deutsch*, *Українська*, *العربية*).
- VLC Redesign starts in **English** until you choose a language, whatever the language of Windows; *Reset all
  settings* comes back to English. The installer can set the language for you (see
  [Install and update](Install-and-Update#languages)).

The language of the subtitles is set apart: see [Subtitles](Subtitles) and [Settings: Subtitles](Settings-Subtitles).

## The complete languages

Afrikaans, Albanian, Arabic, Armenian, Basque, Belarusian, Bengali, Bosnian, Bulgarian, Catalan, Central Kurdish,
Chinese (Simplified), Chinese (Traditional), Croatian, Czech, Danish, Dutch, Estonian, Finnish, French, Galician,
Georgian, German, Greek, Gujarati, Hebrew, Hindi, Hungarian, Icelandic, Indonesian, Irish, Italian, Japanese,
Kannada, Kazakh, Khmer, Korean, Latvian, Lithuanian, Macedonian, Malay, Malayalam, Marathi, Mongolian, Nepali,
Norwegian Bokmål, Norwegian Nynorsk, Occitan, Persian, Polish, Portuguese (Brazil), Portuguese (Portugal), Punjabi,
Romanian, Russian, Scottish Gaelic, Serbian, Sinhala, Slovak, Slovenian, Spanish (Spain), Spanish (Latin America),
Swedish, Tamil, Telugu, Thai, Turkish, Ukrainian, Urdu, Vietnamese, Walloon and Welsh.

English comes in two spellings: *English* and *English (UK)*.

VLC's other catalogs (Asturian, Kabyle, Uzbek...) are listed after them, marked **(partial)**: VLC's own texts are
translated there, the redesign's stay in English.

Beyond the words, each language gets:

- **Plural forms** of its own (*2 элемента*, *5 элементов*), and minutes and milliseconds written its way (*5 мин*,
  *5 dk*, *5 分钟*).
- **Keyboard keys** named as on its keyboards (*Entf*, *Suppr*, *Canc*).
- **Fonts** of its script: Chinese, Japanese and Korean take the fonts of their language whatever the language of
  Windows; Hindi and the other Indian scripts, Sinhala, Thai, Lao, Khmer, Myanmar and Ethiopic take the fonts of
  Windows' own interface.
- **Room for longer words**: a text too long for its place is shortened with "…" and shown whole in a tooltip, wraps
  where there is room, or its button, switch, tab or menu widens.

## Right to left

Choosing **Hebrew**, **Arabic**, **Persian**, **Urdu** or **Central Kurdish** lays the interface out right to left at
once: everything is mirrored, and the arrows point the way one reads. Each language file says which way it is
written, so a new translation can be right to left too.

What does not mirror, on purpose:

- **The playback bar** keeps its order, as on most players: time runs from left to right on the seek bar, with its
  times on its sides; Previous, Play and Next, and the volume, are in the same order as in English. Its menus and
  tooltips are mirrored.
- The **volume** and the **speed** grow from left to right, and the subtitle timing runs earlier on the left, later
  on the right, as the seek bar.

### Layout direction

Under the language, **Layout direction** chooses **Left to right** or **Right to left**. It follows the language you
choose (a right-to-left language switches it to *Right to left*), and you can change it: a right-to-left language
can stay left to right, as some prefer. Like the language, it changes at once.

The installer asks the same (*Right-to-left layout*, on its Languages page), and a silent install takes
`/RIGHTTOLEFT` or `/LEFTTORIGHT`. The portable zip starts left to right.

## Translate

Anyone can translate VLC Redesign, improve a translation or add a language. The texts are ordinary gettext catalogs
(`.po`), the format VLC uses: any `.po` editor works ([Poedit](https://poedit.net/) is free), as does a plain text
editor.

1. **Get the catalog**: `po/<code>.po` of the repository for an existing language, or a new one from the template
   `po/vlc.pot` (`msginit -i po/vlc.pot -o po/<code>.po -l <code>`, or *New from POT* in Poedit).
2. **Translate** the empty entries and those marked `fuzzy`. Keep the placeholders (`%1`, `%s`...), the HTML tags,
   line breaks and the `&` of menu accelerators. Set the direction entry (`msgctxt "layout direction"`) to `RTL`
   for a right-to-left language.
3. **Try it**: `msgfmt -c --statistics -o vlc.mo po/<code>.po`, copy `vlc.mo` to
   `locale\<code>\LC_MESSAGES\vlc.mo` in the folder of VLC Redesign (the portable zip is the easiest), and choose the
   language in Settings: the interface changes at once. A new language shows in the list by its own name, marked
   "(partial)", until it is added to the complete ones.
4. **Send it**: a pull request on [GitHub](https://github.com/KuchiSofts/vlc-redesign), or the file attached to an
   issue (the *Translation* form). Say whether it covers the whole interface.

The full guide, with the rules for keys, contexts and plurals, is
[TRANSLATING.md](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/TRANSLATING.md). A wrong or awkward text
in your language can be reported with the
[Translation form](https://github.com/KuchiSofts/vlc-redesign/issues/new/choose).

---

[Install and update](Install-and-Update) · [Subtitles](Subtitles) · [Settings: General](Settings-General) · [Home](Home)
