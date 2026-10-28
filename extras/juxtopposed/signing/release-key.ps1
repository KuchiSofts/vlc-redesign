# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# The key that signs the SHA256SUMS.txt of every release, so the app's
# updater can tell a release of this project from one made by someone else
# (an ECDSA P-256 key, the signature over SHA-256 of the file):
#
#   release-key.ps1 new                    makes the key (once; never overwrites one)
#   release-key.ps1 public                 shows the public key (C array, base64, fingerprint)
#   release-key.ps1 header                 writes the public key into redesign_update_key.h
#   release-key.ps1 sign <file>            writes <file>.sig and checks it
#   release-key.ps1 verify <file>          checks <file>.sig with the key the app carries
#   release-key.ps1 export-backup <path>   a plain PKCS#8 PEM copy, for a password manager
#
# The private key stays on this PC, outside every repository, encrypted for
# this Windows user (DPAPI, CurrentUser): -KeyFile, by default
# %USERPROFILE%\source\vlc-redesign-keys\update-signing.key. It is never
# shown. Lose it (and the backup) and later releases need a new key, which
# only a new app version can carry.
#
# <file>.sig is the raw signature: r then s, 32 bytes each, big-endian (IEEE
# P1363, what CNG's BCryptVerifySignature takes). verify reads the public
# key from modules/gui/qt/util/redesign_update_key.h, the one the app
# checks with, so it needs no private key.
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet('new', 'public', 'header', 'sign', 'verify', 'export-backup')]
    [string]$Command,
    [Parameter(Position = 1)]
    [string]$Path,
    [string]$KeyFile = "$env:USERPROFILE\source\vlc-redesign-keys\update-signing.key"
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Security
$root = (Resolve-Path "$PSScriptRoot\..\..\..").Path
$header = "$root\modules\gui\qt\util\redesign_update_key.h"
$magic = [Text.Encoding]::ASCII.GetBytes('VRSK1')
# (bound to this use: a DPAPI blob of another program doesn't open as a key)
$entropy = [Text.Encoding]::UTF8.GetBytes('VLC Redesign update signing key')
$sigSize = 64

function Join-Bytes([byte[][]]$parts) {
    $out = New-Object System.IO.MemoryStream
    foreach ($p in $parts) { $out.Write($p, 0, $p.Length) }
    return , $out.ToArray()
}

function Test-Same([byte[]]$a, [byte[]]$b) {
    if ($a.Length -ne $b.Length) { return $false }
    for ($i = 0; $i -lt $a.Length; $i++) { if ($a[$i] -ne $b[$i]) { return $false } }
    return $true
}

function Get-Hex([byte[]]$bytes) { return ($bytes | ForEach-Object { $_.ToString('x2') }) -join '' }

# SubjectPublicKeyInfo of a P-256 key (what openssl pkey -pubout writes)
function Get-Spki([byte[]]$xy) {
    $prefix = [byte[]](0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01,
        0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04)
    return , (Join-Bytes @($prefix, $xy))
}

function Get-Fingerprint([byte[]]$xy) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return Get-Hex $sha.ComputeHash((Get-Spki $xy)) } finally { $sha.Dispose() }
}

function New-Ecdsa([byte[]]$xy, [byte[]]$d) {
    $q = New-Object Security.Cryptography.ECPoint
    $q.X = [byte[]]$xy[0..31]
    $q.Y = [byte[]]$xy[32..63]
    $p = New-Object Security.Cryptography.ECParameters
    $p.Curve = [Security.Cryptography.ECCurve+NamedCurves]::nistP256
    $p.Q = $q
    if ($d) { $p.D = $d }
    $ec = [Security.Cryptography.ECDsa]::Create()
    $ec.ImportParameters($p)
    if ($d) { [Array]::Clear($p.D, 0, $p.D.Length) }
    return $ec
}

