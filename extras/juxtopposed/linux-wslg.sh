#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# Starts the Linux build (build-qt68, from linux-qt68.sh) on the Windows
# desktop through WSLg, from a Windows shell (Git Bash), in the image of
# linux-qt68.Dockerfile. The GPU of the PC is used through /dev/dxg (Mesa's
# D3D12 driver); WSLG_GPU=0 keeps Mesa's software renderer.
#   extras/juxtopposed/linux-wslg.sh [vlc options...]
# QPA=wayland runs VLC as a Wayland client of WSLg instead of X11 (XWayland).
# The profile is build-qt68/wslg-profile (PROFILE= changes it).
set -eu
export MSYS_NO_PATHCONV=1
src=$(cd "$(dirname "$0")/../.." && pwd -W 2>/dev/null || pwd)
gpu=
if [ "${WSLG_GPU:-1}" = 1 ]; then
    gpu="--device /dev/dxg -v /usr/lib/wsl:/usr/lib/wsl:ro -e GALLIUM_DRIVER=d3d12"
fi
qpa=${QPA:-xcb}
profile=${PROFILE:-/work/build-qt68/wslg-profile}
# shellcheck disable=SC2086
exec docker run --rm ${DOCKER_ARGS:-} $gpu \
    -v "$src:/work" \
    -v /run/desktop/mnt/host/wslg/.X11-unix:/tmp/.X11-unix \
    -v /run/desktop/mnt/host/wslg:/mnt/wslg \
    -e DISPLAY=:0 -e WAYLAND_DISPLAY=/mnt/wslg/runtime-dir/wayland-0 \
    -e PULSE_SERVER=unix:/mnt/wslg/PulseServer \
    -e QT_QPA_PLATFORM="$qpa" -e LANG=C.UTF-8 \
    -e XDG_CONFIG_HOME="$profile/config" -e XDG_CACHE_HOME="$profile/cache" \
    -e XDG_DATA_HOME="$profile/data" -e XDG_RUNTIME_DIR=/tmp/runtime \
    vlc-redesign-linux:qt68 sh -c '
        mkdir -p -m 700 /tmp/runtime "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME" "$XDG_DATA_HOME"
        export LD_LIBRARY_PATH="/work/build-m0/deps/lib:$LD_LIBRARY_PATH"
        [ ! -d /usr/lib/wsl/lib ] || export LD_LIBRARY_PATH="/usr/lib/wsl/lib:$LD_LIBRARY_PATH"
        cd /work/build-qt68/linux
        exec dbus-run-session -- ./vlc --no-one-instance "$@"' vlc "$@"
