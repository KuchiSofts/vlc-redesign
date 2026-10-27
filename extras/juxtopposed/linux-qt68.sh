#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# Linux build against Qt 6.8.3; run inside the image from linux-qt68.Dockerfile
# after linux.sh has built build-m0/deps (medialibrary, wayland-scanner).
set -eu
cd /work
jobs=${JOBS:-$(nproc)}
export PKG_CONFIG_PATH="/work/build-m0/deps/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export LD_LIBRARY_PATH="/work/build-m0/deps/lib:${LD_LIBRARY_PATH:-}"
export PATH="/work/build-m0/deps/bin:$PATH"
mkdir -p build-qt68/linux build-qt68/logs
cd build-qt68/linux
# With the translations (as the Windows build's -l): the interface
# language chosen in the settings needs them
if [ ! -f Makefile ] || grep -q -- "--disable-nls" config.log; then
    /work/configure --enable-nls --disable-skins2 --disable-rust --enable-wayland \
        --enable-qt --enable-medialibrary QMAKE6="$QTDIR/bin/qmake6" \
        CFLAGS='-O1 -g0' CXXFLAGS='-O1 -g0' > ../logs/configure.log 2>&1
fi
grep -q '^[^#].*-DQT_HAS_X11_COMPOSITOR' modules/gui/qt/Makefile
grep -q '^[^#].*-DQT_HAS_WAYLAND_COMPOSITOR' modules/gui/qt/Makefile
make -j"$jobs" > ../logs/build.log 2>&1
echo "BUILD OK"
