#!/bin/bash
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Builds VLC Redesign for macOS on a Mac (Xcode, or a GitHub macOS runner):
# VideoLAN's prebuilt contribs of its macOS CI when they exist for this
# tree (else the contribs from source), Qt 6.8.3 (the official build, by
# aqtinstall: the contribs of macOS have no Qt), VLC with the Qt interface
# and without the macosx one, then "VLC Redesign.app" and its disk image
# vlc-redesign-X.Y.Z-macos-<arch>.dmg (docs/MASTER-PLAN.md, 13.5).
# extras/juxtopposed/macos/README.md tells why each step is what it is.
#
#   extras/juxtopposed/macos-build.sh [-a arm64|x86_64] [-j jobs] [-b dir] [-o dir] [-c] [-n] [-g]
#
#   -a  the architecture: the Mac's own (the default; no cross build yet)
#   -j  parallel jobs (the number of cores)
#   -b  the build folder (build-macos-<arch> in the source tree)
#   -o  where the disk image goes (the build folder)
#   -c  the contribs from source, even when prebuilt ones exist
#   -n  no disk image: VLC Redesign.app only
#   -g  keep the debug symbols (not stripped)
#
# Signing: ad hoc unless MACOS_SIGN_IDENTITY names a "Developer ID
# Application" identity of the keychain; then also notarized when
# NOTARY_PROFILE names a notarytool keychain profile
# (xcrun notarytool store-credentials).
# QTDIR: a Qt 6.8 for macOS to use instead of downloading one.

set -eo pipefail

die() { echo "macos-build: $*" >&2; exit 1; }
step() { printf '\n== %s\n' "$*"; }

root=$(cd "$(dirname "$0")/../.." && pwd)
arch=$(uname -m)
jobs=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
build=
out=
from_source=
make_dmg=yes
keep_symbols=
while getopts "a:j:b:o:cngh" opt; do
    case $opt in
        a) arch=$OPTARG ;;
        j) jobs=$OPTARG ;;
        b) build=$OPTARG ;;
        o) out=$OPTARG ;;
        c) from_source=yes ;;
        n) make_dmg= ;;
        g) keep_symbols=yes ;;
        *) sed -n '5,28p' "$0"; exit 1 ;;
    esac
done

case $arch in
    arm64|aarch64) arch=arm64; host_arch=aarch64 ;;
    x86_64) host_arch=x86_64 ;;
    *) die "unknown architecture $arch" ;;
esac
[ "$(uname -m)" = "$arch" ] || die "this Mac is $(uname -m): it builds its own architecture only"
[ "$(uname -s)" = Darwin ] || die "runs on macOS only"

# (found before the environment of the build, which keeps the PATH of macOS)
brew=$(command -v brew || true)
rsvg=$(command -v rsvg-convert || true)

QT_VERSION=${QT_VERSION:-6.8.3}
# Qt 6.8 needs macOS 12
MIN_MACOS=12.0
APP_NAME="VLC Redesign"
build=${build:-$root/build-macos-$arch}
mkdir -p "$build/logs"
build=$(cd "$build" && pwd)
out=${out:-$build}
mkdir -p "$out"
out=$(cd "$out" && pwd)
logs=$build/logs

# A step's output goes to its log; the end of the log is shown if it fails
run() {
    local name=$1; shift
    if ! "$@" > "$logs/$name.log" 2>&1; then
        echo "macos-build: $name failed; the end of $logs/$name.log:" >&2
        tail -n 120 "$logs/$name.log" >&2
        exit 1
    fi
}

version=$(sed -n 's/^#define VLC_REDESIGN_VERSION "\(.*\)".*$/\1/p' "$root/modules/gui/qt/util/redesign_version.hpp")
[ -n "$version" ] || die "no VLC_REDESIGN_VERSION in modules/gui/qt/util/redesign_version.hpp"
echo "VLC Redesign $version for macOS $arch, in $build"

