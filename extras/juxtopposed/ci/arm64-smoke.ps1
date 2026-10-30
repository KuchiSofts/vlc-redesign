# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Smoke test of a Windows on ARM64 package (win64-docker.ps1 -Arch aarch64):
#   1. every .exe and .dll is an ARM64 binary (PE machine 0xAA64): no x64
#      file that Windows would run emulated, no ARM64EC one
#   2. on ARM64 Windows: vlc-cache-gen makes the plugin cache (it loads every
#      plugin, so a plugin that can't load on ARM64 shows), then
#      launch-test.ps1: the window, quit, a clip played to its end, no QML
#      warning (an ARM64 vlc.exe runs natively: never emulated)
# Run by the Windows build workflow on a windows-11-arm runner, and by hand:
#   powershell -ExecutionPolicy Bypass -File arm64-smoke.ps1 -VlcDir <package>
# -CheckOnly does step 1 alone (any PC: the package is only read). The exit
# code is the number of failed checks; the results are in
# <WorkDir>\vlc-launch-test.
param(
    [Parameter(Mandatory = $true)][string]$VlcDir,
    [string]$WorkDir = $env:TEMP,
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
$VlcDir = (Resolve-Path $VlcDir).Path
$failures = 0
function Note([string]$text) { Write-Host $text }
function Fail([string]$text) { $script:failures++; Write-Host "FAIL: $text" }

# 1. The PE machine of every binary
$machines = @{ 0x8664 = 'x64'; 0xAA64 = 'ARM64'; 0xA641 = 'ARM64EC'; 0x14C = 'x86'; 0x1C4 = 'ARM' }
function Get-PeMachine([string]$path) {
    $stream = [IO.File]::OpenRead($path)
    try {
        $reader = New-Object IO.BinaryReader($stream)
        if ($reader.ReadUInt16() -ne 0x5A4D) { return $null }
        $stream.Position = 0x3C
        $stream.Position = $reader.ReadInt32()
        if ($reader.ReadUInt32() -ne 0x4550) { return $null }
        return [int]$reader.ReadUInt16()
    } finally {
        $stream.Dispose()
    }
}
$binaries = @(Get-ChildItem $VlcDir -Recurse -File -Include *.exe, *.dll |
    Where-Object { $_.FullName -notlike (Join-Path $VlcDir 'portable*') -and $_.Name -notlike '*.old-*' -and
                   # Inno Setup's uninstaller (installed copies) is always x86
                   $_.Name -notlike 'unins*.exe' })
if ($binaries.Count -lt 300) { Fail "only $($binaries.Count) binaries in $VlcDir" }
$wrong = @()
foreach ($b in $binaries) {
    $m = Get-PeMachine $b.FullName
    if ($m -ne 0xAA64) {
        $what = if ($null -eq $m) { 'not a PE file' } elseif ($machines.ContainsKey($m)) { $machines[$m] } else { '0x{0:X4}' -f $m }
        $wrong += "$($b.FullName.Substring($VlcDir.Length + 1)) ($what)"
    }
}
if ($wrong) {
    Fail "$($wrong.Count) of $($binaries.Count) binaries are not ARM64"
    $wrong | Select-Object -First 20 | ForEach-Object { Note "  $_" }
} else {
    Note "ok: the $($binaries.Count) binaries are ARM64 (machine 0xAA64)"
}
foreach ($f in 'vlc.exe', 'libvlc.dll', 'libvlccore.dll', 'vlc-cache-gen.exe', 'vlc-redesign-update.exe',
               'plugins/gui/libqt_plugin.dll', 'plugins/video_output/libdirect3d11_plugin.dll') {
    # ("/": the workflow also runs this part on Linux)
    if (-not (Test-Path (Join-Path $VlcDir $f))) { Fail "not in the package: $f" }
}
if ($CheckOnly) {
    Note "failures: $failures"
    exit $failures
}

# 2. On ARM64 Windows only
$os = [Runtime.InteropServices.RuntimeInformation]::OSArchitecture
if ("$os" -ne 'Arm64') {
    Fail "this is $os Windows: the package runs on ARM64 Windows (use -CheckOnly here)"
    exit $failures
}

# The plugin cache: never in a package made on an x64 PC (make-release.ps1)
Remove-Item (Join-Path $VlcDir 'plugins\plugins.dat') -ErrorAction SilentlyContinue
$gen = Start-Process -FilePath (Join-Path $VlcDir 'vlc-cache-gen.exe') -ArgumentList "`"$(Join-Path $VlcDir 'plugins')`"" `
    -PassThru -NoNewWindow
$null = $gen.Handle
if (-not $gen.WaitForExit(180 * 1000)) {
    Fail 'vlc-cache-gen did not end within 180 s'
    try { $gen.Kill() } catch { }
} elseif ($gen.ExitCode -ne 0) {
    Fail "vlc-cache-gen ended with exit code $($gen.ExitCode)"
} elseif (-not (Test-Path (Join-Path $VlcDir 'plugins\plugins.dat'))) {
    Fail 'vlc-cache-gen made no plugins.dat'
} else {
    Note "ok: plugin cache ($([math]::Round((Get-Item (Join-Path $VlcDir 'plugins\plugins.dat')).Length / 1KB)) KB)"
}

# The launch test of the x64 package, on this one
& (Join-Path $PSScriptRoot 'launch-test.ps1') -VlcDir $VlcDir -WorkDir $WorkDir
$failures += $LASTEXITCODE
Note "failures: $failures"
exit $failures
