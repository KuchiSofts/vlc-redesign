#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# The update of VLC Redesign on macOS (modules/gui/qt/util/redesign_updater.cpp),
# in the bundle as Contents/Resources/vlc-redesign-update.sh. The app downloads
# the release's disk image of its architecture and checks it against the
# release's signed SHA256SUMS.txt first; this script does the rest:
#
#   vlc-redesign-update.sh stage <image.dmg> <VLC Redesign.app>
#       The application of the image, copied next to the running one as
#       <folder>/.vlc-redesign-update.app (the same volume: put in place by a
#       rename), its code signature checked whole (codesign --verify --deep
#       --strict) and its bundle identifier compared with the running one's.
#       Prints the copy's path. Exit codes: 2 the image can't be opened, 3 the
#       copy can't be written, 4 the copy is damaged, 5 it is another app.
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

case "${1:-}" in
stage)
    [ $# -eq 3 ] || die 1 "usage: stage <image.dmg> <app>"
    image=$2
    target=${3%/}
    folder=$(dirname "$target")
    staged="$folder/.vlc-redesign-update.app"
    mount=$(mktemp -d "${TMPDIR:-/tmp}/vlc-redesign-update.XXXXXX") || die 3 "no temporary folder"
    trap 'hdiutil detach "$mount" -quiet -force >/dev/null 2>&1; rmdir "$mount" 2>/dev/null' EXIT
    hdiutil attach -nobrowse -readonly -noautoopen -mountpoint "$mount" "$image" >/dev/null 2>&1 \
        || die 2 "the image can't be opened"
    source=
    for app in "$mount"/*.app; do
        [ -d "$app" ] && { source=$app; break; }
    done
    [ -n "$source" ] || die 4 "no application in the image"
    rm -rf "$staged"
    ditto "$source" "$staged" 2>/dev/null || { rm -rf "$staged"; die 3 "the copy can't be written in $folder"; }
    # (a file the system marked as downloaded would make Gatekeeper ask again)
    xattr -dr com.apple.quarantine "$staged" 2>/dev/null
    codesign --verify --deep --strict "$staged" >/dev/null 2>&1 \
        || { rm -rf "$staged"; die 4 "the copy's code signature isn't intact"; }
    plist=/usr/libexec/PlistBuddy
    new_id=$("$plist" -c 'Print :CFBundleIdentifier' "$staged/Contents/Info.plist" 2>/dev/null)
    own_id=$("$plist" -c 'Print :CFBundleIdentifier' "$target/Contents/Info.plist" 2>/dev/null)
    [ -n "$new_id" ] && [ "$new_id" = "$own_id" ] \
        || { rm -rf "$staged"; die 5 "the image holds another application ($new_id)"; }
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
