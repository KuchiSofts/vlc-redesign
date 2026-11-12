# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Starts a built VLC Redesign package and plays a clip with it: the interface
# must open its window, quit when asked, play a file to its end, and log no
# QML warning. Run by the Windows build workflow (a runner without a GPU or a
# sound card) and by hand:
#   powershell -ExecutionPolicy Bypass -File launch-test.ps1 -VlcDir <package>
# The results are in <WorkDir>\vlc-launch-test (logs, result.txt); the exit
# code is the number of failed checks.
param(
    [Parameter(Mandatory = $true)][string]$VlcDir,
    [string]$WorkDir = $env:TEMP
)
$ErrorActionPreference = 'Stop'
$VlcDir = (Resolve-Path $VlcDir).Path
$vlc = Join-Path $VlcDir 'vlc.exe'
$work = Join-Path $WorkDir 'vlc-launch-test'
New-Item -ItemType Directory -Force $work | Out-Null
# a profile of its own: never the one of the user, nor the settings of another
# copy ("standalone")
New-Item -ItemType Directory -Force (Join-Path $VlcDir 'portable') | Out-Null
Set-Content (Join-Path $VlcDir 'portable\standalone') ''
$result = Join-Path $work 'result.txt'
Set-Content $result "launch test of $vlc"
$failures = 0
# On GitHub: every line in the run's summary, a failure also as an error on
# the run's page (both visible without signing in; the log isn't)
function Gh-Note([string]$text) {
    if ($env:GITHUB_STEP_SUMMARY) { Add-Content $env:GITHUB_STEP_SUMMARY "- $text" }
}
function Gh-Fail([string]$title, [string]$text) {
    if ($env:GITHUB_ACTIONS) {
        $escaped = $text.Replace('%', '%25').Replace("`r", '%0D').Replace("`n", '%0A')
        Write-Host "::error title=$title::$escaped"
    }
}
function Note([string]$text) { Add-Content $result $text; Write-Host $text; Gh-Note $text }
function Fail([string]$text) { $script:failures++; Note "FAIL: $text"; Gh-Fail 'launch test' $text }

# A clip VLC reads without a decoder: 2 s of 320 x 240 YUV 4:2:0 at 25 fps
# (Y4M), a bar moving across it
$clip = Join-Path $work 'clip.y4m'
$width = 320; $height = 240; $frames = 50
$stream = [IO.File]::Create($clip)
try {
    $header = [Text.Encoding]::ASCII.GetBytes("YUV4MPEG2 W$width H$height F25:1 Ip A1:1 C420jpeg`n")
    $stream.Write($header, 0, $header.Length)
    $mark = [Text.Encoding]::ASCII.GetBytes("FRAME`n")
    $luma = New-Object byte[] ($width * $height)
    $chroma = New-Object byte[] ($width * $height / 4)
    for ($i = 0; $i -lt $chroma.Length; $i++) { $chroma[$i] = 128 }
    for ($f = 0; $f -lt $frames; $f++) {
        $bar = [int]($f * ($width - 32) / $frames)
        for ($y = 0; $y -lt $height; $y++) {
            $row = $y * $width
            for ($x = 0; $x -lt $width; $x++) {
                $luma[$row + $x] = if ($x -ge $bar -and $x -lt $bar + 32) { 235 } else { 16 }
            }
        }
        $stream.Write($mark, 0, $mark.Length)
        $stream.Write($luma, 0, $luma.Length)
        $stream.Write($chroma, 0, $chroma.Length)
        $stream.Write($chroma, 0, $chroma.Length)
    }
} finally {
    $stream.Dispose()
}

