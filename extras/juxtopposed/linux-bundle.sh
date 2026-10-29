#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# A self-contained tarball of the Linux build (linux-qt68.sh) for test PCs
# without the Docker image: VLC installed in a folder, Qt 6.8.3 (libraries,
# plugins, QML modules), medialibrary, and the libraries of Ubuntu 24.04 it
# needs, except those that belong to the PC (C library, OpenGL/EGL/GBM/DRM,
# VA-API/VDPAU drivers, udev/systemd). Run inside the image of
# linux-qt68.Dockerfile:
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

# The libraries of the system they need, but those of the PC. libwayland-*
# too: the PC's Mesa needs its own (Debian 13's EGL wants a symbol of
# libwayland 1.23 the bundled 1.22 lacked, and VLC quit on Wayland)
mkdir -p "$root/syslib"
keep='^(libc|libm|libpthread|libdl|librt|ld-linux.*|libGL|libGLX.*|libGLdispatch|libEGL.*|libOpenGL|libgbm|libdrm.*|libva.*|libwayland-.*|libgallium.*|libudev|libsystemd|libstdc\+\+|libgcc_s)\.so'
export LD_LIBRARY_PATH="$root/lib:$root/lib/vlc:$root/qt/lib:/work/build-m0/deps/lib"
find "$root" -type f \( -name '*.so*' -o -perm -u+x \) -print | while read -r f; do
    file -b "$f" | grep -q '^ELF' || continue
    ldd "$f" 2>/dev/null | sed -n 's/^\s*\(\S*\) => \(\/\S*\) .*/\1 \2/p'
done | sort -u | while read -r name path; do
    case "$path" in "$root"/*) continue;; esac
    echo "$name" | grep -Eq "$keep" && continue
    [ -e "$root/syslib/$name" ] || cp -L "$path" "$root/syslib/$name"
done

# The newest glibc it needs (the build's distribution): an older system gets
# a message instead of the loader's "GLIBC_2.38 not found"
glibc=$(find "$root" -type f \( -name '*.so*' -o -perm -u+x \) -exec objdump -p {} + 2>/dev/null \
    | grep -o 'GLIBC_2\.[0-9][0-9]*' | sort -u -V | tail -n 1 | sed 's/^GLIBC_//')
[ -n "$glibc" ] || { echo "no glibc version found in the bundle" >&2; exit 1; }
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
export LD_LIBRARY_PATH="$here/lib:$here/lib/vlc:$here/qt/lib:$here/syslib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export VLC_PLUGIN_PATH="$here/lib/vlc/plugins"
export VLC_DATA_PATH="$here/share/vlc"
export VLC_LIB_PATH="$here/lib/vlc"
export QT_PLUGIN_PATH="$here/qt/plugins"
export QML_IMPORT_PATH="$here/qt/qml" QML2_IMPORT_PATH="$here/qt/qml"
exec "$here/bin/vlc" "$@"
EOF
sed -i "s/@GLIBC@/$glibc/" "$root/vlc"
chmod +x "$root/vlc"
"$root/lib/vlc/vlc-cache-gen" "$root/lib/vlc/plugins" 2>/dev/null || true

tar -C /tmp/bundle -cJf "$out/vlc-redesign-linux-$arch.tar.xz" vlc-redesign
du -sh "$root" "$out/vlc-redesign-linux-$arch.tar.xz"
