# Languages

VLC Redesign speaks **72 languages** besides English, each covering every text of the interface, and changes from
one to another at once, without a restart. Right-to-left languages are laid out right to left. This page explains
where to choose the language, what each language brings, how right to left works, and how to translate.

| | |
|---|---|
| **Complete languages** | 72, plus English and British English |
| **Other catalogs** | 37 of VLC's own catalogs, listed as *(partial)* when they have enough of VLC's texts |
| **Right to left** | Hebrew, Arabic, Persian, Urdu, Central Kurdish |
| **Default** | English, whatever the language of Windows, until you choose one |
| **Where** | **Settings → General → Appearance → Interface language**, or the installer's *Languages* page |

![Settings, Appearance: interface language, layout direction, theme and accent](images/settings.jpg)

## Choose the language

**⋯ → Settings → General → Appearance → Interface language** (<kbd>Ctrl</kbd>+<kbd>P</kbd>).

- The whole interface changes at once: the sidebar, the pages, the player, the settings, the menus and the dialogs
  (VLC's classic windows the next time they open). There is no restart.
- As for the theme, the new language is kept when Settings closes; the Settings window has no Apply button.
  Choosing the language in VLC's classic preferences changes it at once too.
- The list starts with **Auto** (the language of Windows), then the complete languages, each by its own name
  (*Deutsch*, *Українська*, *العربية*), then the partial ones, marked *(partial)*.
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

English comes in two spellings: *English* (American, VLC's own texts) and *British English* (shown as *English
(United Kingdom)* in the installer).

VLC's 37 other catalogs (Asturian, Kabyle, Uzbek, Interlingua...) are listed after them, marked **(partial)**:
VLC's own texts are translated there, the redesign's stay in English. A catalog with next to nothing translated
(under 16 KB) is not listed. The installer offers only the complete languages, so that the app never starts half
in English.

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

The installer asks the same (*Right-to-left layout*, on its Languages page, shown only for those five languages),
and a silent install takes `/RIGHTTOLEFT` or `/LEFTTORIGHT`. The portable zip starts in English, left to right.

## In the installer

The installer's **Languages** page (new in 0.3.1) sets two languages before the first start:

- **Interface language**: any of the complete languages, Setup's own language to begin with.
- **Subtitle language**: the default language of the subtitles, the interface's until you choose another.

A silent install takes `/UILANG=` and `/SUBLANG=` with the codes of the app (`he`, `de`, `pt_BR`, `zh_TW`...), for
example `/VERYSILENT /UILANG=he /SUBLANG=en`. A copy that already has a language keeps it. Details:
[Install and update](Install-and-Update#languages).

> **Good to know.** The interface language is kept in a file named `language` in the copy's `portable` folder, not
> in the registry, so an installed official VLC keeps its own language.

## Translate

Anyone can translate VLC Redesign, improve a translation or add a language.

**The easiest way: [Weblate](https://hosted.weblate.org/engage/vlc-redesign/).** Translate in the browser, with a GitHub account or a Weblate one; Weblate shows
the context of each text, checks the placeholders and sends the work back as a pull request. It holds the texts VLC
Redesign adds; VLC's own texts are translated by VideoLAN. A language that is missing can be asked for there.

[![Translation status](https://hosted.weblate.org/widget/vlc-redesign/interface/svg-badge.svg)](https://hosted.weblate.org/engage/vlc-redesign/)

**Or with the files.** The texts are ordinary gettext catalogs
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
