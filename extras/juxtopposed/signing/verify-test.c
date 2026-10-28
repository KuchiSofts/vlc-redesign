/*****************************************************************************
 * verify-test.c: checks a file's .sig as the app's updater does
 *****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/

/*
 * The test of modules/gui/qt/util/redesign_update_verify.h, with the key of
 * redesign_update_key.h (or one given in hex, X then Y):
 *
 *   verify-test <file> [<signature>] [--key <128 hex digits>]
 *
 * The signature is <file>.sig unless given. Prints "good" (exit 0) or
 * "bad" (exit 1). Built with MinGW, e.g. in the Windows build image:
 *
 *   x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -I modules/gui/qt/util \
 *       extras/juxtopposed/signing/verify-test.c -o verify-test.exe
 *
 * or with MSVC: cl /W4 /I modules\gui\qt\util extras\juxtopposed\signing\verify-test.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "redesign_update_key.h"
#include "redesign_update_verify.h"

static unsigned char *read_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    unsigned char *data = NULL;
    long length;
    if (!f)
        return NULL;
    if (fseek(f, 0, SEEK_END) == 0 && (length = ftell(f)) >= 0 && fseek(f, 0, SEEK_SET) == 0) {
        data = (unsigned char *)malloc((size_t)length + 1);
        if (data && fread(data, 1, (size_t)length, f) != (size_t)length) {
            free(data);
            data = NULL;
        }
        *size = (size_t)length;
    }
    fclose(f);
    return data;
}

static int hex_key(const char *hex, unsigned char key[64])
{
    int i;
    if (strlen(hex) != 128)
        return 0;
    for (i = 0; i < 64; i++) {
        unsigned int byte;
        if (sscanf(hex + 2 * i, "%2x", &byte) != 1)
            return 0;
        key[i] = (unsigned char)byte;
    }
    return 1;
}

int main(int argc, char **argv)
{
    unsigned char key[64];
    const char *file = NULL, *sig_path = NULL;
    char default_sig[4096];
    unsigned char *data, *sig;
    size_t size = 0, sig_size = 0;
    int i, good;

    memcpy(key, vlc_redesign_update_key, sizeof(key));
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--key") && i + 1 < argc) {
            if (!hex_key(argv[++i], key)) {
                fprintf(stderr, "--key: 128 hex digits (X then Y)\n");
                return 2;
            }
        } else if (!file) {
            file = argv[i];
        } else if (!sig_path) {
            sig_path = argv[i];
        }
    }
    if (!file) {
        fprintf(stderr, "verify-test <file> [<signature>] [--key <hex>]\n");
        return 2;
    }
    if (!sig_path) {
        snprintf(default_sig, sizeof(default_sig), "%s.sig", file);
        sig_path = default_sig;
    }
    data = read_file(file, &size);
    sig = read_file(sig_path, &sig_size);
    if (!data || !sig) {
        fprintf(stderr, "can't read %s\n", !data ? file : sig_path);
        return 2;
    }
    good = vlc_redesign_verify_signature(key, data, size, sig, sig_size);
    printf("%s\n", good ? "good" : "bad");
    free(data);
    free(sig);
    return good ? 0 : 1;
}