function Start-Vlc([string]$name, [string[]]$arguments) {
    $log = Join-Path $work "$name.log"
    Remove-Item $log -ErrorAction SilentlyContinue
    $all = @('--verbose=2', '--file-logging', "--logfile=`"$log`"", '--no-qt-privacy-ask', '--aout=dummy') + $arguments
    $process = Start-Process -FilePath $vlc -ArgumentList $all -PassThru
    # (the handle kept: the exit code stays readable)
    $null = $process.Handle
    return $process
}

# What a process that does not end was doing, before it is ended: the last
# lines of its log, how its threads wait, and a small dump (the stacks of
# its threads and its modules, no memory) beside the crash dumps, which the
# workflow keeps. On CI only: a dump stays on no one's PC.
function Save-Stuck($process, [string]$name) {
    $log = Join-Path $work "$name.log"
    if (Test-Path $log) {
        Note "  the last lines of $name.log:"
        Get-Content $log -Tail 8 | ForEach-Object { Note "    $_" }
    }
    try {
        $process.Refresh()
        $waits = $process.Threads | Group-Object { "$($_.ThreadState) $(if ($_.ThreadState -eq 'Wait') { $_.WaitReason })".Trim() } |
            ForEach-Object { "$($_.Count) $($_.Name)" }
        Note "  threads: $($process.Threads.Count) ($($waits -join ', '))"
    } catch { Note "  threads: not read ($_)" }
    if (-not $env:CI) { return }
    try {
        if (-not ('Stuck.Dump' -as [type])) {
            Add-Type -Namespace Stuck -Name Dump -MemberDefinition @'
[DllImport("dbghelp.dll", SetLastError = true)]
public static extern bool MiniDumpWriteDump(IntPtr process, uint id, Microsoft.Win32.SafeHandles.SafeFileHandle file,
                                            uint type, IntPtr exception, IntPtr stream, IntPtr callback);
'@
        }
        $dumps = Join-Path $work 'dumps'
        New-Item -ItemType Directory -Force $dumps | Out-Null
        $path = Join-Path $dumps "stuck-$name.dmp"
        $file = [IO.File]::Create($path)
        try {
            # MiniDumpWithUnloadedModules | MiniDumpWithThreadInfo
            $done = [Stuck.Dump]::MiniDumpWriteDump($process.Handle, [uint32]$process.Id, $file.SafeFileHandle,
                                                    0x1020, [IntPtr]::Zero, [IntPtr]::Zero, [IntPtr]::Zero)
        } finally { $file.Dispose() }
        if ($done) { Note "  dump: dumps\stuck-$name.dmp ($([int]((Get-Item $path).Length / 1KB)) KB)" }
        else { Note "  dump: not written (error $([Runtime.InteropServices.Marshal]::GetLastWin32Error()))" }
    } catch { Note "  dump: not written ($_)" }
}

function Wait-Exit($process, [int]$seconds, [string]$what, [string]$name) {
    if (-not $process.WaitForExit($seconds * 1000)) {
        Fail "$what did not end within $seconds s"
        if ($name) { Save-Stuck $process $name }
        try { $process.Kill() } catch { }
        return
    }
    if ($process.ExitCode -ne 0) { Fail "$what ended with exit code $($process.ExitCode)" }
    else { Note "ok: $what ended (exit code 0)" }
}

# 1. The interface: its window, then quit over the rc interface
$port = 4212
$process = Start-Vlc 'interface' @('--extraintf=rc', "--rc-host=127.0.0.1:$port")
$watch = [Diagnostics.Stopwatch]::StartNew()
$window = $false
while ($watch.Elapsed.TotalSeconds -lt 60 -and -not $process.HasExited) {
    $process.Refresh()
    if ($process.MainWindowHandle -ne [IntPtr]::Zero) { $window = $true; break }
    Start-Sleep -Milliseconds 100
}
if ($window) { Note ("ok: window after {0:N1} s" -f $watch.Elapsed.TotalSeconds) }
else { Fail 'no window within 60 s' }
Start-Sleep 3
try {
    $client = New-Object Net.Sockets.TcpClient('127.0.0.1', $port)
    $writer = New-Object IO.StreamWriter($client.GetStream())
    $writer.Write("quit`n"); $writer.Flush()
    Start-Sleep 1
    $client.Close()
} catch {
    Fail "rc interface: $_"
}
Wait-Exit $process 20 'the interface' 'interface'

# 1b. The same with a graphics driver that takes long to start: closing must
# not wait for it (the interface once ended only when the driver had started:
# a close that never ended on a runner without a graphics card). A build with
# asserts simulates such a driver (VLC_REDESIGN_TEST_SLOW_WARMUP, in ms); a
# release build ignores the variable and this is one more quit.
$env:VLC_REDESIGN_TEST_SLOW_WARMUP = '30000'
try {
    $port = 4214
    $process = Start-Vlc 'slow-driver' @('--extraintf=rc', "--rc-host=127.0.0.1:$port")
    $watch = [Diagnostics.Stopwatch]::StartNew()
    while ($watch.Elapsed.TotalSeconds -lt 60 -and -not $process.HasExited) {
        $process.Refresh()
        if ($process.MainWindowHandle -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 100
    }
    Start-Sleep 3
    try {
        $client = New-Object Net.Sockets.TcpClient('127.0.0.1', $port)
        $writer = New-Object IO.StreamWriter($client.GetStream())
        $writer.Write("quit`n"); $writer.Flush()
        Start-Sleep 1
        $client.Close()
    } catch {
        Fail "rc interface (slow driver): $_"
    }
    Wait-Exit $process 20 'the interface, its graphics driver still starting,' 'slow-driver'
    $log = Join-Path $work 'slow-driver.log'
    if ((Test-Path $log) -and (Select-String -Path $log -Pattern 'warm-up did not end within' -SimpleMatch -Quiet)) {
        Note 'ok: the slow driver was let go'
    } else {
        Note 'note: no slow driver simulated (a build without asserts, or no Direct3D 11 warm-up here)'
    }
} finally {
    Remove-Item Env:VLC_REDESIGN_TEST_SLOW_WARMUP -ErrorAction SilentlyContinue
}

# 2. Playback: the clip to its end
$process = Start-Vlc 'playback' @('--play-and-exit', "`"$clip`"")
Wait-Exit $process 60 'playback of the clip' 'playback'
$log = Join-Path $work 'playback.log'
foreach ($line in 'Received first picture', 'EOF reached') {
    if ((Test-Path $log) -and (Select-String -Path $log -Pattern $line -SimpleMatch -Quiet)) { Note "ok: $line" }
    else { Fail "the clip did not play: no `"$line`" in the log" }
}

# 3. No QML warning in either run (a warning of a QML file names it: qrc:;
# Qt logs it as "qt generic warning" or "qt warning: (default)", errors too)
foreach ($name in 'interface', 'slow-driver', 'playback') {
    $log = Join-Path $work "$name.log"
    if (-not (Test-Path $log)) { Fail "no log of the $name run"; continue }
    $warnings = @(Select-String -Path $log -Pattern 'qt (generic )?(warning|error|critical).*qrc:')
    if ($warnings.Count -gt 0) {
        Fail "$($warnings.Count) QML warnings in the $name run"
        $warnings | Select-Object -First 20 | ForEach-Object { Note "  $($_.Line)" }
    } else {
        Note "ok: no QML warning in the $name run"
    }
}

Note "failures: $failures"
exit $failures
