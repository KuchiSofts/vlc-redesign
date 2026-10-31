#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# A self-contained tarball of the Linux build (linux-qt68.sh) for test PCs
# without the Docker image: VLC installed in a folder, Qt 6.8.3 (libraries,
# plugins, QML modules), medialibrary, and the libraries of Ubuntu 24.04 it
# needs, except those that belong to the PC (C library, OpenGL/EGL/GBM/DRM,
# X11, D-Bus, ALSA, fontconfig, udev/systemd: the list below), each found by
# its RUNPATH. Run inside the image of linux-qt68.Dockerfile:
#   docker run --rm -v "$PWD:/work" vlc-redesign-linux:qt68 sh extras/juxtopposed/linux-bundle.sh
# Writes build-qt68/vlc-redesign-linux-ARCH.tar.xz (x86_64 or aarch64, the
# machine's); on the PC:
#   tar xf vlc-redesign-linux-x86_64.tar.xz && vlc-redesign/vlc [options]
# linux-packages.sh makes the AppImage, .deb, .rpm and Flatpak manifest from it.
set -eu
build=/work/build-qt68/linux
out=/work/build-qt68
arch=$(uname -m)
root=/tmp/bundle/vlc-redesign
rm -rf /tmp/bundle /tmp/vlcinst
mkdir -p "$root"

make -C "$build" install DESTDIR=/tmp/vlcinst > "$out/logs/bundle-install.log" 2>&1
cp -a /tmp/vlcinst/usr/local/. "$root/"
# (medialibrary is static, in its plugin)

mkdir -p "$root/qt/lib"
cp -a "$QTDIR"/lib/libQt6*.so* "$QTDIR"/lib/libicu*.so* "$root/qt/lib/" 2>/dev/null || true
cp -a "$QTDIR/plugins" "$QTDIR/qml" "$root/qt/"
# (no debug or design-time files)
find "$root/qt" \( -name '*.debug' -o -name '*.qmltypes' -o -name 'designer' \) -prune -exec rm -rf {} +
# (nor the SQL drivers for database servers: their client libraries are not
# there, and VLC uses none)
find "$root/qt/plugins/sqldrivers" -name 'libqsql*.so' ! -name libqsqlite.so -delete

# The libraries of the system they need, but those of the PC (in the spirit
# of the AppImage project's excludelist), which every system with glibc 2.38
# and a desktop has:
# - the C library (libresolv too: it needs the GLIBC_PRIVATE symbols of the
#   PC's libc), libstdc++, libgcc_s;
# - OpenGL/EGL/GBM/DRM and libwayland-* (the PC's Mesa needs its own: Debian
#   13's EGL wants a symbol of libwayland 1.23 the bundled 1.22 lacked, and
#   VLC quit on Wayland), and for the same reason the X libraries Mesa uses
#   (libX11, libxcb and its DRI, GLX, Present, Sync, XFixes, RandR and SHM
#   extensions, libxshmfence), libz, libzstd, libexpat;
# - libdbus-1: its private symbols tie it to the PC's D-Bus tools (a bundled
#   copy made dbus-launch fail: "version LIBDBUS_PRIVATE_1.16.2 not found");
# - libasound, libfontconfig, libfreetype: they load the PC's plugins and
#   configuration (PipeWire or Pulse for ALSA, fonts.conf), where a copy of
#   Ubuntu 24.04 looks in its own paths (without ALSA, only VLC's ALSA, sndio
#   and MIDI plugins don't load);
# - udev, systemd.
system='^(ld-linux.*|libc|libm|libmvec|libpthread|libdl|librt|libresolv|libanl|libutil|libthread_db|libBrokenLocale|libnss_.*|libstdc\+\+|libgcc_s|libGL|libGLX.*|libGLdispatch|libEGL.*|libOpenGL|libgbm|libdrm.*|libgallium.*|libwayland-.*|libX11|libX11-xcb|libXau|libXdmcp|libxcb|libxcb-(dri2|dri3|glx|present|randr|shm|sync|xfixes)|libxshmfence|libz|libzstd|libexpat|libdbus-1|libasound|libfontconfig|libfreetype|libudev|libsystemd)\.so'
# The loaders of the GPU's drivers (VA-API, VDPAU, OpenCL) are the PC's too:
# they find its drivers, in its paths. But FFmpeg's libavutil needs them, and
# without them (no libva2 on a fresh Ubuntu) the avcodec plugin did not load:
# no video. Copies in syslib/fallback/NAME/, which the launcher adds only for
# those the PC lacks (no driver then: decoding on the CPU)
loaders='^(libva|libva-drm|libva-x11|libva-wayland|libvdpau|libOpenCL)\.so'
mkdir -p "$root/syslib"
export LD_LIBRARY_PATH="$root/lib:$root/lib/vlc:$root/qt/lib:/work/build-m0/deps/lib"
find "$root" -type f \( -name '*.so*' -o -perm -u+x \) -print | while read -r f; do
    file -b "$f" | grep -q '^ELF' || continue
    ldd "$f" 2>/dev/null | sed -n 's/^\s*\(\S*\) => \(\/\S*\) .*/\1 \2/p'
