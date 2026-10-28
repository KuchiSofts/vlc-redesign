#!/bin/bash
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# The launch test of a macOS build (the macOS build workflow; any Mac):
#   1. headless: a generated clip played to its end with the dummy
#      interface, video and audio outputs;
#   2. the Qt interface: the app started, still running after 10 s, then
#      asked to quit over the rc interface; no QML warning in its log;
#   3. the Qt interface playing the clip (reported, not counted: a CI Mac
#      may have no GPU for the video).
# A crash report of the app (~/Library/Logs/DiagnosticReports) fails it.
#
#   extras/juxtopposed/ci/macos-smoke.sh <VLC Redesign.app | vlc-redesign-*.dmg> [work dir]

set -o pipefail
target=${1:?usage: macos-smoke.sh <app or dmg> [work dir]}
work=${2:-${TMPDIR:-/tmp}/vlc-macos-smoke}
rm -rf "$work"
mkdir -p "$work"
work=$(cd "$work" && pwd)
result=$work/result.txt
: > "$result"
failures=0
note() { echo "$*" | tee -a "$result"; }
fail() { failures=$((failures + 1)); note "FAIL: $*"; }

mount=
cleanup() { [ -z "$mount" ] || hdiutil detach "$mount" -quiet -force || true; }
trap cleanup EXIT

case $target in
    *.dmg)
        mount=$work/mount
        mkdir -p "$mount"
        hdiutil attach "$target" -nobrowse -readonly -mountpoint "$mount" -quiet || { echo "cannot open $target"; exit 1; }
        app=$(find "$mount" -maxdepth 1 -name '*.app' | head -n 1)
        [ -n "$app" ] || { echo "no app in $target"; exit 1; }
        # (run from a copy, as from /Applications)
        cp -R "$app" "$work/"
        app=$work/$(basename "$app")
        hdiutil detach "$mount" -quiet -force || true
        mount=
        ;;
    *) app=$target ;;
esac
exe=$app/Contents/MacOS/$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$app/Contents/Info.plist")
[ -x "$exe" ] || { echo "no executable in $app"; exit 1; }
note "launch test of $app ($(uname -m), macOS $(sw_vers -productVersion))"
codesign --verify --deep --strict "$app" && note "ok: signature" || fail "the signature does not verify"

# A clip VLC reads without a decoder: 2 s of 320 x 240 YUV 4:2:0 at 25 fps
# (Y4M), a bar moving across it
clip=$work/clip.y4m
python3 - "$clip" <<'EOF'
import sys
w, h, frames = 320, 240, 50
with open(sys.argv[1], 'wb') as f:
    f.write(b'YUV4MPEG2 W%d H%d F25:1 Ip A1:1 C420jpeg\n' % (w, h))
    chroma = bytes([128]) * (w * h // 4)
    for i in range(frames):
        bar = i * (w - 32) // frames
        row = bytes(235 if bar <= x < bar + 32 else 16 for x in range(w))
        f.write(b'FRAME\n' + row * h + chroma + chroma)
EOF

reports_before=$(ls ~/Library/Logs/DiagnosticReports 2>/dev/null | sort)

# Starts the app in the background: $pid, its log $work/<name>.log
start() {
    local name=$1; shift
    "$exe" --verbose=2 --file-logging --logfile="$work/$name.log" --no-qt-privacy-ask "$@" \
        > "$work/$name.out" 2>&1 &
    pid=$!
}
# Waits for $pid for up to $2 s; its exit code, 124 if it had to be killed
wait_exit() {
    local pid=$1 tenths=$(( $2 * 10 ))
    while [ $tenths -gt 0 ]; do
        if ! kill -0 "$pid" 2>/dev/null; then
            wait "$pid"
            return $?
        fi
        sleep 0.1
        tenths=$((tenths - 1))
    done
    kill "$pid" 2>/dev/null
    sleep 2
    kill -9 "$pid" 2>/dev/null
    wait "$pid" 2>/dev/null
    return 124
}
has() { grep -qF -- "$2" "$work/$1.log" 2>/dev/null; }
qml_warnings() {
    grep -E 'qt (generic )?(warning|error|critical).*qrc:' "$work/$1.log" 2>/dev/null
}

# 1. Headless
start headless --intf=dummy --vout=dummy --aout=dummy --play-and-exit "$clip"
wait_exit $pid 60
code=$?
[ $code = 0 ] && note "ok: headless playback ended (exit code 0)" || fail "headless playback: exit code $code"
has headless 'EOF reached' && note "ok: EOF reached" || fail "headless: the clip did not play to its end"

# 2. The Qt interface
port=4212
start interface --extraintf=rc --rc-host=127.0.0.1:$port --aout=dummy
sleep 10
if kill -0 $pid 2>/dev/null; then
    note "ok: the Qt interface still runs after 10 s"
    # what the window looks like (kept with the logs; not a check: the
    # runner may refuse the screen recording)
    screencapture -x "$work/interface.png" 2>/dev/null || true
else
    wait $pid
    fail "the app ended within 10 s (exit code $?)"
fi
grep -E 'using interface module "qt"' "$work/interface.log" > /dev/null \
    && note "ok: the Qt interface was loaded" || fail "the Qt interface was not loaded"
grep -E 'Qt check failed|cannot start any interface' "$work/interface.log" "$work/interface.out" \
    && fail "Qt did not start"
printf 'quit\n' | nc -w 2 127.0.0.1 $port > /dev/null 2>&1 || fail "rc interface: no answer"
wait_exit $pid 30
code=$?
[ $code = 0 ] && note "ok: the interface quit (exit code 0)" || fail "the interface: exit code $code"
if warnings=$(qml_warnings interface); then
    fail "$(echo "$warnings" | wc -l | tr -d ' ') QML warnings"
    echo "$warnings" | head -n 20 | tee -a "$result"
else
    note "ok: no QML warning"
fi

# 3. The Qt interface playing the clip (not counted)
start playback --aout=dummy --play-and-exit "$clip"
wait_exit $pid 60
code=$?
if [ $code = 0 ] && has playback 'Received first picture'; then
    note "ok: the Qt interface played the clip"
else
    note "note: the Qt interface did not play the clip (exit code $code; a CI Mac may have no GPU)"
fi
qml_warnings playback | head -n 20 | sed 's/^/note: /' | tee -a "$result"

# No crash
reports_after=$(ls ~/Library/Logs/DiagnosticReports 2>/dev/null | sort)
crashes=$(comm -13 <(echo "$reports_before") <(echo "$reports_after") | grep -iE 'VLC|vlc-' || true)
if [ -n "$crashes" ]; then
    fail "crash reports: $crashes"
    mkdir -p "$work/crashes"
    echo "$crashes" | while IFS= read -r c; do
        cp ~/Library/Logs/DiagnosticReports/"$c" "$work/crashes/" 2>/dev/null
    done
else
    note "ok: no crash report"
fi

note "$failures failure(s)"
[ $failures = 0 ]
