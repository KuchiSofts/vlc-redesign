#!/bin/sh
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# The Linux packages of VLC Redesign, made from the bundle of linux-bundle.sh:
#   vlc-redesign-X.Y.Z-ARCH.AppImage (and .zsync: update information for the
#       GitHub releases, gh-releases-zsync)
#   vlc-redesign_X.Y.Z_DEBARCH.deb, vlc-redesign-X.Y.Z-1.ARCH.rpm: VLC in
#       /opt/vlc-redesign and the command vlc-redesign, so nothing of the
#       distribution's vlc package is replaced and both can be installed
#   vlc-redesign-X.Y.Z-linux-ARCH.tar.xz: the bundle itself
#   flatpak/org.kuchisofts.VLCRedesign.yml: a Flatpak manifest (KDE 6.8
#       runtime) with its desktop entry, metainfo and icon, written only:
#       flatpak-builder builds it from the source
#   SHA256SUMS-linux-ARCH.txt
# Run inside the image of linux-qt68.Dockerfile, after linux-bundle.sh:
#   docker run --rm -v "$PWD:/work" vlc-redesign-linux:qt68 sh extras/juxtopposed/linux-packages.sh
# Options:
#   --arch x86_64|aarch64  the bundle's architecture (default: this machine's)
#   --bundle FILE          default: build-qt68/vlc-redesign-linux-ARCH.tar.xz
#   --out DIR              default: build-qt68/packages
# The version is the one of modules/gui/qt/util/redesign_version.hpp. The
# tools (appimagetool and the AppImage runtime, nfpm for the .deb and .rpm)
# are downloaded once into build-qt68/tools (TOOLS=) and checked by SHA-256;
# the packages are put together in /tmp/vlc-redesign-pkg (STAGE=), where
# file modes and symbolic links are kept.
set -eu
src=$(cd "$(dirname "$0")/../.." && pwd)
arch=$(uname -m)
bundle=
out=
while [ $# -gt 0 ]; do
    case "$1" in
        --arch) arch=$2; shift 2 ;;
        --bundle) bundle=$2; shift 2 ;;
        --out) out=$2; shift 2 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done
case "$arch" in
    x86_64) deb_arch=amd64 elf_machine='x86-64' ;;
    aarch64) deb_arch=arm64 elf_machine='ARM aarch64' ;;
    *) echo "--arch: x86_64 or aarch64, not $arch" >&2; exit 2 ;;
esac
host=$(uname -m)
case "$host" in
    x86_64) nfpm_host=x86_64 ;;
    aarch64) nfpm_host=arm64 ;;
    *) echo "the tools run on x86_64 or aarch64, not $host" >&2; exit 2 ;;
esac
bundle=${bundle:-$src/build-qt68/vlc-redesign-linux-$arch.tar.xz}
out=${out:-$src/build-qt68/packages}
tools=${TOOLS:-$src/build-qt68/tools}
stage=${STAGE:-/tmp/vlc-redesign-pkg}

hpp="$src/modules/gui/qt/util/redesign_version.hpp"
version=$(sed -n 's/^#define VLC_REDESIGN_VERSION "\(.*\)"$/\1/p' "$hpp")
repo=$(sed -n 's/^#define VLC_REDESIGN_REPO "\(.*\)"$/\1/p' "$hpp")
[ -n "$version" ] && [ -n "$repo" ] || { echo "no version in $hpp" >&2; exit 1; }
owner=${repo%/*}
project=${repo#*/}
id=org.kuchisofts.VLCRedesign
maintainer='KuchiSofts <32385966+KuchiSofts@users.noreply.github.com>'
homepage="https://github.com/$repo"
# the day of the release in the metainfo
if [ -n "${SOURCE_DATE_EPOCH:-}" ]; then
    day=$(date -u -d "@$SOURCE_DATE_EPOCH" +%F)
else
    day=$(date -u +%F)
fi
[ -f "$bundle" ] || { echo "no bundle: $bundle (linux-bundle.sh makes it)" >&2; exit 1; }
echo "VLC Redesign $version, $arch, from $bundle"