done | sort -u | while read -r name path; do
    case "$path" in "$root"/*) continue;; esac
    echo "$name" | grep -Eq "$system" && continue
    if echo "$name" | grep -Eq "$loaders"; then
        mkdir -p "$root/syslib/fallback/$name"
        [ -e "$root/syslib/fallback/$name/$name" ] || cp -L "$path" "$root/syslib/fallback/$name/$name"
        continue
    fi
    [ -e "$root/syslib/$name" ] || cp -L "$path" "$root/syslib/$name"
done

# Every program and library finds the others by a search path of its own
# (RUNPATH, relative to its folder), not by an LD_LIBRARY_PATH of the
# launcher: every program VLC starts (dbus-launch, xdg-open, the browser, the
# file manager) inherited it and loaded these libraries instead of its own.
# (VLC's own helpers, vlc-qt-check, vlc-cache-gen and vlc-waveform, find them
# the same way.)
find "$root" -type f \( -name '*.so*' -o -perm -u+x \) -print | while read -r f; do
    file -b "$f" | grep -q '^ELF' || continue
    readelf -d "$f" 2>/dev/null | grep -q '(NEEDED)' || continue
    d=$(dirname "$f")
    rpath=
    for dir in lib lib/vlc qt/lib syslib; do
        rel=$(realpath -m --relative-to="$d" "$root/$dir")
        [ "$rel" = . ] && p='$ORIGIN' || p="\$ORIGIN/$rel"
        rpath="$rpath${rpath:+:}$p"
    done
    patchelf --set-rpath "$rpath" "$f"
done
# and Qt its plugins and QML modules by qt.conf, next to the programs that
# start it (QT_PLUGIN_PATH made Qt programs opened from VLC, a KDE file
# manager, load the plugins of this Qt 6.8)
for f in "$root/bin/vlc" "$root"/libexec/vlc/vlc-qt-check; do
    [ -x "$f" ] || continue
    d=$(dirname "$f")
    printf '[Paths]\nPrefix = %s\n' "$(realpath -m --relative-to="$d" "$root/qt")" > "$d/qt.conf"
done
# Checked without the build's paths: each library found from the bundle or
# the image (Qt's plugins for what VLC doesn't use may name absent libraries)
missing=$(find "$root" -path "$root/qt/plugins" -prune -o -path "$root/qt/qml" -prune -o \
        -type f \( -name '*.so*' -o -perm -u+x \) -print | while read -r f; do
    file -b "$f" | grep -q '^ELF' || continue
    env -u LD_LIBRARY_PATH ldd "$f" 2>/dev/null | grep 'not found' | sed "s|^|$f: |"
done)
[ -z "$missing" ] || { echo "libraries not found by their RUNPATH:" >&2; echo "$missing" >&2; exit 1; }

# The newest glibc it needs (the build's distribution): an older system gets
# a message instead of the loader's "GLIBC_2.38 not found"
glibc=$(find "$root" -type f \( -name '*.so*' -o -perm -u+x \) -exec objdump -p {} + 2>/dev/null \
    | grep -o 'GLIBC_2\.[0-9][0-9]*' | sort -u -V | tail -n 1 | sed 's/^GLIBC_//')
[ -n "$glibc" ] || { echo "no glibc version found in the bundle" >&2; exit 1; }
# The PC's libraries without which VLC can't play (libVLC, the avcodec
# plugin) or show its interface (Qt on X11 and Wayland, EGL), with the package
# of Ubuntu and Debian that has each: the launcher names those missing instead
# of the loader's "cannot open shared object file"
needs() { # FILE...
    for f in "$@"; do env -u LD_LIBRARY_PATH ldd "$f" 2>/dev/null; done \
        | sed -n 's/^\s*\(\S*\) => \(\/\S*\) .*/\1 \2/p' | sort -u | while read -r name path; do
        echo "$name" | grep -Eq "$system" || continue
        echo "$name" | grep -Eq '^(ld-linux|libc|libm|libmvec|libpthread|libdl|librt|libresolv|libstdc\+\+|libgcc_s)\.so' && continue
        pkg=$(dpkg -S "*/$(basename "$(readlink -f "$path")")" 2>/dev/null | sed -n '1s/[:,].*//p')
        printf '%s=%s ' "$name" "${pkg:-?}"
    done
}
core_libs=$(needs "$root"/lib/libvlc.so.[0-9]* "$root/lib/vlc/plugins/codec/libavcodec_plugin.so")
# (EGL is loaded by Qt's integration plugins, which no ldd shows)
gui_libs="libEGL.so.1=libegl1 $(needs "$root/lib/vlc/plugins/gui/libqt_plugin.so" \
    "$root"/qt/plugins/platforms/libqxcb.so "$root"/qt/plugins/platforms/libqwayland*.so)"
