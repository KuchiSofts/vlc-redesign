/*****************************************************************************
 * subs_charset.c: character encoding of text subtitles
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <errno.h>
#include <stdbool.h>
#ifdef SUBS_CHARSET_DEBUG
# include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef SUBS_CHARSET_STANDALONE
/* the test program (extras/juxtopposed/subtitle-charset.c) */
# include <iconv.h>
typedef iconv_t vlc_iconv_t;
# define vlc_iconv_open iconv_open
# define vlc_iconv_close iconv_close
static size_t vlc_iconv(vlc_iconv_t cd, const char **in, size_t *inleft,
                        char **out, size_t *outleft)
{
    return iconv(cd, (char **)in, inleft, out, outleft);
}
#else
# include <vlc_common.h>
# include <vlc_charset.h>
# include <vlc_stream.h>
# include <vlc_variables.h>
#endif

#include "subs_charset.h"

/* Enough text to tell the encodings apart, quickly */
#define ANALYSED_MAX (128 * 1024)

/*****************************************************************************
 * Conversion
 *****************************************************************************/

/* The Hebrew presentation forms U+FB1D..U+FB4E as letter + mark(s): the
 * Windows-1255 converters of GNU libiconv and glibc compose a letter and its
 * points into them, which fonts and shapers handle worse than the letters */
static const uint16_t hebrew_forms[][3] = {
    { 0x05D9, 0x05B4, 0 }, { 0, 0, 0 },           { 0x05F2, 0x05B7, 0 }, /* FB1D-FB1F */
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },     /* FB20-FB24 */
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },     /* FB25-FB29 */
    { 0x05E9, 0x05C1, 0 }, { 0x05E9, 0x05C2, 0 },                        /* FB2A-FB2B */
    { 0x05E9, 0x05BC, 0x05C1 }, { 0x05E9, 0x05BC, 0x05C2 },              /* FB2C-FB2D */
    { 0x05D0, 0x05B7, 0 }, { 0x05D0, 0x05B8, 0 }, { 0x05D0, 0x05BC, 0 }, /* FB2E-FB30 */
    { 0x05D1, 0x05BC, 0 }, { 0x05D2, 0x05BC, 0 }, { 0x05D3, 0x05BC, 0 }, /* FB31-FB33 */
    { 0x05D4, 0x05BC, 0 }, { 0x05D5, 0x05BC, 0 }, { 0x05D6, 0x05BC, 0 }, /* FB34-FB36 */
    { 0, 0, 0 },           { 0x05D8, 0x05BC, 0 }, { 0x05D9, 0x05BC, 0 }, /* FB37-FB39 */
    { 0x05DA, 0x05BC, 0 }, { 0x05DB, 0x05BC, 0 }, { 0x05DC, 0x05BC, 0 }, /* FB3A-FB3C */
    { 0, 0, 0 },           { 0x05DE, 0x05BC, 0 }, { 0, 0, 0 },           /* FB3D-FB3F */
    { 0x05E0, 0x05BC, 0 }, { 0x05E1, 0x05BC, 0 }, { 0, 0, 0 },           /* FB40-FB42 */
    { 0x05E3, 0x05BC, 0 }, { 0x05E4, 0x05BC, 0 }, { 0, 0, 0 },           /* FB43-FB45 */
    { 0x05E6, 0x05BC, 0 }, { 0x05E7, 0x05BC, 0 }, { 0x05E8, 0x05BC, 0 }, /* FB46-FB48 */
    { 0x05E9, 0x05BC, 0 }, { 0x05EA, 0x05BC, 0 }, { 0x05D5, 0x05B9, 0 }, /* FB49-FB4B */
    { 0x05D1, 0x05BF, 0 }, { 0x05DB, 0x05BF, 0 }, { 0x05E4, 0x05BF, 0 }, /* FB4C-FB4E */
};

static size_t utf8_put(char *p, uint32_t c)
{
    if (c < 0x80) { p[0] = c; return 1; }
    if (c < 0x800) { p[0] = 0xC0 | (c >> 6); p[1] = 0x80 | (c & 0x3F); return 2; }
    if (c < 0x10000) {
        p[0] = 0xE0 | (c >> 12); p[1] = 0x80 | ((c >> 6) & 0x3F);
        p[2] = 0x80 | (c & 0x3F); return 3;
    }
    p[0] = 0xF0 | (c >> 18); p[1] = 0x80 | ((c >> 12) & 0x3F);
    p[2] = 0x80 | ((c >> 6) & 0x3F); p[3] = 0x80 | (c & 0x3F); return 4;
}