# --- tools ------------------------------------------------------------------
appimagetool_version=1.9.1
runtime_version=20251108
nfpm_version=2.47.0
case "$host" in
    x86_64) appimagetool_sha=ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0
            nfpm_sha=0660ca602b2d2d2ae4781a06c692b3eeb9d437ffea05b831d76e41f4a3188783 ;;
    aarch64) appimagetool_sha=f0837e7448a0c1e4e650a93bb3e85802546e60654ef287576f46c71c126a9158
             nfpm_sha=1c0f5f2999b9a974bfb04fdb0cc3306096de530ac5dbb25d739cc5f5219c919c ;;
esac
# the runtime is the one of the packaged architecture
case "$arch" in
    x86_64) runtime_sha=2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d ;;
    aarch64) runtime_sha=00cbdfcf917cc6c0ff6d3347d59e0ca1f7f45a6df1a428a0d6d8a78664d87444 ;;
esac

fetch() { # URL SHA256 FILE
    if [ -f "$3" ] && echo "$2  $3" | sha256sum --check --status; then
        return
    fi
    curl --fail --silent --show-error --location --retry 3 --output "$3.part" "$1"
    echo "$2  $3.part" | sha256sum --check --quiet
    mv "$3.part" "$3"
}
mkdir -p "$tools" "$out"
fetch "https://github.com/AppImage/appimagetool/releases/download/$appimagetool_version/appimagetool-$host.AppImage" \
    "$appimagetool_sha" "$tools/appimagetool-$appimagetool_version-$host.AppImage"
fetch "https://github.com/AppImage/type2-runtime/releases/download/$runtime_version/runtime-$arch" \
    "$runtime_sha" "$tools/runtime-$runtime_version-$arch"
fetch "https://github.com/goreleaser/nfpm/releases/download/v$nfpm_version/nfpm_${nfpm_version}_Linux_$nfpm_host.tar.gz" \
    "$nfpm_sha" "$tools/nfpm-$nfpm_version-$nfpm_host.tar.gz"

rm -rf "$stage"
mkdir -p "$stage/tools"
# appimagetool unpacked, as FUSE is not there in a container
cp "$tools/appimagetool-$appimagetool_version-$host.AppImage" "$stage/tools/appimagetool.AppImage"
chmod +x "$stage/tools/appimagetool.AppImage"
(cd "$stage/tools" && ./appimagetool.AppImage --appimage-extract > /dev/null && mv squashfs-root appimagetool)
appimagetool="$stage/tools/appimagetool/AppRun"
tar -C "$stage/tools" -xzf "$tools/nfpm-$nfpm_version-$nfpm_host.tar.gz" nfpm
nfpm="$stage/tools/nfpm"

# --- the bundle -------------------------------------------------------------
tar -C "$stage" -xJf "$bundle"
b="$stage/vlc-redesign"
[ -x "$b/vlc" ] && [ -x "$b/bin/vlc" ] || { echo "$bundle is not a bundle of linux-bundle.sh" >&2; exit 1; }
file -bL "$b/bin/vlc" | grep -q "$elf_machine" \
    || { echo "$bundle is not $arch: $(file -bL "$b/bin/vlc")" >&2; exit 1; }
# Qt's SQL drivers for database servers (MySQL, PostgreSQL, ODBC, Mimer)
# need their client libraries and VLC uses none of them (as linux-bundle.sh
# now leaves them out)
find "$b/qt/plugins/sqldrivers" -name 'libqsql*.so' ! -name libqsqlite.so -delete 2>/dev/null || true
# nor the files to build against libVLC (they stay in the tarball)
rm -rf "$b/include" "$b/lib/pkgconfig"
find "$b" \( -name '*.la' -o -name '*.a' \) -type f -delete

# The libraries it takes from the system (those linux-bundle.sh leaves out):
# the dependencies of the .deb and the .rpm
find "$b" -type f \( -name '*.so*' -o -perm -u+x \) > "$stage/files"
while read -r f; do
    objdump -p "$f" 2>/dev/null || true
