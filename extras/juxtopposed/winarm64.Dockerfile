# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# Official Windows ARM64 image of extras/ci/gitlab-ci.yml (llvm-mingw with
# UCRT, the image of the win64-arm-llvm job), plus rsync for incremental
# source syncs in win64-docker.sh. Used by win64-docker.ps1 -Arch aarch64.
FROM registry.videolan.org/vlc-debian-llvm-ucrt:20260611225331
USER root
RUN apt-get update \
    && apt-get install -y --no-install-recommends rsync \
    && rm -rf /var/lib/apt/lists/*
USER videolan