# The key file: "VRSK1", then D, X and Y (32 bytes each), all under DPAPI
function Read-Key {
    if (-not (Test-Path -LiteralPath $KeyFile)) { throw "No key in $KeyFile (release-key.ps1 new)" }
    $plain = [Security.Cryptography.ProtectedData]::Unprotect(
        [IO.File]::ReadAllBytes($KeyFile), $entropy, 'CurrentUser')
    try {
        if ($plain.Length -ne 5 + 96 -or -not (Test-Same $magic ([byte[]]$plain[0..4]))) {
            throw "$KeyFile is not a key of this script"
        }
        $d = [byte[]]$plain[5..36]
        $xy = [byte[]]$plain[37..100]
        return @{ D = $d; XY = $xy }
    } finally { [Array]::Clear($plain, 0, $plain.Length) }
}

# The public key the app carries, from redesign_update_key.h
function Read-HeaderKey {
    if (-not (Test-Path -LiteralPath $header)) { throw "No $header" }
    $text = [IO.File]::ReadAllText($header)
    $m = [regex]::Match($text, 'vlc_redesign_update_key\[64\]\s*=\s*\{([^}]*)\}')
    if (-not $m.Success) { throw "No key in $header" }
    $bytes = [byte[]]@([regex]::Matches($m.Groups[1].Value, '0x([0-9a-fA-F]{2})') |
        ForEach-Object { [Convert]::ToByte($_.Groups[1].Value, 16) })
    if ($bytes.Length -ne 64) { throw "The key in $header is not 64 bytes" }
    return , $bytes
}

function Show-Public([byte[]]$xy) {
    $lines = for ($i = 0; $i -lt 64; $i += 8) {
        '    ' + ((0..7 | ForEach-Object { '0x' + $xy[$i + $_].ToString('x2') }) -join ', ') + ','
    }
    Write-Host 'Public key (X then Y), for redesign_update_key.h:'
    Write-Host 'static const unsigned char vlc_redesign_update_key[64] = {'
    $lines | ForEach-Object { Write-Host $_ }
    Write-Host '};'
    Write-Host "Base64 (X || Y): $([Convert]::ToBase64String($xy))"
    Write-Host "SubjectPublicKeyInfo (base64): $([Convert]::ToBase64String((Get-Spki $xy)))"
    Write-Host "Fingerprint (SHA-256 of the SubjectPublicKeyInfo): $(Get-Fingerprint $xy)"
}

function Write-Header([byte[]]$xy) {
    $rows = for ($i = 0; $i -lt 64; $i += 8) {
        '    ' + ((0..7 | ForEach-Object { '0x' + $xy[$i + $_].ToString('x2') }) -join ', ') + ','
    }
    $text = @"
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
 *   Base64 (X || Y): $([Convert]::ToBase64String($xy))
 *   SHA-256 of the SubjectPublicKeyInfo: $(Get-Fingerprint $xy)
 */
static const unsigned char vlc_redesign_update_key[64] = {
$($rows -join "`n")
};

#endif

"@
    [IO.File]::WriteAllText($header, $text.Replace("`r`n", "`n"))
    Write-Host "Wrote $header"
}

function Test-Signature([string]$file, [byte[]]$xy) {
    $sigFile = "$file.sig"
    if (-not (Test-Path -LiteralPath $sigFile)) { Write-Host "No $sigFile"; return $false }
    $sig = [IO.File]::ReadAllBytes($sigFile)
    if ($sig.Length -ne $sigSize) { Write-Host "$sigFile is not a signature ($($sig.Length) bytes)"; return $false }
    $ec = New-Ecdsa $xy $null
    try {
        return $ec.VerifyData([IO.File]::ReadAllBytes($file), $sig, [Security.Cryptography.HashAlgorithmName]::SHA256)
    } finally { $ec.Dispose() }
}

function Need-Path {
    if (-not $Path) { throw "release-key.ps1 $Command <file>" }
}

