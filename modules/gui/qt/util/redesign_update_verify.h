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
 * Windows; Linux through GnuTLS and macOS through its Security framework (below). */

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
#elif defined(__linux__)
/* Linux: the same check with the system's GnuTLS (every distribution has
 * libgnutls.so.30; VLC's own TLS uses it), loaded here: the build links
 * nothing more. Without it nothing is checked, so nothing is installed. */
# include <dlfcn.h>
# include <stddef.h>
# include <string.h>

# define VLC_REDESIGN_SIGNATURE_SIZE 64

typedef struct { unsigned char *data; unsigned int size; } vlc_redesign_datum;

/* r or s as a DER INTEGER: leading zeros dropped, a 0 added when the top bit is set */
static inline size_t vlc_redesign_der_int(unsigned char *out, const unsigned char *v)
{
    size_t skip = 0, n = 0;
    while (skip < 31 && v[skip] == 0)
        skip++;
    out[n++] = 0x02;
    out[n++] = (unsigned char)(32 - skip + (v[skip] & 0x80 ? 1 : 0));
    if (v[skip] & 0x80)
        out[n++] = 0;
    memcpy(out + n, v + skip, 32 - skip);
    return n + 32 - skip;
}


/* 1: the signature is good; 0: it isn't, or it couldn't be checked */
static inline int vlc_redesign_verify_signature(const unsigned char key[64],
                                                const void *data, size_t size,
                                                const unsigned char *sig, size_t sig_size)
{
    typedef int (*init_fn)(void **);
    typedef void (*deinit_fn)(void *);
    typedef int (*import_fn)(void *, int, const vlc_redesign_datum *, const vlc_redesign_datum *);
    typedef int (*verify_fn)(void *, int, unsigned int, const vlc_redesign_datum *,
                             const vlc_redesign_datum *);
    typedef int (*id_fn)(const char *);
    unsigned char der[2 + 2 * 35];
    vlc_redesign_datum x, y, text, signature;
    void *gnutls, *public_key = NULL;
    init_fn init;
    deinit_fn deinit;
    import_fn import_ecc;
    verify_fn verify;
    id_fn curve_id, sign_id;
    size_t n;
    int good = 0;

    if (!key || (!data && size) || !sig || sig_size != VLC_REDESIGN_SIGNATURE_SIZE || size > 0xffffffffu)
        return 0;
    gnutls = dlopen("libgnutls.so.30", RTLD_LAZY | RTLD_LOCAL);
    if (!gnutls)
        return 0;
    init = (init_fn)dlsym(gnutls, "gnutls_pubkey_init");
    deinit = (deinit_fn)dlsym(gnutls, "gnutls_pubkey_deinit");
    import_ecc = (import_fn)dlsym(gnutls, "gnutls_pubkey_import_ecc_raw");
    verify = (verify_fn)dlsym(gnutls, "gnutls_pubkey_verify_data2");
    curve_id = (id_fn)dlsym(gnutls, "gnutls_ecc_curve_get_id");
    sign_id = (id_fn)dlsym(gnutls, "gnutls_sign_get_id");
    if (!init || !deinit || !import_ecc || !verify || !curve_id || !sign_id || init(&public_key) < 0)
        goto out;

    x.data = (unsigned char *)key;      x.size = 32;
    y.data = (unsigned char *)key + 32; y.size = 32;
    if (import_ecc(public_key, curve_id("SECP256R1"), &x, &y) < 0)
        goto out;
    /* r then s, as the SEQUENCE GnuTLS takes */
    n = 2;
    n += vlc_redesign_der_int(der + n, sig);
    n += vlc_redesign_der_int(der + n, sig + 32);
    der[0] = 0x30;
    der[1] = (unsigned char)(n - 2);
    signature.data = der;               signature.size = (unsigned int)n;
    text.data = (unsigned char *)data;  text.size = (unsigned int)size;
    good = verify(public_key, sign_id("ECDSA-SHA256"), 0, &text, &signature) >= 0;

out:
    if (public_key)
        deinit(public_key);
    dlclose(gnutls);
    return good;
}
#elif defined(__APPLE__)
/* macOS: the same check with the system's Security framework
 * (SecKeyVerifySignature, ECDSA over SHA-256 of the message, X9.62 DER), it
 * and CoreFoundation loaded here: the build links nothing more. */
# include <dlfcn.h>
# include <stddef.h>
# include <string.h>
# include <CoreFoundation/CoreFoundation.h>
# include <Security/Security.h>

# define VLC_REDESIGN_SIGNATURE_SIZE 64

/* r or s as a DER INTEGER: leading zeros dropped, a 0 added when the top bit is set */
static inline size_t vlc_redesign_der_int(unsigned char *out, const unsigned char *v)
{
    size_t skip = 0, n = 0;
    while (skip < 31 && v[skip] == 0)
        skip++;
    out[n++] = 0x02;
    out[n++] = (unsigned char)(32 - skip + (v[skip] & 0x80 ? 1 : 0));
    if (v[skip] & 0x80)
        out[n++] = 0;
    memcpy(out + n, v + skip, 32 - skip);
    return n + 32 - skip;
}

