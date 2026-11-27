#!/bin/bash
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# The update of an installed copy on macOS through the app itself (#117).
# macos-update-test.sh runs the update helper from a shell; #117 hung in the
# app, which never saw its helper end (VLC blocks SIGCHLD in every thread).
# Here the app of this build, installed in a folder of its own as from its
# disk image, takes itself for an older version (VLC_REDESIGN_UPDATE_TEST_AS)
# and presses the check, Update now and Restart now by itself
# (VLC_REDESIGN_UPDATE_TEST_AUTO=1). Its own updater then downloads the latest
# release, checks the release key's signature of its checksums, stages it
# with its helper (the child process VLC waits for) and swaps it in when VLC
# quits; the helper opens the new copy. It passes when the installed copy is
# the latest release afterwards.
#
#   macos-app-update-test.sh <dmg or app> [<work folder>] [<version pretended>]
#
# Exit code: the number of failures. Its log: <work>/update.log.

set -u
target=${1:?usage: macos-app-update-test.sh <dmg or app> [work folder] [version pretended]}
work=${2:-${TMPDIR:-/tmp}/vlc-app-update-test}
as=${3:-0.8.6}
rm -rf "$work"
mkdir -p "$work/apps" "$work/home"
work=$(cd "$work" && pwd)
failures=0
note() { printf '%s\n' "$*" | tee -a "$work/result.txt"; }
fail() { failures=$((failures + 1)); note "FAILED: $*"; }
plist=/usr/libexec/PlistBuddy
version_of() { "$plist" -c 'Print :CFBundleShortVersionString' "$1/Contents/Info.plist" 2> /dev/null; }

note "macOS $(sw_vers -productVersion) ($(sw_vers -buildVersion)), $(uname -m)"
# (GH_TOKEN: the workflow's token. The shared addresses of the CI's machines soon reach GitHub's limit for
# anonymous API requests; the app takes it as VLC_REDESIGN_UPDATE_TEST_TOKEN, for its requests to GitHub's API only)
auth=()
[ -n "${GH_TOKEN:-}" ] && auth=(-H "Authorization: Bearer $GH_TOKEN")
# (three tries: a runner's network can drop for a moment, its names not found)
latest=
for try in 1 2 3; do
    latest=$(curl -fsSL --retry 2 ${auth[@]+"${auth[@]}"} https://api.github.com/repos/KuchiSofts/vlc-redesign/releases/latest \
             | python3 -c 'import json, sys; print(json.load(sys.stdin)["tag_name"].lstrip("v"))' 2> /dev/null) && [ -n "$latest" ] && break
    latest=
    sleep $((try * 15))
done
[ -n "$latest" ] || { fail "the latest release could not be read"; exit "$failures"; }
note "the latest release: $latest; this build takes itself for $as"

# installed as from its disk image: a copy in a folder the user can write
case $target in
    *.dmg)
        mount=$work/mount
        mkdir -p "$mount"
        hdiutil attach "$target" -nobrowse -readonly -mountpoint "$mount" -quiet \
            || { fail "the disk image could not be opened"; exit "$failures"; }
        app=$(find "$mount" -maxdepth 1 -name '*.app' | head -n 1)
        cp -R "$app" "$work/apps/"
        hdiutil detach "$mount" -quiet -force || true ;;
    *) cp -R "$target" "$work/apps/" ;;
esac
installed=$(find "$work/apps" -maxdepth 1 -name '*.app' | head -n 1)
[ -n "$installed" ] || { fail "no app to install"; exit "$failures"; }
exe=$installed/Contents/MacOS/$("$plist" -c 'Print :CFBundleExecutable' "$installed/Contents/Info.plist")
note "installed: $installed, version $(version_of "$installed")"

# the app, as a user starts it, in a profile of its own
start=$(date +%s)
VLC_REDESIGN_UPDATE_TEST_AS=$as VLC_REDESIGN_UPDATE_TEST_AUTO=1 VLC_REDESIGN_UPDATE_TEST_TOKEN=${GH_TOKEN:-} \
    HOME=$work/home \
    "$exe" --verbose=2 --file-logging --logfile="$work/update.log" --no-qt-privacy-ask --aout=dummy \
    > "$work/update.out" 2>&1 &
pid=$!

# up to 15 minutes for the installed copy to be replaced by the latest release. Its program file is a new one
# then (another inode): the version alone is not enough, since a build of the released version (on main after a
# release) reads as the latest from the start
inode_of() { stat -f %i "$1" 2> /dev/null; }
original=$(inode_of "$exe")
updated=
while [ $(( $(date +%s) - start )) -lt 900 ]; do
    sleep 10
    if [ "$(inode_of "$exe")" != "$original" ] && [ "$(version_of "$installed")" = "$latest" ]; then
        updated=$(( $(date +%s) - start ))
        break
    fi
done
if [ -n "$updated" ]; then
    note "ok: the installed copy was replaced by $latest after ${updated} s (the app's own updater, its helper and the restart)"
else
    fail "the installed copy was not replaced after 15 min (version $(version_of "$installed"))"
fi
# (the app quits for the swap; a few seconds for it to be gone)
for _ in 1 2 3 4 5 6; do kill -0 "$pid" 2> /dev/null || break; sleep 5; done
kill -0 "$pid" 2> /dev/null && fail "the app that updated itself is still running (it should have quit for the update)"
grep -E 'update (test|check|to )|helper' "$work/update.log" 2> /dev/null | sed -n '1,40p' | tee -a "$work/result.txt"

# the copies opened by the test: closed
pkill -f "$work/apps/" 2> /dev/null || true
sleep 2
pkill -9 -f "$work/apps/" 2> /dev/null || true
exit "$failures"
