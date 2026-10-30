#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# Windows build of the redesign; run inside the official image of
# extras/ci/gitlab-ci.yml (see win64-docker.ps1):
#   ARCH=x86_64   vlc-debian-win64-posix (GCC), as the win64 job
#   ARCH=aarch64  vlc-debian-llvm-ucrt (llvm-mingw), as the win64-arm-llvm
#                 job: UCRT and the Windows 10 API (-u -S 0x0A000006), as
#                 the prebuilt contribs of that job are built
#   /src    repository, read-only
#   /cache  prebuilt contrib archive ($CONTRIB_ARCHIVE)
#   /build  persistent volume: source copy, tools, contribs, objects
#   /out    portable package destination on the host
set -eu
# STRIP=1: strip debug information from the packaged binaries (release zip)
jobs=${JOBS:-$(nproc)}
arch=${ARCH:-x86_64}
case $arch in
    x86_64)
        triplet=x86_64-w64-mingw32
        builddir=win64
        contribdir=contrib/contrib-win64
        prefix=contrib/$triplet
        archflags= ;;
    aarch64)
        triplet=aarch64-w64-mingw32
        # (the names build.sh gives them with -u)
        builddir=winarm64-ucrt
        contribdir=contrib/contrib-winarm64-ucrt
        prefix=contrib/${triplet}ucrt
        archflags='-u -S 0x0A000006' ;;
    *)
        echo "Unknown ARCH $arch (x86_64 or aarch64)" >&2
        exit 1 ;;
esac

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
    # SYMBOLS=<dir>: first keep the function addresses of each binary there
    # (<path>.map: sorted, demangled), so that the offsets in a crash dump of
    # the stripped package can still be named
    if [ -n "${SYMBOLS:-}" ]; then
        (cd /build/package && find . -type f \( -name '*.dll' -o -name '*.exe' \)) | while read -r f; do
            mkdir -p "$SYMBOLS/$(dirname "$f")"
            $triplet-nm -C -n --defined-only "/build/package/$f" 2>/dev/null \
                | grep -E '^[0-9a-f]+ [Tt] ' > "$SYMBOLS/$f.map" || true
        done
    fi
    find /build/package -type f \( -name '*.dll' -o -name '*.exe' \)         -exec $triplet-strip --strip-unneeded {} +
}

# The program that applies an update once VLC has quit (see
# extras/juxtopposed/updater); no PE time stamp, so an update leaves it alone
# when it didn't change. Its version resource is filled by VLC's
# config.status, with the version of the other binaries.
build_updater() {
    (cd $builddir && ./config.status \
        --file=bin/vlc-redesign-update.rc:/src/extras/juxtopposed/updater/vlc-redesign-update.rc.in)
    $triplet-windres -i $builddir/bin/vlc-redesign-update.rc -o $builddir/bin/vlc-redesign-update-rc.o
    $triplet-gcc -O2 -municode -mwindows -static -Wl,--no-insert-timestamp \
        -o /build/package/vlc-redesign-update.exe \
        /src/extras/juxtopposed/updater/vlc-redesign-update.c $builddir/bin/vlc-redesign-update-rc.o \
        -ladvapi32 -lshell32
}

