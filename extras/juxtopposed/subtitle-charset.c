/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * The encoding detection of text subtitles (modules/codec/subs_charset.c)
 * over the corpus of extras/juxtopposed/fixtures/subtitles: each file is
 * detected with the language of its name as VLC finds it ("movie.he.srt")
 * and an English interface (Windows-1252 fallback), converted, and written
 * to <out dir>/<file>.out; subtitle-charset-test.py compares them with the
 * .expected texts.
 *   cc -std=c11 -O1 -Wall -DSUBS_CHARSET_STANDALONE -Imodules/codec \
 *      extras/juxtopposed/subtitle-charset.c modules/codec/subs_charset.c \
 *      -o subtitle-charset [-liconv]
 *   ./subtitle-charset <out dir> <file>...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "subs_charset.h"

/* The language VLC reads in a file name: between its last two dots */
static char *language_of(const char *path)
{
    const char *name = path;
    for (const char *p = path; *p; p++)
        if (*p == '/' || *p == '\\')
            name = p + 1;
    const char *ext = strrchr(name, '.');
    if (ext == NULL)
        return NULL;
    const char *lang = NULL;
    for (const char *p = name; p < ext; p++)
        if (*p == '.')
            lang = p + 1;
    if (lang == NULL)
        return NULL;
    char *ret = malloc(ext - lang + 1);
    memcpy(ret, lang, ext - lang);
    ret[ext - lang] = '\0';
    return ret;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <out dir> <file>...\n", argv[0]);
        return 2;
    }
    int failures = 0;
    for (int i = 2; i < argc; i++)
    {
        FILE *f = fopen(argv[i], "rb");
        if (f == NULL)
        {
            perror(argv[i]);
            failures++;
            continue;
        }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *buf = malloc(size + 1);
        size_t got = fread(buf, 1, size, f);
        fclose(f);

        char *lang = language_of(argv[i]);
        const char *langs[1] = { lang };
        subs_charset_t cs = subs_charset_Detect(buf, got, langs, lang ? 1 : 0, "CP1252");
        size_t out_size = 0;
        char *text = subs_charset_ToUTF8(buf + cs.bom_size, got - cs.bom_size,
                                         cs.charset, &out_size);
        printf("%s\t%s\n", argv[i], cs.charset);
        if (text == NULL)
        {
            fprintf(stderr, "%s: cannot convert from %s\n", argv[i], cs.charset);
            failures++;
        }
        else
        {
            const char *name = argv[i];
            for (const char *p = argv[i]; *p; p++)
                if (*p == '/' || *p == '\\')
                    name = p + 1;
            char out[4096];
            snprintf(out, sizeof (out), "%s/%s.out", argv[1], name);
            FILE *o = fopen(out, "wb");
            if (o != NULL)
            {
                fwrite(text, 1, out_size, o);
                fclose(o);
            }
            free(text);
        }
        free(lang);
        free(buf);
    }
    return failures != 0;
}
