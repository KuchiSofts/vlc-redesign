# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# The Windows on ARM64 installer on ARM64 Windows (the Windows build
# workflow's windows-11-arm runner):
#   1. vlc-redesign-<version>-winarm64-setup.exe installed silently (every
#      component, Live TV too);
#   2. the installed folder holds the package's files, every one of them;
#   3. arm64-smoke.ps1 on the installed copy: every binary ARM64, the plugin
#      cache made (every plugin loaded), the window, quit, a clip played to
#      its end silently (--aout=dummy), no QML warning;
#   4. screenshots (not counted, kept with the logs): the main window, then
#      About (Shift+F1);
#   5. the uninstaller removes the copy.
# Steps 4 types real keys into the window: CI only (-Interactive is refused
# when CI is not set), never on a PC someone uses.
#   pwsh -File arm64-install-test.ps1 -Setup <setup.exe> -Package <package> [-WorkDir <dir>]
# The exit code is the number of failed checks; the results are in
# <WorkDir>\vlc-launch-test.
param(
    [Parameter(Mandatory = $true)][string]$Setup,
    [Parameter(Mandatory = $true)][string]$Package,
    [string]$WorkDir = $env:TEMP,
    [string]$InstallDir = "$env:ProgramFiles\VLC Redesign"
)
$ErrorActionPreference = 'Stop'
$Setup = (Resolve-Path $Setup).Path
$Package = (Resolve-Path $Package).Path
$work = Join-Path $WorkDir 'vlc-launch-test'
New-Item -ItemType Directory -Force $work | Out-Null
$result = Join-Path $work 'install-result.txt'
Set-Content $result "install test of $Setup"
$failures = 0
function Note([string]$text) { Add-Content $result $text; Write-Host $text }
function Fail([string]$text) { $script:failures++; Note "FAIL: $text" }

$os = [Runtime.InteropServices.RuntimeInformation]::OSArchitecture
if ("$os" -ne 'Arm64') { Fail "this is $os Windows: the ARM64 installer is for ARM64 Windows"; exit $failures }
if (-not $env:CI) { Fail 'CI only: it installs for every user and types into the window'; exit $failures }

# 1. Silent install
$log = Join-Path $work 'setup.log'
$p = Start-Process -FilePath $Setup -PassThru -Wait -ArgumentList @(
    '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/NOCANCEL', '/ALLUSERS',
    "/DIR=`"$InstallDir`"", "/LOG=`"$log`"")
if ($p.ExitCode -ne 0) { Fail "setup ended with exit code $($p.ExitCode) (see setup.log)"; exit $failures }
if (-not (Test-Path (Join-Path $InstallDir 'vlc.exe'))) { Fail "no vlc.exe in $InstallDir"; exit $failures }
Note "ok: installed in $InstallDir"
# (an install for every user starts vlc-cache-gen without waiting for it)
Get-Process vlc-cache-gen -ErrorAction SilentlyContinue | ForEach-Object {
    if (-not $_.WaitForExit(180 * 1000)) { Fail 'vlc-cache-gen of the installer did not end within 180 s' }
}

# 2. The same files as the package (the installer adds its uninstaller and
# the plugin cache; the Live TV add-on is a component of its own)
function Get-Files([string]$dir) {
    Get-ChildItem $dir -Recurse -File | ForEach-Object { $_.FullName.Substring($dir.Length + 1) } |
        Where-Object { $_ -notlike 'portable\*' -and $_ -notlike 'unins*' -and $_ -ne 'plugins\plugins.dat' -and $_ -notlike '*.old-*' }
}
$packaged = @(Get-Files $Package)
$installed = @(Get-Files $InstallDir)
$missing = @($packaged | Where-Object { $installed -notcontains $_ })
$extra = @($installed | Where-Object { $packaged -notcontains $_ })
if ($missing) {
    Fail "$($missing.Count) of the package's $($packaged.Count) files not installed"
    $missing | Select-Object -First 20 | ForEach-Object { Note "  $_" }
} else {
    Note "ok: the package's $($packaged.Count) files installed"
}
if ($extra) { Note "note: installed and not in the package: $($extra -join ', ')" }
if (Test-Path (Join-Path $InstallDir 'plugins\misc\libiptv_plugin.dll')) { Note 'ok: Live TV add-on installed' }
else { Fail 'the Live TV add-on was not installed (component "livetv")' }

# 3. The smoke test of the package, on the installed copy
& (Join-Path $PSScriptRoot 'arm64-smoke.ps1') -VlcDir $InstallDir -WorkDir $WorkDir
$failures += $LASTEXITCODE

# 4. Screenshots: the main window, About (Shift+F1, GlobalShortcuts.qml)
Add-Type -AssemblyName System.Drawing, System.Windows.Forms
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Input {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    public static void Key(byte vk, bool up) { keybd_event(vk, 0, up ? 2u : 0u, UIntPtr.Zero); }
}
'@
function Save-Screen([string]$name) {
    $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $g = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
        $bitmap.Save((Join-Path $work "$name.png"), [System.Drawing.Imaging.ImageFormat]::Png)
        Note "screenshot: $name.png"
    } catch {
        Note "note: no screenshot $name ($_)"
    } finally { $g.Dispose(); $bitmap.Dispose() }
}
$vlc = Join-Path $InstallDir 'vlc.exe'
$port = 4213
$process = Start-Process -FilePath $vlc -PassThru -ArgumentList @(
    '--no-qt-privacy-ask', '--aout=dummy', '--extraintf=rc', "--rc-host=127.0.0.1:$port",
    '--verbose=2', '--file-logging', "--logfile=`"$(Join-Path $work 'about.log')`"")
$watch = [Diagnostics.Stopwatch]::StartNew()
while ($watch.Elapsed.TotalSeconds -lt 60 -and -not $process.HasExited -and $process.MainWindowHandle -eq [IntPtr]::Zero) {
    Start-Sleep -Milliseconds 200
    $process.Refresh()
}
if ($process.MainWindowHandle -ne [IntPtr]::Zero) {
    Start-Sleep 4
    # (Alt pressed and released lets this process bring the window forward)
    [Input]::Key(0x12, $false); [Input]::Key(0x12, $true)
    $null = [Input]::SetForegroundWindow($process.MainWindowHandle)
    Start-Sleep 1
    Save-Screen 'main'
    [Input]::Key(0x10, $false); [Input]::Key(0x70, $false); [Input]::Key(0x70, $true); [Input]::Key(0x10, $true)
    Start-Sleep 3
    Save-Screen 'about'
} else {
    Note 'note: no window for the screenshots'
}
try {
    $client = New-Object Net.Sockets.TcpClient('127.0.0.1', $port)
    $writer = New-Object IO.StreamWriter($client.GetStream())
    $writer.Write("quit`n"); $writer.Flush()
    Start-Sleep 1
    $client.Close()
} catch { }
if (-not $process.WaitForExit(20000)) { try { $process.Kill() } catch { } }

# 5. Uninstall
$uninstaller = Join-Path $InstallDir 'unins000.exe'
if (Test-Path $uninstaller) {
    $u = Start-Process -FilePath $uninstaller -PassThru -Wait -ArgumentList '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART'
    Start-Sleep 3
    if ($u.ExitCode -ne 0) { Fail "the uninstaller ended with exit code $($u.ExitCode)" }
    elseif (Test-Path (Join-Path $InstallDir 'vlc.exe')) { Fail 'vlc.exe is still there after the uninstaller' }
    else { Note 'ok: uninstalled' }
} else {
    Fail 'no uninstaller (unins000.exe)'
}

Note "failures: $failures"
exit $failures