# The prebuilt libupnp never returns from its HTTP requests (pupnp renames
# the MinGW string functions into ones calling themselves, see
# contrib/src/upnp): build it from source with the fix, once per contrib
# prefix (a prebuilt archive brings the old one back), and link the UPnP
# plugin again.
upnp_from_source() {
    stamp=$prefix/.upnp-from-source
    sum=$(cat contrib/src/upnp/rules.mak contrib/src/upnp/*.patch | sha1sum | cut -d' ' -f1)
    [ "$(cat "$stamp" 2>/dev/null)" = "$sum" ] && return 1
    # (set -e does not apply in a function called as a condition)
    (cd $contribdir && rm -f .upnp && make .upnp) || exit 1
    echo "$sum" > "$stamp"
    rm -f $builddir/modules/libupnp_plugin.la
}

# The copy has no .git: src/Makefile then reads the version from revision.txt.
git -C /src -c safe.directory='*' describe --tags --long --match '?.*.*' --always \
    > /build/revision.txt
cmp -s /build/revision.txt src/revision.txt || cp /build/revision.txt src/revision.txt

archive=${CONTRIB_ARCHIVE:?prebuilt contrib archive of extras/ci/get-contrib-sha.sh}
if [ ! -s "$archive" ]; then
    echo "Missing $archive" >&2
    exit 1
fi
mkdir -p $contribdir
latest=$contribdir/vlc-contrib-$triplet-latest.tar.zst
if ! cmp -s "$archive" "$latest"; then
    cp "$archive" "$latest"
    rm -rf $prefix
fi

# The catalogs (po/*.gmo) are made again only when vlc.pot changes: a
# translation changed alone kept its previous catalog. Always made again
# (seconds).
rm -f po/*.gmo po/stamp-po $builddir/po/*.gmo $builddir/po/stamp-po

# A package that lacks a part (an install that stopped half way) never
# passes: the programs, the core, the translations, the Lua scripts and the
# plugins every copy needs, and most of the others
check_package() {
    for f in vlc.exe libvlc.dll libvlccore.dll vlc-cache-gen.exe vlc-preparser.exe \
             plugins/gui/libqt_plugin.dll plugins/codec/libavcodec_plugin.dll \
             plugins/demux/libmkv_plugin.dll plugins/video_output/libdirect3d11_plugin.dll \
             plugins/audio_output/libmmdevice_plugin.dll plugins/access/libfilesystem_plugin.dll \
             plugins/lua/liblua_plugin.dll; do
        test -s "/build/package/$f" || { echo "Not in the package: $f" >&2; return 1; }
    done
    plugins=$(find /build/package/plugins -name '*.dll' | wc -l)
    [ "$plugins" -ge 300 ] || { echo "Only $plugins plugins in the package" >&2; return 1; }
    translations=$(find /build/package/locale -name '*.mo' 2>/dev/null | wc -l)
    [ "$translations" -ge 20 ] || { echo "Only $translations translations in the package" >&2; return 1; }
    test -d /build/package/lua/intf || { echo "No Lua scripts in the package" >&2; return 1; }
}

# QUICK=1: the tree is configured already, only rebuild what changed. A full
# build.sh run bootstraps and configures again, which takes minutes. A new
# contrib archive removed the contribs above: only a full run unpacks them
# (a quick one went on without them and configure found no Lua).
if [ "${QUICK:-}" = 1 ] && [ -f $builddir/Makefile ] && [ -d $prefix/include ]; then
    export PATH="/build/vlc/extras/tools/build/bin:$PATH"
    upnp_from_source || true
    make -C $builddir -j"$jobs"
    make -C $builddir package-win-common
    check_package
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
# as -static-libgcc does; this -L comes before the contrib one. (GCC only:
# llvm-mingw has no libgcc and links its own unwinder statically, build.sh.)
if [ "$arch" = x86_64 ]; then
    mkdir -p /build/static-libgcc
    cp "$($triplet-gcc -print-file-name=libgcc_eh.a)" /build/static-libgcc/libgcc_s.a
    export LDFLAGS="${LDFLAGS:-} -static-libgcc -L/build/static-libgcc"
fi
export JOBS="$jobs"
rm -rf /build/package
# Fail instead of silently rebuilding every contrib from source.
sed 's/|| PREBUILT_FAILED=yes/|| exit 1/' extras/package/win32/build.sh \
    > extras/package/win32/build-prebuilt-only.sh
# -l: with the translations (--enable-nls), or the language chosen in the
# settings does nothing (issue #5); a tree configured without them needs a
# build without -Quick once.
# With -o and no installer (-i) build.sh only installs the package
# (package-win-common): any failure is a real one. Release mode (-r) also
# asks for upstream's zip, 7z and NSIS installer (INSTALLER=r), which make
# package-win32 look for a vlc-<version> folder that -o never writes, and the
# build failed at its very end: an -i that names none of them comes after it.
release_only=
for a in "$@"; do
    case $a in
        -r) release_only='-i none' ;;
        -i*) release_only=; break ;;
    esac
done
status=0
# shellcheck disable=SC2086 # (archflags, release_only: several options)
sh extras/package/win32/build-prebuilt-only.sh -a $arch $archflags -p -l -o /build/package "$@" $release_only || status=$?
rm -f extras/package/win32/build-prebuilt-only.sh
if [ "$status" -ne 0 ]; then
    echo "build.sh failed ($status)" >&2
    exit "$status"
fi

check_package
if upnp_from_source; then
    make -C $builddir -j"$jobs"
    make -C $builddir package-win-common
fi
build_updater
strip_package
# The host mount refuses owner/group/permission and directory time changes.
# portable/ holds the test profile (VLC uses it instead of %APPDATA%\vlc).
# *.old-* are DLLs win64-docker.ps1 moved aside because a dead VLC maps them.
rsync -rt --omit-dir-times --delete --checksum --exclude=/portable --exclude='*.old-*' \
    /build/package/ /out/
echo "Package ready: $(du -sh /out | cut -f1)"
