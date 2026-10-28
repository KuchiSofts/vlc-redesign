# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# Build a VLC Redesign release for Windows into -Dist:
#   vlc-redesign-<version>-win64.zip        portable package
#   vlc-redesign-<version>-win64-setup.exe  installer (Inno Setup 6.7+)
#   SHA256SUMS.txt
#   SHA256SUMS.txt.sig                     its signature by the release key
#                                            (signing/release-key.ps1)
#   vlc-redesign-<version>-delta-from-<old>.vrdelta  small updates from the
#                                            3 latest releases whose zip is in -Dist
# -Arch aarch64 makes the Windows on ARM64 files instead (same -Dist):
#   vlc-redesign-<version>-winarm64.zip, vlc-redesign-<version>-winarm64-setup.exe,
#   vlc-redesign-<version>-winarm64-delta-from-<old>.vrdelta
# from -Package ~\source\vlc-redesign-release-arm64 (volume vlc-winarm64-release).
# SHA256SUMS.txt lists the files of both architectures of <version> that are
# in -Dist: make the x64 and the ARM64 ones, then sign the last one written.
# The version comes from modules/gui/qt/util/redesign_version.hpp unless
# -Version is given. -Quick only rebuilds what changed; -SkipBuild packages
# the existing -Package folder as it is; -Signed packages a folder whose
# binaries are signed already (the signed package of the Windows build
# workflow, docs/code-signing.md): never built, its binaries never changed.
# Needs Python with bsdiff4 for the small updates (pip install bsdiff4).
# The release key must be on this PC (signing/release-key.ps1): the app takes
# no release from 0.3.2 on without SHA256SUMS.txt.sig. -NoSumsSignature leaves
# it out (the GitHub workflow, which never has the key: SHA256SUMS.txt is
# signed here once the release files are final, docs/code-signing.md).
param(
    [string]$Version,
    [ValidateSet('x86_64', 'aarch64')][string]$Arch = 'x86_64',
    [string]$Package,
    [string]$Dist = "$env:USERPROFILE\source\vlc-redesign-dist",
    [switch]$Quick,
    [switch]$SkipBuild,
    [switch]$Signed,
    [switch]$NoSumsSignature,
    # parallel compile jobs of the build (win64-docker.ps1 -Jobs; 0: every CPU)
    [int]$Jobs = 0
)
if ($Signed) { $SkipBuild = $true }
# The name of the architecture in the file names (docs/MASTER-PLAN.md, 13.5)
# and what tells the small updates apart: the x64 ones keep the name they had
# before ARM64, which the copies already installed look for
$platforms = @{ x86_64 = 'win64'; aarch64 = 'winarm64' }
$platform = $platforms[$Arch]
$deltaInfix = @{ win64 = ''; winarm64 = '-winarm64' }
if (-not $Package) {
    $Package = if ($Arch -eq 'aarch64') { "$env:USERPROFILE\source\vlc-redesign-release-arm64" }
               else { "$env:USERPROFILE\source\vlc-redesign-release" }
}
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..\..").Path

if (-not $Version) {
    $header = "$root\modules\gui\qt\util\redesign_version.hpp"
    if (-not (Test-Path $header)) { throw "Give -Version or create $header" }
    $m = Select-String -Path $header -Pattern 'VLC_REDESIGN_VERSION\s+"([^"]+)"'
    if (-not $m) { throw "No VLC_REDESIGN_VERSION in $header" }
    $Version = $m.Matches[0].Groups[1].Value
}
# x.y.z only: configure and meson (the version resources) and Inno Setup's
# VersionInfoVersion take nothing else, and a bad one must stop the release
# before anything is written to -Dist
if ($Version -notmatch '^\d+\.\d+\.\d+$') { throw "Bad version '$Version' (x.y.z)" }
Write-Host "VLC Redesign $Version"

if (-not $SkipBuild) {
    # named parameters as a hashtable: an array would pass "-Out" as a value
    $buildArgs = @{ Arch = $Arch; Out = $Package; Strip = $true; Jobs = $Jobs
                    Volume = $(if ($Arch -eq 'aarch64') { 'vlc-winarm64-release' } else { 'vlc-win64-release' }) }
    if ($Quick) {
        & "$PSScriptRoot\win64-docker.ps1" @buildArgs -Quick
    } else {
        # -r goes on to build.sh: release mode
        & "$PSScriptRoot\win64-docker.ps1" @buildArgs -r
    }
    if ($LASTEXITCODE) { throw "Build failed ($LASTEXITCODE)" }
}
if (-not (Test-Path "$Package\vlc.exe")) { throw "No package in $Package" }