/* 1: the signature is good; 0: it isn't, or it couldn't be checked */
static inline int vlc_redesign_verify_signature(const unsigned char key[64],
                                                const void *data, size_t size,
                                                const unsigned char *sig, size_t sig_size)
{
    typedef CFDataRef (*data_create_fn)(CFAllocatorRef, const UInt8 *, CFIndex);
    typedef CFDictionaryRef (*dict_create_fn)(CFAllocatorRef, const void **, const void **, CFIndex,
                                              const CFDictionaryKeyCallBacks *,
                                              const CFDictionaryValueCallBacks *);
    typedef void (*release_fn)(CFTypeRef);
    typedef SecKeyRef (*key_create_fn)(CFDataRef, CFDictionaryRef, CFErrorRef *);
    typedef Boolean (*verify_fn)(SecKeyRef, SecKeyAlgorithm, CFDataRef, CFDataRef, CFErrorRef *);
    unsigned char point[65], der[2 + 2 * 35];
    void *cf, *sec;
    data_create_fn data_create;
    dict_create_fn dict_create;
    release_fn release;
    key_create_fn key_create;
    verify_fn verify;
    const CFDictionaryKeyCallBacks *key_callbacks;
    const CFDictionaryValueCallBacks *value_callbacks;
    const CFStringRef *attr_type, *type_ec, *attr_class, *class_public, *algorithm;
    CFDataRef key_data = NULL, text = NULL, signature = NULL;
    CFDictionaryRef attributes = NULL;
    SecKeyRef public_key = NULL;
    const void *keys[2], *values[2];
    size_t n;
    int good = 0;

    if (!key || (!data && size) || !sig || sig_size != VLC_REDESIGN_SIGNATURE_SIZE || size > 0x7fffffffu)
        return 0;
    cf = dlopen("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation", RTLD_LAZY | RTLD_LOCAL);
    sec = dlopen("/System/Library/Frameworks/Security.framework/Security", RTLD_LAZY | RTLD_LOCAL);
    if (!cf || !sec)
        goto out;
    data_create = (data_create_fn)dlsym(cf, "CFDataCreate");
    dict_create = (dict_create_fn)dlsym(cf, "CFDictionaryCreate");
    release = (release_fn)dlsym(cf, "CFRelease");
    key_callbacks = (const CFDictionaryKeyCallBacks *)dlsym(cf, "kCFTypeDictionaryKeyCallBacks");
    value_callbacks = (const CFDictionaryValueCallBacks *)dlsym(cf, "kCFTypeDictionaryValueCallBacks");
    key_create = (key_create_fn)dlsym(sec, "SecKeyCreateWithData");
    verify = (verify_fn)dlsym(sec, "SecKeyVerifySignature");
    attr_type = (const CFStringRef *)dlsym(sec, "kSecAttrKeyType");
    type_ec = (const CFStringRef *)dlsym(sec, "kSecAttrKeyTypeECSECPrimeRandom");
    attr_class = (const CFStringRef *)dlsym(sec, "kSecAttrKeyClass");
    class_public = (const CFStringRef *)dlsym(sec, "kSecAttrKeyClassPublic");
    algorithm = (const CFStringRef *)dlsym(sec, "kSecKeyAlgorithmECDSASignatureMessageX962SHA256");
    if (!data_create || !dict_create || !release || !key_callbacks || !value_callbacks || !key_create
        || !verify || !attr_type || !type_ec || !attr_class || !class_public || !algorithm)
        goto out;

    /* the public key as an uncompressed point: 04, X, Y */
    point[0] = 0x04;
    memcpy(point + 1, key, 64);
    keys[0] = *attr_type;  values[0] = *type_ec;
    keys[1] = *attr_class; values[1] = *class_public;
    attributes = dict_create(NULL, keys, values, 2, key_callbacks, value_callbacks);
    key_data = data_create(NULL, point, sizeof point);
    if (!attributes || !key_data)
        goto out;
    public_key = key_create(key_data, attributes, NULL);
    if (!public_key)
        goto out;
    /* r then s, as the SEQUENCE the framework takes */
    n = 2;
    n += vlc_redesign_der_int(der + n, sig);
    n += vlc_redesign_der_int(der + n, sig + 32);
    der[0] = 0x30;
    der[1] = (unsigned char)(n - 2);
    signature = data_create(NULL, der, (CFIndex)n);
    text = data_create(NULL, (const UInt8 *)data, (CFIndex)size);
    if (signature && text)
        good = verify(public_key, *algorithm, text, signature, NULL) ? 1 : 0;

out:
    if (release) {
        if (text) release(text);
        if (signature) release(signature);
        if (public_key) release(public_key);
        if (key_data) release(key_data);
        if (attributes) release(attributes);
    }
    if (sec) dlclose(sec);
    if (cf) dlclose(cf);
    return good;
}
#endif

#endif
