# Translating VLC Redesign

Anyone can translate VLC Redesign, improve a translation, or add a new language. The texts are ordinary
gettext catalogs, the same format VLC uses, so any `.po` editor works ([Poedit](https://poedit.net/) is
free) as well as a plain text editor.

## On Weblate (the easiest)

The texts VLC Redesign adds are translated on **[Hosted Weblate](https://hosted.weblate.org/engage/vlc-redesign/)**, in the browser: sign in (a GitHub account
works), choose your language and translate. Weblate shows each text with its context, checks the placeholders, and
sends the translations back to this repository as pull requests; they are in the next release.

[![Translation status](https://hosted.weblate.org/widget/vlc-redesign/interface/multi-auto.svg)](https://hosted.weblate.org/engage/vlc-redesign/)

- Only the texts of VLC Redesign are there (`po/redesign`, about 1,700). VLC's own texts are translated by VideoLAN
  and arrive with each update of VLC.
- A language that is not listed yet can be asked for on Weblate (*Start new translation*); the maintainers add it.
- Maintainers: `extras/juxtopposed/translations/weblate-sync.py` cuts `po/redesign` out of the catalogs
  (`export`, after new texts) and puts Weblate's translations back into `po/<code>.po` (`import`, before a build).

The rest of this page is for translating the files directly.

## Where the texts are

| File | What it is |
|---|---|
| `po/vlc.pot` | Every text of the program, in English: the template of a new language |
| `po/<code>.po` | One language (`he.po`, `pt_BR.po`, `zh_TW.po`...), shared with VLC's own texts |
| `po/LINGUAS` | The languages built into the release |
| `extras/juxtopposed/installer/` | The installer's texts (`vlc-redesign.iss`, `[CustomMessages]`) and license notices |

The redesigned interface is the entries whose `#:` references point to `modules/gui/qt/...`; the rest are VLC's
own texts (settings help, shortcut names, messages drawn on the video), many of which the interface shows too.

## The direction of your language

Every catalog has one entry that tells VLC Redesign which way your language is written:

```po
#. The direction your language is written in: "RTL" if from right to left
#. (Arabic, Hebrew, Persian, Urdu...), else "LTR". ...
msgctxt "layout direction"
msgid "LTR"
msgstr "RTL"
```

Write `RTL` for a right-to-left language and leave `LTR` (or the entry empty) otherwise. When someone chooses the
language in *Settings → Appearance → Interface language*, the whole interface is laid out in that direction at once.
They can still change it under the language, in *Layout direction*. An empty entry falls back to the direction Qt
knows for the language.

## Translating

1. **An existing language:** open `po/<code>.po`. **A new one:** start from the template:
   `msginit -i po/vlc.pot -o po/<code>.po -l <code>` (or *New from POT* in Poedit), and check that the
   `Plural-Forms` line of the header is right for your language.
2. Translate the empty entries, and the ones marked `fuzzy` (an old guess, never shown: translate them again and
   remove the flag).
3. Keep exactly:
   - placeholders: `%1 %2` (these may move), `%s %d %i %u %ld %.1f %%`;
   - HTML tags, `\n` line breaks, and spaces at the start or end;
   - a final `:` `…` or `...`;
   - `&` before a letter (the keyboard accelerator of a menu, on a letter of your translation).
4. Leave in English: VLC, VLC Redesign, VideoLAN, Chromecast, DLNA, OpenSubtitles, Windows, codec and format names
   (H.264, AAC, MKV), and units such as ms, kHz and dB, unless your language writes them differently.
5. Two contexts mean two meanings of one English word:
   - `msgctxt "key"` entries are keyboard keys (Home, Delete, Page Up), as printed on the keyboards of your language.
   - `msgctxt "media streams"` *Tracks* are the audio and subtitle tracks of a video. The plain *Tracks* is the
     songs of the music library.
6. Keep buttons, menu items and labels about as short as the English; explanations are full sentences.

## Trying it

1. Check the catalog and compile it: `msgfmt -c --statistics -o vlc.mo po/<code>.po`. Fix every error it prints.
2. Copy `vlc.mo` to `locale\<code>\LC_MESSAGES\vlc.mo` in the folder of VLC Redesign (the portable zip is the
   easiest), creating the folders for a new language.
3. Start VLC Redesign and choose your language in *Settings → Appearance → Interface language*: the interface
   changes at once, without a restart.
   - A new language appears in the list by its own name, marked "(partial)", until it is added to the complete
     languages.
   - Look through the pages for texts that are cut or too long.

## Sending it

Open a pull request with your `.po` file on [GitHub](https://github.com/KuchiSofts/vlc-redesign), or attach it to
an issue. Say whether it covers the whole interface: a complete language is listed first, without "(partial)",
and can be offered by the installer. For the installer, translate the 73 `english.` messages of
`vlc-redesign.iss` and `notice.txt` (the other languages there are examples).
