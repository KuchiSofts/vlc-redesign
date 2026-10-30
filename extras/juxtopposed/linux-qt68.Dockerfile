# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# Linux build image with Qt 6.8.3, the same Qt version as the Windows build
# (contrib/src/main.mak). Ubuntu 24.04 ships Qt 6.4, which cannot load the
# redesign's QML (Rectangle per-corner radii need Qt 6.7). On top of the
# image of linux.Dockerfile:
#   docker build -t vlc-redesign-linux:dev -f linux.Dockerfile .
#   docker build -t vlc-redesign-linux:qt68 -f linux-qt68.Dockerfile .
FROM vlc-redesign-linux:dev

USER root
RUN apt-get update \
    && apt-get install -y --no-install-recommends python3-pip python3-venv \
       libxcb-cursor0 libxcb-cursor-dev libxkbcommon-dev libgl-dev libegl-dev \
       libfontconfig1-dev libfreetype-dev libx11-xcb-dev libxcb-icccm4 \
       libxcb-image0 libxcb-keysyms1 libxcb-render-util0 libxcb-shape0 \
       libwayland-dev x11-utils libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*
# On ARM64 (aarch64) Qt's own build is linux_gcc_arm64, in gcc_arm64: linked
# as gcc_64 so that the paths below are the same on both
RUN case "$(uname -m)" in \
         aarch64) qthost=linux_arm64 qtarch=linux_gcc_arm64 qtdir=gcc_arm64 ;; \
         *) qthost=linux qtarch=linux_gcc_64 qtdir=gcc_64 ;; \
       esac \
    && python3 -m venv /opt/aqt \
    && /opt/aqt/bin/pip install --no-cache-dir aqtinstall \
    && for base in https://download.qt.io/ https://mirrors.ocf.berkeley.edu/qt/ \
                   https://qt.mirror.constant.com/ https://mirror.netcologne.de/qtproject/; do \
         rm -rf /opt/qt; \
         /opt/aqt/bin/aqt install-qt --base "$base" "$qthost" desktop 6.8.3 "$qtarch" \
           -m qtshadertools qtwaylandcompositor -O /opt/qt && break; \
       done \
    && { [ "$qtdir" = gcc_64 ] || ln -s "$qtdir" /opt/qt/6.8.3/gcc_64; } \
    && test -x /opt/qt/6.8.3/gcc_64/bin/qmake \
    && ln -sf qmake /opt/qt/6.8.3/gcc_64/bin/qmake6 \
    && rm -rf /root/.cache

ENV QTDIR=/opt/qt/6.8.3/gcc_64
ENV PATH=/opt/qt/6.8.3/gcc_64/bin:$PATH
ENV PKG_CONFIG_PATH=/opt/qt/6.8.3/gcc_64/lib/pkgconfig
ENV LD_LIBRARY_PATH=/opt/qt/6.8.3/gcc_64/lib
ENV QT_PLUGIN_PATH=/opt/qt/6.8.3/gcc_64/plugins
ENV QML_IMPORT_PATH=/opt/qt/6.8.3/gcc_64/qml
ENV QML2_IMPORT_PATH=/opt/qt/6.8.3/gcc_64/qml
USER ubuntu
WORKDIR /work