done < "$stage/files" > "$stage/objdump"
awk '$1 == "NEEDED" { print $2 }' "$stage/objdump" | sort -u > "$stage/needed"
find "$b" -name '*.so*' \( -type f -o -type l \) -exec basename {} \; | sort -u > "$stage/provided"
comm -23 "$stage/needed" "$stage/provided" > "$stage/system"
glibc=$(grep -o 'GLIBC_2\.[0-9][0-9]*' "$stage/objdump" | sort -u -V | tail -n 1)
glibcxx=$(grep -o 'GLIBCXX_3\.4\.[0-9][0-9]*' "$stage/objdump" | sort -u -V | tail -n 1)
echo "From the system: $(tr '\n' ' ' < "$stage/system")"
echo "Needs $glibc${glibcxx:+, $glibcxx}"

debpkg() {
    case "$1" in
        libc.so.6|libm.so.6|libpthread.so.0|libdl.so.2|librt.so.1|libresolv.so.2|libutil.so.1|ld-linux-*) echo libc6 ;;
        libstdc++.so.6) echo libstdc++6 ;;
        libgcc_s.so.1) echo libgcc-s1 ;;
        libGL.so.1) echo libgl1 ;;
        libGLX.so.0) echo libglx0 ;;
        libGLdispatch.so.0) echo libglvnd0 ;;
        libOpenGL.so.0) echo libopengl0 ;;
        libEGL.so.1) echo libegl1 ;;
        libgbm.so.1) echo libgbm1 ;;
        libdrm.so.2) echo libdrm2 ;;
        libdrm_amdgpu.so.1) echo libdrm-amdgpu1 ;;
        libdrm_intel.so.1) echo libdrm-intel1 ;;
        libdrm_nouveau.so.2) echo libdrm-nouveau2 ;;
        libdrm_radeon.so.1) echo libdrm-radeon1 ;;
        libva.so.2) echo libva2 ;;
        libva-drm.so.2) echo libva-drm2 ;;
        libva-x11.so.2) echo libva-x11-2 ;;
        libva-wayland.so.2) echo libva-wayland2 ;;
        libvdpau.so.1) echo libvdpau1 ;;
        libudev.so.1) echo libudev1 ;;
        libsystemd.so.0) echo libsystemd0 ;;
        *) return 1 ;;
    esac
}
# GLIBCXX_3.4.N: the GCC whose libstdc++ has it
gcc_of() {
    n=${1##*.}
    if [ "$n" -le 29 ]; then echo 11
    elif [ "$n" -eq 30 ]; then echo 12
    elif [ "$n" -le 32 ]; then echo 13
    else echo 14
    fi
}
: > "$stage/deb-depends"
: > "$stage/rpm-depends"
while read -r so; do
    p=$(debpkg "$so") || { echo "no package known for $so: add it to debpkg()" >&2; exit 1; }
    case "$p" in
        libc6) p="libc6 (>= ${glibc#GLIBC_})" ;;
        libstdc++6) [ -n "$glibcxx" ] && p="libstdc++6 (>= $(gcc_of "$glibcxx"))" ;;
    esac
    echo "$p" >> "$stage/deb-depends"
    echo "$so()(64bit)" >> "$stage/rpm-depends"
done < "$stage/system"
echo "libc.so.6($glibc)(64bit)" >> "$stage/rpm-depends"
[ -n "$glibcxx" ] && grep -qx 'libstdc++.so.6' "$stage/system" \
    && echo "libstdc++.so.6($glibcxx)(64bit)" >> "$stage/rpm-depends"
sort -u -o "$stage/deb-depends" "$stage/deb-depends"
sort -u -o "$stage/rpm-depends" "$stage/rpm-depends"