# ---------------------------------------------------------------------------
step "Qt $QT_VERSION"
# The official build is universal (arm64 and x86_64) and made for macOS 12
if [ -n "$QTDIR" ]; then
    qtdir=$QTDIR
else
    qtdir=$build/qt/$QT_VERSION/macos
    if [ ! -x "$qtdir/bin/qmake" ]; then
        run aqt-venv python3 -m venv "$build/aqt"
        run aqt-pip "$build/aqt/bin/pip" install --quiet aqtinstall
        for base in https://download.qt.io/ https://mirrors.ocf.berkeley.edu/qt/ \
                    https://qt.mirror.constant.com/ https://mirror.netcologne.de/qtproject/; do
            rm -rf "$build/qt"
            if "$build/aqt/bin/aqt" install-qt --base "$base" mac desktop "$QT_VERSION" clang_64 \
                    -m qtshadertools -O "$build/qt" > "$logs/aqt.log" 2>&1; then
                break
            fi
            echo "  $base failed, the next mirror"
        done
    fi
fi
[ -x "$qtdir/bin/qmake" ] || die "no Qt in $qtdir (see $logs/aqt.log)"
# (configure looks for qmake6)
[ -e "$qtdir/bin/qmake6" ] || ln -sf qmake "$qtdir/bin/qmake6"
"$qtdir/bin/qmake" -query QT_VERSION

# ---------------------------------------------------------------------------
# The environment of upstream's macOS build (extras/package/macosx):
# Xcode's compilers, the SDK, the symbols missing on old macOS versions
export VLC_FORCE_KERNELVERSION=19       # the triplet of VideoLAN's prebuilt contribs
# shellcheck source=/dev/null
source "$root/extras/package/macosx/env.build.sh" "none"
HOST_ARCH=$host_arch
ACTUAL_HOST_ARCH=$(get_actual_arch "$HOST_ARCH")
SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
export SDKROOT
BUILD_TRIPLET=$(vlcGetBuildTriplet)
HOST_TRIPLET=$(vlcGetHostTriplet)
vlcSetBaseEnvironment > "$logs/environment.log"
echo "host $HOST_TRIPLET, SDK $SDKROOT, $(xcodebuild -version | head -n 1)"

# ---------------------------------------------------------------------------
step "Build tools"
cd "$root/extras/tools"
run tools-bootstrap ./bootstrap
run tools make -j"$jobs"

# ---------------------------------------------------------------------------
step "Contribs"
vlcSetSymbolEnvironment
vlcSetContribEnvironment "$MINIMAL_OSX_VERSION" > "$logs/contrib-environment.log"
contrib=$build/contrib
mkdir -p "$contrib/contrib-$HOST_TRIPLET"
cd "$contrib/contrib-$HOST_TRIPLET"
run contrib-bootstrap "$root/contrib/bootstrap" --build="$BUILD_TRIPLET" --host="$HOST_TRIPLET" --enable-sections
if [ ! -d "$contrib/$HOST_TRIPLET/lib" ]; then
    prebuilt=
    if [ -z "$from_source" ]; then
        # The archive of VideoLAN's CI is named after the last commit that
        # touched the contribs (extras/ci/get-contrib-sha.sh); the
        # redesign's own changes there have none: the last upstream one
        group=macos-$( [ "$arch" = arm64 ] && echo arm64 || echo x86_64 )
        # VLC_CONTRIB_SHA names that commit for a tree without its history
        # (a shallow clone, a snapshot)
        for sha in ${VLC_CONTRIB_SHA:-$(git -C "$root" rev-list -n 40 HEAD -- extras/tools contrib extras/ci extras/package/macosx)}; do
            url="https://artifacts.videolan.org/vlc/$group/vlc-contrib-$HOST_TRIPLET-$sha.tar.zst"
            if curl -fsIL --retry 2 -o /dev/null "$url"; then
                prebuilt=$url
                break
            fi
        done
    fi
    if [ -n "$prebuilt" ] && make prebuilt PREBUILT_URL="$prebuilt" > "$logs/contrib-prebuilt.log" 2>&1; then
        echo "prebuilt: $prebuilt"
        # The archive's pkg-config and libtool files name the SDK of the
        # machine that built it (-isysroot .../MacOSX26.2.sdk): this one's
        # instead, or the link finds no libm
        find "$contrib/$HOST_TRIPLET/lib" \( -name '*.pc' -o -name '*.la' \) -type f -exec \
            sed -E -i '' "s#/Applications/Xcode[^ ]*\.app/Contents/Developer/Platforms/MacOSX\.platform/Developer/SDKs/MacOSX[0-9.]*\.sdk#$SDKROOT#g" {} +
        run contrib-tools make -j"$jobs" tools
    else
        echo "contribs from source (no prebuilt archive for this tree): an hour or more"
        run contrib-fetch make -j"$jobs" fetch
        if ! make -j"$jobs" -k > "$logs/contrib-build.log" 2>&1; then
            run contrib-build-serial make -j1
        fi
    fi
