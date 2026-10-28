/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifndef VLC_REDESIGN_UPDATE_KEY_H
#define VLC_REDESIGN_UPDATE_KEY_H

/* The public key of the releases: SHA256SUMS.txt.sig is the signature of
 * SHA256SUMS.txt by its private key (ECDSA P-256 over SHA-256, r then s).
 * Written by extras/juxtopposed/signing/release-key.ps1 header; the only
 * copy of it in the code. X then Y, 32 bytes each, big-endian.
 *   Base64 (X || Y): HJKABzwPK90U6ZLJm/YEWd/PqqDohoZP5HqzfJO1+CstvS6Bdk9TPAPMD4zMR7/VtNfbuUrMMR5eqe+IOWAsDg==
 *   SHA-256 of the SubjectPublicKeyInfo: bb7f44e0c0d5c92986c4bd7976687bd074feffbbb414f6c17605b6f9619e2f04
 */
static const unsigned char vlc_redesign_update_key[64] = {
    0x1c, 0x92, 0x80, 0x07, 0x3c, 0x0f, 0x2b, 0xdd,
    0x14, 0xe9, 0x92, 0xc9, 0x9b, 0xf6, 0x04, 0x59,
    0xdf, 0xcf, 0xaa, 0xa0, 0xe8, 0x86, 0x86, 0x4f,
    0xe4, 0x7a, 0xb3, 0x7c, 0x93, 0xb5, 0xf8, 0x2b,
    0x2d, 0xbd, 0x2e, 0x81, 0x76, 0x4f, 0x53, 0x3c,
    0x03, 0xcc, 0x0f, 0x8c, 0xcc, 0x47, 0xbf, 0xd5,
    0xb4, 0xd7, 0xdb, 0xb9, 0x4a, 0xcc, 0x31, 0x1e,
    0x5e, 0xa9, 0xef, 0x88, 0x39, 0x60, 0x2c, 0x0e,
};

#endif