# --- desktop entry, metainfo, icons -----------------------------------------
share="$stage/share"
mkdir -p "$share/applications" "$share/metainfo" "$share/icons/hicolor/scalable/apps"
# the MIME types of VLC's own desktop entry (share/vlc.desktop.mimetypes)
mimetypes=$(sed -n 's/^MimeType=//p' "$b/share/applications/vlc.desktop")
[ -n "$mimetypes" ] || { echo "no MimeType in the bundle's vlc.desktop" >&2; exit 1; }
cat > "$share/applications/$id.desktop" <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=VLC Redesign
GenericName=Media player
Comment=Play videos, music, discs and streams
Exec=vlc-redesign --started-from-file %U
TryExec=vlc-redesign
Icon=$id
Terminal=false
Categories=AudioVideo;Audio;Video;Player;
Keywords=Player;Video;Audio;Music;Movie;DVD;Stream;Playlist;Subtitles;
StartupWMClass=vlc
MimeType=$mimetypes
X-KDE-Protocols=ftp,http,https,mms,rtmp,rtsp,sftp,smb
EOF
cat > "$share/metainfo/$id.metainfo.xml" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<component type="desktop-application">
  <id>$id</id>
  <metadata_license>CC0-1.0</metadata_license>
  <project_license>GPL-2.0-or-later</project_license>
  <name>VLC Redesign</name>
  <summary>VLC media player with a new interface</summary>
  <developer id="io.github.kuchisofts">
    <name>KuchiSofts</name>
  </developer>
  <description>
    <p>
      An unofficial build of VLC 4 with the interface designed by Juxtopposed:
      a new library, player, queue and settings, light and dark themes and
      accent colours. It plays everything VLC plays: files, discs, network
      streams and devices, with subtitles and casting.
    </p>
    <p>
      VLC Redesign is not made by VideoLAN. It installs next to the VLC of the
      distribution and does not replace it.
    </p>
  </description>
  <launchable type="desktop-id">$id.desktop</launchable>
  <provides>
    <binary>vlc-redesign</binary>
  </provides>
  <url type="homepage">https://kuchisofts.github.io/$project/</url>
  <url type="bugtracker">$homepage/issues</url>
  <url type="vcs-browser">$homepage</url>
  <screenshots>
    <screenshot type="default">
      <image>https://raw.githubusercontent.com/$repo/main/docs/media/home.png</image>
    </screenshot>
    <screenshot>
      <image>https://raw.githubusercontent.com/$repo/main/docs/media/queue.png</image>
    </screenshot>
    <screenshot>
      <image>https://raw.githubusercontent.com/$repo/main/docs/media/settings.png</image>
    </screenshot>
  </screenshots>
  <branding>
    <color type="primary" scheme_preference="light">#f6e3cc</color>
    <color type="primary" scheme_preference="dark">#5a3410</color>
  </branding>
  <content_rating type="oars-1.1"/>
  <releases>
    <release version="$version" date="$day">
      <url type="details">$homepage/releases/tag/v$version</url>
    </release>
  </releases>
</component>
EOF
install -m 644 "$src/share/icons/scalable/vlc.svg" "$share/icons/hicolor/scalable/apps/$id.svg"
# the PNG sizes, drawn from the designer's vector by make-app-icon.py (the
# small ones hinted, so they stay sharp)
for size in 16 22 24 32 48 64 128 256 512; do
    mkdir -p "$share/icons/hicolor/${size}x$size/apps"
    install -m 644 "$src/share/icons/${size}x$size/vlc.png" "$share/icons/hicolor/${size}x$size/apps/$id.png"
done
printf '#!/bin/sh\n# VLC Redesign, installed in /opt/vlc-redesign\nexec /opt/vlc-redesign/vlc "$@"\n' > "$stage/launcher"
chmod 755 "$stage/launcher"
appimagetool_bin="$stage/tools/appimagetool/usr/bin"
"$appimagetool_bin/desktop-file-validate" "$share/applications/$id.desktop"

