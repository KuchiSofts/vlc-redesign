#!/bin/bash
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# The launch test of a macOS build (the macOS build workflow; any Mac):
#   0. the bundle: every Mach-O signed, the icon, the translations, the Live
#      TV add-on (loaded by VLC: --list), the About badge of this Mac's
#      processor ("macOS Apple Silicon" / "macOS Intel"), the designer's
#      credit (CREDITS.txt, Juxtopposed);
#   1. headless: a generated clip played to its end with the dummy
#      interface, video and audio outputs;
#   2. the Qt interface: the app started, still running after 10 s, the
#      unified title bar on its window (the traffic lights in the top bar),
#      then asked to quit over the rc interface; no QML warning in its log;
#   3. the Qt interface playing the clip (reported, not counted: a CI Mac
#      may have no GPU for the video);
#   4. screenshots, not counted (kept with the logs): the main window in
#      the light and the dark appearance of macOS (the System theme follows
#      it), and About opened from the application menu (System Events: the
#      runner may refuse the accessibility).
# A crash report of the app (~/Library/Logs/DiagnosticReports) fails it.
# NO_IPTV=1: a build made without the Live TV add-on (macos-build.sh -t).
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

# 0. The bundle
contents=$app/Contents
plist() { /usr/libexec/PlistBuddy -c "Print :$1" "$contents/Info.plist" 2>/dev/null; }
macho() { file -b "$1" | grep -q 'Mach-O'; }
# every Mach-O signed on its own (a plugin, a Qt plugin, a helper)
machos=0
unsigned=
while IFS= read -r -d '' f; do
    macho "$f" || continue
    machos=$((machos + 1))
    codesign --verify --strict "$f" 2>/dev/null || unsigned="$unsigned ${f#"$contents"/}"
done < <(find "$contents" -type f \( -perm -u+x -o -name '*.dylib' \) ! -path '*/_CodeSignature/*' -print0)
if [ -z "$unsigned" ] && [ "$machos" -ge 300 ]; then
    note "ok: the $machos binaries are signed"
else
    fail "$machos binaries, not signed:${unsigned:- (too few binaries)}"
fi
# the icon (the redesign's logo, AppIcon.svg)
icon=$(plist CFBundleIconFile)
icon=${icon%.icns}
if [ -n "$icon" ] && [ "$(stat -f %z "$contents/Resources/$icon.icns" 2>/dev/null || echo 0)" -gt 20000 ]; then
    note "ok: icon $icon.icns ($(( $(stat -f %z "$contents/Resources/$icon.icns") / 1024 )) KB)"
else
    fail "no app icon (CFBundleIconFile \"$icon\")"
fi
note "bundle: $(plist CFBundleIdentifier) $(plist CFBundleShortVersionString), macOS $(plist LSMinimumSystemVersion) or later"
# the translations (po/, as in the Windows package)
translations=$(find "$contents/Resources/share/locale" -name 'vlc.mo' 2>/dev/null | wc -l | tr -d ' ')
[ "$translations" -ge 20 ] && note "ok: $translations translations" || fail "only $translations translations in Resources/share/locale"
# the About badge of this processor, in the interface (a UTF-16 string of Qt)
case $(uname -m) in
    arm64) badge="macOS Apple Silicon"; other="macOS Intel" ;;
    *) badge="macOS Intel"; other="macOS Apple Silicon" ;;
esac
has_text() {
    python3 - "$1" "$2" <<'EOF'
import sys
data = open(sys.argv[1], 'rb').read()
text = sys.argv[2]
sys.exit(0 if text.encode('utf-16-le') in data or text.encode() in data else 1)
EOF
}
qtplugin=$contents/Frameworks/plugins/libqt_plugin.dylib
if has_text "$qtplugin" "$badge" && ! has_text "$qtplugin" "$other"; then
    note "ok: About badge \"$badge\""
else
    fail "the About badge is not \"$badge\" alone"
fi
# the designer's credit (CC BY 4.0), with the app and in About
if grep -q Juxtopposed "$contents/Resources/CREDITS.txt" 2>/dev/null; then
    note "ok: designer credited (CREDITS.txt)"
else
    fail "no Resources/CREDITS.txt naming Juxtopposed"
fi
[ -f "$contents/Resources/licenses/Inter-OFL.txt" ] || fail "no licence of the Inter font (Resources/licenses)"
# the Live TV (IPTV) add-on: there, and a plugin VLC loads
if [ -z "$NO_IPTV" ]; then
    if [ -f "$contents/Frameworks/plugins/libiptv_plugin.dylib" ]; then
        if "$exe" --intf=dummy --no-qt-privacy-ask --list 2>/dev/null | grep -qwE '^ *iptv'; then
            note "ok: Live TV add-on (libiptv_plugin.dylib) loaded"
        else
            fail "the Live TV add-on is in the app but VLC does not list the iptv module"
        fi
    else
        fail "no Live TV add-on (Frameworks/plugins/libiptv_plugin.dylib)"
    fi
elif [ -f "$contents/Frameworks/plugins/libiptv_plugin.dylib" ]; then
    fail "NO_IPTV: the Live TV add-on is in the app"
else
    note "ok: no Live TV add-on (NO_IPTV)"
fi

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
# the unified title bar (maininterface/macos_titlebar.cpp): the traffic
# lights in the redesign's top bar, the only bar of the window
if line=$(grep -m 1 'unified title bar:' "$work/interface.log"); then
    note "ok: ${line#*: }"
else
    fail "no unified title bar on the window (the system's title bar above the redesign's)"
fi
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

# 4. Screenshots (not counted): the main window in the light and the dark
# appearance (the System theme follows macOS), then About from the
# application menu
appearance=$(defaults read -g AppleInterfaceStyle 2>/dev/null || true)
set_appearance() {
    if [ "$1" = dark ]; then defaults write -g AppleInterfaceStyle Dark
    else defaults delete -g AppleInterfaceStyle 2>/dev/null || true; fi
}
app_name=$(basename "$exe")
for look in light dark; do
    set_appearance $look
    start "main-$look" --extraintf=rc --rc-host=127.0.0.1:$port --aout=dummy
    sleep 8
    if kill -0 $pid 2>/dev/null; then
        osascript -e "tell application \"System Events\" to set frontmost of (first process whose unix id is $pid) to true" \
            > /dev/null 2>&1 || true
        sleep 1
        screencapture -x "$work/main-$look.png" 2>/dev/null && note "screenshot: main-$look.png" \
            || note "note: no screenshot of the $look window (screen recording refused)"
        if [ $look = dark ]; then
            # About: the first item of the application menu (qt.cpp)
            if osascript -e "tell application \"System Events\" to tell (first process whose unix id is $pid) to click menu item 1 of menu 1 of menu bar item 2 of menu bar 1" \
                    > "$work/about.out" 2>&1; then
                sleep 3
                screencapture -x "$work/about.png" 2>/dev/null && note "screenshot: about.png"
            else
                note "note: About not opened (System Events: $(head -n 1 "$work/about.out"))"
            fi
        fi
        printf 'quit\n' | nc -w 2 127.0.0.1 $port > /dev/null 2>&1 || true
        wait_exit $pid 30 > /dev/null || true
    else
        note "note: the $look run ended early"
    fi
done
# the Mac's own appearance again
if [ "$appearance" = Dark ]; then set_appearance dark; else set_appearance light; fi
note "note: $app_name screenshots in $work"

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
