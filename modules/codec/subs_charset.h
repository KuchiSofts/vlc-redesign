/*****************************************************************************
 * subs_charset.h: character encoding of text subtitles
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

#ifndef VLC_SUBS_CHARSET_H
#define VLC_SUBS_CHARSET_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Subtitle files carry no declaration of their encoding. Many are still in
 * the legacy code page of their language (Windows-1255 Hebrew, 1256 Arabic,
 * 1251 Cyrillic, Shift_JIS, GBK, Big5...), which the code page of the
 * interface language cannot be relied on to match.
 *
 * subs_charset_Detect() names the encoding of a text, in this order:
 *  1. a byte order mark (UTF-8, UTF-16LE/BE, UTF-32LE/BE),
 *  2. UTF-16 without a mark (zero bytes on one side),
 *  3. valid UTF-8 (plain ASCII included),
 *  4. the legacy encoding whose decoding reads most like real text: the
 *     letters of the script of the encoding, words of a single script, the
 *     case of the letters, marks after letters, the frequent rows of the
 *     CJK encodings, no control characters, no undefined bytes,
 *  5. else the code page of the first language hint, else the fallback.
 *
 * Language hints (ISO 639-1/2 codes or English names: the language in the
 * file name, the track language, the preferred subtitle language, the
 * interface language) break the ties between encodings that both decode
 * cleanly, such as Windows-1252 and Windows-1254 for French.
 */

/** The result: an iconv name, and the size of the byte order mark */
typedef struct
{
    const char *charset; /**< iconv name, "UTF-8" for UTF-8 and ASCII */
    size_t bom_size;     /**< bytes of the byte order mark to skip */
} subs_charset_t;

subs_charset_t subs_charset_Detect(const void *buf, size_t size,
                                   const char *const *langs, size_t lang_count,
                                   const char *fallback);

/** The legacy code page of a language ("he", "heb", "Hebrew": "CP1255"),
 * NULL if the language is not known or is written in UTF-8 only */
const char *subs_charset_ForLanguage(const char *lang);

/**
 * Converts a text to UTF-8 (without its byte order mark). Bytes that cannot
 * be converted become U+FFFD, never '?'. The Hebrew presentation forms some
 * converters compose (U+FB1D-U+FB4E) are decomposed back to letters and
 * marks. Returns a nul-terminated string to free(), NULL on error.
 */
char *subs_charset_ToUTF8(const void *buf, size_t size, const char *charset,
                          size_t *out_size);

#ifdef VLC_STREAM_H
/**
 * Reads a whole subtitle file and converts it to UTF-8: the encoding of its
 * byte order mark, else the one chosen in the preferences (subsdec-encoding),
 * else the one detected with the language of the file (lang, may be NULL),
 * the preferred subtitle languages (sub-language) and the code page of the
 * interface language as hints. Returns a memory stream of the text.
 */
stream_t *subs_charset_LoadUTF8(vlc_object_t *obj, stream_t *s,
                                const char *lang);
#endif

#ifdef __cplusplus
}
#endif

#endif