New-Item -ItemType Directory -Force $Dist | Out-Null
$name = "vlc-redesign-$Version-$platform"
$python = (Get-Command python -ErrorAction SilentlyContinue).Source
if (-not $python) { throw 'Python is needed for the small updates (with pip install bsdiff4)' }

# A binary keeps no build time (PE time stamp 0): the files whose code didn't
# change stay the same from one release to the next, and small updates skip them
if ($Signed) {
    # done before signing (the workflow's sign job): the time stamp is part
    # of what a signature covers. Every binary must carry its signature.
    $checked = @(Get-ChildItem $Package -Recurse -File -Include *.exe, *.dll |
        Where-Object { $_.FullName -notlike "*\portable\*" -and $_.Name -notlike '*.old-*' } |
        ForEach-Object { [pscustomobject]@{ Name = $_.Name; Status = (Get-AuthenticodeSignature -LiteralPath $_.FullName).Status } })
    $bad = @($checked | Where-Object { $_.Status -in 'NotSigned', 'HashMismatch' })
    if ($bad) { throw "Not signed: $(($bad | Select-Object -First 10 | ForEach-Object { "$($_.Name) ($($_.Status))" }) -join ', ')" }
    # (a test certificate: signed, not trusted by Windows)
    $untrusted = @($checked | Where-Object { $_.Status -ne 'Valid' }).Count
    if ($untrusted) { Write-Warning "$untrusted of $($checked.Count) signatures are not trusted by Windows" }
    Write-Host "$($checked.Count) binaries signed"
} else {
    & $python "$PSScriptRoot\release\make-delta.py" normalize $Package
    if ($LASTEXITCODE) { throw "make-delta normalize failed ($LASTEXITCODE)" }
}

# VLC trusts plugins\plugins.dat only if every plugin keeps the time and size
# it had when the cache was made. Zip stores times in steps of 2 s, so give
# every file one even-second time first, then make the cache: an unzipped
# copy then starts without scanning its ~600 plugins. (Zip keeps local
# times: unzipped in another time zone, or across a DST change, the first
# start scans them and saves the cache again, src/modules/bank.c.)
# The working folder of the app's updater, left if the package folder was run:
# never shipped (the zip, the installer and the small updates would carry it)
if (Test-Path "$Package\.update") { Remove-Item "$Package\.update" -Recurse -Force }

$now = Get-Date
$stamp = Get-Date -Year $now.Year -Month $now.Month -Day $now.Day -Hour $now.Hour -Minute $now.Minute `
    -Second ($now.Second - $now.Second % 2) -Millisecond 0
Get-ChildItem $Package -Recurse -File | Where-Object { $_.FullName -notlike "*\portable\*" } |
    ForEach-Object { $_.LastWriteTime = $stamp }
# An ARM64 vlc-cache-gen only runs on ARM64 Windows. Made elsewhere, the
# package has no cache: an unzipped copy (or a per-user install) makes it
# at its first start, which takes a few seconds longer; an install for
# everyone makes it during Setup (vlc-redesign.iss).
Remove-Item "$Package\plugins\plugins.dat" -ErrorAction SilentlyContinue
if ($Arch -eq 'x86_64' -or $env:PROCESSOR_ARCHITECTURE -eq 'ARM64') {
    & "$Package\vlc-cache-gen.exe" "$Package\plugins"
    if ($LASTEXITCODE) { throw "vlc-cache-gen failed ($LASTEXITCODE)" }
    (Get-Item "$Package\plugins\plugins.dat").LastWriteTime = $stamp
} else {
    Write-Warning 'No plugin cache in the ARM64 package (made on an x64 PC): copies make it at their first start'
}

# Portable zip: everything but the local profile, with an empty portable\
# so VLC keeps its data next to vlc.exe, not in %APPDATA%\vlc
$zip = "$Dist\$name.zip"
if (Test-Path $zip) { Remove-Item $zip }
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::Open($zip, 'Create')
try {
    $base = (Resolve-Path $Package).Path.TrimEnd('\') + '\'
    Get-ChildItem $Package -Recurse -File | Where-Object {
        $rel = $_.FullName.Substring($base.Length)
        -not ($rel -like 'portable\*') -and -not ($_.Name -like '*.old-*')
    } | ForEach-Object {
        $rel = $_.FullName.Substring($base.Length).Replace('\', '/')
        [void][System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive, $_.FullName, "$name/$rel", 'Optimal')
    }
    $note = $archive.CreateEntry("$name/portable/README.txt")
    $w = New-Object System.IO.StreamWriter($note.Open())
    $w.Write("VLC Redesign keeps its settings, library and history in this folder`r`n" +
             "instead of %APPDATA%\vlc, so it never touches an installed VLC.`r`n" +
             "`r`n" +
             "A new version unpacked into another folder starts with the settings of`r`n" +
             "the copy you used last: an update keeps them. To keep this copy apart`r`n" +
             "(its own settings, never passed on), put an empty file named standalone`r`n" +
             "in this folder. To start over, use Reset all settings in the settings.`r`n")
    $w.Dispose()
} finally { $archive.Dispose() }

# Installer
$iscc = @("${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe") |
    Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { throw 'Inno Setup 6.7+ is needed (choco install innosetup)' }
$archDefine = @(if ($Arch -eq 'aarch64') { '/DArm64' })
& $iscc /Q "/DAppVersion=$Version" "/DSourceDir=$((Resolve-Path $Package).Path)" "/DOutputDir=$Dist" `
    @archDefine "$PSScriptRoot\installer\vlc-redesign.iss"
