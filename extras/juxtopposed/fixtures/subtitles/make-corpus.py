#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
#
# Writes the subtitle encoding corpus: the same cues in every legacy encoding
# of their language, with and without a byte order mark, CRLF and LF, in the
# subtitle formats VLC reads. Each file has a <name>.expected (UTF-8, NFC)
# with the text VLC must show. Files named "<case>.<lang>.<ext>" carry the
# language in their name (a hint, as "movie.he.srt"); "<case>.<ext>" none.
#   python3 make-corpus.py [out dir]     (default: corpus/ next to it)
import os
import sys
import unicodedata

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, 'corpus')

# lang: cues (each cue: lines)
TEXT = {
    'he': [['שָׁלוֹם, מָה שְׁלוֹמְךָ הַיּוֹם?', 'בְּרֵאשִׁית בָּרָא אֱלֹהִים.'],
           ['זה המשפט השני: 3 תפוחים ו-12 אגסים!'],
           ['״ציטוט״ — וגם (סוגריים), נקודה.'],
           ['הסרט Matrix יצא ב-1999, והוא עדיין מצוין.']],
    'he-plain': [['שלום, מה שלומך היום?', 'בראשית ברא אלוהים.'],
                 ['זה המשפט השני: 3 תפוחים ו-12 אגסים!'],
                 ['הסרט Matrix יצא ב-1999, והוא עדיין מצוין.']],
    'ar': [['مَرْحَبًا بِكُمْ فِي الفِيلْمِ!', 'كَيْفَ حَالُكَ اليَوْمَ؟'],
           ['هذه الجملة الثانية: 3 كتب و12 قلماً.'],
           ['شكراً جزيلاً، إلى اللقاء.']],
    'ar-fr': [['Bonjour، كيف حالك؟', "C'est très bien, merci."],
              ['مرحباً à tous les élèves، أهلاً وسهلاً.']],
    'ru': [['Привет! Как дела?', 'Это третья строка: «ёлка» и 42 рубля.'],
           ['Съешь же ещё этих мягких французских булок, да выпей чаю.']],
    'ru-koi': [['Привет! Как дела?', 'Это третья строка: "ёлка" и 42 рубля.'],
               ['Съешь же ещё этих мягких французских булок, да выпей чаю.']],
    'uk': [['Привіт, як справи?', 'Їжак, ґанок і євро.'],
           ['Чуєш їх, доцю, га? Кумедна ж ти, прощайся без ґольфів!']],
    'pl': [['Zażółć gęślą jaźń.', 'Łódź jest piękna – 25 złotych.'],
           ['Pchnąć w tę łódź jeża lub ośm skrzyń fig.']],
    'pl-iso': [['Zażółć gęślą jaźń.', 'Łódź jest piękna - 25 złotych.'],
               ['Pchnąć w tę łódź jeża lub ośm skrzyń fig.']],
    'cs': [['Příliš žluťoučký kůň úpěl ďábelské ódy.', 'Těšíme se na večer!'],
           ['Řekni mi, kdy přijdeš: v 8 hodin?']],
    'fr': [["Ça va très bien, merci ! L'été à Paris coûte 15 € — œuvre."],
           ['Où êtes-vous allés ? Noël, Pâques et la fête.']],
    'fr-iso': [["Ça va très bien, merci ! L'été à Paris coûte 15 € - œuvre."],
               ['Où êtes-vous allés ? Noël, Pâques et la fête.']],
    'de': [['Größe, Übermaß und Äpfel für 3 Österreicher.'],
           ['Fünf Bücher über süße Grüße.']],
    'es': [['¿Qué tal? ¡Mañana iremos al cañón!'], ['El niño comió piña y jalapeño.']],
    'el': [['Καλημέρα! Πώς είστε σήμερα;', 'Ελληνικά γράμματα: αβγδ.'],
           ['Ξεσκεπάζω την ψυχοφθόρα βδελυγμία.']],
    'tr': [["Iğdır'da şöyle bir ılık rüzgâr esti; çok güzel, İstanbul!"],
           ['Pijamalı hasta yağız şoföre çabucak güvendi.']],
    'lt': [['Ačiū, labai gražu! Šiandien žiūrėsime filmą.'], ['Įėjimas į kiemą – už ąžuolo.']],
    'lv': [['Paldies, ļoti skaisti! Šodien skatīsimies filmu ar ģimeni.'],
           ['Glāžšķūņa rūķīši dzērumā čiepj Baha koncertflīģeļu vākus.']],
    'vi': [['Xin chào, bạn có khỏe không?', 'Tiếng Việt rất đẹp.'],
           ['Hôm nay trời đẹp quá, chúng ta đi xem phim nhé!']],
    'th': [['สวัสดีครับ วันนี้อากาศดีมาก', 'ภาษาไทยสวยงาม'],
           ['ผู้ใหญ่หาผ้าใหม่ให้สะใภ้ใช้คล้องคอ']],
    'ja': [['こんにちは、元気ですか？', '今日は映画を見ます。'],
           ['東京でiPhone 15を買いました。カタカナもOK!']],
    'zh': [['你好，今天我们一起去看电影吧。', '中文字幕测试。'],
           ['这部电影非常好看，我很喜欢。']],
    'zh-tw': [['你好，今天我們一起去看電影吧。', '繁體中文字幕測試。'],
              ['這部電影非常好看，我很喜歡。']],
    'ko': [['안녕하세요, 오늘 영화를 보러 갑시다.', '한국어 자막 시험입니다.'],
           ['이 영화는 정말 재미있어요.']],
}