fi
vlcUnsetContribEnvironment > /dev/null
vlcSetLibVLCEnvironment > /dev/null
if [ -z "$keep_symbols" ]; then
    # (the release: no debug symbols)
    export CFLAGS="-arch $ACTUAL_HOST_ARCH" CXXFLAGS="-arch $ACTUAL_HOST_ARCH" \
           OBJCFLAGS="-arch $ACTUAL_HOST_ARCH" OBJCXXFLAGS="-arch $ACTUAL_HOST_ARCH"
fi

# ---------------------------------------------------------------------------
step "Configure"
cd "$root"
[ -x configure ] || run bootstrap ./bootstrap
mkdir -p "$build/vlc"
cd "$build/vlc"
if [ ! -f Makefile ] || [ "$root/configure" -nt Makefile ]; then
    # upstream's options (configure.sh), then the redesign's, which win:
    # the Qt interface and not the macosx one (whose priority is higher),
    # no Sparkle (its feed is VideoLAN's: it would offer VLC), libcurl of
    # macOS for the interface's online subtitles, artwork and update check
    # (the SDK has its headers but no pkg-config file)
    # No ncurses (the terminal interface): the app has none, and the
    # prebuilt ncurses names the SDK of VideoLAN's build machine
    run configure "$root/extras/package/macosx/configure.sh" \
        --build="$BUILD_TRIPLET" --host="$HOST_TRIPLET" \
        --with-contrib="$contrib/$HOST_TRIPLET" \
        --with-macosx-sdk="$SDKROOT" --with-macosx-version-min="$MIN_MACOS" \
        --disable-debug \
        --disable-macosx --disable-sparkle --disable-skins2 --disable-ncurses \
        --enable-qt --enable-nls \
        QMAKE6="$qtdir/bin/qmake6" \
        QT_LIBCURL_CFLAGS="-I$SDKROOT/usr/include" QT_LIBCURL_LIBS="-lcurl"
fi
# (config.log: what configure decided, whatever the Makefile's spacing)
grep -q "^ENABLE_QT_TRUE=''" config.log || die "configure turned the Qt interface off (see $logs/configure.log)"
if grep -q "^ENABLE_MACOSX_UI_TRUE=''" config.log; then
    die "the macosx interface is still enabled: it would win over Qt"
fi
grep -q "^HAVE_QT_LIBCURL_TRUE=''" config.log || echo "warning: no libcurl: no online subtitles, artwork or update check"

# ---------------------------------------------------------------------------
step "Build"
run build make -j"$jobs"
dest=$build/install
rm -rf "$dest"
run install make install DESTDIR="$dest"
vlc_version=$(sed -n 's/^#define PACKAGE_VERSION "\(.*\)"/\1/p' config.h)