if ($LASTEXITCODE) { throw "Installer build failed ($LASTEXITCODE)" }

# The release files of one platform of this version in -Dist: zip, installer,
# small updates (the x64 small updates have no platform in their name)
function Get-ReleaseFiles([string]$p) {
    @("vlc-redesign-$Version-$p.zip", "vlc-redesign-$Version-$p-setup.exe" |
        Where-Object { Test-Path "$Dist\$_" }) +
    @(Get-ChildItem $Dist -Filter "vlc-redesign-$Version$($deltaInfix[$p])-delta-from-*.vrdelta" |
        ForEach-Object { $_.Name })
}

# Small updates from the latest releases, each checked by applying it to
# the old release and comparing every file with this package
& $python "$PSScriptRoot\release\make-delta.py" deltas $Package $Version $Dist --keep 3 --platform $platform
if ($LASTEXITCODE) { throw "make-delta deltas failed ($LASTEXITCODE)" }
$deltas = @(Get-ChildItem $Dist -Filter "vlc-redesign-$Version$($deltaInfix[$platform])-delta-from-*.vrdelta" | ForEach-Object { $_.Name })
foreach ($d in $deltas) {
    $old = $d -replace "^vlc-redesign-$([regex]::Escape($Version))$($deltaInfix[$platform])-delta-from-(.+)\.vrdelta$", '$1'
    $tmp = Join-Path ([IO.Path]::GetTempPath()) "vlc-redesign-delta-check-$old"
    if (Test-Path $tmp) { Remove-Item -Recurse -Force $tmp }
    [System.IO.Compression.ZipFile]::ExtractToDirectory("$Dist\vlc-redesign-$old-$platform.zip", $tmp)
    & $python "$PSScriptRoot\release\apply-delta.py" "$Dist\$d" "$tmp\vlc-redesign-$old-$platform" --check $Package
    $ok = $LASTEXITCODE -eq 0
    Remove-Item -Recurse -Force $tmp
    if (-not $ok) { throw "The delta from $old doesn't give this release" }
}

# One SHA256SUMS.txt for the release: these files and those of the other
# architecture of the same version already in -Dist
$files = @(Get-ReleaseFiles 'win64') + @(Get-ReleaseFiles 'winarm64')
foreach ($f in @("$name.zip", "$name-setup.exe")) {
    if ($f -notin $files) { throw "Missing $f in $Dist" }
}
$sums = foreach ($f in $files) {
    '{0}  {1}' -f (Get-FileHash "$Dist\$f" -Algorithm SHA256).Hash.ToLower(), $f
}
[System.IO.File]::WriteAllText("$Dist\SHA256SUMS.txt", ($sums -join "`n") + "`n")
# (never the signature of an earlier SHA256SUMS.txt)
Remove-Item "$Dist\SHA256SUMS.txt.sig" -ErrorAction SilentlyContinue
if ($NoSumsSignature) {
    Write-Warning 'SHA256SUMS.txt is not signed: sign it before publishing (signing\release-key.ps1 sign <file>)'
} else {
    & "$PSScriptRoot\signing\release-key.ps1" sign "$Dist\SHA256SUMS.txt"
    # checked again with the key the app carries
    & "$PSScriptRoot\signing\release-key.ps1" verify "$Dist\SHA256SUMS.txt"
    if ($LASTEXITCODE) { throw 'The signature of SHA256SUMS.txt does not verify' }
}
Get-ChildItem $Dist -File | Where-Object { $_.Name -in $files -or $_.Name -in 'SHA256SUMS.txt', 'SHA256SUMS.txt.sig' } |
    Select-Object Name, @{n = 'MB'; e = { [math]::Round($_.Length / 1MB, 1) } } | Format-Table -AutoSize
