#!/bin/bash
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# The update of an installed copy on macOS, as a user meets it (#117): a
# released version installed in /Applications from its disk image, as the
# browser leaves it (quarantined), then updated to a newer release with the
# installed copy's own helper (Contents/Resources/vlc-redesign-update.sh),
# started as the app starts it (redesign_updater.cpp: /bin/sh, its input a
# pipe that stays open).
#
#   macos-update-test.sh <from> <to> [<work folder>] [<helper.sh>]
#       <from>, <to>: versions (0.7.2) or disk images (.dmg)
#       <helper.sh>: stage and apply with this helper instead of the
#       installed copy's (a fix of the helper, before its release)
#
# Every step is timed and its messages kept in <work>/*.log; when the staging
# fails, each of its commands is run again by itself, to name the one that
# fails. Exit code: the number of failures.
set -u

from=${1:?usage: macos-update-test.sh <from> <to> [<work>] [<helper.sh>]}
to=${2:?usage: macos-update-test.sh <from> <to> [<work>] [<helper.sh>]}
work=${3:-${TMPDIR:-/tmp}/vlc-redesign-update-test}
helper=${4:-}
repo=https://github.com/KuchiSofts/vlc-redesign/releases/download
case $(uname -m) in
    arm64) arch=arm64 ;;
    *) arch=x86_64 ;;
esac
failures=0
note() { printf '%s\n' "$*"; }
fail() { printf 'FAILED: %s\n' "$*"; failures=$((failures + 1)); }
seconds() { date +%s; }

rm -rf "$work"
mkdir -p "$work"
note "macOS $(sw_vers -productVersion) ($(sw_vers -buildVersion)), $arch"

# the disk image of a version (downloaded), or the one given
image_of() {
    case $1 in
        *.dmg) printf '%s\n' "$1" ;;
        *)
            file=$work/vlc-redesign-$1-macos-$arch.dmg
            curl -fsSL --retry 3 -o "$file" "$repo/v$1/vlc-redesign-$1-macos-$arch.dmg" || return 1
            printf '%s\n' "$file"
            ;;
    esac
}
old_image=$(image_of "$from") || { fail "the disk image of $from could not be downloaded"; exit $failures; }
new_image=$(image_of "$to") || { fail "the disk image of $to could not be downloaded"; exit $failures; }
note "from: $(basename "$old_image") ($(stat -f %z "$old_image") bytes)"
note "to:   $(basename "$new_image") ($(stat -f %z "$new_image") bytes)"

# 1. the old version in /Applications, as a browser's download leaves it
xattr -w com.apple.quarantine "0083;$(printf '%x' "$(seconds)");Safari;$(uuidgen)" "$old_image"
mount=$work/old
mkdir -p "$mount"
hdiutil attach -nobrowse -readonly -noautoopen -mountpoint "$mount" "$old_image" > "$work/install-attach.log" 2>&1 \
    || { cat "$work/install-attach.log"; fail "the disk image of $from can't be opened"; exit $failures; }