# --- .deb and .rpm ----------------------------------------------------------
list() { sed 's/.*/      - "&"/' "$1"; }
{
    cat <<EOF
name: vlc-redesign
arch: $deb_arch
platform: linux
version: $version
release: 1
section: video
priority: optional
maintainer: $maintainer
vendor: KuchiSofts
homepage: $homepage
license: GPL-2.0-or-later
description: |-
  VLC media player with a new interface (unofficial)
  An unofficial build of VLC 4 with the interface designed by Juxtopposed. It
  is installed in /opt/vlc-redesign with its own Qt and libraries, and started
  with vlc-redesign: the distribution's vlc package is not replaced.
contents:
  - src: $b
    dst: /opt/vlc-redesign
    type: tree
  - src: $stage/launcher
    dst: /usr/bin/vlc-redesign
    file_info:
      mode: 0755
  - src: $share/applications/$id.desktop
    dst: /usr/share/applications/$id.desktop
  - src: $share/metainfo/$id.metainfo.xml
    dst: /usr/share/metainfo/$id.metainfo.xml
  - src: $share/icons/hicolor/scalable/apps/$id.svg
    dst: /usr/share/icons/hicolor/scalable/apps/$id.svg
EOF
    for size in 16 22 24 32 48 64 128 256 512; do
        printf '  - src: %s\n    dst: %s\n' "$share/icons/hicolor/${size}x$size/apps/$id.png" \
            "/usr/share/icons/hicolor/${size}x$size/apps/$id.png"
    done
    echo 'overrides:'
    echo '  deb:'
    echo '    depends:'
    list "$stage/deb-depends"
    echo '  rpm:'
    echo '    depends:'
    list "$stage/rpm-depends"
    cat <<EOF
deb:
  compression: xz
rpm:
  summary: VLC media player with a new interface (unofficial)
  group: Applications/Multimedia
  compression: xz
EOF
} > "$stage/nfpm.yaml"
deb="$out/vlc-redesign_${version}_$deb_arch.deb"
rpm="$out/vlc-redesign-$version-1.$arch.rpm"
rm -f "$deb" "$rpm"
"$nfpm" package --config "$stage/nfpm.yaml" --packager deb --target "$deb"
"$nfpm" package --config "$stage/nfpm.yaml" --packager rpm --target "$rpm"

# --- AppImage ---------------------------------------------------------------
app="$stage/AppDir"
mkdir -p "$app"
cp -al "$b/." "$app/"
mkdir -p "$app/usr"
cp -a "$share" "$app/usr/"
mv "$app/usr/share/metainfo/$id.metainfo.xml" "$app/usr/share/metainfo/$id.appdata.xml"
cp "$share/applications/$id.desktop" "$app/"
cp "$share/icons/hicolor/256x256/apps/$id.png" "$app/$id.png"
ln -s "$id.png" "$app/.DirIcon"
cat > "$app/AppRun" <<'EOF'
#!/bin/sh
# VLC Redesign from the AppImage
here=${APPDIR:-$(dirname "$(readlink -f "$0")")}
exec "$here/vlc" "$@"
EOF
chmod 755 "$app/AppRun"
appimage="vlc-redesign-$version-$arch.AppImage"
rm -f "$out/$appimage" "$out/$appimage.zsync"
# (zsyncmake and desktop-file-validate come with appimagetool; with no
# appstreamcli the metainfo is not checked here)
cp "$tools/runtime-$runtime_version-$arch" "$stage/tools/runtime"
(cd "$out" && PATH="$appimagetool_bin:$PATH" ARCH=$arch VERSION=$version "$appimagetool" \
    --runtime-file "$stage/tools/runtime" \
    --updateinformation "gh-releases-zsync|$owner|$project|latest|vlc-redesign-*-$arch.AppImage.zsync" \
    "$app" "$appimage")
chmod 755 "$out/$appimage"

# --- tarball ----------------------------------------------------------------
tarball="vlc-redesign-$version-linux-$arch.tar.xz"
cp "$bundle" "$out/$tarball"

# --- Flatpak manifest -------------------------------------------------------
fp="$out/flatpak"
rm -rf "$fp"
mkdir -p "$fp"
sed -e 's/^Exec=.*/Exec=vlc --started-from-file %U/' -e '/^TryExec=/d' \
    "$share/applications/$id.desktop" > "$fp/$id.desktop"
cp "$share/metainfo/$id.metainfo.xml" "$fp/"
cp "$share/icons/hicolor/scalable/apps/$id.svg" "$fp/"
cat > "$fp/$id.yml" <<EOF
# Flatpak manifest of VLC Redesign $version (made by linux-packages.sh).
#   flatpak-builder --user --install-deps-from=flathub --repo=repo build $id.yml
# On the KDE 6.8 runtime (Qt 6.8, the Qt of the other builds), FFmpeg of its
# freedesktop 24.08 base (the codecs-extra extension adds the patented
# codecs). The other libraries are those of VLC's contribs.
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
cleanup:
  - /include
  - /lib/pkgconfig
  - /share/man
  - '*.la'
  - '*.a'
