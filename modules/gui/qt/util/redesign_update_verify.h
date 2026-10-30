/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#ifndef VLC_REDESIGN_UPDATE_VERIFY_H
#define VLC_REDESIGN_UPDATE_VERIFY_H

/* The check of a release's SHA256SUMS.txt.sig (redesign_update_key.h), with
 * Windows' own CNG: an ECDSA P-256 signature, r then s (32 bytes each), of
 * the SHA-256 of the data. bcrypt.dll is loaded here, from System32:
 * nothing else of the interface needs it, and the build links nothing more.
 * C and C++ (extras/juxtopposed/signing/verify-test.c tests it as it is).
 * Windows only: other systems have no updater. */

#ifdef _WIN32
# include <stddef.h>
# include <string.h>
# include <windows.h>
# include <bcrypt.h>

# define VLC_REDESIGN_SIGNATURE_SIZE 64

/* 1: the signature is good; 0: it isn't, or it couldn't be checked */
static inline int vlc_redesign_verify_signature(const unsigned char key[64],
                                                const void *data, size_t size,
                                                const unsigned char *sig, size_t sig_size)
{
    typedef NTSTATUS (WINAPI *open_fn)(BCRYPT_ALG_HANDLE *, LPCWSTR, LPCWSTR, ULONG);
    typedef NTSTATUS (WINAPI *close_fn)(BCRYPT_ALG_HANDLE, ULONG);
    typedef NTSTATUS (WINAPI *create_hash_fn)(BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE *, PUCHAR, ULONG,
                                              PUCHAR, ULONG, ULONG);
    typedef NTSTATUS (WINAPI *hash_data_fn)(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
    typedef NTSTATUS (WINAPI *finish_hash_fn)(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
    typedef NTSTATUS (WINAPI *destroy_hash_fn)(BCRYPT_HASH_HANDLE);
    typedef NTSTATUS (WINAPI *import_fn)(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR,
                                         BCRYPT_KEY_HANDLE *, PUCHAR, ULONG, ULONG);
    typedef NTSTATUS (WINAPI *destroy_key_fn)(BCRYPT_KEY_HANDLE);
    typedef NTSTATUS (WINAPI *verify_fn)(BCRYPT_KEY_HANDLE, VOID *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
    typedef void (*any_fn)(void);

    HMODULE bcrypt;
    open_fn open_alg;
    close_fn close_alg;
    create_hash_fn create_hash;
    hash_data_fn hash_data;
    finish_hash_fn finish_hash;
    destroy_hash_fn destroy_hash;
    import_fn import_key;
    destroy_key_fn destroy_key;
    verify_fn verify;
    BCRYPT_ALG_HANDLE sha = NULL, ecdsa = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    BCRYPT_KEY_HANDLE public_key = NULL;
    unsigned char digest[32];
    /* BCRYPT_ECCPUBLIC_BLOB: the header, then X and Y */
    unsigned char blob[sizeof(BCRYPT_ECCKEY_BLOB) + 64];
    BCRYPT_ECCKEY_BLOB head;
    const unsigned char *bytes = (const unsigned char *)data;
    size_t done;
    int good = 0;

    if (!key || (!data && size) || !sig || sig_size != VLC_REDESIGN_SIGNATURE_SIZE)
        return 0;
    bcrypt = LoadLibraryExW(L"bcrypt.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!bcrypt)
        return 0;
    /* (through void (*)(void): no warning for the cast of FARPROC) */
    open_alg = (open_fn)(any_fn)GetProcAddress(bcrypt, "BCryptOpenAlgorithmProvider");
    close_alg = (close_fn)(any_fn)GetProcAddress(bcrypt, "BCryptCloseAlgorithmProvider");
    create_hash = (create_hash_fn)(any_fn)GetProcAddress(bcrypt, "BCryptCreateHash");
    hash_data = (hash_data_fn)(any_fn)GetProcAddress(bcrypt, "BCryptHashData");
    finish_hash = (finish_hash_fn)(any_fn)GetProcAddress(bcrypt, "BCryptFinishHash");
    destroy_hash = (destroy_hash_fn)(any_fn)GetProcAddress(bcrypt, "BCryptDestroyHash");
    import_key = (import_fn)(any_fn)GetProcAddress(bcrypt, "BCryptImportKeyPair");
    destroy_key = (destroy_key_fn)(any_fn)GetProcAddress(bcrypt, "BCryptDestroyKey");
    verify = (verify_fn)(any_fn)GetProcAddress(bcrypt, "BCryptVerifySignature");
    if (!open_alg || !close_alg || !create_hash || !hash_data || !finish_hash || !destroy_hash
        || !import_key || !destroy_key || !verify)
        goto out;

    /* SHA-256 of the data, in parts a ULONG can hold */
    if (!BCRYPT_SUCCESS(open_alg(&sha, BCRYPT_SHA256_ALGORITHM, NULL, 0))
        || !BCRYPT_SUCCESS(create_hash(sha, &hash, NULL, 0, NULL, 0, 0)))
        goto out;
    for (done = 0; done < size;) {
        const size_t left = size - done;
        const ULONG part = left > 0x40000000u ? 0x40000000u : (ULONG)left;
        if (!BCRYPT_SUCCESS(hash_data(hash, (PUCHAR)(bytes + done), part, 0)))
            goto out;
        done += part;
    }
    if (!BCRYPT_SUCCESS(finish_hash(hash, digest, sizeof(digest), 0)))
        goto out;

    head.dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    head.cbKey = 32;
    memcpy(blob, &head, sizeof(head));
    memcpy(blob + sizeof(head), key, 64);
    if (!BCRYPT_SUCCESS(open_alg(&ecdsa, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0))
        || !BCRYPT_SUCCESS(import_key(ecdsa, NULL, BCRYPT_ECCPUBLIC_BLOB, &public_key, blob,
                                      (ULONG)sizeof(blob), 0)))
        goto out;
    good = BCRYPT_SUCCESS(verify(public_key, NULL, digest, (ULONG)sizeof(digest),
                                 (PUCHAR)sig, (ULONG)sig_size, 0));

out:
    if (public_key)
        destroy_key(public_key);
    if (ecdsa)
        close_alg(ecdsa, 0);
    if (hash)
        destroy_hash(hash);
    if (sha)
        close_alg(sha, 0);
    FreeLibrary(bcrypt);
    return good;
}
#endif

#endif
