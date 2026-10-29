#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# The Flatpak of VLC Redesign as one file to install,
# vlc-redesign-X.Y.Z-ARCH.flatpak, made from the tarball of linux-packages.sh
# (the same build as the AppImage, .deb and .rpm) on the KDE 6.8 runtime.
# flatpak/org.kuchisofts.VLCRedesign.yml, the manifest that builds VLC from
# its source (for Flathub), stays as linux-packages.sh writes it.
# flatpak-builder needs its sandbox (bubblewrap): a privileged container.
#   docker run --rm --privileged -v "$PWD:/work" -v vlc-redesign-flatpak:/root/.local/share/flatpak \
#       ubuntu:24.04 sh /work/extras/juxtopposed/linux-flatpak.sh
# Options: --arch x86_64|aarch64, --out DIR (default build-qt68/packages)
set -eu
src=$(cd "$(dirname "$0")/../.." && pwd)
arch=$(uname -m)
out=
while [ $# -gt 0 ]; do
    case "$1" in
        --arch) arch=$2; shift 2 ;;
        --out) out=$2; shift 2 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done
out=${out:-$src/build-qt68/packages}
hpp="$src/modules/gui/qt/util/redesign_version.hpp"
version=$(sed -n 's/^#define VLC_REDESIGN_VERSION "\(.*\)"$/\1/p' "$hpp")
id=org.kuchisofts.VLCRedesign
tarball="$out/vlc-redesign-$version-linux-$arch.tar.xz"
fp="$out/flatpak"
[ -f "$tarball" ] && [ -f "$fp/$id.desktop" ] \
    || { echo "no $tarball or $fp/$id.desktop: run linux-packages.sh first" >&2; exit 1; }

# (librsvg: appstream compose reads the SVG icon through gdk-pixbuf)
if ! command -v flatpak-builder > /dev/null; then
    if command -v apt-get > /dev/null; then
        apt-get update -qq
        DEBIAN_FRONTEND=noninteractive apt-get install -y -qq --no-install-recommends \
            flatpak flatpak-builder ca-certificates xz-utils librsvg2-common > /dev/null
    else
        dnf install -y -q flatpak flatpak-builder xz librsvg2
    fi
fi
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

stage=${STAGE:-/tmp/vlc-redesign-flatpak}
rm -rf "$stage"
mkdir -p "$stage"
cp "$tarball" "$stage/vlc-redesign.tar.xz"
cp "$fp/$id.desktop" "$fp/$id.svg" "$stage/"
# without the screenshots: flatpak-builder's appstream compose fetches them
# with no network in its sandbox and fails ("file-read-error"); Flathub
# mirrors them from the manifest of the source build
sed '/<screenshots>/,/<\/screenshots>/d' "$fp/$id.metainfo.xml" > "$stage/$id.metainfo.xml"
cat > "$stage/$id.yml" <<EOF
# VLC Redesign $version from its Linux build (linux-flatpak.sh)
id: $id
runtime: org.kde.Platform
runtime-version: '6.8'
sdk: org.kde.Sdk
command: vlc
finish-args:
  - --share=ipc
  # streams, casting, subtitle search, updates of the lists
  - --share=network
  - --socket=wayland
  - --socket=fallback-x11
  - --socket=pulseaudio
  # video decoding and drawing on the GPU
  - --device=dri
  # the media folders; other files are opened through the portal
  - --filesystem=xdg-videos
  - --filesystem=xdg-music:ro
  - --filesystem=/media:ro
  - --filesystem=/run/media:ro
  # no screen saver while a video plays, the media keys (MPRIS)
  - --talk-name=org.freedesktop.ScreenSaver
  - --own-name=org.mpris.MediaPlayer2.vlc
modules:
  - name: vlc-redesign
    buildsystem: simple
    # (built already, without debug information)
    build-options:
      strip: false
      no-debuginfo: true
    build-commands:
      - mkdir -p /app/vlc-redesign
      - cp -a vlc-redesign/. /app/vlc-redesign/
      - rm -rf /app/vlc-redesign/include /app/vlc-redesign/lib/pkgconfig
      # VLC's own entries name the distribution's VLC
      - rm -rf /app/vlc-redesign/share/applications /app/vlc-redesign/share/metainfo
      - printf '#!/bin/sh\nexec /app/vlc-redesign/vlc "\$@"\n' > vlc && install -Dm755 vlc /app/bin/vlc
      - install -Dm644 $id.desktop /app/share/applications/$id.desktop
      - install -Dm644 $id.metainfo.xml /app/share/metainfo/$id.metainfo.xml
      - install -Dm644 $id.svg /app/share/icons/hicolor/scalable/apps/$id.svg
    sources:
      - type: file
        path: vlc-redesign.tar.xz
      - type: shell
        commands:
          - tar -xJf vlc-redesign.tar.xz && rm vlc-redesign.tar.xz
      - type: file
        path: $id.desktop
      - type: file
        path: $id.metainfo.xml
      - type: file
        path: $id.svg
EOF
cd "$stage"
flatpak-builder --user --install-deps-from=flathub --disable-rofiles-fuse --force-clean \
    --arch="$arch" --repo=repo build "$id.yml"
bundle="vlc-redesign-$version-$arch.flatpak"
flatpak build-bundle --arch="$arch" --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
    repo "$out/$bundle" "$id"
# with the checksums of the other packages
sums="$out/SHA256SUMS-linux-$arch.txt"
touch "$sums"
sed -i "/  $bundle\$/d" "$sums"
(cd "$out" && sha256sum "$bundle" >> "$sums")
rm -rf "$stage"
ls -l "$out/$bundle"