gui_libs=$(for l in $gui_libs; do case " $core_libs " in *" $l "*) ;; *) echo "$l" ;; esac; done | sort -u | tr '\n' ' ')
echo "From the system, to play: $core_libs"
echo "and for the interface: $gui_libs"
cat > "$root/vlc" <<'EOF'
#!/bin/sh
# VLC Redesign from this folder
here=$(cd "$(dirname "$0")" && pwd)
need=@GLIBC@
have=$(getconf GNU_LIBC_VERSION 2>/dev/null | sed -n 's/^glibc //p')
if [ -n "$have" ] && [ "$(printf '%s\n%s\n' "$need" "$have" | sort -V | head -n 1)" != "$need" ]; then
    msg="VLC Redesign needs glibc $need or later (Ubuntu 24.04, Debian 13, Fedora 39 or later). This system has glibc $have."
    echo "$msg" >&2
    if [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]; then
        if command -v zenity > /dev/null; then zenity --error --title="VLC Redesign" --text="$msg" 2> /dev/null
        elif command -v kdialog > /dev/null; then kdialog --title "VLC Redesign" --error "$msg" 2> /dev/null
        fi
    fi
    exit 1
fi
# The libraries of this system: the loader's cache (ldconfig, in /sbin for
# the users of Debian), or the usual folders
case $(uname -m) in
    x86_64) abi=x86-64 multiarch=x86_64-linux-gnu ;;
    aarch64) abi=AArch64 multiarch=aarch64-linux-gnu ;;
    *) abi=none multiarch=none ;;
esac
ldc=
for c in ldconfig /sbin/ldconfig /usr/sbin/ldconfig; do
    command -v "$c" > /dev/null 2>&1 && { ldc=$c; break; }
done
cache=
[ -n "$ldc" ] && cache=$("$ldc" -p 2> /dev/null)
has() { # LIBRARY.so.N
    if [ -n "$cache" ]; then
        # (not grep -q: it stops reading, and where SIGPIPE is ignored, in
        # the Flatpak, printf said "write error: Broken pipe")
        printf '%s\n' "$cache" | grep "^[[:space:]]*$1 (libc6,$abi[,)]" > /dev/null
    else
        for d in /usr/lib/$multiarch /lib/$multiarch /usr/lib64 /lib64 /usr/lib /lib; do
            [ -e "$d/$1" ] && return 0
        done
        return 1
    fi
}
# Those it needs to play and, for the interface, to draw (they come with the
# display driver and the desktop): without them it fell back to the terminal
# and said "Error opening terminal" (a server without a desktop), or the
# avcodec plugin did not load
gui=
[ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ] && gui=1
for a in "$@"; do case $a in -I|--intf|--intf=*|-Idummy|-Irc|-Icli) gui= ;; esac; done
missing= packages=
for l in @CORE_LIBS@ ${gui:+@GUI_LIBS@}; do
    has "${l%%=*}" && continue
    missing="$missing ${l%%=*}"
    case " $packages " in *" ${l#*=} "*) ;; *) packages="$packages ${l#*=}" ;; esac
done
if [ -n "$missing" ]; then
    msg="VLC Redesign needs libraries of this system that are not installed:$missing. On Ubuntu or Debian, install:$packages (on Fedora or Arch, the packages that have these files)."
    echo "$msg" >&2
    if [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]; then
        if command -v zenity > /dev/null; then zenity --error --title="VLC Redesign" --text="$msg" 2> /dev/null
        elif command -v kdialog > /dev/null; then kdialog --title "VLC Redesign" --error "$msg" 2> /dev/null
        fi
    fi
    exit 1
fi
# The libraries are found by their own search paths (RUNPATH) and Qt's
# plugins by bin/qt.conf: no LD_LIBRARY_PATH, QT_PLUGIN_PATH or QML paths
# here, which every program VLC starts would inherit (dbus-launch failed with
# the bundled libdbus). Only the loaders of VA-API, VDPAU and OpenCL this
# system lacks come from syslib/fallback (no driver to load: decoding on the
# CPU); harmless to the others, which can't have them either.
fallback=
for d in "$here"/syslib/fallback/*/; do
    [ -d "$d" ] || continue
    d=${d%/}
    has "${d##*/}" || fallback="$fallback${fallback:+:}$d"
done
[ -n "$fallback" ] && export LD_LIBRARY_PATH="$fallback${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$here/bin/vlc" "$@"
EOF
sed -i -e "s/@GLIBC@/$glibc/" -e "s/@CORE_LIBS@/$core_libs/" -e "s/@GUI_LIBS@/$gui_libs/" "$root/vlc"
grep -q '@[A-Z_]*@' "$root/vlc" && { echo "the launcher has a placeholder left" >&2; exit 1; }
chmod +x "$root/vlc"
"$root/libexec/vlc/vlc-cache-gen" "$root/lib/vlc/plugins"

tar -C /tmp/bundle -cJf "$out/vlc-redesign-linux-$arch.tar.xz" vlc-redesign
du -sh "$root" "$out/vlc-redesign-linux-$arch.tar.xz"