source=$(ls -d "$mount"/*.app | head -n 1)
installed=/Applications/$(basename "$source")
rm -rf "$installed"
ditto "$source" "$installed" 2> "$work/install-copy.log" \
    || { cat "$work/install-copy.log"; fail "$from can't be copied to /Applications"; }
hdiutil detach "$mount" -quiet -force > /dev/null 2>&1
plist=/usr/libexec/PlistBuddy
version_of() { "$plist" -c 'Print :CFBundleShortVersionString' "$1/Contents/Info.plist" 2> /dev/null; }
note "installed: $installed, version $(version_of "$installed")"
note "its attributes: $(xattr "$installed" | tr '\n' ' ')"
codesign -dv "$installed" 2>&1 | grep -E '^(Identifier|Format|Signature|TeamIdentifier)' | sed 's/^/  /'
[ -n "$helper" ] || helper=$installed/Contents/Resources/vlc-redesign-update.sh
[ -f "$helper" ] || { fail "no helper ($helper)"; exit $failures; }
note "helper: $helper"

# 2. the new version staged next to it: the helper under /bin/sh with its
# input a pipe nobody writes to, as QProcess gives it; never more than the
# ten minutes the app waits
started=$(seconds)
# (a named pipe held open by a writer that says nothing: the helper's own
# end is what is waited for, not the writer's)
mkfifo "$work/input"
sleep 900 > "$work/input" &
writer=$!
/bin/sh -x "$helper" stage "$new_image" "$installed" < "$work/input" > "$work/stage.out" 2> "$work/stage.log" &
job=$!
waited=0
while kill -0 $job 2> /dev/null && [ $waited -lt 600 ]; do
    sleep 1
    waited=$((waited + 1))
done
if kill -0 $job 2> /dev/null; then
    fail "the staging still runs after ten minutes (the app says: The update is damaged.)"
    note "its last lines:"
    tail -n 15 "$work/stage.log" | sed 's/^/  /'
    ps -axo pid,etime,command | grep -E 'hdiutil|ditto|codesign|xattr|diskimage' | grep -v grep | sed 's/^/  /'
    pkill -P $job 2> /dev/null
    kill $job 2> /dev/null
    code=timeout
else
    wait $job
    code=$?
fi
kill $writer 2> /dev/null
staged=$(cat "$work/stage.out" 2> /dev/null)
note "staging: exit $code after $(($(seconds) - started)) s, printed \"$staged\""
if [ "$code" = 0 ] && [ -d "$staged" ]; then
    note "ok: the update is staged ($(version_of "$staged"))"
    # (the error output is the app's log: the commands' trace here, and
    # nothing of the shell's own about the watchers it ended)
    if grep -v '^+' "$work/stage.log" | grep -q -e Terminated -e Killed; then
        fail "the helper's error output has the shell's own messages"
        grep -v '^+' "$work/stage.log" | head -n 5 | sed 's/^/  /'
    fi
else
    fail "the update could not be staged"
    note "the helper's trace:"
    sed 's/^/  /' "$work/stage.log" | tail -n 40
    # each command by itself, with what it says
    note "--- each step by itself"
    probe=$work/probe
    mkdir -p "$probe"
    t=$(seconds)
    hdiutil attach -nobrowse -readonly -noautoopen -mountpoint "$probe" "$new_image" < /dev/null > "$work/step-attach.log" 2>&1
    note "hdiutil attach: exit $? after $(($(seconds) - t)) s"
    sed 's/^/  /' "$work/step-attach.log" | tail -n 8
    app=$(ls -d "$probe"/*.app 2> /dev/null | head -n 1)
    note "application in the image: ${app:-none}"
    if [ -n "$app" ]; then
        copy=/Applications/.vlc-redesign-update-probe.app
        rm -rf "$copy"
        t=$(seconds)
        ditto "$app" "$copy" > "$work/step-ditto.log" 2>&1
        note "ditto: exit $? after $(($(seconds) - t)) s"
        sed 's/^/  /' "$work/step-ditto.log" | tail -n 8
        t=$(seconds)
        xattr -dr com.apple.quarantine "$copy" > "$work/step-xattr.log" 2>&1
        note "xattr -dr: exit $? after $(($(seconds) - t)) s"
        sed 's/^/  /' "$work/step-xattr.log" | tail -n 8
        t=$(seconds)
        codesign --verify --deep --strict --verbose=2 "$copy" > "$work/step-codesign.log" 2>&1
        note "codesign --verify --deep --strict: exit $? after $(($(seconds) - t)) s"
        sed 's/^/  /' "$work/step-codesign.log" | tail -n 12
        t=$(seconds)
        codesign --verify --strict --verbose=2 "$copy" > "$work/step-codesign-flat.log" 2>&1
        note "codesign --verify --strict (not deep): exit $? after $(($(seconds) - t)) s"
        sed 's/^/  /' "$work/step-codesign-flat.log" | tail -n 6
        note "identifiers: new $("$plist" -c 'Print :CFBundleIdentifier' "$copy/Contents/Info.plist" 2>&1)," \
             "installed $("$plist" -c 'Print :CFBundleIdentifier' "$installed/Contents/Info.plist" 2>&1)"
        rm -rf "$copy"
    fi
    hdiutil detach "$probe" -quiet -force > /dev/null 2>&1
fi

# 3. swapped in once "VLC" has quit (a process that has), not opened
if [ "$code" = 0 ] && [ -d "$staged" ]; then
    true &
    gone=$!
    wait $gone
    started=$(seconds)
    VLC_REDESIGN_UPDATE_NO_OPEN=1 /bin/sh -x "$helper" apply "$gone" "$staged" "$installed" > "$work/apply.out" 2> "$work/apply.log"
    code=$?
    now=$(version_of "$installed")
    note "apply: exit $code after $(($(seconds) - started)) s, /Applications has version ${now:-none}"
    if [ $code = 0 ] && [ ! -e "$staged" ] && [ ! -e "/Applications/.vlc-redesign-old.app" ] \
            && codesign --verify --deep --strict "$installed" 2> "$work/apply-codesign.log"; then
        note "ok: the update is in place, its signature intact, nothing left beside it"
    else
        fail "the update was not put in place"
        sed 's/^/  /' "$work/apply.log" | tail -n 25
        sed 's/^/  /' "$work/apply-codesign.log" 2> /dev/null | tail -n 8
    fi
    case $to in
        *.dmg) ;;
        *) [ "$now" = "$to" ] || fail "/Applications has version ${now:-none}, not $to" ;;
    esac
fi

# 3b. (#117) the update from the application's zip, the updater's first
# choice from 0.8.1 (no disk image to open): the new image's app zipped as
# the release does (ditto), quarantined as a download, staged next to the
# copy in /Applications by the helper given, its signature checked. Only the
# helper given: the one of a release before 0.8.1 can't unpack a zip.
if [ -n "${4:-}" ]; then
    zipwork=$work/zip
    mkdir -p "$zipwork/image"
    if hdiutil attach -nobrowse -readonly -noautoopen -mountpoint "$zipwork/image" "$new_image" \
            > "$work/zip-attach.log" 2>&1; then
        app=$(ls -d "$zipwork/image"/*.app | head -n 1)
        archive=$zipwork/$(basename "${new_image%.dmg}").app.zip
        ditto -c -k --sequesterRsrc --keepParent "$app" "$archive" 2> "$work/zip-make.log" \
            || fail "the zip of $(basename "$app") could not be made"
        hdiutil detach "$zipwork/image" -quiet -force > /dev/null 2>&1
        xattr -w com.apple.quarantine "0083;$(printf '%x' "$(seconds)");Safari;$(uuidgen)" "$archive"
        target=/Applications/$(basename "$app")
        if [ ! -d "$target" ]; then
            fail "no copy in /Applications to update from the zip"
        else
            t=$(seconds)
            /bin/sh -x "$helper" stage "$archive" "$target" < /dev/null > "$work/zip-stage.out" 2> "$work/zip-stage.log"
            code=$?
            staged=$(cat "$work/zip-stage.out" 2> /dev/null)
            note "staging from the zip: exit $code after $(($(seconds) - t)) s, printed \"$staged\""
            if [ $code = 0 ] && [ -d "$staged" ] && codesign --verify --deep --strict "$staged" 2> "$work/zip-codesign.log"; then
                note "ok: the update from the zip is staged ($(version_of "$staged")), its signature intact"
            else
                fail "the update from the zip could not be staged"
                sed 's/^/  /' "$work/zip-stage.log" | tail -n 30
                sed 's/^/  /' "$work/zip-codesign.log" 2> /dev/null | tail -n 8
            fi
            rm -rf "$staged" /Applications/.vlc-redesign-update.app
        fi
    else
        fail "the disk image of $to can't be opened to zip its application"
        sed 's/^/  /' "$work/zip-attach.log" | tail -n 8
    fi
fi

# 4. a step that never answers, an image whose application can't be read or
# whose contents are never listed, an image that can't be opened (#117: a step blocked on a user's Mac left the
# app on "Preparing the update" for ten minutes, then "damaged"): the helper
# ends by itself, says which step and why. Only the helper given: the one
# of a release before these exit codes has none of them.
if [ -n "${4:-}" ]; then
    # (the installed copy is gone after the swap: any application will do
    # as the one to update, the helper only reads its identifier)
    target=$work/blocked/VLC\ Redesign.app
    mkdir -p "$target/Contents"
    printf '%s\n' '<?xml version="1.0" encoding="UTF-8"?>' \
        '<plist version="1.0"><dict><key>CFBundleIdentifier</key><string>org.kuchisofts.vlcredesign</string></dict></plist>' \
        > "$target/Contents/Info.plist"
    fake=$work/fake
    mkdir -p "$fake"
    # helper_with <what hdiutil does> <expected exit> <expected words> <within seconds>
    helper_with() {
        printf '%s\n' '#!/bin/sh' "$1" > "$fake/hdiutil"
        chmod 755 "$fake/hdiutil"
        t=$(seconds)
        PATH="$fake:$PATH" VLC_REDESIGN_UPDATE_STEP_LIMIT=5 /bin/sh "$helper" stage "$new_image" "$target" \
            < /dev/null > "$work/blocked.out" 2> "$work/blocked.log"
        code=$?
        took=$(($(seconds) - t))
        if [ $code = "$2" ] && grep -q "$3" "$work/blocked.log" && [ $took -le "$4" ]; then
            note "ok: $5: exit $code after $took s, \"$(grep "$3" "$work/blocked.log" | head -n 1)\""
        else
            fail "$5: exit $code after $took s (expected $2 within $4 s, saying \"$3\")"
            sed 's/^/  /' "$work/blocked.log" | tail -n 12
        fi
        [ ! -e "$(dirname "$target")/.vlc-redesign-update.app" ] || fail "$5: a staged copy is left behind"
    }
    helper_with 'case $1 in attach) sleep 120 ;; esac' 7 'attach' 30 "a disk image that never opens"
    helper_with 'case $1 in attach) exit 0 ;; esac' 8 'no application' 30 "an image whose application can't be read"
    # (the first reading of the image's volume held by the system, as for
    # an app that is asked whether it may read a removable volume)
    printf '%s\n' '#!/bin/sh' 'sleep 120' > "$fake/find"
    chmod 755 "$fake/find"
    helper_with 'case $1 in attach) exit 0 ;; esac' 7 'list' 30 "an image whose contents are never listed"
    rm -f "$fake/find"
    helper_with 'case $1 in attach) echo "hdiutil: attach failed - no mountable file systems" >&2; exit 1 ;; esac' \
        2 'no mountable file systems' 30 "an image that can't be opened"
    # (#117) a zip that isn't one: said, nothing left behind
    printf 'not a zip\n' > "$work/broken.app.zip"
    t=$(seconds)
    /bin/sh "$helper" stage "$work/broken.app.zip" "$target" < /dev/null > /dev/null 2> "$work/broken-zip.log"
    code=$?
    took=$(($(seconds) - t))
    if [ $code = 2 ] && grep -q 'archive' "$work/broken-zip.log"; then
        note "ok: a broken zip: exit $code after $took s, \"$(grep 'archive' "$work/broken-zip.log" | head -n 1)\""
    else
        fail "a broken zip: exit $code after $took s (expected 2, saying \"archive\")"
        sed 's/^/  /' "$work/broken-zip.log" | tail -n 12
    fi
    [ ! -e "$(dirname "$target")/.vlc-redesign-update.app" ] || fail "a broken zip: a staged copy is left behind"
fi

rm -rf "$installed" /Applications/.vlc-redesign-update.app /Applications/.vlc-redesign-old.app
note "$failures failure(s)"
exit $failures
