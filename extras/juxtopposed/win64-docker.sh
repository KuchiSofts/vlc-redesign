#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# Windows x86_64 build of the redesign; run inside the official image
# registry.videolan.org/vlc-debian-win64-posix (see win64-docker.ps1).
#   /src    repository, read-only
#   /cache  prebuilt contrib archive ($CONTRIB_ARCHIVE)
#   /build  persistent volume: source copy, tools, contribs, objects
#   /out    portable package destination on the host
set -eu
# STRIP=1: strip debug information from the packaged binaries (release zip)
jobs=${JOBS:-$(nproc)}
triplet=x86_64-w64-mingw32

# Build inside the volume: the host mount is too slow for compiling and
# bootstrap writes into the source tree. Only files git knows about
# (tracked, or new and not ignored) are synced, so build products in the
# volume survive. Files are compared by content and changed ones get the
# time of the copy: keeping the host mtime would leave a file edited during
# a previous build older than the outputs made from its former content.
mkdir -p /build/vlc
git -C /src -c safe.directory='*' ls-files -z --cached --others --exclude-standard \
    > /build/files.new
rsync -rl --checksum --ignore-missing-args --from0 --files-from=/build/files.new /src/ /build/vlc/
if [ -f /build/files.old ]; then
    tr '\0' '\n' < /build/files.old | sort > /build/files.old.txt
    tr '\0' '\n' < /build/files.new | sort > /build/files.new.txt
    comm -23 /build/files.old.txt /build/files.new.txt \
        | (cd /build/vlc && xargs -r -d '\n' rm -f --)
fi
mv /build/files.new /build/files.old
cd /build/vlc

strip_package() {
    [ "${STRIP:-}" = 1 ] || return 0
    find /build/package -type f \( -name '*.dll' -o -name '*.exe' \)         -exec $triplet-strip --strip-unneeded {} +
}

# The program that applies an update once VLC has quit (see
# extras/juxtopposed/updater); no PE time stamp, so an update leaves it alone
# when it didn't change.
build_updater() {
    $triplet-gcc -O2 -municode -mwindows -static -Wl,--no-insert-timestamp         -o /build/package/vlc-redesign-update.exe         /src/extras/juxtopposed/updater/vlc-redesign-update.c -ladvapi32 -lshell32
}

# The prebuilt libupnp never returns from its HTTP requests (pupnp renames
# the MinGW string functions into ones calling themselves, see
# contrib/src/upnp): build it from source with the fix, once per contrib
# prefix (a prebuilt archive brings the old one back), and link the UPnP
# plugin again.
upnp_from_source() {
    stamp=contrib/$triplet/.upnp-from-source
    sum=$(cat contrib/src/upnp/rules.mak contrib/src/upnp/*.patch | sha1sum | cut -d' ' -f1)
    [ "$(cat "$stamp" 2>/dev/null)" = "$sum" ] && return 1
    # (set -e does not apply in a function called as a condition)
    (cd contrib/contrib-win64 && rm -f .upnp && make .upnp) || exit 1
    echo "$sum" > "$stamp"
    rm -f win64/modules/libupnp_plugin.la
}

# The copy has no .git: src/Makefile then reads the version from revision.txt.
git -C /src -c safe.directory='*' describe --tags --long --match '?.*.*' --always \
    > /build/revision.txt
cmp -s /build/revision.txt src/revision.txt || cp /build/revision.txt src/revision.txt

archive=${CONTRIB_ARCHIVE:?prebuilt contrib archive of extras/ci/get-contrib-sha.sh win64}
if [ ! -s "$archive" ]; then
    echo "Missing $archive" >&2
    exit 1
fi
mkdir -p contrib/contrib-win64
latest=contrib/contrib-win64/vlc-contrib-$triplet-latest.tar.zst
if ! cmp -s "$archive" "$latest"; then
    cp "$archive" "$latest"
    rm -rf contrib/$triplet
fi

# QUICK=1: the tree is configured already, only rebuild what changed. A full
# build.sh run bootstraps and configures again, which takes minutes.
if [ "${QUICK:-}" = 1 ] && [ -f win64/Makefile ]; then
    export PATH="/build/vlc/extras/tools/build/bin:$PATH"
    upnp_from_source || true
    make -C win64 -j"$jobs"
    make -C win64 package-win-common
    build_updater
    strip_package
    rsync -rt --omit-dir-times --delete --checksum --exclude=/portable --exclude='*.old-*' \
        /build/package/ /out/
    echo "Package ready: $(du -sh /out | cut -f1)"
    exit 0
fi

# The prebuilt srt, x265 and gme pkg-config files list -lgcc_s, so their
# plugins would import libgcc_s_seh-1.dll, which the plugin loader never
# finds (it only searches System32). Resolve -lgcc_s to the static unwinder,
# as -static-libgcc does; this -L comes before the contrib one.
mkdir -p /build/static-libgcc
cp "$(x86_64-w64-mingw32-gcc -print-file-name=libgcc_eh.a)" /build/static-libgcc/libgcc_s.a
export LDFLAGS="${LDFLAGS:-} -static-libgcc -L/build/static-libgcc"
export JOBS="$jobs"
rm -rf /build/package
# Fail instead of silently rebuilding every contrib from source.
sed 's/|| PREBUILT_FAILED=yes/|| exit 1/' extras/package/win32/build.sh \
    > extras/package/win32/build-prebuilt-only.sh
# In release mode (-r) VLC's packaging also tries package-win32-zip, which
# expects its own folder layout and fails with -o; the package in
# /build/package is complete by then (it was removed above, so a failure
# before the install leaves it empty and the checks below catch it).
sh extras/package/win32/build-prebuilt-only.sh -a x86_64 -p -l -o /build/package "$@" \
    || echo "build.sh failed after packaging (expected in release mode)" >&2
rm -f extras/package/win32/build-prebuilt-only.sh

test -s /build/package/vlc.exe
test -s /build/package/plugins/gui/libqt_plugin.dll
if upnp_from_source; then
    make -C win64 -j"$jobs"
    make -C win64 package-win-common
fi
build_updater
strip_package
# The host mount refuses owner/group/permission and directory time changes.
# portable/ holds the test profile (VLC uses it instead of %APPDATA%\vlc).
# *.old-* are DLLs win64-docker.ps1 moved aside because a dead VLC maps them.
rsync -rt --omit-dir-times --delete --checksum --exclude=/portable --exclude='*.old-*' \
    /build/package/ /out/
echo "Package ready: $(du -sh /out | cut -f1)"