# ---------------------------------------------------------------------------
step "$APP_NAME.app"
app=$build/$APP_NAME.app
contents=$app/Contents
rm -rf "$app"
mkdir -p "$contents/MacOS" "$contents/Frameworks/plugins" "$contents/Resources/share"

# Where VLC looks in a bundle (src/darwin/dirs.m): its libraries and
# plugins in Frameworks, the helper programs next to the executable
# (vlc-qt-check, which the Qt interface starts first; the preparser;
# the waveform helper, player/waveform.cpp), its data in Resources/share
# (pkglibexecdir: libexec/vlc with the prefix of configure.sh)
libexec=$dest/libexec/vlc
[ -d "$libexec" ] || libexec=$dest/lib/vlc
cp "$dest/bin/vlc" "$contents/MacOS/$APP_NAME"
for helper in vlc-qt-check vlc-preparser; do
    [ -x "$libexec/$helper" ] || die "no $helper in $libexec"
    cp "$libexec/$helper" "$contents/MacOS/"
done
[ -x "$dest/bin/vlc-waveform" ] && cp "$dest/bin/vlc-waveform" "$contents/MacOS/"
cp -a "$dest"/lib/libvlc*.dylib "$contents/Frameworks/"
find "$dest/lib/vlc/plugins" -name 'lib*_plugin.dylib' -exec cp -a {} "$contents/Frameworks/plugins/" \;
[ -f "$contents/Frameworks/plugins/libqt_plugin.dylib" ] || die "the Qt interface plugin was not built"
[ ! -f "$contents/Frameworks/plugins/libmacosx_plugin.dylib" ] || die "the macosx interface was built"
[ -f "$contents/Frameworks/plugins/libmedialibrary_plugin.dylib" ]     || echo "warning: no medialibrary in the contribs: no library pages"
# (the compiled Lua scripts: where upstream's VLC.app has them)
[ -d "$libexec/lua" ] && cp -R "$libexec/lua" "$contents/Frameworks/"
cp -R "$dest/share/vlc/." "$contents/Resources/share/"
[ -d "$dest/share/locale" ] && cp -R "$dest/share/locale" "$contents/Resources/share/"
[ -d "$root/share/hrtfs" ] && cp -R "$root/share/hrtfs" "$contents/Resources/share/"
cp "$contrib/$HOST_TRIPLET"/share/java/libbluray*.jar "$contents/Frameworks/plugins/" 2>/dev/null || true

python3 "$root/extras/juxtopposed/macos/make-info-plist.py" \
    --template "$root/share/Info.plist.in" --version "$version" --vlc-version "$vlc_version" \
    --executable "$APP_NAME" --minimum-system "$MIN_MACOS" --out "$contents/Info.plist"
plutil -lint "$contents/Info.plist" > /dev/null
printf 'APPL????' > "$contents/PkgInfo"

# The icon: AppIcon.svg (the redesign's logo on its card) in every size
iconset=$build/AppIcon.iconset
rm -rf "$iconset"
mkdir -p "$iconset"
svg=$root/extras/juxtopposed/macos/AppIcon.svg
master=$build/AppIcon-1024.png
if [ -z "$rsvg" ] && [ -n "$brew" ]; then
    run librsvg "$brew" install librsvg
    rsvg=$("$brew" --prefix)/bin/rsvg-convert
fi
if [ -n "$rsvg" ] && [ -x "$rsvg" ]; then
    "$rsvg" -w 1024 -h 1024 "$svg" -o "$master"
else
    # (Quick Look draws SVG too)
    qlmanage -t -s 1024 -o "$build" "$svg" > /dev/null 2>&1
    mv "$build/AppIcon.svg.png" "$master"
fi
for size in 16 32 128 256 512; do
    sips -z $size $size "$master" --out "$iconset/icon_${size}x${size}.png" > /dev/null
    sips -z $((size * 2)) $((size * 2)) "$master" --out "$iconset/icon_${size}x${size}@2x.png" > /dev/null