/* Decodes one UTF-8 sequence of a valid UTF-8 string */
static uint32_t utf8_get(const unsigned char **pp)
{
    const unsigned char *p = *pp;
    uint32_t c = *p++;
    if (c >= 0xF0)      { c = ((c & 0x07) << 18) | ((p[0] & 0x3F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3; }
    else if (c >= 0xE0) { c = ((c & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F); p += 2; }
    else if (c >= 0xC0) { c = ((c & 0x1F) << 6) | (p[0] & 0x3F); p += 1; }
    *pp = p;
    return c;
}

/* Converts to UTF-8, U+FFFD for each byte that cannot be converted;
 * *errors counts them */
static char *convert(const void *buf, size_t size, const char *charset,
                     size_t *out_size, size_t *errors)
{
    vlc_iconv_t cd = vlc_iconv_open("UTF-8", charset);
    if (cd == (vlc_iconv_t)(-1))
        return NULL;

    size_t cap = size * 4 + 16; /* UTF-8 never needs more for these */
    char *out = malloc(cap + 1);
    if (out == NULL)
    {
        vlc_iconv_close(cd);
        return NULL;
    }
    const char *in = buf;
    size_t inleft = size;
    char *dst = out;
    size_t dstleft = cap;
    *errors = 0;
    while (inleft > 0)
    {
        if (vlc_iconv(cd, &in, &inleft, &dst, &dstleft) != (size_t)(-1))
            continue;
        if (errno != EILSEQ && errno != EINVAL)
            break;
        /* reset the state of a stateful converter, writing the character
         * it holds back (see below) */
        if (vlc_iconv(cd, NULL, NULL, &dst, &dstleft) == (size_t)(-1) ||
            dstleft < 3)
            break;
        /* an invalid or truncated sequence: one byte of it is lost */
        in++;
        inleft--;
        memcpy(dst, "\xEF\xBF\xBD", 3);
        dst += 3;
        dstleft -= 3;
        (*errors)++;
    }
    /* the last character: the converters of Windows-1255 and 1258 hold it
     * back for the combining marks that may follow */
    if (vlc_iconv(cd, NULL, NULL, &dst, &dstleft) == (size_t)(-1))
        (*errors)++;
    vlc_iconv_close(cd);

    /* decompose the Hebrew presentation forms (in place: never longer) */
    const unsigned char *r = (const unsigned char *)out;
    const unsigned char *end = (const unsigned char *)dst;
    char *w = out;
    bool hebrew = false;
    for (const unsigned char *p = r; p + 2 < end; p++)
        if (p[0] == 0xEF && p[1] == 0xAC && p[2] >= 0x9D) /* U+FB1D.. */
        {
            hebrew = true;
            break;
        }
    if (hebrew)
    {
        /* a form (3 bytes) becomes up to 3 code points of 2 bytes: build
         * a new string */
        char *dec = malloc((end - r) * 2 + 1);
        if (dec != NULL)
        {
            w = dec;
            while (r < end)
            {
                const unsigned char *start = r;
                uint32_t c = utf8_get(&r);
                if (c >= 0xFB1D && c <= 0xFB4E && hebrew_forms[c - 0xFB1D][0])
                {
                    for (int i = 0; i < 3 && hebrew_forms[c - 0xFB1D][i]; i++)
                        w += utf8_put(w, hebrew_forms[c - 0xFB1D][i]);
                }
                else
                {
                    memcpy(w, start, r - start);
                    w += r - start;
                }
            }
            free(out);
            out = dec;
            dst = w;
        }
    }
    *dst = '\0';
    if (out_size)
        *out_size = dst - out;
    return out;
}

char *subs_charset_ToUTF8(const void *buf, size_t size, const char *charset,
                          size_t *out_size)
{
    size_t errors;
    return convert(buf, size, charset, out_size, &errors);
}

/*****************************************************************************
 * Languages
 *****************************************************************************/

static const struct
{
    const char *charset;
    const char *langs; /* ISO 639-1, 639-2 and English names, ' ' separated */
} lang_charsets[] = {
    { "CP1255", "he iw heb hebrew yi yid yiddish" },
    { "CP1256", "ar ara arabic fa per fas persian farsi ur urd urdu ps pus pashto "
                "ku kur kurdish ckb sd snd sindhi ug uig uyghur" },
    { "CP1251", "ru rus russian uk ukr ukrainian be bel belarusian bg bul bulgarian "
                "mk mac mkd macedonian sr srp serbian kk kaz kazakh ky kir kyrgyz "
                "tg tgk tajik mn mon mongolian tt tat tatar ba bak bashkir" },
    { "CP1250", "pl pol polish cs cze ces czech sk slo slk slovak hu hun hungarian "
                "ro rum ron romanian hr hrv croatian sl slv slovenian bs bos bosnian "
                "sq alb sqi albanian" },
    { "CP1253", "el gre ell greek" },
    { "CP1254", "tr tur turkish az aze azerbaijani" },
    { "CP1257", "lt lit lithuanian lv lav latvian et est estonian" },
    { "CP1258", "vi vie vietnamese" },
    { "CP874",  "th tha thai" },
    { "CP932",  "ja jpn japanese" },
    { "CP950",  "zh-tw zh-hk zh-mo zh-hant cht tc traditional" },
    { "GB18030","zh chi zho chinese zh-cn zh-sg zh-hans chs sc simplified" },
    { "CP949",  "ko kor korean" },
    { "CP1252", "en eng english fr fre fra french de ger deu german es spa spanish "
                "it ita italian pt por portuguese nl dut nld dutch sv swe swedish "
                "da dan danish no nor norwegian nb nob nn nno fi fin finnish "
                "is ice isl icelandic ca cat catalan eu baq eus basque "
                "gl glg galician ga gle irish af afr afrikaans id ind indonesian "
                "ms may msa malay tl tgl tagalog fil sw swa swahili" },
};

/* "pt_BR", "zh-TW.srt": lower case, '-' for '_' */
static void lang_normalize(const char *lang, char *out, size_t size)
{
    size_t i = 0;
    for (; lang[i] && i + 1 < size; i++)
    {
        char c = lang[i];
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        else if (c == '_')
            c = '-';
        else if (!((c >= 'a' && c <= 'z') || c == '-'))
            break;
        out[i] = c;
    }
    out[i] = '\0';
}

static bool word_in(const char *list, const char *word)
{
    size_t len = strlen(word);
    if (len == 0)
        return false;
    for (const char *p = list; (p = strstr(p, word)) != NULL; p += len)
        if ((p == list || p[-1] == ' ') && (p[len] == ' ' || p[len] == '\0'))
            return true;
    return false;
}

const char *subs_charset_ForLanguage(const char *lang)
{
    if (lang == NULL)
        return NULL;
    char norm[32];
    lang_normalize(lang, norm, sizeof (norm));
    for (int pass = 0; pass < 2; pass++)
    {
        for (size_t i = 0; i < sizeof (lang_charsets) / sizeof (lang_charsets[0]); i++)
            if (word_in(lang_charsets[i].langs, norm))
                return lang_charsets[i].charset;
        /* "he-IL", "pt-BR": the language without its region */
        char *dash = strchr(norm, '-');
        if (dash == NULL)
            break;
        *dash = '\0';
    }
    return NULL;
}

/*****************************************************************************
 * Byte order marks, UTF-16 and UTF-8
 *****************************************************************************/

static bool IsUTF8Valid(const unsigned char *p, size_t size, bool *ascii)
{
    const unsigned char *end = p + size;
    *ascii = true;
    while (p < end)
    {
        unsigned char c = *p++;
        if (c < 0x80)
            continue;
        *ascii = false;
        size_t n;
        uint32_t min;
        if (c >= 0xC2 && c <= 0xDF)      { n = 1; min = 0x80; }
        else if (c >= 0xE0 && c <= 0xEF) { n = 2; min = 0x800; }
        else if (c >= 0xF0 && c <= 0xF4) { n = 3; min = 0x10000; }
        else
            return false;
        uint32_t cp = c & (0x3F >> n);
        for (size_t i = 0; i < n; i++)
        {
            if (p >= end)
                return true; /* cut by the analysed size */
            if ((*p & 0xC0) != 0x80)
                return false;
            cp = (cp << 6) | (*p++ & 0x3F);
        }
        if (cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            return false;
    }
    return true;
}

/*****************************************************************************
 * Scoring of the decoded text
 *****************************************************************************/

enum script
{
    SC_NONE, SC_LATIN, SC_GREEK, SC_CYRILLIC, SC_HEBREW, SC_ARABIC, SC_THAI,
    SC_KANA, SC_HAN, SC_HANGUL,
};

enum
{
    F_LETTER = 1, F_MARK = 2, F_UPPER = 4, F_LOWER = 8, F_SYMBOL = 16,
    F_CONTROL = 32, F_SPACE = 64, F_CJKPUNCT = 128,
};

static enum script classify(uint32_t c, int *flags)
{
    *flags = 0;
    if (c < 0x80)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            *flags = F_LETTER | (c >= 'a' ? F_LOWER : F_UPPER);
            return SC_LATIN;
        }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            *flags = F_SPACE;
        else if (c < 0x20 || c == 0x7F)
            *flags = F_CONTROL;
        return SC_NONE;
    }
    if (c < 0xA0)
    {
        *flags = F_CONTROL;
        return SC_NONE;
    }
    if (c == 0xFFFD)
    {
        *flags = F_CONTROL;
        return SC_NONE;
    }
    if (c >= 0xC0 && c <= 0x24F && c != 0xD7 && c != 0xF7)
    {
        bool upper;
        if (c < 0x100)
            upper = c < 0xDF;
        else if (c == 0x131 || c == 0x138 || c == 0x149 || c == 0x17F)
            upper = false;
        else if ((c >= 0x139 && c <= 0x148) || (c >= 0x179 && c <= 0x17E))
            upper = c & 1;
        else
            upper = !(c & 1);
        *flags = F_LETTER | (upper ? F_UPPER : F_LOWER);
        return SC_LATIN;
    }
    if (c >= 0x1E00 && c <= 0x1EFF)
    {
        *flags = F_LETTER | ((c & 1) ? F_LOWER : F_UPPER);
        return SC_LATIN;
    }
    if (c >= 0x300 && c <= 0x36F)
    {
        *flags = F_MARK;
        return SC_LATIN;
    }
    if (c >= 0x386 && c <= 0x3CE && c != 0x387)
    {
        *flags = F_LETTER | ((c >= 0x3AC || c == 0x390) ? F_LOWER : F_UPPER);
        return SC_GREEK;
    }
    if (c >= 0x400 && c <= 0x4FF)
    {
        bool upper = c < 0x430 || (c >= 0x460 && !(c & 1));
        if (c >= 0x450 && c < 0x460)
            upper = false;
        *flags = F_LETTER | (upper ? F_UPPER : F_LOWER);
        return SC_CYRILLIC;
    }
    if (c >= 0x591 && c <= 0x5C7 && c != 0x5BE && c != 0x5C0 && c != 0x5C3 && c != 0x5C6)
    {
        *flags = F_MARK;
        return SC_HEBREW;
    }
    if ((c >= 0x5D0 && c <= 0x5F2) || (c >= 0xFB1D && c <= 0xFB4F))
    {
        *flags = F_LETTER;
        return SC_HEBREW;
    }
    if ((c >= 0x64B && c <= 0x65F) || c == 0x670)
    {
        *flags = F_MARK;
        return SC_ARABIC;
    }
    if ((c >= 0x620 && c <= 0x64A) || (c >= 0x660 && c <= 0x669) ||
        (c >= 0x66E && c <= 0x6D3) || (c >= 0x6FA && c <= 0x6FF) ||
        (c >= 0xFB50 && c <= 0xFDFF) || (c >= 0xFE70 && c <= 0xFEFC))
    {
        *flags = F_LETTER;
        return SC_ARABIC;
    }
    if (c == 0x60C || c == 0x61B || c == 0x61F || c == 0x640)
    {
        *flags = F_CJKPUNCT; /* punctuation of the script: counts for it */
        return SC_ARABIC;
    }
    if (c >= 0xE01 && c <= 0xE5B)
    {
        if (c == 0xE31 || (c >= 0xE34 && c <= 0xE3A) || (c >= 0xE47 && c <= 0xE4E))
            *flags = F_MARK;
        else
            *flags = F_LETTER;
        return SC_THAI;
    }
    if ((c >= 0x3041 && c <= 0x30FF) || (c >= 0x31F0 && c <= 0x31FF))
    {
        *flags = F_LETTER;
        return SC_KANA;
    }
    if ((c >= 0x4E00 && c <= 0x9FFF) || (c >= 0x3400 && c <= 0x4DBF) ||
        (c >= 0xF900 && c <= 0xFAFF))
    {
        *flags = F_LETTER;
        return SC_HAN;
    }
    if ((c >= 0xAC00 && c <= 0xD7A3) || (c >= 0x3131 && c <= 0x318E))
    {
        *flags = F_LETTER;
        return SC_HANGUL;
    }
    if ((c >= 0x3000 && c <= 0x303F) || (c >= 0xFF01 && c <= 0xFF60) ||
        (c >= 0xFFE0 && c <= 0xFFE6))
    {
        *flags = F_CJKPUNCT;
        return SC_NONE;
    }
    if (c >= 0xE000 && c <= 0xF8FF) /* private use: nothing real */
    {
        *flags = F_CONTROL;
        return SC_NONE;
    }
    if (c == 0xA0 || c == 0x3000)
        *flags = F_SPACE;
    else
        *flags = F_SYMBOL;
    return SC_NONE;
}

enum kind { K_SBCS, K_SJIS, K_EUCJP, K_GB, K_BIG5, K_UHC };

typedef struct
{
    const char *charset;     /* iconv names, '|' separated alternatives */
    enum script script;
    enum kind kind;
    const char *langs;       /* code page of these languages (lang_charsets) */
    /* Latin: the letters with diacritics of each language of the code page
     * (lower case, UTF-8, '|' between languages); the text is scored as the
     * language it fits best: a text mixing the letters of two languages
     * ("èiû" and "ş") is garbage */
    const char *letters;
    const char *neutral;     /* Latin letters it also holds (UTF-8) */
} candidate_t;

#define WESTERN "àâæçèéêëîïôœùûüÿ|äöüßéëèïóá|áéíóúñüàèçòï|áâãàçéêíóôõú|àèéìíîòóùú|åäöæøéüáóðþý"
#define CENTRAL "ąćęłńóśźż|áčďéěíňóřšťúůýžäĺľôŕ|áéíóöőúüű|ăâîșțşţ|čćđšž|ëç"
#define BALTIC  "ąčęėįšųūž|āčēģīķļņšūž|äöõüšž"

/* In the order that wins a tie */
static const candidate_t candidates[] = {
    { "CP1252",      SC_LATIN,    K_SBCS,  "CP1252", WESTERN, NULL },
    { "CP1250",      SC_LATIN,    K_SBCS,  "CP1250", CENTRAL, NULL },
    { "CP1251",      SC_CYRILLIC, K_SBCS,  "CP1251", NULL, NULL },
    { "CP1253",      SC_GREEK,    K_SBCS,  "CP1253", NULL, NULL },
    { "CP1254",      SC_LATIN,    K_SBCS,  "CP1254", WESTERN "|çğıöşüâîû", NULL },
    { "CP1255",      SC_HEBREW,   K_SBCS,  "CP1255", NULL, NULL },
    { "CP1256",      SC_ARABIC,   K_SBCS,  "CP1256", NULL, "àâçèéêëîïôùûüœ" },
    { "CP1257",      SC_LATIN,    K_SBCS,  "CP1257", BALTIC, NULL },
    { "CP1258",      SC_LATIN,    K_SBCS,  "CP1258", "vi", NULL },
    { "CP874",       SC_THAI,     K_SBCS,  "CP874", NULL, NULL },
    { "KOI8-R",      SC_CYRILLIC, K_SBCS,  "CP1251", NULL, NULL },
    { "KOI8-U",      SC_CYRILLIC, K_SBCS,  "CP1251", NULL, NULL },
    { "ISO-8859-2",  SC_LATIN,    K_SBCS,  "CP1250", CENTRAL, NULL },
    { "ISO-8859-15", SC_LATIN,    K_SBCS,  "CP1252", WESTERN "|äöõüšž", NULL },
    { "ISO-8859-6",  SC_ARABIC,   K_SBCS,  "CP1256", NULL, NULL },
    { "ISO-8859-7",  SC_GREEK,    K_SBCS,  "CP1253", NULL, NULL },
    { "ISO-8859-8",  SC_HEBREW,   K_SBCS,  "CP1255", NULL, NULL },
    { "CP932|SHIFT_JIS", SC_KANA, K_SJIS,  "CP932", NULL, NULL },
    { "EUC-JP",      SC_KANA,     K_EUCJP, "CP932", NULL, NULL },
    { "GB18030|GBK", SC_HAN,      K_GB,    "GB18030", NULL, NULL },
    { "CP950|BIG5",  SC_HAN,      K_BIG5,  "CP950", NULL, NULL },
    { "CP949|EUC-KR",SC_HANGUL,   K_UHC,   "CP949", NULL, NULL },
};
#define CANDIDATES (sizeof (candidates) / sizeof (candidates[0]))

/* The lower case of a Latin letter with a diacritic */
static uint32_t latin_lower(uint32_t c)
{
    if (c >= 0xC0 && c <= 0xDE && c != 0xD7)
        return c + 0x20;
    if (c == 0x130)
        return 'i';
    if (c == 0x178)
        return 0xFF;
    if ((c >= 0x139 && c <= 0x148) || (c >= 0x179 && c <= 0x17E))
        return (c & 1) ? c + 1 : c;
    if ((c >= 0x100 && c <= 0x17F) || (c >= 0x1E00 && c <= 0x1EFF))
        return (c & 1) ? c : c + 1;
    if (c == 0x218 || c == 0x21A)
        return c + 1;
    return c;
}

/* Whether a code point is one of the letters of a UTF-8 list (ends at '|') */
static bool in_letters(const char *list, uint32_t c)
{
    if (list == NULL)
        return false;
    const unsigned char *p = (const unsigned char *)list;
    while (*p && *p != '|')
        if (utf8_get(&p) == c)
            return true;
    return false;
}

/* Russian and Ukrainian letter frequencies: о е а и н т с (+2) ... ъ (-4).
 * Text of another code page read as Cyrillic is made of rare letters. */
static int cyrillic_weight(uint32_t c)
{
    if (c >= 0x410 && c <= 0x42F)
        c += 0x20;
    else if (c >= 0x400 && c <= 0x40F)
        c += 0x50;
    static const signed char w[32] = {
        /* а б в г д е ж з и й к л м н о п */
           2, 0, 1, 0, 1, 2, 0, 0, 2, 0, 1, 1, 1, 2, 2, 1,
        /* р с т у ф х ц ч ш щ ъ ы ь э ю я */
           1, 2, 2, 1,-2, 0,-1, 0,-1,-2,-4, 0, 0,-2,-1, 0,
    };
    if (c >= 0x430 && c <= 0x44F)
        return w[c - 0x430];
    if (c == 0x456 || c == 0x457 || c == 0x454 || c == 0x491 || c == 0x451)
        return 0; /* і ї є ґ ё */
    return -2;
}

/* Greek: α ο ε ι τ ν (+2) ... ψ ξ ζ (-1) */
static int greek_weight(uint32_t c)
{
    if (c >= 0x391 && c <= 0x3A9)
        c += 0x20;
    switch (c)
    {
        case 0x3B1: case 0x3BF: case 0x3B5: case 0x3B9: case 0x3C4: case 0x3BD:
            return 2;
        case 0x3C3: case 0x3C1: case 0x3B7: case 0x3C0: case 0x3BA: case 0x3C5:
        case 0x3BC: case 0x3BB: case 0x3AC: case 0x3AD: case 0x3AF: case 0x3CC:
        case 0x3CD: case 0x3AE: case 0x3CE: case 0x3C2:
            return 1;
        case 0x3C8: case 0x3BE: case 0x3B6: case 0x3CA: case 0x3CB: case 0x390:
        case 0x3B0:
            return -1;
        default:
            return 0;
    }
}

static bool thai_consonant(uint32_t c)
{
    return c >= 0xE01 && c <= 0xE2E;
}

/* Rows of the double-byte encodings: the frequent characters (first level
 * kanji/hanzi, kana, hangul) are in a few rows. Real text has most of its
 * characters there, text of another encoding read with it hasn't. */
static int rows_score(const candidate_t *cand, const unsigned char *p, size_t size)
{
    int score = 0;
    for (size_t i = 0; i < size; i++)
    {
        unsigned b = p[i];
        if (b < 0x80)
            continue;
        if (cand->kind == K_SJIS && b >= 0xA1 && b <= 0xDF)
        {
            score -= 2; /* half width katakana: rare */
            continue;
        }
        if (cand->kind == K_EUCJP && b == 0x8E)
        {
            i++;
            score -= 2;
            continue;
        }
        if (i + 1 >= size)
            break;
        unsigned t = p[++i];
        switch (cand->kind)
        {
            case K_SJIS:
                if (b == 0x82 || b == 0x83 || b == 0x81)
                    score += 2;         /* kana, punctuation */
                else if (b >= 0x88 && b <= 0x9F)
                    score += 2;         /* first level kanji */
                else if (b >= 0xE0 && b <= 0xEA)
                    score += 1;         /* second level kanji */
                break;
            case K_EUCJP:
                if (b == 0xA4 || b == 0xA5 || b == 0xA1)
                    score += 2;
                else if (b >= 0xB0 && b <= 0xCF)
                    score += 2;
                else if (b >= 0xD0 && b <= 0xF4)
                    score += 1;
                break;
            case K_GB:
                if (b >= 0xB0 && b <= 0xD7 && t >= 0xA1)
                    score += 2;         /* first level hanzi */
                else if ((b == 0xA1 || b == 0xA3) && t >= 0xA1)
                    score += 1;         /* punctuation, full width */
                else if (b >= 0xD8 && b <= 0xF7 && t >= 0xA1)
                    score += 1;
                break;
            case K_BIG5:
                if (b >= 0xA4 && b <= 0xC6)
                    score += 2;         /* frequent hanzi */
                else if (b >= 0xA1 && b <= 0xA3)
                    score += 1;         /* punctuation */
                else if (b >= 0xC9 && b <= 0xF9)
                    score += 1;
                break;
            case K_UHC:
                if (b >= 0xB0 && b <= 0xC8 && t >= 0xA1)
                    score += 2;         /* hangul */
                else if (b >= 0xA1 && b <= 0xAC && t >= 0xA1)
                    score += 1;
                break;
            default:
                break;
        }
    }
    return score;
}

/* Symbols that real text keeps away from letters (¤ ¦ ¨ ´ ¸ ¹ ½...): text
 * of another code page shows them inside its words */
static bool odd_symbol(uint32_t c)
{
    return (c >= 0xA2 && c <= 0xBE && c != 0xA3 && c != 0xA5 && c != 0xA7 &&
            c != 0xA9 && c != 0xAB && c != 0xAE && c != 0xB0 && c != 0xB7 &&
            c != 0xBB) || (c >= 0x2500 && c <= 0x25FF) || c == 0x2219 ||
            c == 0x221A || c == 0x2248 || c == 0x2264 || c == 0x2265 ||
            c == 0x2320 || c == 0x2321 || c == 0x2552;
}

/* The score of a text as the text of a language of a candidate:
 * letters = one language of the candidate's list (Latin) */
static long score_text(const candidate_t *cand, const char *letters,
                       const char *text, size_t size, size_t errors)
{
    long score = -25 * (long)errors;
    const unsigned char *p = (const unsigned char *)text;
    const unsigned char *end = p + size;

    /* the word being read */
    unsigned word_scripts = 0, word_letters = 0, word_ascii = 0;
    uint32_t prev = ' ';
    enum script prev_script = SC_NONE;
    int prev_flags = F_SPACE;
    enum script last_script = SC_NONE; /* before the spaces */
    bool spaces = false;
    bool symbol_in_word = false;       /* a symbol right after a letter */
    unsigned thai_consonants = 0;      /* in a row */
    const bool cjk = cand->kind != K_SBCS;
    const bool latin = cand->script == SC_LATIN;
    const bool vietnamese = latin && letters && !strcmp(letters, "vi");

    while (p < end)
    {
        uint32_t c = utf8_get(&p);
        int flags;
        enum script s = classify(c, &flags);
        if (!(flags & F_LETTER))
            thai_consonants = 0;

        /* Hebrew final letters (ך ם ן ף ץ) end words */
        if (prev_script == SC_HEBREW && (prev == 0x5DA || prev == 0x5DD ||
            prev == 0x5DF || prev == 0x5E3 || prev == 0x5E5) &&
            (flags & (F_LETTER | F_MARK)) && s == SC_HEBREW)
            score -= 4;
        else if (prev_script == SC_HEBREW && cand->script == SC_HEBREW &&
                 (prev == 0x5DA || prev == 0x5DD || prev == 0x5DF ||
                  prev == 0x5E3 || prev == 0x5E5) && !(flags & (F_LETTER | F_MARK)))
            score += 3; /* ... where they are a mark of Hebrew */
        /* Greek: ς at the end of words only */
        if (prev == 0x3C2 && (flags & F_LETTER) && s == SC_GREEK)
            score -= 6;

        if (flags & F_LETTER)
        {
            if (symbol_in_word)
                score -= 6; /* "Ð¿Ñ": a symbol between two letters */
            symbol_in_word = false;
            if (odd_symbol(prev))
                score -= 4; /* "½uvre" */

            if (c < 0x80)
                word_ascii++;
            else if (cjk && (s == SC_HAN || s == SC_KANA || s == SC_HANGUL))
            {
                if (s == SC_HANGUL)
                    score += cand->script == SC_HANGUL ? 5 : -3;
                else if (s == SC_KANA)
                    score += cand->script == SC_KANA ? 5 : -2;
                else
                    /* Korean writes few hanja: text of another encoding */
                    score += cand->script == SC_HANGUL ? -3 : 2;
                /* Chinese and Japanese have no spaces between words */
                if (spaces && s != SC_HANGUL && cand->script != SC_HANGUL &&
                    (last_script == SC_HAN || last_script == SC_KANA))
                    score -= 3;
            }
            else if (s == cand->script)
            {
                switch (s)
                {
                    case SC_LATIN:
                        if (vietnamese)
                            score += (c >= 0x1EA0 && c <= 0x1EF9) ||
                                     in_letters("ăâđêôơưàáãèéìíòóõùúý", latin_lower(c)) ? 4 : 1;
                        else
                            score += in_letters(letters, latin_lower(c)) ? 4 : 1;
                        break;
                    /* (text is mostly lower case) */
                    case SC_CYRILLIC:
                        score += ((flags & F_LOWER) ? 2 : 1) + cyrillic_weight(c);
                        break;
                    case SC_GREEK:
                        score += ((flags & F_LOWER) ? 2 : 1) + greek_weight(c);
                        break;
                    case SC_THAI:
                        score += 3;
                        /* vowels come every few consonants */
                        if (thai_consonant(c) && ++thai_consonants > 3)
                            score -= 3;
                        else if (!thai_consonant(c))
                            thai_consonants = 0;
                        if (c == 0xE03 || c == 0xE05 || c == 0xE26 ||
                            c == 0xE4F || c == 0xE5A || c == 0xE5B)
                            score -= 8; /* obsolete letters, rare signs */
                        else if (c >= 0xE50 && c <= 0xE59 &&
                                 (prev_script == SC_THAI || p < end))
                            score -= 2; /* Thai digits */
                        else if ((c == 0xE30 || c == 0xE32 || c == 0xE33 || c == 0xE45) &&
                                 !(thai_consonant(prev) || (prev_flags & F_MARK)))
                            score -= 5; /* a vowel after its consonant */
                        if (prev >= 0xE40 && prev <= 0xE44 && !thai_consonant(c))
                            score -= 6; /* a leading vowel before a consonant */
                        break;
                    default:
                        score += 3;
                        break;
                }
            }
            else if (s == SC_LATIN && in_letters(cand->neutral, latin_lower(c)))
                ;
            else
                score -= 3; /* a letter of another script */

            if ((flags & F_UPPER) && (prev_flags & F_LOWER) && prev_script == s &&
                (c >= 0x80 || prev >= 0x80)) /* (not "iPhone", "PlayResX") */
                score -= 8; /* "pRIVET": the case of another code page */
            if (s != SC_HAN && s != SC_KANA && s != SC_HANGUL)
            {
                word_scripts |= 1u << s;
                word_letters++;
            }
        }
        else if (flags & F_MARK)
        {
            if (s == SC_THAI)
            {
                if (thai_consonant(prev) ||
                    ((prev_flags & F_MARK) && prev_script == SC_THAI && c >= 0xE47))
                    score += cand->script == SC_THAI ? 1 : -3;
                else
                    score -= 6; /* a vowel or tone mark without its consonant */
            }
            else if (!(prev_flags & (F_LETTER | F_MARK)) ||
                     (prev_script != s && s != SC_LATIN))
                score -= 5; /* a mark without its letter */
            else if (s == cand->script)
                score += 1;
            else if (s != SC_LATIN)
                score -= 3;
        }
        else
        {
            /* end of a word */
            if (word_letters > 0)
            {
                if (word_scripts & (word_scripts - 1))
                    score -= 6; /* several scripts in one word */
                else if (latin && word_ascii == 0 && word_letters >= 3)
                    score -= 4; /* "ùìåí": a word of accents only */
            }
            word_scripts = word_letters = word_ascii = 0;

            if (flags & F_CONTROL)
                score -= 25;
            else if (flags & F_SYMBOL)
            {
                /* the apostrophe, the hyphens and the middle dot belong
                 * in words */
                if ((prev_flags & F_LETTER) && c != 0x2019 && c != 0xB7 &&
                    c != 0x2010 && c != 0x2011 && c != 0xAD)
                    symbol_in_word = true;
                if (c == 0xA4 || c == 0xA6 || c == 0xA8 || c == 0xB4 || c == 0xB8)
                    score -= 2; /* ¤ ¦ ¨ ´ ¸: next to nothing in real text */
            }
            else if (flags & F_CJKPUNCT)
            {
                if (cjk || s == cand->script)
                    score += 1;
                else
                    score -= 3;
            }
            if (!(flags & F_SYMBOL))
                symbol_in_word = false;
        }

        if (flags & F_SPACE)
            spaces = true;
        else
        {
            spaces = false;
            last_script = s;
        }
        prev = c;
        prev_flags = flags;
        prev_script = s;
    }
    return score;
}

/* The score of a text for a candidate: as the language that fits best */
static long score_candidate(const candidate_t *cand, const char *text,
                            size_t size, size_t errors)
{
    if (cand->letters == NULL)
        return score_text(cand, NULL, text, size, errors);
    long best = 0;
    bool first = true;
    for (const char *l = cand->letters; ; )
    {
        long score = score_text(cand, l, text, size, errors);
        if (first || score > best)
            best = score;
        first = false;
        l = strchr(l, '|');
        if (l == NULL)
            break;
        l++;
    }
    return best;
}

/* Opens the first converter of a list of names ('|' separated) */
static bool open_any(const char *names, char *chosen, size_t size)
{
    while (*names)
    {
        size_t len = strcspn(names, "|");
        if (len < size)
        {
            memcpy(chosen, names, len);
            chosen[len] = '\0';
            vlc_iconv_t cd = vlc_iconv_open("UTF-8", chosen);
            if (cd != (vlc_iconv_t)(-1))
            {
                vlc_iconv_close(cd);
                return true;
            }
        }
        names += len;
        if (*names == '|')
            names++;
    }
    return false;
}

/* The name to return: a string with a static lifetime */
static const char *static_name(const char *chosen)
{
    static const char *const known[] = {
        "CP1252", "CP1250", "CP1251", "CP1253", "CP1254", "CP1255", "CP1256",
        "CP1257", "CP1258", "CP874", "KOI8-R", "KOI8-U", "ISO-8859-2",
        "ISO-8859-15", "ISO-8859-6", "ISO-8859-7", "ISO-8859-8", "CP932",
        "SHIFT_JIS", "EUC-JP", "GB18030", "GBK", "CP950", "BIG5", "CP949",
        "EUC-KR",
    };
    for (size_t i = 0; i < sizeof (known) / sizeof (known[0]); i++)
        if (!strcmp(known[i], chosen))
            return known[i];
    return "CP1252";
}

subs_charset_t subs_charset_Detect(const void *buf, size_t size,
                                   const char *const *langs, size_t lang_count,
                                   const char *fallback)
{
    const unsigned char *p = buf;
    subs_charset_t res = { "UTF-8", 0 };

    /* 1. byte order marks (UTF-32 first: FF FE 00 00 starts as UTF-16LE) */
    if (size >= 4 && !memcmp(p, "\xFF\xFE\x00\x00", 4))
        return (subs_charset_t){ "UTF-32LE", 4 };
    if (size >= 4 && !memcmp(p, "\x00\x00\xFE\xFF", 4))
        return (subs_charset_t){ "UTF-32BE", 4 };
    if (size >= 3 && !memcmp(p, "\xEF\xBB\xBF", 3))
        return (subs_charset_t){ "UTF-8", 3 };
    if (size >= 2 && !memcmp(p, "\xFF\xFE", 2))
        return (subs_charset_t){ "UTF-16LE", 2 };
    if (size >= 2 && !memcmp(p, "\xFE\xFF", 2))
        return (subs_charset_t){ "UTF-16BE", 2 };

    if (size > ANALYSED_MAX)
        size = ANALYSED_MAX;

    /* 2. UTF-16 without a mark: the zero high bytes of ASCII */
    if (size >= 16)
    {
        size_t zero_even = 0, zero_odd = 0;
        for (size_t i = 0; i + 1 < size; i += 2)
        {
            zero_even += p[i] == 0;
            zero_odd += p[i + 1] == 0;
        }
        const size_t units = size / 2;
        if (zero_odd > units * 3 / 10 && zero_even < units / 20)
            return (subs_charset_t){ "UTF-16LE", 0 };
        if (zero_even > units * 3 / 10 && zero_odd < units / 20)
            return (subs_charset_t){ "UTF-16BE", 0 };
    }

    /* 3. UTF-8 */
    bool ascii;
    if (IsUTF8Valid(p, size, &ascii))
        return res;

    /* 4. the legacy encoding whose text reads best */
    const char *hinted[8];
    size_t hint_count = 0;
    for (size_t i = 0; i < lang_count && hint_count < 8; i++)
    {
        const char *cs = subs_charset_ForLanguage(langs[i]);
        if (cs != NULL)
            hinted[hint_count++] = cs;
    }

    long best = 0;
    const char *best_name = NULL;
    for (size_t i = 0; i < CANDIDATES; i++)
    {
        const candidate_t *cand = &candidates[i];
        char chosen[24];
        if (!open_any(cand->charset, chosen, sizeof (chosen)))
            continue;
        size_t text_size, errors;
        char *text = convert(p, size, chosen, &text_size, &errors);
        if (text == NULL)
            continue;
        long score = score_candidate(cand, text, text_size, errors);
        free(text);
        if (cand->kind != K_SBCS)
            score += rows_score(cand, p, size);

        /* the hints: the encoding of the languages of the text, of the
         * user, of the interface */
        for (size_t h = 0; h < hint_count; h++)
            if (!strcmp(hinted[h], cand->langs))
            {
                long weight = 12 - 2 * (long)h;
                if (weight < 4)
                    weight = 4;
                score += (score > 0 ? score * weight / 100 : 0) + 2;
                break;
            }
        if (fallback != NULL && !strcmp(fallback, cand->langs))
            score += 1;
#ifdef SUBS_CHARSET_DEBUG
        fprintf(stderr, "  %-12s %ld\n", chosen, score);
#endif

        if (best_name == NULL || score > best)
        {
            best = score;
            best_name = static_name(chosen);
        }
    }

    /* 5. nothing reads like text: the encoding of the language */
    if (best_name == NULL || best <= 0)
    {
        if (hint_count > 0)
            best_name = hinted[0];
        else if (fallback != NULL && *fallback)
            best_name = fallback;
        else
            best_name = "CP1252";
    }
    res.charset = best_name;
    return res;
}

#ifndef SUBS_CHARSET_STANDALONE
/*****************************************************************************
 * Loading a file
 *****************************************************************************/
stream_t *subs_charset_LoadUTF8(vlc_object_t *obj, stream_t *s,
                                const char *lang)
{
    /* subtitle files are small: read all of it (up to 64 MiB) */
    uint8_t *data = NULL;
    size_t size = 0, alloc = 0;
    for (;;)
    {
        if (alloc - size < 65536)
        {
            if (alloc >= (64 << 20))
                break;
            alloc = alloc ? alloc * 2 : 262144;
            uint8_t *grown = realloc(data, alloc);
            if (grown == NULL)
            {
                free(data);
                return NULL;
            }
            data = grown;
        }
        ssize_t got = vlc_stream_Read(s, &data[size], alloc - size);
        if (got <= 0)
            break;
        size += got;
    }

    /* the hints: the language of the file, then the preferred ones */
    const char *langs[4];
    size_t lang_count = 0;
    if (lang != NULL && *lang)
        langs[lang_count++] = lang;
    char *pref = var_InheritString(obj, "sub-language");
    if (pref != NULL)
    {
        char *save;
        for (char *l = strtok_r(pref, ",", &save);
             l != NULL && lang_count < ARRAY_SIZE(langs);
             l = strtok_r(NULL, ",", &save))
        {
            while (*l == ' ')
                l++;
            if (*l && strcmp(l, "any") && strcmp(l, "none"))
                langs[lang_count++] = l;
        }
    }
    /* xgettext: see subsdec.c: the code page of the interface language */
    const char *ui_charset = vlc_pgettext("GetACP", "CP1252");

    subs_charset_t cs = subs_charset_Detect(data, size, langs, lang_count,
                                            ui_charset);
    const char *how = cs.bom_size ? "byte order mark" : "detected";
    /* the encoding chosen in the preferences, unless the file says */
    char *forced = var_InheritString(obj, "subsdec-encoding");
    if (cs.bom_size == 0 && forced != NULL && *forced)
    {
        /* "system": the code page of the system (iconv's "") */
        cs.charset = strcmp(forced, "system") ? forced : "";
        how = "preferences";
    }
    msg_Dbg(obj, "text encoding: %s (%s)", cs.charset, how);

    size_t text_size;
    char *text = subs_charset_ToUTF8(&data[cs.bom_size], size - cs.bom_size,
                                     cs.charset, &text_size);
    if (text == NULL)
    {
        msg_Err(obj, "cannot convert the subtitles from %s", cs.charset);
        /* as it is: the decoders make valid UTF-8 of it */
        text = (char *)data;
        text_size = size;
        data = NULL;
    }
    free(forced);
    free(pref);
    free(data);

    stream_t *mem = vlc_stream_MemoryNew(obj, (uint8_t *)text, text_size, false);
    if (mem == NULL)
        free(text);
    return mem;
}
#endif