modules:
  - name: libebml
    buildsystem: cmake-ninja
    config-opts:
      - -DCMAKE_BUILD_TYPE=Release
      - -DBUILD_SHARED_LIBS=ON
    sources:
      - type: archive
        url: https://dl.matroska.org/downloads/libebml/libebml-1.4.6.tar.xz
        sha512: 86b469a1c1352e9ea6f581267ab8fc357575f2e410a3a732b12203575744667b152c135cf314978500d6a19fac4ef4015b89fdcfcca43f5bfc49f63e27ecbce2

  - name: libmatroska
    buildsystem: cmake-ninja
    config-opts:
      - -DCMAKE_BUILD_TYPE=Release
      - -DBUILD_SHARED_LIBS=ON
    sources:
      - type: archive
        url: https://dl.matroska.org/downloads/libmatroska/libmatroska-1.7.2.tar.xz
        sha512: 86b21a0ff3e5149a2824a9f21f9460179d02aecf477f2b9f1f92ce2f991033bd0cb209d81a4aeee2c0c5ffc193dac57a84dd4f529939b12c900f2cfb881618fb

  - name: libdvbpsi
    sources:
      - type: archive
        url: https://download.videolan.org/pub/videolan/libdvbpsi/1.3.3/libdvbpsi-1.3.3.tar.bz2
        sha512: 70f9e783a947e026ea50afe98ad2b3bfce075bd8d76932f73a6253b077a3a180aa632e9a1fff20308e2f0f3f881d727b4e4aad9b8da2b384edf9a3a0f9eacc69

  - name: taglib
    buildsystem: cmake-ninja
    config-opts:
      - -DCMAKE_BUILD_TYPE=Release
      - -DBUILD_SHARED_LIBS=ON
      - -DBUILD_BINDINGS=OFF
    sources:
      - type: archive
        url: https://github.com/taglib/taglib/releases/download/v2.3.2/taglib-2.3.2.tar.gz
        sha512: d9f68d075c24cb3535e4ae5e5b062c05d732cbcd7c8a8c0fe4935da27dc7a01e897fd3158f2059e11e46a31b18c3a9caffc66510b7459e1e8c48f29d1ae84c82

  - name: medialibrary
    buildsystem: meson
    config-opts:
      - -Dlibvlc=disabled
    sources:
      - type: archive
        url: https://code.videolan.org/videolan/medialibrary/-/archive/0.14.1/medialibrary-0.14.1.tar.bz2
        sha512: e7a67a7a2a7158e5b75517a82b8540efedb73e5d5f27b660a50d14e82e5d40ec64d9dbc90cf11dcaa7879d2c92a24b1e2007e3a8f0a9fe2f508d6826384c96cb

  - name: vlc-redesign
    buildsystem: simple
    build-commands:
      - ./bootstrap
      - mkdir -p build
      - cd build && ../configure --prefix=/app --enable-nls --disable-skins2 --disable-rust
        --enable-wayland --enable-qt --enable-medialibrary QMAKE6="\$(command -v qmake6 || command -v qmake)"
      - make -C build -j\$FLATPAK_BUILDER_N_JOBS
      - make -C build install
      - /app/lib/vlc/vlc-cache-gen /app/lib/vlc/plugins
      # VLC's own entries name the distribution's VLC
      - rm -f /app/share/applications/vlc*.desktop /app/share/metainfo/org.videolan.vlc.appdata.xml
      - install -Dm644 $id.desktop /app/share/applications/$id.desktop
      - install -Dm644 $id.metainfo.xml /app/share/metainfo/$id.metainfo.xml
      - install -Dm644 $id.svg /app/share/icons/hicolor/scalable/apps/$id.svg
    sources:
      # the release's tag; Flathub wants its commit too
      - type: git
        url: $homepage.git
        tag: v$version
      - type: file
        path: $id.desktop
      - type: file
        path: $id.metainfo.xml
      - type: file
        path: $id.svg
EOF

# --- checksums --------------------------------------------------------------
(cd "$out" && sha256sum "$appimage" "$appimage.zsync" "$(basename "$deb")" "$(basename "$rpm")" "$tarball" \
    > "SHA256SUMS-linux-$arch.txt")
rm -rf "$stage"
ls -l "$out"