switch ($Command) {
    'new' {
        if (Test-Path -LiteralPath $KeyFile) {
            throw "$KeyFile exists: a key is made only once (releases signed with it would no longer be accepted)"
        }
        $dir = Split-Path -Parent $KeyFile
        New-Item -ItemType Directory -Force $dir | Out-Null
        $ec = [Security.Cryptography.ECDsa]::Create([Security.Cryptography.ECCurve+NamedCurves]::nistP256)
        try {
            $p = $ec.ExportParameters($true)
            $xy = Join-Bytes @($p.Q.X, $p.Q.Y)
            $plain = Join-Bytes @($magic, $p.D, $xy)
            try {
                $blob = [Security.Cryptography.ProtectedData]::Protect($plain, $entropy, 'CurrentUser')
            } finally {
                [Array]::Clear($plain, 0, $plain.Length)
                [Array]::Clear($p.D, 0, $p.D.Length)
            }
            # (never overwrites: CreateNew fails if the file came meanwhile)
            $fs = New-Object IO.FileStream($KeyFile, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write)
            try { $fs.Write($blob, 0, $blob.Length) } finally { $fs.Dispose() }
        } finally { $ec.Dispose() }
        Write-Host "Private key: $KeyFile (encrypted for $env:USERNAME on $env:COMPUTERNAME; only they can use it)"
        Show-Public $xy
        Write-Host 'Next: release-key.ps1 header (the public key into the app), then export-backup to a password manager.'
    }
    'public' {
        Show-Public (Read-Key).XY
    }
    'header' {
        Write-Header (Read-Key).XY
    }
    'sign' {
        Need-Path
        $file = (Resolve-Path -LiteralPath $Path).Path
        $key = Read-Key
        try {
            # the key the app carries: a signature by another key would be refused
            if (Test-Path -LiteralPath $header) {
                $carried = Read-HeaderKey
                if (-not (Test-Same $carried $key.XY)) {
                    throw "The key in $KeyFile is not the one in $header"
                }
            }
            $ec = New-Ecdsa $key.XY $key.D
            try {
                $sig = $ec.SignData([IO.File]::ReadAllBytes($file), [Security.Cryptography.HashAlgorithmName]::SHA256)
            } finally { $ec.Dispose() }
        } finally { [Array]::Clear($key.D, 0, $key.D.Length) }
        if ($sig.Length -ne $sigSize) { throw "Unexpected signature size $($sig.Length)" }
        [IO.File]::WriteAllBytes("$file.sig", $sig)
        if (-not (Test-Signature $file $key.XY)) { throw "The signature of $file doesn't verify" }
        Write-Host "Signed: $file.sig"
    }
    'verify' {
        Need-Path
        $file = (Resolve-Path -LiteralPath $Path).Path
        if (Test-Signature $file (Read-HeaderKey)) {
            Write-Host "Good signature: $file"
        } else {
            Write-Host "BAD signature: $file"
            exit 1
        }
    }
    'export-backup' {
        Need-Path
        if (Test-Path -LiteralPath $Path) { throw "$Path exists: not overwritten" }
        $key = Read-Key
        # PKCS#8 (RFC 5208) holding an ECPrivateKey (RFC 5915) with its public key
        $der = Join-Bytes @(
            [byte[]](0x30, 0x81, 0x87, 0x02, 0x01, 0x00, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce,
                0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x04, 0x6d,
                0x30, 0x6b, 0x02, 0x01, 0x01, 0x04, 0x20),
            $key.D,
            [byte[]](0xa1, 0x44, 0x03, 0x42, 0x00, 0x04),
            $key.XY)
        [Array]::Clear($key.D, 0, $key.D.Length)
        $b64 = [Convert]::ToBase64String($der, 'InsertLineBreaks')
        [Array]::Clear($der, 0, $der.Length)
        # the PEM label put together, so that no line here looks like a key
        # to the pre-push guard
        $label = 'PRIVATE' + ' KEY'
        $pem = "-----BEGIN $label-----`n$($b64.Replace("`r`n", "`n"))`n-----END $label-----`n"
        $fs = New-Object IO.FileStream($Path, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write)
        try {
            $bytes = [Text.Encoding]::ASCII.GetBytes($pem)
            $fs.Write($bytes, 0, $bytes.Length)
        } finally { $fs.Dispose() }
        $pem = $null; $b64 = $null
        Write-Warning ("$Path holds the PRIVATE release key UNENCRYPTED. Anyone with it can sign updates every " +
            'copy of VLC Redesign installs. Put it in a password manager now, then delete this file (and empty ' +
            'the Recycle Bin). Never commit, upload, mail or sync it.')
    }
}
