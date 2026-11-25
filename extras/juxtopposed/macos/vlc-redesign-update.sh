#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# The update of VLC Redesign on macOS (modules/gui/qt/util/redesign_updater.cpp),
# in the bundle as Contents/Resources/vlc-redesign-update.sh. The app downloads
# the release's disk image of its architecture and checks it against the
# release's signed SHA256SUMS.txt first; this script does the rest:
#
#   vlc-redesign-update.sh stage <image.dmg | app.zip> <VLC Redesign.app>
#       The application of the image (or of the release's zip of it,
#       vlc-redesign-X.Y.Z-macos-<arch>.app.zip: unpacked by ditto, no disk
#       image to open; #117: on a user's Mac the attach of the image never
#       answered when VLC started it, and did in Terminal), copied next to the running one as
#       <folder>/.vlc-redesign-update.app (the same volume: put in place by a
#       rename), its code signature checked whole (codesign --verify --deep
#       --strict) and its bundle identifier compared with the running one's.
#       Prints the copy's path. Exit codes: 2 the image can't be opened, 3 the
#       copy can't be written, 4 the copy is damaged, 5 it is another app,
#       7 a step did not answer in time, 8 the image is open but no
#       application can be read in it.
#       No step waits for ever (#117: one blocked on a user's Mac left the app
#       on "Preparing the update" for the ten minutes it waited, then said
#       "damaged"): each (the image attached, its contents listed, the copy,
#       the signature's check) has VLC_REDESIGN_UPDATE_STEP_LIMIT seconds
#       (120), and nothing is read from the input. What a step says when it fails goes
#       to the error output with its name, for the app's log.
#
#   vlc-redesign-update.sh apply <pid> <staged.app> <VLC Redesign.app> [<argument>...]
#       Once the process <pid> (the running VLC) has quit: the old application
#       moved aside, the staged one renamed into its place (the old one back
#       if that fails), the old one removed, and the application opened again
#       with the arguments (what was playing, where it was).
#
# Nothing here asks for a password: an application the user can't replace
# (not writable) is never offered the update in place (the app opens the
# release page instead).
set -u

die() { echo "vlc-redesign-update: $2" >&2; exit "$1"; }

# step <name> <command> [<argument>...]: the command, never longer than the
# limit. Returns its exit code, 124 when it was ended for taking too long;
# what it said is in $said (kept for the error output when it fails).
step() {
    name=$1
    shift
    said=
    rm -f "$notes/late"
    "$@" > "$notes/said" 2>&1 &
    job=$!
    # (its own input and outputs: a watcher that outlives the step must not
    # keep the app's pipes open)
    ( sleep "$limit"; : > "$notes/late"; kill -TERM "$job"; sleep 2; kill -KILL "$job" ) < /dev/null > /dev/null 2>&1 &
    watcher=$!
    wait "$job"
    code=$?
    job=
    kill "$watcher" 2>/dev/null
    # (its end taken here, silently: the shell would otherwise write
    # "Terminated: 15" for it on the error output, in the app's log)
    wait "$watcher" 2>/dev/null
    watcher=
    said=$(tail -n 6 "$notes/said" 2>/dev/null)
    if [ -e "$notes/late" ]; then
        echo "vlc-redesign-update: $name: no answer after $limit s" >&2
        return 124
    fi
    [ $code = 0 ] || echo "vlc-redesign-update: $name: exit $code: $said" >&2
    return $code
}