# case name: (text, encoding, language of the file name or None)
CASES = [
    ('he-cp1255', 'he', 'cp1255', 'he'), ('he-iso8859-8', 'he-plain', 'iso8859_8', 'he'),
    ('ar-cp1256', 'ar', 'cp1256', 'ar'), ('ar-iso8859-6', 'ar', 'iso8859_6', 'ar'),
    ('arfr-cp1256', 'ar-fr', 'cp1256', 'ar'),
    ('ru-cp1251', 'ru', 'cp1251', 'ru'), ('ru-koi8r', 'ru-koi', 'koi8_r', 'ru'),
    ('uk-cp1251', 'uk', 'cp1251', 'uk'), ('uk-koi8u', 'uk', 'koi8_u', 'uk'),
    ('pl-cp1250', 'pl', 'cp1250', 'pl'), ('pl-iso8859-2', 'pl-iso', 'iso8859_2', 'pl'),
    ('cs-cp1250', 'cs', 'cp1250', 'cs'),
    ('fr-cp1252', 'fr', 'cp1252', 'fr'), ('fr-iso8859-15', 'fr-iso', 'iso8859_15', 'fr'),
    ('de-cp1252', 'de', 'cp1252', 'de'), ('es-cp1252', 'es', 'cp1252', 'es'),
    ('el-cp1253', 'el', 'cp1253', 'el'), ('el-iso8859-7', 'el', 'iso8859_7', 'el'),
    ('tr-cp1254', 'tr', 'cp1254', 'tr'),
    ('lt-cp1257', 'lt', 'cp1257', 'lt'), ('lv-cp1257', 'lv', 'cp1257', 'lv'),
    ('vi-cp1258', 'vi', 'cp1258', 'vi'),
    ('th-cp874', 'th', 'cp874', 'th'),
    ('ja-sjis', 'ja', 'cp932', 'ja'), ('ja-eucjp', 'ja', 'euc_jp', 'ja'),
    ('zh-gbk', 'zh', 'gb18030', 'zh'), ('zh-big5', 'zh-tw', 'cp950', 'zh-tw'),
    ('ko-cp949', 'ko', 'cp949', 'ko'),
]
UNICODE = [
    ('he-utf8', 'he', 'utf-8', 'he'), ('he-utf8bom', 'he', 'utf-8-sig', 'he'),
    ('ar-utf16le', 'ar', 'utf-16-le-bom', 'ar'), ('ja-utf16be', 'ja', 'utf-16-be-bom', 'ja'),
    ('ru-utf32le', 'ru', 'utf-32-le-bom', 'ru'), ('zh-utf16le-nobom', 'zh', 'utf-16-le', 'zh'),
    ('th-utf8', 'th', 'utf-8', 'th'), ('vi-utf8', 'vi', 'utf-8', 'vi'),
]


def encode_1258(text):
    """Windows-1258 holds the vowels with their hats and horns and five tone
    marks that follow them: the precomposed Vietnamese letters are split."""
    out = b''
    for ch in unicodedata.normalize('NFC', text):
        try:
            out += ch.encode('cp1258')
            continue
        except UnicodeEncodeError:
            pass
        d = unicodedata.normalize('NFD', ch)
        tones = [m for m in d[1:] if m in '̣̀́̃̉']
        rest = d[0] + ''.join(m for m in d[1:] if m not in tones)
        out += unicodedata.normalize('NFC', rest).encode('cp1258')
        out += ''.join(tones).encode('cp1258')
    return out