done
iconutil -c icns "$iconset" -o "$contents/Resources/AppIcon.icns"

# Qt: its frameworks, plugins (cocoa, the mac style, image formats) and
# the QML modules the interface imports; qt.conf in Resources
run macdeployqt "$qtdir/bin/macdeployqt" "$app" \
    -qmldir="$root/modules/gui/qt" \
    -executable="$contents/MacOS/vlc-qt-check" \
    -executable="$contents/Frameworks/plugins/libqt_plugin.dylib" \
    -always-overwrite -verbose=1
[ -d "$contents/Frameworks/QtQuick.framework" ] || die "macdeployqt did not deploy Qt Quick (see $logs/macdeployqt.log)"
[ -f "$contents/PlugIns/platforms/libqcocoa.dylib" ] || die "no Qt cocoa platform plugin"
# The SQL drivers come along with Qt's other plugins, and the Mimer one
# links a library of the build machine (/usr/local/lib): the interface
# uses no Qt SQL (the media library is VLC's), so none of them ships
rm -rf "$contents/PlugIns/sqldrivers"

# The bundle's binaries find their libraries in it, never in the build
# folders or in Qt's download: every reference to one of them becomes
# @rpath/..., the programs search Frameworks, the plugins their parent
macho() { file -b "$1" | grep -q 'Mach-O'; }
rpaths() { otool -l "$1" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }'; }
fix_binary() {
    local f=$1 want=$2 dep r id
    [ -L "$f" ] && return 0
    chmod u+w "$f"
    # a library's own name (the first line otool -L prints for it)
    id=$(otool -D "$f" | tail -n +2)
    case $id in
        ''|@*) ;;
        *) install_name_tool -id "@rpath/$(basename "$id")" "$f" ;;
    esac
    otool -L "$f" | tail -n +2 | awk '{ print $1 }' | while read -r dep; do
        case $dep in
            "$qtdir"/lib/*.framework/*)
                install_name_tool -change "$dep" "@rpath/${dep#"$qtdir"/lib/}" "$f" ;;
            /lib/*|"$dest"/*|"$build"/*|"$contrib"/*)
                install_name_tool -change "$dep" "@rpath/$(basename "$dep")" "$f" ;;
        esac
    done
    rpaths "$f" | while read -r r; do
        case $r in @*) ;; *) install_name_tool -delete_rpath "$r" "$f" ;; esac
    done
    if [ -n "$want" ] && ! rpaths "$f" | grep -qxF "$want"; then
        install_name_tool -add_rpath "$want" "$f"
    fi
}
for f in "$contents/MacOS/"*; do
    macho "$f" && fix_binary "$f" "@executable_path/../Frameworks"
done
for f in "$contents/Frameworks/"*.dylib; do
    fix_binary "$f" ""
done
for f in "$contents/Frameworks/plugins/"*.dylib; do
    fix_binary "$f" "@loader_path/.."
done
# what still points out of the bundle
leaks=$(find "$contents" -type f \( -perm -u+x -o -name '*.dylib' \) -print0 \
        | xargs -0 otool -L 2>/dev/null | awk '/^\t/ { print $1 }' \
        | grep -E "^($build|$qtdir|$contrib|/lib/|/opt/homebrew|/usr/local)" | sort -u || true)
[ -z "$leaks" ] || die "binaries still use libraries outside the bundle: $leaks"

if [ -z "$keep_symbols" ]; then
    find "$contents/MacOS" "$contents/Frameworks" -maxdepth 2 -type f \( -perm -u+x -o -name '*.dylib' \) \
        -not -path '*.framework/*' -print0 | while IFS= read -r -d '' f; do
        macho "$f" && strip -x "$f" 2>/dev/null || true
    done
fi

# ---------------------------------------------------------------------------
step "Sign"
# Ad hoc without a Developer ID (decision 3): it runs on the Mac that made
# it and after "Open Anyway" elsewhere (README). A Developer ID signs with
# the hardened runtime and the entitlements of upstream's VLC.app.
identity=${MACOS_SIGN_IDENTITY:--}
entitlements=$root/extras/package/macosx/vlc-hardening.entitlements
sign() { codesign --force --sign "$identity" "$@"; }
sign_program() {
    if [ "$identity" = - ]; then
        sign "$1"
    else
        sign --options runtime --timestamp --entitlements "$entitlements" "$1"
    fi
}
sign_code() {
    if [ "$identity" = - ]; then sign "$1"; else sign --timestamp "$1"; fi
}
{
    for f in "$contents/Frameworks/"*.framework; do sign_code "$f"; done
    find "$contents/PlugIns" "$contents/Resources/qml" -type f -name '*.dylib' 2>/dev/null \
        | while read -r f; do sign_code "$f"; done
    for f in "$contents/Frameworks/"*.dylib "$contents/Frameworks/plugins/"*.dylib; do sign_code "$f"; done
    find "$contents/Frameworks/lua" -type f 2>/dev/null | while read -r f; do sign_code "$f"; done
} > "$logs/sign.log" 2>&1 || { tail -n 40 "$logs/sign.log"; die "signing failed"; }
# The plugin cache once the plugins are final (it notes their size and date)
if VLC_LIB_PATH="$contents/Frameworks" "$build/vlc/bin/vlc-cache-gen" "$contents/Frameworks/plugins" \
        > "$logs/cache-gen.log" 2>&1; then
    echo "plugin cache: $(du -h "$contents/Frameworks/plugins/plugins.dat" | cut -f1)"
    # a file among code: signed too, as upstream's codesign.sh does, or
    # the app's signature finds it unsigned
    sign_code "$contents/Frameworks/plugins/plugins.dat" >> "$logs/sign.log" 2>&1
else
    echo "warning: no plugin cache (made at the first start)"
    rm -f "$contents/Frameworks/plugins/plugins.dat"
fi
for f in "$contents/MacOS/"*; do
    [ "$(basename "$f")" = "$APP_NAME" ] || sign_program "$f"
done >> "$logs/sign.log" 2>&1
sign_program "$app" >> "$logs/sign.log" 2>&1 || { tail -n 40 "$logs/sign.log"; die "signing the app failed"; }
codesign --verify --deep --strict "$app"
codesign -dv "$app" 2>&1 | grep -E '^(Identifier|Format|Signature|Authority|TeamIdentifier)' || true
du -sh "$app"

[ -n "$make_dmg" ] || exit 0

# ---------------------------------------------------------------------------
step "Disk image"
dmg=$out/vlc-redesign-$version-macos-$arch.dmg
stage=$build/dmg
rm -rf "$stage" "$dmg"
mkdir -p "$stage"
cp -R "$app" "$stage/"
ln -s /Applications "$stage/Applications"
# (hdiutil is known to fail now and then on CI Macs: "Resource busy")
for attempt in 1 2 3; do
    if hdiutil create -volname "$APP_NAME" -srcfolder "$stage" -fs HFS+ -format UDZO -ov "$dmg" \
            > "$logs/hdiutil.log" 2>&1; then
        break
    fi
    [ $attempt = 3 ] && { cat "$logs/hdiutil.log"; die "hdiutil failed"; }
    sleep 5
done
rm -rf "$stage"
if [ "$identity" != - ]; then
    codesign --force --sign "$identity" --timestamp "$dmg"
    if [ -n "$NOTARY_PROFILE" ]; then
        step "Notarize"
        xcrun notarytool submit "$dmg" --keychain-profile "$NOTARY_PROFILE" --wait
        xcrun stapler staple "$dmg"
    fi
fi
(cd "$out" && shasum -a 256 "$(basename "$dmg")" | tee "$(basename "$dmg").sha256")
echo "made $dmg"