case "${1:-}" in
stage)
    [ $# -eq 3 ] || die 1 "usage: stage <image.dmg> <app>"
    # (nothing is typed here: a tool that asks a question gets no answer at
    # once, not a pipe that stays open)
    exec < /dev/null
    image=$2
    target=${3%/}
    folder=$(dirname "$target")
    staged="$folder/.vlc-redesign-update.app"
    limit=${VLC_REDESIGN_UPDATE_STEP_LIMIT:-120}
    mount=$(mktemp -d "${TMPDIR:-/tmp}/vlc-redesign-update.XXXXXX") || die 3 "no temporary folder"
    notes=$(mktemp -d "${TMPDIR:-/tmp}/vlc-redesign-update.XXXXXX") || die 3 "no temporary folder"
    # (at the end, whatever it is: a step still running ended, and its
    # watcher; a copy that is not whole removed; the image released, within
    # the limit too; a zip's unpacked folder removed)
    attached=
    copying=
    whole=
    job=
    watcher=
    trap 'if [ -n "$job" ]; then kill "$job" 2>/dev/null; fi
          if [ -n "$watcher" ]; then kill "$watcher" 2>/dev/null; fi
          if [ -n "$copying" ] && [ -z "$whole" ]; then rm -rf "$staged"; fi
          if [ -n "$attached" ]; then step detach hdiutil detach "$mount" -quiet -force 2>/dev/null; rmdir "$mount" 2>/dev/null; else rm -rf "$mount"; fi
          rm -rf "$notes"' EXIT
    # (asked to stop by the app, whose own limit passed: the same end, through
    # the lines above; without this the shell ends at once and leaves the
    # image attached and the step's tool running)
    trap 'exit 143' TERM INT HUP
    case $image in
    *.zip)
        step unpack ditto -x -k "$image" "$mount"
        case $? in
            0) ;;
            124) die 7 "the archive was not unpacked in time" ;;
            *) die 2 "the archive can't be opened" ;;
        esac
        ;;
    *)
        attached=yes
        step attach hdiutil attach -nobrowse -readonly -noautoopen -mountpoint "$mount" "$image"
        case $? in
            0) ;;
            124) die 7 "the image did not open in time" ;;
            *) die 2 "the image can't be opened" ;;
        esac
        ;;
    esac
    # The first reading of the image's volume, a step too: a Mac that holds
    # it (macOS counts a disk image as a removable volume, which an app may
    # be asked about or refused) must not hold the helper for ever
    step list find "$mount" -maxdepth 1 -type d -name '*.app'
    [ $? = 124 ] && die 7 "the image's contents were not listed in time"
    source=$(grep '\.app$' "$notes/said" 2>/dev/null | head -n 1)
    # (open, and nothing to read in it: not a damaged update, the checksum
    # of the image was right; what the listing said goes to the log)
    [ -n "$source" ] && [ -d "$source" ] \
        || die 8 "no application can be read in the image (the listing says: $(tail -n 3 "$notes/said" 2>/dev/null | tr '\n' ' '))"
    copying=yes
    rm -rf "$staged"
    step copy ditto "$source" "$staged"
    case $? in
        0) ;;
        124) rm -rf "$staged"; die 7 "the copy to $folder did not end in time" ;;
        *) rm -rf "$staged"; die 3 "the copy can't be written in $folder" ;;
    esac
    # (a file the system marked as downloaded would make Gatekeeper ask again)
    step quarantine xattr -dr com.apple.quarantine "$staged" 2>/dev/null
    step signature codesign --verify --deep --strict "$staged"
    case $? in
        0) ;;
        124) rm -rf "$staged"; die 7 "the check of the copy's signature did not end in time" ;;
        *) rm -rf "$staged"; die 4 "the copy's code signature isn't intact" ;;
    esac
    plist=/usr/libexec/PlistBuddy
    new_id=$("$plist" -c 'Print :CFBundleIdentifier' "$staged/Contents/Info.plist" 2>/dev/null)
    own_id=$("$plist" -c 'Print :CFBundleIdentifier' "$target/Contents/Info.plist" 2>/dev/null)
    [ -n "$new_id" ] && [ "$new_id" = "$own_id" ] \
        || { rm -rf "$staged"; die 5 "the image holds another application ($new_id)"; }
    whole=yes
    echo "$staged"
    ;;
apply)
    [ $# -ge 4 ] || die 1 "usage: apply <pid> <staged.app> <app> [<argument>...]"
    pid=$2
    staged=${3%/}
    target=${4%/}
    shift 4
    # VLC quits within seconds; never wait more than two minutes
    waited=0
    while kill -0 "$pid" 2>/dev/null && [ "$waited" -lt 600 ]; do
        sleep 0.2
        waited=$((waited + 1))
    done
    kill -0 "$pid" 2>/dev/null && die 6 "VLC is still running"
    [ -d "$staged" ] || die 4 "no staged update"
    old="$(dirname "$target")/.vlc-redesign-old.app"
    rm -rf "$old"
    if mv "$target" "$old"; then
        if ! mv "$staged" "$target"; then
            mv "$old" "$target"
            die 3 "the update couldn't be put in place"
        fi
        rm -rf "$old"
    else
        die 3 "the application can't be moved"
    fi
    # (VLC_REDESIGN_UPDATE_NO_OPEN=1: the launch test swaps without opening)
    [ "${VLC_REDESIGN_UPDATE_NO_OPEN:-}" = 1 ] && exit 0
    if [ $# -gt 0 ]; then
        open -n "$target" --args "$@"
    else
        open -n "$target"
    fi
    ;;
*)
    die 1 "usage: vlc-redesign-update.sh stage|apply ..."
    ;;
esac