def encode(text, enc):
    if enc == 'cp1258':
        return encode_1258(text)
    if enc.endswith('-bom'):
        base = enc[:-4]
        bom = {'utf-16-le': b'\xff\xfe', 'utf-16-be': b'\xfe\xff', 'utf-32-le': b'\xff\xfe\x00\x00'}[base]
        return bom + text.encode(base)
    return text.encode(enc)


def ms(t):
    return '%02d:%02d:%02d,%03d' % (t // 3600000, t // 60000 % 60, t // 1000 % 60, t % 1000)


def render(fmt, cues, nl):
    lines = []
    if fmt == 'srt':
        for i, cue in enumerate(cues):
            a, b = 1000 + i * 3000, 3500 + i * 3000
            lines += [str(i + 1), '%s --> %s' % (ms(a), ms(b))] + cue + ['']
    elif fmt == 'vtt':
        lines = ['WEBVTT', '']
        for i, cue in enumerate(cues):
            a, b = 1000 + i * 3000, 3500 + i * 3000
            lines += ['%s --> %s' % (ms(a).replace(',', '.'), ms(b).replace(',', '.'))] + cue + ['']
    elif fmt == 'sub':   # MicroDVD
        lines = ['{%d}{%d}%s' % (25 + i * 75, 85 + i * 75, '|'.join(cue)) for i, cue in enumerate(cues)]
    elif fmt == 'ass':
        lines = ['[Script Info]', 'ScriptType: v4.00+', 'PlayResX: 1280', 'PlayResY: 720', '',
                 '[V4+ Styles]',
                 'Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, '
                 'Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, '
                 'Shadow, Alignment, MarginL, MarginR, MarginV, Encoding',
                 'Style: Default,Arial,48,&H00FFFFFF,&H000000FF,&H00000000,&H80000000,0,0,0,0,100,100,0,0,1,2,1,2,40,40,40,1',
                 '', '[Events]',
                 'Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text']
        for i, cue in enumerate(cues):
            a, b = 1 + i * 3, 3 + i * 3
            lines.append('Dialogue: 0,0:00:%02d.00,0:00:%02d.50,Default,,0,0,0,,%s' % (a, b, '\\N'.join(cue)))
    elif fmt == 'smi':
        lines = ['<SAMI>', '<BODY>']
        for i, cue in enumerate(cues):
            lines.append('<SYNC Start=%d><P Class=XXCC>%s' % (1000 + i * 3000, '<br>'.join(cue)))
            lines.append('<SYNC Start=%d><P Class=XXCC>&nbsp;' % (3500 + i * 3000))
        lines += ['</BODY>', '</SAMI>']
    return nl.join(lines) + nl


def write(name, text_key, enc, lang, fmt, nl, first=False):
    cues = [[unicodedata.normalize('NFC', l) for l in cue] for cue in TEXT[text_key]]
    if first:
        cues = [cues[0][:1]]
    content = render(fmt, cues, nl)
    fname = '%s.%s.%s' % (name, lang, fmt) if lang else '%s.%s' % (name, fmt)
    with open(os.path.join(OUT, fname), 'wb') as f:
        f.write(encode(content, enc))
    with open(os.path.join(OUT, fname + '.expected'), 'wb') as f:
        f.write(content.encode('utf-8'))
    return fname


def main():
    os.makedirs(OUT, exist_ok=True)
    for f in os.listdir(OUT):
        os.remove(os.path.join(OUT, f))
    n = 0
    for name, key, enc, lang in CASES:
        # the language in the name and none; LF and CRLF; the formats
        write(name, key, enc, lang, 'srt', '\r\n'); n += 1
        write(name + '-nohint', key, enc, None, 'srt', '\n'); n += 1
        write(name, key, enc, lang, 'sub', '\n'); n += 1
        write(name, key, enc, lang, 'ass', '\r\n'); n += 1
        write(name, key, enc, lang, 'smi', '\r\n'); n += 1
        write(name, key, enc, lang, 'vtt', '\n'); n += 1
        # one cue of one line, nothing in its name: a cue of a track inside
        # a video (MKV, MP4) is detected alone
        write(name + '-short', key, enc, None, 'srt', '\n', first=True); n += 1
    for name, key, enc, lang in UNICODE:
        write(name, key, enc, lang, 'srt', '\r\n'); n += 1
        write(name + '-nohint', key, enc, None, 'srt', '\n'); n += 1
    print('%d files in %s' % (n, OUT))


if __name__ == '__main__':
    main()
